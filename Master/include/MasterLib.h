#ifndef _MasterLib_h
#define _MasterLib_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TurbineEtangLib.h"
#include "Heltec.h"
#include "LinkedList.h" 

typedef enum {
	Manuel,
	Auto
}EmodeTurbine;

String EmodeTurbinetoString(size_t m);
enum commandType{
	button,
	textbox
};
class Command
{
public:
	String Name;
	String Type;
	String Action;
	String Value;

private:

};
class board
{
public:
	LinkedListB::LinkedList<Command> *Commands ;
	board(String N, byte Address) {
		Commands = new LinkedListB::LinkedList<Command>();
		Name = N;
		localAddress = Address;
		
	}
	String Name;
	byte localAddress;
	String msgToSend = "";

	/**
	 * @brief indique le millis du dernier message recu
	 * 
	 */
	unsigned long lastmessage = 0;

	/**
	 * @brief stock le dernier message recu
	 * 
	 */
	Message LastMessage;

	/**
	 * @brief retourne les infos sous forme de json
	 * 
	 * @return String 
	 */
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

	/**
	 * @brief indique si un nouveau message est recu
	 * 
	 */
	bool newMessage = false;
	int attempt = 0;
	bool waitforResponse = false;
	unsigned long lastDemandeStatut = 0;
	

	
	void AddCommand(String Name,  String Type, String Action, String Value ="") {
		Command temp = Command();
		temp.Action = Action;
		temp.Name = Name;
		temp.Type = Type;
		temp.Value = Value;
		Commands->add(temp);
		
	}

	bool isConnected(){
		if ((millis() - lastmessage < 600000) && lastmessage !=0 )
		{
			return true;
		} else
		{
			return false;
		}
		
		
	}
	void sendMessage(byte destination, String outgoing, byte sender)
	{
		LoRa.beginPacket();                   // start packet
		LoRa.write(destination);              // add destination address
		LoRa.write(sender);             // add sender address
		LoRa.write(msgCount);                 // add message ID
		LoRa.write(outgoing.length());        // add payload length
		LoRa.print(outgoing);                 // add payload
		LoRa.endPacket();                     // finish packet and send it
		msgCount++;                           // increment message ID
	}

	void demandeStatut (){
		if (millis()- lastDemandeStatut > 30000)
		{
			Serial.println("Classe demandestatut 0x" + String(localAddress,HEX));
			lastDemandeStatut = millis();
			sendMessage(localAddress, "DemandeStatut",MASTER);
			LoRa.receive();
			waitforResponse = true;
		}
	}

private:
	int msgCount=0;
};

struct wifiparamconnexion{
  const char* SSID;
  const char* Credential;
  bool selected ;
};


#endif
