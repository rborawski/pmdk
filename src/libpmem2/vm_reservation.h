// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * vm_reservation.h -- internal definitions for pmem2_vm_reservation
 */
#ifndef PMEM2_VM_RESERVATION_H
#define PMEM2_VM_RESERVATION_H

#include "libpmem2.h"

struct pmem2_vm_reservation {
	void *addr; /* address of the reservation */
	size_t length; /* length of the reservation */
};

#endif /* PMEM2_VM_RESERVATION */
