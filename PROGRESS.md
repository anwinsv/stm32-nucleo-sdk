# Progress

Last updated: 12 Oct 2026. Covers `main` and `feature/uart`.

## Summary

| Milestone | Status |
|-----------|--------|
| Bare-metal baseline from CubeMX (G491RE) | Done |
| CMake + Ninja build with custom paths and toolchains | Done |
| FreeRTOS kernel integration, context switching verified | Done |
| UART API layer (handle, config, validation, state) | Done (on `feature/uart`) |
| UART polling TX/RX | Implemented |
| UART low-level register layer (clocks, GPIO, BRR, CR1/CR2) | Implemented |
| UART interrupt sync / async TX/RX + generic ISR | Implemented |
| UART wired into the build, NVIC and `main.c` | **Not yet** |
| On-hardware verification of the SDK UART | **Not yet recorded** |

## Commit history

**`main`**

- `8ec3bf9` Initial commit
- `da622b4` chore(bsp): initialize bare-metal STM32G491RE baseline via CubeMX
- `48190ad` build(sdk): add custom paths and toolchains
- `bc234fa` docs: update README
- `a9ab2b8` freertos: integrate FreeRTOS kernel and verify context switching

**`feature/uart`** (on top of the above)

- `9130323` uart: add uart hal files
- `84ecef1` uart: add functionality
- `0f440a7` uart: separate hardware logic and implement polling
- `662cf80` uart: implement sync, async APIs and generic ISR

## What exists today

### Build and RTOS
- CMake + Ninja build, `arm-none-eabi-gcc` toolchain file, CMake presets.
- FreeRTOS kernel with the ARM_CM4F port and `heap_4`.
- `main.c` runs two tasks (LED blink on PA5, periodic `printf`) under the scheduler. Console output still goes through the Cube HAL / BSP `COM1`.

### UART driver (~1,250 lines across 4 files)
- **API layer** (`stm32g4_uart.c/.h`): opaque handle, static per-instance descriptors, config validation, separate TX/RX transfer state, status codes (`UART_TX_BUSY`, `UART_TIMEOUT`, `UART_NOT_OPEN`, ...), safe `uart_close`.
- **Low-level layer** (`stm32g4_uart_ll.c/.h`): RCC clock gating, GPIO alternate-function setup for six instances, kernel-clock selection via `RCC->CCIPR`, PCLK1/PCLK2 derivation, BRR calculation for USART (OVER8 and OVER16) and LPUART (256x), bit-level helpers for TXE / TC / RXNE and their interrupt enables.
- **Polling** TX/RX with software-loop timeouts.
- **Interrupt-driven** TX/RX: TXE -> TC state machine, RXNE receive, `ulTaskNotifyTake` / `vTaskNotifyGiveFromISR` for the sync calls, user callbacks for async calls, `portYIELD_FROM_ISR` on exit.

## Known gaps

These are the concrete things between "code written" and "UART usable". Items 1 to 3 are the next actions.

1. **Low-level source not in the build.** `CMakeLists.txt` lists `sdk/src/stm32g4_uart.c` but not `sdk/src/stm32g4_uart_ll.c`, so the `uart_ll_*` symbols will be unresolved at link time.
2. **No interrupt wiring.** `uart_irq_handler()` is not called from any `USARTx_IRQHandler`, and the NVIC enable and priority for each instance is not set anywhere. Priority must be at or below `configMAX_SYSCALL_INTERRUPT_PRIORITY` because the ISR calls FreeRTOS `FromISR` APIs.
3. **SDK UART unused by the application.** `main.c` has no reference to the SDK UART yet; it still uses `HAL_UART_Transmit` via the BSP. Replacing `_write` with `uart_write_polling` (and later `uart_write_sync`) would be the first real test.
4. **Error handling in the ISR.** Overrun, framing, parity and noise flags are not read or cleared. An RX overrun can leave RXNE-driven receive stuck, and `rx_cb` / `tx_cb` only ever receive `UART_OK`.
5. **Timeout cleanup.** After a sync timeout the state is `UART_TRANSFER_TIMED_OUT` and the interrupt is disabled, but `tx_task` / `rx_task` stay set; a later async call clears them, but the state machine would be cleaner with an explicit reset to idle.
6. **Polling timeouts are instruction-count based**, not time based, so the real duration depends on clock and optimisation level.
7. **Stale comments.** `stm32g4_uart.h` still says the layer "does not access RCC, GPIO, NVIC" and mentions a "HAL-only implementation", and the instance enum comment says LPUART1 is not covered while the enum and LL layer do support it. `FIFO` and flow-control settings are accepted in `uart_config_t` but not applied by the LL layer.
8. **Repo metadata.** GitHub's "About" text says NUCLEO-F446; the board is NUCLEO-G491RE.
9. **No tests or CI.** No hardware loopback test, no host-side unit tests, no GitHub Actions build.

## Hardware verification log

Nothing recorded yet for the SDK UART. Suggested first entries once tested:

| Date | Test | Instance | Result |
|------|------|----------|--------|
| | `uart_write_polling` echo to ST-LINK VCP | LPUART1 | |
| | `uart_read_polling` loopback | LPUART1 | |
| | `uart_write_sync` from a task | LPUART1 | |
| | `uart_read_sync` with timeout | LPUART1 | |
| | `uart_write_async` + callback | LPUART1 | |
