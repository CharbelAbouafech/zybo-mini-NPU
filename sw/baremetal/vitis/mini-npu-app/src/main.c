/*
 * main.c - AXI DMA loopback test (bare-metal, Zybo Z7-10)
 *
 * Data path:  DDR -> DMA MM2S -> AXI4-Stream Data FIFO -> DMA S2MM -> DDR
 *
 * For each transfer size the test:
 *   1. fills tx_buf with a pattern and clears rx_buf
 *   2. flushes tx_buf from the cache to DDR and invalidates rx_buf
 *   3. starts S2MM (receiver) first, then MM2S (sender)
 *   4. polls until both DMA channels are idle
 *   5. invalidates rx_buf again, compares it with tx_buf, prints MB/s
 *
 * LEDs: LED0 = running, LED1 = PASS, LED3 = FAIL.
 */

#include "xaxidma.h"      /* AXI DMA driver (XAxiDma_...)                    */
#include "xparameters.h"  /* addresses and IDs from the .xsa                 */
#include "xil_cache.h"    /* Xil_DCacheFlushRange / Xil_DCacheInvalidateRange */
#include "xil_io.h"       /* Xil_Out32 for the LED GPIO                      */
#include "xil_printf.h"   /* xil_printf over UART1                           */
#include "xtime_l.h"      /* XTime_GetTime, COUNTS_PER_SECOND                */

/* ---- Hardware addresses (from xparameters.h) ---------------------------- */
#define DMA_DEV_ID      XPAR_AXI_DMA_0_DEVICE_ID
#define LED_GPIO_BASE   XPAR_AXI_GPIO_0_BASEADDR   /* channel 1 = 4 LEDs */
#define GPIO_DATA       0x0                        /* channel 1 data register      */
#define GPIO_TRI        0x4                        /* channel 1 direction, 0 = out */

/* ---- Test settings ------------------------------------------------------ */
#define MAX_LEN         (1024 * 1024)   /* 1 MB; the 23-bit length register allows up to 8 MB - 1 */
#define TIMEOUT_LOOPS   100000000U      /* give up if the DMA is busy this long */

/* 32-byte aligned (one Cortex-A9 cache line), so flush/invalidate never
 * touches data that belongs to something else. */
static u8 tx_buf[MAX_LEN] __attribute__((aligned(32)));
static u8 rx_buf[MAX_LEN] __attribute__((aligned(32)));

static XAxiDma dma;   /* driver instance */

static void set_leds(u32 value)
{
    Xil_Out32(LED_GPIO_BASE + GPIO_DATA, value & 0xF);
}

/* Convert timer ticks to microseconds. */
static u32 ticks_to_us(XTime ticks)
{
    return (u32)((ticks * 1000000ULL) / COUNTS_PER_SECOND);
}

/* Throughput in MB/s (1 MB = 10^6 bytes). Integer math: xil_printf has no %f. */
static u32 mb_per_s(u32 bytes, XTime ticks)
{
    if (ticks == 0) {
        return 0;
    }
    return (u32)(((u64)bytes * COUNTS_PER_SECOND) / (ticks * 1000000ULL));
}

/* Set up the DMA driver in simple (register) mode, interrupts off. */
static int init_dma(void)
{
    XAxiDma_Config *cfg = XAxiDma_LookupConfig(DMA_DEV_ID);
    if (cfg == NULL) {
        xil_printf("ERROR: no config found for DMA device %d\r\n", DMA_DEV_ID);
        return XST_FAILURE;
    }

    if (XAxiDma_CfgInitialize(&dma, cfg) != XST_SUCCESS) {
        xil_printf("ERROR: DMA initialization failed\r\n");
        return XST_FAILURE;
    }

    if (XAxiDma_HasSg(&dma)) {
        xil_printf("ERROR: DMA is in scatter-gather mode, expected simple mode\r\n");
        return XST_FAILURE;
    }

    /* We poll for "done" in this test; interrupts come later. */
    XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

    return XST_SUCCESS;
}

