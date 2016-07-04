/* Fast DHT Lirary
 *
 * Copyright (C) 2015 Sergey Denisov.
 * Written by Sergey Denisov aka LittleBuster (DenisovS21@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version 3
 * of the Licence, or (at your option) any later version.
 *
 * Original library written by Adafruit Industries. MIT license.
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifndef __DHT22_H__
#define __DHT22_H__

#include <stdint.h>


#define DHT_COUNT 6
#define DHT_MAXTIMINGS 85


/*
 * Sensor's port
 */
#define DDR_DHT DDRD
#define PORT_DHT PORTD
#define PIN_DHT PIND


/**
 * Init dht sensor
 * @dht: sensor struct
 * @pin: PORT & DDR pin
 */
void dht_init(char pin);

/**
 * Reading temperature from sensor
 * @dht: sensor struct
 * @temp: out temperature pointer
 *
 * Returns 1 if succeful reading
 * Returns 0 if fail reading
 */
char dht_read_temp(char pin, float *temp);

/**
 * Reading humidity from sensor
 * @dht: sensor struct
 * @hum: out humidity pointer
 *
 * Returns 1 if succeful reading
 * Returns 0 if fail reading
 */
char dht_read_hum(char pin, float *hum);

/**
 * Reading temperature and humidity from sensor
 * @dht: sensor struct
 * @temp: out temperature pointer
 * @hum: out humidity pointer
 *
 * Returns 1 if succeful reading
 * Returns 0 if fail reading
 *
 * The fastest function for getting temperature + humidity.
 */
char dht_read_data(char pin, float *temp, float *hum);


void dht_init(char pin)
{
    /* Setup the pins! */
    DDR_DHT &= ~(1 << pin);
    PORT_DHT |= (1 << pin);
}

static char dht_read(char pin, char *data)
{
    char tmp;
    char sum = 0;
    char j = 0, i;
    char last_state = 1;
    uint16_t counter = 0;
    /*
     * Pull the pin 1 and wait 250 milliseconds
     */
    PORT_DHT |= (1 << pin);
    _delay_ms(250);

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    /* Now pull it low for ~20 milliseconds */
    DDR_DHT |= (1 << pin);
    PORT_DHT &= ~(1 << pin);
    _delay_ms(20);
    cli();
    PORT_DHT |= (1 << pin);
    _delay_us(40);
    DDR_DHT &= ~(1 << pin);

    /* Read the timings */
    for (i = 0; i < DHT_MAXTIMINGS; i++) {
        counter = 0;
        while (1) {
            tmp = ((PIN_DHT & (1 << pin)) >> 1);
            _delay_us(3);

            if (tmp != last_state)
                break;

            counter++;
            _delay_us(1);

            if (counter == 255)
                break;
        }

        last_state = ((PIN_DHT & (1 << pin)) >> 1);

        if (counter == 255)
            break;

        /* Ignore first 3 transitions */
        if ((i >= 4) && (i % 2 == 0)) {
            /* Shove each bit into the storage bytes */
            data[j/8] <<= 1;
            if (counter > DHT_COUNT)
                data[j/8] |= 1;
            j++;
        }
    }

    sei();
    sum = data[0] + data[1] + data[2] + data[3];

    if ((j >= 40))// && (data[4] == (sum & 0xFF)))
        return 1;
    return 0;
}

char dht_read_temp(char pin, float *temp)
{
char data[6];
    if (dht_read(pin, data)) {
        *temp = data[2] & 0x7F;
        *temp *= 256;
        *temp += data[3];
        *temp /= 10;

        if (data[2] & 0x80)
            *temp *= -1;
        return 1;
    }
    return 0;
}

char dht_read_hum(char pin, float *hum)
{
char data[6];
    if (dht_read(pin, data)) {
        *hum = data[0];
        *hum *= 256;
        *hum += data[1];
        *hum /= 10;
        if (*hum == 0.0f)
            return 0;
        return 1;
    }
    return 0;
}

char dht_read_data(char pin, float *temp, float *hum)
{
char data[6];
    if (dht_read(pin, data)) {
        /* Reading temperature */
        *temp = data[2] & 0x7F;
        *temp *= 256;
        *temp += data[3];
        *temp /= 10;

        if (data[2] & 0x80)
            *temp *= -1;

        /* Reading humidity */
        *hum = data[0];
        *hum *= 256;
        *hum += data[1];
        *hum /= 10;
        if (*hum == 0.0f)
            return 0;
        return 1;
    }
    return 0;
}
#endif