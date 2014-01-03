#include "GSM.h"
#include <SoftwareSerial.h>
#include "GPS.h"

#define BUTTON 7

signed char ret_val;
uint16_t num_of_rx_bytes;
byte *ptr_to_data;
//char buffer[COMM_BUF_LEN];
SoftwareSerial debug(10, 11);

void getcellinfo();
char *extract_cell_data(char *buff, char id);
void fill_packet(char id);
void triangulate();
char gsm_get_latitude();

struct cell_struct
{
    unsigned char id[8];
    unsigned char lac[8];
    unsigned char strength[5];
    float latitude, longitude;
};

struct towers_struct
{
    struct cell_struct cell[4];
    unsigned char mnc[6];
};

struct pos
{
    float latitude, longitude;
};

struct pos mypos;

struct towers_struct towers;

const uint8_t PROGMEM header[]  = "POST /glm/mmap HTTP/1.1\r\nHost: www.google.com\r\nContent-type: application/binary\r\nContent-Length: 55\r\n\r\n";
const uint8_t PROGMEM header_xively[] = "PUT /v2/feeds/1098360437.csv HTTP/1.1\r\nHost: api.xively.com\r\nX-ApiKey: J0M4jK2f6m57whJSbLD13xKXRJULR4pT9fUP4gnpFB39qSZC\r\nContent-Length: ";

uint8_t packet[] =
{
    0x00 , 0x0e, // Function Code?
    0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, //Session ID?
    0x00 , 0x00, // Contry Code
    0x00 , 0x00, // Client descriptor
    0x00 , 0x00, // Version
    0x1b, // Op Code?
    0x00 , 0x00 , 0x00 , 0x00, // MNC
    0x00 , 0x00 , 0x01 , 0x94, // MCC
    0x00 , 0x00 , 0x00 , 0x03,
    0x00 , 0x00,
    0x00 , 0x00 , 0x00 , 0x00, //CID
    0x00 , 0x00 , 0x00 , 0x00, //LAC
    0x00 , 0x00 , 0x00 , 0x00, // MNC
    0x00 , 0x00 , 0x01 , 0x94, // MCC
    0xff , 0xff , 0xff , 0xff, // ??
    0x00 , 0x00 , 0x00 , 0x00, // Rx Level?
};


#define MNC1    17
#define MNC2    39
#define CID     31
#define LAC     35


char address[] = "www.google.com";
char address_xively[] = "api.xively.com";
unsigned char buf[70];

// GPS serial port
SoftwareSerial mySerial(3, 2);
Adafruit_GPS GPS(&mySerial);

boolean usingInterrupt = false;
void useInterrupt(boolean);

