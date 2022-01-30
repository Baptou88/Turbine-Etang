#include <Arduino.h>
#include <heltec.h>
#include <Adafruit_INA219.h>




#include "TurbineEtangLib.h"








Message receivedMessage;
byte localAddress = RADIATEUR;
bool newMessage = false;
unsigned long lastMessage = 0;
int msgCount = 0;
Adafruit_INA219 ina219;

float current_mA;

void onReceive(int packetSize){
  if (packetSize == 0) return;          // if there's no packet, return
	
	//// read packet header bytes:
	receivedMessage.recipient = LoRa.read();          // recipient address
	receivedMessage.sender = LoRa.read();            // sender address
	receivedMessage.msgID = LoRa.read();
	//byte incomingMsgId = LoRa.read();     // incoming msg ID
	byte incomingLength = LoRa.read();    // incoming msg length

	receivedMessage.Content = "";                 // payload of packet

	while (LoRa.available())             // can't use readString() in callback
	{
		receivedMessage.Content += (char)LoRa.read();      // add bytes one by one
	}

	if (incomingLength != receivedMessage.length())   // check length for error
	{
		Serial.println("error: message length does not match length");
		return;                             // skip rest of function
	}
	Serial.println("msg recu: " + String(receivedMessage.Content));
	// if the recipient isn't this device or broadcast,
	if (receivedMessage.recipient != localAddress && receivedMessage.recipient != 0xFF)
	{
		Serial.println("message pas pour moi");
		return;                             // skip rest of function
	}
  newMessage = true;
  lastMessage = millis();
}
void mesureSysteme(){
   current_mA = ina219.getCurrent_mA();
}
void setup() {
  // put your setup code here, to run once:
  Heltec.begin(true,true,true,true,868E6);

  LoRa.setSpreadingFactor(8);
	LoRa.setSyncWord(0x12);
	LoRa.setSignalBandwidth(125E3);
  LoRa.onReceive(onReceive);
  LoRa.setSpreadingFactor(8);
	LoRa.setSyncWord(0x12);
	LoRa.setSignalBandwidth(125E3);
  LoRa.receive();

  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }


}

void loop() {
  mesureSysteme();
  if (newMessage)
  {
    newMessage = false;
    
    sendMessage(MASTER,"ok");
    LoRa.receive();
  }
  
  Heltec.display->clear();
  Heltec.display->drawString(0,0,(String)current_mA);
  Heltec.display->display();
  


  
  delay(100);
}