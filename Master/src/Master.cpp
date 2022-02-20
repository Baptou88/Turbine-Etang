/*
 Name:		Master.ino
 Created:	21/05/2021 18:31:59
 Author:	Baptou
*/



//#include <AsyncTCP.h>

//#include <HTTP_Method.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SD.h>
#include <WiFi.h>
#include <heltec.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <AutoPID.h>
#include <LinkedList.h>
#include <Preferences.h>
#include <logger.h>


#include "TurbineEtangLib.h"
#include "digitalInput.h"
#include "menu.h"
#include "MasterLib.h"
#include "websocket.h"
#include "connexionWifi.h"
#include "ProgrammatedTask.h"



#define BAND 868E6
//#define PRGButton 0

digitalInput* prgButton = new digitalInput(0,INPUT_PULLUP);
digitalInput* encodLeft = new digitalInput(36,INPUT_PULLUP);
digitalInput* encodRight = new digitalInput(38,INPUT_PULLUP);
digitalInput* testbtn = new digitalInput(2,INPUT_PULLDOWN);

#define __DEBUG
// #define USEIPFIXE

//#ifdef __DEBUG
	int intervalleEnvoi = 30000; // 30sec
//#endif
//#ifndef __DEBUG
	//int intervalleEnvoi = 60000; // 1 min
//#endif

RTC_DATA_ATTR byte defaultConnectWifi = 0; //

bool previousEtatbutton = false;
SPIClass spi1;

// Set web server port number to 80
//WiFiServer serverHTTP(80);	//80 est le port standard du protocole HTTP
AsyncWebServer serverHTTP(80);	//80 est le port standard du protocole HTTP
AsyncWebSocket ws("/ws");

WiFiUDP ntpUDP;
const char* ntpServer = "europe.pool.ntp.org";
NTPClient timeClient(ntpUDP,ntpServer,3600,3600);

Preferences preferences;


board localboard("Master", MASTER);

board EtangBoard("Etang", ETANG);

board TurbineBoard("Turbine", TURBINE);

board RadiateurBoard("Radiateur", Radiateur);

//board *allBoard[3] = {  &localboard, &EtangBoard , &TurbineBoard};
String  modeWifi[] = {"AP", "STA","ScanWifi"};

wifiparamconnexion paramWifi;

menu menuModeWifi(3,3,encodRight,encodLeft);

menu menuStationWifi(2,2,encodRight,encodLeft);
//LinkedList<board*> allBoard = new LinkedList<board*>();
LinkedListB::LinkedList<board*> *allBoard = new  LinkedListB::LinkedList<board*>();

LinkedListB::LinkedList<ProgrammatedTask*> *ProgrammatedTasks = new LinkedListB::LinkedList<ProgrammatedTask*>();

unsigned long lastDemandeStatut = 0;
unsigned long previouscheckTask = 0;

EmodeTurbine modeTurbine = Manuel;


byte lastLoraChecked = 0;

//const char* SSID = "Honor 10";
//const char* PASSWORD = "97540708";
const char* HOSTNAME = "ESP32LoRa";

//battery voltage
uint16_t battery_voltage = 0;
float XS = 0.0028;//0.00225;      //The returned reading is multiplied by this XS to get the battery voltage.
uint16_t MUL = 1000;
uint16_t MMUL = 100;
unsigned long lastBatterycheck = 0;
unsigned long lastBatteryVoltage = 0;
uint16_t FBattery = 3700;


byte localAddress = 0x0A;
long msgCount = 0;
byte displayMode = 0;

double NiveauEtang = 0;
double correctionVanne =0;
double setpoint = -.8;
double pidNiveauEtang = -1 * NiveauEtang;


double OuvertureVanne = 0;
double OuvertureMaxVanne = 1800; 
double pidOuvertureVanne = 0;
double pidOuvertureMaxVanne = -1800; 
double Setpoint = 0;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, 32, NEO_GRB + NEO_KHZ800);


//pid vanne
AutoPID myPID(&pidNiveauEtang, &setpoint, &correctionVanne, pidOuvertureMaxVanne, pidOuvertureVanne, 1, .5, 0.5);



//sauvegarde des données
unsigned long lastSaveData = 0;


Message receivedMessage;


//Static IpAdress
IPAddress local_IP(192,168,1,148);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

enum mode{
	initSTA,
	initSoftAP,
	selectModeWifi,
	selectStation,
	normal,
	testbutton, // TODO remove this
};
mode Mode = initSTA; //TODO change this



