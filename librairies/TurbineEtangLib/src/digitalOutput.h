#include <Arduino.h>

#if !defined(_DIGITALOUTPUT)
#define _DIGITALOUTPUT

class digitalOutput
{
private:
    
    bool _state = false;
    byte _pin;
public:
    digitalOutput(byte pin);
    ~digitalOutput();
    void setState(bool state);
    bool getState();;
    void loop();
    void toggle();

};

bool digitalOutput::getState(){
    return _state;
}
void digitalOutput::setState(bool state){
    _state = state;
}

void digitalOutput::loop(){
    digitalWrite(_pin , _state);
}
void digitalOutput::toggle(){
    _state = !_state;
}

digitalOutput::digitalOutput(byte pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
}

digitalOutput::~digitalOutput()
{
}


#endif // _DIGITALOUTPUT
