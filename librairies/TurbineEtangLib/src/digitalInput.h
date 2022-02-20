#include <Arduino.h>

#if !defined(_DIGITALINPUT)
#define _DIGITALINPUT


class digitalInput
{
private:
    uint8_t _pin;
    bool _state;
    bool _previousState;
    uint8_t _mode;
    int _debonceTime = 10;
    unsigned long lastDebounceTime = 0;
    unsigned long pressedSince = 0;
public:
    digitalInput(uint8_t pin,uint8_t mode);
    ~digitalInput();
    void loop();
    bool isPressed();
    bool isReleased();
    bool frontMontant();
    bool frontDesceandant();
    unsigned long pressedTime();

    bool getState();
};

/**
 * @brief retourne le temps en ms d'appui
 * 
 * @return unsigned long 
 */
unsigned long digitalInput::pressedTime(){
    if (isPressed())
    {
        return millis() -pressedSince ;
    }
    return 0;
    
}
bool digitalInput::getState(){
    return _state;
}
digitalInput::digitalInput(uint8_t pin, uint8_t mode)
{
   _pin = pin;
   _mode = mode; 
   pinMode(_pin , _mode);
}



void digitalInput::loop(){
    
    int reading  = digitalRead(_pin);
    // if (reading != _previousState) {
    //     // reset the debouncing timer
    //     lastDebounceTime = millis();
    // }
    if ((millis() - lastDebounceTime) > _debonceTime) {
        lastDebounceTime = millis();
        if (reading != _previousState && reading == LOW)
        {
           pressedSince = millis();
        }
        
        _previousState = _state;
        _state = reading;
    }
      
}
bool digitalInput::isPressed(){
    if (_mode == INPUT_PULLUP)
    {
        if (_state == LOW)
        {
            return true;
        }
        return false;
    }else //if (_mode == INPUT_PULLDOWN)
    {
        if (_state == LOW)
        {
            return false;
        }
        return true;
    }
    return _state;
}
bool digitalInput::isReleased(){
    if (_mode == INPUT_PULLUP)
    {
        if (_state == HIGH)
        {
            return true;
        }
        return false;
    }else //if (_mode == INPUT_PULLDOWN)
    {
        if (_state == HIGH)
        {
            return false;
        }
        return true;
    }
    return _state;
}
bool  digitalInput::frontDesceandant(){
    return (_previousState == HIGH && _state == LOW); 
}
bool  digitalInput::frontMontant(){
    return (_previousState == LOW && _state == HIGH); 
}
digitalInput::~digitalInput()
{
}



#endif // _DIGITALINPUT
