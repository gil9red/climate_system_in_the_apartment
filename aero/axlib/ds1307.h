
//---------------------------------------<                             axlib v1.1                                >----------------------------------------------------
//---------------------------------------<        Библиотека для работы с часами реального времени DS1307        >----------------------------------------------------
//---------------------------------------<              Кузнецов Алексей 2015 http://www.avrki.ru                >----------------------------------------------------

#ifndef DS1307_H_
#define DS1307_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/ds1307.h>)."
#endif


//-------------------------------------------------------------------------
//							Подключаемые библиотеки
//-------------------------------------------------------------------------
#include <axlib/type_var.h>
#include <axlib/i2c.h>
//-------------------------------------------------------------------------
//						Объявление служебных псевдонимов
//-------------------------------------------------------------------------

// Адрес устройства

#define DS1307_ADD_W				0xD0
#define DS1307_ADD_R				0xD1

// Адреса регистров

#define DS1307_SECONDS				0x00
#define DS1307_MINUTES				0x01
#define DS1307_HOURS				0x02
#define DS1307_DAY					0x03
#define DS1307_DATE					0x04
#define DS1307_MONTH				0x05
#define DS1307_YEAR					0x06
#define DS1307_SETS					0x07

// Режим вывода часов 

#define DS1307_HOURS_12				0x40
#define DS1307_24					0x3F
#define DS1307_AM					0x5F
#define DS1307_PM					0x60
#define DS1307_GET_PM				0x01
#define DS1307_GET_AM				0x00

// Выбор частоты выходного сигнала SQW/OUT

#define DS1307_SQW_OUT_1			0x00	// 1 Гц
#define DS1307_SQW_OUT_4			0x01	// 4.096 кГц
#define DS1307_SQW_OUT_8			0x02	// 8.193 кГц
#define DS1307_SQW_OUT_32			0x03	// 32.768 кГц

// Включение/вылючение SQW/OUT

#define DS1307_SQW_ON				0x10
#define DS1307_SQW_OFF				0xEF

// Логический уровень при выключеном SQW/OUT

#define DS1307_OUT_HIGHT			0x80
#define DS1307_OUT_LOW				0x00

// Ответы функций

#define DS1307_ERR					FALSE
#define DS1307_OK					TRUE

//-------------------------------------------------------------------------
//	Функция инициализации часов реального врмени DS1307.
//
// Принимвет аргумент режима вывода часов 12 или 24
//-------------------------------------------------------------------------

void ds1307_init(void)
{
	BYTE temp = 0;
	i2c_init();
	
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_SECONDS);
	i2c_restart();
	i2c_send_byte(DS1307_ADD_R);
	temp = i2c_read_byte(NACK);
	i2c_stop();
	
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_SECONDS);
	i2c_restart();
	i2c_send_byte(temp & 0x7F);
	i2c_stop();
}

//-------------------------------------------------------------------------
//	Функция подготовки значения данных
//
//
//	Принимает в качестве аргумента десятичное значение
//
//	Возвращаемое значение
//
//		Преобразованное для записи в часы
//
//-------------------------------------------------------------------------

BYTE ds1307_byte(BYTE data)
{
	BYTE temp = 0;
	
	while(data > 9)
	{
		data -= 10;
		temp++;
	}
	
	return (data | (temp << 4));
}

//-------------------------------------------------------------------------
//	Функция чтения времени
//
//
//	Принимает аргумент указатель на первый элемент массива.
//
//	После вызовы данной функции в массиве будет лежать прочитанное время.
//	В первом элементе часы, во втором минуты, в третьем секунды.
//
//							
//-------------------------------------------------------------------------

BYTE ds1307_read_time(BYTE *str)
{	
	BYTE temp[3];
	UBYTE i = 0;
	BYTE hour = 0;
	
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_SECONDS);
	i2c_restart();
	i2c_send_byte(DS1307_ADD_R);
	temp[2] = i2c_read_byte(ACK);
	temp[1] = i2c_read_byte(ACK);
	temp[0] = i2c_read_byte(NACK);
	i2c_stop();
	
	if(temp[0] & 0x40) // AM/PM
		{
			*str = ((temp[0] & 0x0F)+(((temp[0] >> 4) & 0x01) * 10));
			str++;
			
			hour = ((temp[0] >> 5) & 0x01);
			
			i = 1;
			while(i < 3)
			{
				*str = ((temp[i] & 0x0F)+((temp[i] >> 4) * 10));
				str++;
				i++;
			}
		}
	else // 24
		{
			i = 0;
			while(i < 3)
			{
				*str = ((temp[i] & 0x0F)+((temp[i] >> 4) * 10));
				str++;
				i++;
			}
		}
	return hour;
}

