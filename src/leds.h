#ifndef _LEDS_H_
#define _LEDS_H_


#define LED_GREEN_BIT 0x1
#define LED_RED_BIT 0x2
#define LED_BLUE_BIT 0x4

#define LED_FORWARD_BIT 0x1
#define LED_BACKWARD_BIT 0x2
#define LED_STOP_BIT 0x4

extern void leds_update(int led_bits);
extern void leds_set_rgb(int led_bits);

#endif