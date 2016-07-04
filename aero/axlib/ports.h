
//-------------------------------------------------<                   axlib v1.1                    >----------------------------------------------------
//-------------------------------------------------<  ���������� ��� ������ � ������� �����/������   >----------------------------------------------------
//-------------------------------------------------<    �������� ������� 2015 http://www.avrki.ru    >----------------------------------------------------


#ifndef PORTS_H_
#define PORTS_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/ports.h>)."
#endif


//-------------------------------------------------------------------------
//  ���������� ������ A
//-------------------------------------------------------------------------

#ifdef PORTA
	#define PORTA_OUT_ALL()	(DDRA = 0xFF);
	#define PORTA_IN_ALL()	(DDRA = 0x00);	
	// ��������� ����� A �� ����� �� �����
	#define PORTA_OUT(x)	(DDRA |= (1 << x))	
	// ��������� ����� A �� ���� �� �����
	#define PORTA_IN(x)		(DDRA &= ~(1 << x))	
	// ����� � ���� A �� ����� 1
	#define PORTA_ON(x)		(PORTA |= (1 << x))
	// ����� � ���� A �� ����� 0
	#define PORTA_OFF(x)	(PORTA &= ~(1 << x))	
	// ������ �� ����� A �� �����
	#define PORTA_RD(x)		(PINA & (1 << x))	
	// �������������� ����� ����� A
	#define PORTA_XOR(x)	(PORTA ^= (1 << x))	
	// ����� �������� � ����
	#define PORTA_DATA_OUT(a)	PORTA = a	
	// ������ �� �����
	#define PORTA_DATA_IN()	PINA
#endif

//-------------------------------------------------------------------------
//  ���������� ������ B
//-------------------------------------------------------------------------

#ifdef	PORTB
	#define PORTB_OUT_ALL()	(DDRB = 0xFF);
	#define PORTB_IN_ALL()	(DDRB = 0x00);	
	// ��������� ����� B �� ����� �� �����
	#define PORTB_OUT(x)	(DDRB |= (1 << x))	
	// ��������� ����� B �� ���� �� �����
	#define PORTB_IN(x)		(DDRB &= ~(1 << x))	
	// ����� � ���� B �� ����� 1
	#define PORTB_ON(x)		(PORTB |= (1 << x))
	// ����� � ���� B �� ����� 0
	#define PORTB_OFF(x)	(PORTB &= ~(1 << x))	
	// ������ �� ����� B �� �����
	#define PORTB_RD(x)		(PINB & (1 << x))	
	// �������������� ����� ����� B
	#define PORTB_XOR(x)	(PORTB ^= (1 << x))	
	// ����� �������� � ����
	#define PORTB_DATA_OUT(b)	PORTB = b	
	// ������ �� �����
	#define PORTB_DATA_IN()	PINB
#endif

//-------------------------------------------------------------------------
//  ���������� ������ C
//-------------------------------------------------------------------------

#ifdef	PORTC
	#define PORTC_OUT_ALL()	(DDRC = 0xFF);
	#define PORTC_IN_ALL()	(DDRC = 0x00);	
	// ��������� ����� C �� ����� �� �����
	#define PORTC_OUT(x)	(DDRC |= (1 << x))	
	// ��������� ����� C �� ���� �� �����
	#define PORTC_IN(x)		(DDRC &= ~(1 << x))	
	// ����� � ���� C �� ����� 1
	#define PORTC_ON(x)		(PORTC |= (1 << x))
	// ����� � ���� C �� ����� 0
	#define PORTC_OFF(x)	(PORTC &= ~(1 << x))	
	// ������ �� ����� C �� �����
	#define PORTC_RD(x)		(PINC & (1 << x))	
	// �������������� ����� ����� C
	#define PORTC_XOR(x)	(PORTC ^= (1 << x))	
	// ����� �������� � ����
	#define PORTC_DATA_OUT(c)	PORTC = c	
	// ������ �� �����
	#define PORTC_DATA_IN()	PINC
#endif

//-------------------------------------------------------------------------
//  ���������� ������ D
//-------------------------------------------------------------------------

#ifdef	PORTD
	#define PORTD_OUT_ALL()	(DDRD = 0xFF);
	#define PORTD_IN_ALL()	(DDRD = 0x00);		
	// ��������� ����� D �� ����� �� �����
	#define PORTD_OUT(x)	(DDRD |= (1 << x))	
	// ��������� ����� D �� ���� �� �����
	#define PORTD_IN(x)		(DDRD &= ~(1 << x))	
	// ����� � ���� D �� ����� 1
	#define PORTD_ON(x)		(PORTD |= (1 << x))
	// ����� � ���� D �� ����� 0
	#define PORTD_OFF(x)	(PORTD &= ~(1 << x))	
	// ������ �� ����� D �� �����
	#define PORTD_RD(x)		(PIND & (1 << x))	
	// �������������� ����� ����� D
	#define PORTD_XOR(x)	(PORTD ^= (1 << x))	
	// ����� �������� � ����
	#define PORTD_DATA_OUT(d)	PORTD = d	
	// ������ �� �����
	#define PORTD_DATA_IN()	PIND
#endif

//-------------------------------------------------------------------------

#endif /* PORTS_H_ */