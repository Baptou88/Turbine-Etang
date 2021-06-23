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
//#include "HttpServer.h"
#include "TurbineEtangLib.h"
#include <FS.h>
#include <ArduinoJson.h>


#define BAND 868E6
#define PRGButton 0

bool previousEtatbutton = false;
SPIClass spi1;

// Set web server port number to 80
//WiFiServer serverHTTP(80);	//80 est le port standard du protocole HTTP
AsyncWebServer serverHTTP(80);	//80 est le port standard du protocole HTTP

// Definition de parametre Wifi

const char* SSID = "Livebox-44C1";
const char* PASSWORD = "20AAF66FCE1928F64292F3E28E";

//const char* SSID = "Honor 10";
//const char* PASSWORD = "97540708";
const char* HOSTNAME = "ESP32LoRa";

byte localAddress = 0x0A;
long msgCount = 0;
byte displayMode = 0;

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
					retour += "<input type=\""+ allBoard[i]->Commands[j].Type + "\" name=\""+ allBoard[i]->Commands[j].Name + "\" value=\"" + allBoard[i]->Commands[j].Name + "\" onclick=\"update(this,"+ "'"+allBoard[i]->Commands[j].Action +"')\">\n";
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

	}
	return String();
	return String();
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

	localboard.AddCommand("VextOff",0,"button","VEXTOFF");
	localboard.AddCommand("VextOn",1,"button","VEXTON");
	localboard.AddCommand("Save data", 2,"button","SDATA");
}
void TraitementCommande(String c){
	if (c == "VEXTON")
	{	
		Serial.println("VEXTON");
		Heltec.VextON();
	}
	if (c == "VEXTOFF")
	{
		Serial.println("VEXTOFF");
		Heltec.VextOFF();
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
			}
			
			
			request->send(200, "text/plain", "J'ai recu: b=" + request->getParam("b")->value() + " et c: " + request->getParam("c")->value() );
			
		}
		
	});
	serverHTTP.on("/maj", HTTP_GET, [](AsyncWebServerRequest* request) {
		String retour = "{ \"msSystem\" :" + String(millis()) + "," +
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
	default:
		break;
	}
	
	Heltec.display->display();

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
		
		break;
	case TURBINE:
		allBoard[2]->lastmessage = millis();
		allBoard[2]->LastMessage = receivedMessage;
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
void setup() {
	Heltec.begin(true, true, true, true, BAND);
	//InitSD();
	initWifi();
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
	RouteHttp();
}

// the loop function runs over and over again until power down or reset
void loop() {
   
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
	delay(200);
}
