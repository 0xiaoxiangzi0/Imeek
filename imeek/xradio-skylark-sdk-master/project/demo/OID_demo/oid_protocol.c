#include "oid_protocol.h"

const uint32_t DataArray2 [9][2] = {
{0xDAD	,0x1388 },
{0xDAE	,0x1964 },
{0xDAF	,0xDAC  },
{0xDB0	,0x1964 },
{0xDB1	,0xDAC  },
{0x161f ,0xD52  },
{0xDA7	,0x6CF  },
{0xDB4	,0x2    },
{0xD95	,0x2    }
};

uint8_t m_TranMutiData[7];


/*OID通 信 协 议 :
  1:空 闲 时 :  SCL为 低，SDA为 高
  2:起 始 信 号 : SCL拉 高,等 待 1us
  3:读 写 /数 据 位:   (1):SCL保 持 高 电 平 ，SDA切 换 至 需 要 的 电 平 ， 保 持 2.7us(4us)(确 保 SDA电 平 稳 定 )
                         (2):SCL拉 低 ，保 持 1.3us(2us)(确 保 从 器 件 成 功 读 取 SDA上 的 数 据 )
                         (3):重 复 以 上 步 骤 ，直 到 数 据 发 送 完 成；
  4:停 止 信 号 :SCK保 持 低 电 平 超 过 42.7us(64us)

  5:如 何 读 取 OID数 据:
  (1)发 送 起 始 信 号
  (2)第 一 位 为 读 写 位 ， 依 照 第 3步 将 SDA拉 低
  (3)拉 高 SCL1us，将 SDA切 换 为 输 入 模 式 ；
  (4)保 持 SCL高 电 平 2.7us(4us)，拉 低 SCL  1.3us(2us)，读 取 SDA上 的 数 据 ；
  (5)重 复 步 骤 (4),直 到 所 有 的 数 据 结 束  (先 发 高 位 )
  (6)发 送 停 止 信 号

  6:如 何 写 入 OID数 据 :(两 个 命 令 间 的 间 距 必 须 大 于 250ms)
  (1)发 送 起 始 信 号
  (2)第 一 位 为 读 写 位 ， 依 照 第 3步 将 SDA拉 高
  (3) 重 复 第 3步 ，直 到 所 有 数 据 发 送 完 成  ( 先 发 高 位 )
  (4)SDA引 脚 拉 高 ，发 送 停 止 信 号
*/

void oid_gpio_input_init(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(OID_SDA_PORT, OID_SDA_PIN, &param);
}

void oid_gpio_output_init(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(OID_SDA_PORT, OID_SDA_PIN, &param);
}

/*该 函 数 实 际 延 时 时 间 会 比 输 入 值 多 0.6us的 时 间 */
void oid_delay_us(uint32_t us) 
{
	HAL_UDelay(us);
}

/*毫 秒 级 延 时 */
void oid_delay_ms(uint32_t ms) 
{
	OS_MSleep(ms);
}

void oid_stop(void)
{
    OID_SDA_PIN_MODE(OID_PIN_INPUT);
    OID_SCL_0();
    oid_delay_us(43);
	//	  OID_SCL_0();
}

void oid_send_data(uint8_t data)
{
    char i;
    //发送读写位
    OID_SDA_1();
	oid_delay_us(3);
	OID_SCL_0();
	oid_delay_us(1);

	//发送数据位
	for(i = 0; i<8; i++){
		OID_SCL_1();
		if(data & 0x80){
		    OID_SDA_1();
		}
		else{
		    OID_SDA_0();
		}
		data <<= 1;
		oid_delay_us(3);
		OID_SCL_0();
		oid_delay_us(1);
	}
}

void OIDGpioInit(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(OID_SCL_PORT, OID_SCL_PIN, &param);
    OID_SCL_0();

	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(OID_SDA_PORT, OID_SDA_PIN, &param);
}

