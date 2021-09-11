/*
 Name:		Turbine.ino
 Created:	21/05/2021 18:29:24
 Author:	Baptou
*/

#include <Arduino.h>
#include <heltec.h>
#include "TurbineEtangLib.h"
#include <Preferences.h>
#include <ArduinoJSON.h>
#include <Adafruit_INA260.h>


#define BAND 868E6
#define PRGButton 0

#define TAILLETAB 24

//entree
#define FCVanneOuverte 0
#define FCVanneFerme 1
#define EPRGBUTTON 2

//sortie
#define OuvertureVanne 0
#define FermetureVanne 1

byte localAddress = 0x0B;
byte msgCount = 0;

//encodeur
#define pinEncodA  36
#define pinEncodB  37
#define pinTaqui 32


//intensite
Adafruit_INA260 ina260 = Adafruit_INA260();
double currentValue = 0;
unsigned long previousMesureIntensite = 0;
int maxIntensite = 4000; //mA

String StatutVanne = "Arret";

Preferences preferences;

const int pinOuvertureVanne = 12;
const int pinFermetureVanne = 13;
const byte pinFCVanneOuverte = 39;
const byte pinFCVanneFermee = 38;
volatile long countEncodA = 0;
volatile long countEncodB = 0;

// Taquimetre
unsigned long previousMillisTaqui = 0;
volatile unsigned long countTaqui = 0;
float rpmTurbine = 0;
unsigned long previousCalculTaqui = 0;




float tourMoteurVanne = 18 / float(44); 

unsigned long dernieredetectionEncodA = 0;

bool bOuvertureTotale = false;
bool bFermetureTotale = false;
Message receivedMessage;
int incCodeuse = 600;
long ouvertureMax = 1800;
byte displayMode = 0;
long consigneMoteur = 0;
volatile int sensMoteur = 0;
long posMoteur;
bool previousEtatbutton = false;
//Automate

enum Etapes
{
	Init,
	DelaiPOM,
	AttenteOrdre,
	TraitementMessage,
	OuvrirVanne,
	FermerVanne,
	StopMoteur,
	StopMoteurIntensite,
	EnvoyerMessage,
	OuvertureTotale,
	FermetureTotale,
	POMO,
	POMF,
	STOPPOMF,
	STOPPOMO,
};
String EtapeToString(Etapes E) {
	switch (E)
	{
	case Init:
		return "Init";
		break;
	case DelaiPOM:
		return "delaipom";
		break;
	case AttenteOrdre:
		return "AttendeOrdre";
		break;
	case TraitementMessage:
		return "TraitementMessage";
		break;
	case OuvrirVanne:
		return "OuvrirVanne";
		break;
	case FermerVanne:
		return "FermerVanne";
		break;
	case StopMoteur:
		return "StopMoteur";
		break;
	case EnvoyerMessage:
		return "EnvoiMsge";
		break;
	case OuvertureTotale:
		return "OuvertureTotale";
		break;
	case FermetureTotale:
		return "FermetureTotale";
		break;
	case StopMoteurIntensite:
		return "StopMoteurIntensite";
		break;
	case POMO:
		return "POMO";
		break;
	case POMF:
		return "POMF";
		break;
	case STOPPOMF:
		return "STOPPOMF";
		break;
	case STOPPOMO:
		return "STOPPOMO";
		break;
	default:
		return "default";
		break;
	}
}
Etapes EtapeActuel;
Etapes SousEtapeActuel;

// déclaration des tableaux 
byte Etape[TAILLETAB];
int Transition[TAILLETAB];
int Entree[TAILLETAB];
int Entree_1[TAILLETAB];
int FrontMontant[TAILLETAB];
int Sortie[TAILLETAB];
unsigned long Tempo[TAILLETAB];



//conversion degrés moteur en increment
int degToInc(int degres) {
	return ((degres * incCodeuse) / 360);
}
//conversion degrés vanne en increment
int degvanneToInc(int degVanne){
	return degToInc(degVanne) / tourMoteurVanne;
}
//Asservissement Moteur
void asservissementMoteur() {
	
	if (sensMoteur>0)
	{
		consigneMoteur -= countEncodA;
		//posMoteur += countEncodA;
	}
	else if (sensMoteur < 0) {
		consigneMoteur += countEncodA;
		//posMoteur -= countEncodA;
	}
	//consigneMoteur -= countEncodA;
	countEncodA = 0;
}

