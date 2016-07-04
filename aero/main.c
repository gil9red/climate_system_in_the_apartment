#define I2C_SPEED 2
#define CIKLE 5 //������ �� ���� �����.
#define CO2_divider 4 // ������ CO2 ���������� � ... ��� ����

//���� ����������.
#define STAR 0x10 
#define JAIL 0x11 


//���������� ��������
#define IND_IDDLE	PORTB= PORTB&0b11100011; 			// ���
#define IND_I2C		PORTB=(PORTB&0b11100011)|(2<<2); 	// I2C 
#define IND_UART	PORTB=(PORTB&0b11100011)|(3<<2); 	// UART
#define IND_1WIRE 	PORTB=(PORTB&0b11100011)|(4<<2); 	// 1WIRE 
#define IND_KBD 	PORTB=(PORTB&0b11100011)|(1<<2); 	// ����� ����������


#define I2C_R(x) ((x)|0b00000001) // ��������� ���� ������
#define I2C_W(x) ((x)&0b11111110) // ��������� ���� ������ (������ �� ����, ��� ���������)


#include <avr/io.h>//���������� �����/������
#include <avr/interrupt.h>//���������� ����������
#include <util/delay.h>
#include <avr/eeprom.h>
#include "main_init.h "
#include "axlib/i2c.h"
#include "axlib/usart.h"
#include "axlib/ds3231.h"
#include "axlib/1w.h"
#include "axlib/ds1820.h"
#include <avr/interrupt.h>
#include <math.h>
#include <avr/wdt.h>
//#include "dht.c"
#include "i2c_lcd.h"


 
/*
����:
PB0 � 1-wire
PB1 - ������ �������� ������������.
PB2 � 0 ��� ���������� ��������
PB3 � 2 ��� ���������� ��������
PB4 � 1 ��� ���������� ��������
PB5 
PC0 � ������� �������� ��������� �����
PC1 � ������ ��������� ����� 1
PC2 � ������ ��������� ����� 2
PC3 � ���������� RTC
PC4 � SDA
PC5 � SCL
PC6 � RESET
ADC6
ADC7
PD0 � RXD
PD1 � TXD
PD2 � ���������� ���������� (Int0)
PD3
PD4 - ������� ����������
PD5 - ������� ����������
PD6 � DHT22
PD7 � DHT22
*/
#define PIN_RTC (1<<3)
#define PIN_OSCIL (0b11100)
#define PIN_ANEMO0 (1<<4)
#define PIN_ANEMO1 (1<<5)
#define PIN_DHT0 (1<<6)
#define PIN_DHT1 (1<<7)
#define PIN_GERCONE (1<<1)
#define gercone() (!(PINB&PIN_GERCONE)) // 1 � ���� ������ �������

// ���������� ds1820 ������������: 
// 0 � ��������� �������� �������
//�� ������� 1, 2, 3, 7 (�� ���� �������), �� �������: 4, 5, 6 (�� ���� �������).

// ������ �����������
char ds1820addr[8][8]={
{0x28, 0xff, 0x2d, 0x40, 0x50, 0x15, 0x02, 0x68}, //0
{0x28, 0xff, 0xc1, 0x64, 0x50, 0x15, 0x02, 0xed}, //1
{0x28, 0xff, 0x43, 0x92, 0x50, 0x15, 0x02, 0xdd}, //2
{0x28, 0xff, 0x80, 0x6f, 0x50, 0x15, 0x02, 0x5d}, //3
{0x28, 0xff, 0xde, 0x54, 0x50, 0x15, 0x02, 0xb6}, //4
{0x28, 0xff, 0xfa, 0x62, 0x50, 0x15, 0x01, 0xf2}, //5
{0x28, 0xff, 0x64, 0x94, 0x50, 0x15, 0x01, 0x9f}, //6
{0x28, 0xff, 0xB8, 0x8B, 0x50, 0x15, 0x02, 0xA0}, //7
};

// ���������������� ���������� ���������: ������� ���.�/� ������� ���������� �� 1 ������� � �������.
// ׸��, � ���� ���������� ���������!!! � ����� ��-13 ��������� ����� ����������!!! ������� �����-����� �������.
#define ANEMO_INPUT_SPEED 4.5
#define ANEMO_OUTPUT_SPEED 3
unsigned const char 
	NORMAL_SPEED_IN[12]={
	0,	66,	90,	114, //���������� ������������ ��������
	63,	97,	115,135, //���������� ������������ �� ������ ��������
	75,	106,123,142}, //���������� ������������ �� ������ ��������
	NORMAL_SPEED_OUT[4]={0,63,83, 105}; // ���������� ������ ��� ������ �������� �����������. 
//���������� �������. ��� IN � [�����������<<2+����������-1]
//���� ��� ��������� MAX_SPEED_ERROR ��� ������ ������� �� ������� ����� � �������� ������� �������.
#define SPEED_DELTA_NORMAL 25 //������������ ���������� ����������, � ���������
#define MAX_SPEED_ERRORS 7
char speed_errors_count=0;//���������� ��������� ������ �������� ������.
#define predicted_speed_in() ((float)NORMAL_SPEED_IN[(fan_cond<<2)+fan_in]) 
#define predicted_speed_out() ((float)NORMAL_SPEED_OUT[fan_out]) 

