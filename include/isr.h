#ifndef ISR_H
#define ISR_H

#include "defs.h"

void isrs_install();
void fault_handler(struct regs *r);

#endif
