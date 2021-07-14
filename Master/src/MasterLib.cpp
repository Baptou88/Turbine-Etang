#include "MasterLib.h"

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