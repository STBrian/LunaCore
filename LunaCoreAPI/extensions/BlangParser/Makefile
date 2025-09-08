.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR 		?= 	$(CURDIR)
include $(DEVKITARM)/3ds_rules

NAME		:=	BlangParser
BUILD		:= 	build
INCLUDES	:= 	include
SOURCES 	:= 	src

LC_TARGET_VERSION_MAJOR	:=	0
LC_TARGET_VERSION_MINOR	:=	13
LC_TARGET_VERSION_PATCH	:=	0

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH		:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS		:=	$(ARCH) -O2 -s -fPIC -ffreestanding -fno-builtin \
				-fdata-sections -fomit-frame-pointer -ffunction-sections -fno-strict-aliasing -fvisibility=hidden

CFLAGS		+=	$(INCLUDE) -D__3DS__ -D__LCEX__ -DLC_TARGET_VERSION_MAJOR=$(LC_TARGET_VERSION_MAJOR) \
				-DLC_TARGET_VERSION_MINOR=$(LC_TARGET_VERSION_MINOR) \
				-DLC_TARGET_VERSION_PATCH=$(LC_TARGET_VERSION_PATCH)

CXXFLAGS	:= $(CFLAGS) -g -fno-rtti -fno-exceptions -std=gnu++17

ASFLAGS		:=	$(ARCH)
LDFLAGS		:= -T $(TOPDIR)/lcex.ld $(ARCH) -L $(PORTLIBS)/lib -O2 -nostdlib -fpic -Wl,--gc-section,--strip-discarded,--strip-debug,-Map=test.map

LIBS		:=  -lctru
LIBDIRS		:= 	$(CTRPFLIB) $(CTRULIB) $(PORTLIBS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(NAME)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
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
#---------------------------------------------------------------------------------
clean:
	@echo clean ... 
	@rm -fr $(BUILD) $(OUTPUT).lcex $(OUTPUT).elf

re: clean all

#---------------------------------------------------------------------------------

else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).lcex : $(OFILES)
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
%.lcex: %.elf
#---------------------------------------------------------------------------------
	@echo creating $(notdir $@)
	@$(OBJCOPY) -O binary $(OUTPUT).elf $@

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
