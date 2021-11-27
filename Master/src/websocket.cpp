#include "TurbineEtangLib.h"
#include "Heltec.h"
#include <ESPAsyncWebServer.h>
#include "MasterLib.h"
#include <LinkedList.h>

extern AsyncWebSocket ws;
extern AsyncWebServer serverHTTP;
//extern board allBoard[];

// typedef enum {
// 	Manuel,
// 	Auto
// }EmodeTurbine;
extern EmodeTurbine modeTurbine;
//extern board *allBoard[3];
extern LinkedListB::LinkedList<board*> *allBoard;
extern double OuvertureVanne;

void notifyClients() {
  //ws.textAll(String("ModeTurbine") + String(modeTurbine));
  String retour = "{ \"msSystem\" :" + String(millis()) + "," +
			//"\"modeTurbine\":\"" + String(EmodeTurbinetoString(modeTurbine)) + "\"," + 
			"\"modeTurbine\": {\"name\":\" " + String(EmodeTurbinetoString(modeTurbine)) + "\", \"id\":"+ modeTurbine + "}," + 
      "\"OuvertureVanne\": " + String(OuvertureVanne) + "," + 
		"\"boards\" : [" + allBoard->get(0)->toJson() + "," + allBoard->get(1)->toJson() + "," + allBoard->get(2)->toJson() + "]}";

  ws.textAll(retour);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.println("ws message: " + String((char*)data));
    String msg = (char*)data;
    if (strcmp((char*)data, "toggle") == 0) {
      Serial.println("toggle");
      notifyClients();
    }
    if (msg.startsWith("ModeTurbine"))
    {
      msg.remove(0,12);
      Serial.println(msg);
      switch (msg.toInt())
      {
      case 0:
        modeTurbine =  EmodeTurbine::Manuel;
        break;
      case 1:
        modeTurbine = EmodeTurbine::Auto;
      default:
        break;
      }
    }
    
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        Serial.println("WebSocket error ");
        break;
  }
}

void initWebSocket() {
    Serial.println("init websocket ");
    ws.onEvent(onWsEvent);
    serverHTTP.addHandler(&ws);
}