String processor(const String& var) {
	//Serial.println(var);
	String retour = String();
	if (var == "BUTTONPLACEHOLDER") {
		String buttons = "";
		buttons += "<h4>Output - LED</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"0\" " + String(localAddress) + "><span class=\"slider\"></span></label>\n";
		//buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + String(localAddress) + "><span class=\"slider\"></span></label>";
		//buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + String(localAddress) + "><span class=\"slider\"></span></label>";
		return buttons;
	}
	else if (var == "ListeLoRa")
	{
		
		for (byte i = 0; i < allBoard->size(); i = i + 1) {
			retour += "<div class=\"card mb-2\" id=\"board-"+ String(allBoard->get(i)->localAddress) + "\">\n";
			retour += "<div class=\"card-header\">";
			retour += "<h4 class= \"card-title\">" + String(allBoard->get(i)->Name) + " <span class=\"text-muted\"> " + String(allBoard->get(i)->localAddress, HEX) +   " " + String(allBoard->get(i)->isConnected()) + "</span></h4>\n";
			retour += "</div>";
			retour += "<div class=\"spinner-border\" role=\"status\" style=\"display:none\">\n<span class=\"visually-hidden\">Loading...</span>\n</div>";
			retour += "<div class = \"card-body\">\n";
			//retour += "<h5>LastMessage: </h5><div class=\"message\">" + allBoard->get(i)->LastMessage.Content + "</div>\n";
			board *temp = allBoard->get(i);
			for (byte j = 0; j < temp->Commands->size(); j++) 
			{
				if (allBoard->get(i)->Commands->get(j).Name != "")
				{
					if (temp->Commands->get(j).Type == "button")
					{
						retour += "<div>";
						retour += "<input class=\"btn btn-outline-dark m-1\" type=\""+ temp->Commands->get(j).Type + "\" name=\""+ temp->Commands->get(j).Name + "\" value=\"" + temp->Commands->get(j).Name + "\" onclick=\"update(this,"+ "'"+temp->Commands->get(j).Action +"')\">\n";
						retour += "</div>";

					} else if (temp->Commands->get(j).Type == "textbox" ||temp->Commands->get(j).Type == "number")
					{
						retour += "<div class=\"form-floating mb-3\">\n";
						retour += "\t<input class=\"form-control\" type=\""+ temp->Commands->get(j).Type + "\" name=\""+ temp->Commands->get(j).Name + "\" value=\"" + temp->Commands->get(j).Value + "\" onchange=\"update(this,"+ "'"+temp->Commands->get(j).Action +"'+'=' +this.value)\">\n";
						retour += "\t<label for=\"\" class=\"form-label\">"+ temp->Commands->get(j).Name + "</label>\n";
						retour += "</div>\n";
					} else if (temp->Commands->get(j).Type == "range")
					{
						retour += "<div class=\"input-group mb-3\">\n";
						retour += "\t<label for=\"timerSlider\" class=\"form-label\">"+ temp->Commands->get(j).Name + "</label>\n";
						retour += "\t<input type=\""+ temp->Commands->get(j).Type + "\" onchange=\"update(this,"+ "'"+temp->Commands->get(j).Action +"'+'=' +this.value)\" id=\"timerSlider\" min=\"0\" max=\"100\" value=\""+temp->Commands->get(j).Value +"\" step=\"1\" class=\"form-range\">\n";
						retour += "</div>\n";
					}
					else if (temp->Commands->get(j).Type == "progress")
					{
					retour += "<div class=\"progress\">\n";
					retour += "\t<div class=\"progress-bar w\" style=\"width: "+ temp->Commands->get(j).Value +"\" role=\"progressbar\" aria-valuenow=\"75\" aria-valuemin=\"0\" aria-valuemax=\"100\"></div>\n";
					retour += "</div>\n";
					} else
					 
					{
						retour += "<p>erreur commande name="+ temp->Commands->get(j).Name +"</p>";
					}
					
					//retour += "<input type=\""+ allBoard->get(i)->Commands[j].Type + "\" name=\""+ allBoard->get(i)->Commands[j].Name + "\" value=\"" + allBoard->get(i)->Commands[j].Name + "\" onclick=\"update(this,"+ "'"+allBoard->get(i)->Commands[j].Action +"')\">\n";

					
				}
			}
			retour += "<button type=\"button\" class=\"btn btn-info btn-sm\" data-bs-toggle=\"modal\" data-bs-target=\"#modal"+ allBoard->get(i)->Name + "\">Open Modal</button>\n";
			retour+= "</div>\n</div>\n";
			
			
		}
		return retour;
		
	}
	else if (var == "ListeLoRaModal")
	{
		

		for (byte i = 0; i < allBoard->size(); i = i + 1) {
			
			retour += "<div class=\"modal fade\" id=\"modal" + allBoard->get(i)->Name + "\" tabindex=\"-1\" aria-labelledby=\"exampleModalLabel\" aria-hidden=\"true\">\n";
			retour += "<div class=\"modal-dialog\">\n";

				
			retour += "<div class=\"modal-content\">\n";
			retour += "<div class=\"modal-header\">\n";
			retour += "<h4 class=\"modal-title\">"+ allBoard->get(i)->Name + " </h4>\n";
			retour += "<button type=\"button\" class=\"btn-close\" data-bs-dismiss=\"modal\" aria-label=\"Close\"></button>\n"; //&times;
			
				retour+= "</div>\n";
				retour+= "<div class=\"modal-body\">\n";
				retour+= "<p>Address: " + String(allBoard->get(i)->localAddress) + "</p>\n";
				retour+= "<pre class=\"message\">" + String(allBoard->get(i)->LastMessage.Content) +"</pre>";
				retour+= "<p>Derniere update : <span class=\"lastUpdate\">" + String(allBoard->get(i)->lastmessage) +"</span> sec</p>";
				retour+= "</div>";
				retour+= "<div clas =\"modal-footer\">\n";
				retour+= "<button type=\"button\" class=\"btn btn-default\" data-bs-dismiss = \"modal\">Close</button>\n";
				retour+= "</div>\n";
				retour+= "</div>\n";

				retour+= "</div>\n";
				retour+= "</div>\n";


		}
		return retour;

	} else if (var == "ModeTurbine")
	{
		
		retour += "<select class=\"selectModeTurbine  form-select w-50\" name=\"text\" onchange=\"sendws('ModeTurbine=' + this.value);\">";
		
		for (size_t i = Manuel	; i <= Auto; i++)
		{
			retour += "<option value=\""+ String(i) + "\">"+ EmodeTurbinetoString(i) + "</option>";
		}
		
		retour += "</select>";
		return retour;
	} else if (var == "StatutSysteme")
	{
		retour += "<div class='container'>";
		retour += "<h2>Statut Systeme:</h2>";
		retour += "Mode Turbine: <div class='ModeTurbine'>" + String(EmodeTurbinetoString(modeTurbine)) + "</div>" ;
		retour += "Ouverture Vanne: <div class='OuvertureVanne'>" + String(OuvertureVanne) + "</div>";

		retour += "</div>";

		return retour;
	} else if (var == "ListeProgram")
	{
		
		for (size_t i = 0; i < ProgrammatedTasks->size(); i++)
		{
			//retour += (String)ProgrammatedTasks->get(i)->name;
			retour += "<div class=\"card border my-2 p-2\" style=\"width: auto\">\n";
			retour += "<h5 class=\"card-title\">" + (String)ProgrammatedTasks->get(i)->name + "</h5>\n";
			//retour += "<h3>" + (String)ProgrammatedTasks->get(i)->name + "</h3>";
			retour += "<form  action =\"/updateprogrammateur\" method=\"post\">";
			retour += "<div class=\"form-group\">";
    		retour += "<label for=\"exampleFormControlInput1\">ID</label>";
    		retour += "<input type=\"number\" class=\"form-control\" name=\"id\" id=\"exampleFormControlInput1\" placeholder=\"id\" value=\""+(String) i +"\" hidden>";
  			retour += "</div>";
			retour += "<div class=\"form-check form-switch\" >";
    		retour += "<input class=\"form-check-input\" type=\"checkbox\" name=\"active\" role=\"switch\" id=\"flexSwitchCheckChecked\" " + String("\%ProgrammatedTasks" + String(i) + ":isActive\%")  + " >";
  			retour += "<label class=\"form-check-label\" for=\"flexSwitchCheckChecked\">Checked switch checkbox input</label>";
			retour += "</div>";
			retour += "<label for=\"appt\">Heure du déclanchement:</label>\n";
			retour += "<input type=\"time\" id=\"appt\" name=\"appt\"  value= \"\%ProgrammatedTasks" + String(i) + ":getHours\%" + ":" + "\%ProgrammatedTasks" + String(i) + ":getMinutes\%" +"\"  required>\n";
			retour += "<label for=\"customRange1\" class=\"form-label\">Example range</label>";
			retour += "<input type=\"range\" value=\"\%ProgrammatedTasks" + String(i) + ":targetVanne\%" + "\" name=\"targetVanne\" class=\"form-range\" id=\"customRange1\">";
			retour += "<label for=\"appte\">DeepSleep (ms) :</label>\n";
			retour += "<input type=\"number\" id=\"appte\" name=\"appt\"  value= \"\%ProgrammatedTasks" + String(i) + ":deepsleep\%"  +"\"  required>\n";
			retour += "<button class=\"btn btn-primary\" type=\"submit\">Mettre a jour</button>";
			retour += "<a href=\"/programmateur/?delete="+(String) i +"\" class=\"btn btn-danger\">Supprimer</a>";
			retour += "</form>";
			retour += "</div>\n";

		} 
		
		
		return retour;
	}else if (var.startsWith("ProgrammatedTasks"))
	{
		String temp = var;
		temp.replace("ProgrammatedTasks","");
		int separateur = temp.indexOf(":");
		int num_tache = temp.substring(0,separateur).toInt();
		String methode = temp.substring(separateur+1,64);
		if (methode == "getHours")
		{
			return ProgrammatedTasks->get(num_tache)->getHours();
		}
		if (methode == "getMinutes")
		{
			return ProgrammatedTasks->get(num_tache)->getMinutes();
		}
		if (methode == "isActive")
		{
			return String(ProgrammatedTasks->get(num_tache)->isActive()? "checked" : "");
		}
		if (methode == "targetVanne")
		{
			return String(ProgrammatedTasks->get(num_tache)->targetVanne );
		}
		if (methode == "deepsleep")
		{
			return String(ProgrammatedTasks->get(num_tache)->deepsleep );
		}
		
		return "erreur processing ProgrammatedTasks";
	}
	
	
	return String("Erreur Processeur template");
	
}
JsonObject getJsonFromFile(DynamicJsonDocument *doc, String filename){
	File myFile = SPIFFS.open(filename);
	if (myFile)
	{
		DeserializationError error = deserializeJson(*doc,myFile);
		if (error)
		{
			return doc->to<JsonObject>();
		}
		return doc->as<JsonObject>();
		
	}else
	{
		return doc->to<JsonObject>();
	}
	
}
// bool appendJsontoFile(DynamicJsonDocument *doc, String filename){
// 	File file = SPIFFS.open(filename, "a");
// 	serializeJson(doc, file);
// 	file.println();
// 	file.close();
// 	return true;
// }
bool saveJsonToFile(DynamicJsonDocument *doc, String filename){
	File myFile = SPIFFS.open(filename, FILE_WRITE);
	if (myFile)
	{
		serializeJson(*doc, myFile);
		myFile.close();
		return true;

	} else {
		return false;
	}
}

