.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
  $(error "Set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

LD       := $(CC)
LIBNDS   := $(DEVKITPRO)/libnds
CALICO   := $(DEVKITPRO)/calico

#---------------------------------------------------------------------------------
TARGET   := numberslide
BUILD    := build
SOURCES  := source \
            ../numberslide-common \
            ../numberslide-common/i18n \
            ../numberslide-common/sprites
INCLUDES := source \
            ../numberslide-common \
            ../numberslide-common/i18n \
            ../numberslide-common/sprites
#---------------------------------------------------------------------------------

ARCH     := -march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork

CFLAGS   := -g -Wall -O2 -fomit-frame-pointer -ffunction-sections -fdata-sections \
            $(ARCH) $(INCLUDE) -DARM9 -DNDS -D__NDS__
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS     := -lnds9
LIBDIRS  := $(LIBNDS)

GAME_ICON := $(DEVKITPRO)/calico/share/nds-icon.bmp
GAME_TITLE := NumberSlide

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(CURDIR)/$(dir)/*.c)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(CURDIR)/$(dir)/*.s)))

export OFILES   := $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CALICO)/include \
                   -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(CALICO)/lib

export CFLAGS LDFLAGS LIBS LD

.PHONY: $(BUILD) clean

$(BUILD): arm7/arm7_stub.elf
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

arm7/arm7_stub.elf: arm7/arm7_stub.s
	@arm-none-eabi-as -mcpu=arm7tdmi -o arm7/arm7_stub.o $<
	@arm-none-eabi-ld -Ttext=0x037F8000 -o $@ arm7/arm7_stub.o

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds arm7/*.o arm7/*.elf

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).nds : $(OUTPUT).elf $(TOPDIR)/arm7/arm7_stub.elf
	@ndstool -c $@ -h 0x200 -9 $< -7 $(TOPDIR)/arm7/arm7_stub.elf -b $(GAME_ICON) "$(GAME_TITLE);built with devkitARM;http://devkitpro.org"
	@echo built ... $(notdir $@)

$(OUTPUT).elf : $(OFILES)
	@echo linking $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

include $(DEVKITARM)/base_rules

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
