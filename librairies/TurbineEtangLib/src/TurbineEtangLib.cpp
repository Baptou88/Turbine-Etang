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
void sendMessage(byte destination, String outgoing)
{
	LoRa.beginPacket();                   // start packet
	LoRa.write(destination);              // add destination address
	LoRa.write(localAddress);             // add sender address
	LoRa.write(msgCount);                 // add message ID
	LoRa.write(outgoing.length());        // add payload length
	LoRa.print(outgoing);                 // add payload
	LoRa.endPacket();                     // finish packet and send it
	msgCount++;                           // increment message ID
}

String string_wakeup_reason(){
	esp_sleep_wakeup_cause_t wakeup_reason;
	wakeup_reason =  esp_sleep_get_wakeup_cause();
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