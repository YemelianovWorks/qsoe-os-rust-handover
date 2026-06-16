# Makefile -- top-level QSOE umbrella build orchestrator.
#
# Goals:
#   all        build both OS variants (nq, then lq); each component
#              owns its build, this file only descends.
#   prepare    obtain the components matching this tree's release tag
#              (delegated to proj_obtain.sh, see component.list).
#   nvme       build the QEMU NVMe test image (a GPT skeleton).  This is
#              host-side tooling shared by BOTH variants -- the disk runs
#              under QEMU on either kernel -- so the image and its builder
#              (host_tools/mkgpt.py) live here at the umbrella, not in a
#              variant.  The variants' emu.sh delegate to this target; they
#              never lay the image themselves.  (The board uses a real disk.)
#
# Copyright (c) 2026 Yuri Zaporozhets <yuriz@qsoe.net>
# SPDX-License-Identifier: Apache-2.0

.PHONY: all prepare nvme nvme-populate

all:
	$(MAKE) -C nq
	$(MAKE) -C lq

prepare:
	./proj_obtain.sh

# ---- NVMe QEMU test image -------------------------------------------
# Eight 16-MiB partitions; p8 carries the fs-qrv type GUID (same as the
# on-disk layout on real hardware).  The GPT skeleton is idempotent: an
# existing image with a valid primary GPT ("EFI PART" at byte 512) is left
# untouched.  p8 is then (re)populated with a qrvfs image carrying the
# userspace that lives on disk rather than in the boot cpio -- today
# /bin/suite, served as /usr/bin/suite once the fs is mounted.  mkfs-qrv is
# host tooling shared by both variants, so it lives here at the umbrella.
NVME_IMG       := build/nvme.img
NVME_IMG_SIZE  := 192M
NVME_NPARTS    := 8
NVME_PARTS     := 16 16 16 16 16 16 16 16

FSQRV_PART     := 8
FSQRV_SIZE_MB  := 16
MKFS_QRV       := build/mkfs-qrv
FSQRV_ROOT     := build/fsqrv-root
FSQRV_IMG      := build/fsqrv.img
# The on-disk userspace, taken from quser's build output (built by nq/lq
# before emu.sh delegates here).  Each "<src>:<name>" pair becomes
# /usr/bin/<name> under the mount.  The test binaries live here rather than
# in the boot cpio -- modpkg carries only what bring-up needs.
FSQRV_BINS     := quser/build/test/suite/suite.elf:suite \
                  quser/build/test/msgpass/test_msgpass.elf:test_msgpass \
                  quser/build/test/syncspace/test_syncspace.elf:test_syncspace

nvme: $(NVME_IMG) nvme-populate

$(NVME_IMG): host_tools/mkgpt.py
	@mkdir -p $(dir $@)
	@if [ ! -f $@ ] || \
	    [ "$$(dd if=$@ bs=8 skip=64 count=1 2>/dev/null)" != "EFI PART" ]; then \
		truncate -s $(NVME_IMG_SIZE) $@; \
		echo "make nvme: $@ ($(NVME_IMG_SIZE), GPT, $(NVME_NPARTS) x 16 MiB, p8 = fs-qrv)"; \
		host_tools/mkgpt.py --fsqrv $(NVME_NPARTS) $@ $(NVME_PARTS); \
	fi

$(MKFS_QRV): host_tools/mkfs-qrv.c quser/fs/qrv/fs.h
	@mkdir -p $(dir $@)
	@cc -O2 -Wall -I quser/fs/qrv -o $@ $<

# Populate p8 from a proto root assembled out of quser's build output.
# Always runs (the on-disk userspace changes with quser); the staged tree
# becomes /usr/bin/* under the mount.  If quser hasn't been built yet there
# is nothing to stage, so the GPT skeleton is left intact with a note.
nvme-populate: $(NVME_IMG) $(MKFS_QRV)
	@rm -rf $(FSQRV_ROOT); mkdir -p $(FSQRV_ROOT)/bin; \
	have=0; \
	for pair in $(FSQRV_BINS); do \
		src=$${pair%%:*}; name=$${pair##*:}; \
		if [ -f "$$src" ]; then cp "$$src" $(FSQRV_ROOT)/bin/$$name; have=1; fi; \
	done; \
	if [ $$have = 1 ]; then \
		"$(MKFS_QRV)" -s $(FSQRV_SIZE_MB) $(FSQRV_IMG) $(FSQRV_ROOT) >/dev/null; \
		host_tools/mkgpt.py --write-part $(FSQRV_PART) $(NVME_IMG) \
			$(FSQRV_IMG) $(NVME_PARTS); \
		echo "make nvme: p8 populated -> /usr/bin from $(FSQRV_ROOT)"; \
	else \
		echo "make nvme: no fs binaries built -- p8 left empty (build quser first)"; \
	fi
