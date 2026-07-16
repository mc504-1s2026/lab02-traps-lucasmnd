#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include "kernel/string.h"

extern int _hartid[];

void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	char command_buf[256], input_buf[256];
	size_t command_index = 0;

	serial_puts("Shell initialized!\r\n");

	while(1){
		serial_puts("> ");

		size_t read_bytes = serial_read(input_buf);

		for(unsigned long i = 0; i < read_bytes; i++){
			char current_char = input_buf[i];

			serial_putc(input_buf[i]);

			if(current_char == '\r'){
				serial_putc('\n');
				command_buf[command_index + i] = '\0';

				char output[64];

				if(strncmp("uptime", command_buf, 6) == 0){
					snprintf(output, sizeof(output), "%ds\r\n", timer_read());
					serial_puts(output);
				} else if(strncmp("echo ", command_buf, 5) == 0){
					serial_puts(&command_buf[5]);
					serial_puts("\r\n");
				} else if(strncmp("alarm ", command_buf, 6) == 0){
					u64 seconds = strtou64(&command_buf[6], 10);
					timer_set_alarm(seconds);
					timer_irq_enable();
				} else {
					snprintf(output, sizeof(output), "Unknown Command: %s\r\n", command_buf);
					serial_puts(output);
				}


			} else {
				command_buf[command_index + i] = current_char;
			}
		}
		command_index += read_bytes;
	}
}