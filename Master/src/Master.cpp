/*
 Name:		Master.ino
 Created:	21/05/2021 18:31:59
 Author:	Baptou
*/

// the setup function runs once when you press reset or power the board

//#include <AsyncTCP.h>

//#include <HTTP_Method.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SD.h>
#include <WiFi.h>
#include <heltec.h>
#include "TurbineEtangLib.h"
#include <FS.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <AutoPID.h>

#include "websocket.h"




#define BAND 868E6
#define PRGButton 0

bool previousEtatbutton = false;
SPIClass spi1;

// Set web server port number to 80
//WiFiServer serverHTTP(80);	//80 est le port standard du protocole HTTP
AsyncWebServer serverHTTP(80);	//80 est le port standard du protocole HTTP
AsyncWebSocket ws("/ws");

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

typedef enum {
	Manuel,
	Auto
}EmodeTurbine;

String EmodeTurbinetoString(size_t m){
	switch (m)
	{
	case Manuel:
		return "Manuel";
		break;
	case Auto:
		return "Auto";
		break;
	default:
		return "default";
		break;
		
	}
}
EmodeTurbine modeTurbine = Manuel;
const char* ntpServer = "pool.ntp.org";

// Definition de parametre Wifi

const char* SSID = "Livebox-44C1";
const char* PASSWORD = "20AAF66FCE1928F64292F3E28E";

//const char* SSID = "Honor 10";
//const char* PASSWORD = "97540708";
const char* HOSTNAME = "ESP32LoRa";

byte localAddress = 0x0A;
long msgCount = 0;
byte displayMode = 0;

double NiveauEtang = 0.5;
double correctionVanne =0;
double setpoint = -.8;
double pidNiveauEtang = -1 * NiveauEtang;


double OuvertureVanne = 0;
double OuvertureMaxVanne = 1800; 
double pidOuvertureVanne = 0;
double pidOuvertureMaxVanne = -1800; 

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
unsigned long lastCorrectionVanne = 0;

class Command
{
public:
	String Name;
	String Type;
	String Action;

private:

};


class board
{
public:
	board(String N, byte Address) {
		Name = N;
		localAddress = Address;
		
	}
	String Name;
	byte localAddress;
	bool connected = false;
	unsigned long lastmessage = 0;
	Message LastMessage;
	String toJson() {
		if (this->localAddress == MASTER)
		{
			this->lastmessage = millis();
		}
		
		return "{ \"Name\" : \"" + Name + "\"," +
			"\"localAddress\" : " + localAddress + "," +
			"\"lastMessage\" : " + this->LastMessage.toJson() + "," +
			"\"lastUpdate\" : " + lastmessage + "" + 
			"}";
	};
	bool newMessage = false;
	Command Commands[10];
	void AddCommand(String Name, int id, String Type, String Action) {
		Commands[id].Action = Action;
		Commands[id].Name = Name;
		Commands[id].Type = Type;
	}

private:
	
};

Message receivedMessage;

board localboard("Master", MASTER);
board EtangBoard("Etang", ETANG);

board TurbineBoard("Turbine", TURBINE);

board *allBoard[3] = {  &localboard, &EtangBoard , &TurbineBoard};


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="fr">
<head>
<meta charset="UTF-8">
<title>EtangTurbine</title>
<style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
	.container{background-color: #ccc; border:1px #b30000 solid; width: 300px; margin: auto;}
  </style>
</head>
<body>
<h1>Bonjour</h1>
%BUTTONPLACEHOLDER%
<h2>%test%</h2>
<div class="container">%ListeLoRa%</div>
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}
function maj(){
var xhr = new XMLHttpRequest();
xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
     
	var myObj = JSON.parse( this.responseText);
for (i in myObj.boards) {
  x = document.getElementById('board-' + myObj.boards[i].Name)
	x.getElementsByClassName("message")[0].innerHTML = myObj.boards[i].lastMessage.content
//console.log(x);
}
console.log(myObj);

    }
  };
