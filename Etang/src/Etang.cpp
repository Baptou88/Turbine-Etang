/*
 Name:		Etang.ino
 Created:	21/05/2021 18:29:42
 Author:	Baptou
*/


#include <EEPROM.h>
#include <heltec.h>
#include <VL53L1X.h>
#include <TurbineEtangLib.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_INA219.h>
// #include <esp_adc_cal.h>
// #include <driver/adc.h>


#define PRGButton 0
#define TAILLETAB 16


#define EPRGBUTTON 0


Adafruit_BMP280 bmp; // I2C

Adafruit_INA219 ina;

// battery reader
//esp_adc_cal_characteristics_t *adc_chars;
uint16_t voltage;
#define DEFAULT_VREF            1100    // Default VREF use if no e-fuse calibration
#define VBATT_SAMPLE            500     // Battery sample rate in ms
#define VBATT_SMOOTH            50      // Number of averages in sample
#define ADC_READ_STABILIZE      5       // in ms (delay from GPIO control and ADC connections times)
#define VBATT_GPIO              21      // Heltec GPIO to toggle VBatt read connection ... WARNING!!! This also connects VEXT to VCC=3.3v so be careful what is on header.  Also, take care NOT to have ADC read connection in OPEN DRAIN when GPIO goes HIGH
#define HELTEC_V2_1             1       // Set this to switch between GPIO13(V2.0) and GPIO37(V2.1) for VBatt ADC.
#define VOLTAGE_DIVIDER         3.20    // Lora has 220k/100k voltage divider so need to reverse that reduction via (220k+100k)/100k on vbat GPIO37 or ADC1_1 (early revs were GPIO13 or ADC2_4 but do NOT use with WiFi.begin())
#define MAXBATT                 4200    // The default Lipo is 4200mv when the battery is fully charged.
#define LIGHT_SLEEP_VOLTAGE     3400  //3750    // Point where start light sleep
#define MINBATT                 3300  //3200  // The default Lipo is 3200mv when the battery is empty...this WILL be low on the 3.3v rail specs!!!
#define LO_BATT_SLEEP_TIME      10*60*1000*1000     // How long when low batt to stay in sleep (us)

float temp = 0;
float pressure = 0;
byte displayMode = 0;
VL53L1X vl53l1x;
long timingBudget = 180000;

bool previousEtatButton = false;

Message receivedMessage;
bool newMessage = false ;

int NiveauEtang = 0;
unsigned long previousMesure = 0;
unsigned long previousSend = 0;

byte localAddress = Etang;
byte msgCount = 0;
int NiveauMax = 0;
int NiveauMin = 1000;

const int longeurEtang = 12000;
const int largeurEtang= 6000;

bool EnvoyerStatut = false;

// déclaration des tableaux 
byte Etape[TAILLETAB];
int Transition[TAILLETAB];
int Entree[TAILLETAB];
int Entree_1[TAILLETAB];
int FrontMontant[TAILLETAB];
int Sortie[TAILLETAB];
unsigned long Tempo[TAILLETAB];

Preferences preferences;




enum Etapes
{
	Init,
	Start,
	EcranOn,
	MiseEnVeille,
	Sleep,
	Reveil,
	EcranSuivant,
	

};
void InitTableau(void) {
	int I;
	for (I = 0; I < TAILLETAB ; I++) {
		Etape[I] = 0;
		Transition[I] = 0;
		Entree[I] = 0;
		Entree_1[I] = 0;
		FrontMontant[I] = 0;
		Sortie[I] = 0;
		Tempo[I] = 0;
	}
}
void gestionFrontMontant(void) {
	int I;
	for (I = 0;I < TAILLETAB;I++) {
		FrontMontant[I] = 0;
		if (Entree[I] > Entree_1[I])FrontMontant[I] = 1;
		Entree_1[I] = Entree[I];
	}
}
void startTempo(int NumTempo, int Duree) {
	if (Tempo[NumTempo] == 0 ) { // on test si la tempo n'est pas déjà lancé
		Tempo[NumTempo] = millis() + Duree;  // on stok dans la tempo la valeur que devrat contenir millis() à la fin de la tempo
	}
}
void gestionTempo(void) {
	int I;
	for (I = 0;I < TAILLETAB;I++) {
		if (Tempo[I] != 0) {
			if (Tempo[I] < millis()) Tempo[I] = 0;   // si millis() et supérieur à la valeur memorisé dans la tempo la tempo est terminé 
													 // donc on la force à 0
		}
	}
}
bool finTempo(int numTempo) {
	if (Tempo[numTempo] == 0) {
		return(true);
	}
	else {
		return(false);
	}
}
void acquisitionEntree(void) {
	Entree[EPRGBUTTON] = digitalRead(PRGButton)? false : true;
	
}
void miseAjourSortie(void) {
	
}

