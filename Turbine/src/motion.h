#if !defined(_MOTION)
#define _MOTION

extern int incCodeuse;
extern float tourMoteurVanne;
extern double posMoteur;
extern long ouvertureMax;
extern double Setpoint;
/**
 * @brief conversion degrés en incrément
 * 
 * @param degres 
 * @return int 
 */
int degToInc(int degres) {
	return ((degres * incCodeuse) / 360);
}
/**
 * @brief conversion degrés vanne en increment
 * 
 * @param degVanne 
 * @return int 
 */
int degvanneToInc(int degVanne){
	
	float incMoteur = degToInc(degVanne);
	
	
	return incMoteur / tourMoteurVanne;
}

/**
 * @brief pourcentage ouverture vanne
 * 
 * @return float 
 */
float pPosMoteur(){
	return  float(posMoteur) / float(ouvertureMax);
}
/**
 * @brief pourcentage cible vanne
 * 
 * @return float 
 */
float pSetpoint(){
	return  float(Setpoint) / float(ouvertureMax);
}


#endif // _MOTION
