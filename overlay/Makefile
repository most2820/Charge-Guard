#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

APP_TITLE	:=	Charge Guard
APP_VERSION	:=	1.0.0
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include
NO_ICON		:=  1

include ${TOPDIR}/libs/libultrahand/ultrahand.mk

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+simd+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -Wno-address-of-packed-member -O3 -ffunction-sections -ffast-math -fno-finite-math-only -flto -fomit-frame-pointer \
            -fuse-linker-plugin -finline-small-functions \
            -fno-strict-aliasing -frename-registers -falign-functions=16 \
			$(ARCH) $(DEFINES)

# Enable appearance overriding
UI_OVERRIDE_PATH := /config/charge-guard/
CFLAGS += -DUI_OVERRIDE_PATH="\"$(UI_OVERRIDE_PATH)\""

# Exception wrap utilization (for smaller compilation size)
CFLAGS += -DUSE_EXCEPTION_WRAP=1

# Requires USE_EXCEPTION_WRAP and inclusion of exception_wrap.hpp in main
LDFLAGS += -Wl,-wrap,__cxa_throw \
           -Wl,-wrap,_Unwind_Resume \
           -Wl,-wrap,__gxx_personality_v0


CFLAGS	+=	$(INCLUDE) -D__SWITCH__ -DAPP_VERSION="\"$(APP_VERSION)\""

CXXFLAGS	:= $(CFLAGS) -std=c++23 -Wno-dangling-else -fno-unwind-tables -fno-asynchronous-unwind-tables

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	+=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lnx

CXXFLAGS += -fno-exceptions -ffunction-sections -fdata-sections -fno-rtti
LDFLAGS += -Wl,--gc-sections -Wl,--as-needed

CXXFLAGS += -flto -fuse-linker-plugin -flto=6
LDFLAGS += -flto=6

LIBDIRS	:= $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo "Cleaning ... $(TARGET)"
	@rm -fr $(BUILD) $(TARGET).ovl $(TARGET).nro $(TARGET).nacp $(TARGET).elf

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

all	:	 $(OUTPUT).ovl

$(OUTPUT).ovl		:	$(OUTPUT).elf $(OUTPUT).nacp 
	@elf2nro $< $@ $(NROFLAGS)
	@echo "built ... $(notdir $(OUTPUT).ovl)"
	@printf 'ULTR' >> $@
	@printf "Ultrahand signature has been added.\n"

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

%.bin.o	%_bin.h :	%.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