void setup()
{
    debug.begin(4800);

    pinMode(BUTTON, INPUT);
    digitalWrite(BUTTON, HIGH);

    gsm.InitSerLine(115200);
    GPS.begin(9600);

#if 0
    GPS.sendCommand(GPSGGA, 2);
    GPS.sendCommand(GPSGLL, 0);
    GPS.sendCommand(GPSGSA, 0);
    GPS.sendCommand(GPSGSV, 0);
    GPS.sendCommand(GPSRMC, 2);
    GPS.sendCommand(GPSVTG, 0);
#else
    GPS.sendCommand((char *)PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand((char *)PMTK_SET_NMEA_UPDATE_1HZ);
#endif
    useInterrupt(true);

#if 1
    gsm.TurnOn();
    while (!gsm.IsRegistered())
    {
        gsm.CheckRegistration();
        delay(1000);
    }

    debug.println("Reg suc");
#endif

    // use GPRS APN "internet" - it is necessary to find out right one for
    // your GSM provider
    /* provide some delay for initiation of gprs */
    delay(2000);
    ret_val = 0;
#if 0
    while (!ret_val)
    {
        ret_val = gsm.InitGPRS(NULL, NULL, NULL);
    }

    if (ret_val)
    {
        debug.println("Connected");
    }
    else
    {
        debug.println("Connection Error");
    }
#endif
    debug.println("Start OK");

    int i;
    for (i = 0; i < 3; i++)
    {
        debug.println((char *)towers.cell[i].id);
        debug.println((char *)towers.cell[i].lac);
        debug.println((char *)towers.cell[i].strength);
    }

}

uint32_t timer = millis();
void loop()
{
    if (GPS.newNMEAreceived())
    {
        if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
            return;  // we can fail to parse a sentence in which case we should just wait for another
    }
#if 0
    if (timer > millis())  timer = millis();
    if (millis() - timer > 2000)
    {
        timer = millis(); // reset the timer
    }
#endif
    if (digitalRead(BUTTON) == LOW)
    {
         debug.println("B press");
        // Send data to server
        delay(2000);
        char status = 0;
        if (GPS.fix)
        {
            debug.println("gps");
            mypos.latitude = GPS.latitude;
            mypos.longitude  = GPS.longitude;
            status = 1;
        }
        else
        {
            debug.println("gsm");
            gsm_get_latitude();
            status = 1;
        }
        if (status)
        {
            debug.println("server");
            send_server();
        }
    }
}



/* Functions */

SIGNAL(TIMER0_COMPA_vect)
{
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
#if 0
#ifdef UDR0
    if (GPSECHO)
        if (c) UDR0 = c;
    // writing direct to UDR0 is much much faster than Serial.print
    // but only one character can be written at a time.
#endif
#endif
}

void useInterrupt(boolean v)
{
    if (v)
    {
        // Timer0 is already used for millis() - we'll just interrupt somewhere
        // in the middle and call the "Compare A" function above
        OCR0A = 0xAF;
        TIMSK0 |= _BV(OCIE0A);
        usingInterrupt = true;
    }
    else
    {
        // do not call the interrupt function COMPA anymore
        TIMSK0 &= ~_BV(OCIE0A);
        usingInterrupt = false;
    }
}

void getcellinfo()
{
    char *p1, *p2;
    p1 = strstr((char *)gsm.comm_buf, ":0,\"");
    if (p1)
    {
        p1 += 4;
        p1 = strchr(p1, ',');
        p2 = strchr(p1 + 1, ',');
        *p2 = 0;
        strcpy((char *)towers.cell[0].strength, (p1 + 1));
        p1 = p2 + 1;
        p1 = strchr(p1, ',');
        p1 = strchr(p1 + 1, ',');
        p2 = strchr(p1 + 1, ',');
        *p2 = 0;
        strcpy((char *)towers.mnc, (p1 + 1));
        p1 = p2 + 1;
        p1 = strchr(p1 + 1, ',');
        p2 = strchr(p1 + 1, ',');
        *p2 = 0;
        strcpy((char *)towers.cell[0].id, (p1 + 1));
        p1 = p2 + 1;
        p2 = strchr(p1 + 1, ',');
        *p2 = 0;
        strcpy((char *)towers.cell[0].lac, (p1));
        p1 = p2 + 1;
        p1 = extract_cell_data(p1, ":1,\"", 1);
        extract_cell_data(p1, ":2,\"", 2);
    }
}

char *extract_cell_data(char *a, char *buff, char id)
{
    char *p1, *p2;
    if (p1 = strstr((char *)a, buff))
    {
        p1 += 4;
        p1 = strchr(p1, ',');
        p2 = strchr(p1 + 1, ',');
        *p2 = 0;
        strcpy((char *)towers.cell[id].strength, (p1 + 1));
        p1 = p2 + 1;
        p1 = strchr(p1, ',');
        p2 = strchr(p1 + 1, ',');
        *p2 = 0;
        strcpy((char *)towers.cell[id].id, (p1 + 1));
        p1 = p2 + 1;
        p2 = strchr(p1, '"');
        *p2 = 0;
        strcpy((char *)towers.cell[id].lac, (p1));
        a = p2 + 1;
    }
    return a;
}


void fill_packet(char id)
{
#if 0
    char i;
    debug.print("Packet\n");
    for (i = 0; i < 55; i++)
    {
        debug.print(packet[i], HEX);
    }
    debug.print("\n");
#endif
    unsigned long temp;
    temp = strtol ((char *)towers.mnc, NULL, 10);
    packet[MNC1] = (byte)(temp >> 24);
    packet[MNC1 + 1] = (byte)(temp >> 16);
    packet[MNC1 + 2] = (byte)(temp >> 8);
    packet[MNC1 + 3] = (byte)(temp);
    packet[MNC2 + 1] = (byte)(temp >> 24);
    packet[MNC2 + 2] = (byte)(temp >> 16);
    packet[MNC2 + 3] = (byte)(temp >> 8);
    packet[MNC2 + 4] = (byte)(temp);
    temp = strtol ((char *)towers.cell[id].id, NULL, 16);
    packet[CID + 0] = (byte)(temp >> 24);
    packet[CID + 1] = (byte)(temp >> 16);
    packet[CID + 2] = (byte)(temp >> 8);
    packet[CID + 3] = (byte)(temp);
    temp = strtol ((char *)towers.cell[id].lac, NULL, 10);
    packet[LAC + 0] = (byte)(temp >> 24);
    packet[LAC + 1] = (byte)(temp >> 16);
    packet[LAC + 2] = (byte)(temp >> 8);
    packet[LAC + 3] = (byte)(temp);
#if 0
    for (i = 0; i < 55; i++)
    {
        debug.print(packet[i], HEX);
    }
    debug.print("\n");
#endif
}



void triangulate()
{
    unsigned char mean, i;
    unsigned char s[3];
    mean = 0;
    float lat, lgt;
    lat = 0;
    lgt = 0;
    for (i = 0; i < 3; i++)
    {
        debug.print((char *)towers.cell[i].strength);
        debug.print(", ");
        s[i] = (char)strtol((char *)towers.cell[i].strength, NULL, 10);
        debug.print(s[i], DEC);
        debug.print(", ");
        s[i] = 110 - s[i];
        mean += s[i];
        lat += towers.cell[i].latitude * s[i];
        lgt += towers.cell[i].longitude * s[i];

        debug.print(s[i], DEC);
        debug.print(", ");
        debug.print(mean, DEC);
        debug.print(", ");
        debug.print(lat, 6);
        debug.print(", ");
        debug.println(lgt, 6);
    }

    mypos.latitude = lat / mean;
    mypos.longitude = lgt / mean;

    debug.print(mypos.latitude, 6);
    debug.print(",");
    debug.println(mypos.longitude, 6);

}


char gsm_get_latitude()
{
    gsm.getTowerInfo();
    getcellinfo();
    unsigned char i;
    for (i = 0; i < 3; i++)
    {
        fill_packet(i);
        ret_val = gsm.OpenTCPSocket(address, 80);
        if (ret_val)
        {
            //debug.println("TCP Open");
        }
        else
        {
            //debug.println("TCP Open Error");
        }
#if 1
        ret_val = gsm.SendTCPdata_tower((unsigned char*)header, packet);
        if (ret_val)
        {
            //debug.println("Send OK");
            char *ptr = strstr((char *)gsm.comm_buf, "\r\n\r\n");
            char *t;
            if (ptr)
            {
                ptr += 5;
                memcpy((char *)buf, ptr, 20);
                /*for(char i=0;i<20;i++)
                {
                    debug.println((unsigned char)buf[i], HEX);
                }*/
                unsigned long lat, lgt;
                char str[20];
                lat = (unsigned long)buf[6] << 24;
                lat += (unsigned long)buf[7] << 16;
                lat += (unsigned long)buf[8] << 8;
                lat += (unsigned long)buf[9];
                sprintf((char *)str, "%ld", lat);
                debug.print((char *)str);

                lgt = (unsigned long)buf[10] << 24;
                lgt += (unsigned long)buf[11] << 16;
                lgt += (unsigned long)buf[12] << 8;
                lgt += (unsigned long)buf[13];
                sprintf((char *)str, "%ld", lgt);
                debug.println((char *)str);

                towers.cell[i].latitude = (float)lat / 1000000;
                towers.cell[i].longitude = (float)lgt / 1000000;
                debug.print(towers.cell[i].latitude, 6);
                debug.print(",");
                debug.println(towers.cell[i].longitude, 6);
            }
        }
        else
        {
            debug.println("Send Error");
        }
    }
#endif
    triangulate();
}


char send_server()
{
    // Assumption of max length as 999
    char temp[20], length, i;
    debug.println(mypos.latitude, 6);
    debug.println(mypos.longitude, 6);
    ret_val = gsm.OpenTCPSocket(address_xively, 80);
    strcpy((char *)buf, (char*)"   \r\n\r\n");
    strcat((char *)buf, "Latitude,");
    dtostrf(mypos.latitude, 11, 6, temp);
    strcat((char*)buf, temp);
    strcat((char*)buf, "\r\n");
    strcat((char *)buf, "Longitude,");
    dtostrf(mypos.longitude, 11, 6, temp);
    strcat((char*)buf, temp);
    length = strlen((char*)buf);
    length-= 7;
    itoa(length, temp, 10);
    length = strlen((char*)temp);
    for(i=0;i<length;i++) {
        buf[i] = temp[i];
    }
    ret_val = gsm.SendTCPdata((unsigned char*)header_xively, buf, NULL);
}
/*
char *dtostrf (double val, signed char width, unsigned char prec, char *sout)
{
    char fmt[20];
    sprintf(fmt, "%%%d.%df", width, prec);
    sprintf(sout, fmt, val);
    return sout;
}
*/