//-------------------------------------------------------------------------
//	Функция записи времени
//
//	Принимает аргументы часы, минуты, секунды.
//
//-------------------------------------------------------------------------

void ds1307_write_time(BYTE h1224, BYTE hours, BYTE minutes, BYTE seconds)
{
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_SECONDS);
	i2c_send_byte(ds1307_byte(seconds));
	i2c_send_byte(ds1307_byte(minutes));
	
	if(h1224 == DS1307_24)
		{
			i2c_send_byte(ds1307_byte(hours) & DS1307_24);
		}
	else
		{
			if(h1224 == DS1307_AM)
				{
					if(hours > 12) hours -= 12;
					i2c_send_byte((ds1307_byte(hours) & DS1307_AM) | DS1307_HOURS_12);
				}
			else
				{
					if(hours > 12) hours -= 12;
					i2c_send_byte(ds1307_byte(hours)| DS1307_PM);
				}
		}
	
	i2c_stop();
}

//-------------------------------------------------------------------------
//	Функция чтения даты
//
//
//	Принимает аргумент указатель на первый элемент массива.
//
//	После вызовы данной функции в массиве будет лежать прочитанное время.
//	В первом элементе день недели, во втором число, в третьем месяц, в четвертом год 00-99.
//
//
//-------------------------------------------------------------------------

void ds1307_read_data(BYTE *str)
{
	BYTE temp[3];
	UBYTE i = 0;
	
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_DAY);
	i2c_restart();
	i2c_send_byte(DS1307_ADD_R);
	*str = i2c_read_byte(ACK);
	temp[0] = i2c_read_byte(ACK);
	temp[1] = i2c_read_byte(ACK);
	temp[2] = i2c_read_byte(NACK);
	i2c_stop();
	
	str++;
	while(i < 4)
		{
			*str = ((temp[i] & 0x0F)+((temp[i] >> 4) * 10));
			str++;
			i++;
		}
}

//-------------------------------------------------------------------------
//	Функция записи даты
//
//	Принимает аргументы день недели(1-7), число(1-31), месяц(1-12), год(00-99).
//
//-------------------------------------------------------------------------

void ds1307_write_data(BYTE data, BYTE day, BYTE month, BYTE year)
{
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_DAY);
	i2c_send_byte(data);
	i2c_send_byte(ds1307_byte(day));
	i2c_send_byte(ds1307_byte(month));
	i2c_send_byte(ds1307_byte(year));
	i2c_stop();
}

//-------------------------------------------------------------------------
//	Функция управления выходом SQW/OUT при включенном SQWE 
//
//	Принимает аргумент делителя частоты.
//
//			DS1307_SQW_OUT_1	1 Гц
//			DS1307_SQW_OUT_4	4.096 кГц
//			DS1307_SQW_OUT_8	8.193 кГц
//			DS1307_SQW_OUT_32	32.768 кГц
//-------------------------------------------------------------------------

void ds1307_sqw_on(BYTE rs)
{
	BYTE out = 0x10;
	
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_SETS);
	i2c_send_byte(out | rs);
	i2c_stop();
}

//-------------------------------------------------------------------------
//	Функция управления выходом SQW/OUT при выключенном SQWE
//
//	Принимает аргумент состояния логического уровня на выводе.
//
//			DS1307_OUT_HIGHT			
//			DS1307_OUT_LOW
//-------------------------------------------------------------------------

void ds1307_sqw_off(BYTE out)
{
	i2c_start();
	i2c_send_byte(DS1307_ADD_W);
	i2c_send_byte(DS1307_SETS);
	i2c_send_byte(out);
	i2c_stop();
}

#endif /* DS1307_H_ */