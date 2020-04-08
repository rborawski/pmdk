# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation
#

#
# src/test/pmem2_vm_reservation/Makefile -- build pmem2_vm_reservation unit test
#
TOP = ../../..

vpath %.c $(TOP)/src/test/unittest

INCS += -I$(TOP)/src/libpmem2
TARGET = pmem2_vm_reservation
OBJS += pmem2_vm_reservation.o\
	ut_pmem2_config.o\
	ut_pmem2_utils.o\
	ut_pmem2_source.o

LIBPMEM2=internal-debug

include ../Makefile.inc
