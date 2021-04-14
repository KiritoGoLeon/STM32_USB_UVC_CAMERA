#include "driver_qspi_flash.h"
#include "quadspi.h"


uint16_t QSPI_FLASH_TYPE=W25Q128;	//Ĭ����W25Q256
uint8_t QSPI_FLASH_QPI_MODE=0;		//QSPIģʽ��־:0,SPIģʽ;1,QPIģʽ.


static void QSPI_FLASH_Qspi_Enable(void);			//ʹ��QSPIģʽ
static void QSPI_FLASH_Qspi_Disable(void);			//�ر�QSPIģʽ
static uint8_t QSPI_FLASH_ReadSR(uint8_t regno);             //��ȡ״̬�Ĵ��� 
static void QSPI_FLASH_4ByteAddr_Enable(void);     //ʹ��4�ֽڵ�ַģʽ
static void QSPI_FLASH_Write_SR(uint8_t regno,uint8_t sr);   //д״̬�Ĵ���
static void QSPI_FLASH_Write_Enable(void);  		//дʹ�� 
static void QSPI_FLASH_Write_Disable(void);		//д����
static void QSPI_FLASH_Wait_Busy(void);           	//�ȴ�����


static uint8_t QSPI_Receive(uint8_t* buf,uint32_t datalen);
static uint8_t QSPI_Transmit(uint8_t* buf,uint32_t datalen);
static void QSPI_Send_CMD(uint32_t instruction,uint32_t address,uint32_t dummyCycles,uint32_t instructionMode,uint32_t addressMode,uint32_t addressSize,uint32_t dataMode);

//4KbytesΪһ��Sector
//16������Ϊ1��Block
//W25Q256
//����Ϊ32M�ֽ�,����512��Block,8192��Sector

//��ʼ��SPI FLASH��IO��
void QSPI_FLASH_Init(void)
{
    uint8_t temp;

    QSPI_FLASH_Qspi_Enable();			//ʹ��QSPIģʽ
    QSPI_FLASH_TYPE=QSPI_FLASH_ReadID();	//��ȡFLASH ID.
    printf("ID:%x\r\n",QSPI_FLASH_TYPE);
    if(QSPI_FLASH_TYPE==W25Q128)        //SPI FLASHΪW25Q256
    {
        temp=QSPI_FLASH_ReadSR(3);      //��ȡ״̬�Ĵ���3���жϵ�ַģʽ
        if((temp&0X01)==0)			//�������4�ֽڵ�ַģʽ,�����4�ֽڵ�ַģʽ
        {
            QSPI_FLASH_Write_Enable();	//дʹ��
            QSPI_Send_CMD(W25X_Enable4ByteAddr,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);//QPI,ʹ��4�ֽڵ�ַָ��,��ַΪ0,������_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,0���ֽ�����
        }
        QSPI_FLASH_Write_Enable();		//дʹ��
        QSPI_Send_CMD(W25X_SetReadParam,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_4_LINES); 		//QPI,���ö�����ָ��,��ַΪ0,4�ߴ�����_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,1���ֽ�����
        temp=3<<4;					//����P4&P5=11,8��dummy clocks,104M
        QSPI_Transmit(&temp,1);		//����1���ֽ�
    }
}


//����ֵ����:
//0XEF13,��ʾоƬ�ͺ�ΪW25Q80
//0XEF14,��ʾоƬ�ͺ�ΪW25Q16
//0XEF15,��ʾоƬ�ͺ�ΪW25Q32
//0XEF16,��ʾоƬ�ͺ�ΪW25Q64
//0XEF17,��ʾоƬ�ͺ�ΪW25Q128
//0XEF18,��ʾоƬ�ͺ�ΪW25Q256
uint16_t QSPI_FLASH_ReadID(void)
{
    uint8_t temp[2];
    uint16_t deviceid;
    if(QSPI_FLASH_QPI_MODE)
			QSPI_Send_CMD(W25X_ManufactDeviceID,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_4_LINES,QSPI_USE_ADRESS_BITS,QSPI_DATA_4_LINES);//QPI,��id,��ַΪ0,4�ߴ�������_24λ��ַ_4�ߴ����ַ_4�ߴ���ָ��,�޿�����,2���ֽ�����
    else 
			QSPI_Send_CMD(W25X_ManufactDeviceID,0,0,QSPI_INSTRUCTION_1_LINE,QSPI_ADDRESS_1_LINE,QSPI_USE_ADRESS_BITS,QSPI_DATA_1_LINE);			//SPI,��id,��ַΪ0,���ߴ�������_24λ��ַ_���ߴ����ַ_���ߴ���ָ��,�޿�����,2���ֽ�����
    QSPI_Receive(temp,2);
    deviceid=(temp[0]<<8)|temp[1];
    return deviceid;
}

