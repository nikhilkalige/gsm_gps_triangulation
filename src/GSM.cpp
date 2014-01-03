/*
   GSM.cpp - library for the GSM Playground - GSM Shield for Arduino
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


#include "Arduino.h"
#include "GSM.h"

extern "C" {
  #include <string.h>
}

// -----------------------------------------------------------------
// IMPORTANT:
// definition of GSM instance is placed here
// to enable implementation of GPS functionality as a standard class
// so the users who use the OLD GSM module without GPS functionality
// will still have GSM module without GPS methods
// -----------------------------------------------------------------
GSM gsm;




/**********************************************************
Method returns GSM library version

return val: 100 means library version 1.00
            101 means library version 1.01
**********************************************************/
int GSM::GSMLibVer(void)
{
  return (GSM_LIB_VERSION);
}

/**********************************************************
  Constructor definition
***********************************************************/

GSM::GSM(void)
{
  // set some GSM pins as inputs, some as outputs
  pinMode(GSM_ON, OUTPUT);               // sets pin 5 as output
  pinMode(GSM_RESET, OUTPUT);            // sets pin 4 as output

  pinMode(DTMF_OUTPUT_ENABLE, OUTPUT);   // sets pin 2 as output
  // deactivation of IC8 so DTMF is disabled by default
  digitalWrite(DTMF_OUTPUT_ENABLE, LOW);

  // not registered yet
  module_status = STATUS_NONE;

  // initialization of speaker volume
  // until now IP address has not been assigned
}

char GSM::getTowerInfo(void)
{
    char temp;
    temp = SendATCmdWaitResp("AT+CENG?", 5000, MAX_INTERCHAR_TMOUT,"OK",2);
    if(temp == AT_RESP_OK)
    {
        temp = 1;
    }
    return temp;
}

/**********************************************************
Methods return the state of corresponding
bits in the status variable

- these methods do not communicate with the GSM module

return values:
      0 - not true (not active)
      >0 - true (active)
**********************************************************/
byte GSM::IsRegistered(void)
{
  return (module_status & STATUS_REGISTERED);
}

byte GSM::IsInitialized(void)
{
  return (module_status & STATUS_INITIALIZED);
}

/**********************************************************
  Checks if the GSM module is responding
  to the AT command
  - if YES  nothing is made
  - if NO   switch on sequence is repeated until there is a response
            from GSM module
**********************************************************/
void GSM::TurnOn(void)
{
  SetCommLineStatus(CLS_ATCMD);


  while (AT_RESP_ERR_NO_RESP == SendATCmdWaitRespF(PSTR("AT"), 500, 20, "OK", 5)) {
    // there is no response => turn on the module

    // generate switch on pulse
    digitalWrite(GSM_ON, HIGH);
    delay(1200);
    digitalWrite(GSM_ON, LOW);
    delay(1200);

#ifdef DEBUG_PRINT
    // parameter 0 - because module is off so it is not necessary
    // to send finish AT<CR> here
    DebugPrintF(PSTR("DEBUG: GSM module is off\r\n"), 0);
#endif

#ifndef GE836_GPS
    // Be aware: reset pin on the new GE836-GPS module has different functionionlity:
    // it does not mean reset(as for GE836 without GPS) but "Hardware Unconditional Shutdown"

    // reset the module GE836 just for sure
    // this is helpful mainly in situation when GSM Playground+Arduino board are reseted
    // (and not switched-off and switched-on)during development
    digitalWrite(GSM_RESET, HIGH);
    delay(400);
    digitalWrite(GSM_RESET, LOW);
    delay(500);
#endif


    delay(3000); // wait before next try
  }
  SetCommLineStatus(CLS_FREE);

  // send collection of first initialization parameters for the GSM module
  InitParam(PARAM_SET_0);
}


