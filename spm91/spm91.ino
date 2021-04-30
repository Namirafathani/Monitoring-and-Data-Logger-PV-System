/**********************************************************************
  ModbusEnergyMonitor MODIFIED by D.F. SYAHBANA
  An example to collect data from a Modbus energy monitor using ModbusSensor class
  to datalogger, include a RTC DS3231 and a SD card
  version 0.1 ALPHA 14/12/2015

  Author: Jaime García  @peninquen
  License: Apache License Version 2.0.

  Modified for:
  Emon vModbus
  MHPS Monitoring
  Al-Umanaa Boarding School, Sukabumi
  emon.se-movement.com
**********************************************************************/
/*

*/
//#include <LiquidCrystal.h>
#include "ModbusSensor.h"
//#include "connectwifi.h"
//#include "resetwifi.h"
//#include "httppost.h"

/*********** SUBMODULE def **************/
//MicroSD
//#define CS_pin 24
//File myFile;

//LCD
//const int rs = 12, en = 11, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define device_id 1
#define SERIAL_OUTPUT 1

#if SERIAL_OUTPUT
#   define SERIAL_BEGIN(...) Serial.begin(__VA_ARGS__)
#   define SERIAL_PRINT(...) Serial.print(__VA_ARGS__)
#   define SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#   define SERIAL_BEGIN(...)
#   define SERIAL_PRINT(...)
#   define SERIAL_PRINTLN(...)
#endif

#define MB_SERIAL_PORT &Serial1   // Arduino has only one serial port, Mega has 3 serial ports.
// if use Serial 0, remember disconect Tx (pin0) when upload sketch, then re-conect
#define MB_BAUDRATE       9600          // b 2400
#define MB_BYTEFORMAT     SERIAL_8N2    // Prty n
#define TxEnablePin       8
#define TIMEOUT           100

#define ID_1  1                       // id 001  modbus id of the energy monitor
#define REFRESH_INTERVAL  5000        // refresh time, 5 SECONDS
#define WRITE_INTERVAL 900000UL        // values send to serial port, 1 minute ( 1* 60 * 1000)
#define WRITE_INTERVAL_SEC 20
#define WRITE_INTERVAL_SD 300000UL
#define DISPLAY_INTERVAL 2000
#define KWH_2_WS 36000000

// Direcciones registros de datos solo lectura. Valores tipo float.
// Utilizar funcion 04 lectura, numero de registros 16-bits 2.

#define VOL_ADR 0x40003    // VOLTAJE.
#define CUR_ADR 0x40004    // CORRIENTE.
#define POW_ADR 0x40006    // POTENCIA ACTIVA. 
#define APO_ADR 0x0012    // Potencia Aparente.
#define PFA_ADR 0x001E    // Factor de potencia.
#define FRE_ADR 0x40012    // Frecuencia.
#define PEN_ADR 0x40001    // ENERGIA IMPORTADA KWH
#define REN_ADR 0x004A    // Energia exportada.
#define TEN_ADR 0x0156    // Energia activa Total.
#define TRE_ADR 0x0158    // Energia reactiva Total.

// multiplication factor, store value as an integer
#define VOL_FAC 10
#define CUR_FAC 100
#define POW_FAC 10
#define PFA_FAC 100
#define FRE_FAC 10
#define ENE_FAC 100

/* ESP8266 */
//String _ssid = "DeathofCharizard";
//String _password = "flyingchicken";
//String _server = "www.se-movement.com";
//String _uri = "/EMS/add.php";
//String _data = "";

int post_interval = 1000;
int current_millis_http=0;
int prev_millis_http=0;

modbusMaster MBserial(MB_SERIAL_PORT, TxEnablePin);  // instance to collect data using Modbus protocol over RS485

//variables to poll, process and send values
modbusSensor volt(&MBserial, ID_1, VOL_ADR, CHANGE_TO_ZERO);
modbusSensor curr(&MBserial, ID_1, CUR_ADR, CHANGE_TO_ZERO);
modbusSensor pwr(&MBserial, ID_1, POW_ADR, CHANGE_TO_ZERO);
modbusSensor enrg(&MBserial, ID_1, PEN_ADR, HOLD_VALUE);
modbusSensor freq(&MBserial, ID_1, FRE_ADR, CHANGE_TO_ZERO);
modbusSensor aPwr(&MBserial, ID_1, APO_ADR, CHANGE_TO_ZERO);
modbusSensor pwrFact(&MBserial, ID_1, PFA_ADR, CHANGE_TO_ONE);

uint16_t voltage, maxVoltage, minVoltage; // integer, factor x10
uint16_t current, maxCurrent, minCurrent; // integer, factor x100
uint16_t power, maxPower, minPower;       // integer, factor x10
uint16_t lastEnergy, energy, avgPower;    // integer, factor x100
uint16_t frequency, maxFreq, minFreq;     // integer, factor x100
uint16_t aPower, maxApower, minApower;    // integer, factor x10
uint16_t powerFactor, maxPF, minPF;       // integer, factor x100

