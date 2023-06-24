/*
* Vzorovy kod pro online teplomer
* MeteoMini s ESP32-C3, SHT40 - cidlo teploty a vlhkosti 
*
* Knihovny: 
* https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json
* https://github.com/adafruit/Adafruit_SHT4X
* https://github.com/madhephaestus/ESP32AnalogRead
*
* Dokumentace:
* https://wiki.tmep.cz/doku.php?id=zarizeni:vlastni_hardware
*
* Vytvoril chiptron.cz (2022)
*/

#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP32AnalogRead.h> 
#include "Adafruit_SHT4x.h"
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(10); //pin 2 utilizado para o pino DQ
DallasTemperature sensors(&oneWire);

ESP32AnalogRead adcB;
#define ADCBpin 0
#define bDeviderRatio 1.82274 //1.7693877551  // Delic napeti z akumulatoru 1MOhm + 1.3MOhm

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

/*----------------------------------*/
/* VYPLNTE NAZEV WIFI SITE A HESLO  */
const char* ssid = "kuzbici";
const char* password = "kubusA113Rd";
/* VASE DOMENA CIDLA NA TMEP.CZ */
String serverName = "http://dn92u9-6mcn4v.tmep.cz/index.php?"; // misto Domena vlozte vasi domenu cidla
/*----------------------------------*/

void setup() {
  sensors.begin();
  Wire.begin(19,18); // I2C sbernicce pro cidlo teploty a vlhkosti
  int pokus = 0; // promena pro pocitani pokusu o pripojeni
    Serial.begin(115200); // vystup do konzole
  delay(100); 
  
  /* NASTAVENI MERENI NAPETI AKUMULATORU  */
  adcB.attach(ADCBpin);
  /*--------------------------------------*/

  /*  NASTAVENI CIDLA TEPLOTY A VLHKOSTI  */
  sht4.begin(); // inicializace
  sht4.setPrecision(SHT4X_HIGH_PRECISION); // vysoka presnost mreni
  sht4.setHeater(SHT4X_NO_HEATER); // bez interniho ohrivace
  /*--------------------------------------*/

  /* PRIPOJENI K WIFI SITI  */
  WiFi.begin(ssid, password);
  Serial.println("Pripojovani");
  while(WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
    if(pokus > 20) // Pokud se behem 10s nepripoji, uspi se na 300s = 5min
    {
      esp_sleep_enable_timer_wakeup(30 * 1000000);
      esp_deep_sleep_start();
    }
    pokus++;
  }

  Serial.println();
  Serial.print("Pripojeno do site, IP adresa zarizeni: ");
  Serial.println(WiFi.localIP());
  /*------------------------*/
}

void loop() {
  
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  Serial.print("DS: ");  Serial.print(tempC); Serial.println(" stC");
  
  /* MERENI NAPETI AKUMULATORU */
  float bat_voltage = adcB.readVoltage() * bDeviderRatio;
  Serial.print("Napeti akumulatoru = " );
  Serial.print(bat_voltage);
  Serial.println(" V");
  /*--------------------------*/

  /* CIDLO TEPLOTY A VLHKOSTI */
  sensors_event_t hum, temp;
  sht4.getEvent(&hum, &temp); // cekani na aktualni data
  Serial.print("Teplota: "); Serial.print(temp.temperature); Serial.println(" stC");
  Serial.print("Vlhkost: "); Serial.print(hum.relative_humidity); Serial.println("% rH");
  /*--------------------------*/

  /* ODESLANI DATA NA TMEP.cz */
  if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      //GUID pro teplotu "teplota", pro vlhkost "vlhkost", pro vypocet % kapacity akumulatoru "v" (prepocet na %)
      //String serverPath = serverName + "teplota=" + temp.temperature + "&vlhkost=" + hum.relative_humidity + "&v=" + bat_voltage; 
      String serverPath = serverName + "teplota=" + temp.temperature + "&vlhkost=" + hum.relative_humidity + "&v=" + bat_voltage + "&DS= " + tempC; 
      
      // zacatek http spojeni
      http.begin(serverPath.c_str());
      
      // http get request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Uvolneni
      http.end();
    }
    else {
      Serial.println("Wi-Fi odpojeno");
    }
    /*----------------------------*/

  /* USPANI ZARIZENI  */
  esp_sleep_enable_timer_wakeup(300 * 1000000); // Zarizeni se uspi na 5min
  esp_deep_sleep_start(); // Spusteni uspani
}
