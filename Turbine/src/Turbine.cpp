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


#include "digitalInput.h"
#include "digitalOutput.h"

#define BAND 868E6
#define PRGButton 0

#define TAILLETAB 24

//entree
// #define FCVanneOuverte 0
// #define FCVanneFerme 1
// #define EPRGBUTTON 2
digitalInput FCVanneOuverte(39,INPUT_PULLDOWN); 
digitalInput FCVanneFermee(38,INPUT_PULLDOWN);
digitalInput* PrgButton= new digitalInput(PRGButton,INPUT_PULLUP);
//sortie
digitalOutput* OuvertureVanne = new digitalOutput(12);
digitalOutput* FermetureVanne = new digitalOutput(13);
// #define OuvertureVanne 0
// #define FermetureVanne 1

byte localAddress = 0x0B;
byte msgCount = 0;

//encodeur
#define pinEncodA  37
#define pinEncodB  36
#define pinTaqui 32


//intensite
Adafruit_INA260 ina260 = Adafruit_INA260();
double currentValue = 0;
unsigned long previousMesureIntensite = 0;
int maxIntensite = 16000; //mA

String StatutVanne = "Arret";

Preferences preferences;


// const int pinOuvertureVanne = 12;
// const int pinFermetureVanne = 13;


// const byte pinFCVanneOuverte = 39;
// const byte pinFCVanneFermee = 38;

volatile long countEncodA = 0;
volatile long countEncodB = 0;

// Taquimetre
unsigned long previousMillisTaqui = 0;
volatile unsigned long countTaqui = 0;
float rpmTurbine = 0;
unsigned long previousCalculTaqui = 0;

bool EnvoyerStatut = false;


float tourMoteurVanne = 18 / float(44); 

unsigned long dernieredetectionEncodA = 0;

bool bOuvertureTotale = false;
bool bFermetureTotale = false;
Message receivedMessage;
bool newMessage = false;
int incCodeuse = 400;

/**
 * @brief ouverture max en tick
 * 
 */
long ouvertureMax = 2000;
byte displayMode = 0;
long consigneMoteur = 0;
volatile int sensMoteur = 0;
long posMoteur;

//Automate

