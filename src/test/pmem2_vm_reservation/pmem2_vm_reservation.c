// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * pmem2_vm_reservation.c -- pmem2_vm_reservation unittests
 */
#include "libpmem2.h"
#include "unittest.h"
#include "ut_pmem2_utils.h"
#include "ut_pmem2_config.h"
#include "vm_reservation.h"
#include "config.h"
#include "source.h"
#include "out.h"
#include "map.h"
#include "pmem2.h"
#include "ut_pmem2.h"

/*
 * XXX: check includes and Makefile/proj!
 */

/*
 * prepare_source -- fill pmem2_source
 */
static void
prepare_source(struct pmem2_source *src, int fd)
{
#ifdef _WIN32
	src->handle = (HANDLE)_get_osfhandle(fd);
#else
	src->fd = fd;
#endif
}

/*
 * prepare_config -- fill pmem2_config
 */
static void
prepare_config(struct pmem2_config *cfg, struct pmem2_source *src,
	int *fd, const char *file, size_t length, size_t offset, int access)
{
	*fd = OPEN(file, access);

	pmem2_config_init(cfg);
	cfg->offset = offset;
	cfg->length = length;
	cfg->requested_max_granularity = PMEM2_GRANULARITY_PAGE;

	prepare_source(src, *fd);
}

/*
 * test_unaligned_addr - set unaligned reservation addr
 */
static int
test_unaligned_addr(const struct test_case *tc, int argc, char *argv[])
{
	struct pmem2_vm_reservation *rsv = NULL;

	/* "randomly" chosen value of addr */
	void *addr = (void *)0x12345;
	size_t len = Ut_mmap_align;

	int ret = pmem2_vm_reservation_new(&rsv, addr, len);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_ADDRESS_UNALIGNED);

	return 0;
}

/*
 * test_unaligned_len - set unaligned reservation len
 */
static int
test_unaligned_len(const struct test_case *tc, int argc, char *argv[])
{
	struct pmem2_vm_reservation *rsv = NULL;

	/* "randomly" chosen value of len */
	size_t len = 12345;
	void *addr = (void *)Ut_mmap_align;

	int ret = pmem2_vm_reservation_new(&rsv, addr, len);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_LENGTH_UNALIGNED);

	return 0;
}

/*
 * test_reserve_occupied_region - reserve memory in already occupied region
 */
static int
test_reserve_occupied_region(const struct test_case *tc, int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_reserve_occupied_region <file> <size>");

	char *file = argv[0];
	size_t filesize = ATOUL(argv[1]);
	struct pmem2_config cfg;
	struct pmem2_source src;
	struct pmem2_map *map = NULL;
	struct pmem2_vm_reservation *rsv = NULL;
	int fd;
	void *addr;

	prepare_config(&cfg, &src, &fd, file, filesize, 0, O_RDWR);
	int ret = pmem2_map(&cfg, &src, &map);
	UT_ASSERTeq(ret, 0);

	/* let's get addr of the current mapping */
	addr = pmem2_map_get_address(map);

	/* let's set rsv len as filesize */
	size_t reserv_len = filesize;

	/* do the reservation to the mapped area */
	ret = pmem2_vm_reservation_new(&rsv, addr, reserv_len);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	/* unmap mapping */
	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	CLOSE(fd);

	return 2;
}

/*
 * test_reserve_free_region - reserve memory in free region
 */
static int
test_reserve_free_region(const struct test_case *tc, int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_reserve_free_region <file> <size>");

	char *file = argv[0];
	size_t filesize = ATOUL(argv[1]);
	struct pmem2_config cfg;
	struct pmem2_source src;
	struct pmem2_map *map = NULL;
	struct pmem2_vm_reservation *rsv = NULL;
	int fd;
	void *addr;

	prepare_config(&cfg, &src, &fd, file, filesize, 0, O_RDWR);
	int ret = pmem2_map(&cfg, &src, &map);
	UT_ASSERTeq(ret, 0);

	/* let's get addr of the current mapping */
	addr = pmem2_map_get_address(map);

	/* unmap current mapping */
	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	CLOSE(fd);

	/* let's set rsv len as filesize */
	size_t reserv_len = filesize;

	/* do the reservation to the currently unmapped area */
	ret = pmem2_vm_reservation_new(&rsv, addr, reserv_len);
	UT_ASSERTeq(ret, 0);

	/*
	 * XXX: sprawdzic czy na pewno sie zarezerwowalo?
	 */

	UT_ASSERTeq(rsv->addr, addr);
	UT_ASSERTeq(rsv->length, reserv_len);

	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(rsv, NULL);

	return 2;
}

/*
 * test_reserve_region_by_kernel - reserve memory by letting kernel to choose
 * the addr
 */
static int
test_reserve_region_by_kernel(const struct test_case *tc,
	int argc, char *argv[])
{
	struct pmem2_vm_reservation *rsv = NULL;

	/* "randomly" chosen value of len */
	size_t len = Ut_mmap_align;
	void *addr = NULL;

	int ret = pmem2_vm_reservation_new(&rsv, addr, len);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTne(rsv, NULL);

	UT_ASSERTne(rsv->addr, addr);
	UT_ASSERTeq(rsv->length, len);

	/*
	 * XXX: sprawdzic czy na pewno sie zarezerwowalo?
	 */

	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(rsv, NULL);

	return 0;
}

/*
 * XXX: dodac wiecej testow takich jak w pmem2_map na flage noreplace?
 */

/*
 * test_cases -- available test cases
 */
static struct test_case test_cases[] = {
	TEST_CASE(test_unaligned_addr),
	TEST_CASE(test_unaligned_len),
	TEST_CASE(test_reserve_occupied_region),
	TEST_CASE(test_reserve_free_region),
	TEST_CASE(test_reserve_region_by_kernel),
};

#define NTESTS (sizeof(test_cases) / sizeof(test_cases[0]))

int
main(int argc, char **argv)
{
	START(argc, argv, "pmem2_vm_reservation");

	util_init();
	out_init("pmem2_vm_reservation",
		"TEST_LOG_LEVEL", "TEST_LOG_FILE", 0, 0);
	TEST_CASE_PROCESS(argc, argv, test_cases, NTESTS);
	out_fini();

	DONE(NULL);
}

#ifdef _MSC_VER
MSVC_CONSTR(libpmem2_init)
MSVC_DESTR(libpmem2_fini)
#endif