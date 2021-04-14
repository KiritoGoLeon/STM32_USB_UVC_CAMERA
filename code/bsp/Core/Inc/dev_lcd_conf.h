#ifndef __DEV_LCD_CONF_H
#define __DEV_LCD_CONF_H

#include "stdio.h"
#include "stdint.h"

// �Ƿ�ʹ��RGBҺ��
#define USE_RGB_LCD   1
// RGB SDRAM�����ַ
#define RGB_LCD_BASE_ADDR  0XC0000000
#define RGB_LCD_PIXELS_BYTE  2
// �Ƿ�ʹ��16bit����
#define USE_8080_16BIT  0

#if USE_8080_16BIT
// 16 bit  
//LCD��ַ�ṹ��
typedef struct
{
	volatile uint16_t REG;
	volatile uint16_t RAM;
}FSMC_LCD_TypeDef;
#define FSMC_LCD_BASE_ADDR        ((uint32_t)(0x60000000 | ((1<<20) -2)))
// A116��Ϊ�������������� 
#else
// 8bit
typedef struct
{
	volatile uint8_t REG;
	volatile uint8_t RAM;
} FSMC_LCD_TypeDef;

#define FSMC_LCD_BASE_ADDR        ((uint32_t)(0x60000000 | ((1<<20) -2)/2))
#endif

#define FSMC_LCD             ((FSMC_LCD_TypeDef *)FSMC_LCD_BASE_ADDR)

// LCD ID
#define LCD_ID_ILI9481  0x9481

// LCD ID
#define LCD_ID_S6D04D1  0x6041


// �������ˢ�·����Ӧ��, �������Ӳ������ ��������ݶԲ�������
#define LCD_USE_HARD_DIR 0 // ���ʹ��Ӳ������, ��ô����Ҫ����任  // �����ʹ��, Ĭ���Ǻ���

typedef enum{
	LCD_DIR_HORIZONTAL =0,
	LCD_DIR_VERTICAL
}LCD_DIR_ENUM;

#define LCD_DIR LCD_DIR_HORIZONTAL // 0 ����  1����
//#define LCD_DIR LCD_DIR_VERTICAL // 0 ����  1����

//������ɫ
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	   0x001F  
#define BRED             0XF81F
#define GRED 			       0XFFE0
#define GBLUE			       0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			     0XBC40 //��ɫ
#define BRRED 			     0XFC07 //�غ�ɫ
#define GRAY  			     0X8430 //��ɫ

#define FONT_USE_BACK_COLOR  0   // �����Ƿ�ʹ�ñ���ɫ
#define FONT_BACK_COLOR     WHITE //���屳��ɫ 

//GUI��ɫ
typedef enum
{
	LCD_TYPE_8080=0,
	LCD_TYPE_RGB
}LCD_TYPE;


// 8080Һ������
typedef struct
{
	uint16_t w_ram_cmd;		//��ʼдgramָ��
	uint16_t set_x_cmd;		//����x����ָ��
	uint16_t set_y_cmd;		//����y����ָ�� 
	
	void (*set_set_windows)(uint16_t start_x,uint16_t start_y,uint16_t end_x,uint16_t end_y);
	void (*set_set_cursor)(uint16_t x,uint16_t y);
	void (*write_gram_pre)(void);
	void (*write_reg)(uint16_t value);
	void (*write_data)(uint16_t value);
	void (*write_color)(uint16_t color_value);
	uint16_t (*read_data)(void);
}LCD_8080_Typedef;


static void LTDC_Draw_Point(uint16_t x,uint16_t y,uint32_t color);
static uint32_t LTDC_Read_Point(uint16_t x,uint16_t y);
static void LTDC_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint32_t color);
static void LTDC_Color_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);


typedef struct LCD_RGB_Config_t
{
	uint16_t id;
	uint16_t pix_width;
	uint16_t pix_height;
	uint8_t hsw;
	uint8_t hfp;
	uint8_t hbp;
	uint8_t vsw;
	uint8_t vbp;
	uint8_t vfp;
}LCD_RGB_Config;


// RGBҺ������
typedef struct
{
	void (*draw_point)(uint16_t x,uint16_t y,uint32_t color);
	uint32_t (*read_point)(uint16_t x,uint16_t y);
	void (*fill)(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint32_t color);
	void (*color_fill)(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);
}LCD_RGB_Typedef;


typedef struct  
{		 	 
	uint16_t width;		//LCD ���
	uint16_t height;	//LCD �߶�
	
	uint16_t now_width;		//LCD ���
	uint16_t now_height;	//LCD �߶�
	
	uint16_t id;			//LCD ID
	uint8_t  dir;			//���������������ƣ�0��������1��������	
	LCD_TYPE  lcd_type;
	LCD_8080_Typedef lcd_8080;
	LCD_RGB_Typedef lcd_rgb;
}LCDTypedef; 

#endif
