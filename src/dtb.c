#include <stddef.h>

#include <kernel/dtb.h>
#include <kernel/printf.h>
#include <kernel/string.h>
#include <kernel/types.h>

/* this could be optmized using the rev8 instruction
 * from the Zbb extension */
u32 be_to_le(u32 be)
{
	u8 *ptr = (u8*) &be;
	u8 bytes[4] = { ptr[3], ptr[2], ptr[1], ptr[0] };
	return *(u32*)bytes;

}

extern phys_addr_t kernel_dtb;

#define DTB_HEADER_GET(dtb, field) ({ \
	void *_field_ptr = (void*)(dtb) + offsetof(struct dtb_header, field);	\
	u32 _field_be = *(u32*)_field_ptr;					\
	be_to_le(_field_be);							\
})

#define DTB_STRINGS(dtb) ((void*)(dtb) + DTB_HEADER_GET(dtb, off_dt_strings))
#define DTB_STRUCTS(dtb) ((void*)(dtb) + DTB_HEADER_GET(dtb, off_dt_struct))

void dtb_dump_header(struct dtb *dtb)
{
	struct dtb_header *header = &dtb->header;
	trace("dtb header at 0x%x\n"
	      "magic: 0x%x\n"
	      "totalsize: 0x%x\n"
	      "off_dt_struct: 0x%x\n"
	      "off_dt_strings: 0x%x\n"
	      "off_mem_rsvmap: 0x%x\n"
	      "version: 0x%x\n"
	      "last_comp_version: 0x%x\n"
	      "boot_cpuid_phys: 0x%x\n"
	      "size_dt_strings: 0x%x\n"
	      "size_dt_struct: 0x%x\n",
	      header,
	      DTB_HEADER_GET(dtb, magic),
	      DTB_HEADER_GET(dtb, totalsize),
	      DTB_HEADER_GET(dtb, off_dt_struct),
	      DTB_HEADER_GET(dtb, off_dt_strings),
	      DTB_HEADER_GET(dtb, off_mem_rsvmap),
	      DTB_HEADER_GET(dtb, version),
	      DTB_HEADER_GET(dtb, last_comp_version),
	      DTB_HEADER_GET(dtb, boot_cpuid_phys),
	      DTB_HEADER_GET(dtb, size_dt_strings),
	      DTB_HEADER_GET(dtb, size_dt_struct)
	);
}

enum dtb_struct_token {
	FDT_UNKNOWN	= 0x00000000U,
	FDT_BEGIN_NODE	= 0x00000001U,
	FDT_END_NODE	= 0x00000002U,
	FDT_PROP	= 0x00000003U,
	FDT_NOP		= 0x00000004U,
	FDT_END		= 0x00000005U,
};

char *dtb_struct_token_str(enum dtb_struct_token token)
{
	char *str;
	switch (token) {
		case FDT_BEGIN_NODE:
			str = "FDT_BEGIN_NODE";
			break;
		case FDT_END_NODE:
			str = "FDT_END_NODE";
			break;
		case FDT_PROP:
			str = "FDT_PROP";
			break;
		case FDT_NOP:
			str = "FDT_NOP";
			break;
		case FDT_END:
			str = "FDT_END";
			break;
		default:
			str = "FDT_UNKNOWN";
			break;
	}

	return str;
}


enum dtb_struct_token dtb_read_struct_token(u32 *ptr)
{
	u32 data = be_to_le(*ptr);
	enum dtb_struct_token token;
	switch (data) {
		case FDT_BEGIN_NODE:
			token = FDT_BEGIN_NODE;
			break;
		case FDT_END_NODE:
			token = FDT_END_NODE;
			break;
		case FDT_PROP:
			token = FDT_PROP;
			break;
		case FDT_NOP:
			token = FDT_NOP;
			break;
		case FDT_END:
			token = FDT_END;
			break;
		default:
			token = FDT_UNKNOWN;
			break;
	}

	trace("dtb_match_node_name: [0x%x] %s (0x%x)\n", ptr,
	      dtb_struct_token_str(token), be_to_le(*ptr));

	return token;
}

struct dtb_node_search_result dtb_match_node_name_rec(struct dtb *dtb, char *name, u32 *ptr)
{
	struct dtb_node node = { 0 };
	struct dtb_node_search_result result = { 0 };
	enum dtb_struct_token token;
	u32 len, nameoff;