/*
����������� ����� �� I2C

�������� ������� 1: (������ ������ ������������������ ������� (0))
0,1,2 � ������:
0 � ������� ����������� (slow|fast)
1 � ����/���
2 � ���������� (Hi|Lo)
3,4,5 � �������
3 � ������� ����������� (Lo|Hi)
4 � ����/���
5 � ���������� (Hi|Lo)
6 � ���� ������� �� 25� (off|on)
7 � ����������� (off|On) � ��� ������ ������� ��������� ���������� 25�
�������� ���������� ���������� �� "�������" ������� �� �����: ��-������, �� �������� ���������� ���������� ������ "��������� �������", ��-������, ������������� �� ��������.

�������� ������� 2: (�� ��������� ��� ����).
0 - 
1 -
2 - ����� ������� (12�)
3 - ����� ����������� (24�) (������� ��������� ����� ������� 25�)
4 - ��������(220�)
5 - ���������(220�)
6 - 
7 - 


���� ������������:
4 � ��� ������� (off|on)(���������� � ����������� �������� �����������)
5 - ���������� (Lo|Hi)
6 - ����������� (off|on)
7 � ����������� (cold|hot)

����������:
0 � �����, ��������.
1 � �����, ��������Ż
2 � ������
3 � �����, ������� ��������
4 � �������, �������� �������
*/

volatile char PORTR1=0;//1 ������ ����
volatile char PORTR2=0;//2 ������ ����
volatile char PORTRC=0;//���� ������������
volatile char PORTLED=0;//���������� ������ ����������.
volatile char PORTLED_BLINK=0;//���������� ������ ���������� � ��������.
#define fan_in_fast() {PORTR1=PORTR1|1;PORTR1=PORTR1&~(4);} // fast coil - Hi voltage
#define fan_in_normal() {PORTR1=PORTR1&~(1+4);} // slow coil � Hi voltage
#define fan_in_slow() {PORTR1=PORTR1|4;PORTR1=PORTR1&~(1);}//Slow coil � Lo voltage
#define fan_in_on() PORTR1=PORTR1|(1<<1)
#define is_fan_in_on (PORTR1&(1<<1))
#define fan_in_off() PORTR1=PORTR1&~(1<<1)
#define fan_out_fast() {PORTR1=PORTR1|(1<<3);PORTR1=PORTR1&~(4<<3);} // fast coil - Hi voltage
#define fan_out_normal() {PORTR1=PORTR1&~((1+4)<<3);} // slow coil � Hi voltage
#define fan_out_slow() {PORTR1=PORTR1|(4<<3);PORTR1=PORTR1&~(1<<3);}//Slow coil � Lo voltage
#define fan_out_on() PORTR1=PORTR1|((1<<1)<<3)
#define is_fan_out_on (PORTR1&((1<<1)<<3))
#define fan_out_off() PORTR1=PORTR1&~((1<<1)<<3)
#define humidator_on() {PORTR1=PORTR1|(1<<7); power25v_on();}
#define is_humidator_on (PORTR1&(1<<7))
#define humidator_off() PORTR1=PORTR1&~(1<<7)
#define power25v_on() PORTR1=PORTR1|(1<<6)
#define is_power25v_on (PORTR1&(1<<6))
#define is_power25v_used (is_water_out_on||is_humidator_on)
#define power25v_off() PORTR1=PORTR1&~(1<<6)

#define ozonator_on() PORTR2=PORTR2|(1<<4)
#define is_ozonator_on (PORTR2&(1<<4))
#define ozonator_off() PORTR2=PORTR1&~(1<<4)
#define light_on() PORTR2=PORTR2|(1<<5)
#define is_light_on (PORTR2&(1<<5))
#define light_off() PORTR2=PORTR1&~(1<<5)
#define water_in_on() PORTR2=PORTR2|(1<<2)
#define is_water_in_on (PORTR2&(1<<2))
#define water_in_off() PORTR2=PORTR1&~(1<<2)
#define water_out_on() {PORTR2=PORTR2|(1<<3); power25v_on();}
#define is_water_out_on (PORTR2&(1<<3))
#define water_out_off() PORTR2=PORTR1&~(1<<3)


#define condishen_on() PORTRC|=((1)<<4)
#define is_condishen_on (PORTRC&(1<<4))
#define condishen_off() PORTRC&=~((1)<<4)
#define condishen_fan_fast() PORTRC|=((2)<<4)
#define is_condishen_fan_fast (PORTRC&(2<<4))
#define condishen_fan_slow() PORTRC&=~((2)<<4)
#define condishen_compressor_on() PORTRC|=((4)<<4)
#define is_condishen_compressor_on (PORTRC&(4<<4))
#define condishen_compressor_off() PORTRC&=~((4)<<4)
#define condishen_compressor_hot() PORTRC|=((8)<<4)
#define is_condishen_compressor_hot (PORTRC&(8<<4))
#define condishen_compressor_cool() PORTRC&=~((8)<<4)

#define led_blue_on() 		{PORTLED|=(1<<0);PORTLED_BLINK&=~(1<<0);}
#define led_white_on() 		{PORTLED|=(1<<1);PORTLED_BLINK&=~(1<<1);}
#define led_green_on() 		{PORTLED|=(1<<2);PORTLED_BLINK&=~(1<<2);}
#define led_yellow_on() 	{PORTLED|=(1<<3);PORTLED_BLINK&=~(1<<3);}
#define led_red_on() 		{PORTLED|=(1<<4);PORTLED_BLINK&=~(1<<4);}
#define led_blue_off() 		{PORTLED&=~(1<<0);PORTLED_BLINK&=~(1<<0);}
#define led_white_off() 	{PORTLED&=~(1<<1);PORTLED_BLINK&=~(1<<1);}
#define led_green_off() 	{PORTLED&=~(1<<2);PORTLED_BLINK&=~(1<<2);}
#define led_yellow_off() 	{PORTLED&=~(1<<3);PORTLED_BLINK&=~(1<<3);}
#define led_red_off() 		{PORTLED&=~(1<<4);PORTLED_BLINK&=~(1<<4);}
#define led_blue_blink() 	PORTLED_BLINK|=(1<<0)
#define led_white_blink() 	PORTLED_BLINK|=(1<<1)
#define led_green_blink() 	PORTLED_BLINK|=(1<<2)
#define led_yellow_blink() 	PORTLED_BLINK|=(1<<3)
#define led_red_blink()		PORTLED_BLINK|=(1<<4)

