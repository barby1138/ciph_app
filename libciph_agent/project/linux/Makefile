# Makefile

CC=gcc
CFLAGS=-MD
CXX=g++
CXXFLAGS=$(CFLAGS) -W -fexceptions -fPIC -fpermissive -w -mavx2
AR=ar
ARFLAGS=rus

OS=linux

LIBNAME=libciph_agent

ROOTDIR=../../..
LIBDIR=$(ROOTDIR)/$(LIBNAME)
OBJDIR=$(ROOTDIR)/obj/$(OS)/$(CFG)
DESTDIR=$(ROOTDIR)/lib/$(OS)/$(CFG)
TARGET=$(DESTDIR)/$(LIBNAME).a

DEFINE=-D_LINUX -D_REDHAT

INCLUDE=-I$(ROOTDIR) -I$(ROOTDIR)/libcommon -I$(ROOTDIR)/libmemif_client -I$(ROOTDIR)/libdpdk_cryptodev_client \
		-I$(ROOTDIR)/libmemcpy \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/quark_1_0_1

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

