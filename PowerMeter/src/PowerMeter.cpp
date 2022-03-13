#include <Arduino.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>

const byte pinCurrentSensor = 36;
const byte pinCurrentSensorSct013 = 34;
const byte pinTensionSensor = 39;


float rawCurrentValuewcs = 0;
float currentValueWcs = 0;

float rawCurrentValueSct013 = 0;
float currentValueSct013 = 0;
float tensionValue = 0;


Adafruit_SSD1306 display(128, 64, &Wire);

void mesureSysteme(void){
  rawCurrentValuewcs = (analogRead(pinCurrentSensor));//-1846)*0.032;
  currentValueWcs = (rawCurrentValuewcs -1840) * 0.032;
  rawCurrentValueSct013 = analogRead(pinCurrentSensorSct013);
  currentValueSct013 = (rawCurrentValueSct013-2970)*0.0625;
  tensionValue = analogRead(pinTensionSensor)*0.07;
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 17, 2);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.drawCircle(20,20,10,SSD1306_WHITE);
  display.display();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text

  pinMode(pinCurrentSensor,INPUT);
  pinMode(pinCurrentSensorSct013,INPUT);
  pinMode(pinTensionSensor,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  mesureSysteme();

  display.clearDisplay();
  display.setCursor(10,10);
  display.print(String(rawCurrentValueSct013) );
  display.setCursor(70,10);
  display.print(String(currentValueSct013) + " mA");
  display.setCursor(10,20);
  display.print(String(rawCurrentValuewcs) );
  display.setCursor(70,20);
  display.print(String(currentValueWcs) + " mA");
  display.setCursor(10,30);
  display.print(String(tensionValue) + " V");
  display.display();

  
  static unsigned long previousSend = 0;
  if (millis()-previousSend >5000)
  {
    previousSend = millis();
    Serial1.print("voltage=" + String(tensionValue)+";");
    Serial1.println("current=" +String(random(2))+";");
  }

  delay(200);
}