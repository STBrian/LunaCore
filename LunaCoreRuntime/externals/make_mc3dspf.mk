#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

CTRPFLIB	?=	$(DEVKITPRO)/libctrpf

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	mc3dspf
BUILD		:=	build
SOURCES		:=	Sources Sources/Api Sources/Extensions Sources/Helpers Sources/Hooks Sources/Asterium/AstHook \
				Sources/Minecraft/Common/Client/Game Sources/Minecraft/Common/Client/Gui \
				Sources/Minecraft/Common/Client/Gui/Components Sources/Minecraft/Common/Client/Gui/Gui Sources/Minecraft/Common/Client/Gui/Screens \
				Sources/Minecraft/Common/Client/Input Sources/Minecraft/Common/Client/Options Sources/Minecraft/Common/Client/Player \
				Sources/Minecraft/Common/Client/Renderer/Entity Sources/Minecraft/Common/Client/Renderer/Renderer \
				Sources/Minecraft/Common/NBT Sources/Minecraft/Common/Platform Sources/Minecraft/Common/Resources \
				Sources/Minecraft/Common/World Sources/Minecraft/Common/World/Entity \
				Sources/Minecraft/Common/World/Entity/Player Sources/Minecraft/Common/World/Inventory \
				Sources/Minecraft/Common/World/Item Sources/Minecraft/Common/World/Item/Crafting \
				Sources/Minecraft/Common/World/Level Sources/Minecraft/Common/World/Level/Block \
				Sources/Minecraft/Common/World/Level/BlockEntity Sources/Minecraft/Common/World/Level/Chunk \
				Sources/Minecraft/Common/World/Phys \
				Sources/Minecraft/gctr Sources/Minecraft/gstd \
				Sources/Minecraft/src-deps/Input Sources/Minecraft/src-deps/Renderer
#DATA		:=	data
INCLUDES	:=	Sources ../platform/include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations -Wno-psabi \
			-ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=c++20

ASFLAGS	:=	-g $(ARCH)

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(CTRPFLIB) $(CTRULIB)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/lib/lib$(TARGET).a

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

lib:
	@[ -d $@ ] || mkdir -p $@

$(BUILD): lib
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/../make_mc3dspf.mk

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) lib

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	$(OFILES)

#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------