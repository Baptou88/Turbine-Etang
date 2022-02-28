#include <Arduino.h>

 class HtmlElements
{
private:
    /* data */
public:
    HtmlElements();
    ~HtmlElements();
    String Render();
};

HtmlElements::HtmlElements()
{
}

HtmlElements::~HtmlElements()
{
}
class HtmlButton : public HtmlElements
{
private:
    String _Name;
    String _Action;
    String _Value;
public:
    HtmlButton(String Name,String Action, String Value){
        _Name = Name;
        _Action = Action;
        _Value = Value;
    }
    ~HtmlButton();
    String Render(void){
        return "<input class=\"btn btn-outline-dark m-1\" type=\"button\" name=\""+ _Name + "\" value=\"" + _Value + "\" onclick=\"update(this,"+ "'"+ _Action +"')\">\n";;
    }
};

class HtmlRange
{
private:
    int _minValue = 0;
    int _maxValue = 100;
    int _Value=0;
    int _step = 1;
    String _Action;

public:
    HtmlRange(String Name,String Action,int Value,int minValue = 0,int maxValue = 100, int step = 1){
        _minValue = minValue;
        _maxValue = maxValue;
        _Value = Value;
        _step = step;
    }
    ~HtmlRange();
    String Render(){
        return "<input type=\"range\" onchange=\"update(this,'" + _Action +"'+'=' +this.value)\" id=\"setPointSlider\" min=\"0\" max=\"100\" value=\""+ String(_Value) +"\" step=\"1\" class=\"form-range\">\n";
    }
};





