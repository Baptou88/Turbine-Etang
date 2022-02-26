#include <Arduino.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>

const byte pinCurrentSensor = 36;
const byte pinTensionSensor = 39;
float currentValue = 0;
float tensionValue = 0;


Adafruit_SSD1306 display(128, 64, &Wire);

void mesureSysteme(void){
  currentValue = (analogRead(pinCurrentSensor));//-1846)*0.032;
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
  pinMode(pinTensionSensor,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  mesureSysteme();

  display.clearDisplay();
  display.setCursor(10,10);
  display.print(String(currentValue) + " mA");
  display.setCursor(10,20);
  display.print(String(tensionValue) + " V");
  display.display();
  Serial.println(currentValue);
  
  static unsigned long previousSend = 0;
  if (millis()-previousSend >5000)

  {
    Serial1.println("voltage:" + String(tensionValue));
  }

  delay(200);
}