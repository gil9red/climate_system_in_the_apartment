
//-------------------------------------------------<                axlib v1.1                 >----------------------------------------------------
//-------------------------------------------------<     ���������� ��� ������ � ���������     >----------------------------------------------------
//-------------------------------------------------< �������� ������� 2015 http://www.avrki.ru >----------------------------------------------------


#ifndef TIMERS_H_
#define TIMERS_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/timers.h>)."
#endif

//-------------------------------------------------------------------------
//							������������ ����������
//-------------------------------------------------------------------------

#include <avr/io.h>
#include <axlib/type_var.h>
#include <avr/interrupt.h>

//-------------------------------------------------------------------------
//						���������� ��������� �����������
//-------------------------------------------------------------------------

#define TIME_F_CPU 	(0.001/(1/F_CPU))	// ������� ����� �� 1 �� ��� ������� ������
#define F_CPU_8		(F_CPU/8)			// ������������ �� 8
#define F_CPU_64	(F_CPU/64)			// ������������ �� 64

#define TIMER_0		0
#define TIMER_1		1
#define TIMER_2		2
#define TIMER_3		3
#define TIMER_4		4
#define TIMER_5		5
#define TIMER_6		6
#define TIMER_7		7

#define OFF			0
#define ON			1

#define T0_START	((TFLAG >> TIMER_0) & 0x01)
#define T1_START	((TFLAG >> TIMER_1) & 0x01)
#define T2_START	((TFLAG >> TIMER_2) & 0x01)
#define T3_START	((TFLAG >> TIMER_3) & 0x01)
#define T4_START	((TFLAG >> TIMER_4) & 0x01)
#define T5_START	((TFLAG >> TIMER_5) & 0x01)
#define T6_START	((TFLAG >> TIMER_6) & 0x01)
#define T7_START	((TFLAG >> TIMER_7) & 0x01)

#define T0_STOP		TFLAG &= ~(1 << TIMER_0)
#define T1_STOP		TFLAG &= ~(1 << TIMER_1)
#define T2_STOP		TFLAG &= ~(1 << TIMER_2)
#define T3_STOP		TFLAG &= ~(1 << TIMER_3)
#define T4_STOP		TFLAG &= ~(1 << TIMER_4)
#define T5_STOP		TFLAG &= ~(1 << TIMER_5)
#define T6_STOP		TFLAG &= ~(1 << TIMER_6)
#define T7_STOP		TFLAG &= ~(1 << TIMER_7)

//-------------------------------------------------------------------------
//							������������� ����������
//-------------------------------------------------------------------------

volatile BYTE	TFLAG = 0x00;		// ���� ����������� �������
volatile BYTE	TONOFF = 0x00;		// ���� ����������� �������
volatile WORD	TIC[8];				// ������� �����������
volatile WORD	TIC_DAT[8];			// ������� �����������

//-------------------------------------------------------------------------
//			������� ���������� TIMER2 �� ���������� ��������
//-------------------------------------------------------------------------

ISR(TIMER2_COMP_vect)
{
	for(BYTE i=0; i<8; i++)		// ���������� ���� ��������� �� �������.
		{
			if(((TONOFF >> i) & 0x01) & (!((TFLAG >> i) & 0x01))) 
				{
					TIC[i]++; // ���������� ���� ������� �������
					
					if((TIC[i] >= TIC_DAT[i]))
						{
							TFLAG |= (1 << i);
							TIC[i] = 0x00;
						}
				}
		}	
	TCNT2=0x00;
}

//-------------------------------------------------------------------------
//				   ������� ������������� �������
//-------------------------------------------------------------------------

void timers_init(void)
{
	if(TIME_F_CPU > 256)
		{
			if((TIME_F_CPU/8) > 256)
				{
					TCCR2 |= (1 << CS21)|(1 << CS20);		// ������������
					OCR2 = (BYTE)(0.001/(1/F_CPU_64));		// �������� ���������
				}
			else
				{
					TCCR2 |= (1 << CS21);					// ������������
					OCR2 = (BYTE)(0.001/(1/F_CPU_64));		// �������� ���������
				}
		}
	else
		{
			TCCR2 |= (1 << CS20);					// ������������
			OCR2 = (BYTE)(0.001/(1/F_CPU));			// �������� ���������
		}
		
	TCNT2 = 0x00;				// �������
	TIMSK |= (1 << OCIE2);		// ���������� ���������� �2 �� ����������    
	
	sei(); 
}

//-------------------------------------------------------------------------
//				   ������� ��������� ��������
//
//	������������ ���������
//
//				BYTE TIMERn - ����� ������� � ��������� �� TIMER_0 �� TIMER_7
//				WORD time   - ����� ������� ������ �������. �������� � �������������
//				BYTE level	- ���������/���������� ������� ON/OFF
//-------------------------------------------------------------------------

void timer(BYTE TIMERn, WORD time, BYTE level)
{
	if(level == ON)
		{
			TONOFF |= (1 << TIMERn);
			TIC_DAT[TIMERn] = time;
			TIC[TIMERn] = 0x00;
		}
	else
		{
			TONOFF &= ~(1 << TIMERn);
			TIC_DAT[TIMERn] = 0x00;
			TIC[TIMERn] = 0x00;
		}
	
}


#endif /* TIMERS_H_ */