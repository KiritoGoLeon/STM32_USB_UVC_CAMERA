#include "dev_lcd.h"
#include "font.h"

#include "driver_LCD_ILI9481.h"
#include "driver_LCD_S6D04D1.h"
#include "driver_LCD_RGB.h"

LCDTypedef g_lcd;

// ��ȡlcd id
uint16_t fsmc_lcd_read_id()
{
    uint16_t  id = 0;
    FSMC_LCD->REG = 0xB9;
    FSMC_LCD->RAM = 0XFF;
    FSMC_LCD->RAM= 0X83;
    FSMC_LCD->RAM = 0X69;

    FSMC_LCD->REG = 0XBF;
    id=FSMC_LCD->RAM;	//1dummy read
    id=FSMC_LCD->RAM;	//2����0X02
    id=FSMC_LCD->RAM;   	//3��ȡ04
    id=FSMC_LCD->RAM;   	//4��ȡ94
    id<<=8;
    id|=FSMC_LCD->RAM;   //5��ȡ81
    return id;
}




// ��ʼ��
void dev_lcd_init(void)
{
    g_lcd.dir = LCD_DIR;
    // ����
    dev_lcd_power_on();
#if USE_RGB_LCD
    driver_LCD_RGB_init(&g_lcd);
#else
    if(fsmc_lcd_read_id() == LCD_ID_ILI9481) {
        driver_LCD_ILI9481_init(&g_lcd);
    } else {
        driver_LCD_S6D04D1_init(&g_lcd);
    }
#endif

    // ����  ����
    if(g_lcd.dir == LCD_DIR_VERTICAL) {
        g_lcd.now_height = g_lcd.width;
        g_lcd.now_width = g_lcd.height;
    } else {
        g_lcd.now_height = g_lcd.height;
        g_lcd.now_width = g_lcd.width;
    }
    touch_init(g_lcd.width,g_lcd.height,LCD_DIR,g_lcd.id);
    dev_lcd_clear(BLACK);
}
// ����
void dev_lcd_power_on(void)
{
    HAL_GPIO_WritePin(LCD_BK_GPIO_Port,LCD_BK_Pin,GPIO_PIN_SET);
}

// �ص�
void dev_lcd_power_off(void)
{
    HAL_GPIO_WritePin(LCD_BK_GPIO_Port,LCD_BK_Pin,GPIO_PIN_RESET);
}

// ����
void dev_lcd_clear(uint16_t rgb_value)
{
#if USE_RGB_LCD
    g_lcd.lcd_rgb.fill(0,0,g_lcd.now_width-1,g_lcd.now_height-1,rgb_value);
#else
    uint32_t all_pix = g_lcd.now_width * g_lcd.now_height;
    uint32_t i = 0;
    // ע�����õĴ�СΪw-1 h-1
    g_lcd.lcd_8080.set_set_windows(0,0,g_lcd.now_width-1,g_lcd.now_height-1);
    for(i = 0; i<all_pix; i++)
    {
        g_lcd.lcd_8080.write_color(rgb_value);
    }
#endif
}




/*******************************************************************************
//���ٻ���  modify YZ  ע�����õ�x,y��С, ��Χ��0-pix-1
//x,y:����
//color:��ɫ
*******************************************************************************/
void dev_lcd_draw_point(uint16_t x,uint16_t y,uint16_t color)
{
#if USE_RGB_LCD
    g_lcd.lcd_rgb.draw_point(x,y,color);
#else
    g_lcd.lcd_8080.set_set_cursor(x,y);
    g_lcd.lcd_8080.write_color(color);
#endif

}

/*******************************************************************************
//��ָ��λ����ʾһ���ַ�   YZ
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//size:�����С 12/16/24
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
//���ͣ�
//1���ַ�����(size/2)���߶�(size)һ�롣
//2���ַ�ȡģΪ����ȡģ����ÿ��ռ�����ֽڣ���󲻹������ֽ�����ռһ�ֽڡ�
//3���ַ���ռ�ռ�Ϊ��ÿ����ռ�ֽڣ�������
//csize=(size/8+((size%8)?1:0))*(size/2)
//�˺�*ǰΪ����ÿ����ռ�ֽ������˺�*��Ϊ�������ַ��߶�һ�룩
*******************************************************************************/
void dev_lcd_show_char(uint16_t x,uint16_t y,uint16_t num,uint8_t size,uint8_t mode,uint16_t  font_color, uint16_t  back_color)
{
    uint8_t temp,t1,t;
    uint16_t  y0=y;
    uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//�õ�����һ���ַ���Ӧ������ռ���ֽ���
    num=num-' ';//�õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩
    for(t=0; t<csize; t++)
    {
        if(size==12)temp=asc2_1206[num][t]; 	 	//����1206����
        else if(size==16)temp=asc2_1608[num][t];	//����1608����
        else if(size==24)temp=asc2_2412[num][t];	//����2412����
        else return;								//û�е��ֿ�
        for(t1=0; t1<8; t1++)
        {
            if(temp&0x80) {
                dev_lcd_draw_point(x,y,font_color);
            } else {
                if(mode) {
                    dev_lcd_draw_point(x,y,back_color);
                }

            }
            temp<<=1;
            y++;
            if(y>=g_lcd.now_height)return;		//��������
            if((y-y0)==size)
            {
                y=y0;
                x++;
                if(x>=g_lcd.now_width)return;	//��������
                break;
            }
        }
    }
}

