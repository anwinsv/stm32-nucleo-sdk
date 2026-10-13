/**
 * @file    stm32g4_nvic.c
 * @brief   Nested Vectored Interrupt Controller (NVIC) driver implementation.
 */

#include "stm32g4_nvic.h"

/**
 * @brief Maximum implemented device IRQ number for STM32G491.
 */
#define NVIC_MAX_DEVICE_IRQN        (101)

/**
 * @brief Priority grouping setting for Group 4 (PRIGROUP = 3).
 *
 * In Cortex-M4 with 4 priority bits (__NVIC_PRIO_BITS = 4):
 *   PRIGROUP = 3 -> 4 bits preemption priority (0..15), 0 bits sub-priority.
 */
#define NVIC_PRIGROUP_GROUP4        (3U)

/**
 * @brief Validity bitmap for device IRQs 0 to 101 on STM32G491xx.
 *
 * Total device IRQs = 102 (spanning 4 x 32-bit words).
 * A bit set to 1 indicates the IRQ is populated in hardware.
 * Unpopulated gaps (e.g. 48, 50, 61-62, 66-74, 82-85, 88-89) are 0.
 *
 * - Word 0 (IRQs  0..31): 0xFFFFFFFFU (all populated)
 * - Word 1 (IRQs 32..63): 0x9FFAFFFFU (gaps at 48, 50, 61, 62)
 * - Word 2 (IRQs 64..95): 0xFCC3F803U (valid: 64-65, 75-81, 86-87, 90-95)
 * - Word 3 (IRQs 96..101): 0x0000003FU (bits 0..5 valid for IRQs 96..101)
 */
static const uint32_t irq_valid_bitmap[4] = {
    0xFFFFFFFFU,
    0x9FFAFFFFU,
    0xFCC3F803U,
    0x0000003FU
};

/**
 * @brief Internal helper to validate a device IRQ against the population bitmap.
 *
 * @param irqn IRQ number to check.
 * @return true if valid and populated, false otherwise.
 */
static inline bool is_valid_device_irq(IRQn_Type irqn)
{
    if (((int32_t)irqn < 0) || ((int32_t)irqn > NVIC_MAX_DEVICE_IRQN))
    {
        return false;
    }

    uint32_t n = (uint32_t)irqn;
    return ((irq_valid_bitmap[n >> 5U] & (1U << (n & 0x1FU))) != 0U);
}

/**
 * @brief Internal helper to validate if a system exception supports priority configuration.
 *
 * Configurable system exceptions on Cortex-M4:
 *   - MemoryManagement_IRQn (-12)
 *   - BusFault_IRQn (-11)
 *   - UsageFault_IRQn (-10)
 *   - SVCall_IRQn (-5)
 *   - DebugMonitor_IRQn (-4)
 *   - PendSV_IRQn (-2)
 *   - SysTick_IRQn (-1)
 *
 * Reset (-15), NMI (-14), and HardFault (-13) have fixed hardware priorities
 * and are not configurable.
 *
 * @param irqn System exception number to check.
 * @return true if priority can be configured, false otherwise.
 */
static inline bool is_valid_system_exception(IRQn_Type irqn)
{
    return (irqn == MemoryManagement_IRQn ||
            irqn == BusFault_IRQn         ||
            irqn == UsageFault_IRQn       ||
            irqn == SVCall_IRQn           ||
            irqn == DebugMonitor_IRQn     ||
            irqn == PendSV_IRQn           ||
            irqn == SysTick_IRQn);
}

/* ── Public API Implementation ── */

nvic_status_t nvic_init(void)
{
    __NVIC_SetPriorityGrouping(NVIC_PRIGROUP_GROUP4);
    return NVIC_OK;
}

nvic_status_t nvic_irq_enable(IRQn_Type irqn)
{
    if (!is_valid_device_irq(irqn))
    {
        return NVIC_INVALID_IRQ;
    }

    __NVIC_EnableIRQ(irqn);
    return NVIC_OK;
}

nvic_status_t nvic_irq_disable(IRQn_Type irqn)
{
    if (!is_valid_device_irq(irqn))
    {
        return NVIC_INVALID_IRQ;
    }

    __NVIC_DisableIRQ(irqn);
    return NVIC_OK;
}

uint32_t nvic_irq_is_enabled(IRQn_Type irqn)
{
    if (!is_valid_device_irq(irqn))
    {
        return 0U;
    }

    return __NVIC_GetEnableIRQ(irqn);
}

nvic_status_t nvic_set_priority(IRQn_Type irqn, uint32_t priority)
{
    if (priority > NVIC_PRIORITY_LOWEST)
    {
        return NVIC_INVALID_PRIORITY;
    }

    if ((int32_t)irqn >= 0)
    {
        if (!is_valid_device_irq(irqn))
        {
            return NVIC_INVALID_IRQ;
        }
    }
    else
    {
        if (!is_valid_system_exception(irqn))
        {
            return NVIC_INVALID_IRQ;
        }
    }

    __NVIC_SetPriority(irqn, priority);
    return NVIC_OK;
}

nvic_status_t nvic_get_priority(IRQn_Type irqn, uint32_t *priority)
{
    if (priority == NULL)
    {
        return NVIC_INVALID_PARAM;
    }

    if ((int32_t)irqn >= 0)
    {
        if (!is_valid_device_irq(irqn))
        {
            return NVIC_INVALID_IRQ;
        }
    }
    else
    {
        if (!is_valid_system_exception(irqn))
        {
            return NVIC_INVALID_IRQ;
        }
    }

    *priority = __NVIC_GetPriority(irqn);
    return NVIC_OK;
}

nvic_status_t nvic_set_pending(IRQn_Type irqn)
{
    if (!is_valid_device_irq(irqn))
    {
        return NVIC_INVALID_IRQ;
    }

    __NVIC_SetPendingIRQ(irqn);
    return NVIC_OK;
}

nvic_status_t nvic_clear_pending(IRQn_Type irqn)
{
    if (!is_valid_device_irq(irqn))
    {
        return NVIC_INVALID_IRQ;
    }

    __NVIC_ClearPendingIRQ(irqn);
    return NVIC_OK;
}

uint32_t nvic_is_pending(IRQn_Type irqn)
{
    if (!is_valid_device_irq(irqn))
    {
        return 0U;
    }

    return __NVIC_GetPendingIRQ(irqn);
}

uint32_t nvic_is_active(IRQn_Type irqn)
{
    if (!is_valid_device_irq(irqn))
    {
        return 0U;
    }

    return __NVIC_GetActive(irqn);
}

uint32_t nvic_enter_critical(void)
{
    uint32_t saved_primask = __get_PRIMASK();
    __disable_irq();
    return saved_primask;
}

void nvic_exit_critical(uint32_t saved_state)
{
    __set_PRIMASK(saved_state);
}

uint32_t nvic_mask_irqs_below(uint32_t priority)
{
    uint32_t saved_basepri = __get_BASEPRI();

    if (priority > NVIC_PRIORITY_LOWEST)
    {
        return saved_basepri;
    }

    if (priority == 0U)
    {
        __set_BASEPRI(0U);
    }
    else
    {
        __set_BASEPRI(priority << (8U - __NVIC_PRIO_BITS));
    }

    return saved_basepri;
}

void nvic_restore_irq_mask(uint32_t saved_basepri)
{
    __set_BASEPRI(saved_basepri);
}

void nvic_system_reset(void)
{
    __NVIC_SystemReset();
}
