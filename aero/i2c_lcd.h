/*
 *
 * File Name	: 'i2c_lcd_lib.h'
 * Title		: i2c PCF8574 LCd interface
 * Author:		: Tamagotchi - Copyright (C) 2013
 * Created		: 2013-04-19
 * Revised		: 2013-04-20
 * Version		: 0.6
 * Target MCU	: Atmel AVR
 * 
 * D4_pin -> P4 - LCD-Pin D4 must be connected to P4 on the PCF8574
 * D5_pin -> P5 - LCD-Pin D5 must be connected to P5 on the PCF8574
 * D6_pin -> P6 - LCD-Pin D6 must be connected to P6 on the PCF8574
 * D7_pin -> P7 - LCD-Pin D7 must be connected to P7 on the PCF8574
 * 
 */

#ifndef I2C_LCD_H_
#define I2C_LCD_H_

/* PCF8574 */
#define P7	7
#define P6	6
#define P5	5
#define P4	4
#define P3	3
#define P2	2
#define P1	1
#define P0	0

//#define PCF8574		0x42  //  0100 1110 это i2c adress 0b01000000 и A0 + A1 + A2 Стр. 9 даташита.
//#define PCF8574A	0b01110000				//i2c adress

#define LCD_RS_PIN			P0	/**< LCD-Pin RS is connected to P0 on the PCF8574 */
#define LCD_RW_PIN			P1	/**< LCD-Pin RW is connected to P1 on the PCF8574 */
#define LCD_E_PIN			P2	/**< LCD-Pin E is connected to P2 on the PCF8574 */
#define LCD_BLIGHT_PIN		P3	/**< LCD-BACKLIGHT pin is connected to P3 on the PCF8574 */

#define LCD_RS				(1 << LCD_RS_PIN)
#define LCD_RW				(1 << LCD_RW_PIN)
#define LCD_BLIGHT_ON			(1 << LCD_BLIGHT_PIN)
volatile char LCD_BLIGHT;
char e_LCD_BLIGHT EEMEM;
#define LCD_E				(1 << LCD_E_PIN)

#define LCD_CLEAR			0	//clear display
#define LCD_HOME			1	//return to home position
#define LCD_CGRAM			6	//set CG RAM address
#define LCD_DDRAM			7	//set DD RAM address

#define LCD_ENTRYMODE			0x04			/**< Set entrymode */
	#define LCD_INCREASE		LCD_ENTRYMODE | 0x02	/**<	Set cursor move direction -- Increase */
	#define LCD_DECREASE		LCD_ENTRYMODE | 0x00	/**<	Set cursor move direction -- Decrease */
	#define LCD_DISPLAYSHIFTON	LCD_ENTRYMODE | 0x01	/**<	Display is shifted */
	#define LCD_DISPLAYSHIFTOFF	LCD_ENTRYMODE | 0x00	/**<	Display is not shifted */

#define LCD_DISPLAYMODE			0x08			/**< Set displaymode */
	#define LCD_DISPLAYON		LCD_DISPLAYMODE | 0x04	/**<	Display on */
	#define LCD_DISPLAYOFF		LCD_DISPLAYMODE | 0x00	/**<	Display off */
	#define LCD_CURSORON		LCD_DISPLAYMODE | 0x02	/**<	Cursor on */
	#define LCD_CURSOROFF		LCD_DISPLAYMODE | 0x00	/**<	Cursor off */
	#define LCD_BLINKINGON		LCD_DISPLAYMODE | 0x01	/**<	Blinking on */
	#define LCD_BLINKINGOFF		LCD_DISPLAYMODE | 0x00	/**<	Blinking off */

#define LCD_SHIFTMODE			0x10			/**< Set shiftmode */
	#define LCD_DISPLAYSHIFT	LCD_SHIFTMODE | 0x08	/**<	Display shift */
	#define LCD_CURSORMOVE		LCD_SHIFTMODE | 0x00	/**<	Cursor move */
	#define LCD_RIGHT			LCD_SHIFTMODE | 0x04	/**<	Right shift */
	#define LCD_LEFT			LCD_SHIFTMODE | 0x00	/**<	Left shift */

#define LCD_DISPLAYMODE			0x08			/**< Set displaymode */
	#define LCD_DISPLAYON		LCD_DISPLAYMODE | 0x04	/**<	Display on */
	#define LCD_DISPLAYOFF		LCD_DISPLAYMODE | 0x00	/**<	Display off */
	#define LCD_CURSORON		LCD_DISPLAYMODE | 0x02	/**<	Cursor on */
	#define LCD_CURSOROFF		LCD_DISPLAYMODE | 0x00	/**<	Cursor off */
	#define LCD_BLINKINGON		LCD_DISPLAYMODE | 0x01	/**<	Blinking on */
	#define LCD_BLINKINGOFF		LCD_DISPLAYMODE | 0x00	/**<	Blinking off */

#define LCD_SHIFTMODE			0x10			/**< Set shiftmode */
	#define LCD_DISPLAYSHIFT	LCD_SHIFTMODE | 0x08	/**<	Display shift */
	#define LCD_CURSORMOVE		LCD_SHIFTMODE | 0x00	/**<	Cursor move */
	#define LCD_RIGHT			LCD_SHIFTMODE | 0x04	/**<	Right shift */
	#define LCD_LEFT			LCD_SHIFTMODE | 0x00	/**<	Left shift */

#define LCD_CONFIGURATION		0x20				/**< Set function */
	#define LCD_8BIT		LCD_CONFIGURATION | 0x10	/**<	8 bits interface */
	#define LCD_4BIT		LCD_CONFIGURATION | 0x00	/**<	4 bits interface */
	#define LCD_2LINE		LCD_CONFIGURATION | 0x08	/**<	2 line display */
	#define LCD_1LINE		LCD_CONFIGURATION | 0x00	/**<	1 line display */
	#define LCD_5X10		LCD_CONFIGURATION | 0x04	/**<	5 X 10 dots */
	#define LCD_5X7			LCD_CONFIGURATION | 0x00	/**<	5 X 7 dots */

// cursor position to DDRAM mapping
#define LCD_LINE0_DDRAMADDR		0x00
#define LCD_LINE1_DDRAMADDR		0x40
#define LCD_LINE2_DDRAMADDR		0x14
#define LCD_LINE3_DDRAMADDR		0x54

void i2c_lcd_init();
void i2c_lcd_putch(unsigned char chr);
void i2c_lcd_cmd(unsigned char cmd);
void i2c_lcd_puts(const char *s);
void i2c_lcd_gotoxy(unsigned char radek, unsigned char sloupec);
//void i2c_lcd_pio(unsigned char data);
void i2c_lcd_clr(void);
void i2c_lcd_home(void);
void i2c_lcd_pgmputs(const uint8_t *progmem_s);
void i2c_lcd_defch(const uint8_t *pc, uint8_t ch_code);

#include "i2c_lcd.c"

#endif /* I2C_LCD_H_ */