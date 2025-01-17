#include "driver_LCD_ILI9481.h"

#include "delay.h"


// M0 M1 M2 : 0 1 0  -- 16bit
// M0 M1 M2 : 1 1 0  -- 8bit
// M0 M1 M2 : 1 0 1  -- 3 -line spi
// M0 M1 M2 : 1 1 1  -- 4 -line spi

/***  ��������
//------>  Y   480
|
|
|
|
X

320

***/

#define LCD_PIX_WIDTH  480
#define LCD_PIX_HEIGHT 320


#define LCD_SET_X_CMD  0X2A
#define LCD_SET_Y_CMD  0X2B
#define LCD_WRITE_GRAM_CMD  0X2C


static LCDTypedef *g_mlcd;
static void lcd_config(LCDTypedef *lcd);
static void lcd_init(void);

//----------- start---------
// ��ʼ��
void driver_LCD_ILI9481_init(LCDTypedef *lcd)
{
    // ���ó�ʼ��
    lcd_config(lcd);

    g_mlcd = lcd;
    // ��ʼ��Һ����
    lcd_init();

    lcd->id = LCD_ID_ILI9481;
}

// д����
static  void lcd_write_reg(uint16_t value)
{
    FSMC_LCD->REG =  value;
}

// д����
static  void lcd_write_data(uint16_t value)
{
    FSMC_LCD->RAM = value;
}

// д��ɫ
static  void lcd_write_color(uint16_t value)
{
#if USE_8080_16BIT
    FSMC_LCD->RAM = value;
#else
    FSMC_LCD->RAM = value>>8;
    FSMC_LCD->RAM = value&0xff;
#endif
}
// ��ȡ����
static  uint16_t lcd_read_data(void)
{
    volatile uint16_t value = FSMC_LCD->RAM;
    return value;
}

static  void lcd_write_gram_pre(void)
{
    FSMC_LCD->REG = LCD_WRITE_GRAM_CMD;
}

// д����
static  void lcd_set_windows(uint16_t start_x,uint16_t start_y,uint16_t end_x,uint16_t end_y)
{
    uint16_t msx = 0;
    uint16_t msy = 0;

    uint16_t mex = 0;
    uint16_t mey = 0;

#if LCD_USE_HARD_DIR
    msx =start_x;
    msy =start_y;
    mex =end_x;
    mey =end_y;
#else
    // ����
    if(g_mlcd->dir== LCD_DIR_VERTICAL) {
        // ԭ���ֱ���
        msx = start_y;
        msy = LCD_PIX_HEIGHT - end_x -1;
        mex = end_y,
        mey = LCD_PIX_HEIGHT - start_x -1;
    } else { // ����
        msx =start_x;
        msy =start_y;
        mex =end_x;
        mey =end_y;
    }
#endif
    //printf("x:%d  y:%d   ex:%d  ey:%d\r\n",msx,msy,mex,mey);

    lcd_write_reg(LCD_SET_X_CMD);
    lcd_write_data(msx>>8);
    lcd_write_data(msx&0xff);
    lcd_write_data(mex>>8);
    lcd_write_data(mex&0xff);

    lcd_write_reg(LCD_SET_Y_CMD);
    lcd_write_data(msy>>8);
    lcd_write_data(msy&0xff);
    lcd_write_data(mey>>8);
    lcd_write_data(mey&0xff);

    lcd_write_reg(LCD_WRITE_GRAM_CMD);
}

// д����
static  void lcd_set_cursor(uint16_t x,uint16_t y)
{
    uint16_t mx = 0;
    uint16_t my = 0;

#if LCD_USE_HARD_DIR
    mx = x;
    my = y;
#else
    // ����
    if(g_mlcd->dir== LCD_DIR_VERTICAL) {
        mx = y;
        my = LCD_PIX_HEIGHT - x -1;
    } else { // ����
        mx = x;
        my = y;
    }

#endif
    lcd_write_reg(LCD_SET_X_CMD);
    lcd_write_data(mx>>8);
    lcd_write_data(mx&0xff);
    lcd_write_data(mx>>8);
    lcd_write_data(mx&0xff);

    lcd_write_reg(LCD_SET_Y_CMD);
    lcd_write_data(my>>8);
    lcd_write_data(my&0xff);
    lcd_write_data(my>>8);
    lcd_write_data(my&0xff);

    lcd_write_reg(LCD_WRITE_GRAM_CMD);
}




// ----------- end -----------