enum EtapesEcran{
	Initi,
	Reveil,
	Affiche,
	Suivant,
	Sleep,
};
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
	
	float incMoteur = degToInc(degVanne);
	
	
	return incMoteur / tourMoteurVanne;
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
	countEncodB = 0;
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
	FCVanneFermee.loop();
	FCVanneOuverte.loop();
	PrgButton->loop();
	// Entree[FCVanneOuverte] = digitalRead(pinFCVanneOuverte) ? true :false ;
	// Entree[FCVanneFerme] = digitalRead(pinFCVanneFermee) ? true : false;
	// Entree[EPRGBUTTON] = digitalRead(PRGButton)? true : false;
	mesureIntensite();
	if (millis()> previousCalculTaqui + 2000)
	{
		previousCalculTaqui = millis();
		rpmTurbine = mesureTaqui();
	}
}
void TraitementCommande(String c){
	Serial.println("TraitementCommande: " + (String)c);
	if (c == "DemandeStatut")
	{
		EnvoyerStatut = true;
	} else
	{
		sendMessageConfirmation(receivedMessage.msgID);
		LoRa.receive();
	}
	
	
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
	if (c.startsWith("DEGV"))
	{
		c.remove(0, 4);
		Serial.println(c.toInt());
		consigneMoteur = degvanneToInc(c.toInt());
		Serial.println(consigneMoteur);
		if (consigneMoteur > 0)
		{
			sensMoteur = 1;
		}
		else
		{
			sensMoteur = -1;
		}
		
	}
	if (c.startsWith("DEG"))
	{
		c.remove(0, 3);
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
		preferences.putInt("ouvertureMax",ouvertureMax);
	}
	if (c.startsWith("SetMaxI="))
	{
		c.replace("SetMaxI=","");
		maxIntensite = c.toInt();
		preferences.putInt("maxIntensite",maxIntensite);
		
	}
	if (c == "ouvertureMax")
	{
		c.remove(0,12);
		ouvertureMax = c.toInt();
		preferences.putInt("ouvertureMax",ouvertureMax);
		
	}
	if (c.startsWith("DeepSleep="))
	{
		c.replace("DeepSleep=","");
		Serial.println("DeepSleep " + String(c.toInt()));
		esp_sleep_enable_timer_wakeup(c.toInt()*1000);
		esp_deep_sleep_start();
	}
	if (c.startsWith("LightSleep="))
	{
		c.replace("LightSleep=","");
		Serial.println("LightSleep " + String(c.toInt()));
		esp_sleep_enable_timer_wakeup(c.toInt()*1000);
		esp_light_sleep_start();
	}
	if (c.startsWith("P="))
	{
		/* ouverture vanne pourcentage */
		c.replace("P=","");
		float cibleOuverture = c.toFloat() / 100;
		int cibleTick = ouvertureMax * cibleOuverture;
		consigneMoteur  =  posMoteur - cibleTick;
		if (consigneMoteur > 0)
		{
			sensMoteur = 1;
		}else
		{
			sensMoteur = -1;
		}
		
		
	}
		



}
void EvolutionGraphe(void) {

	// calcul des transitions

	Transition[0] = Etape[Init];
	Transition[1] = Etape[DelaiPOM] && PrgButton->frontDesceandant();//Entree[EPRGBUTTON] == false /*&& FrontMontant[BP]*/;
	Transition[2] = Etape[DelaiPOM] && FCVanneFermee.isPressed() && finTempo(2);   // (Entree[FCVanneFerme] == true) 
	Transition[3] = Etape[DelaiPOM] && FCVanneFermee.isReleased() && finTempo(2); //(Entree[FCVanneFerme] == false)
	Transition[4] = Etape[POMO] && FCVanneFermee.isReleased() ;//(Entree[FCVanneFerme] == false);
	Transition[5] = Etape[POMF] && FCVanneFermee.isPressed() ;//(Entree[FCVanneFerme] == true);
	Transition[6] = Etape[STOPPOMF] ;
	Transition[7] = Etape[AttenteOrdre] && consigneMoteur < 0;
	Transition[8] = Etape[AttenteOrdre] && consigneMoteur > 0;
	Transition[9] = Etape[OuvrirVanne] && (consigneMoteur < 0 || FCVanneOuverte.isPressed());//Entree[FCVanneOuverte]
	Transition[10] = Etape[FermerVanne] && (consigneMoteur > 0 || FCVanneFermee.isPressed());//Entree[FCVanneFerme]
	Transition[11] = Etape[StopMoteur] && finTempo(0);
	Transition[12] = Etape[AttenteOrdre] && EnvoyerStatut;
	Transition[13] = Etape[EnvoyerMessage] ;
	Transition[14] = Etape[AttenteOrdre] && bOuvertureTotale;
	Transition[15] = Etape[AttenteOrdre] && bFermetureTotale;
	Transition[16] = Etape[OuvertureTotale] && FCVanneOuverte.isPressed();//(Entree[FCVanneOuverte] == true)
	Transition[17] = Etape[FermetureTotale] && FCVanneFermee.isPressed();//(Entree[FCVanneFerme] == true)
	Transition[18] = Etape[OuvrirVanne] && currentValue> maxIntensite;
	Transition[19] = Etape[FermerVanne] && currentValue> maxIntensite;
	Transition[20] = Etape[OuvertureTotale] && currentValue> maxIntensite;
	Transition[21] = Etape[FermetureTotale] && currentValue> maxIntensite;
	Transition[22] = Etape[StopMoteurIntensite] ;

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
	if (Transition[22]) Etape[StopMoteurIntensite] =0;

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
	if (Transition[22]) Etape[AttenteOrdre] =1;

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
		//Sortie[FermetureVanne] = 1;
		FermetureVanne->setState(true);
		asservissementMoteur();
	}

	if (Etape[OuvrirVanne] == 1)
	{
		EtapeActuel = OuvrirVanne;
		StatutVanne = "Ouverture";
		//Sortie[OuvertureVanne] = 1;
		OuvertureVanne->setState(true);
		asservissementMoteur();
	}

	if (Etape[StopMoteur])
	{
		consigneMoteur = 0;
		StatutVanne = "Arret";
		startTempo(0, 1000);
		EtapeActuel = StopMoteur;
		// Sortie[OuvertureVanne] = 0;
		// Sortie[FermetureVanne] = 0;
		OuvertureVanne->setState(false);
		FermetureVanne->setState(false);
		
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
		EnvoyerStatut = false;
		
	}
	if (Etape[OuvertureTotale])
	{
		sensMoteur = 1;
		EtapeActuel = OuvertureTotale;
		StatutVanne = "Ouverture";
		bOuvertureTotale = false;
		// Sortie[OuvertureVanne] = 1;
		OuvertureVanne->setState(true);		
	}
	if (Etape[FermetureTotale])
	{
		sensMoteur = -1;
		StatutVanne = "Fermeture";
		EtapeActuel = FermetureTotale;
		bFermetureTotale = false;
		
		// Sortie[FermetureVanne] = 1;
		FermetureVanne->setState(true);
	}
	if (Etape[StopMoteurIntensite])
	{
		EtapeActuel = StopMoteurIntensite;
		StatutVanne = "Arret Intensite";
		// Sortie[OuvertureVanne] = 0;
		// Sortie[FermetureVanne] = 0;
		FermetureVanne->setState(false);
		OuvertureVanne->setState(false);



		StaticJsonDocument<96> doc;
		String json; 
		doc["Ouverture"] = pPosMoteur();
		doc["Taqui"] = rpmTurbine;
		doc["StatutVanne"] = StatutVanne;
		serializeJson(doc,json);
		Serial.println(json);
		sendMessage(MASTER, json);
		delay(200);

	}
	if (Etape[POMO])
	{
		EtapeActuel = POMO;
		// Sortie[OuvertureVanne] = 1;
		OuvertureVanne->setState(true);
		
	}
	if (Etape[POMF])
	{
		EtapeActuel = POMF;
		//Sortie[OuvertureVanne] = 0;
		//Sortie[FermetureVanne] = 1;
		FermetureVanne->setState(true);
		OuvertureVanne->setState(false);
	}
	if (Etape[STOPPOMF])
	{
		EtapeActuel = STOPPOMF;
		// Sortie[OuvertureVanne] = 0;
		// Sortie[FermetureVanne] = 0;
		FermetureVanne->setState(false);
		OuvertureVanne->setState(false);
		posMoteur = 0;
		countEncodA = 0;
	}
	if (Etape[STOPPOMO])
	{
		EtapeActuel = STOPPOMO;
		// Sortie[OuvertureVanne] = 0;
		// Sortie[FermetureVanne] = 0;
		FermetureVanne->setState(false);
		OuvertureVanne->setState(false);
		ouvertureMax = posMoteur;
	}

	
}