//��ȡSPI FLASH,��֧��QPIģʽ
//��ָ����ַ��ʼ��ȡָ�����ȵ�����
//pBuffer:���ݴ洢��
//ReadAddr:��ʼ��ȡ�ĵ�ַ(���32bit)
//NumByteToRead:Ҫ��ȡ���ֽ���(���65535)
void QSPI_FLASH_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead)
{
    QSPI_Send_CMD(W25X_FastReadData,ReadAddr,8,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_4_LINES,QSPI_USE_ADRESS_BITS,QSPI_DATA_4_LINES);	//QPI,���ٶ�����,��ַΪReadAddr,4�ߴ�������_32λ��ַ_4�ߴ����ַ_4�ߴ���ָ��,8������,NumByteToRead������
    QSPI_Receive(pBuffer,NumByteToRead);
}

//SPI��һҳ(0~65535)��д������256���ֽڵ�����
//��ָ����ַ��ʼд�����256�ֽڵ�����
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(���32bit)
//NumByteToWrite:Ҫд����ֽ���(���256),������Ӧ�ó�����ҳ��ʣ���ֽ���!!!
void QSPI_FLASH_Write_Page(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
    QSPI_FLASH_Write_Enable();					//дʹ��
    QSPI_Send_CMD(W25X_PageProgram,WriteAddr,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_4_LINES,QSPI_USE_ADRESS_BITS,QSPI_DATA_4_LINES);	//QPI,ҳдָ��,��ַΪWriteAddr,4�ߴ�������_32λ��ַ_4�ߴ����ַ_4�ߴ���ָ��,�޿�����,NumByteToWrite������
    QSPI_Transmit(pBuffer,NumByteToWrite);
    QSPI_FLASH_Wait_Busy();					   //�ȴ�д�����
}

//�޼���дSPI FLASH
//����ȷ����д�ĵ�ַ��Χ�ڵ�����ȫ��Ϊ0XFF,�����ڷ�0XFF��д������ݽ�ʧ��!
//�����Զ���ҳ����
//��ָ����ַ��ʼд��ָ�����ȵ�����,����Ҫȷ����ַ��Խ��!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(���32bit)
//NumByteToWrite:Ҫд����ֽ���(���65535)
//CHECK OK
void QSPI_FLASH_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
    uint16_t pageremain;
    pageremain=256-WriteAddr%256; //��ҳʣ����ֽ���
    if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//������256���ֽ�
    while(1)
    {
        QSPI_FLASH_Write_Page(pBuffer,WriteAddr,pageremain);
        if(NumByteToWrite==pageremain)break;//д�������
        else //NumByteToWrite>pageremain
        {
            pBuffer+=pageremain;
            WriteAddr+=pageremain;

            NumByteToWrite-=pageremain;			  //��ȥ�Ѿ�д���˵��ֽ���
            if(NumByteToWrite>256)pageremain=256; //һ�ο���д��256���ֽ�
            else pageremain=NumByteToWrite; 	  //����256���ֽ���
        }
    }
}

