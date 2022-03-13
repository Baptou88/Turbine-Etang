/*
 Name:		TurbineEtangLib.cpp
 Created:	21/05/2021 19:15:41
 Author:	Baptou
 Editor:	http://www.visualmicro.com
*/

#include "TurbineEtangLib.h"
#include "Heltec.h"


extern int msgCount;
extern byte localAddress;

// typedef mesg_t{
// 	byte destination,
// 	byte sender,
// 	int msgCount
// }


/**
 * @brief Envoyer message Lora
 * 
 * @param destination utiliser 0xff pour envoyer à toutes les lora
 * @param outgoing message 
 */
void sendMessage(byte destination, String outgoing, bool async )
{
	LoRa.beginPacket();                   // start packet
	LoRa.write(destination);              // add destination address
	LoRa.write(localAddress);             // add sender address
	LoRa.write(msgCount);                 // add message ID
	LoRa.write(outgoing.length());        // add payload length
	LoRa.print(outgoing);                 // add payload
	LoRa.endPacket(async);                     // finish packet and send it
	msgCount++;                           // increment message ID
}

void sendMessageConfirmation(int msgNumber, byte destination ){
  sendMessage(destination,"ok="+(String)msgNumber);
}
/**
 * @brief raison du réveil
 * @param wakeup_reason description
 */

String string_wakeup_reason(esp_sleep_wakeup_cause_t wakeup_reason){
	
	
	switch (wakeup_reason)
	{
    case ESP_SLEEP_WAKEUP_EXT0 : return "Wakeup caused by external signal using RTC_IO"; break;
    case ESP_SLEEP_WAKEUP_EXT1 : return "Wakeup caused by external signal using RTC_CNTL"; break;
    case ESP_SLEEP_WAKEUP_TIMER : return "Wakeup caused by timer"; break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : return "Wakeup caused by touchpad"; break;
    case ESP_SLEEP_WAKEUP_ULP : return "Wakeup caused by ULP program"; break;
    case ESP_SLEEP_WAKEUP_GPIO : return "Wakeup caused by GPIO"; break;
    case ESP_SLEEP_WAKEUP_UART : return "Wakeup caused by UART"; break;
    default : return "Wakeup was not caused by deep sleep: " + String(wakeup_reason); break;
  }

}
String wakeup_touchpad(touch_pad_t touchPin){
	
	switch(touchPin)
  {
    case 0  : return "Touch detected on GPIO 4"; break;
    case 1  : return "Touch detected on GPIO 0"; break;
    case 2  : return "Touch detected on GPIO 2"; break;
    case 3  : return "Touch detected on GPIO 15"; break;
    case 4  : return "Touch detected on GPIO 13"; break;
    case 5  : return "Touch detected on GPIO 12"; break;
    case 6  : return "Touch detected on GPIO 14"; break;
    case 7  : return "Touch detected on GPIO 27"; break;
    case 8  : return "Touch detected on GPIO 33"; break;
    case 9  : return "Touch detected on GPIO 32"; break;
    default : return "Wakeup not by touchpad"; break;
  }

}