// ������ I2C
#define I2C_RTC 0b11010000
/*
������ PCF8574
0 ����1 -  8 ��.
1 � �������
2 - ���� ������������.
3 - ����������
4 � ���������� ������ ����������
5 � ����2 � 8 ��.
*/
#define PCF8574_R1 0
#define PCF8574_R2 5
#define PCF8574_DISP 1
#define PCF8574_COND 2
#define PCF8574_KBD 3
#define PCF8574_LED 4

void send_PCF8574_i(char number, unsigned char data)//��� ��������� ����� ����������.
{
IND_I2C;
i2c_start();
i2c_send_byte(0x40|(number<<1));
i2c_send_byte(data); 
i2c_stop();
IND_IDDLE;
}
inline  void send_PCF8574(char number, unsigned char data) 
	{
	cli();
	send_PCF8574_i(number, data);
	sei(); 
	}

unsigned char read_PCF8574_i(char number) //��� ��������� ����� ����������.
{
unsigned char ret;
IND_I2C;
i2c_start();
i2c_send_byte(0x41|(number<<1));// ��������� � ������ ������.
ret = i2c_read_byte(NACK); //
i2c_stop();
IND_IDDLE;
return ret;
}
inline unsigned char read_PCF8574(char number) 
	{
	char reply;
	cli();
	reply=read_PCF8574_i(number);
	sei();
	return reply;
	} 


//������� ��������� 
#define hours(x) ((x)*3600)
#define mins(x) ((x)*60)
//�������� ������� ������������� �����������������, ����� ���, ����� ����������, ����� ������������.
#define OZONE_TEST_TIME (25)
#define OZONE_WAIT_TIME (mins(10)+OZONE_TEST_TIME)
#define OZONE_WORK_TIME (mins(40)+OZONE_WAIT_TIME)
#define OZONE_REMOVE_TIME (hours(3)+OZONE_WORK_TIME)
unsigned int ozonator_timer=0;//������� ������ ������ � ��������� ���������.
char ozonator_started=0;// 0 � ��������. 1 � ������ �� ���������, ��� �� ����������. 2 � �������.

//������ CO2 � DHT
#define CO2_I2C 0x32
unsigned int global_co2;
unsigned char global_t, global_h, co2_preheat, co2_errors;
void read_CO2_meter(void)
{
unsigned int co2;
unsigned char co2_lo, co2_hi, t, h, crc;
cli();
IND_I2C;
i2c_speed=I2C_SPEED*10;
i2c_start();
i2c_send_byte(I2C_R(CO2_I2C));// ��������� � ������ ������.
i2c_read_byte(ACK); //��� ����.
co2_hi = i2c_read_byte(ACK); 
co2_lo = i2c_read_byte(ACK); 
co2=(co2_hi<<8)+co2_lo;
t= i2c_read_byte(ACK); 
h= i2c_read_byte(ACK); 
co2_preheat= i2c_read_byte(ACK); 
co2_errors= i2c_read_byte(ACK); 
crc = i2c_read_byte(NACK); 
i2c_stop();
i2c_speed=I2C_SPEED;
while(!(I2C_PIN&(1<<SDA)));//������ ��� ������������, ���, ���� ������ ��2"��������" SDA. �� ����, ������, �� ������ ��������� ��� �� ���� ������, ��, ������-��, �� ������ ��� ������.
_delay_ms(200);
read_PCF8574_i(0);//������ ��������� � I2C ����� ��������� � ���� ������������ ������ �������� � ������, ��� ��� �������� ���-������ ��������.
IND_IDDLE;
sei();
if((t<100)&&(h<100)&&(co2<=6000) && ((unsigned char)crc==(unsigned char)(co2_hi+co2_lo+t+h+co2_preheat+co2_errors)))//����������� ��� ������
	{
	global_t=t;
	global_h=h;
	global_co2=co2;
	}
return;
}


volatile unsigned int TCNT0_16=0;//������ �0 8-������, ������� ��� �������� �������� ������ ��������� ����������.
volatile unsigned char keycode=0xff, keycode0=0xff;//����-��� ����������. 0xff � ������ (��� ������ �� ���������). 
volatile int thermo_in[4], thermo_out[4], thermo_r, humid_in, humid_out, humid_r, co2_r;
//thermo_in 0-3 � ����������� ������� (�� ���� �������: ���� ������������, ����������� ������, �����, ���� � �������.
//thermo_out 0-3 � ����������� ������� (�� ���� �������: ����� �� �������, ���� ������������, ����������� ������, ����� �� ������������)
volatile unsigned char time[3], new_sec=0;//����, ������ � �������. 
 
 
 
float abs_humid(float rel_humid_percent, float t_celsium)// ���������� ���������� ��������� ������� � �/�3. ��������� � ���������, ����������� � �������� �������
{
float E_gPa;//���������� ����������� �������� �������� ���� � ������� ��� ���� �����������, ���

if(t_celsium>0)	E_gPa=6.1121*exp( (18.678 - t_celsium/ 234.5)*t_celsium/ (257.14 + t_celsium));
	else 		E_gPa=6.1115*exp( (23.036 - t_celsium/ 333.7)*t_celsium / (279.82 + t_celsium));
// �� (Buck Research Manual 1996 �.), ���. �� http://www.pogoda.by/glossary/?nd=3&id=24

return 217*E_gPa*(rel_humid_percent/100)/(273.15+t_celsium);
}


 
//��������� �������:
char e_hmode EEMEM; // 0-10 � Auto(�������������� ���������), 0xff - auto
char e_auto_co2 EEMEM;
char e_auto_temp EEMEM;
char e_auto_cond EEMEM;
char e_pref_temp EEMEM;//�����������, ������� ��������� ������������.
char e_mode EEMEM; // 'M' � Manual
char e_fan_in EEMEM, e_fan_out EEMEM, e_fan_cond EEMEM; //�������� ������ ����������, ��������� � ��������������� �����������, 0-1-2-(3)
char e_humidator EEMEM;// ��������� �����������, 1/0
char e_condishen EEMEM, e_condishen_direction EEMEM;// ��������� ����������� ������������ 1|0 ���/����, 1|0 ������/����������
char e_led EEMEM;//���������� ������ ����������.
char e_led_blink EEMEM;//���������� ������ ����������.

