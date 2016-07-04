/*
 *
 * File Name	: 'i2c_lcd_lib.c'
 * Title		: i2c PCF8574 LCd interface
 * Author:		: Tamagotchi - Copyright (C) 2013
 * Created		: 2013-04-19
 * Revised		: 2013-04-20
 * Version		: 0.6
 * Target MCU	: Atmel AVR
 *
 */ 

#include <util/delay.h>
#include <avr/pgmspace.h>

//#include "i2cmaster.h" // Использую другую i2c библиотеку
#include "i2c_lcd.h"


#define STROBE_EN() do {lcd_data |= LCD_E; i2c_lcd_pio(lcd_data); lcd_data &= ~LCD_E; i2c_lcd_pio(lcd_data); }while(0)

unsigned char lcd_data;

/*
void i2c_lcd_pio(unsigned char data)
{
	i2c_start(PCF8574 + I2C_WRITE);
	i2c_write(data + LCD_BLIGHT);
	i2c_stop();
}*/
#define i2c_lcd_pio(x) send_PCF8574(1, (x)+ LCD_BLIGHT)

	
void i2c_lcd_cmd(unsigned char cmd)
{	
cli();
	lcd_data = (cmd & 0xF0);
	lcd_data &= ~LCD_RS;
	STROBE_EN();
	i2c_lcd_pio(lcd_data);
	_delay_us(100);
	
	lcd_data = ((cmd & 0x0F)<<4);
	lcd_data &= ~LCD_RS;
	STROBE_EN();
	i2c_lcd_pio(lcd_data);
	
	if(cmd & 0b11111100) _delay_us(100);else _delay_ms(2);
sei();
} 

void i2c_lcd_putch(unsigned char chr)
{
cli();
	lcd_data = (chr & 0xF0);
	lcd_data |= LCD_RS;
	STROBE_EN();
	_delay_us(100);
	
	lcd_data = ((chr & 0x0F)<<4);
	lcd_data |= LCD_RS;
	STROBE_EN();
	i2c_lcd_pio(lcd_data);
	_delay_ms(2);
sei();
}

void i2c_lcd_init()
{
cli();
	i2c_lcd_pio(0b00000000);
	_delay_ms(40);
// 3 x 0x03h, 8bit
	lcd_data = 0b00110000;
	i2c_lcd_pio(lcd_data);
	STROBE_EN();
	_delay_ms(5);							//delay > 4,1ms
	STROBE_EN();
	_delay_us(100);
	STROBE_EN();
	_delay_us(100);
// 4bit - 0x02h
	lcd_data = 0b00100000;
	i2c_lcd_pio(lcd_data);
	STROBE_EN();
	i2c_lcd_pio(lcd_data);
	_delay_us(100);
// From now on in 4-bit-Mode
	i2c_lcd_cmd(LCD_4BIT | LCD_2LINE | LCD_5X7);		//-	2-Lines, 5x7-Matrix
	i2c_lcd_cmd(LCD_DISPLAYOFF);						//-	Display off
	i2c_lcd_cmd(LCD_CLEAR);								//-	Clear Screen
	i2c_lcd_cmd(LCD_INCREASE | LCD_DISPLAYSHIFTOFF);	//-	Entrymode (Display Shift: off, Increment Address Counter)
	i2c_lcd_cmd(LCD_DISPLAYON | LCD_BLINKINGOFF);		//-	Display on
sei();
}

void i2c_lcd_gotoxy(unsigned char x, unsigned char y)
{
	register uint8_t DDRAM_addr;
	// remap lines into proper order
	switch(y)
	{
		case 0: DDRAM_addr = LCD_LINE0_DDRAMADDR + x; break;
		case 1: DDRAM_addr = LCD_LINE1_DDRAMADDR + x; break;
		case 2: DDRAM_addr = LCD_LINE2_DDRAMADDR + x; break;
		case 3: DDRAM_addr = LCD_LINE3_DDRAMADDR + x; break;
		default: DDRAM_addr = LCD_LINE0_DDRAMADDR+x;
	}
	// set data address
	i2c_lcd_cmd(1<<LCD_DDRAM | DDRAM_addr);
}

void i2c_lcd_puts(const char *s)
{
	while(*s) i2c_lcd_putch(*s++);
}

//Copies string from flash memory to LCD
//const uint8_t location[] PROGMEM="AVR.TAMATRONIX.COM\0";
//i2c_lcd_pgmputs(location);	
void i2c_lcd_pgmputs(const uint8_t *progmem_s)
{
	register char c;
	while ((c = pgm_read_byte(progmem_s++)))
	{
		i2c_lcd_putch(c);
	}
}

void i2c_lcd_defch(const uint8_t *pc, uint8_t ch_code)
{
	uint8_t a, cl;
	uint16_t i;
	a = (ch_code<<3)|0x40;
	for (i=0; i<8; i++)
	{
		cl = pgm_read_byte(&pc[i]);
		i2c_lcd_cmd(a++);
		i2c_lcd_putch(cl);
	}
}

void i2c_lcd_clr(void)				//Clears LCD
{
	i2c_lcd_cmd(1<<LCD_CLEAR);
}

void i2c_lcd_home(void)			//LCD cursor home
{
	i2c_lcd_cmd(1<<LCD_HOME);
}
