#ifndef __DTB_H__
#define __DTB_H__

#include <kernel/types.h>
#include <kernel/defs.h>

struct __packed dtb_header {
	u32 magic;
	u32 totalsize;
	u32 off_dt_struct;
	u32 off_dt_strings;
	u32 off_mem_rsvmap;
	u32 version;
	u32 last_comp_version;
	u32 boot_cpuid_phys;
	u32 size_dt_strings;
	u32 size_dt_struct;
};

struct dtb {
	struct dtb_header header;
};

struct dtb_node {
	char *name;
	u32 *start;
	u32 *end;
	u32 *prop_start;
};

struct dtb_node_search_result {
	bool match;
	struct dtb_node node;
	u32 *ptr;
};

struct dtb_prop {
	char *name;
	size_t len;
	void *value;
};

void dtb_dump_header(struct dtb *dtb);
struct dtb_node dtb_match_node_name(struct dtb *dtb, char *name);
struct dtb_prop dtb_node_match_property(struct dtb *dtb, struct dtb_node *node, char *name);

#endif
