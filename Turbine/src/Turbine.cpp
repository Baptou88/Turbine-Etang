/*
 Name:		Turbine.ino
 Created:	21/05/2021 18:29:24
 Author:	Baptou
*/

#include <Arduino.h>
#include <heltec.h>
#include "TurbineEtangLib.h"

#define BAND 868E6
#define TAILLETAB 16

//entree
#define FCVanneOuverte 0
#define FCVanneFerme 1

//sortie
#define OuvertureVanne 0
#define FermetureVanne 1

byte localAddress = 0x0B;
byte msgCount = 0;

//encodeur
#define pinEncodA  37
#define pinEncodB  36

const int pinOuvertureVanne = 12;
const int pinFermetureVanne = 13;
const byte pinFCVanneOuverte = 39;
const byte pinFCVanneFermee = 38;
volatile long countEncodA = 0;
volatile long countEncodB = 0;
unsigned long dernieredetection = 0;

bool bOuvertureTotale = false;
bool bFermetureTotale = false;

int incCodeuse = 600;

long consigneMoteur = 0;
volatile int sensMoteur = 0;
long posMoteur;

//Automate

enum Etapes
{
	Init,
	AttenteOrdre,
	TraitementMessage,
	OuvrirVanne,
	FermerVanne,
	StopMoteur,
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


int degToInc(int degres) {
	return ((degres * incCodeuse) / 360);
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
			if (s.startsWith("M"))
			{
				s.remove(0,1);
				consigneMoteur = s.toInt();
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
			if (s.startsWith("D"))
			{
				s.remove(0, 1);
				consigneMoteur = degToInc(s.toInt());
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
			if (s.startsWith("V"))
			{
				s.remove(0, 1);
				if (s.toInt() == 1)
				{
					Heltec.VextON();
				}
				else {
					Heltec.VextOFF();
				}
			}
			if (s.startsWith("OT"))
			{
				s.remove(0, 2);
				bOuvertureTotale = true;
			}
			if (s.startsWith("FT"))
			{
				s.remove(0, 2);
				bFermetureTotale = true;
			}
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
		sendMessage(MASTER, "posMoteur" + String(posMoteur));
	}
	if (Etape[OuvertureTotale])
	{
		EtapeActuel = OuvertureTotale;
		bOuvertureTotale = false;
		Sortie[OuvertureVanne] = 1;
		
	}
	if (Etape[FermetureTotale])
	{
		EtapeActuel = FermetureTotale;
		bFermetureTotale = false;
		
		Sortie[FermetureVanne] = 1;
	}

	
}

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

void miseAjourSortie(void) {
	digitalWrite(pinOuvertureVanne, Sortie[OuvertureVanne]);
	digitalWrite(pinFermetureVanne, Sortie[FermetureVanne]);
}
// fonction d'arrêt d'une tempo
void stopTempo(int numTempo) {
	Tempo[numTempo] = 0;
}
 void EncodA() {
	 if (millis()> dernieredetection + 1)
	 {
		 dernieredetection = millis();
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
// the setup function runs once when you press reset or power the board
void setup() {
	Heltec.begin(true, true, true, true, BAND);

	pinMode(pinFCVanneFermee, INPUT_PULLDOWN );
	pinMode(pinFCVanneOuverte, INPUT_PULLUP);

	pinMode(pinFermetureVanne, OUTPUT);
	pinMode(pinOuvertureVanne, OUTPUT);

	pinMode(pinEncodA, INPUT);
	attachInterrupt(pinEncodA, EncodA,CHANGE);
#ifdef pinEncodB
	pinMode(pinEncodB, INPUT);
	attachInterrupt(pinEncodB, EncodB,CHANGE);
#endif // pinEncodB

	
	delay(1000);

	Heltec.display->clear();
	// initialisation du grafcet
	InitTableau();
	// forçage de l'étape initiale
	Etape[Init] = 1;

}

// the loop function runs over and over again until power down or reset
void loop() {
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
