#define I2C_SPEED 2
#define CIKLE 5 //секунд на один опрос.
#define CO2_divider 4 // датчик CO2 опрашивать в ... раз реже

//коды клавиатуры.
#define STAR 0x10 
#define JAIL 0x11 


//Инидкаторы загрузки
#define IND_IDDLE	PORTB= PORTB&0b11100011; 			// жду
#define IND_I2C		PORTB=(PORTB&0b11100011)|(2<<2); 	// I2C 
#define IND_UART	PORTB=(PORTB&0b11100011)|(3<<2); 	// UART
#define IND_1WIRE 	PORTB=(PORTB&0b11100011)|(4<<2); 	// 1WIRE 
#define IND_KBD 	PORTB=(PORTB&0b11100011)|(1<<2); 	// Опрос клавиатуры


#define I2C_R(x) ((x)|0b00000001) // установка бита чтения
#define I2C_W(x) ((x)&0b11111110) // установка бита записи (вообще не надо, для симметрии)


#include <avr/io.h>//Библиотека ввода/вывода
#include <avr/interrupt.h>//Библиотека прерываний
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
Пины:
PB0 — 1-wire
PB1 - Геркон заслонки кондиционера.
PB2 — 0 бит индикатора загрузки
PB3 — 2 бит индикатора загрузки
PB4 — 1 бит индикатора загрузки
PB5 
PC0 — Питание датчиков влажности земли
PC1 — Датчик влажности земли 1
PC2 — Датчик влажности земли 2
PC3 — прерывание RTC
PC4 — SDA
PC5 — SCL
PC6 — RESET
ADC6
ADC7
PD0 — RXD
PD1 — TXD
PD2 — Прерывание клавиатуры (Int0)
PD3
PD4 - Счётчик анемометра
PD5 - Счётчик анемометра
PD6 — DHT22
PD7 — DHT22
*/
#define PIN_RTC (1<<3)
#define PIN_OSCIL (0b11100)
#define PIN_ANEMO0 (1<<4)
#define PIN_ANEMO1 (1<<5)
#define PIN_DHT0 (1<<6)
#define PIN_DHT1 (1<<7)
#define PIN_GERCONE (1<<1)
#define gercone() (!(PINB&PIN_GERCONE)) // 1 — если геркон замкнут

// Термометры ds1820 Расположение: 
// 0 — термометр кухонной вытяжки
//на притоке 1, 2, 3, 7 (по ходу воздуха), на вытяжке: 4, 5, 6 (по ходу воздуха).

// Адреса термометров
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

// Экспериментально полученные константы: сколько куб.м/ч воздуха приходится на 1 импульс в секунду.
// Чёрт, я хочу нормальный анемометр!!! У моего МС-13 показания вдвое расходятся!!! Записал более-менее средние.
#define ANEMO_INPUT_SPEED 4.5
#define ANEMO_OUTPUT_SPEED 3
unsigned const char 
	NORMAL_SPEED_IN[12]={
	0,	66,	90,	114, //вентилятор кондиционера выключен
	63,	97,	115,135, //вентилятор кондиционера на первой скорости
	75,	106,123,142}, //вентилятор кондиционера на второй скорости
	NORMAL_SPEED_OUT[4]={0,63,83, 105}; // нормальные потоки для каждой скорости вентилятора. 
//Подсчитана вручную. Для IN — [кондиционер<<2+вентилятор-1]
//Если при измерении MAX_SPEED_ERROR раз подряд выходит за границу нормы — включает красную мигалку.
#define SPEED_DELTA_NORMAL 25 //максимальное допустимое отклонение, в процентах
#define MAX_SPEED_ERRORS 7
char speed_errors_count=0;//количество найденных ошибок скорости подряд.
#define predicted_speed_in() ((float)NORMAL_SPEED_IN[(fan_cond<<2)+fan_in]) 
#define predicted_speed_out() ((float)NORMAL_SPEED_OUT[fan_out]) 

