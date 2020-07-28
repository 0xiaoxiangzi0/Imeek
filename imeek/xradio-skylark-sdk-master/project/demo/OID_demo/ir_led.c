#include <stdio.h>
#include "kernel/os/os.h"
#include "driver/chip/hal_gpio.h"
#include "ir_led.h"

void ir_led_gpio_init(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(LEFT_IR_LED_PORT ,LEFT_IR_LED_PIN, &param);
	HAL_GPIO_Init(RIGHT_IR_LED_PORT,RIGHT_IR_LED_PIN,&param);
	HAL_GPIO_Init(FRONT_IR_LED_PORT,FRONT_IR_LED_PIN,&param);
}

void IrEnableGpioInit(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(IR_ENABLE_PORT ,IR_ENABLE_PIN, &param);
	IR_DISABLE();
}


