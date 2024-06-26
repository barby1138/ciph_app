# Makefile

CC=gcc
CFLAGS=-MD
CXX=gcc
CXXFLAGS=$(CFLAGS) -W -fexceptions -fPIC -fpermissive -w -mavx2 -std=c++11
AR=ar
ARFLAGS=rus
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=-shared

OS=linux

LIBNAME=libciph_agent

ROOTDIR=../../..
LIBDIR=$(ROOTDIR)/$(LIBNAME)
OBJDIR=$(ROOTDIR)/obj/$(OS)/$(CFG)
DESTDIR=$(ROOTDIR)/lib/$(OS)/$(CFG)

#TARGET=$(DESTDIR)/$(LIBNAME).a
TARGET=$(DESTDIR)/$(LIBNAME).so

COMMONLIBS=
SHAREDLIBS= -lmemif_client -lstdc++

DEFINE=-D_LINUX -D_REDHAT

INCLUDE=-I$(ROOTDIR) -I$(ROOTDIR)/libmemif_client -I$(ROOTDIR)/libdpdk_cryptodev_client \
		-I$(ROOTDIR)/libmemcpy -I$(ROOTDIR)/libmemif/src

LDFLAGS+=-L$(DESTDIR) 

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

#
#$(TARGET): $(OBJS) $(DESTDIR)
#	$(AR) $(ARFLAGS) $@ $(OBJS)
#endif

$(TARGET): $(OBJS) $(DESTDIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(SHAREDLIBS) $(COMMONLIBS)

.PHONY: clean
clean:
	-rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