void InitBoard(void) {
	EtangBoard.AddCommand("SaveEEPROM", "button", "SEEPROM");
	EtangBoard.AddCommand("ClearEEPROM", "button", "ClearEEPROM");
	EtangBoard.AddCommand("SetMax", "button", "SMAX");
	EtangBoard.AddCommand("SetMin", "button", "SMIN");
	EtangBoard.AddCommand("Veille","button","SLEEP");
	EtangBoard.AddCommand("Veille2","button","SLEEPTP");

	TurbineBoard.AddCommand("OuvertureTotale", "button", "OT");
	TurbineBoard.AddCommand("FermetureTotale", "button", "FT");
	TurbineBoard.AddCommand("+1T  Moteur","button","DEG360");
	TurbineBoard.AddCommand("-1T  Moteur","button","DEG-360");
	TurbineBoard.AddCommand("SetMin","button","SMIN");
	TurbineBoard.AddCommand("SetMax","button","SMAX");
	TurbineBoard.AddCommand("+1T  Vanne","button","DEGV360");
	TurbineBoard.AddCommand("-1T  Vanne","button","DEGV-360");
	TurbineBoard.AddCommand("% Vanne", "range","P",String(OuvertureVanne));
	TurbineBoard.AddCommand("Max Intensité Moteur (mA)", "number","SetMaxI","10000");
	TurbineBoard.AddCommand("Ouverture Vanne", "progress","",String(OuvertureVanne));

	
	localboard.AddCommand("Save data", "button","SDATA");
	localboard.AddCommand("ClearData", "button", "CDATA");
	localboard.AddCommand("Save data","button","SDATA2");
	localboard.AddCommand("Intervalle msg (ms)", "number","ITMSG",String(intervalleEnvoi));
}
bool saveData(void ){
	DynamicJsonDocument doc(100000);
	JsonObject obj;
	obj = getJsonFromFile(&doc,"/data.json");
	serializeJson(obj,Serial);
	//JsonArray data;
	JsonObject objArrayData  =  obj["data"].createNestedObject();


 
	objArrayData["niveauEtang"] = NiveauEtang;
	objArrayData["ouverture"] = OuvertureVanne;
	objArrayData["time"] = timeClient.getEpochTime();

	saveJsonToFile(&doc,"/data.json");
	return true;
}
void TraitementCommande(String c){

	
	if ( c == "CDATA")
	{
		SPIFFS.remove("/data.json");
		File file = SPIFFS.open("/data.json", FILE_WRITE);
		file.close();
	}
	if (c == "SDATA2")
	{
		saveData();
	}
	
	if (c == "SDATA")
	{
		DynamicJsonDocument doc(1024);
 
		JsonObject obj;
		// Open file
		File file = SPIFFS.open("/data.json");
		if (!file) {
			Serial.println(F("Failed to create file, probably not exists"));
			Serial.println(F("Create an empty one!"));
			obj = doc.to<JsonObject>();
		} else {
	
			DeserializationError error = deserializeJson(doc, file);
			if (error) {
				// if the file didn't open, print an error:
				Serial.print(F("Error parsing JSON: "));
				Serial.println(error.c_str());
	
				// create an empty JSON object
				obj = doc.to<JsonObject>();
			} else {
				// GET THE ROOT OBJECT TO MANIPULATE
				obj = doc.as<JsonObject>();
			}
		
 
		}
	
		// close the file already loaded:
		file.close();
	
		obj[F("millis")] = millis();
	
		JsonArray data;
		// Check if exist the array
		if (!obj.containsKey(F("data"))) {
			Serial.println(F("Not find data array! Crete one!"));
			data = obj.createNestedArray(F("data"));
		} else {
			Serial.println(F("Find data array!"));
			data = obj[F("data")];
		}
	
		// create an object to add to the array
		JsonObject objArrayData = data.createNestedObject();
	
		objArrayData["ouverture"] = 80;
		objArrayData["niveau"] = 50;
		objArrayData["name"] = "niveau";
	
		SPIFFS.remove("/data.json");
	
		// Open file for writing
		file = SPIFFS.open("/data.json", FILE_WRITE);
	
		// Serialize JSON to file
		if (serializeJson(doc, file) == 0) {
			Serial.println(F("Failed to write to file"));
		}
	
		// Close the file
		file.close();
	
	}
	// if (c.startsWith("ModeTurbine"))
	// {
	// 	c.remove(0,12);
	// 	Serial.println("ModeTurbine: " + String(c));
		
	// 	switch (c.toInt())
	// 	{
	// 	case 0:
	// 		modeTurbine = Manuel;
	// 		break;
	// 	case 1:
	// 		modeTurbine = Auto;
	// 		break;
	// 	default:
	// 		break;
	// 	}
	// }
	if (c.startsWith("DeepSLEEP"))
	{
		/* code */
	}
	if (c.startsWith("ITMSG="))
	{
		c.replace("ITMSG=","");
		Serial.println("Modif de l'intervalle d'envoi de msg par " + String(c.toInt()));
		intervalleEnvoi = c.toInt();
		preferences.putInt("intervalleEnvoi",intervalleEnvoi);
	}
	
	
	
		
}
board* searchBoardById(int id) {
	for (size_t i = 0; i < allBoard->size(); i++)
	{
		if (id == allBoard->get(i)->localAddress)
		{
			return allBoard->get(i);
		}
		
	}
	//return allBoard->get(0);
	return 0;
}