/**********************************************************
  Sends parameters for initialization of GSM module

  group:  0 - parameters of group 0 - not necessary to be registered in the GSM
          1 - parameters of group 1 - it is necessary to be registered
**********************************************************/
void GSM::InitParam(byte group)
{
  char string[20];

  switch (group) {
    case PARAM_SET_0:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;
      SetCommLineStatus(CLS_ATCMD);

      // Reset to the factory settings
      //SendATCmdWaitRespF(PSTR("AT&F1"), 1000, 20, "OK", 5);
      // switch off echo
      SendATCmdWaitRespF(PSTR("ATE0"), 500, 20, "OK", 5);
      SendATCmdWaitRespF(PSTR("AT+CREG=1"), 500, 20, "OK", 5);
      SendATCmdWaitRespF(PSTR("AT+CENG=1,1"), 500, 20, "OK", 5);
      // setup fixed baud rate
      //      SendATCmdWaitRespF(PSTR("AT+IPR=115200"), 500, 20, "OK", 5);
#if 0
      sprintf(string, "AT+IPR=%li", actual_baud_rate);
      SendATCmdWaitResp(string, 500, 20, "OK", 5);
      // setup communication mode
#ifndef GE836_GPS
      SendATCmdWaitRespF(PSTR("AT#SELINT=1"), 500, 20, "OK", 5);
#else
      SendATCmdWaitRespF(PSTR("AT#SELINT=2"), 500, 20, "OK", 5);
#endif
      // Switch ON User LED - just as signalization we are here
      SendATCmdWaitRespF(PSTR("AT#GPIO=8,1,1"), 500, 20, "OK", 5);
      // Sets GPIO9 as an input = user button
      SendATCmdWaitRespF(PSTR("AT#GPIO=9,0,0"), 500, 20, "OK", 5);
      // allow audio amplifier control
      SendATCmdWaitRespF(PSTR("AT#GPIO=5,0,2"), 500, 20, "OK", 5);
      // Switch OFF User LED- just as signalization we are finished
      SendATCmdWaitRespF(PSTR("AT#GPIO=8,0,1"), 500, 20, "OK", 5);
      // set character set ”8859-1” - ISO 8859 Latin 1
      //SendATCmdWaitRespF(PSTR("AT+CSCS=\"8859-1\""), 500, 20, "OK", 5);
#endif
      SetCommLineStatus(CLS_FREE);
      break;

    case PARAM_SET_1:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;
      SetCommLineStatus(CLS_ATCMD);
#if 0
      // Audio codec - Full Rate (for DTMF usage)
      SendATCmdWaitRespF(PSTR("AT#CODEC=1"), 500, 20, "OK", 5);
      // Hands free audio path
      SendATCmdWaitRespF(PSTR("AT#CAP=1"), 500, 20, "OK", 5);
      // Echo canceller enabled
      SendATCmdWaitRespF(PSTR("AT#SHFEC=1"), 500, 20, "OK", 5);
      // Ringer tone select (0 to 32)
      SendATCmdWaitRespF(PSTR("AT#SRS=26,0"), 500, 20, "OK", 5);
      // Microphone gain (0 to 7) - response here sometimes takes
      // more than 500msec. so 1000msec. is more safety
      SendATCmdWaitRespF(PSTR("AT#HFMICG=7"), 1000, 20, "OK", 5);
#endif
      // set the SMS mode to text
      SendATCmdWaitRespF(PSTR("AT+CMGF=1"), 500, 20, "OK", 5);
#if 0
      // Auto answer after first ring enabled
      // auto answer is not used
      //SendATCmdWaitRespF(PSTR("ATS0=1"), 500, 20, "OK", 5);

      // select ringer path to handsfree
      SendATCmdWaitRespF(PSTR("AT#SRP=1"), 500, 20, "OK", 5);
      // select ringer sound level
      SendATCmdWaitRespF(PSTR("AT+CRSL=2"), 500, 20, "OK", 5);
      // we must release comm line because SetSpeakerVolume()
      // checks comm line if it is free
      SetCommLineStatus(CLS_FREE);
      // select speaker volume (0 to 14)
      SetSpeakerVolume(9);
#endif
      SetCommLineStatus(CLS_FREE);
      // init SMS storage
      //InitSMSMemory();
      // select phonebook memory storage
      SendATCmdWaitRespF(PSTR("AT+CPBS=\"SM\""), 1000, 20, "OK", 5);
      break;
  }

}

/**********************************************************
Method checks if the GSM module is registered in the GSM net
- this method communicates directly with the GSM module
  in contrast to the method IsRegistered() which reads the
  flag from the module_status (this flag is set inside this method)

- must be called regularly - from 1sec. to cca. 10 sec.

return values:
      REG_NOT_REGISTERED  - not registered
      REG_REGISTERED      - GSM module is registered
      REG_NO_RESPONSE     - GSM doesn't response
      REG_COMM_LINE_BUSY  - comm line between GSM module and Arduino is not free
                            for communication
**********************************************************/
byte GSM::CheckRegistration(void)
{
  byte status;
  byte ret_val = REG_NOT_REGISTERED;

  if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  PrintlnF(PSTR("AT+CREG?"));
  // 5 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  status = WaitResp(5000, MAX_MID_INTERCHAR_TMOUT);

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived("+CREG: 1,1")
      || IsStringReceived("+CREG: 1,5")) {
      // it means module is registered
      // ----------------------------
      module_status |= STATUS_REGISTERED;


      // in case GSM module is registered first time after reset
      // sets flag STATUS_INITIALIZED
      // it is used for sending some init commands which
      // must be sent only after registration
      // --------------------------------------------
      if (!IsInitialized()) {
        module_status |= STATUS_INITIALIZED;
        SetCommLineStatus(CLS_FREE);
        InitParam(PARAM_SET_1);
      }
      ret_val = REG_REGISTERED;
    }
    else {
      // NOT registered
      // --------------
      module_status &= ~STATUS_REGISTERED;
      ret_val = REG_NOT_REGISTERED;
    }
  }
  else {
    // nothing was received
    // --------------------
    ret_val = REG_NO_RESPONSE;
  }
  SetCommLineStatus(CLS_FREE);


  return (ret_val);
}

