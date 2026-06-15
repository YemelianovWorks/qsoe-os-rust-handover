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

.PHONY: all prepare nvme

all:
	$(MAKE) -C nq
	$(MAKE) -C lq

prepare:
	./proj_obtain.sh

# ---- NVMe QEMU test image -------------------------------------------
# Eight 16-MiB partitions; p8 carries the fs-qrv type GUID (same as the
# on-disk layout on real hardware).  Idempotent: an existing image with a
# valid primary GPT ("EFI PART" at byte 512) is left untouched so any
# content survives a rebuild.
NVME_IMG       := build/nvme.img
NVME_IMG_SIZE  := 192M
NVME_NPARTS    := 8
NVME_PARTS     := 16 16 16 16 16 16 16 16

nvme: $(NVME_IMG)

$(NVME_IMG): host_tools/mkgpt.py
	@mkdir -p $(dir $@)
	@if [ ! -f $@ ] || \
	    [ "$$(dd if=$@ bs=8 skip=64 count=1 2>/dev/null)" != "EFI PART" ]; then \
		truncate -s $(NVME_IMG_SIZE) $@; \
		echo "make nvme: $@ ($(NVME_IMG_SIZE), GPT, $(NVME_NPARTS) x 16 MiB, p8 = fs-qrv)"; \
		host_tools/mkgpt.py --fsqrv $(NVME_NPARTS) $@ $(NVME_PARTS); \
	fi
