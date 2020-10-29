# Makefile

CC=gcc
CFLAGS=-MD
CXX=g++
CXXFLAGS=$(CFLAGS) -std=c++11 -Wall -fexceptions -fpermissive -Wattributes -mavx2
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=

OS=linux

ROOTDIR=../../..
SRCDIR=$(ROOTDIR)/ciph_app_test
OBJDIR=$(ROOTDIR)/obj/$(OS)/$(CFG)
LIBDESTDIR=$(ROOTDIR)/lib/$(OS)/$(CFG)
DESTDIR=$(ROOTDIR)/bin/$(OS)/$(CFG)

TARGET=$(DESTDIR)/ciph_app_test

COMMONLIBS= -lpthread -lrt
SHAREDLIBS= -lcommon \
		-lciph_agent -lmemif_client -lmemif\
		-lquark -lhyperon -lmeson -lexpat

DEFINE=-D_LINUX -D_REDHAT

INCLUDE=-I$(ROOTDIR) -I$(ROOTDIR)/libciph_agent -I$(ROOTDIR)/libcommon \
		-I$(ROOTDIR)/libdpdk_cryptodev_client -I$(ROOTDIR)/libmemif_client \
		-I$(ROOTDIR)/libmemcpy \
		-I$(RTE_SDK)/$(RTE_TARGET)/include \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/quark_1_0_1 \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/meson_1_0_1 \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/nucleon_1_0_1 \
		-I$(ROOTDIR)/../fwk/fridmon[18-01-2017]/hyperon_1_0_1

LDFLAGS+=-L/usr/lib -L$(LIBDESTDIR) \
	-L$(ROOTDIR)/../fwk/fridmon[18-01-2017]/quark_1_0_1/lib/redhat/$(CFG) \
	-L$(ROOTDIR)/../fwk/fridmon[18-01-2017]/meson_1_0_1/lib/redhat/$(CFG) \
	-L$(ROOTDIR)/../fwk/fridmon[18-01-2017]/hyperon_1_0_1/lib/redhat/$(CFG)

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

CFLAGS+=$(DEFINE) $(INCLUDE)

SRC= \
	$(SRCDIR)/ciph_app_test.c
HDR= \
	$(SRCDIR)/*.h

SRCS=$(SRC) $(HDR) 

OBJS=$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(subst $(ROOTDIR),$(OBJDIR),$(filter %.c %.cc %.cpp %.cxx,$(SRCS)))))))
DEPS=$(patsubst %.o,%.d,$(OBJS))

.PHONY: all
all: $(TARGET)

$(DESTDIR): 
	-mkdir -p $(DESTDIR)

$(OBJS):
	-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $(wildcard $(patsubst %.o,%.c*,$(subst $(OBJDIR),$(ROOTDIR),$@)))

$(TARGET): $(OBJS) $(DESTDIR) 
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(SHAREDLIBS) $(COMMONLIBS)
	cp $(TARGET) ../../../../bin

.PHONY: clean
clean:
	-rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

