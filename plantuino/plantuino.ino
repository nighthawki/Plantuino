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
#define DHT_PIN A0
#define SOIL_MOISTURE_PIN_1 A1
#define SOIL_MOISTURE_PIN_2 A2
#define SOIL_MOISTURE_PIN_3 A3
#define SOIL_MOISTURE_PIN_4 A4

// light on and off time
#define LIGHT_INTERVAL      60000 // one minute
#define LIGHT_ON            5
#define LIGHT_OFF           23

// temperature and humidity constants
#define HUM_TEMP_INTERVAL   10000 // 10 seconds
#define TEMP_LOW            19
#define TEMP_HIGH           21
#define HUM_LOW             33
#define HUM_HIGH            65

// Temperature and Humidity Sensor
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE, 6);

ThreadController controll = ThreadController();
Thread lightControllThread = Thread();
Thread humidControllThread = Thread();

RelayModule* relay_1_1 = NULL;
RelayModule* relay_1_2 = NULL;
RelayModule* relay_2 = NULL;

void setup() {
  Serial.begin(9600);
  while (!Serial) ;

  lightControllThread.onRun(lightCallback);
  lightControllThread.setInterval(LIGHT_INTERVAL);
  humidControllThread.onRun(controllTempAndHum);
  humidControllThread.setInterval(HUMID_INTERVAL);

  controll.add(&lightControllThread);
  controll.add(&humidControllThread);

  relay_1_1 = new RelayModule(11); // Time-Controll Light
  relay_1_2 = new RelayModule(12); // Humidity-Controll
  relay_2 = new RelayModule(10); // Fan-Controll Zuluft

  dht.begin();

  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Welcome!");
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

void controllTempAndHum() {
  //int soilMoistureValue = analogRead(A1);
  //int soilmoisturepercent = map(soilMoistureValue, 830, 500, 0, 100);

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
      turnRelayOn(relay_1_1);
    } else {
      turnRelayOff(relay_1_1);
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
    turnRelayOn(relay_1_2);
  } else if (hum >= HUM_HIGH) {
    turnRelayOff(relay_1_2);
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

/*
 * Display Temparature and Humidity on LCD Screen in one line
 * fits perfectly in a 20 characters LCD display
 */
void displayTempAndHum(float temp, float hum) {
  char temp_buff[5], hum_buff[5];
  dtostrf(temp, 2, 1, temp_buff);
  dtostrf(hum, 2, 1, hum_buff);

  char lcd_buff[20] = "Tmp: ";
  strcat(lcd_buff, temp_buff);
  strcat(lcd_buff, "°");
  strcat(lcd_buff, "Hum: ");
  strcat(lcd_buff, hum_buff);
  strcat(lcd_buff, "%");

  lcd.setCursor(0, 1);
  lcd.write(lcd_buff, 20);
}
