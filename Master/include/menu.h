#include <arduino.h>
#include <digitalInput.h>

#if !defined(_MENU)
#define _MENU

class menu
{
private:
  /* data */
public:
/**
 * @brief Construct a new menu object
 * 
 * @param maxI 
 * @param maxR 
 */
  menu(int maxI,int maxR,digitalInput* buttonRight);
  ~menu();
  void onRender(void(*callback)(int));
  void onSelect(void(*callback)(int));
  void onNext();
  int getFirst();
  int getLast();
  int selectNext();
  int selectPrevious();

  void loop();
  
  int maxItems = 0;
  int maxRow =0;
  int select = 0;
  int first=0;
  int last =0;
  digitalInput *btnRight;
};

/**
 * @brief Construct a new menu::menu object
 * 
 * @param maxI maximum d'elements
 * @param maxR maximum d'elements pour le rendu
 * @param buttonRight bontou pour selectionner l'elements suivant
 */
menu::menu(int maxI,int maxR,digitalInput* buttonRight = NULL)
{
  maxItems = maxI;
  maxRow = maxR;
  last = maxR;
  Serial.println("maxI: " + String(maxItems));
  Serial.println("maxr: " + String(maxRow));
  if (buttonRight != NULL)
  {
    btnRight = buttonRight;
  }
  
}

menu::~menu()
{
}

void menu::loop(){
  if (btnRight != NULL)
  {
    if(btnRight->frontDesceandant()){
      selectNext();
    }
  }
  
}
int menu::getFirst(){
  return first;
}
int menu::getLast(){
  return last;
}
int menu::selectNext(){
  select++;
  if (first + select > maxRow-1)
  {
    first++;
    last++;
    
  }
  
  if (select > maxItems-1)
  {
    first=0;
    last=maxRow;
    select=0;
    return 0;
  }
  return select;
  
}
int menu::selectPrevious(){
 
  if (first ==select)
  {
    first--;
    last--;
    
  }
   select--;
  
  if (select <0)
  {
    first=0;
    last=maxRow;
    select=0;
    return 0;
  }
  return select;
  
}

#endif //_MENU
