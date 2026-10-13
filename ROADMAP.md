# Roadmap

Planned work for `stm32-nucleo-sdk`, in rough priority order. Items are intentions, not commitments; dates are deliberately omitted.

## Phase 1 - Finish and verify UART (current)

Goal: UART is merged to `main` and proven on the board.

- [ ] Add `sdk/src/stm32g4_uart_ll.c` to `CMakeLists.txt`; get a clean build
- [ ] Add `USARTx_IRQHandler` / `LPUART1_IRQHandler` entries that call `uart_irq_handler()`
- [ ] Add NVIC enable and priority setup (respect `configMAX_SYSCALL_INTERRUPT_PRIORITY`)
- [ ] Route `printf` (`_write`) through the SDK UART instead of the Cube HAL
- [ ] Hardware tests on LPUART1 (ST-LINK VCP): polling TX/RX, sync TX/RX, async + callbacks, timeouts
- [ ] Handle ORE / FE / NE / PE in the ISR, clear flags, report `UART_ERROR` through callbacks
- [ ] Clean up stale header comments; implement or remove FIFO and flow-control config fields
- [ ] Fix GitHub "About" text (NUCLEO-G491RE)
- [ ] Merge `feature/uart` into `main`

## Phase 2 - Make UART RTOS-ready

- [ ] Per-handle mutex so multiple tasks can share an instance safely
- [ ] Ring-buffer RX with `StreamBuffer` / `Queue` so data is not lost between `read` calls
- [ ] Timer-based (or tick-based) polling timeouts instead of loop counts
- [ ] DMA-backed TX/RX with idle-line detection
- [ ] Simple logging module on top of UART (levels, task-safe, non-blocking)

## Phase 3 - CAN Driver Development (FDCAN)

Goal: High-performance FDCAN driver supporting Classic CAN (2.0B) and CAN-FD, adhering to the API / Low-Level split.

- [ ] Low-level layer (`stm32g4_fdcan_ll.c/.h`):
  - [ ] RCC clock gating and GPIO AF setup for FDCAN instances
  - [ ] Core initialization (CCCR: normal mode, internal/external loopback, bus monitoring)
  - [ ] Bit timing configuration for nominal rate (up to 1 Mbps) and data rate with BRS (up to 5/8 Mbps)
  - [ ] Message RAM allocation (Tx event FIFO, dedicated Tx buffers, Rx FIFO0, Rx FIFO1)
  - [ ] Standard (11-bit) and extended (29-bit) filter bank configuration
- [ ] API layer (`stm32g4_fdcan.c/.h`):
  - [ ] Handle management and validation (`fdcan_open`, `fdcan_close`, `fdcan_config`)
  - [ ] Transmission API: Classic CAN 2.0B & CAN-FD with BRS (Bit Rate Switching)
  - [ ] Polling and interrupt-driven frame transfer APIs
- [ ] RTOS integration:
  - [ ] ISR dispatch via `FDCANx_IT0_IRQHandler` / `FDCANx_IT1_IRQHandler`
  - [ ] Queue/StreamBuffer based RX frame dispatch to consumer tasks
  - [ ] Per-instance mutex for thread-safe concurrent transmission
- [ ] Hardware verification:
  - [ ] Internal loopback verification
  - [ ] External transceiver loopback and USB-CAN adapter verification on Nucleo board

## Phase 4 - More peripherals

Follow the same API-layer / low-level-layer split for each:

- [ ] GPIO and EXTI (replace BSP LED/button usage)
- [ ] Timers: PWM, input capture, free-running time base
- [ ] I2C (master) with an on-board or breakout sensor as the test target
- [ ] SPI (master)
- [ ] ADC (with DMA) and DAC
- [ ] Watchdog (IWDG) and low-power modes

## Phase 5 - Reduce vendor dependency

- [ ] Own clock tree setup (replace `SystemClock_Config` from CubeMX)
- [ ] Own startup/vector table handling and SysTick/tick configuration
- [ ] Drop the Cube HAL and BSP from the build, keeping CMSIS device headers only

## Phase 6 - Quality and tooling

- [ ] GitHub Actions: configure + build on every push and PR
- [ ] Host-side unit tests for pure logic (BRR calculation, config validation, state machine) with mocked LL
- [ ] On-target test app with pass/fail output over UART
- [ ] `clang-format` / `clang-tidy` config, MISRA-oriented warning set
- [ ] Doxygen-generated API docs
- [ ] Per-peripheral usage examples under `examples/`
- [ ] Tagged releases and a changelog

## Ideas / maybe later

- Shell / command-line interface over UART
- Second board target (another Nucleo) to prove the LL/API split ports cleanly
- Bootloader with UART firmware update
- Rust bindings, to tie in with the `learn-rust` repo