	token = dtb_read_struct_token(ptr);
	if (token != FDT_BEGIN_NODE) {
		trace("dtb_match_node_name_rec: [0x%x] expected FDT_BEGIN_NODE, got %s (0x%x)!\n",
		      ptr, dtb_struct_token_str(token), be_to_le(*ptr));
		result.ptr = ptr;
		result.match = false;
		return result;
	}

	node.start = ptr;
	ptr += 1;
	node.name = (char*)ptr;
	/* the name takes up strlen(node.name) + 1 (for the null terminator) bytes, rounded
	 * up to the next 4 byte boundary */
	node.prop_start = ptr + DIV_ROUND_UP(strlen(node.name) + 1, 4);
	trace("dtb_match_node_name_rec: node \"%s\", strlen=0x%x\n", node.name, strlen(node.name));
	trace("dtb_match_node_name_rec: node.prop_start = 0x%x\n", node.prop_start);


	/* node is a match; return immediately */
	if (strcmp(node.name, name) == 0) {
		trace("dtb_match_node_name_rec: [\"%s\"] matched\n", node.name);
		result.match = true;
		result.node = node;
		result.ptr = ptr;
		return result;
	}

	/* node is not a match; keep reading */

	ptr = node.prop_start;

	/* skip properties/FDT_NOPs (if any)... */
	while (1) {
		token = dtb_read_struct_token(ptr);
		if (token == FDT_PROP) {
			ptr += 1;
			len = be_to_le(*ptr);
			ptr += 1;
			nameoff = be_to_le(*ptr);
			ptr += 1;
			ptr += DIV_ROUND_UP(len, 4);

			trace("dtb_match_node_name_rec: [0x%x] [%s] prop \"%s\", len 0x%x\n",
			      ptr, node.name, DTB_STRINGS(dtb) + nameoff, len);

			continue;
		} else if (token == FDT_NOP) {
			ptr += 1;
			continue;
		} else {
			break;
		}
	}

	trace("dtb_match_node_name_rec: [0x%x] [%s] finished parsing properties\n",
	      ptr, node.name);

	/* visit other subnodes (if any) */
	while (1) {
		token = dtb_read_struct_token(ptr);
		/* skip NOPs as usual */
		if (token == FDT_NOP) {
			ptr += 1;
		} else if (token == FDT_BEGIN_NODE) {
			result = dtb_match_node_name_rec(dtb, name, ptr);

			/* we found a match somewhere deeper in the tree; return it */
			if (result.match) {
				trace("dtb_match_node_name_rec: %s: returning match for \"%s\"\n",
				      node.name, result.node.name);
				return result;
			}

			/* otherwise keep going */
			ptr = result.ptr;
		} else {
			break;
		}
	}

	/* we should reach an FDT_END_NODE at this point */
	/* https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html#tree-structure */
	if (token == FDT_END_NODE) {
		ptr += 1;
		result.ptr = ptr;
		result.match = false;
		return result;
	} else {
		trace("dtb_match_name_node_rec: [0x%x] expected FDT_END_NODE, got %s (0x%x)\n",
		      dtb_struct_token_str(token), be_to_le(*ptr));
		result.ptr = ptr;
		result.match = false;
		return result;
	}
}

struct dtb_node dtb_match_node_name(struct dtb *dtb, char *name)
{
	struct dtb_node_search_result result = { 0 };
	u32 *ptr;

	ptr = DTB_STRUCTS(dtb);
	result =  dtb_match_node_name_rec(dtb, name, ptr);
	if (result.match) {
		return result.node;
	} else {
		return (struct dtb_node) { 0 };
	}
}

struct dtb_prop dtb_node_match_property(struct dtb *dtb, struct dtb_node *node, char *name)
{
	enum dtb_struct_token token;
	u32 *ptr = node->prop_start;
	struct dtb_prop prop;

	while (1) {
		token = dtb_read_struct_token(ptr);
		if (token == FDT_PROP) {
			ptr += 1;
			prop.len = be_to_le(*ptr);
			ptr += 1;
			prop.name = DTB_STRINGS(dtb) + be_to_le(*ptr);
			ptr += 1;
			prop.value = ptr;
			ptr += DIV_ROUND_UP(prop.len, 4);

			trace("dtb_match_node_name_rec: [0x%x] [%s] prop \"%s\", len 0x%x\n",
			      ptr, node->name, prop.name, prop.len);

			/* it's a match */
			if (strcmp(prop.name, name) == 0)
				return prop;
			continue;
		} else if (token == FDT_NOP) {
			ptr += 1;
			continue;
		} else {
			break;
		}
	}

	memset(&prop, 0, sizeof(struct dtb_prop));
	return prop;
}
