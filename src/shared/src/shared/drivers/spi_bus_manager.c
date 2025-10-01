#include "shared/drivers/spi_bus_manager.h"
#include <string.h>

#if SPI_BUS_MANAGER_DEBUG
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

/* ============================ DEBUG LOG BUFFER ============================ */
/* Huge ring buffer for debug logs. Inspect with GDB (g_spi_dbg_log / g_spi_dbg_pos). */
#ifndef SPI_DBG_LOG_BUF_SIZE
#define SPI_DBG_LOG_BUF_SIZE (16 * 1024) /* 256 KB; adjust if you need more */
#endif

/* Exposed globals for GDB inspection */
char g_spi_dbg_log[SPI_DBG_LOG_BUF_SIZE];
volatile uint32_t g_spi_dbg_pos = 0;

/* Internal helper: fast, IRQ-safe append into ring buffer. */
static inline void spi_dbg_log_write(const char *src, size_t n)
{
    if (!src || n == 0)
        return;

    /* Enter short critical section (preserve PRIMASK) */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    uint32_t pos = g_spi_dbg_pos;
    uint32_t cap = (uint32_t)SPI_DBG_LOG_BUF_SIZE;

    /* Write with wrap-around. We never block; we just overwrite old data. */
    for (size_t i = 0; i < n; ++i)
    {
        // DEBUG: Place placeholder here, and get memory address.
        // Then open cortex-debug memory window (legacy) to see live log.
        char *addr_dbg_fetcher = &g_spi_dbg_log[pos];
        g_spi_dbg_log[pos] = src[i];
        pos++;
        if (pos >= cap)
            pos = 0;
    }
    g_spi_dbg_pos = pos;

    /* Leave critical section */
    if (!primask)
        __enable_irq();
}

/* printf-style logger. Keep messages short in ISR. */
static void spi_dbg_log(const char *fmt, ...)
{
    char tmp[192]; /* small stack buffer to limit ISR time; truncate if needed */
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    if (len <= 0)
        return;
    size_t n = (size_t)((len < (int)sizeof(tmp)) ? len : (int)sizeof(tmp));
    spi_dbg_log_write(tmp, n);
}

/* Optional helper: put a newline quickly (avoids extra format work). */
static inline void spi_dbg_nl(void) { spi_dbg_log_write("\n", 1); }

/* Quick hexdump of small buffers (useful for SPI headers). Keep tiny in ISR). */
static void spi_dbg_hexdump(const uint8_t *p, size_t n)
{
    if (!p || n == 0)
        return;
    /* Cap to avoid huge ISR time */
    if (n > 32)
        n = 32;
    char line[3 * 32 + 8];
    size_t off = 0;
    for (size_t i = 0; i < n && off + 3 < sizeof(line); ++i)
        off += (size_t)snprintf(line + off, sizeof(line) - off, "%02X ", p[i]);
    spi_dbg_log_write(line, off);
    spi_dbg_nl();
}

/* Quick helper to reset/mark the log (call from your code if needed) */
void spi_dbg_mark(const char *tag)
{
    spi_dbg_log("=== %s ===\n", tag ? tag : "MARK");
}
#else
/* No-op if debug disabled */
#define spi_dbg_log(...) ((void)0)
#define spi_dbg_nl() ((void)0)
#define spi_dbg_hexdump(p, n) ((void)0)
#define spi_dbg_mark(tag) ((void)0)
#endif

/* ------------------------- Internal helpers/macros ------------------------- */

#define SPI_Q_INCR(i, cap) (uint16_t)(((i) + 1) % (cap))
#define SPI_Q_EMPTY(m) ((m)->q_head == (m)->q_tail)
#define SPI_Q_FULL(m) (SPI_Q_INCR((m)->q_tail, (m)->q_capacity) == (m)->q_head)

