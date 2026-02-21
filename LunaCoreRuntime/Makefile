.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR 		?= 	$(CURDIR)
include $(DEVKITARM)/3ds_rules

CTRPFLIB	?=	$(DEVKITPRO)/libctrpf

TARGET		:= 	LunaCore
PLGINFO 	:= 	plgInfo.yaml

BUILD		:= 	Build
INCLUDES	:= 	includes externals/MC3DSPluginFramework/Includes
SOURCES 	:= 	src externals/MC3DSPluginFramework/Sources

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH		:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -marm

CFLAGS		:=	$(ARCH) $(INCLUDE) -D__3DS__ -Os -s -mword-relocations \
				-fdata-sections -fomit-frame-pointer -ffunction-sections -fno-strict-aliasing -fvisibility=hidden

ifeq ($(EXPERIMENTAL), 1)
	CFLAGS		+=	-DEXPERIMENTAL
endif
ifeq ($(DEBUG), 1)
	CFLAGS		+=	-DDEBUG
endif

CXXFLAGS	:= $(CFLAGS) -g -fno-rtti -fno-exceptions -std=gnu++20

ASFLAGS		:=	$(ARCH)
LDFLAGS		:= -T $(TOPDIR)/3gx.ld $(ARCH) -L $(PORTLIBS)/lib -Os -Wl,--gc-sections,--strip-discarded,--strip-debug,-Map=lunacore.map \
				-T $(TOPDIR)/linker_symbols.ld -Wl,--wrap=abort

LIBS		:=  -llua5.1 -lfslib -lctrpf -lctru -lffi
LIBDIRS		:= 	$(CTRPFLIB) $(CTRULIB) $(PORTLIBS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES			:=	$(addprefix ../,$(shell find $(SOURCES) -name "*.c"))
CPPFILES		:=	$(addprefix ../,$(shell find $(SOURCES) -name "*.cpp"))
SFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export LD 		:= 	$(CXX)
export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I $(CURDIR)/$(dir) ) \
					$(foreach dir,$(LIBDIRS),-I $(dir)/include) \
					-I $(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L $(dir)/lib)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ... 
	@rm -fr $(BUILD) $(shell find $(SOURCES) -name "*.o") $(shell find $(SOURCES) -name "*.d") $(OUTPUT).3gx $(OUTPUT).elf

re: clean all

#---------------------------------------------------------------------------------

else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).3gx : $(OFILES)
$(OUTPUT).elf : $(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
.PRECIOUS: %.elf
%.3gx: %.elf
#---------------------------------------------------------------------------------
	@echo creating $(notdir $@)
	@3gxtool -s $(word 1, $^) $(TOPDIR)/$(PLGINFO) $@
	@rm ../src/main.d ../src/main.o 

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
