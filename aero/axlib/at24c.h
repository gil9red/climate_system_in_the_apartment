
//-------------------------------------------------<                axlib v1.1                 >----------------------------------------------------
//-------------------------------------------------<   ���������� ��� ������ � EEPROM AT24Cx   >----------------------------------------------------
//-------------------------------------------------< �������� ������� 2015 http://www.avrki.ru >----------------------------------------------------

#ifndef AT24C_H_
#define AT24C_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/at24c.h>)."
#endif

//-------------------------------------------------------------------------
//							������������ ����������
//-------------------------------------------------------------------------
#include <avr/io.h>
#include <util/delay.h>
#include <axlib/type_var.h>
#include <axlib/i2c.h>


//-------------------------------------------------------------------------
//							���������� �������
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//							������������� ����������
//
//	����������� ���������
//
//		BYTE add		����� ����������  �� 0x50 �� 0x57
//
//	������������ ��������
//
//		ACK		��� ������� �������������
//		NACK	��� ��������� �������������
//-------------------------------------------------------------------------

BYTE at24c_init(BYTE add)
{
	BYTE eeprom_add = ((add << 1) & 0xFE);
	BYTE answer = NACK;
	
	i2c_start();
	// ��������  ������ ����������
	answer = i2c_send_byte(eeprom_add);
	if(answer == NACK) return answer;
	i2c_stop();
	
	return answer;
}

//-------------------------------------------------------------------------
//							������� ������ ����� � ������
//
//	����������� ���������
//
//		UBYTE add		����� ����������  �� 0x50 �� 0x57
//		UWORD addbyte	����� ����� �� 0 �� ����.
//		BYTE data		���� ������
//
//	������������ ��������
//
//		ACK		��� ������� ������ ������
//		NACK	��� ��������� ������ ������
//-------------------------------------------------------------------------


BYTE at24c_write_byte(UBYTE add, UWORD addpage, BYTE data)
{
	BYTE eeprom_add = ((add << 1) & 0xFE);
	BYTE byte_add_H = (addpage >> 8) & 0xFF;
	BYTE byte_add_L = (addpage & 0xFF);
	BYTE answer = NACK;
	
	i2c_start();
	
	// ��������  ������ ����������
	answer = i2c_send_byte(eeprom_add);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	// ������ ������ � ������ ����������
	answer = i2c_send_byte(data);
	if(answer == NACK) return answer;
	
	i2c_stop();
	_delay_ms(5);
	
	return answer;
}

//-------------------------------------------------------------------------
//							������� ������ �������� � ������
//
//	����������� ���������
//
//		UBYTE add		����� ����������  �� 0x50 �� 0x57
//		UWORD addbyte	����� ���������� ����� �������� �� 0 �� ����.
//		BYTE *data		��������� �� ������ ������� �������
//		UBYTE count		���������� ���� � ��������
//
//	������������ ��������
//
//		ACK		��� ������� ������ ������
//		NACK	��� ��������� ������ ������
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
	
	// ��������  ������ ����������
	answer = i2c_send_byte(eeprom_add);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	// ������ ������ � ������ ����������
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
//							������� ������ ������� � ������
//
//	����������� ���������
//
//		UBYTE add		����� ����������  �� 0x50 �� 0x57
//		UWORD addbyte	����� ���������� ����� ��� ������ ������� � ������
//		BYTE *data		��������� �� ������ ������� �������
//		UBYTE count		���������� ���� ��� ������
//
//	������������ ��������
//
//		ACK		��� ������� ������ ������
//		NACK    ��� ��������� ������ ������
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
//							������� ������ ����� �� ������
//
//	����������� ���������
//
//		UBYTE add		����� ����������  �� 0x50 �� 0x57
//		UWORD addbyte	����� ���������� ����� ��� ������ ������� �� ������
//
//	������������ ��������
//
//				��� ������� ������ ������� ������������ �����
//		NACK	��� ��������� ������ �����
//-------------------------------------------------------------------------

BYTE at24c_read_byte(UBYTE add, UWORD addbyte)
{	
	BYTE eeprom_add_w = ((add << 1) & 0xFE);
	BYTE eeprom_add_r = (eeprom_add_w | 0x01);
	BYTE byte_add_H = (addbyte >> 8) & 0xFF;
	BYTE byte_add_L = (addbyte & 0xFF);
	BYTE answer = NACK;
	
	i2c_start();
	
	// ��������  ������ ����������
	answer = i2c_send_byte(eeprom_add_w);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	i2c_restart();
	
	// ��������  ������ ����������
	answer = i2c_send_byte(eeprom_add_r);
	if(answer == NACK) return answer;
	
	// ��������� �����
	answer = i2c_read_byte(NACK);
	
	i2c_stop();
	_delay_ms(5);
	
	return answer;
}

//-------------------------------------------------------------------------
//							������� ������ ������� �� ������
//
//	����������� ���������
//
//		UBYTE add		����� ����������  �� 0x50 �� 0x57
//		UWORD addbyte	����� ���������� ����� ��� ������ ������� �� ������
//		BYTE *data		��������� �� ������ ������� �������
//		UBYTE count		���������� ���� ��� ������
//
//	������������ ��������
//
//		ACK		��� ������� ������ ������
//		NACK	��� ��������� ������ ������
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
	
	// ��������  ������ ����������
	answer = i2c_send_byte(eeprom_add_w);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_H);
	if(answer == NACK) return answer;
	// �������� �������� ����� ������ ���������� ����� ������
	answer = i2c_send_byte(byte_add_L);
	if(answer == NACK) return answer;
	
	i2c_restart();
	
	// ��������  ������ ����������
	answer = i2c_send_byte(eeprom_add_r);
	if(answer == NACK) return answer;
	
	count--;
	
	while( i < count)
	{
		*data = i2c_read_byte(ACK);
		data++;
		i++;
	}
	// ��������� ���������� �����
	*data = i2c_read_byte(NACK);
	
	i2c_stop();
	_delay_ms(5);
	
	return answer;
}

#endif /* AT24C_H_ */