/*
Виртуальные порты на I2C

Релейный регистр 1: (первым указан нормальнозамкнутый контакт (0))
0,1,2 — приток:
0 — обмотки вентилятора (slow|fast)
1 — выкл/вкл
2 — напряжение (Hi|Lo)
3,4,5 — вытяжка
3 — обмотки вентилятора (Lo|Hi)
4 — выкл/вкл
5 — напряжение (Hi|Lo)
6 — Блок питания на 25В (off|on)
7 — увлажнитель (off|On) — для работы требует включения напряжения 25В
Подавать пониженное напряжение на "быструю" обмотку не нужно: во-первых, по скорости получается нормальная работа "медленной обмотки", во-вторых, трансформатор не выдержит.

Релейный регистр 2: (по умолчанию все выкл).
0 - 
1 -
2 - полив балкона (12В)
3 - полив подоконника (24В) (требует включения блока питания 25В)
4 - озонатор(220В)
5 - освещение(220В)
6 - 
7 - 


Реле кондиционера:
4 — вся система (off|on)(вентилятор и возможность включить кондиционер)
5 - вентилятор (Lo|Hi)
6 - кондиционер (off|on)
7 — кондиционер (cold|hot)

Светодиоды:
0 — синий, озонатор.
1 — белый, «ВНИМАНИЕ»
2 — зелёный
3 — жёлтый, «ошибка питания»
4 — красный, «фильтры забиты»
*/

volatile char PORTR1=0;//1 группа реле
volatile char PORTR2=0;//2 группа реле
volatile char PORTRC=0;//реле кондиционера
volatile char PORTLED=0;//Светодиоды панели управления.
volatile char PORTLED_BLINK=0;//Светодиоды панели управления — мигающие.
#define fan_in_fast() {PORTR1=PORTR1|1;PORTR1=PORTR1&~(4);} // fast coil - Hi voltage
#define fan_in_normal() {PORTR1=PORTR1&~(1+4);} // slow coil — Hi voltage
#define fan_in_slow() {PORTR1=PORTR1|4;PORTR1=PORTR1&~(1);}//Slow coil — Lo voltage
#define fan_in_on() PORTR1=PORTR1|(1<<1)
#define is_fan_in_on (PORTR1&(1<<1))
#define fan_in_off() PORTR1=PORTR1&~(1<<1)
#define fan_out_fast() {PORTR1=PORTR1|(1<<3);PORTR1=PORTR1&~(4<<3);} // fast coil - Hi voltage
#define fan_out_normal() {PORTR1=PORTR1&~((1+4)<<3);} // slow coil — Hi voltage
#define fan_out_slow() {PORTR1=PORTR1|(4<<3);PORTR1=PORTR1&~(1<<3);}//Slow coil — Lo voltage
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

// Адреса I2C
#define I2C_RTC 0b11010000
/*
Адреса PCF8574
0 Реле1 -  8 шт.
1 — Дисплей
2 - Реле кондиционера.
3 - клавиатура
4 — светодиоды пульта управления
5 — Реле2 — 8 шт.
*/
#define PCF8574_R1 0
#define PCF8574_R2 5
#define PCF8574_DISP 1
#define PCF8574_COND 2
#define PCF8574_KBD 3
#define PCF8574_LED 4

void send_PCF8574_i(char number, unsigned char data)//без изменения флага прерываний.
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

unsigned char read_PCF8574_i(char number) //без изменения флага прерываний.
{
unsigned char ret;
IND_I2C;
i2c_start();
i2c_send_byte(0x41|(number<<1));// открываем в режиме чтения.
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


//Таймеры озонатора 
#define hours(x) ((x)*3600)
#define mins(x) ((x)*60)
//озонатор сначала демонстрирует работоспособность, потом ждёт, потом включается, потом проветривает.
#define OZONE_TEST_TIME (25)
#define OZONE_WAIT_TIME (mins(10)+OZONE_TEST_TIME)
#define OZONE_WORK_TIME (mins(40)+OZONE_WAIT_TIME)
#define OZONE_REMOVE_TIME (hours(3)+OZONE_WORK_TIME)
unsigned int ozonator_timer=0;//Сколько секунд прошло с включения озонатора.
char ozonator_started=0;// 0 — выключен. 1 — запрос на включение, ещё не подтверждён. 2 — включён.

//Датчик CO2 и DHT
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
i2c_send_byte(I2C_R(CO2_I2C));// открываем в режиме чтения.
i2c_read_byte(ACK); //так надо.
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
while(!(I2C_PIN&(1<<SDA)));//прежде чем возвращаться, ждём, пока датчик СО2"отпустит" SDA. По идее, вообще, он должен отпустить ещё до этой строки, но, почему-то, не всегда это делает.
_delay_ms(200);
read_PCF8574_i(0);//первое обращение к I2C после обращения к мому самодельному слейву приводит к ошибке, так что прочитаю что-нибудь ненужное.
IND_IDDLE;
sei();
if((t<100)&&(h<100)&&(co2<=6000) && ((unsigned char)crc==(unsigned char)(co2_hi+co2_lo+t+h+co2_preheat+co2_errors)))//Прочиталось без ошибок
	{
	global_t=t;
	global_h=h;
	global_co2=co2;
	}
return;
}


volatile unsigned int TCNT0_16=0;//таймер т0 8-битный, поэтому для подсчёта значений заведём отдельную переменную.
volatile unsigned char keycode=0xff, keycode0=0xff;//скан-код клавиатуры. 0xff — ошибка (ещё ничего не прочитано). 
volatile int thermo_in[4], thermo_out[4], thermo_r, humid_in, humid_out, humid_r, co2_r;
//thermo_in 0-3 — температуры притока (по ходу воздуха: вход рекуператора, центральная камера, выход, вход в комнату.
//thermo_out 0-3 — температура вытяжки (по ходу воздуха: выход из комнаты, вход рекуператора, центральная камера, выход из рекуператора)
volatile unsigned char time[3], new_sec=0;//часы, минуты и секунды. 
 
 
 
float abs_humid(float rel_humid_percent, float t_celsium)// возвращает абсолютную влажность воздуха в г/м3. Влажность в процентах, температура в градусах цельсия
{
float E_gPa;//предельное парциальное давление водяного пара в воздухе при этой температуре, гПа

if(t_celsium>0)	E_gPa=6.1121*exp( (18.678 - t_celsium/ 234.5)*t_celsium/ (257.14 + t_celsium));
	else 		E_gPa=6.1115*exp( (23.036 - t_celsium/ 333.7)*t_celsium / (279.82 + t_celsium));
// По (Buck Research Manual 1996 г.), цит. по http://www.pogoda.by/glossary/?nd=3&id=24

return 217*E_gPa*(rel_humid_percent/100)/(273.15+t_celsium);
}


 
//состояние системы:
char e_hmode EEMEM; // 0-10 — Auto(поддерживаемая влажность), 0xff - auto
char e_auto_co2 EEMEM;
char e_auto_temp EEMEM;
char e_auto_cond EEMEM;
char e_pref_temp EEMEM;//температура, которую стараемся поддерживать.
char e_mode EEMEM; // 'M' — Manual
char e_fan_in EEMEM, e_fan_out EEMEM, e_fan_cond EEMEM; //скорость работы приточного, вытяжного и кондиционерного вентилятора, 0-1-2-(3)
char e_humidator EEMEM;// состояние увлажнителя, 1/0
char e_condishen EEMEM, e_condishen_direction EEMEM;// состояние компрессора кондиционера 1|0 вкл/выкл, 1|0 нагрев/охлаждение
char e_led EEMEM;//светодиоды панели управления.
char e_led_blink EEMEM;//светодиоды панели управления.

char update_mode; //1 — во время изменения режима работы
#define mode (auto_co2||auto_temp||auto_cond) // 0 — Manual, 1 - auto
char hmode; // 0-10 — Auto(поддерживаемая влажность), 0xff - auto
char auto_co2;
char auto_temp;
char auto_cond;
char pref_temp;
char fan_in, fan_out, fan_cond; //скорость работы приточного, вытяжного и кондиционерного вентилятора, 0-1-2-(3)
char humidator;// состояние увлажнителя, 1/0
char condishen, condishen_direction;// состояние компрессора кондиционера 1|0 вкл/выкл, 1|0 нагрев/охлаждение
char error_r1, error_cond;

void update_relay()
	{
	// обновляет все реле.
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
	error_cond=~read_PCF8574(PCF8574_COND)&0xf0;//там только 4 реле, поэтому смотрим 4 старших бита
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
	
	//отладочная информация: что мы записывали, и что прочитали.
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
owire_init(); // Инициализация шины 1wire
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
i2c_send_byte(0xE);//базовый адрес: управляющие байты
i2c_send_byte(0x0);//прерывание разрешено
i2c_send_byte(00);//часы включены
i2c_stop();
IND_IDDLE;
sei();


//----------------------------------------------------------
// Инициализация
//Пины и прерывания
PCICR=PCICR|(1<<1);//Разрешаем прерывания на PCINT8...15
PCMSK1=PCMSK1|PIN_RTC;//разрешаем прерывание на PCINT11, RTC
DDRC=DDRC&!(PIN_RTC); PORTC=PORTC|PIN_RTC;//C3 (RTC) — вход с подтяжкой.

DDRB=DDRB&!(PIN_OSCIL); // Вывод загрузки на осциллограф

PORTB|=PIN_GERCONE; // Герокон на ввод с подтяжкой
DDRB&=~PIN_GERCONE;

TCCR0B = (1<<CS02)|(1<<CS01); //Счетчик0: тактирование по спаду T0
TCCR1B = (1<<CS12)|(1<<CS11); //Счетчик1: тактирование по спаду T1
TIMSK0 = 1<<TOIE0; //при переполнении т0 будем вызывать прерывание с подсчётом.
PORTD=PORTD|(PIN_ANEMO0+PIN_ANEMO1+PIN_DHT0+PIN_DHT1);//ещё тут должна быть подтяжка.


EIMSK |= (1 << INT0);  // разрешаем прерывание INT0 (клавиатура). По низкому уровню, по умолчанию.
PORTD=PORTD|(4);//Оказывается, тут тоже нужна подтяжка
//----------------------------------------------------------
/*Клавиатура
 123A
 456B
 789C
 *0#D
 
 4 младших разряда — столбцы (левый 0, правый 3), 4 старших — строки (4 - верхняя).
 Пишем в строки, читаем из столбцов (в них должны быть 1).
 кейкод * = 0x10, # = 0x11, кейкод ошибки — 0xff
*/
send_PCF8574(PCF8574_KBD,0x0F);
read_PCF8574(PCF8574_KBD);// и сразу читаем, чтобы сбросить флаг прерывания.

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

wdt_enable(WDTO_8S);//ставим сторожевой таймер на 8 секунд.
wdt_reset();//YAY! Мы не зависли! Сбрасываем сторожевой таймер!

eeprom_update_byte(&e_led,0);



while(1)
	{
	int t[8], fan_in_speed, fan_out_speed;//, t1,t2,h1,h2;
	//float t1,t2,h1,h2;
	char txtbuf[21], data[2];//, err1,err2;

	if((keycode!=0xff)&&(keycode!=keycode0))// это не дребезг, а впервые нажатая клавиша.
		{
		keycode0=keycode;
		switch (update_mode)
			{
			case 0: //основной режим
				if(!mode)//ручная настройка вентиляторов
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

				if(hmode==0xff)//ручная настройка увлажнителя
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
			
			case 1: //клавиатура в режиме Update_mode
				switch(keycode)
					{
					case 0x0A:
							if(ozonator_started==0)
								{//если не было запроса на озонирование — стандартная функция.
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
								eeprom_update_byte(&e_led_blink,PORTLED_BLINK);//записываем жёлтый мигающий светодиод. Если будет сбой питания, он загорится.
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
			
			case 2: //ручная настройка температуры 
				if((keycode>0)&&(keycode<10))
					{
					pref_temp=pref_temp/10+keycode;//младший знак
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
			
			default: update_mode=0; break;//на случай, если вдруг вышли за диапазон.
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
		sprintf(txtbuf, "%u:%u:%u", time[2], !(time[2]%CIKLE), time[2]);//Не трогать! Иначе следующий IF не работает!

		if(ozonator_started==2)
			{
			if(ozonator_timer<OZONE_TEST_TIME)
				{//если время тестового пуска озона.
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
					{//если время ждать запуска озонатора.
					led_white_on();
					led_blue_off();
					ozonator_off();
					fan_in=0;
					i2c_lcd_gotoxy(9,1);
					sprintf(txtbuf, "WAIT:%5ds", OZONE_WAIT_TIME-ozonator_timer);
					i2c_lcd_puts(txtbuf);
					}
			
				else if (ozonator_timer<OZONE_WORK_TIME)
						{//если время озонировать .
						led_white_blink();
						led_blue_on();
						ozonator_on();
						fan_in=1;
						i2c_lcd_gotoxy(9,1);
						sprintf(txtbuf, "WORK:%5ds", OZONE_WORK_TIME-ozonator_timer);
						i2c_lcd_puts(txtbuf);
						}
					else if (ozonator_timer<OZONE_REMOVE_TIME)
							{//если время убирать озон.
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
							//закончили озонировать;
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
		
		if(!(time[2]%CIKLE))// раз в 4 секунды.
			{

			sprintf(txtbuf, "%02u:%02u", time[0], time[1]);
			i2c_lcd_gotoxy(0,0);
			i2c_lcd_puts(txtbuf);

			if((!update_mode)||(ozonator_started)) 
				{
				i2c_lcd_gotoxy(17,0);
				i2c_lcd_putch((error_r1^PORTR1)|(error_cond^PORTRC)?'#':(gercone()?'+':'-'));
				}
			
			if((!(time[2]%(CIKLE*CO2_divider)))&&(!ozonator_started))//в спец.режиме не выводим.
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

			//Все рассчёты по анемометрам здесь.
			TCNT0_16+=TCNT0;
			TCNT0=0;
			fan_in_speed=(int)((TCNT0_16*ANEMO_INPUT_SPEED)/CIKLE);
			fan_out_speed=(int)((TCNT1*ANEMO_OUTPUT_SPEED)/CIKLE);
			TCNT1=0;
			TCNT0_16=0;
			relative_speed_in=(fan_in||fan_cond)?((unsigned int)(((float)fan_in_speed*100)/(predicted_speed_in()))):100;
			relative_speed_out=fan_out?((unsigned int)(((float)fan_out_speed*100)/(predicted_speed_out()))):100;
			if((relative_speed_in<(100-SPEED_DELTA_NORMAL))||(relative_speed_out<(100-SPEED_DELTA_NORMAL))||(relative_speed_in>(100+SPEED_DELTA_NORMAL))||(relative_speed_out>(100+SPEED_DELTA_NORMAL)))speed_errors_count++;
				else speed_errors_count=0;//считаем, сколько раз подряд скорость выходила за рамки нормы.
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
				{//никакой автоматики при озонации
				if(hmode!=0xff)//автоматически поддерживаем влажность
					{
					if(global_h<(hmode*10-5))
						{
						//влажность низкая. Надо испарять — со скоростью, с которой можем.
						int max_hum=fan_in_speed*(abs_humid(70, t[7]/10)-abs_humid(80, t[1]/10));//сколько можно до 80% с температуры притока до температуры на входе в квартиру.;
						int second=(time[1]%10)*60+time[2];//текущая секунда в десятиминутном интервале
						float max_hum_part=max_hum/400.0;// как соотносится максимально достижимая скорость испарения и мощность увлажнителя?
						if (max_hum_part>1)humidator=1; //увлажнитель может работать постоянно, всё равно его не хватит.
						else 
							{//работает только долю десятиминутного интервала.
							if(second<600*max_hum_part) humidator=1;
							else humidator=0;
							}
						}
					if (global_h>(hmode*10+5))
						{ // влажность достаточная, выключаем
						humidator=0;
						}					
					}
				if(auto_temp)//автоматически поддерживаем температуру
					{
					if((global_t>(pref_temp+2))&&(global_t>(t[1]/10+5)))//дома теплее, чем надо, и теплее, чем на улице (на первом термометре притока)
						{
						fan_in=3;
						fan_out=1;
						}
					if ((global_t<(pref_temp))||(global_t<(t[1]/10+2)))
						{
						fan_in=0;
						fan_out=0;
						//Но можно ещё учесть СО2
						if(auto_co2&&(global_co2>750)&&(co2_errors<20)&&(co2_preheat>180))//если включена автоматика по СО2, а датчик СО2 прогрет и не глючитfan_in=0;
							{
							fan_in=2;
							fan_out=2;
							}
						}
						
					}
				if((!auto_temp)&&auto_co2&&(co2_errors<20)&&(co2_preheat>180))//если автоматика по температуре не работает, включена автоматика по СО2, а датчик СО2 прогрет и не глючит
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
				if(auto_cond)//автоматически включаем кондиционер
					{
					if(global_t>(pref_temp+3))//дома теплее, чем надо
						{
						condishen=1;
						condishen_direction=0;
						}
					if ((global_t<pref_temp)&&(global_t>(pref_temp-3)))
						{
						condishen=0;
						condishen_direction=0;
						}
					if((global_t<(pref_temp-3)))//дома холоднее, чем надо
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
wdt_reset();//YAY! Мы не зависли! Сбрасываем сторожевой таймер!


if(PINC&PIN_RTC) // PCINT срабатывает дважды на период, на восходящей и нисходящей полуволне, учитываем только восходящую
	{
	new_sec=1;
	if (ozonator_started==2) ozonator_timer++;
	PORTLED^=PORTLED_BLINK;
	keycode0=0xff;//сбрасываем предыдущую нажатую клавишу — для возможности повторного нажатия.
	EIMSK |= (1 << INT0);  // И разрешаем прерывание INT0 (клавиатура). По низкому уровню, по умолчанию.
	}
}

ISR(TIMER0_OVF_vect){TCNT0_16+=0x100;} //делаем из Т0 16-битный счётчик, :- ).



//-------------------------------------------
ISR(INT0_vect) // клавиатура
{
EIMSK &= ~(1 << INT0);  // Запрещаем прерывание INT0 (клавиатура). мы будем дёргать регистр клавиатуры. И не надо нам рекурсивных прерываний.
cli(); // и вообще, запрещаем все прерывания.
#define KB_DELAY 1
IND_KBD;

/*Клавиатура
 123A 3
 456B 2
 789C 1
 *0#D 0
 
 7654
 кейкод * = 0x10, # = 0x11, кейкод ошибки — 0xff
 Пишем в столбцы, читаем из строк (в них должны быть 1).
*/

char buf, line=0;
keycode=0x0;
_delay_us(KB_DELAY);
buf=read_PCF8574_i(PCF8574_KBD);
	send_PCF8574_i(PCF8574_KBD,0x1F);//column=4;

buf=~(buf|0xf0);//если поставить выражение прямо в switch, получается странный глюк.
switch(buf)// какой столбец прижат line кнокопой к земле?
	{
	case 0x08: line=0; break;
	case 0x04: line=1; break;
	case 0x02: line=2; break;
	case 0x01: line=3; break;
	default: keycode=0xff; break;//нажато несколько клавиш. На этом исследование закночено.
	}
	
if(keycode!=0xff) // если уже нашли ошибку, то дальше сканировать не будем.
	{
	//send_PCF8574_i(PCF8574_KBD,0x1F);//column=4; отослали выше
	_delay_us(KB_DELAY);
	buf=read_PCF8574_i(PCF8574_KBD);
	send_PCF8574_i(PCF8574_KBD,0x2F);//column=3;

	if((buf&0xf)==0xf)
		{
		keycode = 0xA+line;
		}
		else
			{
			//send_PCF8574_i(PCF8574_KBD,0x2F);//column=3; отослали выше
			buf=read_PCF8574_i(PCF8574_KBD);
			send_PCF8574_i(PCF8574_KBD,0x4F);//column=2;
			if((buf&0xf)==0xf)
				{
				if (line<3) keycode = line*3+3; else keycode=JAIL;
				}
			else
				{
				//send_PCF8574_i(PCF8574_KBD,0x4F);//column=2; отослали выше
				buf=read_PCF8574_i(PCF8574_KBD);
				send_PCF8574_i(PCF8574_KBD,0x8F);//column=1;
				if((buf&0xf)==0xf)
					{
					if (line<3) keycode=line*3+2; else keycode = 0;
					}
				else
					{
					//send_PCF8574_i(PCF8574_KBD,0x8F);//column=1; отослали выше
					_delay_us(KB_DELAY);
					buf=read_PCF8574_i(PCF8574_KBD);
					if((buf&0xf)==0xf)
						{
						if (line<3) keycode=line*3+1; else keycode = 0x10;
						}
					else //мы перебрали все строки, но не нашли нужную => нажато более одной клавиши
						keycode=0xff;
					}
				}
			}
	}
send_PCF8574_i(PCF8574_KBD,0x0F); // возвращаем клавиатуру в режим ожидания
_delay_us(KB_DELAY);
read_PCF8574_i(PCF8574_KBD);// сбрасываем прерывание
IND_IDDLE;
sei();
}