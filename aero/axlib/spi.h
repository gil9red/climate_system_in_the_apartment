
//-------------------------------------------------<                   axlib v1.1                    >----------------------------------------------------
//-------------------------------------------------<       ���������� ��� ������ � ����� SPI         >----------------------------------------------------
//-------------------------------------------------<    �������� ������� 2015 http://www.avrki.ru    >----------------------------------------------------


#ifndef SPI_H_
#define SPI_H_

#if !defined(MAIN_INIT_H_)
	#error "You must included (#include \"main_init.h\") befor use (#include <axlib/spi.h>)."
#endif

#include <axlib/type_var.h>

#ifdef SPDR

// �������� SPI
#define SPI_ON			0x40

// ��������� SPI
#define SPI_OFF			0

// ����� ������
#define SPI_MASTER		0x10

// ����� �����
#define SPI_SLAVE		0

// ����������� �������� (������� ������ ������)
#define SPI_DIRECT_LSB	0x20

// ����������� �������� (������� ������ ������)
#define SPI_DIRECT_MSB	0

// ��������� ����������
#define SPI_INT_ON		0x80

// ��������� ����������
#define SPI_INT_OFF		0

// ������������� �������� ������
#define SPI_CPOL_LOW	0

// ������������� �������� ������ 
#define SPI_CPOL_HIGHT	0x08

// ����������� ����� ������� 
#define SPI_CPHA_1EDGE	0
// ������������� �������� ������ 
#define SPI_CPHA_2EDGE	0x04

// ������������ ������� (��������� �������� �������� ��� �������)
#define SPI_PRESCALER_2		0
#define SPI_PRESCALER_4		1
#define SPI_PRESCALER_8		2
#define SPI_PRESCALER_16	3
#define SPI_PRESCALER_32	4
#define SPI_PRESCALER_64	5
#define SPI_PRESCALER_128	6
#define SPI_PRESCALER_NO	7

#ifdef _AVR_ATMEGA8A_H_INCLUDED // ���� MEGA8

	#define SPI_M_PORT	(DDRB |= (1 << 3) | (1 << 5) | (1 << 2))
	#define SPI_M_SS	(PORTB &= ~(1 << 2))
	#define SPI_S_PORT	(DDRB |= (1 << 4))
	#define SPI_SS	(PINB & (1 << 2))
	
#else // ��� ������ ��

	#define SPI_M_PORT	DDRB |= ((1 << 5) | (1 << 7) | (1 << 4))
	#define SPI_M_SS	(PORTB &= ~(1 << 4))
	#define SPI_S_PORT	(DDRB |= (1 << 6))
	#define SPI_SS	(PINB & (1 << 4))

#endif

// ���� ������� ������ � ������ Slave
#define FLAG_DATA	0
#define FLAG_OUT	1

// ���������� ��������
typedef struct
{
	BYTE SPI_set;			// ��������/��������� SPI
	BYTE SPI_Mode;			// ����� ������ ��� �����
	BYTE SPI_Direct;		// ����� ����������� �������� �����
	BYTE SPI_Prescaler;		// ����� ������������
	BYTE SPI_Polaric;		// ����� ���������� ��������� �������
	BYTE SPI_Phase;			// ����� ���� ��������� �������
	
}SPI_InitTypeDef;

// ���������� �������

// ������������� SPI
void SPI_init(SPI_InitTypeDef *spi);

// ������ ����� � �������� �� ���� ��� ������ ����������
BYTE SPI_M_byte_io(BYTE data);

// ������ ����� �� ���� ��� ����� ����������
BYTE SPI_S_byte_read(BYTE timeout);

// ������ ����� � ���� ��� ����� ����������
BYTE SPI_S_byte_write(BYTE data, BYTE timeout);


// �������

//-------------------------------------------------------------------------------------------------------

// ������� ������������� SPI ��������� � �������� ��������� ��������� �� ���������

void SPI_init(SPI_InitTypeDef *spi)
{	
	if(spi == NULL)
		{
			return;
		}
	
	if(spi->SPI_Mode == SPI_MASTER)
		{
			SPI_M_PORT;
			SPI_M_SS;
			SPCR = (spi->SPI_set | spi->SPI_Mode | spi->SPI_Direct | spi->SPI_Polaric | spi->SPI_Phase);
		}
	else
		{
			SPI_S_PORT;
			DDR_S_OUT |= (1 << S_OUT);
			SPCR = (spi->SPI_set | spi->SPI_Mode | spi->SPI_Direct | spi->SPI_Polaric | spi->SPI_Phase);
			
		}
	
	switch (spi->SPI_Prescaler)
		{
			case 0: // 2
				SPSR |= (1 << SPI2X);
				SPCR &= ~(1 << SPR0) | ~(1 << SPR1);
				break;
				
			case 1: // 4
				SPCR &= ~(1 << SPR0) | ~(1 << SPR1);
				break;
				
			case 2: // 8
				SPSR |= (1 << SPI2X);
				SPCR |= (1 << SPR0);
				SPCR &= ~(1 << SPR1);
				break;
				
			case 3: // 16
				SPCR |= (1 << SPR0);
				SPCR &= ~(1 << SPR1);
				break;
				
			case 4: // 32
				SPSR |= (1 << SPI2X);
				SPCR &= ~(1 << SPR0);
				SPCR |= (1 << SPR1);
				break;
				
			case 5: // 64
				SPCR &= ~(1 << SPR0);
				SPCR |= (1 << SPR1);
				break;
				
			case 6: //128
				SPCR |= 0x03;
				break;
			
			case 7: //SLAVE
				SPCR &= 0xFC;
				break;
		}
}

//-------------------------------------------------------------------------------------------------------

// ������ ����� � �������� �� ���� ��� ������ ����������
BYTE SPI_M_byte_io(BYTE data)
{
	BYTE answer = 0;
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));	
	answer = SPDR;
	return answer;
}

//-------------------------------------------------------------------------------------------------------

// ������ ����� �� ���� ��� ����� ����������
BYTE SPI_S_byte_read(BYTE timeout)
{
	BYTE answer = 0;
	while(!(SPSR & (1<<SPIF)))
		{
			if(!timeout)
				{
					return FULL;
				}
			_delay_ms(1);
			timeout--;
		}
	answer = SPDR;
	return answer;
}

// ������ ����� � ���� ��� ����� ����������
BYTE SPI_S_byte_write(BYTE data, BYTE timeout)
{
	while(!(SPSR & (1<<SPIF)));
		{
			if(!timeout)
			{
				return FULL;
			}
			_delay_ms(1);
			timeout--;
		}
	SPDR = data;
	PORT_S_OUT |= (1 << S_OUT);
	
	return NULL;
}
#else
	#error "This is not the microcontroller SPI"
#endif // SPDR

#endif /* SPI_H_ */