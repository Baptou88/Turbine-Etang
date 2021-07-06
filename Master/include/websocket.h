#ifndef _websocket_h
#define _websocketb_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void notifyClients();

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len);

void initWebSocket();

#endif