//pourcentage ouverture vanne
float pPosMoteur(){
	return  float(posMoteur) / float(ouvertureMax);
}
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
	if (Tempo[NumTempo] == 0) { // on test si la tempo n'est pas déjà lancé
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
float mesureTaqui(void){
	float rpm = countTaqui * 60000 / float((millis() - previousMillisTaqui));
	countTaqui = 0;
	previousMillisTaqui = millis();

	return rpm;
}
void mesureIntensite(void){
	// int adcValue = analogRead(pinIntensite);

	// double  adcVoltage = (adcValue / 1024.0) * 5000;
	// currentValue = ((adcVoltage - 2500) / 66);  //int offsetVoltage = 2500;
	if (millis()> previousMesureIntensite + 200)
	{
		previousMesureIntensite = millis();
		currentValue = ina260.readCurrent();
		
	}
	
	
}
void acquisitionEntree(void) {
	Entree[FCVanneOuverte] = digitalRead(pinFCVanneOuverte) ? true :false ;
	Entree[FCVanneFerme] = digitalRead(pinFCVanneFermee) ? true : false;
	Entree[EPRGBUTTON] = digitalRead(PRGButton)? true : false;
	mesureIntensite();
	if (millis()> previousCalculTaqui + 2000)
	{
		previousCalculTaqui = millis();
		rpmTurbine = mesureTaqui();
	}
}
void TraitementCommande(String c){
	if (c.startsWith("M"))
	{
		c.remove(0,1);
		consigneMoteur = c.toInt();
		if (consigneMoteur > 0)
		{
			sensMoteur = 1;
		}
		else
		{
			sensMoteur = -1;
		}
		/*consigneMoteur = abs(consigneMoteur);*/
	}
	if (c.startsWith("D"))
	{
		c.remove(0, 1);
		consigneMoteur = degToInc(c.toInt());
		Serial.println(consigneMoteur);
		if (consigneMoteur > 0)
		{
			sensMoteur = 1;
		}
		else
		{
			sensMoteur = -1;
		}
		/*consigneMoteur = abs(consigneMoteur);*/
	}
	
	if (c.startsWith("OT"))
	{
		c.remove(0, 2);
		bOuvertureTotale = true;
	}
	if (c.startsWith("FT"))
	{
		c.remove(0, 2);
		bFermetureTotale = true;
	}
	if (c == "SMIN")
	{
		posMoteur = 0;
	}
	if (c == "SMAX")
	{
		ouvertureMax = posMoteur;
	}
	if (c == "SetMaxI")
	{
		c.remove(0,6);
		maxIntensite = c.toInt();
		preferences.putInt("maxIntensite",maxIntensite);
		
	}
	if (c == "ouvertureMax")
	{
		c.remove(0,12);
		ouvertureMax = c.toInt();
		preferences.putInt("ouvertureMax",ouvertureMax);
		
	}
	
	
}
void EvolutionGraphe(void) {

	// calcul des transitions

	Transition[0] = Etape[Init];
	Transition[1] = Etape[DelaiPOM] && Entree[EPRGBUTTON] == false /*&& FrontMontant[BP]*/;
	Transition[2] = Etape[DelaiPOM] && (Entree[FCVanneFerme] == true) && finTempo(2);
	Transition[3] = Etape[DelaiPOM] && (Entree[FCVanneFerme] == false) && finTempo(2);
	Transition[4] = Etape[POMO] && (Entree[FCVanneFerme] == false);
	Transition[5] = Etape[POMF] && (Entree[FCVanneFerme] == true);
	Transition[6] = Etape[STOPPOMF] ;
	Transition[7] = Etape[AttenteOrdre] && consigneMoteur < 0;
	Transition[8] = Etape[AttenteOrdre] && consigneMoteur > 0;
	Transition[9] = Etape[OuvrirVanne] && (consigneMoteur < 0 || Entree[FCVanneOuverte]);
	Transition[10] = Etape[FermerVanne] && (consigneMoteur > 0 || Entree[FCVanneFerme]);
	Transition[11] = Etape[StopMoteur] && finTempo(0);
	Transition[12] = Etape[AttenteOrdre] && finTempo(1);
	Transition[13] = Etape[EnvoyerMessage] ;
	Transition[14] = Etape[AttenteOrdre] && bOuvertureTotale;
	Transition[15] = Etape[AttenteOrdre] && bFermetureTotale;
	Transition[16] = Etape[OuvertureTotale] && (Entree[FCVanneOuverte] == true);
	Transition[17] = Etape[FermetureTotale] && (Entree[FCVanneFerme] == true);
	Transition[18] = Etape[OuvrirVanne] && currentValue> maxIntensite;
	Transition[19] = Etape[FermerVanne] && currentValue> maxIntensite;
	Transition[20] = Etape[OuvertureTotale] && currentValue> maxIntensite;
	Transition[21] = Etape[FermetureTotale] && currentValue> maxIntensite;

	// Desactivation des etapes
	if (Transition[0]) Etape[Init] = 0;
	if (Transition[1]) Etape[DelaiPOM] = 0;
	if (Transition[2]) Etape[DelaiPOM] = 0;
	if (Transition[3]) Etape[DelaiPOM] = 0;
	if (Transition[4]) Etape[POMO] = 0;
	if (Transition[5]) Etape[POMF] = 0;
	if (Transition[6]) Etape[STOPPOMF] = 0;
	if (Transition[7]) Etape[AttenteOrdre] = 0;
	if (Transition[8]) Etape[AttenteOrdre] = 0;
	if (Transition[9]) Etape[OuvrirVanne] = 0;
	if (Transition[10]) Etape[FermerVanne] = 0;
	if (Transition[11]) Etape[StopMoteur] = 0;
	if (Transition[12]) Etape[AttenteOrdre] = 0;
	if (Transition[13]) Etape[EnvoyerMessage] = 0;
	if (Transition[14]) Etape[AttenteOrdre] = 0;
	if (Transition[15]) Etape[AttenteOrdre] = 0;
	if (Transition[16]) Etape[OuvertureTotale] = 0;
	if (Transition[17]) Etape[FermetureTotale] = 0;
	if (Transition[18]) Etape[OuvrirVanne] = 0;
	if (Transition[19]) Etape[FermerVanne] = 0;
	if (Transition[20]) Etape[OuvertureTotale] = 0;
	if (Transition[21]) Etape[FermetureTotale] = 0;

	// Activation des etapes
	if (Transition[0]) Etape[DelaiPOM] = 1;
	if (Transition[1]) Etape[AttenteOrdre] = 1;
	if (Transition[2]) Etape[POMO] = 1;
	if (Transition[3]) Etape[POMF] = 1;
	if (Transition[4]) Etape[POMF] = 1;
	if (Transition[5]) Etape[STOPPOMF] = 1;
	if (Transition[6]) Etape[AttenteOrdre] = 1;
	if (Transition[7]) Etape[FermerVanne] = 1;
	if (Transition[8]) Etape[OuvrirVanne] = 1;
	if (Transition[9]) Etape[StopMoteur] = 1;
	if (Transition[10]) Etape[StopMoteur] = 1;
	if (Transition[11]) Etape[AttenteOrdre] = 1;
	if (Transition[12]) Etape[EnvoyerMessage] = 1;
	if (Transition[13]) Etape[AttenteOrdre] = 1;
	if (Transition[14]) Etape[OuvertureTotale] = 1;
	if (Transition[15]) Etape[FermetureTotale] = 1;
	if (Transition[16]) Etape[StopMoteur] = 1;
	if (Transition[17]) Etape[StopMoteur] = 1;
	if (Transition[18]) Etape[StopMoteurIntensite] =1;
	if (Transition[19]) Etape[StopMoteurIntensite] =1;
	if (Transition[20]) Etape[StopMoteurIntensite] =1;
	if (Transition[21]) Etape[StopMoteurIntensite] =1;

	// Gestion des actions sur les etapes
	/*if (Etape[START] == 1) {
		startTempo(0, 5000);
		Sortie[LED] = 1;
	}*/
	if (Etape[Init])
	{
		EtapeActuel = Init;
		startTempo(2,5000);
	}
	if (Etape[DelaiPOM])
	{
		EtapeActuel = DelaiPOM;
		startTempo(2,5000);
	}
	

	if (Etape[AttenteOrdre] == 1)
	{
		startTempo(1,10000);
		EtapeActuel = AttenteOrdre;
		if (Serial.available())
		{
			String s = Serial.readStringUntil('\r');
			TraitementCommande(s);
			
		}
	}
	if (Etape[FermerVanne] == 1)
	{
		EtapeActuel = FermerVanne;
		StatutVanne = "Fermeture";
		Sortie[FermetureVanne] = 1;
		asservissementMoteur();
	}

	if (Etape[OuvrirVanne] == 1)
	{
		EtapeActuel = OuvrirVanne;
		StatutVanne = "Ouverture";
		Sortie[OuvertureVanne] = 1;
		asservissementMoteur();
	}

	if (Etape[StopMoteur])
	{
		consigneMoteur = 0;
		StatutVanne = "Arret";
		startTempo(0, 1000);
		EtapeActuel = StopMoteur;
		Sortie[OuvertureVanne] = 0;
		Sortie[FermetureVanne] = 0;
		
	}
	if (Etape[EnvoyerMessage])
	{
		EtapeActuel = EnvoyerMessage;
		StaticJsonDocument<128> doc;
		String json; 
		doc["Ouverture"] = pPosMoteur();
		doc["Taqui"] = rpmTurbine;
		doc["StatutVanne"] = StatutVanne;
		doc["OuvCodeur"] = posMoteur;
		doc["OuvMaxCodeur"] = ouvertureMax;
		serializeJson(doc,json);
		Serial.println(json);
		sendMessage(MASTER, json);
		LoRa.receive();
		
	}
	if (Etape[OuvertureTotale])
	{
		sensMoteur = 1;
		EtapeActuel = OuvertureTotale;
		StatutVanne = "Ouverture";
		bOuvertureTotale = false;
		Sortie[OuvertureVanne] = 1;
		
	}
	if (Etape[FermetureTotale])
	{
		sensMoteur = -1;
		StatutVanne = "Fermeture";
		EtapeActuel = FermetureTotale;
		bFermetureTotale = false;
		
		Sortie[FermetureVanne] = 1;
	}
	if (Etape[StopMoteurIntensite])
	{
		EtapeActuel = StopMoteurIntensite;
		StatutVanne = "Arret Intensite";
		Sortie[OuvertureVanne] = 0;
		Sortie[FermetureVanne] = 0;



		StaticJsonDocument<96> doc;
		String json; 
		doc["Ouverture"] = pPosMoteur();
		doc["Taqui"] = rpmTurbine;
		doc["StatutVanne"] = StatutVanne;
		serializeJson(doc,json);
		Serial.println(json);
		sendMessage(MASTER, json);

	}
	if (Etape[POMO])
	{
		EtapeActuel = POMO;
		Sortie[OuvertureVanne] = 1;
		
	}
	if (Etape[POMF])
	{
		EtapeActuel = POMF;
		Sortie[OuvertureVanne] = 0;
		Sortie[FermetureVanne] = 1;
	}
	if (Etape[STOPPOMF])
	{
		EtapeActuel = STOPPOMF;
		Sortie[OuvertureVanne] = 0;
		Sortie[FermetureVanne] = 0;
		posMoteur = 0;
		countEncodA = 0;
	}
	if (Etape[STOPPOMO])
	{
		EtapeActuel = STOPPOMO;
		Sortie[OuvertureVanne] = 0;
		Sortie[FermetureVanne] = 0;
		ouvertureMax = posMoteur;
	}

	
}

void miseAjourSortie(void) {
	digitalWrite(pinOuvertureVanne, Sortie[OuvertureVanne]);
	digitalWrite(pinFermetureVanne, Sortie[FermetureVanne]);
}
// fonction d'arrêt d'une tempo
void stopTempo(int numTempo) {
	Tempo[numTempo] = 0;
}

 void EncodA() {
	 if (millis()> dernieredetectionEncodA + 1)
	 {
		 dernieredetectionEncodA = millis();
		 countEncodA += 1;
		 
		 if (sensMoteur > 0)
		 {
			posMoteur++;

		 }
		 else if (sensMoteur < 0)
		 {
			posMoteur = posMoteur -1;

		 }
	 }
	 
}

		
void EncodB() {
	 countEncodB += 1;
}

 void displayData() {
	Heltec.display->clear();
	if (EtapeActuel == DelaiPOM)
	{
		
		Heltec.display->drawString(0,30,"Prise Origine");
		Heltec.display->drawString(0,20,"debut "+ String(Tempo[2] - millis()));
		
	} else {
		switch (displayMode)
		{
		case 0:
			Heltec.display->drawString(0, 0, "EA: " + String(countEncodA));
#ifdef pinEncodB
			Heltec.display->drawString(64, 0, "EB: " + String(countEncodB));
#endif // pinEncodB
			Heltec.display->drawString(0, 20, "posMoteur: " + String(posMoteur));
			Heltec.display->drawString(40, 32, "consigne: " + String(consigneMoteur));
			if (sensMoteur>0)
			{
				Heltec.display->drawString(15, 50, "->");
			}
			else if(sensMoteur < 0)
			{
				Heltec.display->drawString(15, 50, "<-");
			}
			Heltec.display->drawString(25, 50, "Etape: " + String(EtapeToString( EtapeActuel)));
		
			Heltec.display->drawString(5, 55, Entree[FCVanneFerme]? "*" : "");
			Heltec.display->drawString(110, 55, Entree[FCVanneOuverte] ? "*" : "");
			break;
		case 1:
			Heltec.display->drawString(60,0,String(posMoteur));
			Heltec.display->drawString(100,0,String(ouvertureMax));
			
			Heltec.display->drawString(60,50,String(pPosMoteur()));
			Heltec.display->drawProgressBar(5,30,100,10,pPosMoteur()*100);
			break;
		case 2:
			Heltec.display->drawLogBuffer(0,0);
			break;

		case 3:
			#ifdef pinTaqui
				Heltec.display->drawString(0,0,"Taqui");
				Heltec.display->drawString(40,0,String(rpmTurbine));
				Heltec.display->drawString(100,0,"RPM");
				
			#endif
			
			Heltec.display->drawString(0,15,"Intensite: ");
			Heltec.display->drawString(60,15,String(currentValue));
			Heltec.display->drawString(100,15,"mA");
			Heltec.display->drawString(0,28,"MaxI: ");
			Heltec.display->drawString(55,28,String(maxIntensite));
			Heltec.display->drawString(100,28,"mA");
			
			break;

		default:
			break;
		}
		
	 
 	}
	 
	Heltec.display->display();
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
	 Serial.print("Sortie: ");
	 for (byte i = 0; i < TAILLETAB; i++)
	 {
		 Serial.print(Sortie[i]);
	 }
	 Serial.println("-");
 }


    /**************************************************************************/
    /*!
        @brief    analyse du message
        @param    packetSize
                  
        @return   void
    */
    /**************************************************************************/
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

	TraitementCommande(receivedMessage.Content);

	
	
}
void Taqui(void){
 countTaqui++;
}


