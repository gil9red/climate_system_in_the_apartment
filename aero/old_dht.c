/* Name: dht.c
Пример чтения датчиков DHT11 и DHT22 - автоматическое разпознавание.
Взято с http://homes-smart.ru/index.php/oborudovanie/datchiki/datchiki-vlazhnosti-dht11-i-dht22
Проверено на м/к AVR

 */
#include <avr/io.h>
#ifndef DHT_H
#define DHT_H


#define DHT_PORT        PORTD // порт
#define DHT_DDR         DDRD
#define DHT_PIN         PIND

int dhtread (char bit, int *h, int *t) 
//bit — номер бита порта, 
// в *h будет влажность (в десятых долях %), в *t — температура, в десятых долях *.
// соответственно, надо результат на 10 разделить.
// если в программе имеются прерывания,то не забывайте их отлючать перед чтением датчика
{

uint8_t  j = 0, i = 0;
char datadht[5];
    
datadht[0] = datadht[1] = datadht[2] = datadht[3] = datadht[4] = 0;  
   
DHT_DDR|=(1<<bit); //pin as output
DHT_PORT&=~(1<<bit);
_delay_ms(18);


DHT_PORT|=(1<<bit);
DHT_DDR&=~(1<<bit);

_delay_us(50); // +1 для attiny(коррекция без кварца)
if (DHT_PIN&(1<<bit)) {*h=0x8000; *t=0; return 0;}
_delay_us(80); // +1 для attiny(коррекция без кварца)
if (!(DHT_PIN&(1<<bit))){*h=0x8000; *t=0x8000; return 0;};

while (DHT_PIN&(1<<bit));
for (j=0; j<5; j++)
    {
    datadht[j]=0;
    for(i=0; i<8; i++)
        {
        while (!(DHT_PIN&(1<<bit)));
        _delay_us (30);
        if (DHT_PIN&(1<<bit)) 
            datadht[j]|=1<<(7-i);
        while (DHT_PIN&(1<<bit));
        }
    }
    

	if (datadht[0]==0 && datadht[1]==0 && datadht[2]==0 && datadht[3]==0)
	{
	*h=0xfffe;
	*t=0xfffe;
	return 0;//обработка ошибки:не подключен датчик !!
	} 
	else if (datadht[0] + datadht[1] + datadht[2] + datadht[3] == datadht[4]) 
		{
		if (datadht[1]==0 && datadht[3]==0)
			{ //если эти переменные массива обе равны нулю,то это dht11
			*t=datadht[2]*10;
			*h=datadht[0]*10;
			}
		else  
			{ // dht22
			*h = datadht[0] * 256 + datadht[1];
			*t = (datadht[2] & 0x7F)* 256 + datadht[3];
			if (datadht[2] & 0x80)  *t *= -1;
			}
	} else 
		{
		*h=0xffff;
		*t=0xffff;
		return 0; //обработки ошибки: CHECKSUM !!
		}

 
// тут переменные int f - температура*10 и h - влажность*10
// можно переделать в float переменные ,скорректировав формулы выше..


	return 1;
}

#endif
