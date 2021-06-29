/*
 Name:		Etang.ino
 Created:	21/05/2021 18:29:42
 Author:	Baptou
*/

// the setup function runs once when you press reset or power the board
#include <EEPROM.h>
#include <heltec.h>
#include <VL53L1X.h>
#include <TurbineEtangLib.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_BMP280.h>

#define PRGButton 0


Adafruit_BMP280 bmp; // I2C
float temp = 0;
float pressure = 0;
byte displayMode = 0;
VL53L1X vl53l1x;

bool previousEtatButton = false;

Message receivedMessage;

int NiveauEtang = 0;
unsigned long previousMesure = 0;
unsigned long previousSend = 0;

byte localAddress = 0x0C;
byte msgCount = 0;
int NiveauMax = 0;
int NiveauMin = 1000;

const int longeurEtang = 12000;
const int largeurEtang= 6000;

Preferences preferences;

float PNiveau(void)
{
	//-----------------------------------------------capteur-------------
	// ^			^  ^								|
	// |			|  |								v
	// |			|  |  max
	// |	niveau  |  |
	// |   			|  v					|							|
	// |			|  --------------------	|							|
	// |			v						|							|
	// |	min		------------------------|~~~~~~~~~~~~~~~~~~~~~~~~~~~|
	// v									|							|
	// -------------------------------------|___________________________|

	return (float(NiveauMin - NiveauEtang)) / (float(NiveauMin - NiveauMax));
}
void AfficherNiveauJauge(void)
{
	Heltec.display->drawRect(115, 4, 10, 60);
	int pourcenta = 60 - (60 - 10) * PNiveau();
	Heltec.display->fillRect(115, pourcenta, 10, 60);
}



void displayData(void)
{
	Heltec.display->clear();
	switch (displayMode)
	{
	case 0:
		Heltec.display->drawString(0, 10, "Niveau: " + String(NiveauEtang));
		Heltec.display->drawString(0, 22, "Ratio: " + String(PNiveau()*100) + "%");
		Heltec.display->drawString(0, 40, vl53l1x.rangeStatusToString(vl53l1x.ranging_data.range_status));
		AfficherNiveauJauge();

		Heltec.display->drawString(60,2,"Max:" + String(NiveauMax));
		Heltec.display->drawString(60,52,"Min:" + String(NiveauMin));
		break;
	case 1:
		Heltec.display->drawString(0, 10, "Range   " + String(vl53l1x.ranging_data.range_mm));
		Heltec.display->drawString(0, 20, "Status  " + String(vl53l1x.rangeStatusToString(vl53l1x.ranging_data.range_status)));
		Heltec.display->drawString(0, 30, "Ambient " + String(vl53l1x.ranging_data.ambient_count_rate_MCPS));
		Heltec.display->drawString(0, 40, "Peak    " + String(vl53l1x.ranging_data.peak_signal_count_rate_MCPS));

		break;
	case 2 :
		Heltec.display->drawLogBuffer(0,0);
		break;
	
	default:
		Heltec.display->drawString(0, 10, "erreur indice affichage" );
		break;
	}
	
	Heltec.display->display();
}

                
void mesureSysteme(void)
{
	if (millis() - previousMesure > 500)
	{
		
		previousMesure = millis();
		vl53l1x.read();
		
		temp = bmp.readTemperature();
		pressure = bmp.readPressure();
		NiveauEtang = vl53l1x.ranging_data.range_mm;
	}
	

	
}


void initPrefs(void){

	
	
}

void TraitementCommande(String Commande){
	if (Commande.startsWith("SMAX"))
	{
		//Commande.remove(0,3);
		NiveauMax = vl53l1x.ranging_data.range_mm;
		Heltec.display->println("SET MAX ok !");
	}
	if (Commande.startsWith("SMIN"))
	{
		//Commande.remove(0,3);
		NiveauMin = vl53l1x.ranging_data.range_mm;
		Heltec.display->println("SET Min ok !");
	}
	if (Commande.startsWith("SEEPROM"))
	{
		//Commande.remove(0,6);
		Serial.println("sauvegarde");
		preferences.putInt("NiveauMax",NiveauMax);
		preferences.putInt("NiveauMin",NiveauMin);
		Heltec.display->println("Save EEPROM ok !");
	}
	if (Commande== "SLEEP")
	{
		
		Serial.println("Mise en Veille");
	
		Heltec.display->println("Mise en Veille");
		delay(1000);
		esp_sleep_enable_timer_wakeup(10000000); //10sec
		esp_deep_sleep_start();
	}
	if (Commande.startsWith("SLEEPTP"))
	{
		
		Serial.println("Mise en Veille");
	
		Heltec.display->println("Mise en Veille");
		delay(1000);
		esp_sleep_enable_touchpad_wakeup();
		esp_deep_sleep_start();
	}
}
void onReceive(int packetSize)
{
	if (packetSize == 0) return;          // if there's no packet, return

	//// read packet header bytes:
	receivedMessage.recipient = LoRa.read();          // recipient address
	receivedMessage.sender = LoRa.read();            // sender address
	byte incomingMsgId = LoRa.read();     // incoming msg ID
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

	// if the recipient isn't this device or broadcast,
	if (receivedMessage.recipient != localAddress && receivedMessage.recipient != 0xFF)
	{
		Serial.println("This message is not for me.");
		return;                             // skip rest of function
	}
	
	//// if message is for this device, or broadcast, print details:
	Serial.println("Received from: 0x" + String(receivedMessage.sender, HEX));
	Serial.println("Sent to: 0x" + String(receivedMessage.recipient, HEX));
	Serial.println("Message ID: " + String(incomingMsgId));
	Serial.println("Message length: " + String(incomingLength));
	Serial.println("Message: " + receivedMessage.Content);
	Serial.println("RSSI: " + String(LoRa.packetRssi()));
	//Serial.println("Snr: " + String(LoRa.packetSnr()));
	Serial.println();
	Heltec.display->println("0x" + String(receivedMessage.sender,HEX) + " to 0x" + String(receivedMessage.recipient, HEX) + " " + String(receivedMessage.Content));

	//TraitementCommande(receivedMessage.Content);
}

