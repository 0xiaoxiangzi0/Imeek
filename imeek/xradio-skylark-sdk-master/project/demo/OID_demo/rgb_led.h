#ifndef __RGB_LED_H__
#define __RGB_LED_H__

#include <stdio.h>
#include "driver/chip/hal_gpio.h"

/*LED_R与 串 口 接 收 同 引 脚 ，使 用 时 要 注 意 */
#define LED_R_PORT		GPIO_PORT_B
#define LED_R_PIN    	GPIO_PIN_1

#define LED_G_PORT		GPIO_PORT_A
#define LED_G_PIN		GPIO_PIN_6

#define LED_B_PORT		GPIO_PORT_A
#define LED_B_PIN		GPIO_PIN_11

#define LED_EAR_PORT	GPIO_PORT_A
#define LED_EAR_PIN		GPIO_PIN_7

#define LED_EYES_ALL_OFF() do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 1);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 1);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 1);\
                           }while(0)

#define LED_R_ON()         do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 0);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 1);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 1);\
                           }while(0)
#define LED_R_OFF()        LED_EYES_ALL_OFF()


#define LED_G_ON()         do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 1);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 0);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 1);\
                           }while(0)
#define LED_G_OFF()        LED_EYES_ALL_OFF()

#define LED_B_ON()         do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 1);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 1);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 0);\
                           }while(0)
#define LED_B_OFF()        LED_EYES_ALL_OFF()

//眼睛灯品红色
#define LED_PINKISH_ON()   do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 0);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 1);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 0);\
                           }while(0)
#define LED_PINKISH_OFF()  LED_EYES_ALL_OFF()

//眼睛灯黄色
#define LED_YELLOW_ON()    do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 0);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 0);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 1);\
                           }while(0)
#define LED_YELLOW_OFF()   LED_EYES_ALL_OFF()

//眼睛灯青色
#define LED_CYAN_ON()      do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 1);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 0);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 0);\
                           }while(0)
#define LED_CYAN_OFF()     LED_EYES_ALL_OFF()

//眼睛灯白色
#define LED_WHITE_ON()     do{\
                               HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, 0);\
                               HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, 0);\
                               HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, 0);\
                           }while(0)
#define LED_WHITE_OFF()    LED_EYES_ALL_OFF()


#define LED_EAR_ON()   HAL_GPIO_WritePin(LED_EAR_PORT, LED_EAR_PIN, 0)
#define LED_EAR_OFF()  HAL_GPIO_WritePin(LED_EAR_PORT, LED_EAR_PIN, 1)

void rgb_gpio_init(void);
void led_ear_gpio_init(void);

#endif

