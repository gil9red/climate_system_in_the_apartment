
//---------------------------------------<                             axlib v1.1                                >----------------------------------------------------
//---------------------------------------<        ���������� ��� ������ � ������ ��������� ������� DS3231        >----------------------------------------------------
//---------------------------------------<              �������� ������� 2015 http://www.avrki.ru                >----------------------------------------------------

#ifndef DS3231_H_
#define DS3231_H_

#if !defined(MAIN_INIT_H_)
#error "You must included (#include \"main_init.h\") befor use (#include <axlib/ds3231.h>)."
#endif


//-------------------------------------------------------------------------
//							������������ ����������
//-------------------------------------------------------------------------
#include <axlib/type_var.h>
#include <axlib/i2c.h>
//-------------------------------------------------------------------------
//						���������� ��������� �����������
//-------------------------------------------------------------------------

// ����� ����������

#define DS3231_ADD_W				0xD0
#define DS3231_ADD_R				0xD1

// ������ ���������

#define DS3231_SECONDS				0x00	// ����� �������� ������
#define DS3231_MINUTES				0x01	// ����� �������� �����
#define DS3231_HOURS				0x02	// ����� �������� �����
#define DS3231_DAY					0x03	// ����� �������� ��� ������
#define DS3231_DATE					0x04	// ����� �������� �����
#define DS3231_MONTH				0x05	// ����� �������� ������
#define DS3231_YEAR					0x06	// ����� �������� ����
#define DS3231_ALARM_1_SEC			0x07	// ����� �������� ���������� 1 ������
#define DS3231_ALARM_1_MIN			0x08	// ����� �������� ���������� 1 �����
#define DS3231_ALARM_1_HOR			0x09	// ����� �������� ���������� 1 �����
#define DS3231_ALARM_1_DAY			0x0A	// ����� �������� ���������� 1 ��� ������
#define DS3231_ALARM_1_DAT			0x0A	// ����� �������� ���������� 1 �����
#define DS3231_ALARM_2_MIN			0x0B	// ����� �������� ���������� 2 �����
#define DS3231_ALARM_2_HOR			0x0C	// ����� �������� ���������� 2 �����
#define DS3231_ALARM_2_DAY			0x0D	// ����� �������� ���������� 2 ��� ������
#define DS3231_ALARM_2_DAT			0x0D	// ����� �������� ���������� 2 �����
#define DS3231_CONTROL				0x0E	// ����� �������� ����������
#define DS3231_STATUS				0x0F	// ����� �������� ���������
#define DS3231_SET_CLOCK			0x10	// ����� �������� ������������� ������� ���������� ������� (0x81 - 0x7F)
#define DS3231_T_MSB				0x11	// ����� �������� ��������� ���������� ����������� �������� ����
#define DS3231_T_LSB				0x12	// ����� �������� ��������� ���������� ����������� �������� ����

// ����� ������ ����� 

#define DS3231_HOURS_12				0x40
#define DS3231_24					0x3F
#define DS3231_AM					0x5F
#define DS3231_PM					0x60
#define DS3231_GET_24				0x02
#define DS3231_GET_PM				0x01
#define DS3231_GET_AM				0x00

// ������� ���������

#define DS3231_A1M1					0x07
#define DS3231_A1M2					0x07
#define DS3231_A1M3					0x07
#define DS3231_A1M4					0x07
#define DS3231_A2M2					0x07
#define DS3231_A2M3					0x07
#define DS3231_A2M4					0x07
#define DS3231_DY_DT				0x06
#define DS3231_EOSC					0x07
#define DS3231_BBSQW				0x06
#define DS3231_CONV					0x05
#define DS3231_INTCN				0x02
#define DS3231_A2IE					0x01
#define DS3231_A1IE					0x00
#define DS3231_OSF					0x07
#define DS3231_EN32KHZ				0x03
#define DS3231_BSY					0x02
#define DS3231_A2F					0x01
#define DS3231_A1F					0x00
#define DS3231_SIGN					0x07

// ����� ������� ��������� ������� INT/SQW

