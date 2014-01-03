/***********************************
This is the Adafruit GPS library - the ultimate GPS library
for the ultimate GPS module!

Tested and works great with the Adafruit Ultimate GPS module
using MTK33x9 chipset
    ------> http://www.adafruit.com/products/746
Pick one up today at the Adafruit electronics shop
and help support open source hardware & software! -ada

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/
#ifndef _ADAFRUIT_GPS_H
#define _ADAFRUIT_GPS_H

#if ARDUINO >= 100
#include <SoftwareSerial.h>
#else
#include <NewSoftSerial.h>
#endif


const unsigned char PROGMEM PMTK_SET_NMEA_OUTPUT_RMCGGA[] = "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28";
const unsigned char PROGMEM PMTK_SET_NMEA_UPDATE_1HZ[] = "$PMTK300,1000,0,0,0,0*1C";

#if 0
const unsigned char PROGMEM psrf_cmd[]  = "$PSRF103,00,00,01,01*xx";

#define GPSGGA '0'
#define GPSGLL '1'
#define GPSGSA '2'
#define GPSGSV '3'
#define GPSRMC '4'
#define GPSVTG '5'

#define cmd_pos  10
#define period_pos 16
#define chksum_pos 21
#endif

// how long to wait when we're looking for a response
#define MAXWAITSENTENCE 5

#if ARDUINO >= 100
#include "Arduino.h"
#if !defined(__AVR_ATmega32U4__)
#include "SoftwareSerial.h"
#endif
#else
#include "WProgram.h"
#include "NewSoftSerial.h"
#endif


class Adafruit_GPS
{
public:
    void begin(uint16_t baud);

    Adafruit_GPS(SoftwareSerial *ser); // Constructor when using SoftwareSerial

    char *lastNMEA(void);
    boolean newNMEAreceived();
    void common_init(void);
    //void sendCommand(char cmd, char time);
    void sendCommand(char *string);
    void pause(boolean b);

    boolean parseNMEA(char *response);
    uint8_t parseHex(char c);

    char read(void);
    boolean parse(char *);
    void interruptReads(boolean r);

    boolean wakeup(void);
    boolean standby(void);

    uint8_t hour, minute, seconds, year, month, day;
    uint16_t milliseconds;
    float latitude, longitude, geoidheight, altitude;
    float speed, angle, magvariation, HDOP;
    char lat, lon, mag;
    boolean fix;
    uint8_t fixquality, satellites;

    boolean waitForSentence(char *wait, uint8_t max = MAXWAITSENTENCE);
    //boolean LOCUS_StartLogger(void);
    //boolean LOCUS_ReadStatus(void);

private:
    boolean paused;

    uint8_t parseResponse(char *response);
    SoftwareSerial *gpsSwSerial;
};


#endif
