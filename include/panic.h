#ifndef PANIC_H
#define PANIC_H

/* Fatal: print a red panic screen and halt the CPU for ever. */
void panic(const char *fmt, ...) __attribute__((noreturn));

/* Non-fatal: print a yellow warning banner and return to the caller. */
void panic_nonfatal(const char *fmt, ...);

/* Dump a CPU exception frame (used by the fault handler). */
struct regs;
void panic_from_regs(struct regs *r);

#endif
