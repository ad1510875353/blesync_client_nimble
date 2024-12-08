#ifndef __GPIOCONTROL_H__
#define __GPIOCONTROL_H__

#include <driver/gpio.h>

#define LED1 12
#define LED2 13
#define IO_ISR 2
#define AUTO_LOW 3
#define ESP_INTR_FLAG_DEFAULT 0

void gpio_init(void);
void set_led_state(uint8_t state);
void generate_falling_edge();


#endif 