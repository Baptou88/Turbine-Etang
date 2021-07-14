#ifndef _MasterLib_h
#define _MasterLib_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TurbineEtangLib.h"

typedef enum {
	Manuel,
	Auto
}EmodeTurbine;

String EmodeTurbinetoString(size_t m);

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




#endif
