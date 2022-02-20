#include <Arduino.h>

class Moteur
{
private:
    byte _pinFermeture = 0;
    byte _pinOuverture = 0;
    int _speed = 0;
    int maxAxel = 0;
    int minVitesse = 0;
    int speed = 0;
    int previousPWMMoteur = 0;
public:
    Moteur(byte pinOuverture, byte pinFermeture);
    ~Moteur();
    void setSpeed(int speed);
    void loop();
    void setVitesseMin(int VitesseMin);
    long getPWMMoteur();
    
};

Moteur::Moteur(byte pinOuverture, byte pinFermeture)
{
    _pinOuverture = pinOuverture;
    _pinFermeture = pinFermeture;
    pinMode(pinOuverture,OUTPUT);
    pinMode(pinFermeture,OUTPUT);

    ledcSetup(1, 5000, 8);
    ledcSetup(2, 5000, 8); 
}

Moteur::~Moteur()
{
}
void Moteur::loop(){
    
    long PWMMoteur = this->getPWMMoteur();
    if (PWMMoteur>0)
   {
     ledcWrite(1,PWMMoteur);
     ledcWrite(2,0);
   }  else if (PWMMoteur < 0)
   {
     ledcWrite(1,0);
     ledcWrite(2,abs(PWMMoteur));
   } else
   
   {
     ledcWrite(1,0);
     ledcWrite(2,0);
   }
}

long Moteur::getPWMMoteur(){
    long PWMMoteur = speed;

    if (PWMMoteur > 255)  PWMMoteur = 255;
    
    if (PWMMoteur < -255)  PWMMoteur = -255;

    if (PWMMoteur - previousPWMMoteur > maxAxel)
    {
        PWMMoteur = previousPWMMoteur + maxAxel;
        
    } else if (PWMMoteur -previousPWMMoteur <-maxAxel)
    {
        
        PWMMoteur = previousPWMMoteur - maxAxel;
    }
    previousPWMMoteur = PWMMoteur;

    if (PWMMoteur >0 && PWMMoteur <minVitesse) PWMMoteur = 0;

    if (PWMMoteur < 0 && PWMMoteur > -minVitesse) PWMMoteur = -0;

    return PWMMoteur;
}