// the setup function runs once when you press reset or power the board
void setup() {

	Wire1.begin(SDA, SCL); 
	Heltec.begin(true, true, true, true, BAND);
	Heltec.display->setLogBuffer(10,30);
	pinMode(pinFCVanneFermee, INPUT_PULLDOWN );
	pinMode(pinFCVanneOuverte, INPUT_PULLUP);

	pinMode(pinFermetureVanne, OUTPUT);
	pinMode(pinOuvertureVanne, OUTPUT);

	pinMode(pinEncodA, INPUT);
	attachInterrupt(pinEncodA, EncodA,RISING);
#ifdef pinEncodB
	pinMode(pinEncodB, INPUT);
	attachInterrupt(pinEncodB, EncodB,CHANGE);
#endif // pinEncodB

#ifdef pinTaqui
	pinMode(pinTaqui, INPUT);
	attachInterrupt(pinTaqui, Taqui,RISING);
#endif


	if (preferences.begin("Turbine",false))
	{
		ouvertureMax = preferences.getLong("ouvertureMax",ouvertureMax);
		maxIntensite = preferences.getLong("maxIntensite", maxIntensite);
	}
	
	//TODO: essayer de changer  wire1 par wire
	if (!ina260.begin(0x40, &Wire1)) {
		Heltec.display->drawString(0,12,"Couldn't find INA260 chip");
		Serial.println("Couldn't find INA260 chip");
		Heltec.display->display();
		while (1);
	}
	Heltec.display->drawString(0,12,"INA260 ok !");
	Heltec.display->display();
	Serial.println("Found INA260 chip");

	 

	delay(1000);

	Heltec.display->clear();
	// initialisation du grafcet
	InitTableau();
	// forçage de l'étape initiale
	Etape[Init] = 1;
	LoRa.onReceive(onReceive);
	LoRa.receive();

}

// the loop function runs over and over again until power down or reset
void loop() {
	if (Etape[DelaiPOM] == true)
	{
		
	} else
	{
		if ((digitalRead(PRGButton) == LOW) && !previousEtatbutton )
		{
			previousEtatbutton = true;
			if (displayMode < 3 )
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
			previousEtatbutton = false;
		}	
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
	displayData();
	delay(20);
	//debugGrafcet();
}
