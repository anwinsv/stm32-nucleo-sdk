# stm32-nucleo-sdk

A bare-metal-style SDK for the **STM32 Nucleo-G491RE** (STM32G491RE, Cortex-M4F @ 170 MHz) with **FreeRTOS** integration.

Peripheral drivers talk to the silicon through CMSIS register access and are wrapped in FreeRTOS primitives (task notifications today, more to follow). The goal is a small, readable SDK with a clean split between a hardware-agnostic API layer and a low-level register layer.

> [!NOTE]
> **Status:** Work in progress.
> - 🗺️ **[ROADMAP.md](ROADMAP.md)**: Architecture milestones, upcoming drivers, and long-term plans.
> - 📊 **[PROGRESS.md](PROGRESS.md)**: Current completion status, verified features, and known gaps.

## Features

| Area | State |
|------|-------|
| CMake + Ninja build with `arm-none-eabi-gcc` | Working |
| FreeRTOS kernel (ARM_CM4F port, `heap_4`) | Integrated, context switching verified |
| UART driver: polling, interrupt sync, interrupt async | Implemented on `feature/uart` (not yet merged) |
| UART ISR dispatch, NVIC setup, DMA, RTOS-aware locking | Planned |

## Tech stack

- **MCU / board:** STM32G491RE on NUCLEO-G491RE
- **RTOS:** FreeRTOS, ARM_CM4F port, `heap_4.c`
- **Build:** CMake (>= 3.22) + Ninja
- **Toolchain:** `arm-none-eabi-gcc` (an `starm-clang` toolchain file is also present)
- **Hardware access:** CMSIS register access in the SDK; STM32Cube HAL and the Nucleo BSP are still used for startup, clock config, LED/button and the current `printf` console

## Repository layout

```
.
├── Core/                 CubeMX-generated application code (main.c, IRQ handlers, GPIO, MSP)
├── Drivers/              Vendor code: CMSIS, STM32G4xx HAL, Nucleo BSP
├── middleware/FreeRTOS/  FreeRTOS kernel, ARM_CM4F port, heap implementations
├── sdk/                  Custom SDK (on feature/uart)
│   ├── inc/              Public headers
│   │   ├── stm32g4_uart.h       UART API (opaque handle, config, transfer calls)
│   │   └── stm32g4_uart_ll.h    Low-level register interface (internal)
│   └── src/
│       ├── stm32g4_uart.c       UART driver logic, state machine, FreeRTOS hooks
│       └── stm32g4_uart_ll.c    Clocks, GPIO AF, USART/LPUART registers, baud rate
├── cmake/                Toolchain files and CubeMX CMake glue
├── CMakeLists.txt
├── CMakePresets.json
├── nucleo-g491.ioc       CubeMX project
├── PROGRESS.md           Current development progress & gap analysis
├── README.md             Project documentation
├── ROADMAP.md            Project roadmap and milestones
└── STM32G491xx_FLASH.ld  Linker script
```

Vendor code is kept separate from the SDK and middleware so it can be updated without touching custom code.

## Architecture

Each driver is split into two layers:

1. **API layer** (`stm32g4_<periph>.c/.h`) owns handle lifetime, configuration validation, transfer state and RTOS synchronisation. It never touches registers.
2. **Low-level layer** (`stm32g4_<periph>_ll.c/.h`) owns every register access: RCC clock gating, GPIO alternate functions, peripheral registers.

Application code only sees an opaque `uart_handle_t`, so driver state cannot be modified from outside.

## UART driver (`feature/uart`)

Supported instances: USART1, USART2, USART3, UART4, UART5, LPUART1.

Configurable: baud rate, word length (7/8/9), parity, stop bits, oversampling (16/8), mode (TX/RX/TX+RX), plus per-direction completion callbacks.

| Call | Behaviour |
|------|-----------|
| `uart_open` / `uart_close` | Acquire / release a driver-owned handle; enables clocks and GPIO AF on open |
| `uart_config` | Validates settings and programs CR1/CR2/BRR |
| `uart_write_polling` / `uart_read_polling` | Blocking, scheduler-independent, usable before FreeRTOS starts |
| `uart_write_sync` / `uart_read_sync` | Interrupt-driven; calling task blocks on a task notification with a tick timeout |
| `uart_write_async` / `uart_read_async` | Interrupt-driven; returns immediately and invokes `tx_cb` / `rx_cb` on completion |
| `uart_irq_handler` | Generic ISR body; call it from the instance's `USARTx_IRQHandler` |

Default pin map used by the low-level layer:

| Instance | TX | RX |
|----------|----|----|
| USART1 | PC4 | PC5 |
| USART2 | PD5 | PD6 |
| USART3 | PB10 | PB11 |
| UART4 | PC10 | PC11 |
| UART5 | PC12 | PD2 |
| LPUART1 | PA2 | PA3 (ST-LINK virtual COM port) |

Minimal usage:

```c
uart_handle_t uart = uart_open(UART_INSTANCE_LPUART1);

uart_config_t cfg = {
    .baudrate     = 115200U,
    .word_length  = UART_WORD_LENGTH_8,
    .parity       = UART_PARITY_NONE,
    .stop_bits    = UART_STOP_BITS_1,
    .oversampling = UART_OVERSAMPLING_16,
    .mode         = UART_MODE_TX_RX,
};
uart_config(uart, &cfg);

const uint8_t msg[] = "hello\r\n";
uart_write_polling(uart, msg, sizeof msg - 1U, NULL);
```

> **Note:** the interrupt-driven calls need the instance's NVIC interrupt enabled and its IRQ handler forwarding to `uart_irq_handler()`. That wiring is not in the repository yet; see [PROGRESS.md](PROGRESS.md#known-gaps).

## Building

Requirements: CMake, Ninja and the Arm GNU toolchain (`arm-none-eabi-gcc`) on your `PATH`.

```sh
# 1. Configure
cmake -G Ninja -B build "-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake"

# 2. Build
ninja -C build

# 3. Flash build/nucleo-g491.elf (or .bin / .hex)
#    with STM32CubeProgrammer or OpenOCD
```

## Branches

| Branch | Purpose |
|--------|---------|
| `main` | Baseline: CubeMX BSP, CMake build, FreeRTOS integrated |
| `feature/freertos-port` | FreeRTOS integration work |
| `feature/uart` | UART driver (API layer, low-level layer, polling / sync / async) |

## Contributing / feedback

This is a personal learning and portfolio project; issues and suggestions are welcome.