char update_mode; //1 � �� ����� ��������� ������ ������
#define mode (auto_co2||auto_temp||auto_cond) // 0 � Manual, 1 - auto
char hmode; // 0-10 � Auto(�������������� ���������), 0xff - auto
char auto_co2;
char auto_temp;
char auto_cond;
char pref_temp;
char fan_in, fan_out, fan_cond; //�������� ������ ����������, ��������� � ��������������� �����������, 0-1-2-(3)
char humidator;// ��������� �����������, 1/0
char condishen, condishen_direction;// ��������� ����������� ������������ 1|0 ���/����, 1|0 ������/����������
char error_r1, error_cond;

void update_relay()
	{
	// ��������� ��� ����.
	char buf[25];
	switch(fan_in)
		{
		case 1: fan_in_on(); fan_in_slow(); 	break;
		case 2: fan_in_on(); fan_in_normal(); 	break;
		case 3: fan_in_on(); fan_in_fast(); 	break;
		case 0: 
		default: fan_in_off(); fan_in_normal(); break;
		}
	switch(fan_out)
		{
		case 1: fan_out_on(); fan_out_slow(); 	break;
		case 2: fan_out_on(); fan_out_normal();break;
		case 3: fan_out_on(); fan_out_fast(); 	break;
		case 0: 
		default: fan_out_off(); fan_out_normal(); break;
		}
	switch(fan_cond)
		{
		case 1: condishen_on(); condishen_fan_slow(); 	break;
		case 2: condishen_on(); condishen_fan_fast(); 	break;
		case 0: 
		default: condishen_off();condishen_fan_slow(); break;
		}
	switch(condishen)
		{
		case  1: condishen_compressor_on();		break;
		case  0: 
		default: condishen_compressor_off();	break;
		}
	switch(condishen_direction)
		{
		case  1: condishen_compressor_hot(); 	break;
		case  0: 
		default: condishen_compressor_cool();	break;
		}
	switch(humidator)
		{
		case  1: humidator_on();	break;
		case  0: 
		default: humidator_off();	break;
		}
	
	if(!is_power25v_used) power25v_off();
	
	send_PCF8574(PCF8574_R1, ~(PORTR1));
	send_PCF8574(PCF8574_R2, ~(PORTR2));
	i2c_speed=250*I2C_SPEED;
	send_PCF8574(PCF8574_COND, ~(PORTRC));
	i2c_speed=I2C_SPEED;
	error_r1=(~read_PCF8574(PCF8574_R1));
	i2c_speed=250*I2C_SPEED;
	error_cond=~read_PCF8574(PCF8574_COND)&0xf0;//��� ������ 4 ����, ������� ������� 4 ������� ����
	i2c_speed=I2C_SPEED;
	send_PCF8574(PCF8574_LED, ~PORTLED);


	if(!update_mode)sprintf(buf, "%2d     %d/%d%c%c%d%c", 
		pref_temp,
		fan_in, fan_out, humidator?'H':'D',
		(error_r1^PORTR1)|(error_cond^PORTRC)?'#':(gercone()?'+':'-'), 
		fan_cond,condishen? (condishen_direction?'H':'C') :'_');
	else switch (update_mode)
			{
			case 1: sprintf(buf, "%2d    UPD MODE", pref_temp); break;
			case 2: sprintf(buf, "%2d    UPD TEMP", pref_temp); break;
			case 3: sprintf(buf, "%2d    UPD SPEC", pref_temp); break;
			}
	if(! mode)	
		{
		buf[2]='M';
		buf[3]=(hmode==0xff)?'M':'0'+hmode;
		}
	else
		{
		buf[2]=auto_co2?	'A':'-';
		buf[3]=auto_temp?	'T':'-';
		buf[4]=auto_cond?	'C':'-';
		buf[5]=(hmode==0xff)?'M':'0'+hmode;
		}
	i2c_lcd_gotoxy(6,0);
	i2c_lcd_puts(buf);
	
	//���������� ����������: ��� �� ����������, � ��� ���������.
	//sprintf(buf, "%2x-%2x %2x-%2x",PORTR1,error_r1, PORTRC, error_cond);
	//i2c_lcd_gotoxy(0,1);
	//i2c_lcd_puts(buf);
	}



