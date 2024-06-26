# Makefile

CC=gcc
CFLAGS=-MD
CXX=gcc
CXXFLAGS=$(CFLAGS) -W -fexceptions -fPIC -mavx2 -std=c++11
#-fpermissive
AR=ar
ARFLAGS=rus

OS=linux

LIBNAME=libdpdk_cryptodev_client

ROOTDIR=../../..
LIBDIR=$(ROOTDIR)/$(LIBNAME)
OBJDIR=$(ROOTDIR)/obj/$(OS)/$(CFG)
DESTDIR=$(ROOTDIR)/lib/$(OS)/$(CFG)
TARGET=$(DESTDIR)/$(LIBNAME).a

DEFINE=-D_LINUX -D_REDHAT

DPDK_VER=20.05

INCLUDE=-I$(ROOTDIR) \
		-I$(ROOTDIR)/CRCpp/inc \
		-I$(ROOTDIR)/libmemcpy \
		-I$(ROOTDIR)/3rdparty/fridmon-0.1.10/quark_1_0_1 \
		-I$(ROOTDIR)/3rdparty/fridmon-0.1.10/meson_1_0_1 \
		-I$(ROOTDIR)/3rdparty/dpdk-$(DPDK_VER)/x86_64-native-linuxapp-gcc/include 

ifndef CFG
CFG=debug
endif
ifeq "$(CFG)"  "release"
CFLAGS+=-O2
DEFINE+=-DNDEBUG
else
ifeq "$(CFG)"  "debug"
CFLAGS+=-g -O0
DEFINE+=-D_DEBUG
endif
endif

CFLAGS+=$(DEFINE) $(INCLUDE)

LIBSRC=  \
	$(LIBDIR)/*.h \
	$(LIBDIR)/*.c*

SRCS=$(wildcard $(LIBSRC))
OBJS=$(patsubst %.cxx,$(OBJDIR)/%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(subst $(ROOTDIR),$(OBJDIR),$(filter %.c %.cc %.cpp %.cxx,$(SRCS)))))))
DEPS=$(patsubst %.o,%.d,$(OBJS))

.PHONY: all
all: $(TARGET)

$(DESTDIR): 
	-mkdir -p $(DESTDIR)

$(OBJS):
	-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $(wildcard $(patsubst %.o,%.c*,$(subst $(OBJDIR),$(ROOTDIR),$@)))

$(TARGET): $(OBJS) $(DESTDIR)
	$(AR) $(ARFLAGS) $@ $(OBJS)

.PHONY: clean
clean:
	-rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
