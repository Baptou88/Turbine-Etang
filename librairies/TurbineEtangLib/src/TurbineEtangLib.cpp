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