xhr.open("GET", "/maj", true);
xhr.send();
setTimeout(maj,5000);
}
maj();
</script>
</body>
</html>
)rawliteral";
String processor(const String& var) {
	//Serial.println(var);
	if (var == "BUTTONPLACEHOLDER") {
		String buttons = "";
		buttons += "<h4>Output - LED</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"0\" " + String(localAddress) + "><span class=\"slider\"></span></label>\n";
		//buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + String(localAddress) + "><span class=\"slider\"></span></label>";
		//buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + String(localAddress) + "><span class=\"slider\"></span></label>";
		return buttons;
	}
	else if (var == "ListeLoRa")
	{
		String retour = "";
		
		for (byte i = 0; i < 3; i = i + 1) {
			retour += "<div class=\"card mb-2\" id=\"board-"+ String(allBoard[i]->localAddress) + "\">\n";
			retour += "<h4 class= \"card-title\">" + String(allBoard[i]->Name) + " <span class=\"text-muted\"> " + String(allBoard[i]->localAddress, HEX) +   " " + String(allBoard[i]->connected) + "</span></h4>\n";
			
			retour += "<div class = \"card-body\">\n";
			//retour += "<h5>LastMessage: </h5><div class=\"message\">" + allBoard[i]->LastMessage.Content + "</div>\n";
			for (byte j = 0; j < 10; j++) 
			{
				if (allBoard[i]->Commands[j].Name != "")
				{
					if (allBoard[i]->Commands[j].Type == "button")
					{
						retour += "<input type=\""+ allBoard[i]->Commands[j].Type + "\" name=\""+ allBoard[i]->Commands[j].Name + "\" value=\"" + allBoard[i]->Commands[j].Name + "\" onclick=\"update(this,"+ "'"+allBoard[i]->Commands[j].Action +"')\">\n";

					} else if (allBoard[i]->Commands[j].Name == "textbox")
					{
						retour += "<input type=\""+ allBoard[i]->Commands[j].Type + "\" name=\""+ allBoard[i]->Commands[j].Name + "\" value=\"" + allBoard[i]->Commands[j].Name + "\" onclick=\"update(this,"+ "'"+allBoard[i]->Commands[j].Action +"')\">\n";
					}
					//retour += "<input type=\""+ allBoard[i]->Commands[j].Type + "\" name=\""+ allBoard[i]->Commands[j].Name + "\" value=\"" + allBoard[i]->Commands[j].Name + "\" onclick=\"update(this,"+ "'"+allBoard[i]->Commands[j].Action +"')\">\n";

					
				}
			}
			retour += "<button type=\"button\" class=\"btn btn-info btn-sm\" data-bs-toggle=\"modal\" data-bs-target=\"#modal"+ allBoard[i]->Name + "\">Open Modal</button>\n";
			retour+= "</div>\n</div>\n";
			
			
		}
		return retour;
		
	}
	else if (var == "ListeLoRaModal")
	{
		String retour = "";

		for (byte i = 0; i < 3; i = i + 1) {
			
			retour += "<div class=\"modal fade\" id=\"modal" + allBoard[i]->Name + "\" tabindex=\"-1\" aria-labelledby=\"exampleModalLabel\" aria-hidden=\"true\">\n";
			retour += "<div class=\"modal-dialog\">\n";

				
			retour += "<div class=\"modal-content\">\n";
			retour += "<div class=\"modal-header\">\n";
			retour += "<h4 class=\"modal-title\">"+ allBoard[i]->Name + " </h4>\n";
			retour += "<button type=\"button\" class=\"btn-close\" data-bs-dismiss=\"modal\" aria-label=\"Close\"></button>\n"; //&times;
			
				retour+= "</div>\n";
				retour+= "<div class=\"modal-body\">\n";
				retour+= "<p>Address: " + String(allBoard[i]->localAddress) + "</p>\n";
				retour+= "<pre class=\"message\">" + String(allBoard[i]->LastMessage.Content) +"</pre>";
				retour+= "<p>Derniere update : <span class=\"lastUpdate\">" + String(allBoard[i]->lastmessage) +"</span> sec</p>";
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
		String retour = "";
		retour += "<select name=\"text\" onchange=\" updateb('10','ModeTurbine=' + this.value); websocket.send('modeturbine=auto');\">";
		
		for (size_t i = Manuel	; i <= Auto; i++)
		{
			retour += "<option value=\""+ String(i) + "\">"+ EmodeTurbinetoString(i) + "</option>";
		}
		
		retour += "</select>";
		return retour;
	}
	return String();
	
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
	EtangBoard.AddCommand("SaveEEPROM", 0,"button", "SEEPROM");
	EtangBoard.AddCommand("ClearEEPROM", 1, "button", "ClearEEPROM");
	EtangBoard.AddCommand("SetMax", 2, "button", "SMAX");
	EtangBoard.AddCommand("SetMin", 3, "button", "SMIN");
	EtangBoard.AddCommand("Veille",4,"button","SLEEP");
	EtangBoard.AddCommand("Veille2",5,"button","SLEEPTP");

	TurbineBoard.AddCommand("OuvertureTotale", 0, "button", "OT");
	TurbineBoard.AddCommand("FermetureTotale", 1, "button", "FT");
	TurbineBoard.AddCommand("+1T  Moteur",2,"button","D360");
	TurbineBoard.AddCommand("-1T  Moteur",3,"button","D-360");
	TurbineBoard.AddCommand("SetMin",4,"button","SMIN");
	TurbineBoard.AddCommand("SetMax",5,"button","SMAX");

	
	localboard.AddCommand("Save data", 0,"button","SDATA");
	localboard.AddCommand("ClearData", 1 , "button", "CDATA");
	localboard.AddCommand("Save data", 2,"button","SDATA2");
}
bool saveData(void){
	DynamicJsonDocument doc(1024);
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
				Serial.println(F("Error parsing JSON "));
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
	if (c.startsWith("ModeTurbine"))
	{
		c.remove(0,12);
		Serial.println("ModeTurbine: " + String(c));
		
		switch (c.toInt())
		{
		case 0:
			modeTurbine = Manuel;
			break;
		case 1:
			modeTurbine = Auto;
			break;
		default:
			break;
		}
	}
	
		
}

void RouteHttp() {
	serverHTTP.onNotFound([](AsyncWebServerRequest* request) {
		request->send_P(404, "text/html", "404 notfound");
		});
	serverHTTP.on("/index", HTTP_GET, [](AsyncWebServerRequest* request) {
		 String S = "bonjour" + String(localAddress);
		request->send_P(200, "text/html", index_html ,processor);
	});
	serverHTTP.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
		Heltec.display->println(String(request->url()));
		if (request->hasParam("output") && request->hasParam("state")) {
			Heltec.display->print("output: " + String(request->getParam("output")->value()));
			Heltec.display->println("  state: " + String(request->getParam("state")->value()));
			Serial.println(request->getParam("state")->value().toInt());
			digitalWrite(25, request->getParam("state")->value().toInt());
			request->send(200, "text/plain", "OK");
		}
		else if (request->hasParam("b") && request->hasParam("c")) {
			
			if (request->getParam("b")->value().toInt() == localAddress)
			{
				Serial.println("Commande pour moi");
				TraitementCommande(request->getParam("c")->value());
			} else
			{
				sendMessage( request->getParam("b")->value().toInt() ,request->getParam("c")->value());
				LoRa.receive();
			}
			
			
			request->send(200, "text/plain", "J'ai recu: b=" + request->getParam("b")->value() + " et c: " + request->getParam("c")->value() );
			
		}
		
	});
	serverHTTP.on("/maj", HTTP_GET, [](AsyncWebServerRequest* request) {
		String retour = "{ \"msSystem\" :" + String(millis()) + "," +
			//"\"modeTurbine\":\"" + String(EmodeTurbinetoString(modeTurbine)) + "\"," + 
			"\"modeTurbine\": {\"name\":\" " + String(EmodeTurbinetoString(modeTurbine)) + "\", \"id\":"+ modeTurbine + "}," + 
		"\"boards\" : [" + allBoard[0]->toJson() + "," + allBoard[1]->toJson() + "," + allBoard[2]->toJson() + "]}";
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
	delay(1000);
	
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
		//Heltec.display->drawString(40, 45, String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]));
		Heltec.display->drawString(40, 45, String(WiFi.localIP().toString()));
		break;
	case 1:
		Heltec.display->drawLogBuffer(1, 1);
		break;
	case 2:
		Heltec.display->drawString(0,10,String(EmodeTurbinetoString( modeTurbine)));
		break;
	default:
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
	
	int nombre = NiveauEtang * strip.numPixels() ;
	for (size_t i = 0; i < nombre; i++)
	{
		strip.setPixelColor(i, strip.Color(0, 0, 127));
	}
	strip.show();
}
void deserializeResponse(byte board, String Response){
	
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
		NiveauEtang=doc["Niveau"];
		pidNiveauEtang = -1 * NiveauEtang;
		break;
	case TURBINE:
		if (doc.containsKey("Ouverture") && doc.containsKey("OuvMaxCodeur"))
		{
			Serial.println("J'ai bien les key du json de turbine");
			OuvertureVanne = doc["Ouverture"];
			OuvertureMaxVanne = doc["OuvMaxCodeur"];
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
	Serial.println(receivedMessage.Content);
	// if the recipient isn't this device or broadcast,
	if (receivedMessage.recipient != localAddress && receivedMessage.recipient != 0xFF)
	{
		
		return;                             // skip rest of function
	}
	

 
	switch (receivedMessage.sender)
	{
	case MASTER:
		
		break;
	case ETANG:
		allBoard[1]->lastmessage = millis();
		allBoard[1]->LastMessage = receivedMessage;
		allBoard[1]->newMessage = true;

		
		break;
	case TURBINE:
		allBoard[2]->lastmessage = millis();
		allBoard[2]->LastMessage = receivedMessage;
		allBoard[2]->newMessage = true;

		break;
	default:
		
		break;
	}
	//// if message is for this device, or broadcast, print details:
	// Serial.println("Received from: 0x" + String(receivedMessage.sender, HEX));
	// Serial.println("Sent to: 0x" + String(receivedMessage.recipient, HEX));
	// Serial.println("Message ID: " + String(incomingMsgId));
	// Serial.println("Message length: " + String(incomingLength));
	// Serial.println("Message: " + receivedMessage.Content);
	// Serial.println("RSSI: " + String(LoRa.packetRssi()));
	// //Serial.println("Snr: " + String(LoRa.packetSnr()));
	// Serial.println();
	Heltec.display->println("0x" + String(receivedMessage.sender,HEX) + " to 0x" + String(receivedMessage.recipient, HEX) + " " + String(receivedMessage.Content));
	LoRa.receive();
}

void  initWifi(void)
{
	Heltec.display->clear();
	Serial.print("Connecting to ");
	Serial.println(SSID);
	Heltec.display->drawString(0, 00, "Connecting to ");
	Heltec.display->drawString(00, 10, SSID);
	Heltec.display->display();
	WiFi.begin(SSID, PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	// Print local IP address 
	Serial.println("");
	Serial.println("WiFi connected at IP address:");
	Serial.println(WiFi.localIP());
	Heltec.display->drawString(0, 20, "WiFi connected at IP address:");
	Heltec.display->drawString(0, 30, String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]));
	WiFi.mode(WIFI_STA);
	// Start Web Server
	serverHTTP.begin();

	Heltec.display->drawString(0, 40, "server started");
	//Heltec.display->drawString(10, 12, String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]));
	Heltec.display->display();
	if (MDNS.begin(HOSTNAME)) {
		MDNS.addService("http", "tcp", 80);
		Serial.println("MDNS responder started");
		Serial.print("You can now connect to http://");
		Serial.print(HOSTNAME);
		Serial.println(".local");
		
	}
	delay(1000);
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
		delay(2000);
		return;
	}
	else
	{
		Heltec.display->drawString(0, 10, "Card Mount ok");
	}
	Heltec.display->display();
	delay(2000);

}

