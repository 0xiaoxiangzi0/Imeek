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


/*OIDͨ �� Э �� :
  1:�� �� ʱ :  SCLΪ �ͣ�SDAΪ ��
  2:�� ʼ �� �� : SCL�� ��,�� �� 1us
  3:�� д /�� �� λ:   (1):SCL�� �� �� �� ƽ ��SDA�� �� �� �� Ҫ �� �� ƽ �� �� �� 2.7us(4us)(ȷ �� SDA�� ƽ �� �� )
                         (2):SCL�� �� ���� �� 1.3us(2us)(ȷ �� �� �� �� �� �� �� ȡ SDA�� �� �� �� )
                         (3):�� �� �� �� �� �� ��ֱ �� �� �� �� �� �� �ɣ�
  4:ͣ ֹ �� �� :SCK�� �� �� �� ƽ �� �� 42.7us(64us)

  5:�� �� �� ȡ OID�� ��:
  (1)�� �� �� ʼ �� ��
  (2)�� һ λ Ϊ �� д λ �� �� �� �� 3�� �� SDA�� ��
  (3)�� �� SCL1us���� SDA�� �� Ϊ �� �� ģ ʽ ��
  (4)�� �� SCL�� �� ƽ 2.7us(4us)���� �� SCL  1.3us(2us)���� ȡ SDA�� �� �� �� ��
  (5)�� �� �� �� (4),ֱ �� �� �� �� �� �� �� ��  (�� �� �� λ )
  (6)�� �� ͣ ֹ �� ��

  6:�� �� д �� OID�� �� :(�� �� �� �� �� �� �� �� �� �� �� �� 250ms)
  (1)�� �� �� ʼ �� ��
  (2)�� һ λ Ϊ �� д λ �� �� �� �� 3�� �� SDA�� ��
  (3) �� �� �� 3�� ��ֱ �� �� �� �� �� �� �� �� ��  ( �� �� �� λ )
  (4)SDA�� �� �� �� ���� �� ͣ ֹ �� ��
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

/*�� �� �� ʵ �� �� ʱ ʱ �� �� �� �� �� ֵ �� 0.6us�� ʱ �� */
void oid_delay_us(uint32_t us) 
{
	HAL_UDelay(us);
}

