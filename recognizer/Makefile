# Couldn't understand or usefully modify the former makefile.
# Copied the one from ../server_src here and put the former one
# in Makefile.old

##########################
# common definitions. For non-UNC sites, uncomment one of the lines
# that defines hw_os for the machine you are on in the section just
# below. Then, the code should compile in your environment.
#
#HW_OS := sgi_irix
#HW_OS := sparc_solaris
#HW_OS := sparc_solaris_64
#HW_OS := hp700_hpux10
#HW_OS := pc_linux
HW_OS := pc_linux64
#HW_OS := pc_linux_ia64
# Try using this to cross-compile for ARM on a PC running Linux
#HW_OS := pc_linux_arm
# Try using this to cross-compile for ARM on a PC running Cygwin
#HW_OS := pc_cygwin_arm
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
#HW_OS := powerpc_aix
#HW_OS := powerpc_macosx
#HW_OS := universal_macosx
#HW_OS := macosx_32_64
#HW_OS := macosx_64
##########################

INSTALL_DIR := /usr/local
BIN_DIR := $(INSTALL_DIR)/bin
#INCLUDE_DIR := $(INSTALL_DIR)/include
INCLUDE_DIR := ../common
LIB_DIR := $(INSTALL_DIR)/lib

ifndef	HW_OS
# hw_os does not exist on FreeBSD at UNC
UNAME := $(shell uname -s)
ifeq ($(UNAME), FreeBSD)
  HW_OS := pc_FreeBSD
else
  # pc_cygwin doesn't have HW_OS
  ifeq ($(UNAME), CYGWIN_NT-4.0)
    HW_OS := pc_cygwin
  else
    ifeq ($(UNAME), CYGWIN_98-4.10)
      HW_OS := pc_cygwin
    else
      HW_OS := $(shell hw_os)
    endif
  endif
endif
endif


# check if its for pxfl
ifdef PBASE_ROOT
  HW_OS := hp_flow
  ifeq ($(PXFL_COMPILER), aCC)
    HW_OS = hp_flow_aCC
  endif
endif

CC := g++
ARCH_LIBS := -lpthread -lquat -lm

ifeq ($(HW_OS),pc_linux)
  CC := g++
endif

ifeq ($(HW_OS),pc_linux_ia64)
  CC := g++
endif

ifeq ($(HW_OS), pc_linux_arm)
	CC := arm-linux-g++
	RANLIB := arm-linux-ranlib
	AR := arm-linux-ar ruv
endif

ifeq ($(HW_OS), pc_cygwin_arm)
	CC := arm-unknown-linux-gnu-g++
	RANLIB := arm-unknown-linux-gnu-ranlib
	AR := arm-unknown-linux-gnu-ar ruv
endif

ifeq ($(HW_OS),pc_linux64)
  CC := g++ -m64
endif

ifeq ($(HW_OS),sgi_irix)
  ifndef SGI_ABI
      SGI_ABI := n32
  endif
  ifndef SGI_ARCH
      SGI_ARCH := mips3
  endif
  OBJECT_DIR_SUFFIX := .$(SGI_ABI).$(SGI_ARCH)
  CC := CC -$(SGI_ABI) -$(SGI_ARCH)
  GLUTLIBS := -lglut -lGLU -lGL -lXmu -lXext -lX11
endif

ifeq ($(HW_OS),hp700_hpux10)
  CC := CC +a1
endif
ifeq ($(HW_OS), hp_flow_aCC)
  CC := /opt/aCC/bin/aCC
endif

ifeq ($(HW_OS),sparc_solaris)
  CC := /opt/SUNWspro/bin/CC
  ARCH_LIBS := -lsocket -lnsl $(ARCH_LIBS)
endif

ifeq ($(HW_OS),sparc_solaris_64)
  CC := /opt/SUNWspro/bin/CC -xarch=v9a
  AR := /opt/SUNWspro/bin/CC -xarch=v9a -xar -o
  ARCH_LIBS := -lsocket -lnsl $(ARCH_LIBS)
endif

ifeq ($(HW_OS),powerpc_aix)
#  CC := /usr/ibmcxx/bin/xlC_r -g -qarch=pwr3 -w -+
  CC := /usr/ibmcxx/bin/xlC_r $(DEBUG_FLAGS) -qarch=pwr3 -w -+
endif

ifeq ($(HW_OS),pc_cygwin)
  ARCH_LIBS := -lquat -lcygwin -luser32 -lgdi32 -lcomdlg32 -lwsock32
endif

#CFLAGS = -I../ -I../quat -g
override CFLAGS := -I../ -I../quat -I${INCLUDE_DIR} $(DEBUG_FLAGS) $(CFLAGS)
override CXXFLAGS := -I../ -I../quat -I${INCLUDE_DIR} -std=c++11 $(DEBUG_FLAGS) $(CXXFLAGS)

.SUFFIXES:	.cc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.C.o:
	$(CC) $(CXXFLAGS) -c $*.C

.cc.o:
	$(CC) $(CXXFLAGS) -c $*.cc

OBJ_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)
LIB_DIR := ../$(OBJ_DIR)

#LFLAGS = -g -L$(LIB_DIR) -L../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX)
override LFLAGS := -L$(LIB_DIR) -L../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX) $(DEBUG_FLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.C
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CXXFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.cc
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CXXFLAGS) -o $@ -c $<

INSTALL_APPS := recognizer

all:	recognizer

.PHONY:	recognizer
recognizer:	$(OBJ_DIR)/recognizer

$(OBJ_DIR)/recognizer: $(OBJ_DIR)/recognizer.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/recognizer \
		$(OBJ_DIR)/recognizer.o -lvrpn $(ARCH_LIBS)

install: all
	-mkdir -p $(BIN_DIR)
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )
	( cd $(OBJ_DIR) ; cp $(INSTALL_APPS) $(BIN_DIR) )
	( cd $(BIN_DIR) ; strip $(INSTALL_APPS) )

uninstall:
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )

clean:
	rm -f $(OBJ_DIR)/*

$(LIB_DIR)/libvrpn.a:


