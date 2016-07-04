
//-------------------------------------------------<                axlib v1.1                 >----------------------------------------------------
//-------------------------------------------------<   Библиотека для работы с EEPROM AT24Cx   >----------------------------------------------------
//-------------------------------------------------< Кузнецов Алексей 2015 http://www.avrki.ru >----------------------------------------------------

#ifndef AT24C_H_
#define AT24C_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/at24c.h>)."
#endif

//-------------------------------------------------------------------------
//							Подключаемые библиотеки
//-------------------------------------------------------------------------
#include <avr/io.h>
#include <util/delay.h>
#include <axlib/type_var.h>
#include <axlib/i2c.h>


//-------------------------------------------------------------------------
//							Объявление функций
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//							Инициализация микросхемы
//
//	Принимаемые аргументы
//
//		BYTE add		Адрес микросхемы  от 0x50 до 0x57
//
//	Возвращаемое значение
//
//		ACK		При удачной инициализации
//		NACK	При неудачной инициализации
//-------------------------------------------------------------------------

BYTE at24c_init(BYTE add)
{
	BYTE eeprom_add = ((add << 1) & 0xFE);
	BYTE answer = NACK;
	
	i2c_start();
	// Передача  адреса микросхемы
	answer = i2c_send_byte(eeprom_add);
	if(answer == NACK) return answer;
	i2c_stop();
	
	return answer;
}

//-------------------------------------------------------------------------
//							Функция записи байта в память
//
//	Принимаемые аргументы
//
//		UBYTE add		Адрес микросхемы  от 0x50 до 0x57
//		UWORD addbyte	Адрес байта от 0 до макс.
//		BYTE data		Байт данных
//
//	Возвращаемое значение
//
//		ACK		При удачной записи данных
//		NACK	При неудачной записи данных
//-------------------------------------------------------------------------


BYTE at24c_write_byte(UBYTE add, UWORD addpage, BYTE data)
{
	BYTE eeprom_add = ((add << 1) & 0xFE);
	BYTE byte_add_H = (addpage >> 8) & 0xFF;
	BYTE byte_add_L = (addpage & 0xFF);
	BYTE answer = NACK;
	
	i2c_start();
	
	// Передача  адреса микросхемы
	answer = i2c_send_byte(eeprom_add);
	if(answer == NACK) return answer;
	// Передача старшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// Передача младшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	// Запись данных в память микросхемы
	answer = i2c_send_byte(data);
	if(answer == NACK) return answer;
	
	i2c_stop();
	_delay_ms(5);
	
	return answer;
}

//-------------------------------------------------------------------------
//							Функция записи страницы в память
//
//	Принимаемые аргументы
//
//		UBYTE add		Адрес микросхемы  от 0x50 до 0x57
//		UWORD addbyte	Адрес начального байта страницы от 0 до макс.
//		BYTE *data		Указатель на первый элемент массива
//		UBYTE count		Количество байт в странице
//
//	Возвращаемое значение
//
//		ACK		При удачной записи данных
//		NACK	При неудачной записи данных
//-------------------------------------------------------------------------


BYTE at24c_write_page(UBYTE add, UWORD addpage, BYTE *data, UWORD count)
{	
	if(count == 0) return NACK;
	addpage *= 32;
	
	BYTE eeprom_add = ((add << 1) & 0xFE);
	BYTE byte_add_H = (addpage >> 8) & 0xFF;
	BYTE byte_add_L = (addpage & 0xFF);
	BYTE answer = NACK;
	
	i2c_start();
	
	// Передача  адреса микросхемы
	answer = i2c_send_byte(eeprom_add);
	if(answer == NACK) return answer;
	// Передача старшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// Передача младшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	// Запись данных в память микросхемы
	for(UWORD i = 0; i < count; i++)
	{
		answer = i2c_send_byte(*data);
		if(answer == NACK) return answer;
		data++;
	}
	
	i2c_stop();
	_delay_ms(5);
	
	return answer;
}

//-------------------------------------------------------------------------
//							Функция записи массива в память
//
//	Принимаемые аргументы
//
//		UBYTE add		Адрес микросхемы  от 0x50 до 0x57
//		UWORD addbyte	Адрес начального байта для записи массива в память
//		BYTE *data		Указатель на первый элемент массива
//		UBYTE count		Количество байт для записи
//
//	Возвращаемое значение
//
//		ACK		При удачной записи данных
//		NACK    При неудачной записи данных
//-------------------------------------------------------------------------


BYTE at24c_write_str(UBYTE add, UWORD addbyte, BYTE *data, UWORD count)
{
	if(count == 0) return NACK;
	
	BYTE answer = NACK;
	UWORD i = 0;
	
	while(i < count)
	{
		answer = at24c_write_byte(add, addbyte, *data);
		if(answer == NACK) return NACK;
		addbyte++;
		data++;
		i++;
	}
	
	_delay_ms(5);
	
	return answer;
}

//-------------------------------------------------------------------------
//							Функция чтения байта из памяти
//
//	Принимаемые аргументы
//
//		UBYTE add		Адрес микросхемы  от 0x50 до 0x57
//		UWORD addbyte	Адрес начального байта для чтения массива из память
//
//	Возвращаемое значение
//
//				При удачном чтении зачение прочитанного байта
//		NACK	При неудачном чтении байта
//-------------------------------------------------------------------------

BYTE at24c_read_byte(UBYTE add, UWORD addbyte)
{	
	BYTE eeprom_add_w = ((add << 1) & 0xFE);
	BYTE eeprom_add_r = (eeprom_add_w | 0x01);
	BYTE byte_add_H = (addbyte >> 8) & 0xFF;
	BYTE byte_add_L = (addbyte & 0xFF);
	BYTE answer = NACK;
	
	i2c_start();
	
	// Передача  адреса микросхемы
	answer = i2c_send_byte(eeprom_add_w);
	if(answer == NACK) return answer;
	// Передача старшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// Передача младшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	i2c_restart();
	
	// Передача  адреса микросхемы
	answer = i2c_send_byte(eeprom_add_r);
	if(answer == NACK) return answer;
	
	// Получение байта
	answer = i2c_read_byte(NACK);
	
	i2c_stop();
	_delay_ms(5);
	
	return answer;
}

//-------------------------------------------------------------------------
//							Функция чтения массива из памяти
//
//	Принимаемые аргументы
//
//		UBYTE add		Адрес микросхемы  от 0x50 до 0x57
//		UWORD addbyte	Адрес начального байта для чтения массива из память
//		BYTE *data		Указатель на первый элемент массива
//		UBYTE count		Количество байт для чтения
//
//	Возвращаемое значение
//
//		ACK		При удачном чтении данных
//		NACK	При неудачном чтении данных
//-------------------------------------------------------------------------

BYTE at24c_read_str(UBYTE add, UWORD addbyte, BYTE *data, UWORD count)
{
	if(count == 0) return NACK;
	
	BYTE eeprom_add_w = ((add << 1) & 0xFE);
	BYTE eeprom_add_r = (eeprom_add_w | 0x01);
	BYTE byte_add_H = (addbyte >> 8) & 0xFF;
	BYTE byte_add_L = (addbyte & 0xFF);
	BYTE answer = NACK;
	UWORD i = 0;
	
	i2c_start();
	
	// Передача  адреса микросхемы
	answer = i2c_send_byte(eeprom_add_w);
	if(answer == NACK) return answer;
	// Передача старшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// Передача младшего байта адреса начального байта данных
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	i2c_restart();
	
	// Передача  адреса микросхемы
	answer = i2c_send_byte(eeprom_add_r);
	if(answer == NACK) return answer;
	
	count--;
	
	while( i < count)
	{
		*data = i2c_read_byte(ACK);
		data++;
		i++;
	}
	// Получение последнего байта
	*data = i2c_read_byte(NACK);
	
	i2c_stop();
	_delay_ms(5);
	
	return answer;
}

#endif /* AT24C_H_ */