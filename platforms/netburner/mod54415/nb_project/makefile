#NB_COPYRIGHT#
#
# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause
#
# MPLC MOD54415 NNDK application — open this folder in NBEclipse (MOD5441X).
# Command-line build (from this directory):
#   C:\nburn\SetEnv.bat
#   make

NAME = MPLC_MOD54415
SUPPORTED_PLATFORMS = MOD5441X

# MPLC repository root (nb_project is four levels below repo root)
MPLC_ROOT := ../../../../
EFFS_STD_ROOT := $(NNDK_ROOT)/examples/_common/EFFS/STD

# Reserve 1 MB at top of parallel flash for EFFS-STD (MOD5441X)
ifeq ($(PLATFORM),MOD5441X)
    COMPCODEFLAGS = 0xC0040000 0xC1F00000
endif

CPPFLAGS += -DMPLC_NETBURNER_NNDK -DMPLC_BIG_ENDIAN
DBCPPFLAGS += -DMPLC_NETBURNER_NNDK -DMPLC_BIG_ENDIAN

NBINCLUDE += \
	-I"$(MPLC_ROOT)/schema" \
	-I"$(MPLC_ROOT)/hal/include" \
	-I"$(MPLC_ROOT)/runtime/include" \
	-I"../" \
	-I"$(EFFS_STD_ROOT)/src"

XTRALIB = $(NNDK_ROOT)/platform/$(PLATFORM)/original/lib/libStdFFile.a
DBXTRALIB = $(NNDK_ROOT)/platform/$(PLATFORM)/original/lib/libStdFFile.a

C_SRC += \
	loader.c \
	vm.c \
	scheduler.c \
	runtime.c \
	io.c \
	stdlib.c \
	mplc_plc_runtime.c \
	embedded_mplc.c

CPP_SRC += \
	src/main.cpp \
	src/web_update.cpp \
	hal_netburner.cpp \
	mplc_task.cpp \
	simple_ad.cpp \
	mplc_nb_fs.cpp \
	fs_main.cpp \
	effsStdFlashDrv.cpp \
	effs_std.cpp \
	ramdrv_mcf.cpp

# Build .bin only; do not produce .s19 / _APP.s19 artifacts.
TARGET_TYPE = bin

include $(NNDK_ROOT)/make/boilerplate.mk

# boilerplate.mk assigns VPATH := overload $(NNDK_ROOT); append MPLC trees after include.
override VPATH += \
	$(MPLC_ROOT)/runtime/core \
	$(MPLC_ROOT)/runtime/io \
	$(MPLC_ROOT)/runtime/stdlib \
	../ \
	src \
	$(EFFS_STD_ROOT)/src

# Override NNDK cpuinc.mk SREC chain: stage SREC in a temp file, emit .bin, delete intermediates.
MPLC_SREC_STAGE = $(OBJDIR)/release/.$(NAME).srec.stage
DB_MPLC_SREC_STAGE = $(OBJDIR)/debug/.DB$(NAME).srec.stage

$(OBJDIR)/release/$(NAME).bin: $(OBJDIR)/release/$(NAME).elf
	@echo Building $@ from $<
	$(OBJCOPY) -Wl,--gc-sections --strip-all --output-target=srec $< $(MPLC_SREC_STAGE)
	$(PACKCODE) $(MPLC_SREC_STAGE) $@ -M $(OBJDIR)/release/$(NAME).map -B -R $(COMPCODEFLAGS) -P$(PLATFORM)
	$(RM) -f $(MPLC_SREC_STAGE) $(OBJDIR)/release/$(NAME).s19 $(OBJDIR)/release/$(NAME)_APP.s19

$(OBJDIR)/debug/DB$(NAME).bin: $(OBJDIR)/debug/DB$(NAME).elf
	@echo Building $@ from $<
	$(OBJCOPY) -Wl,--gc-sections --strip-all --output-target=srec $< $(DB_MPLC_SREC_STAGE)
	$(PACKCODE) $(DB_MPLC_SREC_STAGE) $@ -M $(OBJDIR)/debug/DB$(NAME).map -B -R $(COMPCODEFLAGS) -P$(PLATFORM)
	$(RM) -f $(DB_MPLC_SREC_STAGE) $(OBJDIR)/debug/DB$(NAME).s19 $(OBJDIR)/debug/DB$(NAME)_APP.s19

.PHONY: s19
s19:
	@echo "$(NAME): S19 outputs disabled; use $(TARGET)"
	@false

$(OBJDIR)/release/$(NAME).s19 $(OBJDIR)/release/$(NAME)_APP.s19 \
$(OBJDIR)/debug/DB$(NAME).s19 $(OBJDIR)/debug/DB$(NAME)_APP.s19:
	@echo "$(NAME): S19 outputs disabled; use $(TARGET)"
	@false
