#ifndef _TIMER
#define _TIMER

#include "Arduino.h"

class Timer
{
private:
    unsigned long _previousMillis = 0;
    long _interval = 0;
    
public:
    Timer(long interval);
    ~Timer();
    void setInterval(long interval);
    boolean isPassed(bool reset );
    void reset();
};





#endif