/*
*******************************************************************************
 *Function:     RecvOIDData
 *Description:  读取OID上传的数据，将其存放在结构体中
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       返回OID_DATA_STRUCT类型结构体
 *Others:      
*******************************************************************************
*/
OID_DATA_STRUCT RecvOIDData(uint32_t *dwOidData1,uint32_t *dwOidData2)
{
	OID_DATA_STRUCT oid_data = {0};
    uint8_t i;

//	uint32_t RecvData1 = 0;
//	uint32_t RecvData2 = 0;

    if(!OID_SDA_READ()){
        OID_SCL_1();
	    OID_SDA_PIN_MODE(OID_PIN_OUTPUT);
	    OID_SDA_0();
        oid_delay_us(10);
        OID_SCL_0();
	    oid_delay_us(10);

		for(i=0; i<64; i++){
			OID_SCL_1();
			OID_SDA_PIN_MODE(OID_PIN_INPUT);
			oid_delay_us(10);
			OID_SCL_0();
			if(OID_SDA_READ()){
				if(i<32){
					*dwOidData1 += 1;//RecvData1保存高32bit数据
				}
				else{
                    *dwOidData2 += 1;//RecvData2保存低32bit数据
				}
			}

			if(i<31){
                *dwOidData1 <<= 1;
			}
			if(i>31 && i<63){
				*dwOidData2 <<= 1;
			}
			oid_delay_us(10);
		}
        
		oid_delay_us(78);  //接收完64bit数据后，产生一个STOP来结束一个传输周期

//        printf("dwOidData1 = 0x%X\n",*dwOidData1);
//        printf("dwOidData2 = 0x%X\n",*dwOidData2);
		oid_data.flag = OID_DATA_VALID;
		oid_data.data_type	= *dwOidData1 >> 31; 
		/*如果该帧为命令帧，按命令帧格式解析*/
		if(1 == oid_data.data_type){
			oid_data.cmd = *dwOidData2 & 0xFFFF;
		}
		//三代笔头
		else{
			oid_data.oid_mode	= (*dwOidData1 >> 30) & 0x01;
			oid_data.code_type	= (*dwOidData1 >> 29) & 0x01;
			oid_data.valid_code = (*dwOidData1 >> 28) & 0x01;
		
			if(1 == oid_data.oid_mode){
				//位置码
				if(1 == oid_data.code_type){
					oid_data.position.y = (*dwOidData2 << 4) >> 18;
					oid_data.position.x = *dwOidData2 & 0x3FFF;
				}
				//圈图码
				else{
					oid_data.index = *dwOidData2 & 0x3FFFFFF;
				}
			}
			else if(0 == oid_data.oid_mode){
				oid_data.index = *dwOidData2 & 0xFFFF;
			}
		}
    }
    else{
	    oid_data.flag = OID_DATA_NO_VALID;
	}
    return oid_data;
}

/*
*******************************************************************************
 *Function:     WakeUpOID
 *Description:  OID笔头刚上电时的唤醒，使笔头进入Normal Mode
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:笔头初始化成功，可以读取笔头数据
 *              FALSE:笔头初始化失败，等待重新唤醒
 *Others:      
*******************************************************************************
*/
uint8_t WakeUpOID(void)
{
	OID_DATA_STRUCT oid_data = {0};
    uint16_t i=0;
	uint32_t RecvData1 = 0;
	uint32_t RecvData2 = 0;
	uint8_t ret = FALSE;

//	taskENTER_CRITICAL();
	
	OID_SCL_1();
	oid_delay_ms(30);  //SCK拉高至少20ms,但小于2s
	OID_SCL_0();       //SN95500退出Configuration Mode,进入Normal Mode
	
    while(1){
		oid_delay_ms(20);
		if(i++>=200){
			printf("WakeUpOID TIME OUT\n");
			break;      //没有侦测到SN95500发送回馈数据，退出后，重新唤醒
		}

		if(!OID_SDA_READ()){
			oid_data = RecvOIDData(&RecvData1,&RecvData2);//Master侦测到SDIO拉低，收取数据
			if((oid_data.flag == OID_DATA_VALID)
			&& (oid_data.cmd == 0x0000FFF8)){  //收到的数据是0xFFF8,唤醒失败，需重新唤醒
				i = 0xFFFF;
				break;
			}
			else{
                printf("WakeUpOID DATA ERROR\n");
			}
			break;
		}
	}
	if(i==0xFFFF){
		ret = TRUE;
	}
	else{
		ret = FALSE;
	}
	
//	taskEXIT_CRITICAL();
	return ret;
}

