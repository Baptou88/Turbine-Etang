#include <Arduino.h>
#include <TurbineEtangLib.h>

#if !defined(_PROGRAMMATEDTASK)
#define _PROGRAMMATEDTASK

class ProgrammatedTask
{
private:
    bool _active = false;
    int targetVanne = 0;
public:
    ProgrammatedTask(int execTime);
    ~ProgrammatedTask();
    bool activate();
    bool deactivate();
    void execute();

};
void ProgrammatedTask::execute(){
    Serial.println("Tache executé");
}
ProgrammatedTask::ProgrammatedTask(int execTime)
{
}

ProgrammatedTask::~ProgrammatedTask()
{
}




#endif // _PROGRAMMATEDTASK