float print_maxVoltage, print_minVoltage, print_maxCurrent, print_minCurrent;
float print_maxPower, print_minPower, print_maxApower, print_minApower;
float print_maxFreq, print_minFreq, print_avgPower, print_energy;

unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

unsigned long previousMillis_modbus = 0;
unsigned long currentMillis_modbus = 0;

unsigned long previousMillis_lcd = 0;
unsigned long currentMillis_lcd = 0;

unsigned long previousMillis_sd = 0;
unsigned long currentMillis_sd = 0;

boolean       firstData;
boolean initial = true;
uint8_t SHOW_KWH = 0;
uint8_t SHOW_WATT = 1;
uint8_t LCD_dataShow = 0;
unsigned long curr_display = 0;
unsigned long prev_display = 0;

//void displayTime()
//{
//  //display time
//  //t = rtc.getTime();
//  //setTimeandDate
//  //rtc.setTime(15, 25, 20);
//  //rtc.setDate(25, 5, 2018);
//  // Send date
//  lcd.setCursor(0, 0);
//  lcd.print("                ");
//  lcd.setCursor(0, 0);
//  //lcd.print(t.date, DEC);
//  //lcd.print(rtc.getMonthStr());
//  //lcd.print(t.year, DEC);
//  //lcd.print(rtc.getDateStr());
//  // Send time
//  //lcd.setCursor(11, 0);
//  //lcd.print(t.hour);
//  //lcd.print(":");
//  //lcd.print(t.min);
//}

//void display_kwh_watt(float energy_kwh, float power_watt)
//{
//
//  /****LCD PRINT*****/
//  curr_display = millis();
//  if (curr_display - prev_display >= DISPLAY_INTERVAL)
//  {
//    prev_display = curr_display;
//    lcd.setCursor(0, 1);
//    lcd.print("                 ");
//    //lcd.setCursor(12,1);
//    //lcd.print("    ");
//    if (LCD_dataShow == SHOW_KWH)
//    {
//      lcd.setCursor(0, 1);
//      lcd.print("Enrgy: ");
//      lcd.print(energy_kwh, 3);
//      lcd.setCursor(13, 1);
//      lcd.print("kWh ");
//      LCD_dataShow = SHOW_WATT;
//    }
//    else if (LCD_dataShow == SHOW_WATT)
//    {
//      lcd.setCursor(0, 1);
//      lcd.print("Power: ");
//      lcd.print(power_watt, 2);
//      lcd.setCursor(12, 1);
//      lcd.print("Watt");
//      LCD_dataShow = SHOW_KWH;
//    }
//  }

//}

void setup() {
  /*****BEGIN*****/
  SERIAL_BEGIN(9600);
  Serial.begin(115200);
//  Serial2.begin(115200);
//  resetwifi();
//  connectwifi(_ssid, _password);
  //RTC
  //rtc.begin();
  //LCD
//  lcd.begin(16, 2);

  //uSD Card
  //  if (!SD.begin(CS_pin)) {
  //    lcd.setCursor(0, 1);
  //    lcd.print("init sdcard failed!");
  //    //return;
  //  }
  //  lcd.setCursor(0, 1);
  //  lcd.print("SDCard OK");
  //  //open file
  //  myFile = SD.open("Energy.txt", FILE_WRITE);
  //  if (myFile) {
  //    lcd.setCursor(11, 1);
  //    lcd.print("START");
  //    //myFile.println("Date; time; Power(Watt); Energy (Kwh)");
  //    myFile.close();
  //  } else {
  //    // if the file didn't open, print an error:
  //    lcd.setCursor(0, 1);
  //    lcd.print("NO SDCard");
  //  }


  MBserial.begin(MB_BAUDRATE, MB_BYTEFORMAT, TIMEOUT, REFRESH_INTERVAL);
  delay(95);
  //SERIAL_PRINTLN(rtc.getTimeStr());
  SERIAL_PRINTLN("time(s), maxVolt(V), minVolt(V), maxCurr(A) minCurr(A), maxPower(W), minPower(W), maxApPower(VA), minApPower(VA), maxFreq(Hz), minFreq(Hz), AvgPower (W), Energy(Kwh)");
  firstData = false;
  power = 0;
  maxPower = 0;    // in case it has been recorded, use it
  minPower = 0;
  lastEnergy = 0;  // in case it has been recorded, use it
  energy = lastEnergy;
}

