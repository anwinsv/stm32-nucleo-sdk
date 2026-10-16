# Progress

Last updated: 16 Oct 2026. Covers `main`, `feature/uart`, `feature/nvic`, and `test/nvic-uart`.

## Summary

| Milestone | Status |
|-----------|--------|
| Bare-metal baseline from CubeMX (G491RE) | Done |
| CMake + Ninja build with custom paths and toolchains | Done |
| FreeRTOS kernel integration, context switching verified | Done |
| UART API layer (handle, config, validation, state) | Done |
| UART polling TX/RX | Implemented |
| UART low-level register layer (clocks, GPIO, BRR, CR1/CR2) | Implemented |
| UART interrupt sync / async TX/RX + generic ISR | Implemented |
| NVIC driver (Group 4 priority, IRQ validation bitmap, BASEPRI) | Done |
| UART wired into the build, NVIC and `main.c` | Done |
| On-hardware verification of the SDK UART & NVIC | Done (16 Oct 2026) |

## Commit history

**`main`**

- `8ec3bf9` Initial commit
- `da622b4` chore(bsp): initialize bare-metal STM32G491RE baseline via CubeMX
- `48190ad` build(sdk): add custom paths and toolchains
- `bc234fa` docs: update README
- `a9ab2b8` freertos: integrate FreeRTOS kernel and verify context switching
- `40ed8b4` freertos: hook HAL_IncTick to SysTick & enable stack/malloc guards
- `304e73c` docs: update README and add PROGRESS and ROADMAP with FDCAN milestones

**`test/nvic-uart`** (integration branch)

- `8838b07` nvic: implement NVIC driver and integrate into build
- `3ea7d25` build(cmake): update uart source paths and add uart_ll to build
- `bdf2710` uart: wire NVIC interrupt handling and add hardware error clearing
- `7c123e1` examples: add interactive uart and nvic test application

## What exists today

### Build and RTOS
- CMake + Ninja build, `arm-none-eabi-gcc` toolchain file, CMake presets.
- FreeRTOS kernel with the ARM_CM4F port and `heap_4`.
- `main.c` runs `vBlinkLEDTask` (PA5 toggle) and launches `uart_example_start()` under the scheduler.

### NVIC driver (`stm32g4_nvic.c/.h`)
- Group 4 priority grouping (PRIGROUP = 3, 4-bit preemption, 0 sub-priority).
- 128-bit IRQ population bitmap validating all 102 STM32G491 IRQs and rejecting unpopulated gaps.
- Safe system exception priority configuration, PRIMASK critical sections, and BASEPRI threshold masking.

### UART driver
- **API layer** (`stm32g4_uart.c/.h`): opaque handle, static per-instance descriptors, config validation, separate TX/RX transfer state, status codes, safe `uart_close`, NVIC priority setup (`priority = 6`) and vector stubs (`USART1..3`, `UART4..5`, `LPUART1`).
- **Low-level layer** (`stm32g4_uart_ll.c/.h`): RCC clock gating, GPIO alternate-function setup, kernel-clock selection, BRR calculation for USART and LPUART (256x), TXE / TC / RXNE interrupt controls, and `uart_ll_clear_errors()` for ORE/FE/PE/NE clearing.
- **Polling & Interrupt TX/RX**: Verified on physical hardware via `examples/uart_example.c`.

## Known gaps

1. ~~**Low-level source not in the build.**~~ (Resolved in `3ea7d25`)
2. ~~**No interrupt wiring.**~~ (Resolved in `bdf2710` — NVIC priority set to 6, enabled in `uart_open`, ISR entry points dispatching to `uart_irq_handler`)
3. ~~**SDK UART unused by the application.**~~ (Resolved in `7c123e1` — wired via `examples/uart_example.c` in `main.c`)
4. ~~**Error handling in the ISR.**~~ (Resolved in `bdf2710` — `uart_ll_clear_errors` clears ORE, FE, NE, PE in hardware)
5. **Timeout cleanup.** After a sync timeout the state is `UART_TRANSFER_TIMED_OUT` and the interrupt is disabled, but `tx_task` / `rx_task` stay set; a later async call clears them, but the state machine would be cleaner with an explicit reset to idle.
6. **Polling timeouts are instruction-count based**, not time based, so the real duration depends on clock and optimisation level.
7. **Stale comments.** `stm32g4_uart.h` still says the layer "does not access RCC, GPIO, NVIC" and mentions a "HAL-only implementation", and the instance enum comment says LPUART1 is not covered while the enum and LL layer do support it. `FIFO` and flow-control settings are accepted in `uart_config_t` but not applied by the LL layer.
8. **Repo metadata.** GitHub's "About" text says NUCLEO-F446; the board is NUCLEO-G491RE.
9. **No tests or CI.** No hardware loopback test, no host-side unit tests, no GitHub Actions build.

## Hardware verification log

Verified on physical NUCLEO-G491RE via ST-LINK Virtual COM port (115200 8N1):

| Date | Test | Instance | Result |
|------|------|----------|--------|
| 16 Oct 2026 | `uart_write_polling` boot banner | LPUART1 | **PASS** |
| 16 Oct 2026 | `uart_write_sync` interrupt-driven TX + TaskNotify | LPUART1 | **PASS** |
| 16 Oct 2026 | `uart_read_sync` interrupt-driven RX + echo loop | LPUART1 | **PASS** |

### Verified Output (PuTTY Session)
```text
========================================
 [Step 1] Polling TX: SUCCESS
 Clock gating, BRR & GPIO AF configured.
 [Step 2] Interrupt Sync TX: SUCCESS
 NVIC delivered TXE/TC IRQ; FreeRTOS task notified.
========================================
 [Step 3] Interactive Echo Ready!
 Type a message and press Enter:
> hello world
[ECHO]: "hello world"
> 
```
