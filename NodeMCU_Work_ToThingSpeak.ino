/*ESP8266 --> ThingSpeak Channel
 
 channel using the ThingSpeak API (https://www.mathworks.com/help/thingspeak).
 
 Requirements:
 
   * ESP8266 Wi-Fi Device
   * Arduino 1.8.8+ IDE
   * Additional Boards URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
   * Library: esp8266 by ESP8266 Community
   * Library: ThingSpeak by MathWorks, DHT11 by Arduino Community
 
 ThingSpeak Setup:
 
   * Sign Up for New User Account - https://thingspeak.com/users/sign_up
   * Create a new Channel by selecting Channels, My Channels, and then New Channel
   * Enable one field
   * Enter SECRET_CH_ID in "secrets.h"
   * Enter SECRET_WRITE_APIKEY in "secrets.h"
 Setup Wi-Fi:
  * Enter SECRET_SSID in "secrets.h"
  * Enter SECRET_PASS in "secrets.h"
*/

//Libraries
#include "ThingSpeak.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
#include "DHT.h"

//These are dedicated pins
#define TRIGGER_PIN D5 
#define ECHO_PIN D6 
#define LED_pin D4
#define DHTPIN D7
#define DHTTYPE DHT11

//Global variables
unsigned long myChannelNumber = SECRET_CH_ID;  //the 8 digits number of my channel (secret.h)
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;  //the API Key (secret.h)
uint32_t timeSpentWithoutWork = 0;  //this will calculate the time without spending work
float duration = 0; 
float distance = 0;
float temperature = 0;
int humidity = 0;
long rssi = 0;
int lastHumidity = 0;  //the last correct value of humidity (pre-filtering)
char ssid[] = SECRET_SSID;   // My network SSID (name)
char pass[] = SECRET_PASS;   // My network password
int keyIndex = 0;// My network key index number (needed only for WEP)
boolean IsWorking = 1;

//Status fields
String myStatus_DHT = "";
String myStatus_Ultrasonic = "";

//Creating instances of WiFiClient and DHT
WiFiClient  client;
DHT dht(DHTPIN, DHTTYPE);


void setup() {
  Serial.begin(115200);
  //Setting the states of the used pins
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_pin, OUTPUT);
  delay(100);

  WiFi.mode(WIFI_STA);  //Our device will work as a station in client host architecture
  
  ThingSpeak.begin(client);  //Establishing communication
}


void loop() {
  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  //Set the fields with the corresponding values
  ThingSpeak.setField(1, rssi);
  ThingSpeak.setField(2, IsWorking);
  ThingSpeak.setField(3, humidity);
  ThingSpeak.setField(4, temperature);
  ThingSpeak.setField(5, distance);

  if(temperature >= 20 && temperature <= 24 && humidity < 60){ 
    myStatus_DHT = String("Temperature is pleasant and the humidity is comfortable at Home."); 
  }
  else if(temperature > 24 && humidity < 60){
    myStatus_DHT = String("There is really warm at Home, but the humidity is on the recommended level.");
  }
  else if(temperature >= 20 && temperature <= 24 && humidity > 60 ) {
    myStatus_DHT = String("My home's temperature is great, but the humidity is above the recommended");
  }
  else if(temperature > 24 && humidity < 60) {
    myStatus_DHT = String("Both the temperature and the humidity is really high.");
  }
  else{
    myStatus_DHT = String("My Home is really cold");
  }

//  ThingSpeak.setStatus(myStatus_DHT);
  
  ThingSpeak.setStatus(myStatus_Ultrasonic);  //Sending the status according to the IsWorking value
  delay(50);
  
  // Write value to certain Fields of a ThingSpeak Channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel write successful.");
  }
  else {
    Serial.println("Problem writing to channel. HTTP error code " + String(x));
  }

  //Getting data
  rssi = WiFi.RSSI();  // Measure Signal Strength (RSSI) of Wi-Fi connection
  temperature = dht.readTemperature();  //Measure the value of temperature around the workstation
  humidity = dht.readHumidity();  //Measure the value of humidity around the workstation

  //Prefiltering
  if (0 <= humidity <= 100){
    lastHumidity = humidity;
  } else {
    humidity = lastHumidity;
  }
  delay(100);

  //Ultrasonic distance measuring
  digitalWrite(TRIGGER_PIN, LOW);  // Get Start
  delayMicroseconds(2);  // stable the line 
  digitalWrite(TRIGGER_PIN, HIGH); // sending 10 us pulse
  delayMicroseconds(10); // delay  
  digitalWrite(TRIGGER_PIN, LOW); // after sending pulse wating to receive signals   
  duration = pulseIn(ECHO_PIN, HIGH); // calculating time 
  distance = (duration) / 58.2; // single path 
  
  Serial.println("rssi: " + rssi);
  Serial.print(distance);
  Serial.println(" cm");
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.println(IsWorking);

 //The permissible distance where the home officer is working
 if(distance < 105 && distance > 5){
    digitalWrite(LED_pin, HIGH);
    IsWorking = 1;
  } else {
    digitalWrite(LED_pin, LOW);
    IsWorking = 0;
    timeSpentWithoutWork = millis();
  }

  //Examining the value of IsWorking
  if(IsWorking == 1) {
    myStatus_Ultrasonic = String("I am working at the moment");
    timeSpentWithoutWork = 0;
  }
  else if(IsWorking == 0 && timeSpentWithoutWork > 240000) {
    myStatus_Ultrasonic = String("I have left my workplace");
  }
  
  delay(29850);  // Wait almost 30 seconds to update the channel again
}