//дSPI FLASH
//��ָ����ַ��ʼд��ָ�����ȵ�����
//�ú�������������!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(���32bit)
//NumByteToWrite:Ҫд����ֽ���(���65535)
uint8_t QSPI_FLASH_BUFFER[4096];
void QSPI_FLASH_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;
    uint8_t * QSPI_FLASH_BUF;
    QSPI_FLASH_BUF=QSPI_FLASH_BUFFER;
    secpos=WriteAddr/4096;//������ַ
    secoff=WriteAddr%4096;//�������ڵ�ƫ��
    secremain=4096-secoff;//����ʣ��ռ��С
    //printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//������
    if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//������4096���ֽ�
    while(1)
    {
        QSPI_FLASH_Read(QSPI_FLASH_BUF,secpos*4096,4096);//������������������
        for(i=0; i<secremain; i++) //У������
        {
            if(QSPI_FLASH_BUF[secoff+i]!=0XFF)break;//��Ҫ����
        }
        if(i<secremain)//��Ҫ����
        {
            QSPI_FLASH_Erase_Sector(secpos);//�����������
            for(i=0; i<secremain; i++)	 //����
            {
                QSPI_FLASH_BUF[i+secoff]=pBuffer[i];
            }
            QSPI_FLASH_Write_NoCheck(QSPI_FLASH_BUF,secpos*4096,4096);//д����������

        } else QSPI_FLASH_Write_NoCheck(pBuffer,WriteAddr,secremain);//д�Ѿ������˵�,ֱ��д������ʣ������.
        if(NumByteToWrite==secremain)break;//д�������
        else//д��δ����
        {
            secpos++;//������ַ��1
            secoff=0;//ƫ��λ��Ϊ0

            pBuffer+=secremain;  //ָ��ƫ��
            WriteAddr+=secremain;//д��ַƫ��
            NumByteToWrite-=secremain;				//�ֽ����ݼ�
            if(NumByteToWrite>4096)secremain=4096;	//��һ����������д����
            else secremain=NumByteToWrite;			//��һ����������д����
        }
    };
}

//��������оƬ
//�ȴ�ʱ�䳬��...
void QSPI_FLASH_Erase_Chip(void)
{
    QSPI_FLASH_Write_Enable();					//SET WEL
    QSPI_FLASH_Wait_Busy();
    QSPI_Send_CMD(W25X_ChipErase,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);//QPI,дȫƬ����ָ��,��ַΪ0,������_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,0���ֽ�����
    QSPI_FLASH_Wait_Busy();						//�ȴ�оƬ��������
}

//����һ������
//Dst_Addr:������ַ ����ʵ����������
//����һ������������ʱ��:150ms
void QSPI_FLASH_Erase_Sector(uint32_t Dst_Addr)
{

    Dst_Addr*=4096;
    QSPI_FLASH_Write_Enable();                  //SET WEL
    QSPI_FLASH_Wait_Busy();
    QSPI_Send_CMD(W25X_SectorErase,Dst_Addr,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_4_LINES,QSPI_USE_ADRESS_BITS,QSPI_DATA_NONE);//QPI,д��������ָ��,��ַΪ0,������_32λ��ַ_4�ߴ����ַ_4�ߴ���ָ��,�޿�����,0���ֽ�����
    QSPI_FLASH_Wait_Busy();   				    //�ȴ��������
}


/**
  * @brief  ����QSPIΪ�ڴ�ӳ��ģʽ
  * @retval QSPI�ڴ�״̬
  */
uint32_t QSPI_EnableMemoryMappedMode()
{
 QSPI_CommandTypeDef      s_command;
  QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;
 
  /* Configure the command for the read instruction */
	s_command.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  s_command.Instruction       = 0xeb;
  s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
  s_command.AddressSize       = QSPI_USE_ADRESS_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
	s_command.AlternateBytesSize = 0;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = 6;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_HALF_CLK_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;


 
  /* Configure the memory mapped mode */
  s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  s_mem_mapped_cfg.TimeOutPeriod     = 0;
 


  if (HAL_QSPI_MemoryMapped(&hqspi, &s_command, &s_mem_mapped_cfg) != HAL_OK)
  {
    return 0;
  }
	
  return 1;
}



/// ------------API   END--------

//W25QXX����QSPIģʽ
static void QSPI_FLASH_Qspi_Enable(void)
{
    uint8_t stareg2;
    stareg2=QSPI_FLASH_ReadSR(2);		//�ȶ���״̬�Ĵ���2��ԭʼֵ
    if((stareg2&0X02)==0)			//QEλδʹ��
    {
        QSPI_FLASH_Write_Enable();		//дʹ��
        stareg2|=1<<1;				//ʹ��QEλ
        QSPI_FLASH_Write_SR(2,stareg2);	//д״̬�Ĵ���2
    }
    QSPI_Send_CMD(W25X_EnterQPIMode,0,0,QSPI_INSTRUCTION_1_LINE,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);//дcommandָ��,��ַΪ0,������_8λ��ַ_�޵�ַ_���ߴ���ָ��,�޿�����,0���ֽ�����
    QSPI_FLASH_QPI_MODE=1;				//���QSPIģʽ
}

