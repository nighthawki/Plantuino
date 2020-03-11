#include "DHT.h"
#include "Time.h"
#include <Wire.h>
#include <DS1307RTC.h>
#include <Thread.h>
#include <ThreadController.h>
#include <RelayModule.h>
#include <LiquidCrystal_I2C.h>

/*
I2C device found at address 0x27  !
I2C device found at address 0x3C  !
I2C device found at address 0x50  !
I2C device found at address 0x68  !
*/
LiquidCrystal_I2C lcd(0x27,20,4);

// PIN settings
#define DHT_PIN                 A0
#define SOIL_MOISTURE_1_PIN     A1
#define SOIL_MOISTURE_2_PIN     A2
#define SOIL_MOISTURE_3_PIN     A3
#define SOIL_MOISTURE_4_PIN     A4
#define RELAY_LIGHT_PIN         11
#define RELAY_HUMIDIFIER_PIN    12
#define RELAY_FAN_PIN           10

// light on and off time
#define LIGHT_INTERVAL      600000 // 10 minute
#define LIGHT_ON            5      // 5 o'clock
#define LIGHT_OFF           23     // 23 o'clock

// temperature and humidity constants
#define HUM_TEMP_INTERVAL   5000 // 10 seconds
#define TEMP_LOW            19
#define TEMP_HIGH           28
#define HUM_LOW             50
#define HUM_HIGH            90

// soil moisture values
#define SOIL_MOIS_INTERVAL  2000 // 10 minuten
#define SM_1_LOW            10
#define SM_1_HIGH           90
#define SM_2_LOW            10
#define SM_2_HIGH           90
#define SM_3_LOW            10
#define SM_3_HIGH           90
#define SM_4_LOW            10
#define SM_4_HIGH           90

// Temperature and Humidity Sensor
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE, 6);

ThreadController controll = ThreadController();
Thread lightControllThread = Thread();
Thread humidControllThread = Thread();
Thread soilMoistureThread = Thread();

RelayModule* relay_light = NULL;
RelayModule* relay_humidifier = NULL;
RelayModule* relay_fan = NULL;

void setup() {
  Serial.begin(9600);
  while (!Serial) ;

  lightControllThread.onRun(lightCallback);
  lightControllThread.setInterval(LIGHT_INTERVAL);
  humidControllThread.onRun(controllTempAndHum);
  humidControllThread.setInterval(HUM_TEMP_INTERVAL);
  soilMoistureThread.onRun(controllSoilMoisture);
  soilMoistureThread.setInterval(SOIL_MOIS_INTERVAL);

  controll.add(&lightControllThread);
  controll.add(&humidControllThread);
  controll.add(&soilMoistureThread);

  relay_light = new RelayModule(RELAY_LIGHT_PIN);
  relay_humidifier = new RelayModule(RELAY_HUMIDIFIER_PIN);
  relay_fan = new RelayModule(RELAY_FAN_PIN);

  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void loop() {  
  controll.run();
 }

/*
 * This function gets periodically called by a Thread.
 * Reads the Real Time Clock if possible.
 */
void lightCallback() {  
  tmElements_t tm;

  if (RTC.read(tm)) {
    controllLight(tm.Hour);
  } else {
    if (RTC.chipPresent()) {
      lcd.setCursor(4, 3);
      lcd.print("Time not set");
    } else {
      lcd.setCursor(1, 3);
      lcd.print("Clock Module Error");
    }
  }
}

void controllSoilMoisture(){
  int value[4];
  int percent[4];
  value[0] = analogRead(SOIL_MOISTURE_1_PIN);
  value[1] = analogRead(SOIL_MOISTURE_2_PIN);
  value[2] = analogRead(SOIL_MOISTURE_3_PIN);
  value[3] = analogRead(SOIL_MOISTURE_4_PIN);

  for (int i = 0; i < 4; i++){
    percent[i] = map(value[i], 830, 430, 0, 100);
    if (percent[i] > 100) {
      percent[i] = 100;
    } else if (percent[i] < 0) {
      percent[i] = 0;
    }

    lcd.setCursor(12, i);
    lcd.print("M");
    lcd.print(i);
    lcd.print(" ");
    lcd.print(percent[i]);
    lcd.print("%");
  }
}

void controllTempAndHum() {
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(hum) || isnan(temp)) {
    lcd.setCursor(1, 3);
    lcd.print("Temp Module Error");
  } else {
    displayTempAndHum(temp, hum);
    controllTemperature(temp);
    controllHumidity(hum);

  }
}

/*
 * Light-Controll
 * When hour is between LIGHT_ON and LIGHT_OFF
 * the Relay will be triggered
 */
void controllLight(float hour){
  if (hour >= LIGHT_ON && hour < LIGHT_OFF) {
      turnRelayOn(relay_light);
    } else {
      turnRelayOff(relay_light);
    }
}

/*
 * Humidity-Controll
 * When humidity falls below HUM_LOW
 * the Relay will be triggered and it 
 * stops when humidity exceeds HUM_HIGH
 */
void controllHumidity(float hum){
  if (hum < HUM_LOW) {
    turnRelayOn(relay_humidifier);
  } else if (hum >= HUM_HIGH) {
    turnRelayOff(relay_humidifier);
  }
}

/*
 * Temperature-Controll
 * When temperature exceeds TEMP_High
 * the Relay will be triggered and it
 * stops when temperature falls below TEMP_LOW
 */
void controllTemperature(float temp){
  if (temp > TEMP_HIGH) {
    turnRelayOn(relay_fan);
  } else if (temp <= TEMP_LOW) {
    turnRelayOff(relay_fan);
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

/*
 * Display Temparature and Humidity on LCD Screen in one line
 * fits perfectly in a 20 characters LCD display
 */
void displayTempAndHum(float temp, float hum) {
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  lcd.print(temp);

  lcd.setCursor(0, 1);
  lcd.print("H: ");
  lcd.print(hum);
}
