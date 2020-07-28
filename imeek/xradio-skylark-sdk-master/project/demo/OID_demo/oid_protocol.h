#ifndef _OID_PROTOCOL_H_
#define _OID_PROTOCOL_H_

#include <stdio.h>
#include "kernel/os/os.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_util.h"

#ifndef _BOOL_
#define _BOOL_
typedef unsigned char MYBOOL;
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif





#define OID_PIN_OUTPUT  0
#define OID_PIN_INPUT   1

#define OID_SCL_PORT		GPIO_PORT_A
#define OID_SCL_PIN			GPIO_PIN_4

#define OID_SDA_PORT		GPIO_PORT_A
#define OID_SDA_PIN			GPIO_PIN_5

//mode可选值:OID_PIN_OUTPUT、OID_PIN_INPUT
#define OID_SDA_PIN_MODE(mode)  do{                            \
		                            if(OID_PIN_INPUT == mode){ \
										oid_gpio_input_init();     \
									}                          \
									                           \
		                            if(OID_PIN_OUTPUT == mode){\
										oid_gpio_output_init();    \
									}                          \
	                            }while(0)       
#define OID_SCL_1()  HAL_GPIO_WritePin(OID_SCL_PORT, OID_SCL_PIN, 1)
#define OID_SCL_0()  HAL_GPIO_WritePin(OID_SCL_PORT, OID_SCL_PIN, 0)

#define OID_SDA_1()  HAL_GPIO_WritePin(OID_SDA_PORT, OID_SDA_PIN, 1)
#define OID_SDA_0()  HAL_GPIO_WritePin(OID_SDA_PORT, OID_SDA_PIN, 0)

#define OID_SDA_READ() HAL_GPIO_ReadPin(OID_SDA_PORT, OID_SDA_PIN)

/*OID模 组 命 令 */
#define OID_CMD_POSITION_ENABLE               0x35
#define OID_CMD_POSITION_DISABLE              0x36
#define OID_CMD_ENTER_SUSPEND_MODE            0x57
#define OID_CMD_MIN_FRAME_RATE_5              0x21
#define OID_CMD_MIN_FRAME_RATE_20             0x24
#define OID_CMD_MIN_FRAME_RATE_30             0x25
#define OID_CMD_MIN_FRAME_RATE_40             0x26
#define OID_CMD_MIN_FRAME_RATE_50             0x27
#define OID_CMD_MIN_FRAME_RATE_80             0x3c
#define OID_CMD_AUTO_SWITCH_TO_POS_ENABLE     0x43
#define OID_CMD_AUTO_SWITCH_TO_POS_DISABLE    0x44
#define OID_CMD_AUTO_SWITCH_TO_GEN_ENABLE     0x45
#define OID_CMD_AUTO_SWITCH_TO_GEN_DISABLE    0x46
#define OID_CMD_POSITION_WORKING_MODE_ENABLE  0x40
#define OID_CMD_POSITION_WORKING_MODE_DISABLE 0xB0
#define OID_CMD_OUTPUT_ANGLE_ENABLE           0x10
#define OID_CMD_OUTPUT_ANGLE_DISABLE          0x11

typedef struct{
    uint16_t y;
	uint16_t x;
}OID_POSITION_STRUCT;

#ifndef OID_DATA_VALID
#define OID_DATA_VALID     1
#define OID_DATA_NO_VALID  0
#endif

typedef enum{
    NO_VALID_CODE,
    POSITION_CODE,
	GENERAL_CODE,
}ENUM_CODE_TYPE;

typedef struct{
	uint8_t flag;                //1:结构体数据有效   0:结构体数据无效
    uint8_t data_type;           //1:command, 0: index
	uint8_t oid_mode;            //1:oid3   0:oid2
	uint8_t code_type;           //1:position code  0:General code
    uint8_t valid_code;          //1:code non-valid   0:code valid
    OID_POSITION_STRUCT position;//x、y position
    uint32_t index;              //圈图码码值
    uint16_t cmd;                //0xfff8:解码芯片进入正常模式
                                 //0xfff1:解码芯片出错，且已经重启
                                 //0xfff6:解码芯片进入校准模式，
                                 //       主控需要将sck与sdio设置为输入模式
}OID_DATA_STRUCT;

extern void OIDGpioInit(void);
extern uint8_t WakeUpOID(void);
extern OID_DATA_STRUCT RecvOIDData(uint32_t *dwOidData1,uint32_t *dwOidData2);
extern uint8_t TransmitCmdToOID(uint8_t Cmd);
uint8_t DecoderInit (void);

#endif
