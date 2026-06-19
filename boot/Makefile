#
# boot/Makefile -- boot-image deployment to the RISC-V workstation
# ("freedom", SiFive Unmatched / FU740), plus the legacy U-Boot script
# images.
#
# The target's boot loader is mr-bml, whose `kernel` command takes either
# an ELF64 or a raw RISC-V Image -- no uImage wrapper.  `make deploy`
# ships both forms of the Skimmer kernel so the mr-bml entry can use
# whichever the board actually boots: the self-relocating seL4 elfloader
# loads fine as an ELF, but the raw Skimmer kernel ELF does not (mr-bml
# jumps to a bad entry), so the flat skimmer.bin -- loaded at the kernel's
# link address -- is the form QSOE/N boots from.  Targets, shipped to
# $(REMOTE_HOST):$(REMOTE_DIR):
#
#   QSOE/N: skimmer.bin        the flat Skimmer kernel image (what mr-bml
#                              boots), and skimmer.elf alongside it for
#                              inspection / future ELF-load support, plus
#                              modpkg.cpio (its initrd, mr-bml `modpkg`)
#   QSOE/L: qsoe-l-sifive.elf   the seL4 elfloader ELF (embeds seL4
#                              kernel + taskman + modpkg via .incbin)
#   plus nvme_p8.dat -- the fs-qrv partition (p8) extracted from
#        ../build/nvme.img, ready to dd onto a real NVMe partition.
#        Only p8 is shipped: the other partitions are empty scratch
#        (mirrors QRV, which deployed nvme_p8.dat alone).
#
# `make` (default) still builds the legacy U-Boot .scr script images
# from the .txt command files for booting by hand from a U-Boot prompt;
# they are no longer part of `make deploy`.
#
# Build the QSOE/L artifact first:
#     make -C ../lq sifive_defconfig && make -C ../lq
#
# Copyright (c) 2026 Yuri Zaporozhets <yuriz@qsoe.net>
# SPDX-License-Identifier: Apache-2.0
#

SCRIPTS     = boot-nq.scr boot-lq.scr
REMOTE_HOST = freedom
REMOTE_DIR  = /boot/qsoe

# Artifact locations inside the umbrella tree.  The NVMe image is shared
# host tooling built at the umbrella (build/nvme.img), not in a variant --
# it is variant-independent, so it carries no NQ_/LQ_ prefix.
NQ_KERNEL_ELF = ../nq/kernel/arch/riscv/skimmer
NQ_KERNEL_BIN = ../nq/kernel/arch/riscv/skimmer.bin
NQ_MODPKG     = ../nq/build/modpkg.cpio
NVME_IMG      = ../build/nvme.img
LQ_QSOE_ELF   = ../lq/build/qsoe-l-sifive.elf

# Only the fs-qrv partition is useful on hardware; the rest of nvme.img is
# empty scratch.  p8 carries the fs-qrv (qrvfs) partition-type GUID --
# mirrors the canonical NVMe layout (see ../host_tools/mkgpt.py).
NVME_FSQRV_PART = 8                      # 1-based partition index of fs-qrv
NVME_P8_DAT     = nvme_p8.dat            # extracted partition, shipped alone
GPTEXTRACT      = ./gptextract.py

all: $(SCRIPTS)

boot-nq.scr: boot-nq.txt
	mkimage -T script -A riscv -O linux -C none -n "Boot QSOE/N" -d $< $@

boot-lq.scr: boot-lq.txt
	mkimage -T script -A riscv -O linux -C none -n "Boot QSOE/L" -d $< $@

# Deploy the ELF kernels (mr-bml loads them directly) plus the NQ initrd
# and the fs-qrv partition image.  The .scr scripts and the raw .bin /
# .uImage forms are intentionally NOT shipped -- mr-bml needs neither.
# The QSOE/L ELF is shipped only if a SiFive build is present.
deploy:
	ssh $(REMOTE_HOST) mkdir -p $(REMOTE_DIR)
	scp $(NQ_KERNEL_BIN) $(REMOTE_HOST):$(REMOTE_DIR)/skimmer.bin
	scp $(NQ_KERNEL_ELF) $(REMOTE_HOST):$(REMOTE_DIR)/skimmer.elf
	scp $(NQ_MODPKG) $(REMOTE_HOST):$(REMOTE_DIR)/
	if test -f $(LQ_QSOE_ELF); then \
	    scp $(LQ_QSOE_ELF) $(REMOTE_HOST):$(REMOTE_DIR)/ ; fi
	if test -f $(NVME_IMG); then \
	    $(GPTEXTRACT) $(NVME_IMG) $(NVME_FSQRV_PART) $(NVME_P8_DAT) && \
	    scp $(NVME_P8_DAT) $(REMOTE_HOST):$(REMOTE_DIR)/ ; fi

clean:
	rm -f $(SCRIPTS) $(NVME_P8_DAT)

.PHONY: all clean deploy
