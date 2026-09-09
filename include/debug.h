#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

/* GDB-style memory examine: `words` 32-bit cells starting at `addr`. */
void dump_memory(uint32_t addr, uint32_t words);

/* Snapshot and pretty-print the CPU / control registers. */
void print_registers(void);

/* Decode the CPUID vendor string and feature bits. */
void print_cpuid(void);

/* Print the current EFLAGS with each bit named. */
void print_eflags(void);

#endif