void RouteHttpSTA() {
	serverHTTP.onNotFound([](AsyncWebServerRequest* request) {
		request->send_P(404, "text/html", "404 notfound");
		});
	
	serverHTTP.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
		Heltec.display->println(String(request->url()));
		if (request->hasParam("b") && request->hasParam("c")) {
			Heltec.display->print(" "+(String) request->getParam("b")->value().c_str());
			Heltec.display->println(" "+(String) request->getParam("c")->value().c_str());
			if (request->getParam("b")->value().toInt() == localAddress)
			{
				Serial.println("Commande pour moi");
				TraitementCommande(request->getParam("c")->value());
			} else
			{
				sendMessage( request->getParam("b")->value().toInt() ,request->getParam("c")->value());
				searchBoardById(request->getParam("b")->value().toInt())->waitforResponse = true;
				LoRa.receive();
			}
			
			
			request->send(200, "text/plain", "J'ai recu: b=" + request->getParam("b")->value() + " et c: " + request->getParam("c")->value() );
			
		} else if (request->hasParam("b") && request->hasParam("DeepSleep"))
		{
			for (size_t i = 1; i < allBoard->size(); i++)
			{
				board* test = allBoard->get(i);
				if (request->getParam("b")->value().toInt() == test->localAddress || request->getParam("b")->value().toInt() == 0xff)
				{
					test->msgToSend += "DeepSleep=" + (String)request->getParam("DeepSleep")->value().c_str();
				}
				
			}
			
			request->send(200,"text/plain","ok pour un sommeil profond");
		} else if (request->hasParam("b") && request->hasParam("LightSleep"))
		{
			for (size_t i = 1; i < allBoard->size(); i++)
			{
				board* test = allBoard->get(i);
				if (request->getParam("b")->value().toInt() == test->localAddress || request->getParam("b")->value().toInt() == 0xff)
				{
					test->msgToSend += "LightSleep=" + (String)request->getParam("LightSleep")->value().c_str();
				}
				
			}
			
			request->send(200,"text/plain","ok pour un sommeil leger");
		}
		
		
	});
	serverHTTP.on("/maj", HTTP_GET, [](AsyncWebServerRequest* request) {
		String retour = "{ \"msSystem\" :" + String(millis()) + "," +
			//"\"modeTurbine\":\"" + String(EmodeTurbinetoString(modeTurbine)) + "\"," + 
			"\"modeTurbine\": {\"name\":\" " + String(EmodeTurbinetoString(modeTurbine)) + "\", \"id\":"+ modeTurbine + "}," + 
		"\"boards\" : [" + allBoard->get(0)->toJson() + "," + allBoard->get(1)->toJson() + "," + allBoard->get(2)->toJson() + "]}";
		//Serial.println("maj" + String(retour));
		request->send(200, "application/json", retour);
		});
	serverHTTP.on("/liste", HTTP_GET, [](AsyncWebServerRequest* request) {
		String retour = "[";
		File root = SPIFFS.open("/");
		if (root.isDirectory()) {
			File file = root.openNextFile();
			while (file) {
				if (retour != "[") {
					retour += ',';
				}
				retour += "{\"type\":\"";
				retour += (file.isDirectory()) ? "dir" : "file";
				retour += "\",\"name\":\"";
				retour += String(file.name()).substring(1);
				retour += "\"}";
				file = root.openNextFile();
			}
		}
		else
		{
			retour += "{\"not directory \"}";
		}
		retour += "]";
		request->send(200, "application/json", retour);
		});
	serverHTTP.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		
		File file = SPIFFS.open("/index.html");
		request->send(SPIFFS,"/index.html", "text/html", false, processor);
		});
	serverHTTP.on("/programmateur/new", HTTP_GET, [](AsyncWebServerRequest* request) {
		ProgrammatedTasks->add(new ProgrammatedTask(12,12,"test ajout"));
		
		//request->send(SPIFFS,"/programmateur.html", "text/html", false, processor);
		request->redirect("/programmateur");
		});
	
	serverHTTP.on("/programmateur", HTTP_GET, [](AsyncWebServerRequest* request) {
		if (request->hasParam("delete"))
		{
			Serial.println("suppression");
			Serial.println(request->getParam("delete")->value().c_str());
			ProgrammatedTasks->remove(request->getParam("delete")->value().toInt());
			request->redirect("/programmateur")	;	
		}
		
		
		request->send(SPIFFS,"/programmateur.html", "text/html", false, processor);
		});
	
	serverHTTP.on("/Programmateur.js", HTTP_GET, [](AsyncWebServerRequest* request) {
		
		
		request->send(SPIFFS,"/Programmateur.js", "application/javascript");
		});
	serverHTTP.on("/updateprogrammateur", HTTP_POST , [](AsyncWebServerRequest* request) {
		
		logPrintlnI("/updateprog");
		int params = request->params();
		for(int i=0;i<params;i++){
			AsyncWebParameter* p = request->getParam(i);
			if(p->isFile()){ //p->isPost() is also true
				Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
			} else if(p->isPost()){
				Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
			} else {
				Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
			}
		}
		//List all parameters (Compatibility)
		int args = request->args();
		for(int i=0;i<args;i++){
			Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
		}
		ProgrammatedTask *test;
		if (request->hasParam("id",true))
		{
			test = ProgrammatedTasks->get(request->getParam("id",true)->value().toInt());
			
		} else
		{
			request->send(400);
			return;
		}
		
		if (request->hasParam("active",true))
		{
			test->activate();
		} else
		{
			test->deactivate();
		}
		if (request->hasParam("appt",true))
		{
			int dp = request->getParam("appt",true)->value().indexOf(":");
			Serial.println("dp " + String(dp));
			test->h = request->getParam("appt",true)->value().substring(0,dp).toInt();
			test->m = request->getParam("appt",true)->value().substring(dp+1,10).toInt();
		}
		if (request->hasParam("targetVanne",true))
		{
			test->targetVanne = request->getParam("targetVanne",true)->value().toInt();
		}
		
		
		
		request->send(200,"text/json","{\"ok\":1}");
		});
	serverHTTP.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/app.js", "application/javascript");
		});
	serverHTTP.on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/manifest.json", "application/javascript");
		});
	serverHTTP.on("/icons/logo-144.png", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/icons/logo-144.png", "application/javascript");
		});
	serverHTTP.on("/icons/logo-192.png", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/icons/logo-192.png", "application/javascript");
		});
	serverHTTP.on("/service-worker.js", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/service-worker.js", "application/javascript");
		});
	serverHTTP.on("/data.json", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/data.json", "application/json");
		});
	
	serverHTTP.on("/secret.html",HTTP_GET,[](AsyncWebServerRequest* request){
		if(!request->authenticate("admin", "admin")){
			return request->requestAuthentication();
		}
    		
  		request->send(SPIFFS , "/secret.html" ,"text/plain");
	});
	serverHTTP.on("/websocket.js", HTTP_GET, [](AsyncWebServerRequest* request){
		request->send(SPIFFS, "/websocket.js", "application/javascript");
	});
	Serial.println("Configuration Route ok");
	
	
}

