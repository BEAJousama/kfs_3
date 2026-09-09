#include "../include/kernel.h"

/* Programmable Interval Timer (channel 0) driving IRQ0. */

#define PIT_BASE_FREQ 1193182u

static volatile uint32_t timer_ticks = 0;
static uint32_t timer_hz = 100;

static void timer_handler(struct regs *r)
{
    (void)r;
    timer_ticks++;
}

void timer_phase(uint32_t hz)
{
    if (hz == 0)
        hz = 100;

    uint32_t divisor = PIT_BASE_FREQ / hz;

    outb(0x43, 0x36);                       /* channel 0, lo/hi byte, mode 3 */
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    timer_hz = hz;
}

void timer_install(void)
{
    timer_phase(100);
    irq_install_handler(0, timer_handler);
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}

uint32_t timer_get_hz(void)
{
    return timer_hz;
}

void timer_uptime(uint32_t *seconds, uint32_t *millis)
{
    uint32_t t = timer_ticks;

    if (seconds)
        *seconds = t / timer_hz;
    if (millis)
        *millis = (t % timer_hz) * 1000u / timer_hz;
}
