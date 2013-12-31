#include "GSM.h"
#include <SoftwareSerial.h>



signed char ret_val;
uint16_t num_of_rx_bytes;
byte* ptr_to_data;
char buffer[COMM_BUF_LEN];
SoftwareSerial debug(10,11);

void getcellinfo();
char* extract_cell_data(char *buff, char id);
void fill_packet(char id);
void triangulate();

struct cell_struct {
    unsigned char id[8];
    unsigned char lac[8];
    unsigned char strength[5];
    float latitude, longitude;
};

struct towers_struct {
    struct cell_struct cell[4];
    unsigned char mnc[6];
};

struct pos {
    float latitude, longitude;
};

struct pos mypos;

struct towers_struct towers;

 uint8_t header[]  = "POST /glm/mmap HTTP/1.1\r\nHost: www.google.com\r\nContent-type: application/binary\r\nContent-Length: 55\r\n\r\n";

uint8_t packet[] = {
    0x00 ,0x0e, // Function Code?
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, //Session ID?
    0x00 ,0x00, // Contry Code
    0x00 ,0x00, // Client descriptor
    0x00 ,0x00, // Version
    0x1b, // Op Code?
    0x00 ,0x00 ,0x00 ,0x00, // MNC
    0x00 ,0x00 ,0x01 ,0x94, // MCC
    0x00 ,0x00 ,0x00 ,0x03,
    0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00, //CID
    0x00 ,0x00 ,0x00 ,0x00, //LAC
    0x00 ,0x00 ,0x00 ,0x00, // MNC
    0x00 ,0x00 ,0x01 ,0x94, // MCC
    0xff ,0xff ,0xff ,0xff, // ??
    0x00 ,0x00 ,0x00 ,0x00,  // Rx Level?
};


#define MNC1    17
#define MNC2    39
#define CID     31
#define LAC     35


char address[] = "www.google.com";
unsigned char buf[50];