void RouteHttpAP(){
	serverHTTP.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
      Serial.println("erer");
      request->send(SPIFFS,"/wifimanager.html","text/html",false,processor);
    });
    serverHTTP.on("/style.css",HTTP_GET,[](AsyncWebServerRequest * request){
      request->send(SPIFFS,"/style.css","");
    });
    serverHTTP.on("/ping.js",HTTP_GET,[](AsyncWebServerRequest * request){
      request->send(SPIFFS,"/ping.js","");
    });
    
    serverHTTP.on("/",HTTP_POST,[](AsyncWebServerRequest *request){
      int params = request->params();
      for (int i = 0; i < params; i++)
      {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost())
        {
			// TODO
        //   if (p->name()== PARAM_INPUT_1)
        //   {
        //     ssid = p->value().c_str();
        //     Serial.print("SSID set to: ");
        //     Serial.println(ssid);
        //     writeFile(SPIFFS,ssidPath,ssid.c_str());
        //   }
        //   if (p->name() == PARAM_INPUT_2) {
        //     pass = p->value().c_str();
        //     Serial.print("Password set to: ");
        //     Serial.println(pass);
        //     // Write file to save value
        //     writeFile(SPIFFS, passPath, pass.c_str());
        //   }
        //   // HTTP POST ip value
        //   if (p->name() == PARAM_INPUT_3) {
        //     ip = p->value().c_str();
        //     Serial.print("IP Address set to: ");
        //     Serial.println(ip);
        //     // Write file to save value
        //     writeFile(SPIFFS, ipPath, ip.c_str());
        //  }
          
        }
        
      }
      //request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      request->send(SPIFFS,"/redirect.html","text/html",false,processor);
      delay(5000);
      //ESP.restart();
    });
	Serial.println("Route HTTP AP Ok!");
}
void WifiEvent(WiFiEvent_t event){
	Serial.printf("[WiFi-event] event: %d\n", event);
	switch (event) {
    case SYSTEM_EVENT_WIFI_READY: 
      Serial.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("Connected to access point");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi access point");
      //WiFi.begin(ssid, password);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("Obtained IP address: ");
      Serial.println("	local IP: " + (String)WiFi.localIP().toString());
	  Serial.println("	DNS   IP: " + (String)WiFi.dnsIP().toString());
	  Serial.println("	MAC     : " + (String)WiFi.macAddress());
	  

	  timeClient.begin();
		if(!timeClient.update()) {
			timeClient.forceUpdate();
		}
	  Serial.println("Time: " + (String)timeClient.getFormattedTime());
	  ArduinoOTA.begin();
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
      Serial.println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      Serial.println("Received probe request");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      Serial.println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
    default: break;
	}
}

String wifiStatusToString(wl_status_t Status) {
	switch (Status)
	{
	case WL_NO_SHIELD:
		return "WL_No_Shield";
		break;
	case WL_IDLE_STATUS:
		return "WL_IDLE_STATUS";
		break;
	case WL_NO_SSID_AVAIL:
		return "WL_NO_SSID_AVAIL";
		break;
	case WL_SCAN_COMPLETED:
		return "WL_SCAN_COMPLETED";
		break;
	case WL_CONNECTED:
		return "WL_CONNECTED";
		break;
	case WL_CONNECT_FAILED:
		return "WL_CONNECT_FAILED";
		break;
	case WL_CONNECTION_LOST:
		return "WL_CONNECTION_LOST";
		break;
	case WL_DISCONNECTED:
		return "WL_CONNECT_FAILED";
		break;
	default:
		return "default";
		break;
	}
}

