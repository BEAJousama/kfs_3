#ifndef KBDUS_H
#define KBDUS_H

#include "defs.h"

void keyboard_handler(struct regs *r);
void keyboard_install();
void shell_toggle();


#endif
