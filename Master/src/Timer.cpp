#include "Timer.h"

Timer::Timer(long interval)
{
    _interval = interval;
}

Timer::~Timer()
{
}

void Timer::setInterval(long interval){
    _interval = interval;
}

boolean Timer::isPassed(bool reset = true){
    unsigned long currentMillis = millis();
    
    
    if (currentMillis - _previousMillis >= _interval){
        if (reset)
        {
            _previousMillis = currentMillis;
        }
        return true;
    } else
    {
        return false;
    }
    
        
}
void Timer::reset(){
    _previousMillis = millis();
}