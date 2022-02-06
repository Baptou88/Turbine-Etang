#include <Arduino.h>
#include <TurbineEtangLib.h>

#if !defined(_PROGRAMMATEDTASK)
#define _PROGRAMMATEDTASK

class ProgrammatedTask
{
private:
    bool _active = false;
    
    
public:
    ProgrammatedTask(byte heures,byte minutes, String Name);
    ~ProgrammatedTask();
    void activate();
    void deactivate();
    void execute();
    bool isActive();
    String getHours();
    String getMinutes();
    String name;
    byte h;
    byte m;
    int targetVanne = 0;
    double deepsleep = 0;
};
bool ProgrammatedTask::isActive(){
    return _active;
}
String  ProgrammatedTask::getHours(){
    return h < 10 ? "0" + String(h) : String(h);
}
String  ProgrammatedTask::getMinutes(){
    return m < 10 ? "0" + String(m) : String(m);
}
void ProgrammatedTask::execute(){
    Serial.println("Tache executé");
}
ProgrammatedTask::ProgrammatedTask(byte heures,byte minutes, String Name)
{
    name=Name;
    h = heures;
    m = minutes;
}

ProgrammatedTask::~ProgrammatedTask()
{
}

void ProgrammatedTask::activate(){
    _active = true;
}
void ProgrammatedTask::deactivate(){
    _active = false;
}


#endif // _PROGRAMMATEDTASK
