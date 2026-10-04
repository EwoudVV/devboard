PREFIX ?= arm-none-eabi-
LIB := lib/libopencm3
CPU := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS := $(CPU) -DSTM32F4 -std=c11 -Os -g3 -Wall -Wextra -Werror \
          -ffunction-sections -fdata-sections -I$(LIB)/include -Isrc
LDFLAGS := $(CPU) -nostartfiles -Tstm32f411.ld -L$(LIB)/lib -Wl,--gc-sections \
           -Wl,-Map=build/devboard.map --specs=nano.specs --specs=nosys.specs
OBJ := build/main.o build/usb_serial.o build/button.o

.PHONY: all clean test flash
all: build/devboard.elf build/devboard.bin build/devboard.hex

$(LIB)/lib/libopencm3_stm32f4.a:
	@test -d $(LIB) || (echo 'Run sh setup.sh first'; exit 1)
	$(MAKE) -C $(LIB) TARGETS=stm32/f4 PREFIX=$(PREFIX)

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(PREFIX)gcc $(CFLAGS) -MMD -MP -c $< -o $@

build/devboard.elf: $(OBJ) $(LIB)/lib/libopencm3_stm32f4.a stm32f411.ld
	$(PREFIX)gcc $(LDFLAGS) $(OBJ) -lopencm3_stm32f4 -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group -o $@
	$(PREFIX)size $@

build/devboard.bin: build/devboard.elf
	$(PREFIX)objcopy -O binary $< $@

build/devboard.hex: build/devboard.elf
	$(PREFIX)objcopy -O ihex $< $@

test: | build
	cc -std=c11 -Wall -Wextra -Werror -Isrc tests/button_test.c src/button.c -o build/button_test
	./build/button_test

flash: build/devboard.elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $< verify reset exit"

clean:
	rm -rf build

-include $(OBJ:.o=.d)