//W25QXX�˳�QSPIģʽ
static void QSPI_FLASH_Qspi_Disable(void)
{
    QSPI_Send_CMD(W25X_ExitQPIMode,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);//дcommandָ��,��ַΪ0,������_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,0���ֽ�����
    QSPI_FLASH_QPI_MODE=0;				//���SPIģʽ
}

//��ȡW25QXX��״̬�Ĵ�����W25QXXһ����3��״̬�Ĵ���
//״̬�Ĵ���1��
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:Ĭ��0,״̬�Ĵ�������λ,���WPʹ��
//TB,BP2,BP1,BP0:FLASH����д��������
//WEL:дʹ������
//BUSY:æ���λ(1,æ;0,����)
//Ĭ��:0x00
//״̬�Ĵ���2��
//BIT7  6   5   4   3   2   1   0
//SUS   CMP LB3 LB2 LB1 (R) QE  SRP1
//״̬�Ĵ���3��
//BIT7      6    5    4   3   2   1   0
//HOLD/RST  DRV1 DRV0 (R) (R) WPS ADP ADS
//regno:״̬�Ĵ����ţ���:1~3
//����ֵ:״̬�Ĵ���ֵ
static uint8_t QSPI_FLASH_ReadSR(uint8_t regno)
{
    uint8_t byte=0,command=0;
    switch(regno)
    {
    case 1:
        command=W25X_ReadStatusReg1;    //��״̬�Ĵ���1ָ��
        break;
    case 2:
        command=W25X_ReadStatusReg2;    //��״̬�Ĵ���2ָ��
        break;
    case 3:
        command=W25X_ReadStatusReg3;    //��״̬�Ĵ���3ָ��
        break;
    default:
        command=W25X_ReadStatusReg1;
        break;
    }
    if(QSPI_FLASH_QPI_MODE)
			QSPI_Send_CMD(command,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_4_LINES);	//QPI,дcommandָ��,��ַΪ0,4�ߴ�����_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,1���ֽ�����
    else 
			QSPI_Send_CMD(command,0,0,QSPI_INSTRUCTION_1_LINE,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_1_LINE);				//SPI,дcommandָ��,��ַΪ0,���ߴ�����_8λ��ַ_�޵�ַ_���ߴ���ָ��,�޿�����,1���ֽ�����
    QSPI_Receive(&byte,1);
    return byte;
}

//дW25QXX״̬�Ĵ���
static void QSPI_FLASH_Write_SR(uint8_t regno,uint8_t sr)
{
    uint8_t command=0;
    switch(regno)
    {
    case 1:
        command=W25X_WriteStatusReg1;    //д״̬�Ĵ���1ָ��
        break;
    case 2:
        command=W25X_WriteStatusReg2;    //д״̬�Ĵ���2ָ��
        break;
    case 3:
        command=W25X_WriteStatusReg3;    //д״̬�Ĵ���3ָ��
        break;
    default:
        command=W25X_WriteStatusReg1;
        break;
    }
    if(QSPI_FLASH_QPI_MODE)
			QSPI_Send_CMD(command,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_4_LINES);	//QPI,дcommandָ��,��ַΪ0,4�ߴ�����_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,1���ֽ�����
    else 
			QSPI_Send_CMD(command,0,0, QSPI_INSTRUCTION_1_LINE,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_1_LINE);				//SPI,дcommandָ��,��ַΪ0,���ߴ�����_8λ��ַ_�޵�ַ_���ߴ���ָ��,�޿�����,1���ֽ�����
    QSPI_Transmit(&sr,1);
}

//W25QXXдʹ��
//��S1�Ĵ�����WEL��λ
static void QSPI_FLASH_Write_Enable(void)
{
    if(QSPI_FLASH_QPI_MODE)
			QSPI_Send_CMD(W25X_WriteEnable,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);	//QPI,дʹ��ָ��,��ַΪ0,������_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,0���ֽ�����
    else 
			QSPI_Send_CMD(W25X_WriteEnable,0,0,QSPI_INSTRUCTION_1_LINE,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);				//SPI,дʹ��ָ��,��ַΪ0,������_8λ��ַ_�޵�ַ_���ߴ���ָ��,�޿�����,0���ֽ�����
}