void displayData(void) {
	Heltec.display->clear();
	switch (displayMode)
	{
	case 0:
		Heltec.display->drawString(0, 0, "localAddress : 0x" + String(localAddress, HEX));
		Heltec.display->drawString(0, 15, wifiStatusToString( WiFi.status()));
		
		Heltec.display->drawString(0, 27, "http://" + String(HOSTNAME) + ".local");
		if (WiFi.isConnected())
		{
			Heltec.display->drawString(40, 45, String(WiFi.localIP().toString()));
		} else
		{
			Heltec.display->drawString(40, 45, "not connected");
		}
		
		
				
		break;
	case 1:
		Heltec.display->drawLogBuffer(0, 0);



		break;
	case 2:
		Heltec.display->drawString(0,0,"Mode Vanne: ");
		Heltec.display->drawString(0,25,String(EmodeTurbinetoString( modeTurbine)));
		
		
		break;
	case 3:
	
		Heltec.display->drawString(0, 0, "Remaining battery still has:");
   		Heltec.display->drawString(0, 10, "VBAT:");
		Heltec.display->drawString(35, 10, (String)battery_voltage);
		Heltec.display->drawString(60, 10, "(mV)");
		if (lastBatteryVoltage < battery_voltage)
		{
			Heltec.display->drawString(60, 20, "^");
		} else
		{
			Heltec.display->drawString(60, 20, "v");
		}
		
		
		break;
	case 4:
		Heltec.display->drawString(0,0,"Nb: " + String(allBoard->size()-1));
		Heltec.display->drawString(80,0,"C ");
		Heltec.display->drawString(100,0,"WR");
		for (size_t i = 1; i < allBoard->size(); i++)
		{
			Heltec.display->drawString(50,(i-1)*12+15,(String)allBoard->get(i)->localAddress);
			Heltec.display->drawString(0,(i-1)*12+15,(String)allBoard->get(i)->Name);
			Heltec.display->drawString(80,(i-1)*12+15,(String)allBoard->get(i)->isConnected());
			Heltec.display->drawString(100,(i-1)*12+15,(String)allBoard->get(i)->waitforResponse);
		}
		
		break;
	case 5:
		Heltec.display->drawString(0,0,"Niveau Etang " + String(NiveauEtang));
		Heltec.display->drawString(0,18,"Niveau Vanne " + String(OuvertureVanne));
		Heltec.display->drawString(0,36,"intervalMsg "+ String(intervalleEnvoi/1000) + "s");

		break;
	default:
		Heltec.display->drawString(0,0,"erreur indice affichage");
		break;
	}
	
	Heltec.display->display();

}

void AffichagePixel(void){
	static int brightness = 0;
	static int sensBrigntness = 1;
	if (brightness >= 255)
	{
		sensBrigntness = -1;
	} 
	if (brightness <= 0)
	{
		sensBrigntness = 1;
	} 
	brightness += sensBrigntness;
	strip.setBrightness(brightness);
	strip.clear();
	
	int nombre = (NiveauEtang * strip.numPixels())  ;
	//Serial.println("nombre led : " + String(nombre) + " max: " + String(strip.numPixels()));
	for (int i = 0; i < nombre; i++)
	{
		
		if (i <= strip.numPixels())
		{
			strip.setPixelColor(i, strip.Color(0, 0, 127));
		} else
		{
			strip.setPixelColor(i-1, strip.Color(48, 0, 127));
		}
		
		
	}
	strip.show();
}




void deserializeResponse(byte board, String Response){
	
	
	if (!Response.startsWith("{"))
	{
		Serial.println(" ce n'est pas du JSON");
		if (Response.startsWith("ok"))
		{
			/* code */ //TODO
		}
		
		return;
	}
	
	logPrintlnV("c'est du json");
	StaticJsonDocument<200> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, Response);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  switch (board)
  {
	case ETANG:
		NiveauEtang = doc["Niveau"];
		pidNiveauEtang = -1 * NiveauEtang;
		break;
	case TURBINE:
		if (doc.containsKey("Ouverture") && doc.containsKey("OuvMaxCodeur") && doc.containsKey("Setpoint"))
		{
			Serial.println("J'ai bien les key du json de turbine");
			OuvertureVanne = doc["Ouverture"];
			OuvertureMaxVanne = doc["OuvMaxCodeur"];
			Setpoint = doc["Setpoint"];
			pidOuvertureMaxVanne = -1*OuvertureMaxVanne;
			pidOuvertureVanne = -1*OuvertureVanne;
			myPID.setOutputRange(  (-1)* (OuvertureMaxVanne - OuvertureVanne), (-1 * OuvertureVanne) );
			//Serial.println("le nouveau min: " + String(myPID.getOutputMin()));
			//Serial.println("le nouveau max: " + String(myPID.getOutputMax()));
			
		} else
		{
			Serial.println("Je n'ai pas les key du json de turbine");
		}
		
		
		
		break;
	default:
		break;
  }
  
  
  
  
}

void onReceive(int packetSize)
{
	if (packetSize == 0) return;          // if there's no packet, return
	logPrintlnI("onreceive");
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
	

	board *test = searchBoardById(receivedMessage.sender);
	Serial.println(test->Name);
	test->lastmessage = millis();
	test->LastMessage = receivedMessage;
	test->newMessage = true;
	
	Heltec.display->println("0x" + String(receivedMessage.sender,HEX) + " to 0x" + String(receivedMessage.recipient, HEX) );
	
	
	
	
	

}

/**
 * @brief initialisation du wifi
 * 
 */
void  initWifi(void)
{

	WiFi.mode(WIFI_STA);
	// Configures static IP address

	#if defined(USEIPFIXE)
	
		if (!WiFi.config(local_IP, gateway, subnet,gateway,secondaryDNS)) { //8,8,8,8
			Serial.println("STA Failed to configure");
		}
	
	#endif // USEIPFIXE
	
	
	
	if (WiFi.setHostname("esp32Lora22"))
	{
		Serial.println("hostname ok");
	}
	Heltec.display->clear();
	Serial.print("Connecting to ");
	Serial.println(paramWifi.SSID);
	Heltec.display->drawString(0, 00, "Connecting to ");
	Heltec.display->drawString(00, 10, paramWifi.SSID);
	Heltec.display->display();

	WiFi.onEvent(WifiEvent);

	
	  
	ArduinoOTA.setHostname("Esp32 Lora");
	// Mode de connexion
	

	WiFi.begin(paramWifi.SSID, paramWifi.Credential);
	
	// Start Web Server
	serverHTTP.begin();

	Heltec.display->drawString(0, 40, "server started");
	Heltec.display->display();
	if (MDNS.begin(HOSTNAME)) {
		MDNS.addService("http", "tcp", 80);
		Serial.println("MDNS responder started");
		Serial.print("You can now connect to http://");
		Serial.print(HOSTNAME);
		Serial.println(".local");
		
	}

	
}