void setup()
{
    debug.begin(4800);
    gsm.InitSerLine(115200);
#if 0
    gsm.TurnOn();
    while (!gsm.IsRegistered()) {
        gsm.CheckRegistration();
        delay(1000);
    }

    debug.println("Registration successfull");
#endif

    // use GPRS APN "internet" - it is necessary to find out right one for
    // your GSM provider
    /* provide some delay for initiation of gprs */
    delay(2000);
    ret_val = 0;
#if 0
    while(!ret_val) {
        ret_val = gsm.InitGPRS("internet", "", "");
    }

    if(ret_val)
    {
        debug.println("Connected");
    }
    else
    {
        debug.println("Connection Error");
    }
#endif
    debug.println("Start OK");
    gsm.getTowerInfo();
    getcellinfo();
    int i;
    for(i=0;i < 3;i++)
    {
        debug.println((char*)towers.cell[i].id);
        debug.println((char*)towers.cell[i].lac);
        debug.println((char*)towers.cell[i].strength);
    }
    for(i=0;i < 3;i++)
    {
        fill_packet(i);

        ret_val = gsm.OpenTCPSocket(address, 80);
        if(ret_val)
        {
            //debug.println("TCP Open");
        }
        else
        {
            //debug.println("TCP Open Error");
        }
#if 1
        ret_val = gsm.SendTCPdata(header,packet,buffer);
        if(ret_val)
        {
            //debug.println("Send OK");
            char *ptr = strstr((char*)gsm.comm_buf, "\r\n\r\n");
            char *t;
            if(ptr)
            {
                ptr+= 5;
                memcpy((char*)buf, ptr,20);
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
                sprintf((char*)str,"%ld", lat);
                debug.print((char*)str);

                lgt = (unsigned long)buf[10] << 24;
                lgt += (unsigned long)buf[11] << 16;
                lgt += (unsigned long)buf[12] << 8;
                lgt += (unsigned long)buf[13];
                sprintf((char*)str,"%ld", lgt);
                debug.println((char*)str);

                towers.cell[i].latitude = (float)lat/1000000;
                towers.cell[i].longitude = (float)lgt/1000000;
                debug.print(towers.cell[i].latitude,6);
                debug.print(",");
                debug.println(towers.cell[i].longitude,6);
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


void loop()
{

}

void getcellinfo()
{
    char *p1, *p2;
    p1 = strstr((char*)gsm.comm_buf, ":0,\"");
    if(p1)
    {
        p1+= 4;
        p1 = strchr(p1,',');
        p2 = strchr(p1+1,',');
        *p2 = 0;
        strcpy((char*)towers.cell[0].strength, (p1+1));
        p1 = p2+1;
        p1 = strchr(p1,',');
        p1 = strchr(p1+1,',');
        p2 = strchr(p1+1,',');
        *p2 = 0;
        strcpy((char*)towers.mnc, (p1+1));
        p1 = p2+1;
        p1 = strchr(p1+1,',');
        p2 = strchr(p1+1,',');
        *p2 = 0;
        strcpy((char*)towers.cell[0].id, (p1+1));
        p1 = p2+1;
        p2 = strchr(p1+1,',');
        *p2 = 0;
        strcpy((char*)towers.cell[0].lac, (p1));
        p1 = p2+1;
        p1 = extract_cell_data(p1,":1,\"",1);
        extract_cell_data(p1,":2,\"",2);
    }
}

char* extract_cell_data(char *a, char *buff, char id)
{
    char *p1, *p2;
    if(p1 = strstr((char*)a, buff))
    {
        p1+= 4;
        p1 = strchr(p1,',');
        p2 = strchr(p1+1,',');
        *p2 = 0;
        strcpy((char*)towers.cell[id].strength, (p1+1));
        p1 = p2+1;
        p1 = strchr(p1,',');
        p2 = strchr(p1+1,',');
        *p2 = 0;
        strcpy((char*)towers.cell[id].id, (p1+1));
        p1 = p2+1;
        p2 = strchr(p1,'"');
        *p2 = 0;
        strcpy((char*)towers.cell[id].lac, (p1));
        a = p2+1;
    }
    return a;
}


void fill_packet(char id)
{
#if 0
    char i;
    debug.print("Packet\n");
    for(i=0;i<55;i++)
    {
        debug.print(packet[i], HEX);
    }
    debug.print("\n");
#endif
    unsigned long temp;
    temp = strtol ((char*)towers.mnc,NULL,10);
    packet[MNC1] = (byte)(temp >> 24);
    packet[MNC1+1] = (byte)(temp >> 16);
    packet[MNC1+2] = (byte)(temp >> 8);
    packet[MNC1+3] = (byte)(temp);
    packet[MNC2+1] = (byte)(temp >> 24);
    packet[MNC2+2] = (byte)(temp >> 16);
    packet[MNC2+3] = (byte)(temp >> 8);
    packet[MNC2+4] = (byte)(temp);
    temp = strtol ((char*)towers.cell[id].id,NULL,16);
    packet[CID+0] = (byte)(temp >> 24);
    packet[CID+1] = (byte)(temp >> 16);
    packet[CID+2] = (byte)(temp >> 8);
    packet[CID+3] = (byte)(temp);
    temp = strtol ((char*)towers.cell[id].lac,NULL,10);
    packet[LAC+0] = (byte)(temp >> 24);
    packet[LAC+1] = (byte)(temp >> 16);
    packet[LAC+2] = (byte)(temp >> 8);
    packet[LAC+3] = (byte)(temp);
#if 0
    for(i=0;i<55;i++)
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
    for(i = 0;i <3;i++)
    {
        debug.print((char*)towers.cell[i].strength);
        debug.print(", ");
        s[i] = (char)strtol((char*)towers.cell[i].strength, NULL, 10);
        debug.print(s[i],DEC);
        debug.print(", ");
        s[i] = 110 - s[i];
        mean+= s[i];
        lat += towers.cell[i].latitude * s[i];
        lgt += towers.cell[i].longitude * s[i];

        debug.print(s[i],DEC);
        debug.print(", ");
        debug.print(mean,DEC);
        debug.print(", ");
        debug.print(lat,6);
        debug.print(", ");
        debug.println(lgt,6);
    }

    mypos.latitude = lat/mean;
    mypos.longitude = lgt/mean;

    debug.print(mypos.latitude,6);
    debug.print(",");
    debug.println(mypos.longitude,6);

}
