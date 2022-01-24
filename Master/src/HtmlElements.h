#include <Arduino.h>

 class HtmlElements
{
private:
    /* data */
public:
    HtmlElements(String Name,String Type,String Action);
    ~HtmlElements();
    String Render();
};

HtmlElements::HtmlElements(String Name,String Type,String Action)
{
}

HtmlElements::~HtmlElements()
{
}
class HtmlButton : public HtmlElements
{
private:
    /* data */
public:
    HtmlButton();
    ~HtmlButton();
};


