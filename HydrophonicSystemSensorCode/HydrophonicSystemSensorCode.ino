
#include "DFRobot_PH.h"
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include "DFRobot_EC.h"
#include <RTClib.h>

#define PH_PIN A1
#define EC_PIN1 A3
#define OX_PIN A4
#define ONE_WIRE_BUS 4 // Data wire is plugged into digital pin 4 on the Arduino
#define VREF    5000//VREF(mv)
#define ADC_RES 1024//ADC Resolution
#define TWO_POINT_CALIBRATION 1
//Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (820) //mv
#define CAL1_T (17)   //℃
//Two-point calibration needs to be filled CAL2_V and CAL2_T
//CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1260) //mv
#define CAL2_T (31)   //℃

float voltagepH,phValue;
DFRobot_PH ph;
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);// Pass our oneWire reference to Dallas Temperature.
uint32_t rawOxygenValue;
float voltageEC,ecValue;

DFRobot_EC ec;
uint8_t Temperature; // not needed
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
uint16_t doValue;

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};


bool firstReading = true; // Flag to indicate the first reading

#define PUMP1_START_HOUR 8
#define PUMP2_START_HOUR 16
#define PUMP_DURATION_MINUTES 30

#define LIGHT_START_HOUR 6
#define LIGHT_END_HOUR 18

const int pumpPins[] = {2, 3, 5};         // work on the pinnnage! determine which arduino will use what pins, put it there, and its good to go
const int lightPins[] = {6, 7, 8, 9, 10, 11, 12, A0, A2, A5, A6, A7};

bool pumpsOn = false;
bool lightsOn = false;

int hourOfDay = 0; // if not using a real time clock module, this is the easiest solution to simulate day-night-cycle.
int minuteOfHour = 0; // same as above

unsigned long elapsedMillis = 0;
const unsigned long minuteInterval = 60000; // 60,000 milliseconds = 1 minute

void setup() {
    ph.begin();
    sensors.begin();//Temperature sensor
    ec.begin();
    Serial.begin(9600);
}

void loop() {
    elapsedMillis = millis() / minuteInterval;

    hourOfDay = (elapsedMillis / 60) % 24; // Hours (0-23)
    minuteOfHour = elapsedMillis % 60; // Minutes (0-59)

    // --- Pump Control Logic ---
    bool inFirstWindow  = hourOfDay == PUMP1_START_HOUR  && minuteOfHour < PUMP_DURATION_MINUTES;
    bool inSecondWindow = hourOfDay == PUMP2_START_HOUR  && minuteOfHour < PUMP_DURATION_MINUTES;

    if ((inFirstWindow || inSecondWindow) && !pumpsOn) {
      pumpsOn = true;
      //if the pins are determined, uncomment the part below
      //for (int i = 0; i < 3; i++) digitalWrite(pumpPins[i], HIGH);
    }
    else if (!inFirstWindow && !inSecondWindow && pumpsOn) {
      pumpsOn = false;
      //if the pins are determined, uncomment the part below
      //for (int i = 0; i < 3; i++) digitalWrite(pumpPins[i], LOW);
    }

    // --- Light Control Logic ---
    if (hourOfDay >= LIGHT_START_HOUR && hourOfDay < LIGHT_END_HOUR && !lightsOn) {
        lightsOn = true;
        // Uncomment the part below if pins are determined
        // for (int i = 0; i < 12; i++) digitalWrite(lightPins[i], HIGH);
    }
    else if ((hourOfDay < LIGHT_START_HOUR || hourOfDay >= LIGHT_END_HOUR) && lightsOn) {
        lightsOn = false;
        // Uncomment the part below if pins are determined
        // for (int i = 0; i < 12; i++) digitalWrite(lightPins[i], LOW);
    }

    // --- Data Sending Logic ---
    if (minuteOfHour == 0 || minuteOfHour == 30){ // Every half an hour
      float ph = getpH();
      float ec = getEc();
      float temp = getTempSensor();
      float doVal = getOxygenSensor();

      // Skip first reading (first data contains also debugging lines stream)
      if (firstReading) {
        firstReading = false;
      } else {
        Serial.print(temp); Serial.print(",");
        Serial.print(ec);   Serial.print(",");
        Serial.print(ph);   Serial.print(",");
        Serial.print(doVal); Serial.println();
      }

      delay(60000); // Prevent re-reading during the same minute
    }
}

float getEc(){
  static unsigned long timepoint = millis();
  int temperature;
    if(millis()-timepoint>1000U){                  //time interval: 1s
        timepoint = millis();

        temperature = getTempSensor();          // read your temperature sensor to execute temperature compensation
        voltageEC = analogRead(EC_PIN1)/1024.0*5000;   // read the voltage
        //ecValue =  ec.readEC(voltageEC,temperature);
        ecValue =  ec.readEC(voltageEC, temperature);  // convert voltage to EC with temperature compensation
        // IMPORTANT INFO! if temperature sensor is not connected, the results may be different than in reality - change it into a constant value instead..
    }

    //ec.calibration(voltageEC,temperature);          // calibration process by Serail CMD
    return ecValue;
}

float getpH(){
  static unsigned long timepoint = millis();
  int temperature;
    if(millis()-timepoint>1000U){                  //time interval: 1s
        timepoint = millis();

        temperature = getTempSensor();         // read your temperature sensor to execute temperature compensation
        voltagepH = analogRead(PH_PIN)/1024.0*5000;  // read the voltage
        phValue = ph.readPH(voltagepH, temperature);  // convert voltage to pH with temperature compensation
        // IMPORTANT INFO! if temperature sensor is not connected, the results may be different than in reality - change it into a constant value instead.

    }
    //ph.calibration(voltagepH,temperature);           // calibration process by Serail CMD
    return phValue;
}

float getTempSensor(){
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    return temp;
}

float getOxygenSensor(){
    float temperature = getTempSensor();
    ADC_Raw = analogRead(OX_PIN);
    ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
    // Convert ADC voltage to dissolved Oxygen concentration
    float do_concentration = readDO(ADC_Voltage, temperature); // μg/L
    // IMPORTANT INFO! if temperature sensor is not connected, the results may be different than in reality - change it into a constant value instead.
    return do_concentration/1000; // mg/L
}


int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c)
{
#if TWO_POINT_CALIBRATION == 0
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}
