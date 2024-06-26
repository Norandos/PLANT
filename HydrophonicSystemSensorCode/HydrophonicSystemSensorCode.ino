
#include "DFRobot_PH.h"
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include "DFRobot_EC.h"

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
uint8_t Temperature;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
uint16_t doValue;

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};


bool firstReading = true; // Flag to indicate the first reading

void setup() {
    Serial.begin(9600);  
    ph.begin();
    sensors.begin();//Temperature sensor
    ec.begin();
}


void loop() {
    float phValue = getpH();
    float ecValue = getEc();
    float temp = getTempSensor();
    float doValue = getOxygenSensor();

    // Skip the first reading to avoid faulty insertion of '0.00' of Conductivity and pH.
    if (firstReading) {
        firstReading = false;
        return; // Skip sending the data
    }

    // Print the sensor data in a CSV format to serial monitor
    Serial.print(temp);
    Serial.print(",");
    Serial.print(ecValue);
    Serial.print(",");
    Serial.print(phValue);
    Serial.print(",");
    Serial.print(doValue);
    Serial.println(); // New line at the end
    
    // Wait for 30 minutes (30 * 60 * 1000 milliseconds)
    //NOTE: Delay is a blocking function and will prevent any other code from being executed. Only using it for simplicity. If more features of the Arduino are required
    //      it is recommended to change to a non-blocking delay form.
    delay(30 * 60 * 1000UL);
    //delay(2000);
    }

float getEc(){
  static unsigned long timepoint = millis();
  int temperature;
    if(millis()-timepoint>1000U){                  //time interval: 1s
        timepoint = millis();
                
        temperature = getTempSensor();          // read your temperature sensor to execute temperature compensation
        voltageEC = analogRead(EC_PIN1)/1024.0*5000;   // read the voltage
        ecValue =  ec.readEC(voltageEC,temperature);  // convert voltage to EC with temperature compensation
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
        phValue = ph.readPH(voltagepH,temperature);  // convert voltage to pH with temperature compensation
        
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