void printLocalTime()
{
  Serial.println(timeClient.getFormattedTime());
}

void setup() {
	Heltec.begin(true, true, true, true, BAND);
	//InitSD();
	initWifi();
	configTime(3600, 3600, ntpServer);
	InitBoard();
	Heltec.display->clear();
	if (!SPIFFS.begin(true)) {
		Serial.println("SPIFFS Mount Failed");
		Heltec.display->drawString(0,10,"SPIFFS Failed");
		return;
	} else
	{
		Heltec.display->drawString(0,10,"SPIFFS OK");
	}
	Heltec.display->display();
	pinMode(25, OUTPUT);
	pinMode(PRGButton, INPUT_PULLUP);

	
	localboard.connected = true;
	

	LoRa.onReceive(onReceive);
	LoRa.receive();

	Serial.println("Heltec.LoRa init succeeded.");
	delay(1000);
	Heltec.display->clear();
	Heltec.display->flipScreenVertically();
	Heltec.display->setLogBuffer(5, 100);

	initWebSocket();
	RouteHttp();


	strip.begin();
  	strip.setBrightness(20);
  	strip.show(); // Initialize all pixels to 'off'
	strip.setPixelColor(1, strip.Color(127, 0, 0));
   	strip.show();

	myPID.setTimeStep(5000);

	timeClient.begin();
	while(!timeClient.update()) {
    	timeClient.forceUpdate();
  	}
	
	
}

