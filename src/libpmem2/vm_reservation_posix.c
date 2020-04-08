// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * vm_reservation_posix.c -- pmem2_vm_reservation posix implementation
 */

#include <unistd.h>
#include <sys/mman.h>

/*
 * XXX: check includes and Makefile/proj!
 */
#include "alloc.h"
#include "vm_reservation.h"
#include "libpmem2.h"
#include "out.h"
#include "pmem2.h"
#include "pmem2_utils.h"

/*
 * validate_addr_alignment -- validate if addr is multiple of pagesize
 */
static int
validate_addr_alignment(void *addr)
{
	/* cannot NULL % Pagesize, NULL is valid */
	if (addr == NULL)
		return 0;

	if ((size_t)addr % Pagesize) {
		ERR("address %p is not a multiple of %llu",
				addr, Pagesize);
		return PMEM2_E_ADDRESS_UNALIGNED;
	}

	return 0;
}

/*
 * validate_len_alignment -- validate if len is multiple of pagesize
 */
static int
validate_len_alignment(size_t len)
{
	if (len % Pagesize) {
		ERR("length %zu is not a multiple of %llu",
				len, Pagesize);
		return PMEM2_E_LENGTH_UNALIGNED;
	}

	return 0;
}

/*
 * mem_reserve -- reserve given memory region as anonymus mapping,
 * it is imitating MAP_FIXED_NOREPLACE flag
 */
static int
mem_reserve(void **addr, size_t length)
{
	char *reserv_addr = mmap(
		*addr, length, PROT_NONE, MAP_ANONYMOUS, -1, 0);
	if (reserv_addr == MAP_FAILED) {
		ERR("!mmap MAP_ANONYMOUS");
		return PMEM2_E_ERRNO;
	}

	/*
	 * There is imitation of the MAP_FIXED_NOREPLACE - we are imitating it
	 * because glibc started exposing this flag in version 4.17.
	 * If given addr is occupied, kernel chooses new addr randomly and
	 * returns it. We do not want that behavior, so we validate it and fail
	 * when addresses do not match.
	 */
	if (*addr != NULL && reserv_addr != *addr) {
		munmap(reserv_addr, length);
		ERR("mapping exists in the given address");
		return PMEM2_E_MAPPING_EXISTS;
	}

	/* when addr was NULL it has to be dereferenced */
	if (*addr == NULL)
		*addr = reserv_addr;

	return 0;
}

/*
 * pmem2_vm_reservation_new -- allocates, init vm_reservation structure
 * and reserve memory region
 */
int
pmem2_vm_reservation_new(struct pmem2_vm_reservation **rsv, void *addr,
		size_t length)
{
	int ret;
	*rsv = pmem2_malloc(sizeof(**rsv), &ret);

	if (ret)
		return ret;

	ASSERTne(rsv, NULL);

	ret = validate_addr_alignment(addr);
	if (ret)
		return ret;

	ret = validate_len_alignment(length);
	if (ret)
		return ret;

	ret = mem_reserve(&addr, length);
	if (ret)
		return ret;

	(*rsv)->addr = addr;
	(*rsv)->length = length;

	return 0;
}

/*
 * pmem2_vm_reservation_delete -- munmap reservation region and deallocate
 * vm reservation structure
 */
int
pmem2_vm_reservation_delete(struct pmem2_vm_reservation **rsv)
{
	int ret = munmap((*rsv)->addr, (*rsv)->length);
	if (ret < 0) {
		ERR("!munmap");
		return PMEM2_E_ERRNO;
	}

	Free(*rsv);
	*rsv = NULL;

	return 0;
}
