/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  Date: December 5th, 2020

  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. 
*/

//Libraries
#include <Wire.h>
#include "MAX30105.h"
#include <LiquidCrystal_I2C.h>
#include "heartRate.h"

//Global Variables
const byte RATE_SIZE = 6;  //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];  //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;  //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
int maxBeat = 0;

//Creating instances of Max30105 and LiquidCrystal_I2C
MAX30105 particleSensor;
LiquidCrystal_I2C lcd(0x27, 16 ,2);

// Serial data variables
//Incoming Serial Data Array
const byte kNumberOfChannelsFromExcel = 6; 
int PulseToExcel = 0;   //This value will show in Excel's channel 1
const char kDelimiter = ',';    // Comma delimiter to separate consecutive data if using more than 1 sensor
const int kSerialInterval = 50;   // Interval between serial writes
unsigned long serialPreviousTime;   // Timestamp to track serial interval
char* arr[kNumberOfChannelsFromExcel];


void setup()
{
  Serial.begin(115200);
//  Serial.println("Initializing...");
   
  lcd.begin();
  lcd.backlight();
  lcd.clear();

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
//    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
//  Serial.println("Place your index finger on the sensor with steady pressure.");
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}


void loop()
{
  long irValue = particleSensor.getIR();  //having the value of the IR LED sensor

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();  //track the time elapsed from the last beat

    beatsPerMinute = 60 / (delta / 1000.0);   //the definition of BPM

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute;   //Store this reading in the array
      rateSpot %= RATE_SIZE;  //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
  
  //If there is no pulse detected
  if((millis()-lastBeat)>10000 || irValue < 50000) {  
      beatAvg= 0;
      beatsPerMinute = 0;
      maxBeat = 0;  
    }

//  Serial.print("IR=");
//  Serial.print(irValue);
//  Serial.print(", BPM=");
//  Serial.print(beatsPerMinute);
//  Serial.print(", Avg BPM=");
//  Serial.print(beatAvg);

if(beatAvg > 35) {
  // Gather and process sensor data
  processSensors();

  // Read Excel variables from serial port (Data Streamer)
  processIncomingSerial();

  // Process and send data to Excel via serial port (Data Streamer)
  processOutgoingSerial();
}

  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("BPM : ");
  lcd.print(beatsPerMinute);

  lcd.setCursor(0,1);
  lcd.print("Avg BPM : ");
  lcd.print(beatAvg);
}

void processSensors() 
{

  if(beatAvg > maxBeat) {
    maxBeat = beatAvg;  
    PulseToExcel = maxBeat;  // Read sensor pin and store to a variable
    // Add any additional raw data analysis below (e.g. unit conversions)  
  }
}

void sendDataToSerial()
{
  // Send data out separated by a comma (kDelimiter)
  // Repeat next 2 lines of code for each variable sent:
  Serial.print(PulseToExcel);
  Serial.print(kDelimiter);
  
  Serial.println(); // Add final line ending character only once
  
}

// OUTGOING SERIAL DATA PROCESSING CODE----------------------------------------
void processOutgoingSerial()
{
   // Enter into this only when serial interval has elapsed
  if((millis() - serialPreviousTime) > kSerialInterval) 
  {
    // Reset serial interval timestamp
    serialPreviousTime = millis(); 
    sendDataToSerial(); 
  }
}

// INCOMING SERIAL DATA PROCESSING CODE----------------------------------------
void processIncomingSerial()
{
  if(Serial.available()){
    parseData(GetSerialData());
  }
}

// Gathers bytes from serial port to build inputString
char* GetSerialData()
{
  static char inputString[64]; // Create a char array to store incoming data
  memset(inputString, 0, sizeof(inputString)); // Clear the memory from a pervious reading
  while (Serial.available()){
    Serial.readBytesUntil('\n', inputString, 64); //Read every byte in Serial buffer until line end or 64 bytes
  }
  return inputString;
}

// Seperate the data at each delimeter
void parseData(char data[])
{
    char *token = strtok(data, ","); // Find the first delimeter and return the token before it
    int index = 0; // Index to track storage in the array
    while (token != NULL){ // Char* strings terminate w/ a Null character. We'll keep running the command until we hit it
      arr[index] = token; // Assign the token to an array
      token = strtok(NULL, ","); // Conintue to the next delimeter
      index++; // incremenet index to store next value
    }
}
