ARM_CC  ?= arm-none-eabi-gcc
SIZE    ?= arm-none-eabi-size
OBJCOPY ?= arm-none-eabi-objcopy

CFLAGS  := -mcpu=cortex-m3 -mthumb -Os -std=gnu11 -ffreestanding \
	-ffunction-sections -fdata-sections -Wall -Wextra -Werror
LDFLAGS := -nostdlib -T link/stm32f103c8.ld -Wl,--gc-sections \
	-Wl,-Map=build/stmclock.map

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))

OPENOCD := openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

all: build/stmclock.elf build/stmclock.hex build/stmclock.bin
	$(SIZE) build/stmclock.elf

build/%.o: src/%.c | build
	$(ARM_CC) $(CFLAGS) -c -o $@ $<

build/stmclock.elf: $(OBJS) link/stm32f103c8.ld
	$(ARM_CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

build/stmclock.hex: build/stmclock.elf
	$(OBJCOPY) -O ihex $< $@

build/stmclock.bin: build/stmclock.elf
	$(OBJCOPY) -O binary $< $@

build:
	mkdir -p build

flash: build/stmclock.elf
	$(OPENOCD) -c "program $< verify reset exit"

clean:
	rm -rf build

.PHONY: all flash clean
