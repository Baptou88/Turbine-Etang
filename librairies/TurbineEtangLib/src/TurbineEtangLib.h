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

enum ELora
{
	Master = 0xA,
	Etang = 0xC,
	Turbine = 0xB
};
void sendMessage(byte destination, String outgoing);
#endif

