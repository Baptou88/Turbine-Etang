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


#define BAND 868E6
#define PRGButton 0

#define TAILLETAB 16

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
#define pinEncodA  37
#define pinEncodB  36
#define pinTaqui 32
#define pinIntensite 33
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


double currentValue = 0;

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
	AttenteOrdre,
	TraitementMessage,
	OuvrirVanne,
	FermerVanne,
	StopMoteur,
	StopMoteurIntensite,
	EnvoyerMessage,
	OuvertureTotale,
	FermetureTotale,
};
String EtapeToString(Etapes E) {
	switch (E)
	{
	case Init:
		return "Init";
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
	default:
		return "default";
		break;
	}
}
Etapes EtapeActuel;
// déclaration des tableaux 
byte Etape[TAILLETAB];
int Transition[TAILLETAB];
int Entree[TAILLETAB];
int Entree_1[TAILLETAB];
int FrontMontant[TAILLETAB];
int Sortie[TAILLETAB];
unsigned long Tempo[TAILLETAB];

//conversion degrés vanne en increment
int degvanneToInc(int degVanne){
	return degToInc(degVanne) / tourMoteurVanne;
}

//conversion degrés moteur en increment
int degToInc(int degres) {
	return ((degres * incCodeuse) / 360);
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
void acquisitionEntree(void) {
	Entree[FCVanneOuverte] = digitalRead(pinFCVanneOuverte) ? true :false ;
	Entree[FCVanneFerme] = digitalRead(pinFCVanneFermee) ? true : false;
	Entree[EPRGBUTTON] = digitalRead(PRGButton)? true : false;
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
}
void EvolutionGraphe(void) {

	// calcul des transitions

	Transition[0] = Etape[Init] /*&& FrontMontant[BP]*/;
	Transition[1] = Etape[AttenteOrdre] && consigneMoteur < 0;
	Transition[2] = Etape[AttenteOrdre] && consigneMoteur > 0;
	Transition[3] = Etape[OuvrirVanne] && (consigneMoteur < 0 || Entree[FCVanneOuverte]);
	Transition[4] = Etape[FermerVanne] && (consigneMoteur > 0 || Entree[FCVanneFerme]);
	Transition[5] = Etape[StopMoteur] && finTempo(0);
	Transition[6] = Etape[AttenteOrdre] && finTempo(1);
	Transition[7] = Etape[EnvoyerMessage] ;
	Transition[8] = Etape[AttenteOrdre] && bOuvertureTotale;
	Transition[9] = Etape[AttenteOrdre] && bFermetureTotale;
	Transition[10] = Etape[OuvertureTotale] && (Entree[FCVanneOuverte] == true);
	Transition[11] = Etape[FermetureTotale] && (Entree[FCVanneFerme] == true);
	

	// Desactivation des etapes
	if (Transition[0]) Etape[Init] = 0;
	if (Transition[1]) Etape[AttenteOrdre] = 0;
	if (Transition[2]) Etape[AttenteOrdre] = 0;
	if (Transition[3]) Etape[OuvrirVanne] = 0;
	if (Transition[4]) Etape[FermerVanne] = 0;
	if (Transition[5]) Etape[StopMoteur] = 0;
	if (Transition[6]) Etape[AttenteOrdre] = 0;
	if (Transition[7]) Etape[EnvoyerMessage] = 0;
	if (Transition[8]) Etape[AttenteOrdre] = 0;
	if (Transition[9]) Etape[AttenteOrdre] = 0;
	if (Transition[10]) Etape[OuvertureTotale] = 0;
	if (Transition[11]) Etape[FermetureTotale] = 0;

	// Activation des etapes
	if (Transition[0]) Etape[AttenteOrdre] = 1;
	if (Transition[1]) Etape[FermerVanne] = 1;
	if (Transition[2]) Etape[OuvrirVanne] = 1;
	if (Transition[3]) Etape[StopMoteur] = 1;
	if (Transition[4]) Etape[StopMoteur] = 1;
	if (Transition[5]) Etape[AttenteOrdre] = 1;
	if (Transition[6]) Etape[EnvoyerMessage] = 1;
	if (Transition[7]) Etape[AttenteOrdre] = 1;
	if (Transition[8]) Etape[OuvertureTotale] = 1;
	if (Transition[9]) Etape[FermetureTotale] = 1;
	if (Transition[10]) Etape[StopMoteur] = 1;
	if (Transition[11]) Etape[StopMoteur] = 1;

	// Gestion des actions sur les etapes
	/*if (Etape[START] == 1) {
		startTempo(0, 5000);
		Sortie[LED] = 1;
	}*/
	if (Etape[Init])
	{
		EtapeActuel = Init;
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
		Sortie[FermetureVanne] = 1;
		asservissementMoteur();
	}

	if (Etape[OuvrirVanne] == 1)
	{
		EtapeActuel = OuvrirVanne;
		Sortie[OuvertureVanne] = 1;
		asservissementMoteur();
	}

	if (Etape[StopMoteur])
	{
		consigneMoteur = 0;
		startTempo(0, 1000);
		EtapeActuel = StopMoteur;
		Sortie[OuvertureVanne] = 0;
		Sortie[FermetureVanne] = 0;
		
	}
	if (Etape[EnvoyerMessage])
	{
		EtapeActuel = EnvoyerMessage;
		StaticJsonDocument<96> doc;
		String json; 
		doc["Ouverture"] = pPosMoteur();
		doc["Taqui"] = rpmTurbine;
		serializeJson(doc,json);
		Serial.println(json);
		sendMessage(MASTER, json);
		LoRa.receive();
		
	}
	if (Etape[OuvertureTotale])
	{
		sensMoteur = 1;
		EtapeActuel = OuvertureTotale;
		bOuvertureTotale = false;
		Sortie[OuvertureVanne] = 1;
		
	}
	if (Etape[FermetureTotale])
	{
		sensMoteur = -1;
		EtapeActuel = FermetureTotale;
		bFermetureTotale = false;
		
		Sortie[FermetureVanne] = 1;
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
float mesureTaqui(void){
	float rpm = countTaqui * 60000 / float((millis() - previousMillisTaqui));
	countTaqui = 0;
	previousMillisTaqui = millis();

	return rpm;
}
 void displayData() {
	 Heltec.display->clear();
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
		#ifdef pinIntensite
			Heltec.display->drawString(5,15,"Intensite: ");
			Heltec.display->drawString(60,15,String(currentValue));
		#endif
		break;

	 default:
		break;
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

void mesureIntensite(void){
	int adcValue = analogRead(pinIntensite);

	double  adcVoltage = (adcValue / 1024.0) * 5000;
	currentValue = ((adcVoltage - 2500) / 66);  //int offsetVoltage = 2500;
}
// the setup function runs once when you press reset or power the board
void setup() {
	Heltec.begin(true, true, true, true, BAND);
	Heltec.display->setLogBuffer(10,30);
	pinMode(pinFCVanneFermee, INPUT_PULLDOWN );
	pinMode(pinFCVanneOuverte, INPUT_PULLUP);

	pinMode(pinFermetureVanne, OUTPUT);
	pinMode(pinOuvertureVanne, OUTPUT);

	pinMode(pinEncodA, INPUT_PULLUP);
	attachInterrupt(pinEncodA, EncodA,CHANGE);
#ifdef pinEncodB
	pinMode(pinEncodB, INPUT);
	attachInterrupt(pinEncodB, EncodB,CHANGE);
#endif // pinEncodB

#ifdef pinTaqui
	pinMode(pinTaqui, INPUT);
	attachInterrupt(pinTaqui, Taqui,RISING);
#endif
#ifdef pinIntensite
	pinMode(pinIntensite,INPUT);
#endif

	if (preferences.begin("Turbine",false))
	{
		ouvertureMax = preferences.getLong("ouvertureMax",ouvertureMax);
	}
	
	 

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
	
	// acquisition des entrées et stockage dans variables internes
	acquisitionEntree();

	// detecttion des fronts monatnts
	gestionFrontMontant();
	// gestion des tempos
	gestionTempo();

	// evolution du grafcet
	EvolutionGraphe();

	//todo changer de place
	mesureIntensite();
	if (millis()> previousCalculTaqui + 2000)
	{
		previousCalculTaqui = millis();
		rpmTurbine = mesureTaqui();
	}
	
	// mise à jour des sorties
	miseAjourSortie();
	displayData();
	delay(20);
	//debugGrafcet();
}