/*
*******************************************************************************
 *Function:     SendCmdToOID
 *Description:  向OID笔头发送一个命令，不做任何检测
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
void SendCmdToOID(uint8_t Cmd)
{
    uint8_t i;
	
	OID_SCL_1();
	OID_SDA_PIN_MODE(OID_PIN_INPUT);
	oid_delay_us(10);
	OID_SCL_0();
	oid_delay_us(10);

	for(i=0; i<8; i++){
        OID_SCL_1();

		if(Cmd & 0x80){
			OID_SDA_PIN_MODE(OID_PIN_INPUT);
		}
		else{
			OID_SDA_PIN_MODE(OID_PIN_OUTPUT);
			OID_SDA_0();
		}
		oid_delay_us(10);
		OID_SCL_0();
		if(i==7){
			OID_SDA_PIN_MODE(OID_PIN_INPUT);
		}
		oid_delay_us(10);
		Cmd<<=1;
	}
	oid_delay_us(78);
}


/*
*******************************************************************************
 *Function:     RecvOIDAck
 *Description:  接收OID笔头返回的命令应答
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
uint8_t RecvOIDAck(void)
{
    uint8_t i;
	uint16_t RecvData1 = 0;
	
    OID_SCL_1();
    OID_SDA_PIN_MODE(OID_PIN_OUTPUT);
    OID_SDA_0();
    oid_delay_us(10);
    OID_SCL_0();
    oid_delay_us(10);

	for(i=0; i<16; i++){
		OID_SCL_1();
		OID_SDA_PIN_MODE(OID_PIN_INPUT);
		oid_delay_us(10);
		OID_SCL_0();
        RecvData1 <<= 1;
		if(OID_SDA_READ()){
		    RecvData1 += 1;
		}
		oid_delay_us(10);
	}
    
	oid_delay_us(78);  //接收完64bit数据后，产生一个STOP来结束一个传输周期
	return (uint8_t)RecvData1;
}


/*
*******************************************************************************
 *Function:     TransmitCmdToOID
 *Description:  向OID笔头发送一个命令,并检测是否成功
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:命令发送成功
 *              1:超时
 *              2:命令回复错误
 *Others:      
*******************************************************************************
*/
uint8_t TransmitCmdToOID(uint8_t Cmd)
{
    uint8_t TxCmd,RxAckCnt;
	uint32_t RecvData1 = 0;
	uint32_t RecvData2 = 0;

    //发送命令前要先检测OID笔头是否有数据要发送过来
	if(!OID_SDA_READ()){
        RecvOIDData(&RecvData1,&RecvData2);
	}

	TxCmd = Cmd;
	SendCmdToOID(TxCmd);

	RxAckCnt = 0;
	
	//等待SN95500返回反馈信号
	while(1){
        if(!OID_SDA_READ()){
			TxCmd = (uint8_t)RecvOIDAck();
			TxCmd -= 1;
			if(TxCmd == Cmd){
				printf("Cmd = %d OK\n",Cmd);
				return 0;    //收到正确应答
			}
			else{
				printf("TxCmd = %d\n",TxCmd);
				printf("Cmd = %d ERROR\n",Cmd);
				return 2;   //收到错误应答
			}
		}

		oid_delay_ms(30);
		RxAckCnt += 1;
		if(RxAckCnt >= 10){  //等待超过300ms还没有回复，返回错误
			printf("cmd = %d OUT TIME\n",Cmd);
			return 1;         
		}      
	}
}