void loop() {
//  sei();
  //displayTime();
  //lcd.setCursor(0,0);lcd.print("Test");
  if (MBserial.available()) {
    voltage = volt.read(VOL_FAC);
    current = curr.read(CUR_FAC);
    power = pwr.read(POW_FAC);
    aPower = aPwr.read(POW_FAC);
    frequency = freq.read(FRE_FAC);
    energy = enrg.read(ENE_FAC);

    if (!firstData) {
      if (maxVoltage < voltage) maxVoltage = voltage;
      if (minVoltage > voltage) minVoltage = voltage;
      if (maxCurrent < current) maxCurrent = current;
      if (minCurrent > current) minCurrent = current;
      if (maxPower < power) maxPower = power;
      if (minPower > power) minPower = power;
      if (maxApower < aPower) maxApower = aPower;
      if (minApower > aPower) minApower = aPower;
      if (maxFreq < frequency) maxFreq = frequency;
      if (minFreq > frequency) minFreq = frequency;
      if (maxPower < power) maxPower = power;
      if (minPower > power) minPower = power;
    }
    else {
      maxVoltage = voltage;
      minVoltage = voltage;
      maxCurrent = current;
      minCurrent = current;
      maxPower = power;
      minPower = power;
      maxApower = aPower;
      minApower = aPower;
      maxFreq = frequency;
      minFreq = frequency;
      firstData = false;
    }
  }

  currentMillis = millis();
  if (currentMillis - previousMillis >= WRITE_INTERVAL) {
    previousMillis = currentMillis;
    avgPower = (energy - lastEnergy) * KWH_2_WS / (WRITE_INTERVAL / 1000); //average power KWh/s to W
    Serial.print(energy); Serial.print(" - "); Serial.print(lastEnergy); Serial.print(" || AvgPower = "); Serial.println(avgPower);
    lastEnergy = energy;
    firstData = true;

    print_maxVoltage = (float)maxVoltage / VOL_FAC, 1;
    print_minVoltage = (float)minVoltage / VOL_FAC, 1;
    print_maxCurrent = 
    (float)maxCurrent / CUR_FAC, 2;
    print_minCurrent = (float)minCurrent / CUR_FAC, 2;
    print_maxPower  = (float)maxPower / POW_FAC, 2;
    print_minPower  = (float)minPower / POW_FAC, 2;
    print_maxApower = (float)maxApower / POW_FAC, 2;
    print_minApower = (float)minApower / POW_FAC, 2;
    print_maxFreq   = (float)maxFreq / FRE_FAC, 2;
    print_minFreq   = (float)minFreq / FRE_FAC, 2;
    print_avgPower  = (float)avgPower / ENE_FAC, 2;
    print_energy    = (float)energy / ENE_FAC, 2;
    print_energy    = print_energy - 0.41;

    SERIAL_PRINT(currentMillis_modbus / 1000); SERIAL_PRINT(",");
    SERIAL_PRINT(print_maxVoltage); SERIAL_PRINT(",");
    SERIAL_PRINT(print_minVoltage); SERIAL_PRINT(",");
    SERIAL_PRINT(print_maxCurrent); SERIAL_PRINT(",");
    SERIAL_PRINT(print_minCurrent); SERIAL_PRINT(",");
    SERIAL_PRINT(print_maxPower); SERIAL_PRINT(",");
    SERIAL_PRINT(print_minPower); SERIAL_PRINT(",");
    SERIAL_PRINT(print_maxApower); SERIAL_PRINT(",");
    SERIAL_PRINT(print_minApower); SERIAL_PRINT(",");
    SERIAL_PRINT(print_maxFreq); SERIAL_PRINT(",");
    SERIAL_PRINT(print_minFreq); SERIAL_PRINT(",");
    SERIAL_PRINT(print_avgPower); SERIAL_PRINT(",");
    SERIAL_PRINTLN(print_energy);
    //Serial.println(rtc.getTimeStr());

/*SEND DATA TO WIFI*/
    
//    _data = 
//      "ID=" + String(device_id) +
//      "&volt=" + String(print_maxVoltage) + 
//      "&curr=" + String(print_maxCurrent) + 
//      "&power=" + String(print_maxPower) + 
//      "&energy=" + String(print_energy);

//Dummy
/*
      _data = 
      "ID=" + String(device_id) +
      "&volt=220.4" + 
      "&curr=1.2" + 
      "&power=264.48" + 
      "&energy=890";
*/
    current_millis_http = millis();
    //if ((current_millis_http - prev_millis_http) >= post_interval)
    //{
     // prev_millis_http = current_millis_http;
//      Serial.println("[SENDING DATA]");
      Serial.println("  V = " + String(print_maxVoltage)); 
      Serial.println("  I = " + String(print_maxCurrent)); 
      Serial.println("  P = " + String(print_maxPower)); 
      Serial.println("  E = " + String(print_energy));
//      httppost(_server, _uri, _data);
   // }
  }



  //currentMillis_lcd = millis();
  //if((currentMillis_lcd - previousMillis_lcd >= DISPLAY_INTERVAL)){
  //previousMillis_lcd = currentMillis_lcd;
  //display_kwh_watt(print_energy, print_avgPower);
  //}

//  display_kwh_watt(print_energy, print_maxPower);
}
