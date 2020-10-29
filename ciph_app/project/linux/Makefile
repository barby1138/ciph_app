# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2016-2017 Intel Corporation

#export RTE_SDK=/home/tsis/projects/dpdk_test/dpdk-stable-19.11.3
#export RTE_TARGET=x86_64-native-linux-gcc

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
ROOTDIR=../../..
#CFG = debug
LIBDESTDIR=$(ROOTDIR)/lib/$(OS)/$(CFG)

CC=gcc
APP = dpdk-crypto-app

#CFLAGS += $(WERROR_FLAGS) 
CFLAGS += -DALLOW_EXPERIMENTAL_API -D_LINUX -D_REDHAT -std=gnu++11 -fpermissive -Wno-unused-parameter -Wno-cast-qual -w -mavx2
#CFLAGS += -O3

# all source are stored in SRCS-y
SRCS-y := ../../ciph_app.cpp
#SRCS-y += ../../dpdk_cryptodev_client.cpp

INCLUDE = -I$(ROOTDIR) -I$(ROOTDIR)/dpdk -I$(ROOTDIR)/libdpdk_cryptodev_client -I$(ROOTDIR)/libciph_agent \
		-I$(ROOTDIR)/libmemif_client -I$(ROOTDIR)/libcommon \
		-I$(ROOTDIR)/libmemcpy \
		-I/home/tsis/projects/dpdk_test/dpdk-stable-19.11.3/x86_64-native-linux-gcc/include \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/quark_1_0_1 \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/meson_1_0_1 \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/nucleon_1_0_1 \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/hyperon_1_0_1

LDFLAGS +=-L/usr/lib -L$(LIBDESTDIR) \
	-L/home/tsis/projects/dpdk_test/dpdk-stable-19.11.3/x86_64-native-linux-gcc/lib \
	-L$(ROOTDIR)/../fwk/fridmon[18-01-2017]/quark_1_0_1/lib/redhat/$(CFG) \
	-L$(ROOTDIR)/../fwk/fridmon[18-01-2017]/meson_1_0_1/lib/redhat/$(CFG) \
	-L$(ROOTDIR)/../fwk/fridmon[18-01-2017]/hyperon_1_0_1/lib/redhat/$(CFG)

CFLAGS += $(INCLUDE)

ifeq ($(CONFIG_RTE_LIBRTE_PMD_CRYPTO_SCHEDULER),y)
LDLIBS += -lrte_pmd_crypto_scheduler -ldpdk_cryptodev_client -lmemif_client -lmemif -lquark -lhyperon -lmeson -lexpat -lstdc++
endif

include $(RTE_SDK)/mk/rte.app.mk
