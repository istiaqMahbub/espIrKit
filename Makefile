#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = eagle
#FLAVOR = release
FLAVOR = debug

#EXTRA_CCFLAGS += -u

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS= eagle.app.v6.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    \
	driver \
	mqtt \
	user \
	modules \
	mesh \
	tcpServer \
	utilities \
	tcpServer \
	base64 \

endif # } PDIR

APPDIR = .
LDDIR = ../ld

CCFLAGS += -Os

TARGET_LDFLAGS =		\
	-nostdlib		\
	-Wl,-EL \
	--longcalls \
	--text-section-literals

ifeq ($(FLAVOR),debug)
    TARGET_LDFLAGS += -g -O2
endif

ifeq ($(FLAVOR),release)
    TARGET_LDFLAGS += -g -O0
endif


dummy: all

#This variable assignment actually only is here in order to run the shell commands, which
#will pull in libesphttpd if it isn't there yet.
force_submodule_update:=$(shell git submodule init) $(shell git submodule update)

COMPONENTS_eagle.app.v6 = \
	driver/libDriver.a \
	modules/libModules.a \
	mqtt/libMqtt.a \
	user/libUserMain.a \
	tcpServer/libTcpServer.a \
	utilities/libSysUtilities.a \
	mesh/libKnkMesh.a \
	tcpServer/libTcpServer.a \
	base64/libBase64.a \

LINKFLAGS_eagle.app.v6 = \
	-L../lib        \
	-nostdlib	\
    -T$(LD_FILE)   \
	-Wl,--no-check-sections	\
	-Wl,--gc-sections	\
    -u call_user_start	\
	-Wl,-static						\
	-Wl,--start-group					\
	-lc					\
	-lgcc					\
	-lhal					\
	-lphy	\
	-lpp	\
	-lnet80211	\
	-llwip	\
	-lwpa	\
	-lcrypto	\
	-lmain	\
	-lmesh \
	-lwps \
	-lsmartconfig \
	-ljson			\
	$(DEP_LIBS_eagle.app.v6)					\
	-Wl,--end-group
	#-lmesh \placre it after lmain when to be used
	
DEPENDS_eagle.app.v6 = \
                $(LD_FILE) \
                $(LDDIR)/eagle.rom.addr.v6.ld

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =		\

# Other potential configuration flags include:
#	-DTXRX_TXBUF_DEBUG
#	-DTXRX_RXBUF_DEBUG
#	-DWLAN_CONFIG_CCX
CONFIGURATION_DEFINES =	-DICACHE_FLASH \
                        -DESP_MESH_SUPPORT

DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

DDEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include -I $(PDIR)mqtt/include -I $(PDIR)SoftUart -I $(PDIR)slaveMeters/include -I $(PDIR)driver/include -I $(PDIR)modules/include -I $(PDIR)meterInterface/include  \
-I $(PDIR)utilities/include -I $(PDIR)mesh/include -I $(PDIR)tcpServer/include -I $(PDIR)base64/include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

.PHONY: FORCE
FORCE:

