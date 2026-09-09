#include "../include/kernel.h"

static char command_buffer[MAX_COMMAND_LENGTH];
static int command_pos = 0;

shell_command_t commands[] = {
    {"help",   "Show this help message",                  cmd_help},
    {"clear",  "Clear the screen",                        cmd_clear},
    {"echo",   "Echo the arguments",                      cmd_echo},
    {"gdt",    "Dump the Global Descriptor Table",        cmd_gdt},
    {"stack",  "Hex dump of the kernel stack (gdb style)", cmd_stack},
    {"regs",   "Dump CPU / control registers",            cmd_regs},
    {"eflags", "Decode the EFLAGS register",              cmd_eflags},
    {"cpuid",  "Show CPU vendor and features",            cmd_cpuid},
    {"mem",    "Hex dump memory: mem <addr> [words]",     cmd_mem},
    {"pmm",    "Physical frame allocator statistics",     cmd_pmm},
    {"paging", "Walk the page tables: paging <addr>",     cmd_paging},
    {"kmalloc","kmalloc <size>  (kernel/identity heap)",  cmd_kmalloc},
    {"kfree",  "kfree <addr>",                            cmd_kfree},
    {"ksize",  "ksize <addr>",                            cmd_ksize},
    {"vmalloc","vmalloc <size>  (virtual/mapped heap)",   cmd_vmalloc},
    {"vfree",  "vfree <addr>",                            cmd_vfree},
    {"vsize",  "vsize <addr>",                            cmd_vsize},
    {"heap",   "kheap / vheap usage summary",             cmd_heap},
    {"memtest","Stress-test both allocators",             cmd_memtest},
    {"uptime", "Time since boot (PIT ticks)",             cmd_uptime},
    {"time",   "Show the wall-clock time (RTC)",          cmd_time},
    {"fault",  "Trigger a CPU fault: fault <div0|bp|gp|ud|pf>", cmd_fault},
    {"panic",  "Trigger a test kernel panic",             cmd_panic},
    {"colors", "Print the VGA colour palette",            cmd_colors},
    {"reboot", "Reboot the machine",                      cmd_reboot},
    {"halt",   "Halt the CPU",                            cmd_halt},
    {NULL, NULL, NULL}
};

uint8_t cmos_read(uint8_t reg) {
    asm volatile("outb %1, %0" :: "dN"(0x70), "a"(reg));
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "dN"(0x71));
    return val;
}

uint8_t bcd_to_bin(uint8_t val) {
    return (val & 0x0F) + ((val / 16) * 10);
}

void read_rtc_time(uint8_t* hour, uint8_t* minute, uint8_t* second) {
    *second = bcd_to_bin(cmos_read(0x00));
    *minute = bcd_to_bin(cmos_read(0x02));
    *hour   = bcd_to_bin(cmos_read(0x04));
}

#define SHCOL(fg) vga_color_entry((fg), VGA_COLOR_BLACK)

void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;

    kprintf_color(SHCOL(VGA_COLOR_LIGHT_CYAN), "KFS_3 debug shell - commands\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        kprintf_color(SHCOL(VGA_COLOR_LIGHT_GREEN), "  %-8s", commands[i].name);
        kprintf_color(SHCOL(VGA_COLOR_LIGHT_GREY), " %s\n", commands[i].description);
    }
    kprintf_color(SHCOL(VGA_COLOR_DARK_GREY),
        "\n  TAB completes commands, BACKSPACE edits, ESC leaves the shell.\n"
        "  ALT+Left/Right switch screens, Up/Down scroll the backlog.\n");
    terminal_reset_color();
}

void cmd_stack(int argc, char **argv) {
    (void)argc; (void)argv;
    print_kernel_stack();
}

void cmd_gdt(int argc, char **argv) {
    (void)argc; (void)argv;
    print_gdt_info();
}

void cmd_regs(int argc, char **argv) {
    (void)argc; (void)argv;
    print_registers();
}

void cmd_eflags(int argc, char **argv) {
    (void)argc; (void)argv;
    print_eflags();
}

void cmd_cpuid(int argc, char **argv) {
    (void)argc; (void)argv;
    print_cpuid();
}

void cmd_mem(int argc, char **argv) {
    if (argc < 2) {
        kprintf("usage: mem <addr> [words]\n");
        kprintf("example: mem 0xB8000 16\n");
        return;
    }

    uint32_t addr = (uint32_t)kstrtoul(argv[1], 0);
    uint32_t words = (argc >= 3) ? (uint32_t)kstrtoul(argv[2], 0) : 16;

    if (words > 256) words = 256;
    dump_memory(addr, words);
}

void cmd_uptime(int argc, char **argv) {
    (void)argc; (void)argv;

    uint32_t sec, ms;
    timer_uptime(&sec, &ms);
    kprintf("up %u.%03u s  (%u ticks @ %u Hz)\n",
            sec, ms, timer_get_ticks(), timer_get_hz());
}

void cmd_fault(int argc, char **argv) {
    if (argc < 2) {
        kprintf("usage: fault <div0|bp|gp|ud>\n");
        return;
    }

    if (strcmp(argv[1], "div0") == 0) {
        volatile int a = 1, b = 0;
        volatile int c = a / b;
        (void)c;
    } else if (strcmp(argv[1], "bp") == 0) {
        __asm__ volatile ("int $3");
    } else if (strcmp(argv[1], "gp") == 0) {
        __asm__ volatile ("mov $0x38, %ax; mov %ax, %ds"); /* selector past GDT limit */
    } else if (strcmp(argv[1], "ud") == 0) {
        __asm__ volatile ("ud2"); /* invalid opcode -> #UD */
    } else if (strcmp(argv[1], "pf") == 0) {
        volatile int *p = (int *)0x30000000; /* mapped nowhere -> #PF */
        *p = 42;
    } else {
        kprintf("unknown fault type: %s\n", argv[1]);
    }
}

void cmd_pmm(int argc, char **argv) {
    (void)argc; (void)argv;

    uint32_t total = pmm_total_frames();
    uint32_t used  = pmm_used_frames();

    kprintf_color(vga_color_entry(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK),
                  "physical frame pool\n");
    kprintf("  frames total : %u  (%u KiB)\n", total, total * 4);
    kprintf("  frames used  : %u  (%u KiB)\n", used, used * 4);
    kprintf("  frames free  : %u  (%u KiB)\n", total - used, (total - used) * 4);
    terminal_reset_color();
}

void cmd_paging(int argc, char **argv) {
    if (argc < 2) {
        kprintf("usage: paging <addr>\n");
        return;
    }
    paging_dump((uint32_t)kstrtoul(argv[1], 0));
}

void cmd_kmalloc(int argc, char **argv) {
    if (argc < 2) { kprintf("usage: kmalloc <size>\n"); return; }
    size_t n = (size_t)kstrtoul(argv[1], 0);
    void *p = kmalloc(n);
    if (!p) { kprintf("kmalloc(%u) failed\n", (unsigned)n); return; }
    kprintf("kmalloc(%u) = 0x%08X  (ksize = %u)\n",
            (unsigned)n, (uint32_t)p, (unsigned)ksize(p));
}

void cmd_kfree(int argc, char **argv) {
    if (argc < 2) { kprintf("usage: kfree <addr>\n"); return; }
    kfree((void *)kstrtoul(argv[1], 0));
    kprintf("kfree done\n");
}

void cmd_ksize(int argc, char **argv) {
    if (argc < 2) { kprintf("usage: ksize <addr>\n"); return; }
    void *p = (void *)kstrtoul(argv[1], 0);
    kprintf("ksize(0x%08X) = %u\n", (uint32_t)p, (unsigned)ksize(p));
}

void cmd_vmalloc(int argc, char **argv) {
    if (argc < 2) { kprintf("usage: vmalloc <size>\n"); return; }
    size_t n = (size_t)kstrtoul(argv[1], 0);
    void *p = vmalloc(n);
    if (!p) { kprintf("vmalloc(%u) failed\n", (unsigned)n); return; }
    kprintf("vmalloc(%u) = 0x%08X  (vsize = %u, phys = 0x%08X)\n",
            (unsigned)n, (uint32_t)p, (unsigned)vsize(p),
            virt_to_phys((uint32_t)p));
}

void cmd_vfree(int argc, char **argv) {
    if (argc < 2) { kprintf("usage: vfree <addr>\n"); return; }
    vfree((void *)kstrtoul(argv[1], 0));
    kprintf("vfree done\n");
}

void cmd_vsize(int argc, char **argv) {
    if (argc < 2) { kprintf("usage: vsize <addr>\n"); return; }
    void *p = (void *)kstrtoul(argv[1], 0);
    kprintf("vsize(0x%08X) = %u\n", (uint32_t)p, (unsigned)vsize(p));
}

void cmd_heap(int argc, char **argv) {
    (void)argc; (void)argv;
    size_t u, f, b;

    kheap_stats(&u, &f, &b);
    kprintf_color(vga_color_entry(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK),
                  "kheap : used %u B, free %u B, %u blocks\n",
                  (unsigned)u, (unsigned)f, (unsigned)b);
    vheap_stats(&u, &f, &b);
    kprintf_color(vga_color_entry(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK),
                  "vheap : used %u B, free %u B, %u blocks\n",
                  (unsigned)u, (unsigned)f, (unsigned)b);
    terminal_reset_color();
}

void cmd_memtest(int argc, char **argv) {
    (void)argc; (void)argv;

    void *a[8];
    int ok = 1;

    for (int i = 0; i < 8; i++) {
        a[i] = kmalloc(64 * (i + 1));
        if (!a[i]) ok = 0;
        else memset(a[i], 0xAB, 64 * (i + 1));
    }
    for (int i = 0; i < 8; i += 2)
        kfree(a[i]);
    void *big = kmalloc(4096);
    if (!big) ok = 0;
    for (int i = 1; i < 8; i += 2)
        kfree(a[i]);
    kfree(big);

    void *v1 = vmalloc(9000);           /* spans >2 pages */
    void *v2 = vmalloc(200);
    if (!v1 || !v2) ok = 0;
    if (v1) memset(v1, 0x5A, 9000);
    vfree(v1);
    vfree(v2);

    kprintf_color(vga_color_entry(ok ? VGA_COLOR_LIGHT_GREEN : VGA_COLOR_LIGHT_RED,
                                  VGA_COLOR_BLACK),
                  "memtest: %s\n", ok ? "PASS" : "FAIL");
    terminal_reset_color();
    cmd_heap(0, 0);
}

void cmd_panic(int argc, char **argv) {
    (void)argc;
    panic("manual panic requested from shell (%s)", argv[0]);
}

void cmd_colors(int argc, char **argv) {
    (void)argc; (void)argv;

    static const char *names[16] = {
        "black", "blue", "green", "cyan", "red", "magenta", "brown", "lgrey",
        "dgrey", "lblue", "lgreen", "lcyan", "lred", "lmagenta", "yellow", "white"
    };
    for (int i = 0; i < 16; i++) {
        kprintf_color(vga_color_entry((enum vga_color)i, VGA_COLOR_BLACK),
                      " %2d %-9s", i, names[i]);
        if (i % 4 == 3) kprintf("\n");
    }
    terminal_reset_color();
}

void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    terminal_clear();
}

void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        kprintf("%s", argv[i]);
        if (i < argc - 1) kprintf(" ");
    }
    kprintf("\n");
}

void cmd_time(int argc, char **argv) {
    (void)argc; (void)argv;
    
    time_t current_time;
    read_rtc_time(&current_time.hour, &current_time.minute, &current_time.second);

    kprintf("Current Time: %02u:%02u:%02u\n", current_time.hour, current_time.minute, current_time.second);
}

void cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    
    kprintf("Rebooting system...\n");
    outb(0x64, 0xFE);
    __asm__ volatile ("cli");
    __asm__ volatile ("hlt");
}

void cmd_halt(int argc, char **argv) {
    (void)argc; (void)argv;
    
    kprintf("System halted. You can power off now.\n");
    __asm__ volatile ("cli");
    __asm__ volatile ("hlt");
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}

int parse_command(char *input, char **argv, int max_args) {
    int argc = 0;
    char *token = input;
    
    while (*token == ' ' || *token == '\t') token++;
    
    while (*token && argc < max_args - 1) {
        argv[argc++] = token;
        
        while (*token && *token != ' ' && *token != '\t') token++;
        
        if (*token) {
            *token = '\0';
            token++; 
            while (*token == ' ' || *token == '\t') token++;
        }
    }
    
    argv[argc] = NULL;
    return argc;
}

void execute_command(char *input) {
    char *argv[MAX_ARGS];
    int argc = parse_command(input, argv, MAX_ARGS);
    
    if (argc == 0) return;
    
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].function(argc, argv);
            return;
        }
    }
    
    kprintf("Command not found: %s\n", argv[0]);
    kprintf("Type 'help' for available commands.\n");
}

int find_matching_commands(const char* partial, char matches[][32], int max_matches) {
    int count = 0;
    int partial_len = strlen(partial);
    
    if (partial_len == 0) {
        for (int i = 0; commands[i].name != NULL && count < max_matches; i++) {
            strcpy(matches[count], commands[i].name);
            count++;
        }
        return count;
    }
    
    for (int i = 0; commands[i].name != NULL && count < max_matches; i++) {
        if (strncmp(commands[i].name, partial, partial_len) == 0) {
            strcpy(matches[count], commands[i].name);
            count++;
        }
    }
    
    return count;
}

void handle_tab_completion() {
    char matches[16][32];
    int match_count;
    
    if (command_pos >= MAX_COMMAND_LENGTH) command_pos = MAX_COMMAND_LENGTH - 1;
    command_buffer[command_pos] = '\0';

    char *current_word = command_buffer;
    
    while (*current_word == ' ' || *current_word == '\t') current_word++;
    
    bool completing_command = true;
    for (int i = 0; i < command_pos; i++) {
        if (command_buffer[i] == ' ' || command_buffer[i] == '\t') {
            completing_command = false;
            break;
        }
    }
    
    if (!completing_command) {
        kprintf("\nAvailable commands: ");
        for (int i = 0; commands[i].name != NULL; i++) {
            kprintf("%s ", commands[i].name);
        }
        kprintf("\n");
        shell_prompt();
        
        for (int i = 0; i < command_pos; i++) {
            terminal_putchar(command_buffer[i]);
        }
        return;
    }
    
    match_count = find_matching_commands(current_word, matches, 16);
    
    if (match_count == 0) {
        return;
    } else if (match_count == 1) {
        int current_len = strlen(current_word);
        int match_len = strlen(matches[0]);
        int chars_to_add = match_len - current_len;
        
        if (command_pos + chars_to_add < MAX_COMMAND_LENGTH - 1) {
            for (int i = 0; i < chars_to_add; i++) {
                command_buffer[command_pos] = matches[0][current_len + i];
                terminal_putchar(command_buffer[command_pos]);
                command_pos++;
            }
            
            if (command_pos < MAX_COMMAND_LENGTH - 2) {
                command_buffer[command_pos] = ' ';
                terminal_putchar(' ');
                command_pos++;
            }
        }
    } else {
        kprintf("\n");
        for (int i = 0; i < match_count; i++) {
            kprintf("%s ", matches[i]);
        }
        kprintf("\n");
        shell_prompt();
        for (int i = 0; i < command_pos; i++) {
            terminal_putchar(command_buffer[i]);
        }
        
        if (match_count > 1) {
            int common_len = strlen(matches[0]);
            
            for (int i = 1; i < match_count; i++) {
                int match_i_len = strlen(matches[i]);
                for (int j = 0; j < common_len && j < match_i_len; j++) {
                    if (matches[0][j] != matches[i][j]) {
                        common_len = j;
                        break;
                    }
                }
            }
            
            int current_len = strlen(current_word);
            if (common_len > current_len) {
                int chars_to_add = common_len - current_len;
                if (command_pos + chars_to_add < MAX_COMMAND_LENGTH - 1) {
                    for (int i = 0; i < chars_to_add; i++) {
                        command_buffer[command_pos] = matches[0][current_len + i];
                        terminal_putchar(command_buffer[command_pos]);
                        command_pos++;
                    }
                }
            }
        }
    }
    
    if (command_pos < MAX_COMMAND_LENGTH) {
        command_buffer[command_pos] = '\0';
    }
}

void shell_handle_key(char c) {
    if (c == 27)  /* ESC is consumed by the scancode layer; ignore here */
        return;

    switch (c) {
        case '\n':
        case '\r':
            kprintf("\n");
            
            if (command_pos >= MAX_COMMAND_LENGTH) command_pos = MAX_COMMAND_LENGTH - 1;
            command_buffer[command_pos] = '\0';
            
            if (command_pos > 0) {
                execute_command(command_buffer);
            }
            
            command_pos = 0;
            command_buffer[0] = '\0';
            shell_prompt();
            break;
            
        case '\b':
        case 127:
            if (command_pos > 0) {
                command_pos--;
                command_buffer[command_pos] = '\0';
                terminal_putchar('\b');
            }
            break;
            
        case '\t':
            handle_tab_completion();
            break;         
        default:
            if (command_pos < MAX_COMMAND_LENGTH - 1 && c >= 32 && c <= 126) {
                command_buffer[command_pos] = c;
                command_pos++;
                command_buffer[command_pos] = '\0';
                terminal_putchar(c);
            }
            break;
    }

}

void shell_prompt(void) {
    kprintf("Shell $> ");
}

void shell_init(void) {
    kprintf("\n=== KFS_3 Debug Shell ===\n");
    kprintf("Kernel From Scratch - memory edition\n");
    kprintf("Type 'help' for available commands\n");
    kprintf("========================\n\n");
    command_pos = 0;
    command_buffer[0] = '\0';
}

bool shell_active = false;

void shell_toggle(void) {
    shell_active = !shell_active;
    
    if (shell_active) {
        kprintf("\n[Shell activated - ESC to exit]\n");
        shell_init();
        shell_prompt();
    } else {
        kprintf("\n[Shell deactivated]\n");
    }
}

bool is_shell_active(void) {
    return shell_active;
}