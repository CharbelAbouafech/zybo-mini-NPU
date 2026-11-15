#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// MM2S (DMA)
#define MM2S_DMACR  0x00 // MM2S control bits: bits 0 = run, bit 2 = reset (0x0000_0000).
#define MM2S_DMASR  0x04 // MM2S status bits : bits 12 = done, bit 4-6 = errors (0x0000_0100).
#define MM2S_SA     0x18 // Source address (physical) (0x0001_1000).
#define MM2S_LENGTH 0x28 // Bytes to send. Writing this starts the transfer. (0x0010_1000).

// S2MM (DMA)
#define S2MM_DMACR  0x30 // S2MM control bits. (0x0011_0000).
#define S2MM_DMASR  0x34 // S2MM status bits. (0x0011_0100).
#define S2MM_DA     0X48 // Destination address (physical) (0x0100_1000).
#define S2MM_LENGTH 0x58 // Bytes to send. Writing it starts the channel. (0x0101_1000).

// DMA control/status bits (32-bit registers)
#define DMACR_RUN       (1u << 0)    // hex 0x0000_0001 = bin 0000_0000_0000_0000_0000_0000_0000_0001 (bit 0)
#define DMACR_RESET     (1u << 2)    // hex 0x0000_0004 = bin 0000_0000_0000_0000_0000_0000_0000_0100 (bit 2)
#define DMASR_DONE      (1u << 12)   // hex 0x0000_1000 = bin 0000_0000_0000_0000_0001_0000_0000_0000 (bit 12)
#define DMASR_ERR_MASK  (0x7u << 4)  // hex 0x0000_0070 = bin 0000_0000_0000_0000_0000_0000_0111_0000 (bits 6:4)

#define BUF_PHYS   0x3F000000u   // physical address of the 16 MB buffer
#define HALF       0x800000u     // 8 MB: offset of the destination half


// write reg
static void reg_write(volatile uint8_t *base, uint32_t off, uint32_t val)
{
    *(volatile uint32_t *)(base + off) = val;
}

// read reg
static uint32_t reg_read(volatile uint8_t *base, uint32_t off)
{
    return *(volatile uint32_t *)(base + off);
}

// dma reset
static int dma_reset(volatile uint8_t *dma)
{
    reg_write(dma, MM2S_DMACR, DMACR_RESET); // dma: reg is MM2S_DMACR. reset is DMACR_RESET

    int tries = 0;
    while (reg_read(dma, MM2S_DMACR) & DMACR_RESET) {
        if (++tries > 1000000) {          /* safety: don't wait forever */
            printf("ERROR: DMA reset timeout\n");
            return -1;
        }
    }
    return 0;
}

// current time in nanoseconds (monotonic clock, not affected by date changes)
static long long now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}


int main(void)
{
    /* 1. DMA: open /dev/uio0 and check for errors */
    int dma_fd = open("/dev/uio0", O_RDWR | O_SYNC ); // open the device
    if (dma_fd < 0)  // fails without sudo
    { 
        perror("open uio0"); 
        return 1; 
    }

    /* 2. DMA: mmap 0x10000 bytes and check for errors */
    volatile uint8_t *dma = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, dma_fd, 0);
    // volatile because the memory is hardware. 
    if (dma == MAP_FAILED ) 
    { 
        perror("mmap uio0"); 
        return 1; 
    }

    /* 3. Buffer: the same for /dev/uio3, size 0x1000000, names buf_fd and buf */
    int buf_fd = open("/dev/uio3", O_RDWR | O_SYNC );
    if (buf_fd < 0) 
    { 
        perror("open uio3"); 
        return 1; 
    }
    volatile uint8_t *buf = mmap(NULL, 0x1000000, PROT_READ | PROT_WRITE, MAP_SHARED, buf_fd, 0);
    if (buf == MAP_FAILED ) 
    { 
        perror("mmap uio3"); 
        return 1; 
    }

    // Reset the DMA
    if (dma_reset(dma) != 0)
    {
        return 1;
    }
    printf("DMA reset done. MM2S status: 0x%08x\n", reg_read(dma, MM2S_DMASR));

    uint32_t len = 1048576;   // 1 MB: large enough to show the real DMA speed

    volatile uint32_t *src = (volatile uint32_t *)buf;
    volatile uint32_t *dst = (volatile uint32_t *)(buf + HALF);

    for (uint32_t i = 0; i < len / 4; i++) {
    /* your two lines */
    src[i] = i * 4 + 1;
    dst[i] = 0;
    }
    printf("src[0..2] = %u %u %u, dst[0] = %u\n", src[0], src[1], src[2], dst[0]);

    // Start the clock: everything from here to "both done" counts as transfer time
    long long t0 = now_ns();

    // Clear the old "done" flags (writing a 1 clears them)
    reg_write(dma, MM2S_DMASR, DMASR_DONE);
    reg_write(dma, S2MM_DMASR, DMASR_DONE);

    // Start the receiver (S2MM) first
    reg_write(dma, S2MM_DMACR, DMACR_RUN);          // switch the channel on
    reg_write(dma, S2MM_DA, BUF_PHYS + HALF);       // destination address (physical)
    reg_write(dma, S2MM_LENGTH, len);               // this starts the receiver

    // Then the sender (MM2S)
    reg_write(dma, MM2S_DMACR, DMACR_RUN);          // switch the channel on
    reg_write(dma, MM2S_SA, BUF_PHYS);              // source address (physical)
    reg_write(dma, MM2S_LENGTH, len);               // this starts the transfer

    // Wait until both channels report "done" (bit 12)
    int tries = 0;
    while (!(reg_read(dma, MM2S_DMASR) & DMASR_DONE) || !(reg_read(dma, S2MM_DMASR) & DMASR_DONE)) 
    {
        if (++tries > 10000000) {
            printf("ERROR: timeout. MM2S 0x%08x, S2MM 0x%08x\n",
                   reg_read(dma, MM2S_DMASR), reg_read(dma, S2MM_DMASR));
            return 1;
        }
    }
    long long t1 = now_ns();   // stop the clock
    printf("Transfer done. MM2S 0x%08x, S2MM 0x%08x\n", reg_read(dma, MM2S_DMASR), reg_read(dma, S2MM_DMASR));

    // Quick look at the received data
    printf("dst[0..2] = %u %u %u\n", dst[0], dst[1], dst[2]);

    // Compare every word that came back with what was sent
    uint32_t errors = 0;
    for (uint32_t i = 0; i < len / 4; i++) {
        if (dst[i] != src[i]) {
            if (errors < 5) {   // show only the first few mismatches
                printf("  mismatch at word %u: sent %u, got %u\n", i, src[i], dst[i]);
            }
            errors++;
        }
    }
    printf("%u bytes: %s (%u errors)\n", len, errors == 0 ? "PASS" : "FAIL", errors);

    // Speed: bytes moved / time taken
    double us   = (t1 - t0) / 1000.0;           // microseconds
    double mbps = len / (us / 1e6) / 1e6;        // MB per second (1 MB = 10^6 bytes)
    printf("time: %.1f us, %.1f MB/s\n", us, mbps);

    return errors == 0 ? 0 : 1;
}

