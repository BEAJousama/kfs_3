#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_install(void);
void timer_phase(uint32_t hz);
uint32_t timer_get_ticks(void);
uint32_t timer_get_hz(void);
void timer_uptime(uint32_t *seconds, uint32_t *millis);

#endif
