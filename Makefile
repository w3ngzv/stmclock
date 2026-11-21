ARM_CC  ?= arm-none-eabi-gcc
SIZE    ?= arm-none-eabi-size
OBJCOPY ?= arm-none-eabi-objcopy

# -fno-tree-loop-distribute-patterns: libc_min.c 里的 memcpy 循环不能被
# 编译器改写成对 memcpy 自身的调用
CFLAGS  := -mcpu=cortex-m3 -mthumb -Os -std=gnu11 -ffreestanding \
	-fno-tree-loop-distribute-patterns \
	-ffunction-sections -fdata-sections -Wall -Wextra -Werror
LDFLAGS := -nostdlib -T link/stm32f103c8.ld -Wl,--gc-sections \
	-Wl,-Map=build/stmclock.map
# 软浮点和 64 位运算的辅助函数来自 libgcc, newlib 的 libm 在本机工具链里不存在
LDLIBS  = $(shell $(ARM_CC) $(CFLAGS) -print-libgcc-file-name)

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))

OPENOCD := openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

all: build/stmclock.elf build/stmclock.hex build/stmclock.bin
	$(SIZE) build/stmclock.elf

build/%.o: src/%.c | build
	$(ARM_CC) $(CFLAGS) -c -o $@ $<

build/stmclock.elf: $(OBJS) link/stm32f103c8.ld
	$(ARM_CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

build/stmclock.hex: build/stmclock.elf
	$(OBJCOPY) -O ihex $< $@

build/stmclock.bin: build/stmclock.elf
	$(OBJCOPY) -O binary $< $@

build:
	mkdir -p build

flash: build/stmclock.elf
	$(OPENOCD) -c "program $< verify reset exit"

# 主机上的单元测试, 只编译不依赖寄存器的源文件
HOST_CC ?= cc
TEST_SRCS := src/calendar.c src/fmt.c

test: tests/test_logic.c $(TEST_SRCS) | build
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -Isrc -o build/test_logic $^
	build/test_logic

clean:
	rm -rf build

.PHONY: all flash test clean
