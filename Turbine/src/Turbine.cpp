#include <Arduino.h>
#include <heltec.h>
#include <PID_v1.h>
#include <Fsm.h>
#include <Adafruit_I2CDevice.h>
#include <Preferences.h>
#include <Adafruit_INA260.h>
#include <ArduinoJSON.h>

#include "Turbine.h"
#include "digitalInput.h"
#include "digitalOutput.h"
#include "Moteur.h"
#include "TurbineEtangLib.h"
#include "motion.h"
#include "menu.h"



//Sortie
byte pinMoteurO = 12;
byte pinMoteurF = 13;

// Entrée 
byte pinEncodeurA = 36;
byte pinEncodeurB = 37;
#define pinTaqui 32

digitalInput FCVanneOuverte(39,INPUT_PULLUP); 
digitalInput FCVanneFermee(38,INPUT_PULLDOWN);
digitalInput* PrgButton= new digitalInput(0,INPUT_PULLUP);

//encodeur
volatile long countEncodA = 0;
unsigned long dernieredetectionEncodA  = 0;

volatile long countEncodB = 0;
unsigned long dernieredetectionEncodB  = 0;

bool pom_success = false;
bool do_send_msg = false;

bool do_ouvertureTotale = false;
bool do_fermetureTotale = false;
//Define Variables we'll be connecting to
double Setpoint, posMoteur, Output;