#define DS3231_SQW_OUT_1HZ			0x00	// 1 ��
#define DS3231_SQW_OUT_1KHZ			0x08	// 1.024 ���
#define DS3231_SQW_OUT_4KHZ			0x10	// 4.096 ���
#define DS3231_SQW_OUT_8KHZ			0x18	// 8.192 ���

// ���������� ������� ��� ���������� SQW/OUT

#define DS3231_OUT_HIGHT			0x80
#define DS3231_OUT_LOW				0x00

// ��������������� ������ �������

#define DS3231_ERR					FALSE
#define DS3231_OK					TRUE
#define DS3231_ON					TRUE
#define DS3231_OFF					FALSE

// ����������

#define DS3231_ALARM_1				0x00
#define DS3231_ALARM_2				0x01
#define DS3231_ALARM_OFF			0x00
#define DS3231_ALARM_1_ON			0x01
#define DS3231_ALARM_2_ON			0x02
#define DS3231_ALARM_ALL_ON			0x03

//-------------------------------------------------------------------------
//	������� ������������� ����� ��������� ������ DS3231.
//
// ��������� �������� ������ ������ ����� 12 ��� 24
//-------------------------------------------------------------------------

void ds3231_init(void)
{
	BYTE temp = 0;
	i2c_init();
	
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(DS3231_CONTROL);
	i2c_restart();
	i2c_send_byte(DS3231_ADD_R);
	temp = i2c_read_byte(NACK);
	i2c_stop();
	
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(DS3231_CONTROL);
	i2c_restart();
	i2c_send_byte(temp & 0x7F);
	i2c_stop();
}


//-------------------------------------------------------------------------
//
//	������� ������ ����� � �������
//
//	��������� ���������:
//
//		BYTE reg // ����� ��������
//		BYTE data // ���� ��� ������
//-------------------------------------------------------------------------

void ds3231_write_reg(BYTE reg, BYTE data)
{
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(reg);
	i2c_send_byte(data);
	i2c_stop();
}

//-------------------------------------------------------------------------
//
//	������� ������ ����� �� ��������
//
//	��������� ���������:
//
//		BYTE reg // ����� ��������
//
//	���������� ����������� ����
//-------------------------------------------------------------------------

BYTE ds3231_read_reg(BYTE reg)
{
	BYTE temp = 0;
	
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(reg);
	i2c_restart();
	i2c_send_byte(DS3231_ADD_R);
	temp = i2c_read_byte(NACK);
	i2c_stop();
	
	return temp;
}

//-------------------------------------------------------------------------
//	������� ���������� �������� ������
//
//
//	��������� � �������� ��������� ���������� ��������
//
//	������������ ��������
//
//		��������������� ��� ������ � ����
//
//-------------------------------------------------------------------------

BYTE ds3231_byte(BYTE data)
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
//	������� ������ �������
//
//
//	��������� �������� ��������� �� ������ ������� �������.
//
//	����� ������ ������ ������� � ������� ����� ������ ����������� �����.
//	� ������ �������� ����, �� ������ ������, � ������� �������.
//
//	������������ ��������:
//
//	����� ����� �����, ���� DS3231_GET_24, ���� DS3231_GET_AM, ���� DS3231_GET_PM	
//						
//-------------------------------------------------------------------------

BYTE ds3231_read_time(BYTE *str)
{	
	BYTE temp[3];
	UBYTE i = 0;
	BYTE hour = 0;
	
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(DS3231_SECONDS);
	i2c_restart();
	i2c_send_byte(DS3231_ADD_R);
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
			hour = DS3231_GET_24;
		}
	return hour;
}

//-------------------------------------------------------------------------
//	������� ������ �������
//
//	��������� ���������:
//
//	��� ����� ����� (DS3231_24, ���� DS3231_AM, ���� DS3231_PM) ����, ������, �������.
//
//-------------------------------------------------------------------------

