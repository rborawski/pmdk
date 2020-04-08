// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * vm_reservation_windows.c -- pmem2_vm_reservation windows implementation
 */

#include "alloc.h"
#include "vm_reservation.h"
#include "libpmem2.h"
#include "out.h"
#include "pmem2.h"
#include "pmem2_utils.h"

/*
 * XXX: check includes and Makefile/proj!
 */

/*
 * validate_addr_alignment -- validate if addr is multiple
 * of dwAllocationGranularity
 */
static int
validate_addr_alignment(void *addr)
{
	/* cannot NULL % dwAllocationGranularity, NULL is valid */
	if (addr == NULL)
		return 0;

	/* to get dwAllocationGranularity */
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);

	if ((size_t)addr % sys_info.dwAllocationGranularity != 0) {
		ERR("address %p is not a multiple of %lu",
				addr, sys_info.dwAllocationGranularity);
		return PMEM2_E_ADDRESS_UNALIGNED;
	}

	return 0;
}

/*
 * validate_len_alignment -- validate if length is multiple
 * of dwAllocationGranularity
 */
static int
validate_len_alignment(size_t len)
{
	/* to get dwAllocationGranularity */
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);

	if (((DWORD)len % sys_info.dwAllocationGranularity) != 0) {
		ERR("length %llu is not a multiple of %lu",
				len, sys_info.dwAllocationGranularity);
		return PMEM2_E_LENGTH_UNALIGNED;
	}

	return 0;
}

/*
 * mem_reserve -- reserve given memory region by VirtualAlloc2
 */
static int
mem_reserve(void **addr, size_t length)
{
	/* get current process and send it to VirtualAlloc2 */
	HANDLE process = GetCurrentProcess();
	int ret = 0;

	void *reserv_addr = (PCHAR)VirtualAlloc2(
		process,
		*addr,
		length,
		MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
		PAGE_NOACCESS,
		NULL,
		0);

	if (reserv_addr == NULL) {
		ERR("!!VirtualAlloc2");
		DWORD ret_windows = GetLastError();

		/* check if address is already occupied */
		if (ret_windows == ERROR_INVALID_ADDRESS)
			ret = PMEM2_E_MAPPING_EXISTS;
		else
			ret = pmem2_lasterror_to_err();
	}

	/* when addr was NULL it has to be dereferenced */
	if (*addr == NULL)
		*addr = reserv_addr;

	return ret;
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
 * pmem2_vm_reservation_delete -- free reservation region and deallocate
 * vm reservation structure
 */
int
pmem2_vm_reservation_delete(struct pmem2_vm_reservation **rsv)
{
	int ret = 0;
	int ret2 = VirtualFree(
		(*rsv)->addr,
		0, /* len must be zero when MEM_RELASE flag is used */
		MEM_RELEASE);

	if (ret2 == 0) {
		ERR("!!VirtualFree");
		ret = pmem2_lasterror_to_err();
	}

	Free(*rsv);
	*rsv = NULL;

	return ret;
}
