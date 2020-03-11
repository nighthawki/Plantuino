#include "DHT.h"
#include "Time.h"
#include <Wire.h>
#include <DS1307RTC.h>
#include <Thread.h>
#include <ThreadController.h>
#include <RelayModule.h>

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

/*
I2C device found at address 0x27  !
I2C device found at address 0x3C  !
I2C device found at address 0x50  !
I2C device found at address 0x68  !
*/

// PIN settings
#define DHT_PIN A0
#define SOIL_MOISTURE_PIN_1 A1
#define SOIL_MOISTURE_PIN_2 A2
#define SOIL_MOISTURE_PIN_3 A3
#define SOIL_MOISTURE_PIN_4 A4

// light on and off time
#define LIGHT_ON    5
#define LIGHT_OFF   23

// temperature low and high values
#define TEMP_LOW    19
#define TEMP_HIGH   21

// humidity low and high values
#define HUM_LOW     33
#define HUM_HIGH    65

// Temperature and Humidity Sensor
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE, 6);

ThreadController controll = ThreadController();
Thread lightControllThread = Thread();
Thread humidControllThread = Thread();
//Thread displayControllThread = Thread();

RelayModule* relay_1_1 = NULL;
RelayModule* relay_1_2 = NULL;
RelayModule* relay_2 = NULL;

void setup() {
  Serial.begin(9600);
  while (!Serial) ;

  lightControllThread.onRun(lightCallback);
  lightControllThread.setInterval(60000);
  humidControllThread.onRun(controllDHT);
  humidControllThread.setInterval(10000);

  controll.add(&lightControllThread);
  controll.add(&humidControllThread);
  //controll.add(&displayControllThread);

  relay_1_1 = new RelayModule(11); // Time-Controll Light
  relay_1_2 = new RelayModule(12); // Humidity-Controll
  relay_2 = new RelayModule(10); // Fan-Controll Zuluft

  dht.begin();

  lcd.backlight();
  lcd.setCursor(3,0);
  lcd.print("Welcome!");
}

void loop() {  
  controll.run();
 }

void lightCallback() {  
  Serial.println("Light Callback");
  tmElements_t tm;

  if (RTC.read(tm)) {
    controllLight(tm.Hour);
  } else {
    if (RTC.chipPresent()) {
      Serial.print("Time not set");
    } else {
      Serial.print("Something went wrong with the clock.");
    }
  }
}

void controllDHT() {
  //int soilMoistureValue = analogRead(A1);
  //int soilmoisturepercent = map(soilMoistureValue, 830, 500, 0, 100);

  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(hum) || isnan(temp)) {
    Serial.print("Failed to read");
  } else {
    char temp_buff[5];
    char temp_disp_buff[12] = "\nTmp:";
    dtostrf(temp,2,1,temp_buff);
    strcat(temp_disp_buff,temp_buff);

    controllFan(temp);
    controllHumidity(hum);
    
    char hum_buff[5];
    char hum_disp_buff[12] = "\nHum:";
    dtostrf(hum,2,1,hum_buff);
    strcat(hum_disp_buff,hum_buff);

    Serial.write(temp_disp_buff, 12);
    Serial.write(hum_disp_buff, 12);
  }
}

void controllLight(float hour){
  if (hour >= LIGHT_ON && hour < LIGHT_OFF) {
      turnRelayOn(relay_1_1);
    } else {
      turnRelayOff(relay_1_1);
    }
}

void controllHumidity(float hum){
  if (hum < HUM_LOW) {
    turnRelayOn(relay_1_2);
  } else if (hum >= HUM_HIGH) {
    turnRelayOff(relay_1_2);
  }
}

void controllFan(float temp){
  if (temp > TEMP_HIGH) {
    turnRelayOn(relay_2);
  } else if (temp <= TEMP_LOW) {
    turnRelayOff(relay_2);
  }
}

void turnRelayOn(RelayModule *relay){
  if (relay->isOff()){
    relay->on();
  }
}

void turnRelayOff(RelayModule *relay){
  if (relay->isOn()) {
    relay->off();
  }
}
