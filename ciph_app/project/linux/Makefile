# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2016-2017 Intel Corporation

#export RTE_SDK=/home/tsis/projects/dpdk_test/dpdk-stable-19.11.3
#export RTE_TARGET=x86_64-native-linux-gcc

ROOTDIR=../../..

DPDK_VER=20.05

RTE_SDK=$(ROOTDIR)/3rdparty/dpdk-$(DPDK_VER)
RTE_TARGET=x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

ifndef CFG
CFG=debug
endif

ifeq "$(CFG)"  "release"
CFLAGS+=-O2
CXXFLAGS+=-O2
DEFINE+=-DNDEBUG
else
ifeq "$(CFG)"  "debug"
CFLAGS+=-g -O0
CXXFLAGS+=-g -O0
DEFINE+=-D_DEBUG
endif
endif

OS = linux
#CFG = debug
LIBDESTDIR=$(ROOTDIR)/lib/$(OS)/$(CFG)

CC=gcc
APP = dpdk-crypto-app

file := "$(ROOTDIR)/VERSION"
_VERSION_ := $(shell cat ${file})

#CFLAGS += $(WERROR_FLAGS) 
CFLAGS += -DVERSION=\"$(_VERSION_)\" -DALLOW_EXPERIMENTAL_API -D_LINUX -D_REDHAT -std=gnu++11 -fpermissive -Wno-unused-parameter -Wno-cast-qual -w -mavx2
#CFLAGS += -O3

# all source are stored in SRCS-y
SRCS-y := $(ROOTDIR)/ciph_app/ciph_app.cpp \
			$(ROOTDIR)/libciph_agent/ciph_agent.cpp

INCLUDE = -I$(ROOTDIR) -I$(ROOTDIR)/dpdk -I$(ROOTDIR)/libdpdk_cryptodev_client -I$(ROOTDIR)/libciph_agent \
		-I$(ROOTDIR)/libmemif_client \
		-I$(ROOTDIR)/libmemcpy \
		-I$(ROOTDIR)/libmemif/src \
		-I$(RTE_SDK)/x86_64-native-linuxapp-gcc/include \
		-I$(ROOTDIR)/3rdparty/fridmon-0.1.10/quark_1_0_1 \
		-I$(ROOTDIR)/3rdparty/fridmon-0.1.10/meson_1_0_1 \
		-I$(ROOTDIR)/3rdparty/fridmon-0.1.10/nucleon_1_0_1 \
		-I$(ROOTDIR)/3rdparty/fridmon-0.1.10/hyperon_1_0_1

#-I$(ROOTDIR)/libcommon
#-L/usr/lib
LDFLAGS +=-L$(LIBDESTDIR) \
	-L$(RTE_SDK)/x86_64-native-linuxapp-gcc/lib \
	-L$(ROOTDIR)/3rdparty/fridmon-0.1.10/quark_1_0_1/lib/redhat/$(CFG) \
	-L$(ROOTDIR)/3rdparty/fridmon-0.1.10/meson_1_0_1/lib/redhat/$(CFG) \
	-L$(ROOTDIR)/3rdparty/fridmon-0.1.10/hyperon_1_0_1/lib/redhat/$(CFG)

CFLAGS += $(INCLUDE)
ifeq ($(CONFIG_RTE_LIBRTE_PMD_CRYPTO_SCHEDULER),y)
LDLIBS = -lrte_pmd_crypto_scheduler -ldpdk_cryptodev_client -lmemif_client -lquark -lhyperon -lmeson -lexpat -lstdc++ 
endif
#$(error   LDLIBS is $(LDLIBS))

#-lmemif
CONFIG_RTE_LIBRTE_PMD_MEMIF=n
include $(RTE_SDK)/mk/rte.app.mk
