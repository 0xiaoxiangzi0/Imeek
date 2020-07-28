#ifndef _STEPPER_MOTOR_H_
#define _STEPPER_MOTOR_H_

#include <stdio.h>
#include "FreeRTOS.h"
#include "kernel/os/os.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_util.h"

#define MOTOR_MSG_DBG_ON			1
#define MOTOR_MSG_INF_ON			1
#define MOTOR_MSG_WRN_ON			1
#define MOTOR_MSG_ERR_ON			1
#define MOTOR_MSG_ABORT_ON			0
#define MOTOR_MSG_SYSLOG			printf
#define MOTOR_MSG_ABORT()			do { } while (0)

#define MOTOR_MSG_LOG(flags, fmt, arg...) \
    do {								\
        if (flags)						\
            MOTOR_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define MOTOR_MSG_DBG(fmt, arg...)	\
    MOTOR_MSG_LOG(MOTOR_MSG_DBG_ON, "[MOTOR MSG DBG] "fmt, ##arg)
#define MOTOR_MSG_INF(fmt, arg...)	\
    MOTOR_MSG_LOG(MOTOR_MSG_INF_ON, "[MOTOR MSG INF] "fmt, ##arg)
#define MOTOR_MSG_WRN(fmt, arg...)	\
    MOTOR_MSG_LOG(MOTOR_MSG_WRN_ON, "[MOTOR MSG WRN] "fmt, ##arg)
#define MOTOR_MSG_ERR(fmt, arg...)								\
    do {														\
        MOTOR_MSG_LOG(MOTOR_MSG_ERR_ON, "[MOTOR MSG ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);						\
        if (MOTOR_MSG_ABORT_ON)									\
            MOTOR_MSG_ABORT();									\
    } while (0)

/*最 大 速 度 档 位*/
#define STEPPER_MOTOR_MAX_SPEED 10

#define MOTOR_FORWARD  1
#define MOTOR_BACKWARD 0

#define STEPPER_MOTOR_EN_PORT	    GPIO_PORT_A
#define STEPPER_MOTOR_EN_PIN	    GPIO_PIN_8

#define LEFT_STEPPER_MOTOR_A_PORT		GPIO_PORT_A
#define LEFT_STEPPER_MOTOR_A_PIN	    GPIO_PIN_19

#define LEFT_STEPPER_MOTOR_B_PORT		GPIO_PORT_A
#define LEFT_STEPPER_MOTOR_B_PIN	    GPIO_PIN_20

#define RIGHT_STEPPER_MOTOR_A_PORT		GPIO_PORT_A
#define RIGHT_STEPPER_MOTOR_A_PIN	    GPIO_PIN_21

#define RIGHT_STEPPER_MOTOR_B1_PORT		GPIO_PORT_A
#define RIGHT_STEPPER_MOTOR_B1_PIN		GPIO_PIN_22
#define RIGHT_STEPPER_MOTOR_B0_PORT	    GPIO_PORT_A
#define RIGHT_STEPPER_MOTOR_B0_PIN	    GPIO_PIN_9

#define STEPPER_MOTOR_EN()   HAL_GPIO_WritePin(STEPPER_MOTOR_EN_PORT, STEPPER_MOTOR_EN_PIN, 1)
#define STEPPER_MOTOR_DIS()  HAL_GPIO_WritePin(STEPPER_MOTOR_EN_PORT, STEPPER_MOTOR_EN_PIN, 0)

#define LEFT_STEPPER_MOTOR_A_1()   HAL_GPIO_WritePin(LEFT_STEPPER_MOTOR_A_PORT, LEFT_STEPPER_MOTOR_A_PIN, 1)
#define LEFT_STEPPER_MOTOR_A_0()   HAL_GPIO_WritePin(LEFT_STEPPER_MOTOR_A_PORT, LEFT_STEPPER_MOTOR_A_PIN, 0)
#define LEFT_STEPPER_MOTOR_B_1()   HAL_GPIO_WritePin(LEFT_STEPPER_MOTOR_B_PORT, LEFT_STEPPER_MOTOR_B_PIN, 1)
#define LEFT_STEPPER_MOTOR_B_0()   HAL_GPIO_WritePin(LEFT_STEPPER_MOTOR_B_PORT, LEFT_STEPPER_MOTOR_B_PIN, 0)
#define RIGHT_STEPPER_MOTOR_A_1()  HAL_GPIO_WritePin(RIGHT_STEPPER_MOTOR_A_PORT, RIGHT_STEPPER_MOTOR_A_PIN, 1)
#define RIGHT_STEPPER_MOTOR_A_0()  HAL_GPIO_WritePin(RIGHT_STEPPER_MOTOR_A_PORT, RIGHT_STEPPER_MOTOR_A_PIN, 0)
#define RIGHT_STEPPER_MOTOR_B_1()  do{ \
 	                                   HAL_GPIO_WritePin(RIGHT_STEPPER_MOTOR_B1_PORT, RIGHT_STEPPER_MOTOR_B1_PIN, 1);\
  	                                   HAL_GPIO_WritePin(RIGHT_STEPPER_MOTOR_B0_PORT, RIGHT_STEPPER_MOTOR_B0_PIN, 0);\
                                   }while(0)
#define RIGHT_STEPPER_MOTOR_B_0()  do{ \
									   HAL_GPIO_WritePin(RIGHT_STEPPER_MOTOR_B1_PORT, RIGHT_STEPPER_MOTOR_B1_PIN, 0);\
								       HAL_GPIO_WritePin(RIGHT_STEPPER_MOTOR_B0_PORT, RIGHT_STEPPER_MOTOR_B0_PIN, 1);\
								   }while(0)

typedef enum{
    LEFT_MOTOR = 1,
    RIGHT_MOTOR,
    ALL_MOTOR
}ENUM_STEPPER_MOTOR;

typedef struct{
    ENUM_STEPPER_MOTOR motor;       /*LEFT_MOTOR,RIGHT_MOTOR*/
	uint8_t            speed;       /*1 到 STEPPER_MOTOR_MAX_SPEED*/
	uint8_t            direction;   /*MOTOR_FORWARD,MOTOR_BACKWARD*/
}STRUCT_STEPPER_MOTOR;

extern STRUCT_STEPPER_MOTOR left_motor_struct;
extern STRUCT_STEPPER_MOTOR right_motor_struct;
extern int MotorTaskCreate(void);
extern void adj_motor_state(uint8_t left_speed, uint8_t left_direction,
	                        uint8_t right_speed, uint8_t right_direction);

#endif