/*
*******************************************************************************
 *Function:     OID_Read_Dec_Info
 *Description:  读取解码寄存器的数据
 *Calls:       
 *Called By:   
 *Input:        Cmd:OID command
 *              *Data1 -> Address
 *              *Data2 -> data
 *Output:       
 *Return:       0:success  and return *Data1 and *Data2
 *				1:Time out 2:ack error 3:length error 4:Cmd failure other: unknow error
 *              
 *Others:      
*******************************************************************************
*/
unsigned int OID_Read_Dec_Info (unsigned int Cmd, unsigned int *Data1, unsigned int *Data2)
{
	uint32_t dwOidData1;
	uint32_t dwOidData2;
	unsigned int i;
	unsigned int Ret;
	unsigned long TimeOutCount;
	unsigned int ErrorCount;

	Ret = -1;
	ErrorCount = 0;

	if (Cmd == 0x74)
	{
		m_TranMutiData[0] = Cmd;
		m_TranMutiData[1] = 3;
		m_TranMutiData[2] = *Data1 >> 8;
		m_TranMutiData[3] = *Data1 & 0x00FF;
		m_TranMutiData[4] = (m_TranMutiData[1] + m_TranMutiData[2] + m_TranMutiData[3]) & 0xFF;

			
		while (ErrorCount < 3)
		{
			for(i = 0;i < 5;i++)
			{
				Ret = TransmitCmdToOID (m_TranMutiData[i]);//tx 8bit cmd to OID, return=0 is tx success,1 is timeout,2 cmd ack is error
				if(Ret)
				{
					//tx cmd error,try again
					break;
				}
			}
			
			if (Ret == 2) 
			{
				ErrorCount++;
			}
			else 
			{
				break;
			}
			//如果要重送，delay一下以便cmd可送成功
			oid_delay_ms(1);
		}		
	}
	else
		return 4;
	
	if (!Ret)
	{
		TimeOutCount = 0;
		
		while (1) 
		{
			if(!(OID_SDA_READ()))//check sdio is low or not
			{
				RecvOIDData(&dwOidData1, &dwOidData2);//收64bit数据,(dwOidData2:bit63-bit32,dwOidData1:bit31-bit0)
                
				//parse and get valid receive data
				if ((dwOidData2 &  0xE0000000) == 0xE0000000)
				{
					//check addr,
					if(*Data1 != ((dwOidData1 >> 16 ) & 0xffff))
					{
						*Data1 = (dwOidData1 >> 16 ) & 0xffff;
					}
					//*Data1 = (dwOidData1 >> 16 ) & 0xffff;
					*Data2 = dwOidData1 & 0xffff;
					return 0;
				}
			}

			oid_delay_ms(1);
			TimeOutCount++;

			if (TimeOutCount > 280)	// Time out = 300ms
				return 1;
		}
	}

	return Ret;
}

/*
*******************************************************************************
 *Function:     OID_Write_Dec_Info
 *Description:  写数据到解码寄存器
 *Calls:       
 *Called By:   
 *Input:        Cmd:OID command
 *              *Data1 -> Address
 *              *Data2 -> data
 *Output:       
 *Return:       0:success  and return *Data1 and *Data2
 *				1:Time out 2:ack error 3:length error 4:Cmd failure 
 *              other: unknow error
 *              
 *Others:      
*******************************************************************************
*/
unsigned int OID_Write_Dec_Info (unsigned int Cmd, unsigned int *Data1 , unsigned int *Data2)
{
	unsigned int i;
	unsigned int Ret;
	unsigned int ErrorCount;

	Ret = -1;
	ErrorCount = 0;
									
	if (Cmd == 0x73)
	{
		m_TranMutiData[0] = Cmd;
		m_TranMutiData[1] = 5;
		m_TranMutiData[2] = *Data1 >> 8;
		m_TranMutiData[3] = *Data1 & 0x00FF;
		m_TranMutiData[4] = *Data2 >> 8;
		m_TranMutiData[5] = *Data2 & 0x00FF;
		m_TranMutiData[6] = (m_TranMutiData[1] + m_TranMutiData[2] + m_TranMutiData[3] + m_TranMutiData[4] + m_TranMutiData[5]) & 0xFF;
	}
	else
		return 4;	// Cmd failure

	while (ErrorCount < 3)
	{
		for(i = 0;i < 7;i++)
		{
			Ret = TransmitCmdToOID (m_TranMutiData[i]);
			if(Ret)
			{
				//tx cmd error,try again
				break;
			}
		}
	
		if (Ret != 0) 
		{
			ErrorCount++;
		}
		else 
		{
			break;
		}
		//如果要重送，delay一下以便cmd可送成功
		oid_delay_ms(1);
	}

	return Ret;
}

