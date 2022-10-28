FLASH_SIZE = 2048
IRAM_SIZE  = 256
XRAM_SIZE  = 256

SRC        = src
SOURCES    = $(SRC)/main.c
SOURCES    += $(SRC)/core.c
SOURCES    += $(SRC)/spi.c
SOURCES    += $(SRC)/nrf24.c
SOURCES    += $(SRC)/motor_control.c
SOURCES    += $(SRC)/led.c
SOURCES    += $(SRC)/radio.c
SOURCES    += $(SRC)/state.c
#SOURCES    += $(SRC)/task.c
#SOURCES    += $(SRC)/scheduler.c
#SOURCES    += $(SRC)/motor_control.c

INCLUDES   = -Iincludes -Iincludes/shared -Iincludes/EFM8BB1/inc
BUILD_DIR  = build
FIRMWARE   = $(BUILD_DIR)/firmware.hex
MEMORY_MAP = $(patsubst %.hex,%.mem,$(FIRMWARE))


PY_FLASHER = efm8-arduino-programmer/flash36.py
PY_FLASHER_PORT ?= /dev/ttyACM0


RELS     = $(patsubst src/%.c,$(BUILD_DIR)/%.rel,$(SOURCES))
RELS_LIB = build/spi.rel build/core.rel build/led.rel build/nrf24.rel build/task.rel build/radio.rel build/scheduler.rel build/motor_control.rel


CC = sdcc
CFLAGS = -mmcs51 --std-sdcc99 --opt-code-size --model-small
#CFLAGS += --code-size $(FLASH_SIZE)
CFLAGS += --iram-size $(IRAM_SIZE)
CFLAGS += --xram-size $(XRAM_SIZE)
CFLAGS += $(INCLUDES)



all: $(FIRMWARE)

$(FIRMWARE): $(RELS)
	@echo Creating hex to: $@
	@$(CC) $(CFLAGS) $^ -o $@


# src/%. limits source code to come from this dir...
$(BUILD_DIR)/%.rel : $(SRC)/%.c
	@echo CC $<
	@$(CC) -c $(CFLAGS) $< -o $@


mem: $(FIRMWARE)
	@cat $(MEMORY_MAP)


lib: $(RELS)
	sdar -rc ASD.lib $(RELS_LIB)


clean:
	rm -rf $(BUILD_DIR)/*


flash_bee: $(FIRMWARE)
	python3 $(PY_FLASHER) $(PY_FLASHER_PORT) $(FIRMWARE)


# Utilities to discover connected devices and flash using Simplicity studio tools
SIMPLICITY_STUDIO_PATH = /home/victor/opt/SimplicityStudio_v5
ADAPTER_PACKS_PATH = "$(SIMPLICITY_STUDIO_PATH)/developer/adapter_packs"
FLASHER = ${ADAPTER_PACKS_PATH}/c8051/flash8051
SerialNo = LCK0068010

discover:
	${ADAPTER_PACKS_PATH}/inspect_c8051/device8051 -slist

flash: $(FIRMWARE)
	${FLASHER} -sn ${SerialNo} -tif c2 -erasemode page -upload $(FIRMWARE)