/* One loopback transfer of len bytes. Returns XST_SUCCESS if the data matches. */
static int run_one(u32 len)
{
    u32 i;
    u32 loops = 0;
    u32 errors = 0;
    XTime t0, t1, t2;

    /* 1. Test pattern: changes every byte and differs between 256-byte blocks. */
    for (i = 0; i < len; i++) {
        tx_buf[i] = (u8)(i + (i >> 8));
        rx_buf[i] = 0;
    }

    /* 2. Cache handling. The DMA reads and writes DDR directly and cannot see
     *    the CPU cache, so: push tx_buf out to DDR, drop old rx_buf lines. */
    XTime_GetTime(&t0);
    Xil_DCacheFlushRange((UINTPTR)tx_buf, len);
    Xil_DCacheInvalidateRange((UINTPTR)rx_buf, len);
    XTime_GetTime(&t1);

    /* 3. Start the receiver (S2MM) first, then the sender (MM2S). */
    if (XAxiDma_SimpleTransfer(&dma, (UINTPTR)rx_buf, len, XAXIDMA_DEVICE_TO_DMA) != XST_SUCCESS) {
        xil_printf("ERROR: could not start S2MM (%d bytes)\r\n", len);
        return XST_FAILURE;
    }
    if (XAxiDma_SimpleTransfer(&dma, (UINTPTR)tx_buf, len, XAXIDMA_DMA_TO_DEVICE) != XST_SUCCESS) {
        xil_printf("ERROR: could not start MM2S (%d bytes)\r\n", len);
        return XST_FAILURE;
    }

    /* 4. Wait until both channels are idle. MM2S sets TLAST on the last beat,
     *    the FIFO passes it on, and S2MM finishes when it sees TLAST. */
    while (XAxiDma_Busy(&dma, XAXIDMA_DMA_TO_DEVICE) ||
           XAxiDma_Busy(&dma, XAXIDMA_DEVICE_TO_DMA)) {
        if (++loops > TIMEOUT_LOOPS) {
            xil_printf("ERROR: timeout after %d bytes started. MM2S status 0x%08x, S2MM status 0x%08x\r\n",
                       len,
                       XAxiDma_ReadReg(dma.RegBase, XAXIDMA_TX_OFFSET + XAXIDMA_SR_OFFSET),
                       XAxiDma_ReadReg(dma.RegBase, XAXIDMA_RX_OFFSET + XAXIDMA_SR_OFFSET));
            return XST_FAILURE;
        }
    }
    XTime_GetTime(&t2);

    /* 5. Read what the DMA wrote: invalidate again so the CPU sees DDR, not stale cache. */
    Xil_DCacheInvalidateRange((UINTPTR)rx_buf, len);

    for (i = 0; i < len; i++) {
        if (rx_buf[i] != tx_buf[i]) {
            if (errors < 5) {
                xil_printf("  mismatch at byte %d: sent 0x%02x, got 0x%02x\r\n",
                           i, tx_buf[i], rx_buf[i]);
            }
            errors++;
        }
    }

    xil_printf("%8d bytes: %s | cache %5d us | DMA %6d us | %4d MB/s\r\n",
               len,
               (errors == 0) ? "OK  " : "FAIL",
               ticks_to_us(t1 - t0),
               ticks_to_us(t2 - t1),
               mb_per_s(len, t2 - t1));

    return (errors == 0) ? XST_SUCCESS : XST_FAILURE;
}

int main(void)
{
    const u32 sizes[] = { 1024, 4096, 65536, 262144, MAX_LEN };
    const u32 num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    u32 n;
    u32 failed = 0;

    xil_printf("\r\n=== Mini-NPU: AXI DMA loopback test ===\r\n");

    Xil_Out32(LED_GPIO_BASE + GPIO_TRI, 0x0);   /* LED channel as outputs */
    set_leds(0x1);                              /* LED0: running */

    if (init_dma() != XST_SUCCESS) {
        set_leds(0x8);                          /* LED3: fail */
        return XST_FAILURE;
    }

    for (n = 0; n < num_sizes; n++) {
        if (run_one(sizes[n]) != XST_SUCCESS) {
            failed++;
        }
    }

    if (failed == 0) {
        xil_printf("PASS: all %d transfer sizes matched\r\n", num_sizes);
        set_leds(0x2);                          /* LED1: pass */
    } else {
        xil_printf("FAIL: %d of %d transfer sizes had errors\r\n", failed, num_sizes);
        set_leds(0x8);                          /* LED3: fail */
    }

    return (failed == 0) ? XST_SUCCESS : XST_FAILURE;
}
