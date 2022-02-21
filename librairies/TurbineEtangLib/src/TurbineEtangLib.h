/*
 Name:		TurbineEtangLib.h
 Created:	21/05/2021 19:15:41
 Author:	Baptou
 Editor:	http://www.visualmicro.com
*/

#ifndef _TurbineEtangLib_h
#define _TurbineEtangLib_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define MASTER 0xA
#define TURBINE 0xB
#define ETANG 0xC
#define RADIATEUR 0xD

#define DEFAULTSF 7
enum ELora
{
	Master = 0xA,
	Turbine = 0xB,
	Etang = 0xC,
	Radiateur = 0xD
};
void sendMessage(byte destination, String outgoing);

void sendMessageConfirmation(int msgNumber, byte destination = MASTER);
class Message
{
public:
	
	byte recipient;
	byte sender;
	int msgID;
	String Content;
	int length() {
		return this->Content.length();
	}
	
	String toJson() {
		String retour = "{ \"recipient\" : \"" + String(recipient) + "\"," +
			"\"sender\" : \"" + String(sender) + "\"," +
			"\"msgId\" : \"" + String(msgID) + "\"," ;
		if (Content == "")
		{
			retour += "\"content\" : \"\"" ;
		} else
		{
			retour += "\"content\" : " + String(Content) + "";
		}
		retour += "}";
		
		return retour;
	}

//private:

};


String string_wakeup_reason(esp_sleep_wakeup_cause_t wakeup_reason);
String wakeup_touchpad(touch_pad_t touchPin);
#endif

