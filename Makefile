TARGET = Projeto_Gas

CMSIS_INC     = Drivers/CMSIS/Include
CMSIS_DEV_INC = Drivers/CMSIS/Device/ST/STM32F1xx/Include
HAL_INC       = Drivers/STM32F1xx_HAL_Driver/Inc
HAL_SRC       = Drivers/STM32F1xx_HAL_Driver/Src

CUBEIDE_TOOLS = /opt/st/stm32cubeide_2.1.1/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools
SYSROOT_INC   = $(CUBEIDE_TOOLS)/arm-none-eabi/include
LIBC_PATH     = $(CUBEIDE_TOOLS)/arm-none-eabi/lib/thumb/v7-m/nofp
LIBGCC_PATH   = /usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7-m/nofp

HAL_SRCS = \
    $(HAL_SRC)/stm32f1xx_hal.c \
    $(HAL_SRC)/stm32f1xx_hal_cortex.c \
    $(HAL_SRC)/stm32f1xx_hal_gpio.c \
    $(HAL_SRC)/stm32f1xx_hal_rcc.c \
    $(HAL_SRC)/stm32f1xx_hal_rcc_ex.c \
    $(HAL_SRC)/stm32f1xx_hal_uart.c \
    $(HAL_SRC)/stm32f1xx_hal_dma.c

HAL_OBJS = $(patsubst $(HAL_SRC)/%.c, obj/hal/%.o, $(HAL_SRCS))

OBJS = \
    obj/main.o \
    obj/stm32f1xx_it.o \
    obj/stm32f1xx_hal_msp.o \
    obj/syscalls.o \
    obj/system_stm32f1xx.o \
    obj/startup.o \
    $(HAL_OBJS)

CC      = clang
LD      = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

INCLUDES = \
    -ICore/Inc \
    -I$(CMSIS_INC) \
    -I$(CMSIS_DEV_INC) \
    -I$(HAL_INC) \
    -I$(SYSROOT_INC)

DEFINES = -DSTM32F103xB -DUSE_HAL_DRIVER

CFLAGS = \
    --target=arm-none-eabi -mcpu=cortex-m3 -mthumb \
    -nostdlib -ffreestanding -Wall -O2 \
    -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables \
    $(INCLUDES) $(DEFINES)

ASFLAGS = --target=arm-none-eabi -mcpu=cortex-m3 -mthumb

LDFLAGS = \
    -T STM32F103C8TX_FLASH.ld \
    -Map=$(TARGET).map \
    -L$(LIBGCC_PATH) -lgcc \
    -L$(LIBC_PATH) -lc

# -------------------------------------------------------
# Alvos principais
# -------------------------------------------------------
all: $(TARGET).bin

obj/main.o: Core/Src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/stm32f1xx_it.o: Core/Src/stm32f1xx_it.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/stm32f1xx_hal_msp.o: Core/Src/stm32f1xx_hal_msp.c
	$(CC) $(CFLAGS) -c $< -o $@

	$(CC) $(CFLAGS) -c $< -o $@

obj/syscalls.o: Core/Src/syscalls.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/system_stm32f1xx.o: Core/Src/system_stm32f1xx.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/startup.o: Core/Startup/startup_stm32f103c8tx.s
	$(CC) $(ASFLAGS) -c $< -o $@

obj/hal/%.o: $(HAL_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# -------------------------------------------------------
# Link e conversão
# -------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

# -------------------------------------------------------
# Diretórios
# -------------------------------------------------------
$(OBJS): | obj/ obj/hal/

obj/:
	mkdir -p obj

obj/hal/:
	mkdir -p obj/hal

# -------------------------------------------------------
# Utilitários
# -------------------------------------------------------
clean:
	rm -f obj/*.o obj/hal/*.o *.elf *.bin *.map

flash: all
	st-flash write $(TARGET).bin 0x8000000

reset:
	st-flash reset

.clangd:
	@echo "CompileFlags:" > .clangd
	@echo "  Add:" >> .clangd
	@echo "    - --target=arm-none-eabi" >> .clangd
	@echo "    - -mcpu=cortex-m3" >> .clangd
	@echo "    - -mthumb" >> .clangd
	@echo "    - -nostdlib" >> .clangd
	@echo "    - -ffreestanding" >> .clangd
	@echo "    - -DSTM32F103xB" >> .clangd
	@echo "    - -DUSE_HAL_DRIVER" >> .clangd
	@echo "    - -I$(CURDIR)/Core/Inc" >> .clangd
	@echo "    - -I$(CURDIR)/Drivers/CMSIS/Include" >> .clangd
	@echo "    - -I$(CURDIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Include" >> .clangd
	@echo "    - -I$(CURDIR)/Drivers/STM32F1xx_HAL_Driver/Inc" >> .clangd
	@echo "    - -I/opt/st/stm32cubeide_2.1.1/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools/arm-none-eabi/include" >> .clangd

.PHONY: all clean flash reset .clangd