void dev_lcd_show_string(uint16_t x,uint16_t y,uint8_t size,uint8_t *p,uint16_t color)
{
    uint8_t x0=x;
    uint16_t width = strlen((char*)p)*size;
    uint16_t height = size;

    width+=x;
    height+=y;
    while((*p<='~')&&(*p>=' '))//�ж��ǲ��ǷǷ��ַ�!
    {
        if(x>=width) {
            x=x0;
            y+=size;
        }
        if(y>=height)break;//�˳�
        dev_lcd_show_char(x,y,*p,size,FONT_USE_BACK_COLOR,color,FONT_BACK_COLOR);
        x+=size/2;
        p++;
    }
}

//����
//x1,y1:�������
//x2,y2:�յ�����
void dev_lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color)
{
    uint16_t t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;
    delta_x=x2-x1; //������������
    delta_y=y2-y1;
    uRow=x1;
    uCol=y1;
    if(delta_x>0)incx=1; //���õ�������
    else if(delta_x==0)incx=0;//��ֱ��
    else {
        incx=-1;
        delta_x=-delta_x;
    }
    if(delta_y>0)incy=1;
    else if(delta_y==0)incy=0;//ˮƽ��
    else {
        incy=-1;
        delta_y=-delta_y;
    }
    if( delta_x>delta_y)distance=delta_x; //ѡȡ��������������
    else distance=delta_y;
    for(t=0; t<=distance+1; t++ ) //�������
    {
        dev_lcd_draw_point(uRow,uCol,color);//����
        xerr+=delta_x ;
        yerr+=delta_y ;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}

// ������
void dev_lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color)
{
    dev_lcd_draw_line(x1,y1,x2,y1,color);
    dev_lcd_draw_line(x1,y1,x1,y2,color);
    dev_lcd_draw_line(x1,y2,x2,y2,color);
    dev_lcd_draw_line(x2,y1,x2,y2,color);
}

//��ָ��λ�û�һ��ָ����С��Բ
//(x,y):���ĵ�
//r    :�뾶
void dev_lcd_draw_circle(uint16_t x0,uint16_t y0,uint8_t r,uint16_t color)
{
    int a,b;
    int di;
    a=0;
    b=r;
    di=3-(r<<1);             //�ж��¸���λ�õı�־
    while(a<=b)
    {
        dev_lcd_draw_point(x0+a,y0-b,color);             //5
        dev_lcd_draw_point(x0+b,y0-a,color);             //0
        dev_lcd_draw_point(x0+b,y0+a,color);             //4
        dev_lcd_draw_point(x0+a,y0+b,color);             //6
        dev_lcd_draw_point(x0-a,y0+b,color);             //1
        dev_lcd_draw_point(x0-b,y0+a,color);
        dev_lcd_draw_point(x0-a,y0-b,color);             //2
        dev_lcd_draw_point(x0-b,y0-a,color);             //7
        a++;
        //ʹ��Bresenham�㷨��Բ
        if(di<0)di +=4*a+6;
        else
        {
            di+=10+4*(a-b);
            b--;
        }
    }
}


void dev_lcd_fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
#if USE_RGB_LCD
    g_lcd.lcd_rgb.fill(sx,sy,ex,ey,color);
#else
    uint16_t i,j;
    int xlen= ex-sx+1;
    for(i=sy; i<=ey; i++)
    {
        for(j=0; j<xlen; j++)
        {
            g_lcd.lcd_8080.set_set_cursor(sx+j,i);
            g_lcd.lcd_8080.write_color(color);
        }
    }
#endif

}

void dev_lcd_fast_fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
#if USE_RGB_LCD
    g_lcd.lcd_rgb.fill(sx,sy,ex,ey,color);
#else
    int xlen= ex-sx+1;
    int ylen = ey - sy +1;

    uint32_t all_pix = xlen * ylen;
    uint32_t i = 0;

    g_lcd.lcd_8080.set_set_windows(sx,sy,ex,ey);
    for(i = 0; i<all_pix; i++)
    {
        g_lcd.lcd_8080.write_color(color);
    }
#endif


}

//��ָ�����������ָ����ɫ��
//(sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex-sx+1)*(ey-sy+1)
//color:Ҫ������ɫ
void dev_lcd_color_fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{
#if USE_RGB_LCD
    g_lcd.lcd_rgb.color_fill(sx,sy,ex,ey,color);
#else
    uint16_t height,width;

    width=ex-sx+1; 			//�õ����Ŀ���
    height=ey-sy+1;			//�߶�
    uint32_t total=width*height,i=0;
    g_lcd.lcd_8080.set_set_windows(sx,sy,ex,ey);
    while(i<=total)
    {
        g_lcd.lcd_8080.write_color(color[i++]);//д������
    }
#endif
}