void InitSD(void) {

	Heltec.display->clear();
	Heltec.display->drawString(0, 0, "Init SD Card");

	SPIClass(1);

	spi1.begin(17, 13, 23, 22);

	if (!SD.begin(22, spi1)) {
		Serial.println("Card Mount Failed");
		Heltec.display->drawString(0, 10, "Card Mount Failed");
		Heltec.display->display();
		delay(200);
		return;
	}
	else
	{
		Heltec.display->drawString(0, 10, "Card Mount ok");
	}
	Heltec.display->display();
	delay(200);

}



void gestionPower(void){
	if (millis()- lastBatterycheck >=  + 15000)
   {
	   lastBatterycheck = millis();
	   lastBatteryVoltage = battery_voltage;
	   battery_voltage = analogRead(37)*XS*MUL; //37
   }
   Heltec.VextON();
}

void handleMode(){
	switch (Mode)
	{
	case initSTA:
		Heltec.display->clear();
		
		WiFi.disconnect();
		WiFi.mode(wifi_mode_t::WIFI_MODE_STA);
		// if (!WiFi.config(IP, gateway, subnet,gateway)){
		// Serial.println("STA Failed to configure");
		
		// }
		if (WiFi.setHostname("esp32Lora22"))
		{
			Serial.println("hostname ok");
		}
		Serial.print("Connecting to ");
		Serial.println(paramWifi.SSID);
		Heltec.display->drawString(0, 00, "Connecting to ");
		Heltec.display->drawString(00, 10, paramWifi.SSID);
		Heltec.display->display();

		WiFi.onEvent(WifiEvent);

		ArduinoOTA.setHostname("Esp32 Lora");

		WiFi.begin(paramWifi.SSID, paramWifi.Credential);
	
		// Start Web Server
		serverHTTP.begin();

		if (MDNS.begin(HOSTNAME)) {
			MDNS.addService("http", "tcp", 80);
			Serial.println("MDNS responder started");
			Serial.print("You can now connect to http://");
			Serial.print(HOSTNAME);
			Serial.println(".local");
			
		}

		initWebSocket();
		RouteHttpSTA();

		delay(200);
		Mode = mode::normal;
		break;
	case mode::initSoftAP:
		Serial.println("ok1");
		Heltec.display->clear();
		Heltec.display->drawString(0,0,"mode AP");
		WiFi.disconnect();
		
		Serial.println("ok");
		WiFi.mode(WIFI_AP);
		serverHTTP.begin();
		RouteHttpAP();
    	WiFi.softAP("ESP-WIFI-Manager", NULL);
		
		Heltec.display->display();
		Mode = mode::normal;
		break;
	case mode::normal:
		
		
		displayData();
		if (prgButton->frontMontant())
			{
				if (displayMode < 6 )
			{
				displayMode++;
			}
			else
			{
				displayMode = 0;
			}
		}
			
		if (prgButton->pressedTime() > 2000)
		{
			Mode = mode::selectModeWifi;
		}
		break;
	case mode::selectStation:
		Heltec.display->clear();
		Heltec.display->drawString(0,0,"Select Station");
		
		menuStationWifi.loop();
		menuStationWifi.render();
		
		if (prgButton->frontDesceandant())
		{
			paramWifi = wifiParams[menuStationWifi.select];
			Serial.println(paramWifi.SSID);
			Mode = mode::initSTA;
		}
		
		Heltec.display->display();
		break;
	case mode::selectModeWifi:
		Heltec.display->clear();
		Heltec.display->drawString(0,0,"MENU mode Wifi");
		// int decalage;
		// decalage =14;
		menuModeWifi.loop();
		menuModeWifi.render();
		// static unsigned long dernierchangement = 0;
		// if (encodRight->frontDesceandant() && !encodLeft.frontDesceandant())
		// {
		// 	if (millis() - dernierchangement > 100)
		// 	{
		// 		dernierchangement = millis();
		// 		menuModeWifi.selectNext();
		// 		Serial.println((String) millis() + " next");
		// 	}else
		// 	{
		// 		Serial.println("cancel");
		// 	}
			
			
			
		// }
		// if (encodLeft.frontDesceandant() && !encodRight->frontDesceandant())
		// {
		// 	if (millis() - dernierchangement > 100)
		// 	{
		// 		Serial.println((String) millis() + " previous");
		// 		dernierchangement = millis();
		// 		menuModeWifi.selectPrevious();
		// 	}else
		// 	{
		// 		Serial.println("cancel");
		// 	}
			
		// }
		
		
		// for (size_t i = 0; i < menuModeWifi.maxRow; i++)
		// {
		// 	// if (modeWifi[i+menuModeWifi.first].selected)
		// 	// {
		// 	// 	Heltec.display->fillRect(0,i*12+decalage,120,12);
		// 	// 	Heltec.display->setColor(OLEDDISPLAY_COLOR::BLACK);
		// 	// }else
		// 	// {
		// 	// 	Heltec.display->setColor(OLEDDISPLAY_COLOR::WHITE);
		// 	// }
		
		// 	Heltec.display->drawString(12,i*12+decalage,modeWifi[i+menuModeWifi.first]);
			
		// 	if (menuModeWifi.select == i+menuModeWifi.first)
		// 	{
		// 		//Heltec.display->fillRect(2,i*12+2,8,8);
		// 		Heltec.display->fillCircle(6,i*12+decalage+6,3);
		// 	}
		
		
		// }
		if (prgButton->frontDesceandant())
		{
			if (modeWifi[menuModeWifi.select] == "AP")
			{
				Mode = mode::initSoftAP;
			}
			if (modeWifi[menuModeWifi.select]== "STA")
			{
				Mode = mode::selectStation;
			}
			if (modeWifi[menuModeWifi.select]== "ScanWifi")
			{
				// TODO Mode = mode:: ;
			}
			

		}
		
		Heltec.display->display();
		break;
	case mode::testbutton:	//TODO Remove This
		testbtn->loop();

		Serial.print("S "+ (String)testbtn->getState());
		Serial.print(" pressed "+ (String)testbtn->isPressed());
		Serial.print(" released "+ (String)testbtn->isReleased());
		Serial.print(" fd "+ (String)testbtn->frontDesceandant());
		Serial.print(" fm "+ (String)testbtn->frontMontant());
		Serial.print(" time "+ (String)testbtn->pressedTime());
		Serial.println("");
		delay(200);
		break;
	default:
		break;
	}
}
void acquisitionEntree(void){
	prgButton->loop();
	encodLeft->loop();
	encodRight->loop();
}
// the setup function runs once when you press reset or power the board
void setup() {
	
	ProgrammatedTasks->add(new ProgrammatedTask(18,0,"test1"));
	ProgrammatedTasks->add(new ProgrammatedTask(21,00,"DeepSleep"));

	ProgrammatedTasks->get(1)->activate();
	ProgrammatedTasks->get(1)->deepsleep = 43200000;

	allBoard->add(&localboard);
	allBoard->add(&TurbineBoard);
	allBoard->add(&EtangBoard);
	allBoard->add(&RadiateurBoard);
	
	Logger::instance().setSerial(&Serial);
	
	menuModeWifi.onRender([](int num, int numel,bool hover){
		//Serial.println("onrendermodeWifi " + (String)num + (String)numel);
		Heltec.display->drawString(12,num*12+14,modeWifi[numel]);
		if (hover)
		{
			Heltec.display->fillCircle(6,num*12+14+6,3);
		}
		
	});
	menuStationWifi.onRender([](int num, int numel,bool hover){
		
		//Serial.println("onrenderStation " + (String)num + (String)numel);
		Heltec.display->drawString(10,num*12+14,wifiParams[numel].SSID);
		if (hover)
		{
			Heltec.display->fillCircle(6,num*12+14+6,3);
		}
		
	});
	
	Heltec.begin(true, true, true, true, BAND);
	// InitSD();
	// initWifi();

	
	Heltec.display->clear();
	if (!SPIFFS.begin(true)) {
		Serial.println("SPIFFS Mount Failed");
		Heltec.display->drawString(0,10,"SPIFFS Failed");
		return;
	} else
	{
		Heltec.display->drawString(0,10,"SPIFFS OK");
	}
	if (preferences.begin("Master",false))
	{
		Serial.println("Preference Initialized");
		Heltec.display->drawString(0,22,"Preference Initialized");
		
		intervalleEnvoi = preferences.getInt("intervalleEnvoi",intervalleEnvoi);
	}
	InitBoard();
	Heltec.display->display();
	pinMode(25, OUTPUT);


	
	paramWifi = wifiParams[defaultConnectWifi];
	
	LoRa.setSpreadingFactor(8);
	LoRa.setSyncWord(0x12);
	LoRa.setSignalBandwidth(125E3);
	LoRa.onReceive(onReceive);
	
	LoRa.receive();

	Serial.println("Heltec.LoRa init succeeded.");
	delay(200);
	Heltec.display->clear();
	Heltec.display->flipScreenVertically();
	Heltec.display->setLogBuffer(5, 100);

	
	

	strip.begin();
  	strip.setBrightness(20);
  	strip.show(); // Initialize all pixels to 'off'
	strip.setPixelColor(1, strip.Color(127, 0, 0));
   	strip.show();

	myPID.setTimeStep(5000);

	

	//battery power
	//adcAttachPin(13);
	//analogSetClockDiv(255); // 1338mS
	
	
}

