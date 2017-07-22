///////////////////////////////////////////////////////////////////////////////
//
// SSD1306 128*64 OLED Display    -=  toxcat :3  22-Jul-2017  =-
//
///////////////////////////////////////////////////////////////////////////////
//
// Example:
//
// int main(void)
//   {
//   char str[32];
//
//   oled_init();
//   oled_clear();
//   oled_print(0,0,"SSD1306 128x64 TEST");
//   oled_line(0,10,127,10);
//   oled_update();
//
//   for(;;)
//     {
//     sprintf(str,"%10lu",HAL_GetTick());
//     oled_print(0,2,str);
//     oled_update();
//     }
//   }
//
///////////////////////////////////////////////////////////////////////////////



#include "stm32f1xx_hal.h"
#include "stdlib.h"


#include "font.h"


// Software SPI

#define SCK_H  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET))
#define SCK_L  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET))

#define DAT_H  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET))
#define DAT_L  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET))

#define RST_H  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET))
#define RST_L  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET))

#define DC_H   (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET))
#define DC_L   (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET))

#define CS_H   (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET))
#define CS_L   (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET))


//delay for reset

#define OLED_DELAY  HAL_Delay(1)


//screen buffer

#define BUFF_SIZE  1024  //columns*strings(pages)=128*8=1024

uint8_t scrBuff[BUFF_SIZE];


#define BIT_SET(reg, bit)  (reg |= (1<<bit))


//commands for the initialization of SSD1306

const uint8_t init[] =
{
0xae, //display off sleep mode
0xd5, //display clock divide
0x80, //
0xa8, //set multiplex ratio
0x3f, //
0xd3, //display offset
0x00, //
0x40, //set display start line
0x8d, //charge pump setting
0x14, //
0x20, //memory addressing mode
0x00, //horizontal addressing mode
0xa1, //segment re-map
0xc8, //COM output scan direction
0xda, //COM pins hardware configuration
0x12, //
0x81, //set contrast (brightness)
0x8f, //0x8f, 0xcf  //0..255
0xd9, //pre-charge period
0xf1, //
0xdb, //VCOMH deselect level
0x40, //
0xa4, //entire display off
0xa6, //normal display, 0xa7 inverse display
0xaf  //display turned on
};


void oled_init(void); //init display
void oled_write(uint8_t mode, uint8_t data);  //write byte  //mode: 1-data, 0-command

//operations with the screen buffer

void oled_clear(void); //clear screen
void oled_update(void); //write buffer to screen

void oled_char(uint8_t x, uint8_t y, uint8_t sign);  //print a character  //x: 0..120 //y: 0..7
void oled_print(uint8_t x, uint8_t y, char *str);    //print a string  //x: 0..120 //y: 0..7

void oled_pixel(uint8_t h, uint8_t y);  //draw pixel  //x: 0..127  //y: 0..63
void oled_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2); //draw line //x: 0..127  //y: 0..63



//-----------------------------------------------------------------------------
void oled_write(uint8_t mode, uint8_t data) //mode: 1-data, 0-command
    {
    (mode) ? DC_H : DC_L;  //data/command

    CS_L; //chip select in the active state

    for(uint8_t k=0x80; k; k>>=1) //send byte
        {
        (data & k) ? DAT_H : DAT_L;
        SCK_H; //on rising edge of SCLK
        SCK_L;
        }

    CS_H;
    }


//-----------------------------------------------------------------------------
void oled_init(void) //init display
    {
    SCK_L; //sck line low
    CS_H;  //chip select inactive

    RST_L; //display chip reset
    OLED_DELAY;
    RST_H;

    for(uint8_t i=0; i<sizeof init; i++) oled_write(0, init[i]); //init
    }


//-----------------------------------------------------------------------------
void oled_clear(void) //clear buffer
    {
    for(uint16_t x=0; x<BUFF_SIZE; x++) scrBuff[x]=0;
    }


//-----------------------------------------------------------------------------
void oled_update(void) //write buffer to screen
    {
    oled_write(0,0x21); //set column address
    oled_write(0,0); //start address
    oled_write(0,127); //end address

    oled_write(0,0x22); //set page address
    oled_write(0,0);
    oled_write(0,7);

    for(uint16_t x=0; x<BUFF_SIZE; x++) oled_write(1,scrBuff[x]);  //write
    }


//-----------------------------------------------------------------------------
void oled_char(uint8_t x, uint8_t y, uint8_t sign)  //x: 0..120 //y: 0..7
    {
    if(x<=120 && y<8)
        {
        if(sign<32 || sign>127) sign=128; //filling for invalid characters
        for(uint8_t k=0; k<5; k++) scrBuff[128*y+x+k] = font[5*(sign-32)+k];
        scrBuff[128*y+x+5]=0x00; //space between chars
        }
    }


//-----------------------------------------------------------------------------
void oled_print(uint8_t x, uint8_t y, char *str)  //0..120 //0..7
    {
    while(*str) //write
        {
        oled_char(x, y, *str++);
        x+=6;
        }
    }


//-----------------------------------------------------------------------------
void oled_pixel(uint8_t x, uint8_t y) //x: 0..127  //y: 0..63
    {
    if(x<=127 && y<=63) BIT_SET(scrBuff[x+128*(y/8)], y%8);
    }


//-----------------------------------------------------------------------------
void oled_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) //x: 0..127  //y: 0..63
    {
    int16_t P;

    int8_t x = x1;
    int8_t y = y1;

    int8_t dx = abs((int8_t)(x2-x1));
    int8_t dy = abs((int8_t)(y2-y1));

    int8_t addx, addy;

    if(x1>x2) addx=-1;
    else addx=1;

    if(y1>y2) addy=-1;
    else addy=1;

    if(dx>=dy)
        {
        P=2*dy-dx;

        for(uint8_t i=0; i<=dx; ++i)
            {
            oled_pixel(x, y);

            if(P<0)
                {
                P+=2*dy;
                x+=addx;
                }
            else
                {
                P+=2*dy-2*dx;
                x+=addx;
                y+=addy;
                }
            }
        }
    else
        {
        P=2*dx-dy;

        for(uint8_t i=0; i<=dy; ++i)
            {
            oled_pixel(x, y);

            if(P<0)
                {
                P+=2*dx;
                y+=addy;
                }
            else
                {
                P+=2*dx-2*dy;
                x+=addx;
                y+=addy;
                }
            }
        }
    }

