/*
  GSM_GPRS.c - GPRS library for the GSM Playground - GSM Shield for Arduino
  www.hwkitchen.com

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <avr/pgmspace.h>
#include "GSM_GPRS.h"
#include "GSM.h"


extern "C" {
  #include <string.h>
}


/**********************************************************
Method returns GPRS library version

return val: 010 means library version 0.10
            101 means library version 1.01
**********************************************************/
int GSM::GPRSLibVer(void)
{
  return (GPRS_LIB_VERSION);
}


/**********************************************************
Method initializes GPRS

apn:      APN string
login:    user id string
password: password string

return:
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - GPRS was not initialized
        1 - GPRS was initialized


an example of usage:
        APN si called internet
        user id and password are not used

        GSM gsm;
        gsm.InitGPRS("internet", "", "");
**********************************************************/
char GSM::InitGPRS(char* apn, char* login, char* password)
{
    char ret_val = -1;
    char cmd[100];

    if (CLS_FREE != GetCommLineStatus()) return (ret_val);
    SetCommLineStatus(CLS_ATCMD);
    ret_val = SendATCmdWaitRespF(PSTR("AT+CSTT=\"CMNET\""), START_XXLONG_COMM_TMOUT, MAX_MID_INTERCHAR_TMOUT, "OK", 2);
    if (ret_val == AT_RESP_OK) {
        ret_val = SendATCmdWaitRespF(PSTR("AT+CIICR"), START_XXLONG_COMM_TMOUT, MAX_MID_INTERCHAR_TMOUT, "OK", 2);
        if (ret_val == AT_RESP_OK) {
            SendATCmdWaitRespF(PSTR("AT+CIFSR"), START_XXLONG_COMM_TMOUT, MAX_MID_INTERCHAR_TMOUT, "", 2);
            if (ret_val == AT_RESP_OK) {
                ret_val = 1;
            }
            else {
                ret_val = 0;
            }
        }
        else ret_val = 0;
    }
    else ret_val = 0;

    SetCommLineStatus(CLS_FREE);
    return (ret_val);
}


char GSM::OpenTCPSocket(char *addr, uint8_t port)
{
    char ret_val = 0;
    char temp;
    char tmp_str[10];
    if (CLS_FREE != GetCommLineStatus()) return (ret_val);
    SetCommLineStatus(CLS_ATCMD);

    while(1) {
        PrintlnF(PSTR("AT+CIPSTATUS"));
        WaitResp(START_XLONG_COMM_TMOUT, MAX_MID_INTERCHAR_TMOUT);
        if(strstr_P((char *)comm_buf, PSTR("IP STATUS")))
        {
            ret_val = 1;
        }
        else if(strstr_P((char *)comm_buf, PSTR("CONNECT OK")))
        {
            ret_val = 9;
        }
        else if(strstr_P((char *)comm_buf, PSTR("TCP CONNECTING")))
        {
            ret_val = 3;
        }
        else {
            ret_val = 2;
        }

        switch(ret_val)
        {
            case 1:
                // open tcp connection
                PrintF(PSTR("AT+CIPSTART=\"TCP\",\""));
                Print(addr);
                Print("\",\"");
                itoa(port, tmp_str, 10);
                Print(tmp_str);
                Println("\"");
                temp = WaitResp(START_XLONG_COMM_TMOUT, MAX_MID_INTERCHAR_TMOUT, "OK");
                if(temp != RX_FINISHED_STR_RECV)
                {
                    ret_val = 10;
                }
                break;
            case 2:
                temp = SendATCmdWaitRespF(PSTR("AT+CIPCLOSE"),START_XLONG_COMM_TMOUT, MAX_MID_INTERCHAR_TMOUT, "OK", 2);
                if (temp != AT_RESP_OK) {
                    ret_val = 10;
                }
                break;
            case 3:
                delay(3000);
                break;
            case 9:
                SetCommLineStatus(CLS_FREE);
                return 1;
                break;
            case 10:
                SetCommLineStatus(CLS_FREE);
                return 0;
                break;
        }
        ret_val = 0;
        delay(2000);
    }
    SetCommLineStatus(CLS_FREE);
    return 0;
}

char GSM::SendTCPdata_tower(unsigned char *data, unsigned char *d2)
{
    char ret_val = 0;
    char status, temp;
    PrintlnF(PSTR("AT+CIPSEND=158"));
    if (RX_FINISHED_STR_RECV == WaitResp(START_LONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, ">")) {
        Print((char*)data);
        ret_val = 1;
        for(uint8_t i=0; i< 55; i++)
        {
            Write(d2[i]);
        }
        temp = WaitResp(30000, 3000);
        /*RxInit(30000, MAX_INTERCHAR_TMOUT, 1, 0);
        do {
            status = IsRxFinished();
        } while (status == RX_NOT_FINISHED);
*/
       /* if(comm_buf_len) {
            strcpy((char*)tmp_buffer, (char*)comm_buf);
        }
        temp = WaitResp(30000, MAX_INTERCHAR_TMOUT);
        if (temp != RX_FINISHED_STR_RECV)
        {
            ret_val = 0;
        }
        Println("EXITAT");*/
        //temp = WaitResp(30000, MAX_INTERCHAR_TMOUT);
        /*ret_val = SendATCmdWaitResp("AT+CIPCLOSE", 10000, 600, "CLOSE OK", 3);
        if(ret_val != AT_RESP_OK)
        {
            SetCommLineStatus(CLS_FREE);
            ret_val = 0;
        }*/
    }
    SetCommLineStatus(CLS_FREE);
    return ret_val;
}

char GSM::SendTCPdata(unsigned char *data, unsigned char *d2, char* tmp_buffer)
{
    char ret_val = 0;
    char status, temp;
    PrintlnF(PSTR("AT+CIPSEND"));
    if (RX_FINISHED_STR_RECV == WaitResp(START_LONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, ">")) {
        PrintF((char*)data);
        Print((char*)d2);
        Write(0x1a);
        /*RxInit(30000, MAX_INTERCHAR_TMOUT, 1, 0);
        do {
            status = IsRxFinished();
        } while (status == RX_NOT_FINISHED);
*/
       /* if(comm_buf_len) {
            strcpy((char*)tmp_buffer, (char*)comm_buf);
        }
        temp = WaitResp(30000, MAX_INTERCHAR_TMOUT);
        if (temp != RX_FINISHED_STR_RECV)
        {
            ret_val = 0;
        }
        Println("EXITAT");*/
        //temp = WaitResp(30000, MAX_INTERCHAR_TMOUT);
        /*ret_val = SendATCmdWaitResp("AT+CIPCLOSE", 10000, 600, "CLOSE OK", 3);
        if(ret_val != AT_RESP_OK)
        {
            SetCommLineStatus(CLS_FREE);
            ret_val = 0;
        }*/
    }
    SetCommLineStatus(CLS_FREE);
    return ret_val;
}