//-----------------------------------------------------------

 
 
 
 
 
main (void)
{
wdt_disable();

usart_init(9600);
i2c_speed=I2C_SPEED;
i2c_init();
ds3231_init();
owire_init(); // ������������� ���� 1wire
i2c_lcd_init();
//dht_init(6);
//dht_init(7);


cli();
IND_I2C;
//ds3231_write_time(BYTE h1224, BYTE hours, BYTE minutes, BYTE seconds);
//ds3231_write_time(DS3231_24, 22, 28, 0x00);
//ds3231_write_data(BYTE data, BYTE day, BYTE month, BYTE year);
//ds3231_write_data(0x07, 0x20, 0x03, 0x16);
i2c_start();
i2c_send_byte(I2C_W(I2C_RTC));
i2c_send_byte(0xE);//������� �����: ����������� �����
i2c_send_byte(0x0);//���������� ���������
i2c_send_byte(00);//���� ��������
i2c_stop();
IND_IDDLE;
sei();


//----------------------------------------------------------
// �������������
//���� � ����������
PCICR=PCICR|(1<<1);//��������� ���������� �� PCINT8...15
PCMSK1=PCMSK1|PIN_RTC;//��������� ���������� �� PCINT11, RTC
DDRC=DDRC&!(PIN_RTC); PORTC=PORTC|PIN_RTC;//C3 (RTC) � ���� � ���������.

DDRB=DDRB&!(PIN_OSCIL); // ����� �������� �� �����������

PORTB|=PIN_GERCONE; // ������� �� ���� � ���������
DDRB&=~PIN_GERCONE;

TCCR0B = (1<<CS02)|(1<<CS01); //�������0: ������������ �� ����� T0
TCCR1B = (1<<CS12)|(1<<CS11); //�������1: ������������ �� ����� T1
TIMSK0 = 1<<TOIE0; //��� ������������ �0 ����� �������� ���������� � ���������.
PORTD=PORTD|(PIN_ANEMO0+PIN_ANEMO1+PIN_DHT0+PIN_DHT1);//��� ��� ������ ���� ��������.


EIMSK |= (1 << INT0);  // ��������� ���������� INT0 (����������). �� ������� ������, �� ���������.
PORTD=PORTD|(4);//�����������, ��� ���� ����� ��������
//----------------------------------------------------------
/*����������
 123A
 456B
 789C
 *0#D
 
 4 ������� ������� � ������� (����� 0, ������ 3), 4 ������� � ������ (4 - �������).
 ����� � ������, ������ �� �������� (� ��� ������ ���� 1).
 ������ * = 0x10, # = 0x11, ������ ������ � 0xff
*/
send_PCF8574(PCF8574_KBD,0x0F);
read_PCF8574(PCF8574_KBD);// � ����� ������, ����� �������� ���� ����������.

//----------------------------------------------------------
unsigned char i, j=1;
unsigned char relative_speed_in, relative_speed_out;


	i2c_lcd_clr();
/*	i2c_lcd_gotoxy(0,0);
	i2c_lcd_puts("i2c I/O expander");
	i2c_lcd_gotoxy(0,1);
	i2c_lcd_puts("PCF8574A+ATmega328"); 
	i2c_lcd_gotoxy(0,2);
	i2c_lcd_puts("I did it! I won me!"); 
	i2c_lcd_gotoxy(8,3);
	i2c_lcd_puts("Yay!"); 
*/

fan_in		=eeprom_read_byte(&e_fan_in);
fan_out		=eeprom_read_byte(&e_fan_out);
fan_cond	=eeprom_read_byte(&e_fan_cond);
humidator	=eeprom_read_byte(&e_humidator);
condishen	=eeprom_read_byte(&e_condishen);
condishen_direction=eeprom_read_byte(&e_condishen_direction);
hmode		=eeprom_read_byte(&e_hmode);
auto_co2	=eeprom_read_byte(&e_auto_co2);
auto_temp	=eeprom_read_byte(&e_auto_temp);
auto_cond	=eeprom_read_byte(&e_auto_cond);
pref_temp	=eeprom_read_byte(&e_pref_temp);
PORTLED		=eeprom_read_byte(&e_led);
PORTLED_BLINK=eeprom_read_byte(&e_led_blink);

update_relay();

LCD_BLIGHT=eeprom_read_byte(&e_LCD_BLIGHT);

wdt_enable(WDTO_8S);//������ ���������� ������ �� 8 ������.
wdt_reset();//YAY! �� �� �������! ���������� ���������� ������!

eeprom_update_byte(&e_led,0);



while(1)
	{
	int t[8], fan_in_speed, fan_out_speed;//, t1,t2,h1,h2;
	//float t1,t2,h1,h2;
	char txtbuf[21], data[2];//, err1,err2;

	if((keycode!=0xff)&&(keycode!=keycode0))// ��� �� �������, � ������� ������� �������.
		{
		keycode0=keycode;
		switch (update_mode)
			{
			case 0: //�������� �����
				if(!mode)//������ ��������� ������������
					{
					switch(keycode)
						{
						case 0x1: 
							fan_in=1;
							update_relay();
							eeprom_update_byte(&e_fan_in,fan_in);
							break;
						case 0x2: 
							fan_in=2;
							update_relay();
							eeprom_update_byte(&e_fan_in,fan_in);
							break;
						case 0x3: 
							fan_in=3;
							update_relay();
							eeprom_update_byte(&e_fan_in,fan_in);
							break;
						case 0xA: 
							fan_in=0;
							update_relay();
							eeprom_update_byte(&e_fan_in,fan_in);
							break;
						case 0x4: 
							fan_out=1;
							update_relay();
							eeprom_update_byte(&e_fan_out,fan_out);
							break;
						case 0x5: 
							fan_out=2;
							update_relay();
							eeprom_update_byte(&e_fan_out,fan_out);
							break;
						case 0x6: 
							fan_out=3;
							update_relay();
							eeprom_update_byte(&e_fan_out,fan_out);
							break;
						case 0xB: 
							fan_out=0;
							update_relay();
							eeprom_update_byte(&e_fan_out,fan_out);
							break;
						case 0x7: 
							fan_cond=0;
							update_relay();
							eeprom_update_byte(&e_fan_cond,fan_cond);
							break;
						case 0x8: 
							fan_cond=1;
							update_relay();
							eeprom_update_byte(&e_fan_cond,fan_cond);
							break;
						case 0x9: 
							fan_cond=2;
							update_relay();
							eeprom_update_byte(&e_fan_cond,fan_cond);
							break;
						case 0xC: 
							if(condishen)condishen=0;
							else condishen=1;
							update_relay();
							eeprom_update_byte(&e_condishen,condishen);
							break;
						case JAIL: // #
							if(condishen_direction)condishen_direction=0; 
								else condishen_direction=1;
							update_relay();
							eeprom_update_byte(&e_condishen_direction,condishen_direction);
							break;
						}
					}

				if(hmode==0xff)//������ ��������� �����������
					{
					if(keycode==0xD)
						{
						if(humidator) humidator=0; 
							else humidator=1;
						update_relay();
						eeprom_update_byte(&e_humidator,humidator);
						}
					}
				break;
			
			case 1: //���������� � ������ Update_mode
				switch(keycode)
					{
					case 0x0A:
							if(ozonator_started==0)
								{//���� �� ���� ������� �� ������������ � ����������� �������.
								auto_co2=!auto_co2;
								update_relay();
								eeprom_update_byte(&e_auto_co2,auto_co2);
								}
							if(ozonator_started==1)
								{
								ozonator_started=2;
								ozonator_timer=0;
								led_white_on();
								update_relay();
								led_yellow_blink();
								eeprom_update_byte(&e_led_blink,PORTLED_BLINK);//���������� ����� �������� ���������. ���� ����� ���� �������, �� ���������.
								led_yellow_off();
								i2c_lcd_gotoxy(10,1);
								i2c_lcd_puts("TESTING");
								update_mode=0;
								}
						break;
					case 0x0B:
							auto_temp=!auto_temp;
							update_relay();
							eeprom_update_byte(&e_auto_temp,auto_temp);
						break;
					case 0x0C:
							auto_cond=!auto_cond;
							update_relay();
							eeprom_update_byte(&e_auto_cond,auto_cond);
						break;
					case 0x0D:
							if(hmode==0xff)hmode=4;
							else hmode=0xff;
							update_relay();
							eeprom_update_byte(&e_hmode,hmode);
						break;
					case JAIL:
							if (ozonator_started==0)
								{
								ozonator_started=1;
								i2c_lcd_gotoxy(0,1);
								i2c_lcd_puts("OZONATOR: START?    ");
								update_relay();
								}
							else if (ozonator_started==1)
								{
								ozonator_started=0;
								update_relay();
								}
						break;
					}
				if((keycode>0)&&(keycode<10))
					{
					hmode=keycode;
					update_relay();
					eeprom_update_byte(&e_hmode,hmode);
					}
				break;
			
			case 2: //������ ��������� ����������� 
				if((keycode>0)&&(keycode<10))
					{
					pref_temp=pref_temp/10+keycode;//������� ����
					update_relay();
					eeprom_update_byte(&e_pref_temp,pref_temp);
					}
				switch (keycode)
					{
					case 0xA:
						pref_temp=10+pref_temp%10;
						update_relay();
						eeprom_update_byte(&e_pref_temp,pref_temp);
						break;
					case 0xB:
						pref_temp=20+pref_temp%10;
						update_relay();
						eeprom_update_byte(&e_pref_temp,pref_temp);
						break;
					case 0xC:
						pref_temp=30+pref_temp%10;
						update_relay();
						eeprom_update_byte(&e_pref_temp,pref_temp);
						break;
					case 0xD:
						pref_temp=pref_temp-pref_temp%10;
						update_relay();
						eeprom_update_byte(&e_pref_temp,pref_temp);
						break;
					}
				break;
			
			default: update_mode=0; break;//�� ������, ���� ����� ����� �� ��������.
			}
		

		if(keycode==0x0) 
			{
			update_mode++;
			if (update_mode>2)update_mode=0;
			ozonator_started=0;
			PORTLED=0;
			PORTLED_BLINK=0;
			eeprom_update_byte(&e_led,PORTLED);
			eeprom_update_byte(&e_led_blink,PORTLED_BLINK);
			update_relay();
			}


		if(keycode==STAR)
			{
			if (LCD_BLIGHT) LCD_BLIGHT=0;
				else LCD_BLIGHT = LCD_BLIGHT_ON;
			eeprom_update_byte(&e_LCD_BLIGHT,LCD_BLIGHT);
			i2c_lcd_gotoxy(0,0);
			}
		}	
	
	
	if(new_sec)
		{
		ds3231_read_time(time);
		sprintf(txtbuf, "%u:%u:%u", time[2], !(time[2]%CIKLE), time[2]);//�� �������! ����� ��������� IF �� ��������!

		if(ozonator_started==2)
			{
			if(ozonator_timer<OZONE_TEST_TIME)
				{//���� ����� ��������� ����� �����.
				led_white_blink();
				led_blue_on();
				ozonator_on();
				fan_in=1;
				fan_out=0;
				fan_cond=0;
				humidator=0;
				i2c_lcd_gotoxy(9,1);
				sprintf(txtbuf, "TEST:%5ds", OZONE_TEST_TIME-ozonator_timer);
				i2c_lcd_puts(txtbuf);
				}
			else if(ozonator_timer<OZONE_WAIT_TIME)
					{//���� ����� ����� ������� ���������.
					led_white_on();
					led_blue_off();
					ozonator_off();
					fan_in=0;
					i2c_lcd_gotoxy(9,1);
					sprintf(txtbuf, "WAIT:%5ds", OZONE_WAIT_TIME-ozonator_timer);
					i2c_lcd_puts(txtbuf);
					}
			
				else if (ozonator_timer<OZONE_WORK_TIME)
						{//���� ����� ����������� .
						led_white_blink();
						led_blue_on();
						ozonator_on();
						fan_in=1;
						i2c_lcd_gotoxy(9,1);
						sprintf(txtbuf, "WORK:%5ds", OZONE_WORK_TIME-ozonator_timer);
						i2c_lcd_puts(txtbuf);
						}
					else if (ozonator_timer<OZONE_REMOVE_TIME)
							{//���� ����� ������� ����.
							led_white_on();
							led_blue_blink();
							ozonator_off();
							fan_in=3;
							fan_out=3;
							eeprom_update_byte(&e_fan_in,fan_in);
							eeprom_update_byte(&e_fan_out,fan_out);
							i2c_lcd_gotoxy(9,1);
							sprintf(txtbuf, "VENT:%5ds", OZONE_REMOVE_TIME-ozonator_timer);
							i2c_lcd_puts(txtbuf);
							}
						else
							{
							//��������� �����������;
							led_white_off();
							led_blue_blink();
							fan_in=1;
							fan_out=1;
							eeprom_update_byte(&e_fan_in,fan_in);
							eeprom_update_byte(&e_fan_out,fan_out);
							eeprom_update_byte(&e_led,PORTLED);
							eeprom_update_byte(&e_led_blink,PORTLED_BLINK);
							ozonator_started=0;
							}
			}
		
		if(!(time[2]%CIKLE))// ��� � 4 �������.
			{

			sprintf(txtbuf, "%02u:%02u", time[0], time[1]);
			i2c_lcd_gotoxy(0,0);
			i2c_lcd_puts(txtbuf);

			if((!update_mode)||(ozonator_started)) 
				{
				i2c_lcd_gotoxy(17,0);
				i2c_lcd_putch((error_r1^PORTR1)|(error_cond^PORTRC)?'#':(gercone()?'+':'-'));
				}
			
			if((!(time[2]%(CIKLE*CO2_divider)))&&(!ozonator_started))//� ����.������ �� �������.
				{
				read_CO2_meter();
				sprintf(txtbuf, "CO2:%04u T:%02u H:%02u  ", global_co2<10000?global_co2:9999, global_t<99?global_t:99, global_h<99?global_h:99);//, co2_preheat, co2_errors);
				i2c_lcd_gotoxy(0,1);
				i2c_lcd_puts(txtbuf);
				}



			for(i=0;i<8;i++)
				{
				t[i]=ds1820_read_t(&ds1820addr[i][0]);
				}
			//err1=dht_read_data(6, &t1, &h1)?'+':'-';
			//err2=dht_read_data(7, &t2, &h2)?'+':'-';
			//sprintf(txtbuf, "%d %02d:%02d:%02d %02d:%02d:%02d",  tmp, (int)t1, (int)h1, (int)abs_humid(h1,t1), (int)t2, (int)h2, (int)abs_humid(h2,t2));

			//��� �������� �� ����������� �����.
			TCNT0_16+=TCNT0;
			TCNT0=0;
			fan_in_speed=(int)((TCNT0_16*ANEMO_INPUT_SPEED)/CIKLE);
			fan_out_speed=(int)((TCNT1*ANEMO_OUTPUT_SPEED)/CIKLE);
			TCNT1=0;
			TCNT0_16=0;
			relative_speed_in=(fan_in||fan_cond)?((unsigned int)(((float)fan_in_speed*100)/(predicted_speed_in()))):100;
			relative_speed_out=fan_out?((unsigned int)(((float)fan_out_speed*100)/(predicted_speed_out()))):100;
			if((relative_speed_in<(100-SPEED_DELTA_NORMAL))||(relative_speed_out<(100-SPEED_DELTA_NORMAL))||(relative_speed_in>(100+SPEED_DELTA_NORMAL))||(relative_speed_out>(100+SPEED_DELTA_NORMAL)))speed_errors_count++;
				else speed_errors_count=0;//�������, ������� ��� ������ �������� �������� �� ����� �����.
			if(speed_errors_count>MAX_SPEED_ERRORS)led_red_blink();
				else led_red_off();
			
			if (!update_mode) sprintf(txtbuf, "Inp: %03d %02d %02d %02d %02d", fan_in_speed, t[1]/10, t[2]/10, t[3]/10, t[7]/10);
				else sprintf(txtbuf, "Inp: %03d (%03u,%03u%c) ", fan_in_speed,((unsigned int)predicted_speed_in()), relative_speed_in,'%');
			i2c_lcd_gotoxy(0,2);
			i2c_lcd_puts(txtbuf);
//			sprintf(txtbuf, "%02d %02d %02d %02d %02d:%04.1f", t[6]/10, t[5]/10, t[4]/10, (int)t2, (int)h2, abs_humid(h2,t2));
			if (!update_mode) sprintf(txtbuf, "Out: %03d %02d %02d %02d   ", fan_out_speed, t[6]/10, t[5]/10, t[4]/10);
				else sprintf(txtbuf, "Inp: %03d (%03u,%03u%c) ", fan_out_speed,((unsigned int)predicted_speed_out()), relative_speed_out, '%');
			i2c_lcd_gotoxy(0,3);
			i2c_lcd_puts(txtbuf);
			
			if(ozonator_started!=2)
				{//������� ���������� ��� ��������
				if(hmode!=0xff)//������������� ������������ ���������
					{
					if(global_h<(hmode*10-5))
						{
						//��������� ������. ���� �������� � �� ���������, � ������� �����.
						int max_hum=fan_in_speed*(abs_humid(70, t[7]/10)-abs_humid(80, t[1]/10));//������� ����� �� 80% � ����������� ������� �� ����������� �� ����� � ��������.;
						int second=(time[1]%10)*60+time[2];//������� ������� � �������������� ���������
						float max_hum_part=max_hum/400.0;// ��� ����������� ����������� ���������� �������� ��������� � �������� �����������?
						if (max_hum_part>1)humidator=1; //����������� ����� �������� ���������, �� ����� ��� �� ������.
						else 
							{//�������� ������ ���� ��������������� ���������.
							if(second<600*max_hum_part) humidator=1;
							else humidator=0;
							}
						}
					if (global_h>(hmode*10+5))
						{ // ��������� �����������, ���������
						humidator=0;
						}					
					}
				if(auto_temp)//������������� ������������ �����������
					{
					if((global_t>(pref_temp+2))&&(global_t>(t[1]/10+5)))//���� ������, ��� ����, � ������, ��� �� ����� (�� ������ ���������� �������)
						{
						fan_in=3;
						fan_out=1;
						}
					if ((global_t<(pref_temp))||(global_t<(t[1]/10+2)))
						{
						fan_in=0;
						fan_out=0;
						//�� ����� ��� ������ ��2
						if(auto_co2&&(global_co2>750)&&(co2_errors<20)&&(co2_preheat>180))//���� �������� ���������� �� ��2, � ������ ��2 ������� � �� ������fan_in=0;
							{
							fan_in=2;
							fan_out=2;
							}
						}
						
					}
				if((!auto_temp)&&auto_co2&&(co2_errors<20)&&(co2_preheat>180))//���� ���������� �� ����������� �� ��������, �������� ���������� �� ��2, � ������ ��2 ������� � �� ������
					{
					if(global_co2>750)
						{
						fan_in=2;
						fan_out=2;
						}
					if(global_co2<600)
						{
						fan_in=0;
						fan_out=0;
						}
					}
				if(auto_cond)//������������� �������� �����������
					{
					if(global_t>(pref_temp+3))//���� ������, ��� ����
						{
						condishen=1;
						condishen_direction=0;
						}
					if ((global_t<pref_temp)&&(global_t>(pref_temp-3)))
						{
						condishen=0;
						condishen_direction=0;
						}
					if((global_t<(pref_temp-3)))//���� ��������, ��� ����
						{
						condishen=1;
						condishen_direction=1;
						}
						
					}
				}
			
			
			}
		update_relay();
		new_sec=0;
		}
	
	}

	
}
//-------------------------------------------