// ���ó�ʼ��
static void lcd_config(LCDTypedef *lcd)
{
    lcd->lcd_type = LCD_TYPE_8080;
    lcd->width = LCD_PIX_WIDTH;
    lcd->height = LCD_PIX_HEIGHT;

    lcd->lcd_8080.set_x_cmd = LCD_SET_X_CMD;
    lcd->lcd_8080.set_y_cmd = LCD_SET_Y_CMD;
    lcd->lcd_8080.w_ram_cmd = LCD_WRITE_GRAM_CMD;

    lcd->lcd_8080.set_set_windows = lcd_set_windows;
    lcd->lcd_8080.set_set_cursor = lcd_set_cursor;
    lcd->lcd_8080.write_reg = lcd_write_reg;
    lcd->lcd_8080.write_data = lcd_write_data;
    lcd->lcd_8080.read_data = lcd_read_data;
    lcd->lcd_8080.write_color = lcd_write_color;
    lcd->lcd_8080.write_gram_pre = lcd_write_gram_pre;
}


// Һ����ʼ��
static void lcd_init(void)
{
    //TEST  cmi35mva 20180510 ok����
    int i,j;
    lcd_write_reg(0x01); //Soft_rese
    delay_ms(220);

    //Exit_sleep_mode
    lcd_write_reg(0x11);
    delay_ms(280);

    lcd_write_reg(0xd0); //Power_Setting
    lcd_write_data(0x07);//07  VC[2:0] Sets the ratio factor of Vci to generate the reference voltages Vci1
    lcd_write_data(0x44);//41  BT[2:0] Sets the Step up factor and output voltage level from the reference voltages Vci1
    lcd_write_data(0x1E);//1f  17   1C  VRH[3:0]: Sets the factor to generate VREG1OUT from VCILVL
    delay_ms(220);

    lcd_write_reg(0xd1); //VCOM Control
    lcd_write_data(0x00);//00
    lcd_write_data(0x0C);//1A   VCM [6:0] is used to set factor to generate VCOMH voltage from the reference voltage VREG1OUT  15    09
    lcd_write_data(0x1A);//1F   VDV[4:0] is used to set the VCOM alternating amplitude in the range of VREG1OUT x 0.70 to VREG1OUT   1F   18

    lcd_write_reg(0xC5);  //Frame Rate
    lcd_write_data(0x03); // 03   02

    lcd_write_reg(0xd2);  //Power_Setting for Normal Mode
    lcd_write_data(0x01);  //01
    lcd_write_data(0x11);  //11

    lcd_write_reg(0xE4);  //?
    lcd_write_data(0xa0);
    lcd_write_reg(0xf3);
    lcd_write_data(0x00);
    lcd_write_data(0x2a);

    //1  Gamma Setting OK
    lcd_write_reg(0xc8);
    lcd_write_data(0x00);
    lcd_write_data(0x26);
    lcd_write_data(0x21);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x1f);
    lcd_write_data(0x65);
    lcd_write_data(0x23);
    lcd_write_data(0x77);
    lcd_write_data(0x00);
    lcd_write_data(0x0f);
    lcd_write_data(0x00);

    //Panel Driving Setting
    lcd_write_reg(0xC0);
    lcd_write_data(0x00); //1//00  REV  SM  GS
    lcd_write_data(0x3B); //2//NL[5:0]: Sets the number of lines to drive the lcd at an interval of 8 lines.
    lcd_write_data(0x00); //3//SCN[6:0]
    lcd_write_data(0x02); //4//PTV: Sets the Vcom output in non-display area drive period
    lcd_write_data(0x11); //5//NDL: Sets the source output level in non-display area.  PTG: Sets the scan mode in non-display area.

    //Interface Control
    lcd_write_reg(0xc6);
    lcd_write_data(0x83);
    //GAMMA SETTING

    lcd_write_reg(0xf0); //?
    lcd_write_data(0x01);

    lcd_write_reg(0xE4);//?
    lcd_write_data(0xa0);


    //////��װ����   NG
    // Set_address_mode  ---- ˢ�·���

    lcd_write_reg(0x36);
#if LCD_USE_HARD_DIR
    // ����
    if(g_mlcd->dir == LCD_DIR_VERTICAL) {
        lcd_write_data(0x0A);
    } else { // ����
        lcd_write_data(0x28);
    }
#else
    lcd_write_data(0x28);
#endif


    lcd_write_reg(0x3a);
    lcd_write_data(0x55);

    lcd_write_reg(0xb4);//Display Mode and Frame Memory Write Mode Setting
    lcd_write_data(0x02);
    lcd_write_data(0x00); //?
    lcd_write_data(0x00);
    lcd_write_data(0x01);

    delay_ms(280);

    lcd_write_reg(0x2a);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x01);
    lcd_write_data(0x3F); //3F

    lcd_write_reg(0x2b);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x01);
    lcd_write_data(0xDf); //DF

    //lcd_write_reg(0x21);
    lcd_write_reg(0x29);
    lcd_write_reg(0x2c);

}




