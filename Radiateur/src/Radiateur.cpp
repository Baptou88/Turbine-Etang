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
Adafruit_INA219 ina219_2(0x44);

float current_mA;
float current_mA_2;

float offset_A = 0;

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
   current_mA_2 = ina219_2.getCurrent_mA();
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
  if (! ina219_2.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  pinMode(2,INPUT);
  for (size_t i = 0; i < 10; i++)
  {
  Serial.println(offset_A);
    offset_A += analogRead(2);
  }

  Serial.println(offset_A);
  offset_A = ((offset_A/10)*3.3)/4095;
  Serial.println(offset_A);

}

void loop() {
  mesureSysteme();
  if (newMessage)
  {
    newMessage = false;
    
    sendMessage(MASTER,"ok");
    LoRa.receive();
  }
  
 float wcs_vol = (analogRead(2)*3.3)/4095;
 float wcs_a =  (wcs_vol - offset_A)/0.0269;

  Heltec.display->clear();
  Heltec.display->drawString(0,0,(String)current_mA + " mA");
  // Heltec.display->drawString(0,0,(String)ina219.getBusVoltage_V()+ " V");
  // Heltec.display->drawString(0,0,(String)ina219.getShuntVoltage_mV()+ " V");
  Heltec.display->drawString(0,30,(String)current_mA_2 + " mA");
  Heltec.display->drawString(50,50,(String)wcs_a + " A");
  Heltec.display->drawString(0,50,(String)wcs_vol + " V");
  Heltec.display->display();
  


  
  delay(100);
}