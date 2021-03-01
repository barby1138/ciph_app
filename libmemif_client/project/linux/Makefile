# Makefile

CC=gcc
CFLAGS=-MD
CXX=gcc
CXXFLAGS=$(CFLAGS) -W -fexceptions -fPIC -fpermissive -w -std=c++11
AR=ar
ARFLAGS=rus

OS=linux

LIBNAME=libmemif_client

ROOTDIR=../../..
LIBDIR=$(ROOTDIR)/$(LIBNAME)
OBJDIR=$(ROOTDIR)/obj/$(OS)/$(CFG)
DESTDIR=$(ROOTDIR)/lib/$(OS)/$(CFG)
TARGET=$(DESTDIR)/$(LIBNAME).a

DEFINE=-D_LINUX

INCLUDE=-I$(ROOTDIR) -I$(ROOTDIR)/libmemif/src -I$(ROOTDIR)/libmemcpy

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
	$(LIBDIR)/memif*.c* \
	$(ROOTDIR)/libmemif/src/*.h \
	$(ROOTDIR)/libmemif/src/*.c*

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