//W25QXXд��ֹ
//��WEL����
static void QSPI_FLASH_Write_Disable(void)
{
    if(QSPI_FLASH_QPI_MODE)
			QSPI_Send_CMD(W25X_WriteDisable,0,0,QSPI_INSTRUCTION_4_LINES,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);//QPI,д��ָֹ��,��ַΪ0,������_8λ��ַ_�޵�ַ_4�ߴ���ָ��,�޿�����,0���ֽ�����
    else 
			QSPI_Send_CMD(W25X_WriteDisable,0,0,QSPI_INSTRUCTION_1_LINE,QSPI_ADDRESS_NONE,QSPI_ADDRESS_8_BITS,QSPI_DATA_NONE);				//SPI,д��ָֹ��,��ַΪ0,������_8λ��ַ_�޵�ַ_���ߴ���ָ��,�޿�����,0���ֽ�����
}

//�ȴ�����
static void QSPI_FLASH_Wait_Busy(void)
{
    while((QSPI_FLASH_ReadSR(1)&0x01)==0x01);   // �ȴ�BUSYλ���
}



//QSPI��������
//instruction:Ҫ���͵�ָ��
//address:���͵���Ŀ�ĵ�ַ
//dummyCycles:��ָ��������
//	instructionMode:ָ��ģʽ;QSPI_INSTRUCTION_NONE,QSPI_INSTRUCTION_1_LINE,QSPI_INSTRUCTION_2_LINE,QSPI_INSTRUCTION_4_LINE
//	addressMode:��ַģʽ; QSPI_ADDRESS_NONE,QSPI_ADDRESS_1_LINE,QSPI_ADDRESS_2_LINE,QSPI_ADDRESS_4_LINE
//	addressSize:��ַ����;QSPI_ADDRESS_8_BITS,QSPI_ADDRESS_16_BITS,QSPI_USE_ADRESS_BITS,QSPI_USE_ADRESS_BITS
//	dataMode:����ģʽ; QSPI_DATA_NONE,QSPI_DATA_1_LINE,QSPI_DATA_2_LINE,QSPI_DATA_4_LINE

static void QSPI_Send_CMD(uint32_t instruction,uint32_t address,uint32_t dummyCycles,uint32_t instructionMode,uint32_t addressMode,uint32_t addressSize,uint32_t dataMode)
{
    QSPI_CommandTypeDef Cmdhandler;

    Cmdhandler.Instruction=instruction;                 	//ָ��
    Cmdhandler.Address=address;                            	//��ַ
    Cmdhandler.DummyCycles=dummyCycles;                     //���ÿ�ָ��������
    Cmdhandler.InstructionMode=instructionMode;				//ָ��ģʽ
    Cmdhandler.AddressMode=addressMode;   					//��ַģʽ
    Cmdhandler.AddressSize=addressSize;   					//��ַ����
    Cmdhandler.DataMode=dataMode;             				//����ģʽ
    Cmdhandler.SIOOMode=QSPI_SIOO_INST_EVERY_CMD;       	//ÿ�ζ�����ָ��
    Cmdhandler.AlternateByteMode=QSPI_ALTERNATE_BYTES_NONE; //�޽����ֽ�
    Cmdhandler.DdrMode=QSPI_DDR_MODE_DISABLE;           	//�ر�DDRģʽ
    Cmdhandler.DdrHoldHalfCycle=QSPI_DDR_HHC_ANALOG_DELAY;

    HAL_QSPI_Command(&hqspi,&Cmdhandler,5000);
}

//QSPI����ָ�����ȵ�����
//buf:�������ݻ������׵�ַ
//datalen:Ҫ��������ݳ���
//����ֵ:0,����
//    ����,�������
static uint8_t QSPI_Receive(uint8_t* buf,uint32_t datalen)
{
    hqspi.Instance->DLR=datalen-1;                           //�������ݳ���
    if(HAL_QSPI_Receive(&hqspi,buf,5000)==HAL_OK) return 0;  //��������
    else return 1;
}

//QSPI����ָ�����ȵ�����
//buf:�������ݻ������׵�ַ
//datalen:Ҫ��������ݳ���
//����ֵ:0,����
//    ����,�������
static uint8_t QSPI_Transmit(uint8_t* buf,uint32_t datalen)
{
    hqspi.Instance->DLR=datalen-1;                            //�������ݳ���
    if(HAL_QSPI_Transmit(&hqspi,buf,5000)==HAL_OK) return 0;  //��������
    else return 1;
}

