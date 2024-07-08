#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "DHT.h"
#include <BH1750.h>
#include <Wire.h>

#define LED_PIN 2
#define DHTPIN 14
#define DHTTYPE DHT11
#define LUX_THRESHOLD 1000

BH1750 lightMeter(0x23);
DHT dht(DHTPIN, DHTTYPE);

float lux = 566;
float hum;
float temp;

void read_data(void);
void sensor_init(void);

#endif
