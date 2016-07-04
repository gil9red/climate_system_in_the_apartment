
//-------------------------------------------------<                   axlib v1.1                    >----------------------------------------------------
//-------------------------------------------------<  Библиотека для работы с портами ввода/вывода   >----------------------------------------------------
//-------------------------------------------------<    Кузнецов Алексей 2015 http://www.avrki.ru    >----------------------------------------------------


#ifndef PORTS_H_
#define PORTS_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/ports.h>)."
#endif


//-------------------------------------------------------------------------
//  Управление портом A
//-------------------------------------------------------------------------

#ifdef PORTA
	#define PORTA_OUT_ALL()	(DDRA = 0xFF);
	#define PORTA_IN_ALL()	(DDRA = 0x00);	
	// Настройка порта A на выход по пинам
	#define PORTA_OUT(x)	(DDRA |= (1 << x))	
	// Настройка порта A на вход по пинам
	#define PORTA_IN(x)		(DDRA &= ~(1 << x))	
	// Вывод в порт A по пинам 1
	#define PORTA_ON(x)		(PORTA |= (1 << x))
	// Вывод в порт A по пинам 0
	#define PORTA_OFF(x)	(PORTA &= ~(1 << x))	
	// Чтение из порта A по пинам
	#define PORTA_RD(x)		(PINA & (1 << x))	
	// Инвертирование битов порта A
	#define PORTA_XOR(x)	(PORTA ^= (1 << x))	
	// Вывод значения в порт
	#define PORTA_DATA_OUT(a)	PORTA = a	
	// Чтение из порта
	#define PORTA_DATA_IN()	PINA
#endif

//-------------------------------------------------------------------------
//  Управление портом B
//-------------------------------------------------------------------------

#ifdef	PORTB
	#define PORTB_OUT_ALL()	(DDRB = 0xFF);
	#define PORTB_IN_ALL()	(DDRB = 0x00);	
	// Настройка порта B на выход по пинам
	#define PORTB_OUT(x)	(DDRB |= (1 << x))	
	// Настройка порта B на вход по пинам
	#define PORTB_IN(x)		(DDRB &= ~(1 << x))	
	// Вывод в порт B по пинам 1
	#define PORTB_ON(x)		(PORTB |= (1 << x))
	// Вывод в порт B по пинам 0
	#define PORTB_OFF(x)	(PORTB &= ~(1 << x))	
	// Чтение из порта B по пинам
	#define PORTB_RD(x)		(PINB & (1 << x))	
	// Инвертирование битов порта B
	#define PORTB_XOR(x)	(PORTB ^= (1 << x))	
	// Вывод значения в порт
	#define PORTB_DATA_OUT(b)	PORTB = b	
	// Чтение из порта
	#define PORTB_DATA_IN()	PINB
#endif

//-------------------------------------------------------------------------
//  Управление портом C
//-------------------------------------------------------------------------

#ifdef	PORTC
	#define PORTC_OUT_ALL()	(DDRC = 0xFF);
	#define PORTC_IN_ALL()	(DDRC = 0x00);	
	// Настройка порта C на выход по пинам
	#define PORTC_OUT(x)	(DDRC |= (1 << x))	
	// Настройка порта C на вход по пинам
	#define PORTC_IN(x)		(DDRC &= ~(1 << x))	
	// Вывод в порт C по пинам 1
	#define PORTC_ON(x)		(PORTC |= (1 << x))
	// Вывод в порт C по пинам 0
	#define PORTC_OFF(x)	(PORTC &= ~(1 << x))	
	// Чтение из порта C по пинам
	#define PORTC_RD(x)		(PINC & (1 << x))	
	// Инвертирование битов порта C
	#define PORTC_XOR(x)	(PORTC ^= (1 << x))	
	// Вывод значения в порт
	#define PORTC_DATA_OUT(c)	PORTC = c	
	// Чтение из порта
	#define PORTC_DATA_IN()	PINC
#endif

//-------------------------------------------------------------------------
//  Управление портом D
//-------------------------------------------------------------------------

#ifdef	PORTD
	#define PORTD_OUT_ALL()	(DDRD = 0xFF);
	#define PORTD_IN_ALL()	(DDRD = 0x00);		
	// Настройка порта D на выход по пинам
	#define PORTD_OUT(x)	(DDRD |= (1 << x))	
	// Настройка порта D на вход по пинам
	#define PORTD_IN(x)		(DDRD &= ~(1 << x))	
	// Вывод в порт D по пинам 1
	#define PORTD_ON(x)		(PORTD |= (1 << x))
	// Вывод в порт D по пинам 0
	#define PORTD_OFF(x)	(PORTD &= ~(1 << x))	
	// Чтение из порта D по пинам
	#define PORTD_RD(x)		(PIND & (1 << x))	
	// Инвертирование битов порта D
	#define PORTD_XOR(x)	(PORTD ^= (1 << x))	
	// Вывод значения в порт
	#define PORTD_DATA_OUT(d)	PORTD = d	
	// Чтение из порта
	#define PORTD_DATA_IN()	PIND
#endif

//-------------------------------------------------------------------------

#endif /* PORTS_H_ */