// fonction d'arrêt d'une tempo
void stopTempo(int numTempo) {
	Tempo[numTempo] = 0;
}
void debugGrafcet(void) {
	 Serial.print("Etape : ");
	 for (byte i = 0; i < TAILLETAB; i++)
	 {
		 Serial.print(Etape[i]);
	 }
	 Serial.println("-");
	 Serial.print("Entree: ");
	 for (byte i = 0; i < TAILLETAB; i++)
	 {
		 Serial.print(Entree[i]);
	 }
	 Serial.println("-");

	 Serial.print("FM    : ");
	 for (byte i = 0; i < TAILLETAB; i++)
	 {
		 Serial.print(FrontMontant[i]);
	 }
	 Serial.println("-");
	 Serial.print("Sortie: ");
	 for (byte i = 0; i < TAILLETAB; i++)
	 {
		 Serial.print(Sortie[i]);
	 }
	 Serial.println("-");
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
void AfficherNiveauJauge(void)
{
	Heltec.display->drawRect(115, 4, 10, 60);
	int pourcenta = 60 - (60 - 10) * PNiveau();
	Heltec.display->fillRect(115, pourcenta, 10, 60);
}


void drawBattery(uint16_t voltage, bool sleep) {
  	Heltec.display->setColor(OLEDDISPLAY_COLOR::BLACK);
	Heltec.display->fillRect(99,0,29,24);

	Heltec.display->setColor(OLEDDISPLAY_COLOR::WHITE);
	Heltec.display->drawRect(104,0,12,6);
	Heltec.display->fillRect(116,2,1,2);

	uint16_t v = voltage;
	if (v < MINBATT) {v = MINBATT;}
	if (v > MAXBATT) {v = MAXBATT;}
	double pct = map(v,MINBATT,MAXBATT,0,100);
	uint8_t bars = round(pct / 10.0);
	Heltec.display->fillRect(105,1,bars,4);
	Heltec.display->setFont(ArialMT_Plain_10);
	Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
	// Draw small "z" when using sleep
	if (sleep > 0) {
		Heltec.display->drawHorizontalLine(121,0,4);
		Heltec.display->drawHorizontalLine(121,5,4);
		Heltec.display->setPixel(124,1);
		Heltec.display->setPixel(123,2);
		Heltec.display->setPixel(122,3);
		Heltec.display->setPixel(121,4);
	}
	Heltec.display->drawString(127,5,String((int)round(pct))+"%");
	Heltec.display->drawString(127,14,String(round(voltage/10.0)/100.0)+"V");
	#if defined(__DEBUG) && __DEBUG > 0
	static uint8_t c = 0;
	if ((c++ % 10) == 0) {
		c = 1;
		Serial.printf("VBAT: %dmV [%4.1f%%] %d bars\n", voltage, pct, bars);
	}
	#endif
	Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void displayData(void)
{
	
	switch (displayMode)
	{
	case 0:
		Heltec.display->drawString(0, 10, "Niveau: " + String(NiveauEtang));
		Heltec.display->drawString(0, 22, "Ratio: " + String(PNiveau()*100) + "%");
		Heltec.display->drawString(0, 40, vl53l1x.rangeStatusToString(vl53l1x.ranging_data.range_status));
		AfficherNiveauJauge();
		
		Heltec.display->drawString(60, 2,"Max:" + String(NiveauMax));
		Heltec.display->drawString(60, 52,"Min:" + String(NiveauMin));
		break;
	case 1:
		Heltec.display->drawString(0, 0, "Range   " + String(vl53l1x.ranging_data.range_mm));
		Heltec.display->drawString(0, 10, "Status  " + String(vl53l1x.rangeStatusToString(vl53l1x.ranging_data.range_status)));
		Heltec.display->drawString(0, 20, "Ambient " + String(vl53l1x.ranging_data.ambient_count_rate_MCPS));
		Heltec.display->drawString(0, 30, "Peak    " + String(vl53l1x.ranging_data.peak_signal_count_rate_MCPS));
		Heltec.display->drawString(0, 40, "TimingBudget: " + String(vl53l1x.getMeasurementTimingBudget()));
		Heltec.display->drawString(0, 50, "Roi center: " + String(vl53l1x.getROICenter() ));
		break;
	case 2 :
		Heltec.display->drawLogBuffer(0,0);
		break;
	
	case 3:
		drawBattery(voltage, voltage < LIGHT_SLEEP_VOLTAGE);
		Heltec.display->drawString(0,0,"0x"+ String(localAddress,HEX));
		Heltec.display->drawString(0,40,(String) + receivedMessage.RSSI + " dbm");
		break;
	default:
		Heltec.display->drawString(0, 10, "erreur indice affichage" );
		break;
	}
	
	
}
void EnvoyerMsgStatut(void){
	StaticJsonDocument<128> doc;
	String json; 
	doc["mesure"] = vl53l1x.ranging_data.range_mm;
	doc["Niveau"] = PNiveau();
	doc["RangeStatus"] = VL53L1X::rangeStatusToString( vl53l1x.ranging_data.range_status);
	doc["distanceMode"] = vl53l1x.getDistanceMode();
	doc["minEtang"] = NiveauMin;
	doc["maxEtang"] = NiveauMax;
	doc["temp"] = temp;
	doc["pressure"] = pressure;
	
	serializeJson(doc,json);
	Serial.println("voila ce que jenvoi: "+ String(json));
	
	sendMessage(0x0A, json);
	LoRa.receive();
	Serial.println("message envoyé");
}
void EvolutionGraphe(void) {

	// calcul des transitions

	Transition[0] = Etape[Init];
	Transition[1] = Etape[Start];
	Transition[2] = Etape[EcranOn] && FrontMontant[EPRGBUTTON];
	Transition[3] = Etape[EcranSuivant];
	Transition[4] = Etape[EcranOn] && finTempo(0);
	Transition[5] = Etape[Etapes::Sleep] && FrontMontant[EPRGBUTTON];
	Transition[6] = Etape[Etapes::Reveil];
	Transition[7] = Etape[MiseEnVeille] && finTempo(1);

	// Desactivation des etapes
	if (Transition[0]) Etape[Init] = 0;
	if (Transition[1]) Etape[Start] = 0;
	if (Transition[2]) Etape[EcranOn] = 0;
	if (Transition[3]) Etape[EcranSuivant] = 0;
	if (Transition[4]) Etape[EcranOn] = 0;
	if (Transition[5]) Etape[Sleep] = 0;
	if (Transition[6]) Etape[Reveil] = 0;
	if (Transition[7]) Etape[MiseEnVeille] = 0;

	// Activation des etapes
	if (Transition[0]) Etape[Start] = 1;
	if (Transition[1]) Etape[EcranOn] = 1;
	if (Transition[2]) Etape[EcranSuivant] = 1;
	if (Transition[3]) Etape[EcranOn] = 1;
	if (Transition[4]) Etape[Etapes::MiseEnVeille] = 1;
	if (Transition[5]) Etape[Etapes::Reveil] = 1;
	if (Transition[6]) Etape[Etapes::EcranOn] = 1;
	if (Transition[7]) Etape[Sleep] = 1;

	// Gestion des actions sur les etapes
	
	if (Etape[Init])
	{
		//Serial.println("Etape Init");
	}
	
	if (Etape[Start])
	{
		//Serial.println("Etape Start");
		startTempo(0,60000);
	}
	
	if (Etape[EcranOn])
	{
		//Serial.println("Etape Ecran On");
		Heltec.display->clear();
		displayData();
		Heltec.display->display();
	}
	if (Etape[EcranSuivant])
	{
		//Serial.println("Etape Ecran Suivant");
		if (displayMode> 3)
		{
			displayMode =0;
		} else
		{
			displayMode++;
		}
		
		
		
		Tempo[0] = millis() + 30000;
	}
	if (Etape[MiseEnVeille])
	{
		startTempo(1,5000);
		Heltec.display->clear();
		displayData();
		
		Heltec.display->setColor(OLEDDISPLAY_COLOR::BLACK);
		
		Heltec.display->fillRect(5,30,114,34);		
		
		Heltec.display->setColor(OLEDDISPLAY_COLOR::WHITE);
		Heltec.display->drawRect(4,29,115,35);	
		Heltec.display->drawString(20,40,"Mise en Veille");
		
		Heltec.display->display();

	}
	
	if (Etape[Sleep])
	{
		//Serial.println("Mise en Veille");
		//Heltec.display->displayOff();
		//Heltec.display->clear();
		Heltec.display->sleep();
		//Heltec.VextOFF();
		
	}
	if (Etape[Reveil])
	{
		//Serial.println("Reveil");
		//Heltec.display->displayOn();
		Heltec.VextON();
		Heltec.display->wakeup();
		//Heltec.display->clear();
		Heltec.display->drawString(0,0,"Ecran On");
		startTempo(0,30000);
	}
	
}


void mesureSysteme(void)
{
	if (millis() - previousMesure > 1000)
	{
		//Serial.println("debut mesure");
		previousMesure = millis();
		
		
		temp = bmp.readTemperature();
		pressure = bmp.readPressure();
		//Serial.println("      mesure");
		
		//Serial.println("fin   mesure");
	}
	if (vl53l1x.dataReady())
	{
		Serial.println("Niveau " + (String)vl53l1x.read(false));
		NiveauEtang = vl53l1x.ranging_data.range_mm;
	}
	

	
}


void initPrefs(void){

	preferences.begin("etang", false );
	
	//preferences.putInt("NiveauMax",16);
	//preferences.putInt("NiveauMin",16);
	NiveauMin = preferences.getInt("NiveauMin",0);
	NiveauMax = preferences.getInt("NiveauMax",0);
	//preferences.putInt("NiveauMax",18);
	Serial.printf("prefs : Min: %u Max: %u", NiveauMin , NiveauMax );
	Serial.println("");
}

void TraitementCommande(String Commande){
	Serial.println("TraitementCommande: " + (String)Commande);
	if (Commande.startsWith("DemandeStatut"))
	{
		Commande.replace("DemandeStatut","");
		EnvoyerStatut = true;
	}
	
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
	if (Commande.startsWith("DeepSleep="))
	{
		Commande.replace("DeepSleep=","");
		Serial.println("DeepSleep " + String(Commande.toInt()));
		//sendMessage(MASTER,"OK");
		vl53l1x.stopContinuous();
		delay(1000);
		//esp_sleep_enable_gpio_wakeup();
		//esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,LOW);
		esp_sleep_enable_timer_wakeup(Commande.toInt()*1000); 
		esp_deep_sleep_start();
	}
	if (Commande.startsWith("LightSleep="))
	{
		Commande.replace("LightSleep=","");
		Serial.println("LightSleep " + String(Commande.toInt()));
		vl53l1x.stopContinuous();
		esp_sleep_enable_timer_wakeup(Commande.toInt()*1000);
		esp_light_sleep_start();
	}
	if (Commande.startsWith("SLEEPTP"))
	{
		
		Serial.println("Mise en Veille");
	
		Heltec.display->println("Mise en Veille");
		delay(1000);
		esp_sleep_enable_touchpad_wakeup();
		esp_deep_sleep_start();
	}
	if (Commande.startsWith("TB"))
	{
		Commande.remove(0,1);
		timingBudget = Commande.toInt();
		vl53l1x.setMeasurementTimingBudget(timingBudget);
	}
	if (Commande.startsWith("DistanceMode="))
	{
		Commande.replace("DistanceMode=","");
		//VL53L1X::DistanceMode dm = Commande.toDouble();
		//DistanceMode dm = Commande;
		switch (Commande.toInt())
		{
		case 0:
			vl53l1x.setDistanceMode(VL53L1X::Short);
			break;
		case 1:
			vl53l1x.setDistanceMode(VL53L1X::Medium);
			break;
		case 2:
			vl53l1x.setDistanceMode(VL53L1X::Long);
			break;
		
		default:
			break;
		}
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
	Serial.println("RSSI: " + String(LoRa.packetRssi()));
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
	receivedMessage.RSSI = LoRa.packetRssi();
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

	newMessage = true;
	
}

// Poll the proper ADC for VBatt on Heltec Lora 32 with GPIO21 toggled
// uint16_t ReadVBatt() {
// 	uint16_t reading = 666;

// 	digitalWrite(VBATT_GPIO, LOW);              // ESP32 Lora v2.1 reads on GPIO37 when GPIO21 is low
// 	delay(ADC_READ_STABILIZE);                  // let GPIO stabilize
// 	#if (defined(HELTEC_V2_1))
// 	pinMode(ADC1_CHANNEL_1, OPEN_DRAIN);        // ADC GPIO37
// 	reading = adc1_get_raw(ADC1_CHANNEL_1);
// 	pinMode(ADC1_CHANNEL_1, INPUT);             // Disconnect ADC before GPIO goes back high so we protect ADC from direct connect to VBATT (i.e. no divider)
// 	#else
// 	pinMode(ADC2_CHANNEL_4, OPEN_DRAIN);        // ADC GPIO13
// 	adc2_get_raw(ADC2_CHANNEL_4,ADC_WIDTH_BIT_12,&reading);
// 	pinMode(ADC2_CHANNEL_4, INPUT);             // Disconnect ADC before GPIO goes back high so we protect ADC from direct connect to VBATT (i.e. no divider
// 	#endif

// 	uint16_t voltage = esp_adc_cal_raw_to_voltage(reading, adc_chars);  
// 	voltage*=VOLTAGE_DIVIDER;

// 	return voltage;
// }


// uint16_t SampleBattery() {
// 	static uint8_t i = 0;
// 	static uint16_t samp[VBATT_SMOOTH];
// 	static int32_t t = 0;
// 	static bool f = true;
// 	if(f){ for(uint8_t c=0;c<VBATT_SMOOTH;c++){ samp[c]=0; } f=false; }   // Initialize the sample array first time
// 	t -= samp[i];   // doing a rolling recording, so remove the old rolled around value out of total and get ready to put new one in.
// 	if (t<0) {t = 0;}

// 	// ADC read
// 	uint16_t voltage = ReadVBatt();

// 	samp[i]=voltage;
// 	#if defined(__DEBUG) && __DEBUG > 0
// 	Serial.printf("ADC Raw Reading[%d]: %d", i, voltage);
// 	#endif
// 	t += samp[i];

// 	if(++i >= VBATT_SMOOTH) {i=0;}
// 	uint16_t s = round(((float)t / (float)VBATT_SMOOTH));
// 	#if defined(__DEBUG) && __DEBUG > 0
// 	Serial.printf("   Smoothed of %d/%d = %d\n",t,VBATT_SMOOTH,s); 
// 	#endif

// 	return s;
// }

// void InitBatteryReader(){
// 	 // Characterize ADC at particular atten
//   #if (defined(HELTEC_V2_1))
//   adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
//   esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
//   adc1_config_width(ADC_WIDTH_BIT_12);
//   adc1_config_channel_atten(ADC1_CHANNEL_1,ADC_ATTEN_DB_6);
//   #else
//   // Use this for older V2.0 with VBatt reading wired to GPIO13
//   adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
//   esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
//   adc2_config_channel_atten(ADC2_CHANNEL_4,ADC_ATTEN_DB_6);
//   #endif

// #if defined(__DEBUG) && __DEBUG > 0
//   Serial.printf("ADC Calibration: ");
//   if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
//       Serial.printf("eFuse Vref\n");
//   } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
//       Serial.printf("Two Point\n");
//   } else {
//       Serial.printf("Default[%dmV]\n",DEFAULT_VREF);
//   }
//  #else
//   if (val_type);    // Suppress warning
//  #endif



//   //#if defined(__DEBUG) && __DEBUG >= 1
//   Serial.printf("ADC Calibration: ");
//   if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
//       Serial.printf("eFuse Vref\n");
//   } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
//       Serial.printf("Two Point\n");
//   } else {
//       Serial.printf("Default[%dmV]\n",DEFAULT_VREF);
//   }
//   //#else
//   if (val_type);    // Suppress warning
//   //#endif

//   // Prime the Sample register
//   for (uint8_t i = 0;i < VBATT_SMOOTH;i++) {
//     SampleBattery();
//   }

//   pinMode(VBATT_GPIO,OUTPUT);
//   digitalWrite(VBATT_GPIO, LOW);              // ESP32 Lora v2.1 reads on GPIO37 when GPIO21 is low
//   delay(ADC_READ_STABILIZE);                  // let GPIO stabilize
// }

// the setup function runs once when you press reset or power the board
void setup() {
	Heltec.begin(true, false, true, true, 868E6);

	SPI.begin(SCK,MISO,MOSI,SS);
	LoRa.setPins(SS,RST_LoRa,DIO0);
	while (!LoRa.begin(868E6))
	{
		Serial.println("erreur lora");
		delay(1000);
	}

	Serial.println(string_wakeup_reason(esp_sleep_get_wakeup_cause()));
	Serial.println(wakeup_touchpad(esp_sleep_get_touchpad_wakeup_status()));
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(PRGButton, INPUT);

	//bmp280
	while (!bmp.begin(0x76)) {
		Heltec.display->drawString(0, 12, "Failed init bmp280");
		Heltec.display->display();
		delay(1000);
  	} 

	Heltec.display->drawString(0, 12, "ok init bmp280");
	
	//Vl53L1X
	vl53l1x.setTimeout(500);
	Heltec.display->drawString(20, 24, "Init vl53l1x");
	while (!vl53l1x.init())
	{
		Heltec.display->drawString(0, 24, "X");
		Serial.println("Failed init VL53L1X");
		Heltec.display->display();
		delay(1000);
		
	}
	
	Heltec.display->drawString(0, 24, "OK");
	Serial.println("Ok init VL53L1X");
	



	
	//InitBatteryReader();

	vl53l1x.setDistanceMode(VL53L1X::Long);
	vl53l1x.setMeasurementTimingBudget(15000);//50000
	
	// Start continuous readings at a rate of one measurement every 50 ms (the
	// inter-measurement period). This period should be at least as long as the
	// timing budget.
	vl53l1x.startContinuous(500);
	
	// vl53l1x.setROICenter(64);
	vl53l1x.setROISize(5,5);
	
	LoRa.setSpreadingFactor(8);
	delay(10);
	LoRa.setSyncWord(0x12);
	delay(10);
	LoRa.setSignalBandwidth(125E3);
	delay(10);

	LoRa.onReceive(onReceive);
	LoRa.receive();

	

	Heltec.display->setLogBuffer(10,30);
	//initEEPROM();
	initPrefs();
	
		
	Heltec.display->display();

	// forçage de l'étape initiale
	Etape[Init] = 1;

	delay(1000);
	Heltec.display->clear();
}

// the loop function runs over and over again until power down or reset
void loop() {

	
	mesureSysteme();
	//displayData();
	


	//voltage = SampleBattery();

	// // if (voltage < MINBATT) {                  // Low Voltage cut off shut down to protect battery as long as possible
	// // 	Heltec.display->setColor(OLEDDISPLAY_COLOR::WHITE);
	// // 	Heltec.display->setFont(ArialMT_Plain_10);
	// // 	Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
	// // 	Heltec.display->drawString(64,24,"Shutdown!!");
	// // 	Heltec.display->drawString(64,36,(String)voltage);
	// // 	Heltec.display->display();
	// // 	delay(2000);
	// // 	#if defined(__DEBUG) && __DEBUG > 0
	// // 	Serial.printf(" !! Shutting down...low battery volotage: %dmV.\n",voltage);
	// // 	delay(10);
	// // 	#endif
	// // 	esp_sleep_enable_timer_wakeup(LO_BATT_SLEEP_TIME);
	// // 	esp_deep_sleep_start();
	// // } else if (voltage < LIGHT_SLEEP_VOLTAGE) {     // Use light sleep once on battery
	// // 	uint64_t s = VBATT_SAMPLE;
	// // 	#if defined(__DEBUG) && __DEBUG > 0
	// // 	Serial.printf(" - Light Sleep (%dms)...battery volotage: %dmV.\n",(int)s,voltage);
	// // 	delay(20);
	// // 	#endif
	// // 	esp_sleep_enable_timer_wakeup(s*1000);     // Light Sleep does not flush buffer
	// // 	esp_light_sleep_start();
	// // }

	// if (millis() - previousSend > 5000)
	// {
	// 	previousSend = millis();
	// 	//sendData();
	// 	StaticJsonDocument<128> doc;
	// 	String json; 
	// 	doc["mesure"] = vl53l1x.ranging_data.range_mm;
	// 	doc["Niveau"] = PNiveau();
	// 	doc["RangeStatus"] = VL53L1X::rangeStatusToString( vl53l1x.ranging_data.range_status);
	// 	doc["temp"] = temp,
	// 	doc["pressure"] = pressure;
		
	// 	serializeJson(doc,json);
	// 	//Serial.println("voila ce que jenvoi: "+ String(json));
	// 	// sendMessage(0x0A, "Niveau: " + String(PNiveau())+ "," +
	// 	// 	"RangeStatus :" + VL53L1X::rangeStatusToString( vl53l1x.ranging_data.range_status) + "," +
	// 	// 	"PeakSignal :" + String(vl53l1x.ranging_data.peak_signal_count_rate_MCPS)+ "," +
	// 	// 	"AmbientSignal :" + String(vl53l1x.ranging_data.ambient_count_rate_MCPS)+ "," 
	// 	// );
	// 	sendMessage(0x0A, json);
	// 	LoRa.receive();
	// }
	if (newMessage)
	{
		newMessage = false;
		TraitementCommande(receivedMessage.Content);
	}
	
	if (EnvoyerStatut)
	{
		EnvoyerMsgStatut();
		EnvoyerStatut = false;
	}
	
	if (Serial.available())
	{
		String s = Serial.readStringUntil('\r');
		TraitementCommande(s);
		
		
	}

	

	// acquisition des entrées et stockage dans variables internes
	acquisitionEntree();

	// detecttion des fronts monatnts
	gestionFrontMontant();
	// gestion des tempos
	gestionTempo();

	// evolution du grafcet
	EvolutionGraphe();

	
	
	// mise à jour des sorties
	miseAjourSortie();
	
	
	//debugGrafcet();
	delay(20);
}

