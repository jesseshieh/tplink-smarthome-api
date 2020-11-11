#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>

//LiquidCrystal_I2C lcd(0x27,16,2); // set the LCD address to 0x27 for a 16 chars and 2 line display


// nest 76 is  81.85 here

#define TEMP_PIN A0
// D4 is used by built-in LED?
#define BUTTON_PIN D1

#define THRESHOLD_TEMP 75
#define SERVER_IP "TODO"
#define SERVER_PORT 7777

#ifndef STASSID
#define STASSID "TODO"
#define STAPSK  "TODO"
#endif

volatile bool enabled = true;
volatile byte interruptCounter = 0;

void setupLcd() {
//  lcd.init(); // initialize the lcd
//  lcd.init(); // why twice?
//  lcd.backlight();
}

void setupWifi() {
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("local address: ");
  Serial.println(WiFi.localIP());
}

ICACHE_RAM_ATTR void handleButton() {
  enabled = !enabled;
  interruptCounter++;
}

void setup()
{
  Serial.begin(9600);
  setupWifi();
//  setupLcd();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButton, RISING);
}

void setPowerState(boolean state) {
  if (state) {
    sendCommand("true");
  } else {
    sendCommand("false");
  }
}

void logTemp(double temp) {
  sendCommand((String)temp);
}

void sendCommand(String msg) {
  if ((WiFi.status() == WL_CONNECTED)) {
    WiFiClient client;
    if (client.connect(SERVER_IP, SERVER_PORT)) {
      client.print(msg + "\n");       

      while (client.connected() || client.available())
      {
        if (client.available())
        {
          String line = client.readStringUntil('\n');
          Serial.println("response: " + line);
        }
      }
      client.stop();
    } else {
      Serial.println("error: failed to connect");
    }
  } else {
    Serial.println("error: wifi not connected");
  }
}

int getPowerState() {
  int result = -1;
  if ((WiFi.status() == WL_CONNECTED)) {
    WiFiClient client;
    if (client.connect(SERVER_IP, SERVER_PORT)) {
      client.print("state\n");       
      while (client.connected() || client.available())
      {
        if (client.available())
        {
          String line = client.readStringUntil('\n');
          if (line == "1") {
            result = 1;
          } else if (line == "0") {
            result = 0;
          }
          Serial.println("response: " + result);
        }
      }
      client.stop();
    } else {
      Serial.println("error: failed to connect");
    }
  } else {
    Serial.println("error: wifi not connected");
  }
  return result;
}

void updateLcd(float tempF) {
//  lcd.setCursor(0, 0);
//  lcd.print("Temp         F  ");
//  lcd.setCursor(6, 0);
//  lcd.print(tempF);  
}

void printDate() {

}

int filteredAnalogRead(int pin) {
  int val = 0;
  for(int i = 0; i < 20; i++) {
    val += analogRead(pin);
    delay(1);
  }

  val = val / 20;
  return val;
}

float currentTemp(int tempPin) {
  int tempReading = filteredAnalogRead(tempPin);
  double tempK = log(10000.0 * ((1024.0 / tempReading - 1)));
  tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );       //  Temp Kelvin
  float tempC = tempK - 273.15;            // Convert Kelvin to Celcius
  float tempF = (tempC * 9.0)/ 5.0 + 32.0; // Convert Celcius to Fahrenheit
  return tempF;
}

void loop()
{ 
  int offset = -4; // feels hacky, but I guess this is how calibration is done?
  double tempF = currentTemp(TEMP_PIN) + offset;
  Serial.println((String)"tempF: " + tempF);
  logTemp(tempF);
  // updateLcd(tempF);

  Serial.println("enabled: " + (String)enabled);
  Serial.println("interruptCounter: " + (String)interruptCounter);
  if (enabled) {
    int state = getPowerState();
  
    if (tempF < THRESHOLD_TEMP) {
      if (state == 0) {
        setPowerState(true); 
      } else {
        Serial.println("power already on");
      }
    } else {
      if (state == 1) {
        setPowerState(false);
      } else {
        Serial.println("power already off");
      }
    }
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    
    Serial.println("press button to enable");
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  

  delay(1000*60);
  
}
