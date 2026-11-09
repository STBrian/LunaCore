.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR 		?= 	$(CURDIR)
include $(DEVKITARM)/3ds_rules

CTRPFLIB	?=	$(DEVKITPRO)/libctrpf

TARGET		:= 	LunaCore
PLGINFO 	:= 	plgInfo.yaml

BUILD		:= 	build
INCLUDES	:= 	includes externals/platform/include externals/MC3DSPluginFramework/Sources
SOURCES 	:= 	src

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH		:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -marm

CFLAGS		:=	$(ARCH) $(INCLUDE) -D__3DS__ -Os -s -mword-relocations \
				-fdata-sections -fomit-frame-pointer -ffunction-sections -fno-strict-aliasing -fvisibility=hidden \
				$(CFLAGS_EXTRA)

CFLAGS		+=	-DCOMMIT_HASH="\"$(shell git rev-parse --short HEAD)\"" \
				-DLUNACORE_VER_MAJOR=$(shell grep -oP 'Major:\s+\K\d+' $(TOPDIR)/$(PLGINFO)) \
				-DLUNACORE_VER_MINOR=$(shell grep -oP 'Minor:\s+\K\d+' $(TOPDIR)/$(PLGINFO)) \
				-DLUNACORE_VER_PATCH=$(shell grep -oP 'Revision:\s+\K\d+' $(TOPDIR)/$(PLGINFO)) \
				-D__LCRUNTIME__

ifeq ($(EXPERIMENTAL), 1)
	CFLAGS		+=	-DEXPERIMENTAL
endif
ifeq ($(DEBUG), 1)
	CFLAGS		+=	-DDEBUG
endif

CXXFLAGS	:= 	$(CFLAGS) -g -fno-rtti -fno-exceptions -std=gnu++20

ASFLAGS		:=	$(ARCH)
LDFLAGS		:= 	-T $(TOPDIR)/3gx.ld $(ARCH) -L $(PORTLIBS)/lib -Os \
				-Wl,--gc-sections,--strip-discarded,--strip-debug,-Map=lunacore.map \
				-T $(TOPDIR)/linker_symbols.ld -Wl,--wrap=abort

LIBS		:= -lctrpf -lctru -lmc3dspf -lFsLib -lffi
ifeq ($(BUILD_JIT), 1)
	LIBS	+=	-lluajit-5.1
else
	LIBS	+= -llua5.1
endif
LIBDIRS		:= 	$(CTRPFLIB) $(CTRULIB) $(PORTLIBS) $(CURDIR)/externals/MC3DSPluginFramework \
				$(CURDIR)/externals/FsLib/3DS/FsLib $(CURDIR)/externals/ffi $(CURDIR)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

SRC_DIRS		:=	$(foreach dir,$(SOURCES),$(shell find $(CURDIR)/$(dir) -type d))
export VPATH	:=	$(SRC_DIRS) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

ALL_SRCS		:=	$(shell find $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.s" \))
SRC_BASENAMES	:=	$(notdir $(ALL_SRCS))
DUPLICATES		:=	$(strip $(foreach f,$(sort $(SRC_BASENAMES)),$(if $(filter-out 1,$(words $(filter $(f),$(SRC_BASENAMES)))),$(f))))

ifneq ($(DUPLICATES),)
$(error Se detectaron archivos fuente con el mismo nombre en diferentes directorios: $(DUPLICATES). Esto provocaria que los archivos .o y .d se sobreescriban silenciosamente en $(BUILD)/. Archivos en conflicto: $(foreach d,$(DUPLICATES),$(filter %/$(d),$(ALL_SRCS))))
endif

CFILES			:=	$(notdir $(filter %.c,$(ALL_SRCS)))
CPPFILES		:=	$(notdir $(filter %.cpp,$(ALL_SRCS)))
SFILES			:=	$(notdir $(filter %.s,$(ALL_SRCS)))

export LD 		:= 	$(CXX)
export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I $(CURDIR)/$(dir) ) \
					$(foreach dir,$(LIBDIRS),-I $(dir)/include) \
					-I $(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L $(dir)/lib)

.PHONY: $(BUILD) compile_mc3dspf clean all

#---------------------------------------------------------------------------------
all: compile_libffi compile_mc3dspf compile_fslib $(BUILD)

compile_mc3dspf:
	@$(MAKE) --no-print-directory -C externals/MC3DSPluginFramework

compile_fslib:
	@$(MAKE) --no-print-directory -C externals/FsLib/3DS/FsLib

compile_libffi:
	@$(MAKE) --no-print-directory -C externals/libffi -f $(CURDIR)/externals/make_libffi.mk

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ... 
	@rm -fr $(BUILD) Build $(shell find $(SOURCES) -name "*.o" 2>/dev/null) $(shell find $(SOURCES) -name "*.d" 2>/dev/null) $(OUTPUT).3gx $(OUTPUT).elf

clean_all: clean
	@$(MAKE) -C externals/MC3DSPluginFramework clean
	@$(MAKE) -C externals/FsLib/3DS/FsLib clean
	@$(MAKE) -C externals/libffi -f $(CURDIR)/externals/make_libffi.mk clean

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
	@rm -f main.d main.o 

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
