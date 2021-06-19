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


#define PRGButton 0

byte displayMode = 0;
VL53L1X sensor;

int NiveauEtang = 0;
unsigned long previousMesure = 0;
unsigned long previousSend = 0;

byte localAddress = 0x0C;
byte msgCount = 0;
int NiveauMax = 0;
int NiveauMin = 1000;
EEPROMClass NiveauMini("NiveauMini", 0x200);
EEPROMClass NiveauMaxi("NiveauMaxi", 0x200);

void AfficherNiveauJauge(void)
{
	Heltec.display->drawRect(110, 4, 10, 60);
	int pourcenta = 60 - (60 - 10) * PNiveau();
	Heltec.display->fillRect(110, pourcenta, 10, 60);
}
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
void initEEPROM(void) {
	Heltec.display->clear();
	if (!NiveauMini.begin(NiveauMini.length()))
	{
		Serial.println("Failed to initialise NiveauMini");
	}
	else
	{
		//NiveauMini.write(0,25);
		NiveauMini.put(0, 27);
		NiveauMini.get(0, NiveauMin);
		//NiveauMini.commit();
		
		Serial.println("Mini: " + String(NiveauMin));
	}
	if (!NiveauMaxi.begin(NiveauMaxi.length()))
	{
		Serial.println("Failed to initialise NiveauMaxi");
	}
	
}
void displayData(void)
{
	Heltec.display->clear();
	switch (displayMode)
	{
	case 0:
		Heltec.display->drawString(0, 10, "Niveau: " + String(NiveauEtang));
	Heltec.display->drawString(40, 22, "Ratio: " + String(PNiveau()) + "%");
	Heltec.display->drawString(0, 40, sensor.rangeStatusToString(sensor.ranging_data.range_status));
	AfficherNiveauJauge();
		break;
	case1:
		Heltec.display->drawString(0, 10, "Range   " + String(sensor.ranging_data.range_mm));
		Heltec.display->drawString(0, 20, "Status  " + String(sensor.rangeStatusToString(sensor.ranging_data.range_status)));
		Heltec.display->drawString(0, 30, "Ambient " + String(sensor.ranging_data.ambient_count_rate_MCPS));
		Heltec.display->drawString(0, 40, "Peak    " + String(sensor.ranging_data.peak_signal_count_rate_MCPS));

		break;
	default:
		break;
	}
	
	Heltec.display->display();
}

void  sendData(void) {
	digitalWrite(LED_BUILTIN, HIGH);
	LoRa.beginPacket();
	LoRa.print(localAddress);
	LoRa.print(1);
	LoRa.print(PNiveau());
	LoRa.endPacket();
	digitalWrite(LED_BUILTIN, LOW);
}
                

void setup() {
	Heltec.begin(true, true, true, true, 868E6);
	pinMode(LED_BUILTIN, OUTPUT);
	//Vl53L1X
	sensor.setTimeout(500);
	Serial.println("Hello World ! ");
	if (!sensor.init())
	{
		Heltec.display->drawString(0, 10, "Failed init vl53l1x");
		Serial.println("Failed init VL53L1X");
	}
	else
	{
		Heltec.display->drawString(0, 10, "ok init vl53l1x");
		Serial.println("Ok init VL53L1X");
	}
	sensor.setDistanceMode(VL53L1X::Long);
	sensor.setMeasurementTimingBudget(50000);

	// Start continuous readings at a rate of one measurement every 50 ms (the
	// inter-measurement period). This period should be at least as long as the
	// timing budget.
	sensor.startContinuous(50);
	initEEPROM();
}

// the loop function runs over and over again until power down or reset
void loop() {
	mesureSysteme();
	displayData();

	if (millis() - previousSend > 5000)
	{
		previousSend = millis();
		//sendData();
		sendMessage(0x0A, "Niveau: " + String(PNiveau())+ "," +
			"RangeStatus :" + VL53L1X::rangeStatusToString( sensor.ranging_data.range_status) + "," +
			"PeakSignal :" + String(sensor.ranging_data.peak_signal_count_rate_MCPS)+ "," +
			"AmbientSignal :" + String(sensor.ranging_data.ambient_count_rate_MCPS)+ "," 
		);
	}
	delay(200);
}

void mesureSysteme(void)
{
	if (millis() - previousMesure > 500)
	{
		
		previousMesure = millis();
		sensor.read();
		
		NiveauEtang = sensor.ranging_data.range_mm;
	}
	if (digitalRead(PRGButton) == LOW)
	{
		if (displayMode < 2)
		{
			displayMode++;
		}
		else
		{
			displayMode = 0;
		}
	}

	
}