// the loop function runs over and over again until power down or reset
void loop() {
	if (WiFi.status() == WL_CONNECTED){
		timeClient.update();
	}
	
   	acquisitionEntree();
   	handleMode();
	AffichagePixel();

	myPID.run();


   
	//gestionPower();
   

	for (size_t i = 1; i < allBoard->size(); i++)
	{
		
		if (allBoard->get(i)->newMessage)
		{
			allBoard->get(i)->newMessage = false;
			deserializeResponse(allBoard->get(i)->localAddress, allBoard->get(i)->LastMessage.Content);
			
			
			if (allBoard->get(i)->waitforResponse )//&& allBoard->get(1)->LastMessage.Content == "ok"
			{
				
				allBoard->get(i)->waitforResponse = false;
				
				ws.textAll("{\"confirmationReception\" : \"" + (String) allBoard->get(i)->localAddress + "\"}");
			
			}
		}
		if (allBoard->get(i)->msgToSend != "")
		{
			logPrintlnW("Envoi "+ (String) allBoard->get(i)->localAddress + " "+ (String) allBoard->get(i)->msgToSend );
			sendMessage(allBoard->get(i)->localAddress,allBoard->get(i)->msgToSend );
			allBoard->get(i)->msgToSend = "";
			delay(200);
		}
		

		//  allBoard->get(i)->demandeStatut();
		//  LoRa.receive();

	}
	
	if (millis()- lastDemandeStatut > intervalleEnvoi)
	{
		Serial.println("lastlorachecked " + String(lastLoraChecked));
		lastDemandeStatut = millis();
		// if (lastLoraChecked > 12 || lastLoraChecked == 0)
		// {
		// 	lastLoraChecked = 11;
		// } 

		if (lastLoraChecked > allBoard->size()-1 || lastLoraChecked == 0)
		{
			lastLoraChecked = 1;
		} 
			
		Serial.println("Demande Statut: 0x" + (String)allBoard->get(lastLoraChecked)->localAddress );
		allBoard->get(lastLoraChecked)->sendMessage(allBoard->get(lastLoraChecked)->localAddress, "DemandeStatut",Master); 
		allBoard->get(lastLoraChecked)->waitforResponse = true;
		
		//sendMessage(lastLoraChecked, "DemandeStatut");
		
		LoRa.receive();
		lastLoraChecked++;
		

		
		
	}
	
	
	static uint32_t previousMillis = 0;
	// if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
	if ((WiFi.status() != WL_CONNECTED) && (millis() - previousMillis >= 30000)) {
		Serial.print(millis());
		Serial.println("Reconnecting to WiFi...");
		WiFi.disconnect();
		WiFi.reconnect();
		previousMillis = millis();
	}
	

	

	
	if (millis()>previouscheckTask + 60000)
	{
		previouscheckTask = millis();
		for (size_t i = 0; i < ProgrammatedTasks->size(); i++)
		{
			ProgrammatedTask *tache = ProgrammatedTasks->get(i);
			if (tache->isActive())
			{
				if (timeClient.getHours() == tache->h && timeClient.getMinutes() == tache->m  )
				{
					Serial.println("exec Tache");
					if (tache->deepsleep != 0)
					{
						for (size_t i = 1; i < allBoard->size()	; i++)
						{
							allBoard->get(i)->msgToSend += "DeepSleep="+ String(tache->deepsleep);
						}
						
					}
					
				}
				
			}
			
		}
		
	}
	
	ws.cleanupClients();
	//ArduinoOTA.handle();

	delay(10);
}
