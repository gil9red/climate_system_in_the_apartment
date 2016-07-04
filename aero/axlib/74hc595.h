
//------------------------------------------<                         axlib v1.1                        >----------------------------------------------------
//------------------------------------------<  ����� ������� ��� ������ �� ��������� ��������� 74HC595  >----------------------------------------------------
//------------------------------------------<         �������� ������� 2015 http://www.avrki.ru         >----------------------------------------------------

#ifndef R_74HC595_H_
#define R_74HC595_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/74hc595.h>)."
#endif

//-------------------------------------------------------------------------
//							������������ ����������
//-------------------------------------------------------------------------

#include <avr/io.h>
#include <axlib/type_var.h>

// ������������� ��������
void reg_74hc595_init(void)
{
	R74HC595_DDR_RESET |= (1 << R74HC595_RESET);
	R74HC595_DDR_CLK |= (1 << R74HC595_CLK);
	R74HC595_DDR_SHIFT |= (1 << R74HC595_SHIFT);
	R74HC595_DDR_DATA |= (1 << R74HC595_DATA); 
	R74HC595_PORT_CLK &= ~(1 << R74HC595_CLK);
	R74HC595_PORT_SHIFT &= ~(1 << R74HC595_SHIFT);
	R74HC595_PORT_RESET |= (1 << R74HC595_RESET);
}

// ������� ������ ��������
void reg_74hc595_reset(void)
{
	R74HC595_PORT_RESET &= ~(1 << R74HC595_RESET);
	R74HC595_PORT_SHIFT |= (1 << R74HC595_SHIFT);
	R74HC595_PORT_SHIFT &= ~(1 << R74HC595_SHIFT);
	R74HC595_PORT_RESET |= (1 << R74HC595_RESET);
}

// ������� ������ ����� ����� �������
void reg_74hc595_byte(BYTE data)
{
	//reg_74hc595_reset();	// �������� ����������� ��������
	
	for(BYTE i=0; i<8; i++)
	{
		if((data << i) & 0x80) // ���������� �� �������, ������� �� ��������, �������
		{
			R74HC595_PORT_DATA |= (1 << R74HC595_DATA);	// ���� 1
		}
		else
		{
			R74HC595_PORT_DATA &= ~(1 << R74HC595_DATA);	// ���� 0
		}
		R74HC595_PORT_CLK |= (1 << R74HC595_CLK);	// �������������
		R74HC595_PORT_CLK &= ~(1 << R74HC595_CLK);
	}
	R74HC595_PORT_SHIFT |= (1 << R74HC595_SHIFT);	// �������
	R74HC595_PORT_SHIFT &= ~(1 << R74HC595_SHIFT);
}

#endif /* R_74HC595_H_ */