/*�� �� �� �� ʱ */
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
    //���Ͷ�дλ
    OID_SDA_1();
	oid_delay_us(3);
	OID_SCL_0();
	oid_delay_us(1);

	//��������λ
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
 *Description:  ��ȡOID�ϴ������ݣ��������ڽṹ����
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       ����OID_DATA_STRUCT���ͽṹ��
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
					*dwOidData1 += 1;//RecvData1�����32bit����
				}
				else{
                    *dwOidData2 += 1;//RecvData2�����32bit����
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
        
		oid_delay_us(78);  //������64bit���ݺ󣬲���һ��STOP������һ����������

//        printf("dwOidData1 = 0x%X\n",*dwOidData1);
//        printf("dwOidData2 = 0x%X\n",*dwOidData2);
		oid_data.flag = OID_DATA_VALID;
		oid_data.data_type	= *dwOidData1 >> 31; 
		/*�����֡Ϊ����֡��������֡��ʽ����*/
		if(1 == oid_data.data_type){
			oid_data.cmd = *dwOidData2 & 0xFFFF;
		}
		//������ͷ
		else{
			oid_data.oid_mode	= (*dwOidData1 >> 30) & 0x01;
			oid_data.code_type	= (*dwOidData1 >> 29) & 0x01;
			oid_data.valid_code = (*dwOidData1 >> 28) & 0x01;
		
			if(1 == oid_data.oid_mode){
				//λ����
				if(1 == oid_data.code_type){
					oid_data.position.y = (*dwOidData2 << 4) >> 18;
					oid_data.position.x = *dwOidData2 & 0x3FFF;
				}
				//Ȧͼ��
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
 *Description:  OID��ͷ���ϵ�ʱ�Ļ��ѣ�ʹ��ͷ����Normal Mode
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:��ͷ��ʼ���ɹ������Զ�ȡ��ͷ����
 *              FALSE:��ͷ��ʼ��ʧ�ܣ��ȴ����»���
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
	oid_delay_ms(30);  //SCK��������20ms,��С��2s
	OID_SCL_0();       //SN95500�˳�Configuration Mode,����Normal Mode
	
    while(1){
		oid_delay_ms(20);
		if(i++>=200){
			printf("WakeUpOID TIME OUT\n");
			break;      //û����⵽SN95500���ͻ������ݣ��˳������»���
		}

		if(!OID_SDA_READ()){
			oid_data = RecvOIDData(&RecvData1,&RecvData2);//Master��⵽SDIO���ͣ���ȡ����
			if((oid_data.flag == OID_DATA_VALID)
			&& (oid_data.cmd == 0x0000FFF8)){  //�յ���������0xFFF8,����ʧ�ܣ������»���
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
 *Description:  ��OID��ͷ����һ����������κμ��
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
 *Description:  ����OID��ͷ���ص�����Ӧ��
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
    
	oid_delay_us(78);  //������64bit���ݺ󣬲���һ��STOP������һ����������
	return (uint8_t)RecvData1;
}


/*
*******************************************************************************
 *Function:     TransmitCmdToOID
 *Description:  ��OID��ͷ����һ������,������Ƿ�ɹ�
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:����ͳɹ�
 *              1:��ʱ
 *              2:����ظ�����
 *Others:      
*******************************************************************************
*/
uint8_t TransmitCmdToOID(uint8_t Cmd)
{
    uint8_t TxCmd,RxAckCnt;
	uint32_t RecvData1 = 0;
	uint32_t RecvData2 = 0;

    //��������ǰҪ�ȼ��OID��ͷ�Ƿ�������Ҫ���͹���
	if(!OID_SDA_READ()){
        RecvOIDData(&RecvData1,&RecvData2);
	}

	TxCmd = Cmd;
	SendCmdToOID(TxCmd);

	RxAckCnt = 0;
	
	//�ȴ�SN95500���ط����ź�
	while(1){
        if(!OID_SDA_READ()){
			TxCmd = (uint8_t)RecvOIDAck();
			TxCmd -= 1;
			if(TxCmd == Cmd){
				printf("Cmd = %d OK\n",Cmd);
				return 0;    //�յ���ȷӦ��
			}
			else{
				printf("TxCmd = %d\n",TxCmd);
				printf("Cmd = %d ERROR\n",Cmd);
				return 2;   //�յ�����Ӧ��
			}
		}

		oid_delay_ms(30);
		RxAckCnt += 1;
		if(RxAckCnt >= 10){  //�ȴ�����300ms��û�лظ������ش���
			printf("cmd = %d OUT TIME\n",Cmd);
			return 1;         
		}      
	}
}

/*
*******************************************************************************
 *Function:     OID_Read_Dec_Info
 *Description:  ��ȡ����Ĵ���������
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
			//���Ҫ���ͣ�delayһ���Ա�cmd���ͳɹ�
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
				RecvOIDData(&dwOidData1, &dwOidData2);//��64bit����,(dwOidData2:bit63-bit32,dwOidData1:bit31-bit0)
                
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
 *Description:  д���ݵ�����Ĵ���
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
		//���Ҫ���ͣ�delayһ���Ա�cmd���ͳɹ�
		oid_delay_ms(1);
	}

	return Ret;
}

/*
*******************************************************************************
 *Function:     DecoderInit
 *Description:  ���ͷ���ͳ�ʼ���������9��������Ϊ�˽����ظ���������
 *              ʲô�Ǳ�ͷ�ظ���������:�ϱ���ʱ��Ҳͬʱ�ϴ��϶����Ч��
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
		//����80+fps
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
		//�˳�80+fps,������ϴ�cmd
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

