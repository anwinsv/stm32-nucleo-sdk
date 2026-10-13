/**
 * @file    stm32g4_nvic.h
 * @brief   Nested Vectored Interrupt Controller (NVIC) driver for STM32G4.
 *
 * Provides a safe, validated interface for interrupt enabling/disabling,
 * priority configuration (enforcing Cortex-M4 Group 4 / PRIGROUP = 3),
 * pending status management, critical sections, and BASEPRI masking.
 */

#ifndef STM32G4_NVIC_H
#define STM32G4_NVIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32g4xx.h"

/**
 * @defgroup NVIC_Priority_Limits Priority Bounds
 * @{
 */
#define NVIC_PRIORITY_HIGHEST       (0U)
#define NVIC_PRIORITY_LOWEST        (15U)
/** @} */

/**
 * @brief NVIC driver return status codes.
 */
typedef enum {
    NVIC_OK = 0U,           /**< Operation completed successfully */
    NVIC_INVALID_IRQ,       /**< IRQn out of range, reserved gap, or non-configurable exception */
    NVIC_INVALID_PRIORITY,  /**< Priority value exceeds supported range (> 15) */
    NVIC_INVALID_PARAM      /**< Generic bad parameter (e.g. NULL pointer) */
} nvic_status_t;

/* ── Initialization ── */

/**
 * @brief Initialize the NVIC controller to Group 4 priority configuration.
 *
 * Configures the priority grouping in SCB->AIRCR to Group 4 (PRIGROUP = 3),
 * assigning all 4 priority bits to preemption priority (0..15) and 0 bits
 * to sub-priority. This aligns with FreeRTOS requirements and ensures
 * deterministic preemption behavior.
 *
 * @retval NVIC_OK on success.
 */
nvic_status_t nvic_init(void);

/* ── IRQ Enable / Disable / Query ── */

/**
 * @brief Enable a device interrupt in the NVIC.
 *
 * @param irqn Device interrupt number (must be >= 0 and valid for STM32G491).
 * @retval NVIC_OK on success.
 * @retval NVIC_INVALID_IRQ if irqn is negative, out of range, or reserved/gap.
 */
nvic_status_t nvic_irq_enable(IRQn_Type irqn);

/**
 * @brief Disable a device interrupt in the NVIC.
 *
 * @param irqn Device interrupt number (must be >= 0 and valid for STM32G491).
 * @retval NVIC_OK on success.
 * @retval NVIC_INVALID_IRQ if irqn is negative, out of range, or reserved/gap.
 */
nvic_status_t nvic_irq_disable(IRQn_Type irqn);

/**
 * @brief Check if a device interrupt is enabled in the NVIC.
 *
 * @param irqn Device interrupt number.
 * @retval 1 if the interrupt is enabled.
 * @retval 0 if disabled or if irqn is invalid.
 */
uint32_t nvic_irq_is_enabled(IRQn_Type irqn);

/* ── Priority Configuration ── */

/**
 * @brief Set the priority of a device interrupt or configurable system exception.
 *
 * Accepts a flat priority level from 0 (highest) to 15 (lowest), corresponding
 * to the 4 implemented priority bits in STM32G4.
 *
 * Valid system exceptions (negative IRQn):
 *   - MemoryManagement_IRQn (-12)
 *   - BusFault_IRQn (-11)
 *   - UsageFault_IRQn (-10)
 *   - SVCall_IRQn (-5)
 *   - DebugMonitor_IRQn (-4)
 *   - PendSV_IRQn (-2)
 *   - SysTick_IRQn (-1)
 *
 * @param irqn Interrupt number or configurable system exception.
 * @param priority Flat priority level (0..15).
 * @retval NVIC_OK on success.
 * @retval NVIC_INVALID_PRIORITY if priority > 15.
 * @retval NVIC_INVALID_IRQ if irqn is invalid or not configurable.
 */
nvic_status_t nvic_set_priority(IRQn_Type irqn, uint32_t priority);

/**
 * @brief Get the current priority of an interrupt or configurable system exception.
 *
 * @param irqn Interrupt number or configurable system exception.
 * @param[out] priority Pointer to store the retrieved priority level (0..15).
 * @retval NVIC_OK on success.
 * @retval NVIC_INVALID_PARAM if priority pointer is NULL.
 * @retval NVIC_INVALID_IRQ if irqn is invalid or not configurable.
 */
nvic_status_t nvic_get_priority(IRQn_Type irqn, uint32_t *priority);

/* ── Pending State Management ── */

/**
 * @brief Set the pending state of a device interrupt.
 *
 * Forces the interrupt to become pending in the NVIC.
 *
 * @param irqn Device interrupt number (>= 0).
 * @retval NVIC_OK on success.
 * @retval NVIC_INVALID_IRQ if irqn is negative, out of range, or reserved/gap.
 */
nvic_status_t nvic_set_pending(IRQn_Type irqn);

/**
 * @brief Clear the pending state of a device interrupt.
 *
 * @param irqn Device interrupt number (>= 0).
 * @retval NVIC_OK on success.
 * @retval NVIC_INVALID_IRQ if irqn is negative, out of range, or reserved/gap.
 */
nvic_status_t nvic_clear_pending(IRQn_Type irqn);

/**
 * @brief Check if a device interrupt is currently pending.
 *
 * @param irqn Device interrupt number.
 * @retval 1 if the interrupt is pending.
 * @retval 0 if not pending or if irqn is invalid.
 */
uint32_t nvic_is_pending(IRQn_Type irqn);

/* ── Active State Query ── */

/**
 * @brief Check if a device interrupt is active (currently executing in an ISR).
 *
 * @param irqn Device interrupt number.
 * @retval 1 if the interrupt is active.
 * @retval 0 if not active or if irqn is invalid.
 */
uint32_t nvic_is_active(IRQn_Type irqn);

/* ── Critical Section Management (PRIMASK-based) ── */

/**
 * @brief Enter a critical section by disabling all configurable interrupts.
 *
 * Reads and returns the current PRIMASK register value before setting PRIMASK
 * to 1. Safe to call from privileged thread or handler mode, before or after
 * the FreeRTOS scheduler starts.
 *
 * @return Previous PRIMASK register value (0 if interrupts were enabled, 1 if disabled).
 */
uint32_t nvic_enter_critical(void);

/**
 * @brief Exit a critical section by restoring the previous PRIMASK state.
 *
 * Restores PRIMASK to the state returned by nvic_enter_critical(), supporting
 * nested critical section blocks.
 *
 * @param saved_state The PRIMASK value returned by matching nvic_enter_critical().
 */
void nvic_exit_critical(uint32_t saved_state);

/* ── BASEPRI Threshold Masking ── */

/**
 * @brief Mask all interrupts with priority numerically equal or lower than the given threshold.
 *
 * Modifies the BASEPRI register. An interrupt with numeric priority >= priority
 * will be masked. Setting priority to 0 unmasks all configurable interrupts.
 *
 * @param priority Priority threshold (0..15). 0 disables BASEPRI masking.
 * @return Previous raw BASEPRI register value for restoring later.
 */
uint32_t nvic_mask_irqs_below(uint32_t priority);

/**
 * @brief Restore the BASEPRI register to a previously saved value.
 *
 * @param saved_basepri Raw BASEPRI value returned by nvic_mask_irqs_below().
 */
void nvic_restore_irq_mask(uint32_t saved_basepri);

/* ── System Reset ── */

/**
 * @brief Initiate a system reset request via SCB/NVIC.
 *
 * Requests the SCB to reset the processor core and on-chip peripherals.
 * This function does not return.
 */
void nvic_system_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32G4_NVIC_H */