void ds3231_write_time(BYTE h1224, BYTE hours, BYTE minutes, BYTE seconds)
{
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(DS3231_SECONDS);
	i2c_send_byte(ds3231_byte(seconds));
	i2c_send_byte(ds3231_byte(minutes));
	
	if(h1224 == DS3231_24)
		{
			i2c_send_byte(ds3231_byte(hours) & DS3231_24);
		}
	else
		{
			if(h1224 == DS3231_AM)
				{
					if(hours > 12) hours -= 12;
					i2c_send_byte((ds3231_byte(hours) & DS3231_AM) | DS3231_HOURS_12);
				}
			else
				{
					if(hours > 12) hours -= 12;
					i2c_send_byte(ds3231_byte(hours)| DS3231_PM);
				}
		}
	
	i2c_stop();
}

//-------------------------------------------------------------------------
//	������� ������ ����
//
//
//	��������� �������� ��������� �� ������ ������� �������.
//
//	����� ������ ������ ������� � ������� ����� ������ ����������� �����.
//	� ������ �������� ���� ������, �� ������ �����, � ������� �����, � ��������� ��� 00-99.
//
//
//-------------------------------------------------------------------------

void ds3231_read_data(BYTE *str)
{
	BYTE temp[3];
	UBYTE i = 0;
	
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(DS3231_DAY);
	i2c_restart();
	i2c_send_byte(DS3231_ADD_R);
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
//	������� ������ ����
//
//	��������� ��������� ���� ������(1-7), �����(1-31), �����(1-12), ���(00-99).
//
//-------------------------------------------------------------------------

void ds3231_write_data(BYTE data, BYTE day, BYTE month, BYTE year)
{
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(DS3231_DAY);
	i2c_send_byte(data);
	i2c_send_byte(ds3231_byte(day));
	i2c_send_byte(ds3231_byte(month));
	i2c_send_byte(ds3231_byte(year));
	i2c_stop();
}

//-------------------------------------------------------------------------
//	������� ���������� ������� INT/SQW
//
//	��������� �������� �������� �������.
//
//			DS3231_SQW_OUT_1HZ	    1 ��
//			DS3231_SQW_OUT_1KHZ 	1 ���
//			DS3231_SQW_OUT_4KHZ	    4.096 ���
//			DS3231_SQW_OUT_8KHZ	    8.193 ���
//-------------------------------------------------------------------------

void ds3231_sqw_on(BYTE rs)
{
	BYTE temp = 0;
	
	temp = ds3231_read_reg(DS3231_CONTROL);
	temp &= 0xA0;
	temp |= ((1 << DS3231_BBSQW) | rs);
	
	ds3231_write_reg(DS3231_CONTROL, temp);
}

//-------------------------------------------------------------------------
//	������� ���������/���������� ������� 32��� �� ������ EN32KHZ
//
//	��������� � �������� ���������:
//
//			DS3231_ON	��������
//			DS3231_OFF	���������
//
//-------------------------------------------------------------------------

void ds3231_en32khz(BYTE level)
{
	BYTE temp = 0;
	
	temp = ds3231_read_reg(DS3231_STATUS);
	
	if(DS3231_ON == level)
		{
			temp |= (1 << DS3231_EN32KHZ);
		}
	else
		{
			temp &= ~(1 << DS3231_EN32KHZ);
		}
	
	ds3231_write_reg(DS3231_STATUS, temp);
}

//-------------------------------------------------------------------------
//	������� ���������� ���������� �����������
//
//-------------------------------------------------------------------------

SBYTE ds3231_read_temp(void)
{
	BYTE temp = 1;
	
	// �������� ������ BSY
	while(temp) 
	{
		temp = ds3231_read_reg(DS3231_STATUS);		
		temp &= (1 << DS3231_BSY);
	}
	
	// ������ �������� CONTROL � ��������� ���� CONV
	temp = ds3231_read_reg(DS3231_CONTROL);
	temp |= (1 << DS3231_CONV);
	ds3231_write_reg(DS3231_CONTROL, temp);
	
	// ������ �����������
	return (SBYTE)ds3231_read_reg(DS3231_T_MSB);
}

//-------------------------------------------------------------------------
//	������� ��������� �����������
//
//	��������� ���������:
//
//		BYTE alarm	 // ����� ����������, ���� DS3231_ALARM_1, ���� DS3231_ALARM_2
//		BYTE h1224	 // ����� ������ �����, ���� DS3231_24, ���� DS3231_AM, ���� DS3231_PM
//		BYTE hours	 // ����
//		BYTE minutes // ������
//-------------------------------------------------------------------------

void ds3231_set_alarm(BYTE alarm, BYTE h1224, BYTE hours, BYTE minutes)
{
	BYTE control_reg = 0;
	BYTE ststus_reg = 0;
	BYTE alarm_reg = 0;
	
	// ���������� SQW, ���������� ���������� ���������� � ����� ��������� ������ ���������.
	
	control_reg = ds3231_read_reg(DS3231_CONTROL);	
	control_reg |= (1 << alarm) | (1 << DS3231_INTCN) | (1 << DS3231_BBSQW);	
	ds3231_write_reg(DS3231_CONTROL, control_reg);
	
	ds3231_write_reg(DS3231_ALARM_1_SEC, 0x00);
	
	// ������ ������� � ���������
	if(alarm == DS3231_ALARM_1) alarm_reg = DS3231_ALARM_1_MIN;
	if(alarm == DS3231_ALARM_2) alarm_reg = DS3231_ALARM_2_MIN;
	i2c_start();
	i2c_send_byte(DS3231_ADD_W);
	i2c_send_byte(alarm_reg);
	i2c_send_byte(ds3231_byte(minutes));

	if(h1224 == DS3231_24)
	{
		i2c_send_byte((ds3231_byte(hours) & DS3231_24) | (1 << DS3231_A1M3));
	}
	else
	{
		if(h1224 == DS3231_AM)
		{
			if(hours > 12) hours -= 12;
			i2c_send_byte((ds3231_byte(hours) & DS3231_AM) | DS3231_HOURS_12 | (1 << DS3231_A1M3));
		}
		else
		{
			if(hours > 12) hours -= 12;
			i2c_send_byte((ds3231_byte(hours)| DS3231_PM) | (1 << DS3231_A1M3));
		}
	}	
	
	i2c_send_byte(ds3231_byte(0x80));
	i2c_stop();
	
	ststus_reg = ds3231_read_reg(DS3231_STATUS);
	ststus_reg &= 0xFC;
	ds3231_write_reg(DS3231_STATUS, ststus_reg);
}

//-------------------------------------------------------------------------
//	������� ���������� ���������� ( �������� ������ ����� ������������� ����������� )
//
//	��������� ���������:
//
//		BYTE alarm	// ����� ����������, ���� DS3231_ALARM_1, ���� DS3231_ALARM_2
//-------------------------------------------------------------------------

void ds3231_stop_alarm(BYTE alarm)
{
	BYTE temp = 0;
	
	temp = ds3231_read_reg(DS3231_CONTROL);
	temp &= ~(1 << alarm);
	ds3231_write_reg(DS3231_CONTROL, temp);
}

//-------------------------------------------------------------------------
//	������� ��������� ���������� ( �������� ������ ����� ������������� ����������� )
//
//	��������� ���������:
//
//		BYTE alarm	// ����� ����������, ���� DS3231_ALARM_1, ���� DS3231_ALARM_2
//-------------------------------------------------------------------------

void ds3231_start_alarm(BYTE alarm)
{
	BYTE temp = 0;
	
	temp = ds3231_read_reg(DS3231_CONTROL);
	temp |= (1 << alarm);
	ds3231_write_reg(DS3231_CONTROL, temp);
}

//-------------------------------------------------------------------------
//	������� ��������� ������ ������������ ����������
//
//	������������ ��������:
//
//		DS3231_ALARM_OFF	//  �� ���� �� ��������
//		DS3231_ALARM_1_ON	//	�������� ��������� 1
//		DS3231_ALARM_2_ON	//	�������� ��������� 2
//		DS3231_ALARM_ALL_ON	//	��������� ��� ����������
//-------------------------------------------------------------------------

BYTE ds3231_get_alarm(void)
{
	BYTE alarm = 0;
	BYTE temp = 0;
	
	alarm = ds3231_read_reg(DS3231_STATUS);
	temp = (alarm & ~(alarm & 0x03));
	ds3231_write_reg(DS3231_STATUS, temp);
	
	return (alarm & 0x03);
}

#endif /* DS3231_H_ */