// the loop function runs over and over again until power down or reset
void loop() {
   
   AffichagePixel();
//    if (EtangBoard.lastmessage != 0)
//    {
	   
	   
	   
//    } else
//    {
// 	   myPID.stop();
//    }
   myPID.run();
   
   //printLocalTime();
   if (millis()> lastCorrectionVanne + 5000)
   {
	    //Serial.println("mesure: "+ String(NiveauEtang ) );
	    lastCorrectionVanne = millis();
		//Serial.println("Correction Vanne: "+ String(correctionVanne) );

		notifyClients();
   }
   
	if ((digitalRead(PRGButton) == LOW) && !previousEtatbutton)
	{
		previousEtatbutton = true;
		if (displayMode < 2 )
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
	displayData();

	for (size_t i = 1; i < 3; i++)
	{
		if (allBoard[i]->newMessage)
		{
			allBoard[i]->newMessage = false;
			deserializeResponse(allBoard[i]->localAddress, allBoard[i]->LastMessage.Content);
		}
		
	}
	

	//printLocalTime();
	//----------------------------------------Serveur HTTP
	
	/*WiFiClient client = serverHTTP.available();
	
	if (client) {
		if (client.connected())
		{
			String request = client.readStringUntil('\r');
			Serial.println(request);
			if (request.startsWith("GET / HTTP/1.1"))
			{
				arHtml(client, "text/html");
				envoiFichier(client, "/EtangTurbine.html");
			}
			else if (request.startsWith("GET /datalog.txt HTTP/1.1"))
			{
				arHtml(client, "text");
				envoiFichier(client, "/data/datalog.txt");
			}
			else
			{
				client.println("er");
			}


			client.stop();
		}
	}*/
	ws.cleanupClients();
	delay(20);
	
}