void miseAjourSortie(void) {
	// digitalWrite(pinOuvertureVanne, Sortie[OuvertureVanne]);
	// digitalWrite(pinFermetureVanne, Sortie[FermetureVanne]);
	OuvertureVanne->loop();
	FermetureVanne->loop();
}
// fonction d'arrêt d'une tempo
void stopTempo(int numTempo) {
	Tempo[numTempo] = 0;
}

 void IRAM_ATTR EncodA() {
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

		
void IRAM_ATTR EncodB() {
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
			Heltec.display->drawString(0, 15, "posMoteur: " + String(posMoteur));
			Heltec.display->drawString(0, 27, "consigne: " + String(consigneMoteur));
			if (sensMoteur>0)
			{
				Heltec.display->drawString(15, 50, "->");
			}
			else if(sensMoteur < 0)
			{
				Heltec.display->drawString(15, 50, "<-");
			}
			Heltec.display->drawString(0, 39, "Etape: " + String(EtapeToString( EtapeActuel)));
		
			Heltec.display->drawString(5, 55, FCVanneFermee.isPressed() ? "*" : "");
			Heltec.display->drawString(110, 55, FCVanneOuverte.isPressed() ? "*" : "");
			break;
		case 1:
			Heltec.display->drawString(0, 0, "EA: " + String(countEncodA));
#ifdef pinEncodB
			Heltec.display->drawString(64, 0, "EB: " + String(countEncodB));
#endif // pinEncodB
			Heltec.display->drawString(0,12,"posM "+String(posMoteur));
			
			Heltec.display->drawString(64,12,"Omax " + String(ouvertureMax));
			
			
			Heltec.display->drawString(60,50,String(pPosMoteur())+"%");
			
			Heltec.display->drawProgressBar(5,30,120,10,pPosMoteur()*100);
			Heltec.display->drawString(5, 55, FCVanneFermee.isPressed() ? "*" : "");
			Heltec.display->drawString(110, 55, FCVanneOuverte.isPressed() ? "*" : "");
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
			Heltec.display->drawString(0,41,"Rap Reduc: ");
			Heltec.display->drawString(60,41,String(tourMoteurVanne));
			
			break;

		default:
			Heltec.display->drawString(0,0,"Erreur displayData");
			break;
		}
		
	 
 	}
	Heltec.display->drawString(120,50,(String)displayMode) ;
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
		 //Serial.print(Sortie[i]);
	 }
	 Serial.print(" F " + (String) FermetureVanne->getState());
	 Serial.print(" O " + (String) OuvertureVanne->getState());
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
		receivedMessage.Content = "";
		return;                             // skip rest of function
	}
	newMessage = true;
	//// if message is for this device, or broadcast, print details:
	Serial.println("Received from: 0x" + String(receivedMessage.sender, HEX));
	Serial.println("Sent to: 0x" + String(receivedMessage.recipient, HEX));
	Serial.println("Message ID: " + String(incomingMsgId));
	Serial.println("Message length: " + String(incomingLength));
	Serial.println("Message: " + receivedMessage.Content);
	Serial.println("RSSI: " + String(LoRa.packetRssi()));
	//Serial.println("Snr: " + String(LoRa.packetSnr()));
	Serial.println();
	// Heltec.display->println("0x" + String(receivedMessage.sender,HEX) + " to 0x" + String(receivedMessage.recipient, HEX) + " " + String(receivedMessage.Content));
	Heltec.display->println(String(receivedMessage.Content));

	

	
	
}
void Taqui(void){
 countTaqui++;
}