/*
*******************************************************************************
 *Function:     DecoderInit
 *Description:  向笔头发送初始化命令，整个9组命令是为了降低重复触发现象，
 *              什么是笔头重复触发现象:上报的时候也同时上传较多的无效码
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:success 1:Exceeded retry time other:unknow error
 *Others:      
*******************************************************************************
*/
uint8_t DecoderInit (void)
{
	unsigned int D11, D22, RetryCnt, i;

	D11 = 0x1670;
	D22 = 0;
	RetryCnt = 0;
	printf("patch cmd tx\r\n");
	while(OID_Read_Dec_Info(0x74, &D11 , &D22))
	{
		RetryCnt++;
		if(RetryCnt >=3)
		{
			//printf("read info fail\r\n");
			return 1;
		}
	}
	oid_delay_ms(1);

	if (D22 == 0x1168)
	{
		printf("0x1168 patch cmd TX\r\n");
		//进入80+fps
		for (i =0; i < 3; i++)//fail,retry 3times
		{
			if(!TransmitCmdToOID(0x3c))
			{
				break;
			}
		}
		if(i == 3)
		{
			printf("0xc8 cmd fail\r\n");
		}
		for (i =0; i < ((sizeof (DataArray2) / sizeof (unsigned int))/2); i++)
		{
			D11 = DataArray2[i][0];
			D22 = DataArray2[i][1];
			RetryCnt = 0;
			while(OID_Write_Dec_Info(0x73, &D11 , &D22))
			{
				RetryCnt++;
				if(RetryCnt >=3)
				{
					//printf("multi cmd fail\r\n");
					return 1;
				}
			}
			oid_delay_ms (1);
		}
		//退出80+fps,必需加上此cmd
		for (i =0; i < 3; i++)//fail,retry 3times
		{
			if(!TransmitCmdToOID(0x3B))
			{
				break;
			}
		}
		if(i == 3)
		{
			printf("0xc8 cmd fail\r\n");
		}

		
	}

//	//=====Dot filter threshold setting
//	RetryCnt = 0;
//	D11 = 0x1641;
//	D22 = 0;		
//	while(OID_Read_Dec_Info(0x74, &D11 , &D22))
//	{
//		RetryCnt++;
//		if(RetryCnt >=3)
//			return 1;
//	}
//	D22 = (unsigned int)(D22 & 0xFE0F);
//	D22 = D22 | 0x0040;//Bit8-Bit4=00100
//	
//	RetryCnt=0;
//	D11 = 0x1641;		
//	while(OID_Write_Dec_Info(0x73, &D11 , &D22))
//	{
//		RetryCnt++;
//		if(RetryCnt >=3)
//			return 1;
//	}
//	
//	//=====Center threshold setting
//	RetryCnt=0;
//	D11 = 0xD6A;
//	D22 = 0x1F;
//	while(OID_Write_Dec_Info(0x73, &D11 , &D22))
//	{
//		RetryCnt++;
//		if(RetryCnt >=3)
//			return 1;
//	}
//	
//	RetryCnt=0;
//	D11 = 0xD2C;
//	D22 = 0x1F;
//	while(OID_Write_Dec_Info(0x73, &D11 , &D22))
//	{
//		RetryCnt++;
//		if(RetryCnt >=3)
//			return 1;
//	}
	return 0;
    
}