double Kp=2.2, Ki=0.0, Kd=0.4;
PID myPID(&posMoteur, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

int maxAxel = 10;
int vmin = 80;
int previousVitesse = 0;
int MoteurPWM = 0;
/**
 * @brief ouverture max en tick
 * 
 */
long ouvertureMax = 2000;
int incCodeuse = 400;
float tourMoteurVanne = 18 / float(44); 

// Taquimetre
unsigned long previousMillisTaqui = 0;
volatile unsigned long countTaqui = 0;
float rpmTurbine = 0;
unsigned long previousCalculTaqui = 0;

long msgCount = 0;
bool newMessage = false;
Message receivedMessage;

SSD1306Wire* display = Heltec.display;

byte localAddress = TURBINE;

Preferences preferences;

Adafruit_INA260 ina260 = Adafruit_INA260();
double currentValue = 0;
unsigned long previousMesureIntensite = 0;
int maxIntensite = 3000; //mA

byte displayMode = 0;

//menus

String  menu_param[] = {"AP", "STA","ScanWifi"};
menu Menu_param(3,3,NULL,NULL);

void INIT(){
  static unsigned long ti = millis();

  display->clear();

  display->drawString(0,0,"INIT: ");
  display->drawString(10,15,String(millis() - ti));
  display->display();
}
void AUTO(){
  
  myPID.SetMode(AUTOMATIC);

  posMoteur += countEncodA;
  countEncodA = 0;

  MoteurPWM = Output;
  if (MoteurPWM >255) MoteurPWM = 255;

  if (MoteurPWM < -255) MoteurPWM = -255;



  if (MoteurPWM - previousVitesse > maxAxel)
  {
    MoteurPWM = previousVitesse + maxAxel;
    //display->drawString(100,40,"+");
  } else if (MoteurPWM -previousVitesse <-maxAxel)
  {
    //display->drawString(100,50,"-");
    MoteurPWM = previousVitesse - maxAxel;
  }

  previousVitesse = MoteurPWM;
  if (MoteurPWM >0 && MoteurPWM <vmin) MoteurPWM = 0;

  if (MoteurPWM < 0 && MoteurPWM > -vmin) MoteurPWM = -0;

  // display->drawString(0,0,"countA: " + (String) countEncodA);
  // display->drawString(60,0,"countB: " + (String) countEncodB);
  // display->drawString(0,15,"posMoteur: " + (String) posMoteur);
  // display->drawString(0,30,"Output: " + (String) Output);
  // display->drawString(0,45,"Setpoint: " + (String) Setpoint);
  displayData();
  
}
//FSM
State state_INIT(NULL,&INIT,NULL);
State state_AUTO(NULL,&AUTO,NULL);

Fsm fsm(&state_INIT);
State state_SendMessage(NULL,[](){
  display->clear();
  display->drawString(0,0,"sendMSG");
  display->display();
  delay(2000);
  do_send_msg = false;
},NULL);
State state_POM(NULL,[](){
  static int state = 0;
  display->clear();
  if (FCVanneFermee.isPressed() && state == 0)
  {
    state = 0;
  } 
  if (FCVanneFermee.isReleased() && state == 0 && state !=4) {
    state = 1;
  }
  if (FCVanneFermee.isPressed() && state !=0 && state  !=4)
  {
    state = 3;
    
  }
  
  
  switch (state)
  {
  case 0:
    MoteurPWM = 255;
    break;
  case 1:
    MoteurPWM = -200;
    break;
  case 3:
    MoteurPWM = 0;
    state = 4;
    break;
  case 4:
    pom_success = true;
    delay(200);
    posMoteur = 0;
    countEncodA = 0;
    countEncodB = 0;
    Serial.println(millis());
    break;
  default:
    Serial.println("defaul");
    break;
  }
  
  display->drawString(0,0,"State, "+ String(state));
  display->display();
},NULL);
State state_OuvertureTotale(NULL,[](){
  posMoteur += countEncodA;
  countEncodA = 0;
  
  myPID.SetMode(MANUAL);
  MoteurPWM = 255;
},NULL);
State state_FermetureTotale(NULL,[](){
  posMoteur += countEncodA;
  countEncodA = 0;
  if (FCVanneFermee.isPressed())
  {
    MoteurPWM = 0;

    Setpoint = posMoteur;
    return;
  }
  myPID.SetMode(MANUAL);
  MoteurPWM = -255;
},NULL);
State state_StopIntensite(NULL,[](){
  myPID.SetMode(MANUAL);
  display->clear();
  display->drawString(10,10,"Stop intensite");
  display->display();
  MoteurPWM = 0;
},NULL);

State state_param(NULL,[](){
  display->clear();
  display->drawString(0,0,"Param:");
  display->drawString(60,0,(String)countEncodA);
  static unsigned lastcountEncodA = 0;
  if (digitalRead(pinEncodeurA)== LOW  && millis()-lastcountEncodA >500)
  {
    lastcountEncodA = millis();
    Menu_param.selectNext();
  }
  

  Menu_param.loop();
  Menu_param.render();

  display->display();
},NULL);

void initTransition(){
  fsm.add_timed_transition(&state_INIT,&state_POM,2000, NULL);
  // fsm.add_transition(&state_INIT, &state_AUTO,[](unsigned long duration){
  //   return PrgButton->isPressed();
  // },NULL);
  fsm.add_transition(&state_POM,&state_AUTO,[](unsigned long duration){
    return pom_success;
  },NULL);
  fsm.add_transition(&state_AUTO ,&state_SendMessage,[](unsigned long duration){
    return false;
  }, NULL);
  fsm.add_transition(&state_SendMessage ,&state_AUTO,[](unsigned long duration){
    return !do_send_msg;
  }, NULL);
  fsm.add_transition(&state_INIT,&state_AUTO,[](unsigned long duration){
    return esp_sleep_get_wakeup_cause()== ESP_SLEEP_WAKEUP_TIMER;
  },NULL);
  fsm.add_transition(&state_AUTO,&state_OuvertureTotale,[](unsigned long duration){
    return do_ouvertureTotale;
  },[](){
    do_ouvertureTotale = false;
  });
  fsm.add_transition(&state_AUTO,&state_FermetureTotale,[](unsigned long duration){
    return do_fermetureTotale;
  },[](){
    do_fermetureTotale = false;
  });
  fsm.add_transition(&state_OuvertureTotale,&state_AUTO, [](unsigned long duration){
    return FCVanneOuverte.isPressed();
  },[](){
    MoteurPWM = 0;
    Setpoint = posMoteur;

  });
  fsm.add_transition(&state_FermetureTotale,&state_AUTO, [](unsigned long duration){
    return FCVanneFermee.isPressed();
  },[](){
    MoteurPWM = 0;
    posMoteur = 0;
    Setpoint = posMoteur;

  });
  fsm.add_transition(&state_AUTO,&state_StopIntensite,[](unsigned long duration){
    return currentValue > maxIntensite;
  },NULL);
  fsm.add_transition(&state_OuvertureTotale,&state_StopIntensite,[](unsigned long duration){
    return currentValue > maxIntensite;
  },NULL);
  fsm.add_transition(&state_FermetureTotale,&state_StopIntensite,[](unsigned long duration){
    return currentValue > maxIntensite;
  },NULL);

  fsm.add_transition(&state_AUTO,&state_param,[](unsigned long duration){
    return PrgButton->pressedTime() > 2000;
  },NULL);
}
void IRAM_ATTR isrEncodA(void){
  if (millis() - dernieredetectionEncodA > 1)
  {
    if (digitalRead(pinEncodeurB) == HIGH)
    {
      //Serial.println("v");
      countEncodA--;
    }else
    {
      countEncodA++;
    }
    
  
  }
  
}
void IRAM_ATTR isrEncodB(void){
  if (millis() - dernieredetectionEncodB > 1)
  {
    countEncodB++;
    
  }
  
}

void TraitementCommande(String c){
	Serial.println("TraitementCommande: " + (String)c);
	if (c == "DemandeStatut")
	{
			StaticJsonDocument<128> doc;
		String json; 
		doc["Ouverture"] = pPosMoteur();
		doc["Setpoint"] = pSetpoint();
		doc["Taqui"] = rpmTurbine;
		//doc["StatutVanne"] = StatutVanne;
		doc["OuvCodeur"] = posMoteur;
		doc["OuvMaxCodeur"] = ouvertureMax;
		serializeJson(doc,json);
		Serial.println(json);
		sendMessage(MASTER, json);
		LoRa.receive();
	} else
	{
		sendMessageConfirmation(receivedMessage.msgID);
		LoRa.receive();
	}
	
	
	if (c.startsWith("M"))
	{
		c.remove(0,1);
		
		Setpoint = c.toInt();
		
	}
	if (c.startsWith("DEGV"))
	{
		c.remove(0, 4);
		
		Serial.println(c.toInt());
		Setpoint += degvanneToInc(c.toInt());
		Serial.println(Setpoint);
				
	}
	if (c.startsWith("DEG"))
	{
		c.remove(0, 3);
		
		Setpoint += degToInc(c.toInt());
		Serial.println(Setpoint);
	
	}
	
	if (c.startsWith("OT"))
	{
		c.remove(0, 2);
		do_ouvertureTotale = true;
	
	}
	if (c.startsWith("FT"))
	{
		c.remove(0, 2);
		do_fermetureTotale = true;

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
		Serial.println("DeepSleep " + String(c.toDouble()));
		Heltec.display->clear();
		Heltec.display->drawString(0,0,"DeepSleep");
		Heltec.display->drawString(0,15,String(c.toDouble()));
		Heltec.display->display();
		delay(2000);
		esp_sleep_enable_timer_wakeup(c.toDouble()*1000);
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
		Setpoint += cibleTick-Setpoint;
		
		
		
	}
		
  


}

void displayData() {
	Heltec.display->clear();
  if (PrgButton->frontDesceandant())
  {
    displayMode ++;
    if (displayMode >4 )
    {
      displayMode = 0;
    }
    
  }
  
		switch (displayMode)
		{
		case 0:
			Heltec.display->drawString(0, 0, "EA: " + String(countEncodA));

			Heltec.display->drawString(64, 0, "EB: " + String(countEncodB));

			Heltec.display->drawString(0, 15, "posMoteur: " + String(posMoteur));
			Heltec.display->drawString(0, 27, "consigne: " + String(Setpoint));
			
			
		
			Heltec.display->drawString(5, 55, FCVanneFermee.isPressed() ? "*" : "");
			Heltec.display->drawString(110, 55, FCVanneOuverte.isPressed() ? "*" : "");
			break;
		case 1:
			Heltec.display->drawString(0, 0, "EA: " + String(countEncodA));

			Heltec.display->drawString(64, 0, "EB: " + String(countEncodB));

			Heltec.display->drawString(0,12,"posM "+String(posMoteur));
			
			Heltec.display->drawString(64,12,"Omax " + String(ouvertureMax));
			
			
			Heltec.display->drawString(80,46,String(pPosMoteur()*100)+"%");
			Heltec.display->drawString(0, 46, "consigne: " + String(Setpoint));
			Heltec.display->drawString(5+((posMoteur)*120) /2000, 42, "^");
			
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
		
	 
 	
	Heltec.display->drawString(120,50,(String)displayMode) ;
	Heltec.display->display();
}

 

float mesureTaqui(void){
	float rpm = countTaqui * 60000 / float((millis() - previousMillisTaqui));
	countTaqui = 0;
	previousMillisTaqui = millis();

	return rpm;
}
/**
 * @brief Acquisition des Entrées
 * 
 */
void acquisitionEntree(void) {
	FCVanneFermee.loop();
	FCVanneOuverte.loop();
	PrgButton->loop();

  if (millis()> previousMesureIntensite + 200)
	{
		previousMesureIntensite = millis();
		currentValue = ina260.readCurrent();
		
	}

  if (millis()> previousCalculTaqui + 1000)
	{
		previousCalculTaqui = millis();
		rpmTurbine = mesureTaqui();
	}
}

/**
 * @brief Mise a jour des sorties
 * 
 */
void miseAjourSortie(void) {
	
	if (MoteurPWM>0)
   {
     ledcWrite(1,MoteurPWM);
     ledcWrite(2,0);
   }  else if (MoteurPWM < 0)
   {
     ledcWrite(1,0);
     ledcWrite(2,abs(MoteurPWM));
   } else
   
   {
     ledcWrite(1,0);
     ledcWrite(2,0);
   }
	
}
void onReceive(int packetSize){
  	if (packetSize == 0) return;          // if there's no packet, return

	//// read packet header bytes:
	byte recipient = LoRa.read();          // recipient address
	byte sender = LoRa.read();            // sender address
	byte incomingMsgId = LoRa.read();     // incoming msg ID
	byte incomingLength = LoRa.read();    // incoming msg length
	
	String Content = "";                 // payload of packet

	while (LoRa.available())             // can't use readString() in callback
	{
		Content += (char)LoRa.read();      // add bytes one by one
	}

	if (incomingLength != Content.length())   // check length for error
	{
		Serial.println("error: message length does not match length");
		return;                             // skip rest of function
	}

	// if the recipient isn't this device or broadcast,
	if (recipient != localAddress && recipient != 0xFF)
	{
		Serial.println("This message is not for me. " + String(recipient));
		return;                             // skip rest of function
	}
  receivedMessage.recipient = recipient;
  receivedMessage.sender = sender;
  receivedMessage.msgID = incomingMsgId;
  receivedMessage.Content = Content;
  
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
void EvolutionGraphe(void){
  
  
}
IRAM_ATTR void isrTaqui(void){
 countTaqui++;
}
void setup() {
  // put your setup code here, to run once:
  Heltec.begin(true,true,true,true,868E6);

  //Sortie Moteur
  pinMode(pinMoteurO,OUTPUT);
  pinMode(pinMoteurF,OUTPUT);
  ledcSetup(1, 5000, 8);
  ledcSetup(2, 5000, 8);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(pinMoteurF, 1);
  ledcAttachPin(pinMoteurO, 2);

  //Encodeur Rotation
  pinMode(pinEncodeurA,INPUT);
  pinMode(pinEncodeurB,INPUT_PULLUP);

  attachInterrupt(pinEncodeurA,isrEncodA,RISING);
  attachInterrupt(pinEncodeurB,isrEncodB,RISING);


#ifdef pinTaqui
	pinMode(pinTaqui, INPUT_PULLUP);
	attachInterrupt(pinTaqui, isrTaqui,RISING);
#endif

  while(!ina260.begin(0x40, &Wire)) {
    Heltec.display->drawString(0,24,"Couldn't find INA260 chip");
    Serial.println("Couldn't find INA260 chip");
    Heltec.display->display();
    delay(1000);
  }
	Heltec.display->drawString(0,24,"INA260 ok !");
	Heltec.display->display();
	Serial.println("Found INA260 chip");
 
 FCVanneFermee.loop();
 FCVanneOuverte.loop();
  initTransition();

  if (preferences.begin("Turbine",false))
	{
		ouvertureMax = preferences.getLong("ouvertureMax",ouvertureMax);
		maxIntensite = preferences.getLong("maxIntensite", maxIntensite);
	}

  if (esp_sleep_get_wakeup_cause()== ESP_SLEEP_WAKEUP_TIMER)
	{

  }
  

  myPID.SetMode(MANUAL);
  myPID.SetOutputLimits(-255, 255);
  Serial.print("Time(s), ");
  Serial.print("Consigne, ");
  Serial.print("posMoteur, ");
  Serial.print("Output, ");
  Serial.print("vitesseM, ");
  Serial.println();

  LoRa.setSpreadingFactor(8);
	LoRa.setSyncWord(0x12);
	LoRa.setSignalBandwidth(125E3);

	LoRa.onReceive(onReceive);
	LoRa.receive();

  //menus

  Menu_param.onRender([](int num, int numel,bool hover){
		//Serial.println("onrendermodeWifi " + (String)num + (String)numel);
		Heltec.display->drawString(12,num*12+14,menu_param[numel]);
		if (hover)
		{
			Heltec.display->fillCircle(6,num*12+14+6,3);
		}
		
	});
}

void loop() {
  // put your main code here, to run repeatedly:

  acquisitionEntree();
  miseAjourSortie();
  EvolutionGraphe();
  fsm.run_machine();

  display->clear();

  if(Serial.available()){
    int valeur;
    valeur=Serial.parseInt();  //récupération des caractères sur le port série
    if(valeur!=0){
      Setpoint = valeur;
    }
  }
  
  
 

  myPID.Compute();
   
   if (newMessage)
	{
		newMessage = false;
		TraitementCommande(receivedMessage.Content);
		
	}

  // Serial.print(millis());
  // Serial.print(" ");
  // Serial.print(Setpoint);
  // Serial.print(" ");
  // Serial.print(posMoteur);
  // Serial.print(" ");
  // Serial.print(Output);
  // Serial.print(" ");
  // Serial.print(MoteurPWM);
  // Serial.print(" ");
  
  // Serial.println();

  delay(20);
}