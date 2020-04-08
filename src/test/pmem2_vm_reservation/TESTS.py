#!../env.py
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

import testframework as t
from testframework import granularity as g


@g.no_testdir()
class Pmem2VmReservationNoDir(t.Test):
    test_type = t.Short

    def run(self, ctx):
        ctx.exec('pmem2_vm_reservation', self.test_case)


@g.require_granularity(g.ANY)
class Pmem2VmReservationDir(t.Test):
    test_type = t.Short
    filesize = 16 * t.MiB

    def run(self, ctx):
        filepath = ctx.create_holey_file(self.filesize, 'testfile1')
        ctx.exec('pmem2_vm_reservation', self.test_case, filepath, self.filesize)


class TEST0(Pmem2VmReservationNoDir):
    """set unaligned reservation addr"""
    test_case = "test_unaligned_addr"


class TEST1(Pmem2VmReservationNoDir):
    """set unaligned reservation len"""
    test_case = "test_unaligned_len"


class TEST2(Pmem2VmReservationDir):
    """reserve memory in already occupied region"""
    test_case = "test_reserve_occupied_region"


class TEST3(Pmem2VmReservationDir):
    """reserve memory in free region"""
    test_case = "test_reserve_free_region"


class TEST4(Pmem2VmReservationNoDir):
	"""reserve memory by letting kernel to choose the addr"""
	test_case = "test_reserve_region_by_kernel"