#include "TurbineEtangLib.h"
#include "Heltec.h"
#include <ESPAsyncWebServer.h>

extern AsyncWebSocket ws;
extern AsyncWebServer serverHTTP;


void notifyClients() {
  ws.textAll(String("ledstate"));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.println("ws message: " + String((char*)data));
    if (strcmp((char*)data, "toggle") == 0) {
      Serial.println("toggle");
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
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
    ws.onEvent(onEvent);
    serverHTTP.addHandler(&ws);
}