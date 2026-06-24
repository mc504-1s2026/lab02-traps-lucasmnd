#ifndef __TEST_H__
#define __TEST_H__

#include <kernel/types.h>

enum ktest_result {
	KTEST_SUCCESS,
	KTEST_FAILURE,
};

struct ktest {
	void *data;
	char *name;
	enum ktest_result result;
	char *failure_file;
	u64 failure_line;
};

struct ktest_case {
	void (*run)(struct ktest *test);
	char name[64];
};

struct ktest_suite {
	const char name[64];

	int (*init)(struct ktest_suite *suite);
	int (*exit)(struct ktest_suite *suite);
	int (*test_init)(struct ktest *test);
	int (*test_exit)(struct ktest *test);

	struct ktest_case *tests;
};

#define KTEST_ASSERT(test, expr)	do {			\
	if (expr) { break; }					\
	(test)->failure_file = __FILE__;			\
	(test)->failure_line = __LINE__;			\
	(test)->result = KTEST_FAILURE;				\
	ktest_log(test, "assertion " #expr " failed (%s:%d)\n",	\
		  test->failure_file, test->failure_line);	\
} while (0)

#define KTEST_EXPECT_EQ(test, left, right)	do {			\
	if ((left) == (right)) { break; }				\
	(test)->failure_file = __FILE__;				\
	(test)->failure_line = __LINE__;				\
	(test)->result = KTEST_FAILURE;					\
	ktest_log((test), "equality assertion "				\
		  #left " == " #right " failed (%s:%d):\n",		\
		  test->failure_file, test->failure_line);		\
	ktest_log((test), "\t expected " #left " = 0x%x, got 0x%x\n",	\
		  (left), (right));					\
} while (0)

/**
 * cursed trick stolen from:
 * https://stackoverflow.com/questions/58477537/labels-redefined-in-macro-in-c
 */
#define KTEST_GENLABEL KTEST_GENLABEL1(__LINE__)
#define KTEST_GENLABEL1(suffix) KTEST_GENLABEL2(suffix)
#define KTEST_GENLABEL2(suffix) ktest_expect_fault_label_##suffix

#define KTEST_EXPECT_FAULT(test, expr) do {					\
	(test)->failure_file = __FILE__;					\
	(test)->failure_line = __LINE__;					\
	ktest_fault_expected = true;						\
	KTEST_GENLABEL: ktest_fault_return_addr = (u64)&&KTEST_GENLABEL;	\
	if (ktest_fault_expected) {						\
		expr								\
	}									\
										\
	ktest_fault_expected = false;					\
										\
	if (!ktest_fault_occurred) {						\
		ktest_log((test), "expected fault assertion failed (%s:%d)!\n",	\
			  (test)->failure_file, (test)->failure_line);		\
		(test)->result = KTEST_FAILURE;					\
	} else {								\
		ktest_fault_occurred = false;					\
	}									\
} while (0)

#define KTEST_EXPECT_NO_FAULT(test, expr) do {						\
	(test)->failure_file = __FILE__;						\
	(test)->failure_line = __LINE__;						\
	ktest_fault_expected = true;							\
	KTEST_GENLABEL: ktest_fault_return_addr = (u64)&&KTEST_GENLABEL;		\
	if (ktest_fault_expected) {							\
		expr									\
	}										\
											\
	ktest_fault_expected = false;							\
											\
	if (ktest_fault_occurred) {							\
		ktest_log((test), "expected no fault assertion failed (%s:%d)!\n",	\
			  (test)->failure_file, (test)->failure_line);			\
		(test)->result = KTEST_FAILURE;						\
		ktest_fault_occurred = false;						\
	}										\
} while (0)


#define KTEST_CASE(func) { .run = func, .name = #func }

#define KTEST_LOG_LEVEL LOG_INFO

#define ktest_log(test, fmt, ...) printk(KTEST_LOG_LEVEL, "ktest: [%s] " fmt_prefix(fmt), (test)->name, ##__VA_ARGS__)

int ktest_suite_run(struct ktest_suite *suite);

#endif