static inline void spi_bus_gpio_set(const spi_bus_gpio *g, bool active)
{
    spi_dbg_log("gpio_set port=%p pin=%u active=%d (active_low=%d)\n",
                (void *)g->port, (unsigned)g->pin, (int)active, (int)g->active_low);

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
static inline void spi_bus_cs_assert(const spi_bus_gpio *cs)
{
    spi_dbg_log("cs_assert port=%p pin=%u\n", (void *)cs->port, (unsigned)cs->pin);
    spi_bus_gpio_set(cs, true);
}
static inline void spi_bus_cs_deassert(const spi_bus_gpio *cs)
{
    spi_dbg_log("cs_deassert port=%p pin=%u\n", (void *)cs->port, (unsigned)cs->pin);
    spi_bus_gpio_set(cs, false);
}

/* DC helper: mode -> level */
static inline void spi_bus_dc_apply(const spi_bus_gpio *dc, spi_bus_dc_mode mode)
{
    spi_dbg_log("dc_apply port=%p pin=%u mode=%d\n", (void *)dc->port, (unsigned)dc->pin, (int)mode);
    if (!dc->port || mode == SPI_BUS_DC_UNUSED)
        return;
    /* COMMAND -> active=false; DATA -> active=true (treat active as DC=1) */
    bool as_active = (mode == SPI_BUS_DC_DATA);
    /* Polarity handled in spi_bus_gpio_set */
    spi_bus_gpio_set(dc, as_active);
}

/* Fast switch SPI registers without full HAL re-init. Stop SPE, write CR1/CR2, restart SPE. */
static void spi_bus_apply_regs(SPI_TypeDef *SPIx, uint32_t cr1, uint32_t cr2)
{
    spi_dbg_log("apply_regs SPIx=%p CR1=0x%08lX CR2=0x%08lX\n", (void *)SPIx, (unsigned long)cr1, (unsigned long)cr2);
    /* Disable SPI to safely change CR1/CR2 */
    CLEAR_BIT(SPIx->CR1, SPI_CR1_SPE);
    SPIx->CR1 = cr1;
    SPIx->CR2 = cr2;
    SET_BIT(SPIx->CR1, SPI_CR1_SPE);
}

/* Optional D-Cache clean (no-op on CM4/G4) */
static inline void spi_bus_clean_dcache_region(const void *addr, size_t len)
{
    spi_dbg_log("dcache_clean addr=%p len=%lu\n", addr, (unsigned long)len);
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
    spi_dbg_log("peek_current busy=%d head=%u tail=%u\n", (int)mgr->busy, mgr->q_head, mgr->q_tail);
    if (SPI_Q_EMPTY(mgr))
        return NULL;
    return &mgr->q[mgr->q_head];
}

/* Pop head (after finishing a transaction) */
static void spi_bus_pop(spi_bus_manager *mgr)
{
    spi_dbg_log("pop head=%u -> %u (cap=%u)\n", mgr->q_head, SPI_Q_INCR(mgr->q_head, mgr->q_capacity), mgr->q_capacity);
    if (!SPI_Q_EMPTY(mgr))
        mgr->q_head = SPI_Q_INCR(mgr->q_head, mgr->q_capacity);
}

/* Start next transaction if any (called at thread-level and ISR-level). */
static void spi_bus_try_start(spi_bus_manager *mgr)
{
    spi_dbg_log("try_start busy=%d empty=%d\n", (int)mgr->busy, (int)SPI_Q_EMPTY(mgr));

    if (mgr->busy)
        return;

    if (SPI_Q_EMPTY(mgr))
        return;

    spi_bus_transaction *t = &mgr->q[mgr->q_head];

    /* callback-only item – no DMA, no CS/DC */
    if (t->kind == SPI_BUS_ITEM_CALLBACK)
    {
        spi_dbg_log("start callback at head=%u user=%p\n", mgr->q_head, t->user);
        if (t->on_done)
            t->on_done(mgr, t->user);
        /* Pop and immediately try the next one (may chain callbacks) */
        spi_bus_pop(mgr);
        /* Do not set busy for pure callback */
        spi_bus_try_start(mgr);
        return;
    }

    mgr->busy = true;

    spi_dbg_log("start tx head=%u dir=%d len=%u dc_mode=%d cs.port=%p ds=%s\n",
                mgr->q_head, (int)t->dir, (unsigned)t->len, (int)t->dc_mode,
                (void *)t->cs.port,
                ((t->cr2 & SPI_CR2_DS) == SPI_DATASIZE_16BIT) ? "16" : "8");

    /* Apply SPI registers quickly */
    spi_bus_apply_regs(mgr->spi->Instance, t->cr1, t->cr2);

    /* DC first, then CS - very important */
    spi_bus_dc_apply(&t->dc, t->dc_mode);
    spi_bus_cs_assert(&t->cs);

    /* Clean DCache for TX if enabled */
    if (mgr->clean_dcache_before_tx && t->tx && t->len)
    {
        size_t bytes = (size_t)t->len * (((t->cr2 & SPI_CR2_DS) == SPI_DATASIZE_16BIT) ? 2u : 1u);
        spi_bus_clean_dcache_region(t->tx, bytes);
    }

    HAL_StatusTypeDef hs;
    if (t->dir == SPI_BUS_DIR_TXRX)
    {
        spi_dbg_log("HAL_SPI_TransmitReceive_DMA tx=%p rx=%p len=%u\n", t->tx, t->rx, (unsigned)t->len);
        hs = HAL_SPI_TransmitReceive_DMA(mgr->spi, (uint8_t *)t->tx, t->rx, t->len);
    }
    else
    {
        spi_dbg_log("HAL_SPI_Transmit_DMA tx=%p len=%u\n", t->tx, (unsigned)t->len);
        hs = HAL_SPI_Transmit_DMA(mgr->spi, (uint8_t *)t->tx, t->len);
    }

    if (hs != HAL_OK)
    {
        spi_dbg_log("DMA start FAILED hs=%d\n", (int)hs);
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
    spi_dbg_log("post_wait timeout=%lu has_cb=%d\n", (unsigned long)t->wait_timeout_ms, (int)(t->wait_ready != NULL));
    if (!t->wait_ready)
        return true;
    uint32_t start = HAL_GetTick();
    for (;;)
    {
        if (t->wait_ready(t->user))
        {
            spi_dbg_log("post_wait done ok\n");
            return true;
        }
        if (t->wait_timeout_ms > 0U)
        {
            if ((HAL_GetTick() - start) >= t->wait_timeout_ms)
            {
                spi_dbg_log("post_wait TIMEOUT after %lu ms\n", (unsigned long)(HAL_GetTick() - start));
                return false;
            }
        }
        __NOP();
        __NOP();
        __NOP();
    }
}

/* Common tail for complete (TX or TXRX) */
static void spi_bus_on_complete_common(spi_bus_manager *mgr, bool half, bool is_txrx)
{
    spi_dbg_log("on_complete_common half=%d txrx=%d\n", (int)half, (int)is_txrx);
    spi_bus_transaction *t = spi_bus_peek_current(mgr);
    if (!t)
    {
        spi_dbg_log("on_complete: no current tx\n");
        mgr->busy = false;
        return;
    }

    if (half)
    {
        spi_dbg_log("on_half user=%p\n", t->user);
        if (t->on_half)
            t->on_half(mgr, t->user);
        return; /* still in progress; don't touch CS or queue */
    }

    {
        /* ensure last bit is clocked out */
        uint32_t t0 = HAL_GetTick();
        while (__HAL_SPI_GET_FLAG(mgr->spi, SPI_FLAG_BSY))
        {
            if ((HAL_GetTick() - t0) > 2)
                break; /* ~2 ms guard */
        }
        spi_bus_cs_deassert(&t->cs);
    }

    /* Post-transfer wait (e.g., device BUSY). Fail -> call error callback. */
    if (!spi_bus_do_post_wait(t))
    {
        spi_dbg_log("on_done: post_wait failed -> on_error\n");
        if (t->on_error)
            t->on_error(mgr, t->user);
    }
    else
    {
        spi_dbg_log("on_done: invoking on_done user=%p\n", t->user);
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
    spi_dbg_log("mgr_create spi=%p storage=%p cap=%u\n", (void *)spi, (void *)storage, (unsigned)capacity);

    spi_bus_manager m;
    m.spi = spi;
    m.q = storage;
    m.q_capacity = capacity;
    m.q_head = 0;
    m.q_tail = 0;
    m.busy = false;
    m.clean_dcache_before_tx = false;

    spi_dbg_log("mgr_create done head=%u tail=%u\n", m.q_head, m.q_tail);
    return m;
}

spi_bus_manager_status spi_bus_manager_submit(spi_bus_manager *mgr, const spi_bus_transaction *t)
{
    spi_dbg_log("submit mgr=%p t=%p\n", (void *)mgr, (void *)t);

    if (!mgr || !t || !mgr->spi || !mgr->q || mgr->q_capacity == 0)
    {
        spi_dbg_log("submit ERR_PARAM (nulls/cap)\n");
        return SPI_BUS_MANAGER_ERR_PARAM;
    }

    if (t->kind != SPI_BUS_ITEM_CALLBACK)
    {
        if (!t->cs.port)
        {
            spi_dbg_log("submit ERR_PARAM (no CS)\n");
            return SPI_BUS_MANAGER_ERR_PARAM;
        }
        if (!t->tx || t->len == 0)
        {
            spi_dbg_log("submit ERR_PARAM (tx/len)\n");
            return SPI_BUS_MANAGER_ERR_PARAM;
        }
        if (t->dir == SPI_BUS_DIR_TXRX && !t->rx)
        {
            spi_dbg_log("submit ERR_PARAM (txrx no rx)\n");
            return SPI_BUS_MANAGER_ERR_PARAM;
        }
    }

    if (SPI_Q_FULL(mgr))
    {
        spi_dbg_log("submit ERR_FULL head=%u tail=%u cap=%u\n", mgr->q_head, mgr->q_tail, mgr->q_capacity);
        return SPI_BUS_MANAGER_ERR_FULL;
    }

    /* Copy by value into queue tail */
    mgr->q[mgr->q_tail] = *t;

    /* Force kind=TX for normal submissions (backward compat) */
    if (mgr->q[mgr->q_tail].kind != SPI_BUS_ITEM_CALLBACK)
    {
        mgr->q[mgr->q_tail].kind = SPI_BUS_ITEM_TX;
    }

    spi_dbg_log("submit queued at %u len=%u dir=%d user=%p\n", mgr->q_tail, (unsigned)t->len, (int)t->dir, t->user);

    mgr->q_tail = SPI_Q_INCR(mgr->q_tail, mgr->q_capacity);

    /* Try to start immediately if bus idle (works from thread level and ISR) */
    spi_bus_try_start(mgr);

    return SPI_BUS_MANAGER_OK;
}

bool spi_bus_manager_is_idle(const spi_bus_manager *mgr)
{
    bool idle = (!mgr->busy) && SPI_Q_EMPTY(mgr);
    spi_dbg_log("is_idle=%d busy=%d head=%u tail=%u\n", (int)idle, (int)mgr->busy, mgr->q_head, mgr->q_tail);
    return idle;
}

void spi_bus_manager_cancel_pending(spi_bus_manager *mgr)
{
    spi_dbg_log("cancel_pending head=%u tail=%u\n", mgr->q_head, mgr->q_tail);
    /* Do not touch current in-flight; just drop everything behind it */
    mgr->q_tail = mgr->q_head;
}

/* -------------------------- HAL integration hooks ------------------------- */

void spi_bus_manager_on_tx_half(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    spi_dbg_log("cb_tx_half mgr=%p hspi=%p\n", (void *)mgr, (void *)hspi);
    if (!mgr || hspi != mgr->spi)
        return;
    spi_bus_on_complete_common(mgr, true, false);
}

void spi_bus_manager_on_txrx_half(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    spi_dbg_log("cb_txrx_half mgr=%p hspi=%p\n", (void *)mgr, (void *)hspi);
    if (!mgr || hspi != mgr->spi)
        return;
    spi_bus_on_complete_common(mgr, true, true);
}

void spi_bus_manager_on_tx_cplt(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    spi_dbg_log("cb_tx_cplt mgr=%p hspi=%p\n", (void *)mgr, (void *)hspi);
    if (!mgr || hspi != mgr->spi)
        return;
    spi_bus_on_complete_common(mgr, false, false);
}

void spi_bus_manager_on_txrx_cplt(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    spi_dbg_log("cb_txrx_cplt mgr=%p hspi=%p\n", (void *)mgr, (void *)hspi);
    if (!mgr || hspi != mgr->spi)
        return;
    spi_bus_on_complete_common(mgr, false, true);
}

void spi_bus_manager_on_error(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi)
{
    spi_dbg_log("cb_error mgr=%p hspi=%p\n", (void *)mgr, (void *)hspi);
    if (!mgr || hspi != mgr->spi)
        return;

    spi_bus_transaction *t = spi_bus_peek_current(mgr);
    if (t)
    {
        spi_dbg_log("cb_error: deassert CS; user=%p\n", t->user);
        spi_bus_cs_deassert(&t->cs);
        if (t->on_error)
            t->on_error(mgr, t->user);
        spi_bus_pop(mgr);
    }
    mgr->busy = false;
    spi_bus_try_start(mgr);
}

spi_bus_manager_status spi_bus_manager_enqueue_callback(spi_bus_manager *mgr,
                                                        spi_bus_done_cb cb,
                                                        void *user)
{
    spi_dbg_log("enqueue_cb mgr=%p cb=%p user=%p\n", (void *)mgr, (void *)cb, user);
    if (!mgr || !cb || !mgr->q || mgr->q_capacity == 0)
        return SPI_BUS_MANAGER_ERR_PARAM;
    if (SPI_Q_FULL(mgr))
        return SPI_BUS_MANAGER_ERR_FULL;

    spi_bus_transaction t;
    /* Zero everything, then set only what callback needs */
    memset(&t, 0, sizeof(t));
    t.kind = SPI_BUS_ITEM_CALLBACK;
    t.on_done = cb;
    t.user = user;

    /* Push like normal submit: copy by value */
    mgr->q[mgr->q_tail] = t;
    spi_dbg_log("enqueue_cb queued at %u\n", mgr->q_tail);
    mgr->q_tail = SPI_Q_INCR(mgr->q_tail, mgr->q_capacity);

    /* Kick the engine (works from ISR or thread) */
    spi_bus_try_start(mgr);
    return SPI_BUS_MANAGER_OK;
}
