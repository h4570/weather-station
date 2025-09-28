#include "app/drivers/spi_bus_manager.h"

/* ------------------------- Internal helpers/macros ------------------------- */

#define SPI_Q_INCR(i, cap) (uint16_t)(((i) + 1) % (cap))
#define SPI_Q_EMPTY(m) ((m)->q_head == (m)->q_tail)
#define SPI_Q_FULL(m) (SPI_Q_INCR((m)->q_tail, (m)->q_capacity) == (m)->q_head)

static inline void spi_bus_gpio_set(const spi_bus_gpio *g, bool active)
{
    if (!g->port)
        return;
    GPIO_PinState s = GPIO_PIN_RESET;
    bool want_active = active;
    if (g->active_low)
        want_active = !active;
    s = want_active ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(g->port, g->pin, s);
}

/* Active means asserting the line (CS active, DC=1 for data, DC=0 for command) */
static inline void spi_bus_cs_assert(const spi_bus_gpio *cs) { spi_bus_gpio_set(cs, true); }
static inline void spi_bus_cs_deassert(const spi_bus_gpio *cs) { spi_bus_gpio_set(cs, false); }

/* DC helper: mode -> level */
static inline void spi_bus_dc_apply(const spi_bus_gpio *dc, spi_bus_dc_mode mode)
{
    if (!dc->port || mode == SPI_BUS_DC_UNUSED)
        return;
    /* COMMAND -> active=false; DATA -> active=true (treat active as DC=1) */
    bool as_active = (mode == SPI_BUS_DC_DATA);
    /* For DC we define active=true == logic '1' request; spi_bus_gpio_set handles polarity if someone configured it. */
    HAL_GPIO_WritePin(dc->port, dc->pin, as_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* Fast switch SPI registers without full HAL re-init. Stop SPE, write CR1/CR2, restart SPE. */
static void spi_bus_apply_regs(SPI_TypeDef *SPIx, uint32_t cr1, uint32_t cr2)
{
    /* Disable SPI to safely change CR1/CR2 */
    CLEAR_BIT(SPIx->CR1, SPI_CR1_SPE);
    /* Preserve reserved bits as needed; assume caller provides sane fields */
    SPIx->CR1 = cr1;
    SPIx->CR2 = cr2;
    SET_BIT(SPIx->CR1, SPI_CR1_SPE);
}

/* Optional D-Cache clean (no-op on CM4/G4) */
static inline void spi_bus_clean_dcache_region(const void *addr, size_t len)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    /* Align to 32 bytes lines */
    uintptr_t a = (uintptr_t)addr;
    uintptr_t start = a & ~((uintptr_t)31);
    uintptr_t end = (a + len + 31) & ~((uintptr_t)31);
    SCB_CleanDCache_by_Addr((void *)start, (int32_t)(end - start));
#else
    (void)addr;
    (void)len;
#endif
}

/* Current in-flight transaction pointer (not re-entrant) */
static spi_bus_transaction *spi_bus_peek_current(spi_bus_manager *mgr)
{
    if (SPI_Q_EMPTY(mgr))
        return NULL;
    return &mgr->q[mgr->q_head];
}

/* Pop head (after finishing a transaction) */
static void spi_bus_pop(spi_bus_manager *mgr)
{
    if (!SPI_Q_EMPTY(mgr))
        mgr->q_head = SPI_Q_INCR(mgr->q_head, mgr->q_capacity);
}

/* Start next transaction if any (called at thread-level and ISR-level). */
static void spi_bus_try_start(spi_bus_manager *mgr)
{
    if (mgr->busy)
        return;
    if (SPI_Q_EMPTY(mgr))
        return;

    spi_bus_transaction *t = &mgr->q[mgr->q_head];
    mgr->busy = true;

    /* Apply SPI registers quickly */
    spi_bus_apply_regs(mgr->spi->Instance, t->cr1, t->cr2);

    /* Assert CS and set DC */
    spi_bus_cs_assert(&t->cs);
    spi_bus_dc_apply(&t->dc, t->dc_mode);

    /* Clean DCache for TX if enabled */
    if (mgr->clean_dcache_before_tx && t->tx && t->len)
        spi_bus_clean_dcache_region(t->tx, (size_t)t->len * (((t->cr2 & SPI_CR2_DS) == SPI_DATASIZE_16BIT) ? 2u : 1u));

    HAL_StatusTypeDef hs;
    if (t->dir == SPI_BUS_DIR_TXRX)
    {
        hs = HAL_SPI_TransmitReceive_DMA(mgr->spi, (uint8_t *)t->tx, t->rx, t->len);
    }
    else
    {
        hs = HAL_SPI_Transmit_DMA(mgr->spi, (uint8_t *)t->tx, t->len);
    }

    if (hs != HAL_OK)
    {
        /* Deassert CS to avoid holding the bus if start failed */
        spi_bus_cs_deassert(&t->cs);
        mgr->busy = false;
        if (t->on_error)
            t->on_error(mgr, t->user);
        /* Drop this transaction to avoid stalling the queue */
        spi_bus_pop(mgr);
        /* Try next (prevent infinite loop if all fail) */
        mgr->busy = false;
        spi_bus_try_start(mgr);
    }
}

/* Process post-transfer wait (busy predicate) in a tight loop with timeout.
   Called in ISR tail (still interrupt context), keep it short: polling + HAL_GetTick() is OK. */
static bool spi_bus_do_post_wait(spi_bus_transaction *t)
{
    if (!t->wait_ready)
        return true;
    uint32_t start = HAL_GetTick();
    for (;;)
    {
        if (t->wait_ready(t->user))
            return true;
        if (t->wait_timeout_ms > 0U)
        {
            if ((HAL_GetTick() - start) >= t->wait_timeout_ms)
                return false;
        }
        /* Short relax: on Cortex-M, a few no-ops are fine; if you need deeper sleep, handle at thread level */
        __NOP();
        __NOP();
        __NOP();
    }
}

/* Common tail for complete (TX or TXRX) */
static void spi_bus_on_complete_common(spi_bus_manager *mgr, bool half, bool is_txrx)
{
    spi_bus_transaction *t = spi_bus_peek_current(mgr);
    if (!t)
    {
        mgr->busy = false;
        return;
    }

    if (half)
    {
        if (t->on_half)
            t->on_half(mgr, t->user);
        return; /* still in progress; don't touch CS or queue */
    }

    /* Full complete: deassert CS */
    spi_bus_cs_deassert(&t->cs);

    /* Post-transfer wait (e.g., device BUSY). Fail -> call error callback. */
    if (!spi_bus_do_post_wait(t))
    {
        if (t->on_error)
            t->on_error(mgr, t->user);
    }
    else
    {
        if (t->on_done)
            t->on_done(mgr, t->user);
    }

    /* Remove transaction and move on */
    spi_bus_pop(mgr);
    mgr->busy = false;
    spi_bus_try_start(mgr);
}

/* ------------------------------ Public API -------------------------------- */

spi_bus_manager spi_bus_manager_create(SPI_HandleTypeDef *spi,
                                       spi_bus_transaction *storage,
                                       uint16_t capacity)
{
    spi_bus_manager m;
    m.spi = spi;
    m.q = storage;
    m.q_capacity = capacity;
    m.q_head = 0;
    m.q_tail = 0;
    m.busy = false;
    m.clean_dcache_before_tx = false;
    return m;
}

spi_bus_manager_status spi_bus_manager_submit(spi_bus_manager *mgr, const spi_bus_transaction *t)
{
    if (!mgr || !t || !mgr->spi || !mgr->q || mgr->q_capacity == 0)
        return SPI_BUS_MANAGER_ERR_PARAM;
    if (!t->cs.port)
        return SPI_BUS_MANAGER_ERR_PARAM;
    if (!t->tx || t->len == 0)
        return SPI_BUS_MANAGER_ERR_PARAM;
    if (t->dir == SPI_BUS_DIR_TXRX && !t->rx)
        return SPI_BUS_MANAGER_ERR_PARAM;
    if (SPI_Q_FULL(mgr))
        return SPI_BUS_MANAGER_ERR_FULL;

    /* Copy by value into queue tail */
    mgr->q[mgr->q_tail] = *t;
    mgr->q_tail = SPI_Q_INCR(mgr->q_tail, mgr->q_capacity);

    /* Try to start immediately if bus idle (works from thread level and ISR) */
    spi_bus_try_start(mgr);
    return SPI_BUS_MANAGER_OK;
}

bool spi_bus_manager_is_idle(const spi_bus_manager *mgr)
{
    return (!mgr->busy) && SPI_Q_EMPTY(mgr);
}

void spi_bus_manager_cancel_pending(spi_bus_manager *mgr)
{
    /* Do not touch current in-flight; just drop everything behind it */
    mgr->q_tail = mgr->q_head;
}

/* -------------------------- HAL integration hooks ------------------------- */

void spi_bus_manager_on_tx_cplt(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    if (!mgr || hspi != mgr->spi)
        return;

    /* Distinguish half vs full completion by DMA flags */
    /* HAL does not pass half/full explicitly here when called from both callbacks.
       A safe way: rely on which callback you wire up:
       - Call this function from both TxHalfCplt and TxCplt.
       - On HalfCplt entry, HT flag is set; on Cplt, TC flag set.
       We can read DMA flags if needed; for simplicity we pass 'half' based on HAL callback origin.
       -> Provide two weak wrappers so user can call this accordingly.
    */
    /* Assume caller maps HalfCplt to this function first -> treat as half */
    /* We can't infer here; provide two public entry points? For simplicity, we treat both as 'full' unless user calls twice. */
    /* Better: expose separate mapping as in header commentary. Here we assume:
       - TxHalfCplt calls this before TxCplt, and we signal half when SPI state is BUSY_TX and DMA HT flag is set.
    */
    bool half = false;
    if (hspi->hdmatx)
    {
        /* Check HT flag */
        if (__HAL_DMA_GET_FLAG(hspi->hdmatx, __HAL_DMA_GET_HT_FLAG_INDEX(hspi->hdmatx)))
        {
            /* If HT is still set, this callback likely came from HalfCplt */
            /* Clear handled by HAL; we just hint half=true if SPI still busy */
            half = (hspi->State == HAL_SPI_STATE_BUSY_TX);
        }
    }

    spi_bus_on_complete_common(mgr, half, false);
}

void spi_bus_manager_on_txrx_cplt(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    if (!mgr || hspi != mgr->spi)
        return;

    bool half = false;
    if (hspi->hdmatx)
    {
        if (__HAL_DMA_GET_FLAG(hspi->hdmatx, __HAL_DMA_GET_HT_FLAG_INDEX(hspi->hdmatx)))
        {
            half = (hspi->State == HAL_SPI_STATE_BUSY_TX_RX);
        }
    }

    spi_bus_on_complete_common(mgr, half, true);
}

void spi_bus_manager_on_error(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    if (!mgr || hspi != mgr->spi)
        return;

    spi_bus_transaction *t = spi_bus_peek_current(mgr);
    if (t)
    {
        spi_bus_cs_deassert(&t->cs);
        if (t->on_error)
            t->on_error(mgr, t->user);
        spi_bus_pop(mgr);
    }
    mgr->busy = false;
    spi_bus_try_start(mgr);
}