void setup() {
	Heltec.begin(true, true, true, true, 868E6);
	
	pinMode(LED_BUILTIN, OUTPUT);

	//bmp280
	if (!bmp.begin(0x76)) {
		Heltec.display->drawString(0, 12, "Failed init bmp280");
		Heltec.display->display();
    
  	} else {
		  Heltec.display->drawString(0, 12, "ok init bmp280");
	}
	//Vl53L1X
	vl53l1x.setTimeout(500);
	Heltec.display->drawString(20, 24, "Init vl53l1x");
	if (!vl53l1x.init())
	{
		Heltec.display->drawString(20, 24, "X");
		Serial.println("Failed init VL53L1X");
		Heltec.display->display();
	}
	else
	{
		Heltec.display->drawString(0, 24, "OK");
		Serial.println("Ok init VL53L1X");
	}
	
	vl53l1x.setDistanceMode(VL53L1X::Long);
	vl53l1x.setMeasurementTimingBudget(180000);//50000
	
	// Start continuous readings at a rate of one measurement every 50 ms (the
	// inter-measurement period). This period should be at least as long as the
	// timing budget.
	vl53l1x.startContinuous(50);
	// vl53l1x.setROICenter(64);
	vl53l1x.setROISize(1,1);
	LoRa.onReceive(onReceive);
	LoRa.receive();



	Heltec.display->setLogBuffer(10,30);
	//initEEPROM();
	initPrefs();
	preferences.begin("etang", false );
	
		//preferences.putInt("NiveauMax",16);
		//preferences.putInt("NiveauMin",16);
		NiveauMin = preferences.getInt("NiveauMin",0);
		NiveauMax = preferences.getInt("NiveauMax",0);
		//preferences.putInt("NiveauMax",18);
		Serial.printf(" prefs : Min: %u Max: %u", NiveauMin , NiveauMax );
		
	Heltec.display->display();
	delay(1000);
	Heltec.display->clear();
}

// the loop function runs over and over again until power down or reset
void loop() {
	mesureSysteme();
	displayData();
	if ((digitalRead(PRGButton) == LOW) && !previousEtatButton)
	{
		previousEtatButton = true;
		if (displayMode < 2)
		{
			displayMode++;
		}
		else
		{
			displayMode = 0;
		}
	}
	if (digitalRead(PRGButton) == HIGH)
	{
		previousEtatButton = false;
	}
	if (millis() - previousSend > 5000)
	{
		previousSend = millis();
		//sendData();
		StaticJsonDocument<128> doc;
		String json; 
		doc["mesure"] = vl53l1x.ranging_data.range_mm;
		doc["Niveau"] = PNiveau();
		doc["RangeStatus"] = VL53L1X::rangeStatusToString( vl53l1x.ranging_data.range_status);
		doc["temp"] = temp,
		doc["pressure"] = pressure;
		
		serializeJson(doc,json);
		Serial.println("voila ce que jenvoi: "+ String(json));
		// sendMessage(0x0A, "Niveau: " + String(PNiveau())+ "," +
		// 	"RangeStatus :" + VL53L1X::rangeStatusToString( vl53l1x.ranging_data.range_status) + "," +
		// 	"PeakSignal :" + String(vl53l1x.ranging_data.peak_signal_count_rate_MCPS)+ "," +
		// 	"AmbientSignal :" + String(vl53l1x.ranging_data.ambient_count_rate_MCPS)+ "," 
		// );
		sendMessage(0x0A, json);
		LoRa.receive();
	}
	if (Serial.available())
	{
		String s = Serial.readStringUntil('\r');
		TraitementCommande(s);
		
		
	}
	if (receivedMessage.Content != "")
	{
		Serial.println(receivedMessage.Content);
		TraitementCommande(receivedMessage.Content);
		receivedMessage.Content= "";
	}
	
	delay(200);
}