ISR(PCINT1_vect) // RTC
{
wdt_reset();//YAY! �� �� �������! ���������� ���������� ������!


if(PINC&PIN_RTC) // PCINT ����������� ������ �� ������, �� ���������� � ���������� ���������, ��������� ������ ����������
	{
	new_sec=1;
	if (ozonator_started==2) ozonator_timer++;
	PORTLED^=PORTLED_BLINK;
	keycode0=0xff;//���������� ���������� ������� ������� � ��� ����������� ���������� �������.
	EIMSK |= (1 << INT0);  // � ��������� ���������� INT0 (����������). �� ������� ������, �� ���������.
	}
}

ISR(TIMER0_OVF_vect){TCNT0_16+=0x100;} //������ �� �0 16-������ �������, :- ).



//-------------------------------------------
ISR(INT0_vect) // ����������
{
EIMSK &= ~(1 << INT0);  // ��������� ���������� INT0 (����������). �� ����� ������ ������� ����������. � �� ���� ��� ����������� ����������.
cli(); // � ������, ��������� ��� ����������.
#define KB_DELAY 1
IND_KBD;

/*����������
 123A 3
 456B 2
 789C 1
 *0#D 0
 
 7654
 ������ * = 0x10, # = 0x11, ������ ������ � 0xff
 ����� � �������, ������ �� ����� (� ��� ������ ���� 1).
*/

char buf, line=0;
keycode=0x0;
_delay_us(KB_DELAY);
buf=read_PCF8574_i(PCF8574_KBD);
	send_PCF8574_i(PCF8574_KBD,0x1F);//column=4;

buf=~(buf|0xf0);//���� ��������� ��������� ����� � switch, ���������� �������� ����.
switch(buf)// ����� ������� ������ line �������� � �����?
	{
	case 0x08: line=0; break;
	case 0x04: line=1; break;
	case 0x02: line=2; break;
	case 0x01: line=3; break;
	default: keycode=0xff; break;//������ ��������� ������. �� ���� ������������ ���������.
	}
	
if(keycode!=0xff) // ���� ��� ����� ������, �� ������ ����������� �� �����.
	{
	//send_PCF8574_i(PCF8574_KBD,0x1F);//column=4; �������� ����
	_delay_us(KB_DELAY);
	buf=read_PCF8574_i(PCF8574_KBD);
	send_PCF8574_i(PCF8574_KBD,0x2F);//column=3;

	if((buf&0xf)==0xf)
		{
		keycode = 0xA+line;
		}
		else
			{
			//send_PCF8574_i(PCF8574_KBD,0x2F);//column=3; �������� ����
			buf=read_PCF8574_i(PCF8574_KBD);
			send_PCF8574_i(PCF8574_KBD,0x4F);//column=2;
			if((buf&0xf)==0xf)
				{
				if (line<3) keycode = line*3+3; else keycode=JAIL;
				}
			else
				{
				//send_PCF8574_i(PCF8574_KBD,0x4F);//column=2; �������� ����
				buf=read_PCF8574_i(PCF8574_KBD);
				send_PCF8574_i(PCF8574_KBD,0x8F);//column=1;
				if((buf&0xf)==0xf)
					{
					if (line<3) keycode=line*3+2; else keycode = 0;
					}
				else
					{
					//send_PCF8574_i(PCF8574_KBD,0x8F);//column=1; �������� ����
					_delay_us(KB_DELAY);
					buf=read_PCF8574_i(PCF8574_KBD);
					if((buf&0xf)==0xf)
						{
						if (line<3) keycode=line*3+1; else keycode = 0x10;
						}
					else //�� ��������� ��� ������, �� �� ����� ������ => ������ ����� ����� �������
						keycode=0xff;
					}
				}
			}
	}
send_PCF8574_i(PCF8574_KBD,0x0F); // ���������� ���������� � ����� ��������
_delay_us(KB_DELAY);
read_PCF8574_i(PCF8574_KBD);// ���������� ����������
IND_IDDLE;
sei();
}