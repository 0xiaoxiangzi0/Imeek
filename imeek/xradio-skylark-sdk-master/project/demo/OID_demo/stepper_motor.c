#include "stepper_motor.h"


STRUCT_STEPPER_MOTOR left_motor_struct  = {LEFT_MOTOR,0,MOTOR_FORWARD};
STRUCT_STEPPER_MOTOR right_motor_struct = {RIGHT_MOTOR,0,MOTOR_FORWARD};

#define LEFT_STEPPER_MOTOR_TASK_CTRL_THREAD_STACK_SIZE		(1 * 1024)
#define RIGHT_STEPPER_MOTOR_TASK_CTRL_THREAD_STACK_SIZE   (1 * 1024)

static OS_Thread_t left_stepper_motor_handler;
static OS_Thread_t right_stepper_motor_handler;

static void stepper_motor_delay(uint32_t ms)
{
    OS_MSleep(ms);
}

void adj_motor_state(uint8_t left_speed, uint8_t left_direction,
	                 uint8_t right_speed, uint8_t right_direction)
{
    left_motor_struct.speed      = left_speed;
    left_motor_struct.direction  = left_direction;
    right_motor_struct.speed     = right_speed;
    right_motor_struct.direction = right_direction;
}

/*左 电 机 前 进 */
static void left_stepper_motor_forward(uint8_t speed)
{	
	LEFT_STEPPER_MOTOR_A_0();
	LEFT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	LEFT_STEPPER_MOTOR_A_1();
	LEFT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	LEFT_STEPPER_MOTOR_A_1();
	LEFT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	LEFT_STEPPER_MOTOR_A_0();
	LEFT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
}

/*左 电 机 后 退 */
static void left_stepper_motor_backward(uint8_t speed)
{
	LEFT_STEPPER_MOTOR_A_0();
	LEFT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	LEFT_STEPPER_MOTOR_A_0();
	LEFT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	LEFT_STEPPER_MOTOR_A_1();
	LEFT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	LEFT_STEPPER_MOTOR_A_1();
	LEFT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
}

/*右 电 机 前 进 */
static void right_stepper_motor_forward(uint8_t speed)
{
	RIGHT_STEPPER_MOTOR_A_0();
	RIGHT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	RIGHT_STEPPER_MOTOR_A_0();
	RIGHT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	RIGHT_STEPPER_MOTOR_A_1();
	RIGHT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	RIGHT_STEPPER_MOTOR_A_1();
	RIGHT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
}

/*右 电 机 后 退 */
static void right_stepper_motor_backward(uint8_t speed)
{
	RIGHT_STEPPER_MOTOR_A_0();
	RIGHT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	RIGHT_STEPPER_MOTOR_A_1();
	RIGHT_STEPPER_MOTOR_B_0();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	RIGHT_STEPPER_MOTOR_A_1();
	RIGHT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
	RIGHT_STEPPER_MOTOR_A_0();
	RIGHT_STEPPER_MOTOR_B_1();
	stepper_motor_delay(STEPPER_MOTOR_MAX_SPEED-speed+1);
}

/*先 将 步 进 电 机 的 转 子 位 置 调 整 到 初 始 位 置 ，为 起 转 做 准 备 */
void stepper_motor_prepare_turn(ENUM_STEPPER_MOTOR motor)
{
	STEPPER_MOTOR_EN();
    if(LEFT_MOTOR & motor){
		MOTOR_MSG_DBG("left_motor enable\n");
		LEFT_STEPPER_MOTOR_A_0();
		LEFT_STEPPER_MOTOR_B_0();
	}
    if(RIGHT_MOTOR & motor){
		MOTOR_MSG_DBG("right_motor enable\n");
		RIGHT_STEPPER_MOTOR_A_0();
		RIGHT_STEPPER_MOTOR_B_0();
	}	
	stepper_motor_delay(100);
}

/*步 进 电 机 转 动 函 数 */
void stepper_motor_turn(STRUCT_STEPPER_MOTOR motor)
{
    if(0 == motor.speed){
		if((0 == left_motor_struct.speed) && (0 == right_motor_struct.speed)){
			STEPPER_MOTOR_DIS();
			stepper_motor_delay(10);
		}
		return;
	}
    else if(motor.speed  > STEPPER_MOTOR_MAX_SPEED){
	    motor.speed  = STEPPER_MOTOR_MAX_SPEED;
	}
	
	STEPPER_MOTOR_EN();
	if((LEFT_MOTOR == motor.motor) && (MOTOR_FORWARD == motor.direction)){
		left_stepper_motor_forward(motor.speed);
	}
	if((LEFT_MOTOR == motor.motor) && (MOTOR_BACKWARD == motor.direction)){
		left_stepper_motor_backward(motor.speed);
	}
	if((RIGHT_MOTOR == motor.motor) && (MOTOR_FORWARD == motor.direction)){
		right_stepper_motor_forward(motor.speed);
	}
	if((RIGHT_MOTOR == motor.motor) && (MOTOR_BACKWARD == motor.direction)){
		right_stepper_motor_backward(motor.speed);
	}
}

/*初 始 化 步 进 电 机 IO口 ，默 认 失 能 */
void stepper_motor_gpio_init(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(STEPPER_MOTOR_EN_PORT,       STEPPER_MOTOR_EN_PIN,      &param);
	HAL_GPIO_Init(LEFT_STEPPER_MOTOR_A_PORT,   LEFT_STEPPER_MOTOR_A_PIN,  &param);
	HAL_GPIO_Init(LEFT_STEPPER_MOTOR_B_PORT,   LEFT_STEPPER_MOTOR_B_PIN,  &param);
	HAL_GPIO_Init(RIGHT_STEPPER_MOTOR_A_PORT,  RIGHT_STEPPER_MOTOR_A_PIN, &param);
	HAL_GPIO_Init(RIGHT_STEPPER_MOTOR_B0_PORT, RIGHT_STEPPER_MOTOR_B0_PIN,&param);
	HAL_GPIO_Init(RIGHT_STEPPER_MOTOR_B1_PORT, RIGHT_STEPPER_MOTOR_B1_PIN,&param);

	STEPPER_MOTOR_DIS();
}

static void left_stepper_motor_task(void *pvParameters)
{
	MOTOR_MSG_DBG("stepper motor create success, handler = 0x%x\n", (uint32_t)left_stepper_motor_handler.handle);
	
    while(1){
		stepper_motor_turn(left_motor_struct);
	}
}

static void right_stepper_motor_task(void *pvParameters)
{
	MOTOR_MSG_DBG("stepper motor create success, handler = 0x%x\n", (uint32_t)right_stepper_motor_handler.handle);
	
    while(1){
		stepper_motor_turn(right_motor_struct);
	}
}

int MotorTaskCreate(void)
{
	stepper_motor_gpio_init();
	stepper_motor_prepare_turn(ALL_MOTOR);

	if (OS_ThreadCreate(&left_stepper_motor_handler,
						"left_stepper_motor_task",
						left_stepper_motor_task,
						NULL,
						OS_THREAD_PRIO_APP,
						LEFT_STEPPER_MOTOR_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		MOTOR_MSG_DBG("left stepper motor thread create error\n");
		return -1;
	}
    if (OS_ThreadCreate(&right_stepper_motor_handler,
						"right_stepper_motor_task",
						right_stepper_motor_task,
						NULL,
						OS_THREAD_PRIO_APP,
						RIGHT_STEPPER_MOTOR_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		MOTOR_MSG_DBG("right stepper motor thread create error\n");
		return -1;
	}	
	return 0;
}

