#include "rgb_led.h"

void rgb_gpio_init(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(LED_R_PORT, LED_R_PIN, &param);
    HAL_GPIO_Init(LED_G_PORT, LED_G_PIN, &param);
	HAL_GPIO_Init(LED_B_PORT, LED_B_PIN, &param);

	LED_R_OFF();
}

void led_ear_gpio_init(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(LED_EAR_PORT, LED_EAR_PIN, &param);
    
	LED_EAR_OFF();
}