String wakeup_reason_toString(esp_sleep_wakeup_cause_t wakeup_reason){
  

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : return "Wakeup caused by external signal using RTC_IO" ; break;
    case ESP_SLEEP_WAKEUP_EXT1 : return "Wakeup caused by external signal using RTC_CNTL" ; break;
    case ESP_SLEEP_WAKEUP_TIMER : return "Wakeup caused by timer" ; break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : return "Wakeup caused by touchpad" ; break;
    case ESP_SLEEP_WAKEUP_ULP : return "Wakeup caused by ULP program" ; break;
    default : return "Wakeup was not caused by deep sleep: " + wakeup_reason ; break;
  }
}
// the setup function runs once when you press reset or power the board
void setup() {

	//Wire.begin(SDA_OLED, SCL_OLED); 
	Heltec.begin(true, true, true, true, BAND);
	Heltec.display->setLogBuffer(10,30);

	Heltec.display->drawString(0,12,wakeup_reason_toString(esp_sleep_get_wakeup_cause()));

	// pinMode(pinFCVanneFermee, INPUT_PULLDOWN );
	// pinMode(pinFCVanneOuverte, INPUT_PULLDOWN);


//encodeurs
	// pinMode(pinFermetureVanne, OUTPUT);
	// pinMode(pinOuvertureVanne, OUTPUT);

	pinMode(pinEncodA, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt( pinEncodA), EncodA,RISING);

#ifdef pinEncodB
	pinMode(pinEncodB, INPUT_PULLUP);

	attachInterrupt( digitalPinToInterrupt( pinEncodB) , EncodB,RISING);
#endif // pinEncodB

#ifdef pinTaqui
	pinMode(pinTaqui, INPUT_PULLUP);
	attachInterrupt(pinTaqui, Taqui,RISING);
#endif

// pinMode(2,OUTPUT);
// ledcSetup(0,5000,8);
// ledcAttachPin(2, 0);
// ledcWrite(0,255);

	if (preferences.begin("Turbine",false))
	{
		ouvertureMax = preferences.getLong("ouvertureMax",ouvertureMax);
		maxIntensite = preferences.getLong("maxIntensite", maxIntensite);
	}
	

	if (!ina260.begin(0x40, &Wire)) {
		Heltec.display->drawString(0,24,"Couldn't find INA260 chip");
		Serial.println("Couldn't find INA260 chip");
		Heltec.display->display();
		while (1);
	}
	Heltec.display->drawString(0,24,"INA260 ok !");
	Heltec.display->display();
	Serial.println("Found INA260 chip");

	 FCVanneFermee.loop();
	 FCVanneOuverte.loop();

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
		
		if (PrgButton->frontDesceandant())
		{
			
			if (displayMode < 4 )
			{
				displayMode++;
			}
			else
			{
				displayMode = 0;
			}
		}
		
	}
	#if defined(_DEBUG)
	if (millis()> dernierAppuibutton +30000)
			{
				Heltec.display->sleep();
			} else
			{
				Heltec.display->wakeup();

			}
	
	
	#endif // _DEBUG
	
	

	if (newMessage)
	{
		newMessage = false;
		TraitementCommande(receivedMessage.Content);
		
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
	delay(10);
	//debugGrafcet();
}
