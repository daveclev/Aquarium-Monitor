/* David Clevenstine comment
 * This is an ESP8266 sketch.
 * It provides: 
 *   Dallas Temperature Sensor reading
 *   Discrete monitor of the state of the return pump shutoff switch
 *   Reporting of temperature and return pump to ThingSpeak 
 * 
 * Connect Dallas sensor 1-wire to pin 
 * Connect discrete input to pin. Code is setup to see logic high as pump off. 
 * v0.1 NOT COMPILING - basic code write based heavily on "Functional_Chiller_v1.6"
 */

// Include for DS18B20:
#include <OneWire.h>
#include <DallasTemperature.h>

// Include for ThingSpeak
#include "ThingSpeak.h" //install library for thing speak
#include <ESP8266WiFi.h>

//Wifi Login
char ssid[] = "ParadiseFruit-WiFi";        // your network SSID (name) 
char pass[] = "Peaches!";    // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

//ThingSpeak Link
unsigned long myChannelNumber = 1448062;
const char * myWriteAPIKey = "KAAHFGQSTTH8U64W";
String myStatus = "";
bool thingSpeakWriteSuccesfull = 0;

int ledA = 5; // the green LED on ESP8266 Thing; not used in this sketch.

// Pin Assignments
const int oneWireBus = 5;      // GPIO where the DS18B20 1-wire is connected
const bool returnPumpOff = 0;  // GPIO where the return pump monitor is attached

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

float temperatureF_DisplayTank;
float temperatureF_ChillerIntake;
float temperatureF_ChillerOutput;
float temperatureF_Ambient;
long lastThingSpeakUpdateTime = 0;
long tsCurrentTimer = 0;
bool returnPumpRunning = 1; // use to hold state of return pump.

long loggedMillis{};
const float setpointTemp{79.9};          //threshold where chiller is triggered

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(returnPumpOff, INPUT); // initialize pin as input
  
  //Wire.begin(); // Start I2C to MCP23017

  sensors.begin(); // Start the DS18B20 sensor

  Serial.println();

  //WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  // Initialize ThingSpeak

}

void loop() {
  // put your main code here, to run repeatedly:
  
  // read state of pump discrete and save to variable.
  returnPumpRunning = not(digitalRead(returnPumpOff)); 
  
  
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }

  // One minute delay between thingspeak updates 
  if ((verifyMillis() - tsCurrentTimer) > 60000){
    lastThingSpeakUpdateTime = verifyMillis();
    thingSpeakUpdate();
  }
  
  
  getTemperature();
  // long tempMillis = verifyMillis();
  // if((tempMillis - serialPrintLast) > 5000)             // limits serial print to 30 second intervals
  // {
  //   printStats();
  //   serialPrintLast = verifyMillis();
  // }
}

float requestTemp(int sensorNumber, int previousTemp){
  sensors.requestTemperatures();
  float x = sensors.getTempFByIndex(sensorNumber);
  if (x > 0)  //eliminate faulty -196.6 readings
    return x;
  else
    return previousTemp;
}

// int checkTemps(){
//   if ((temperatureF_ChillerIntake > setpointTemp) && (temperatureF_DisplayTank > setpointTemp) && ((verifyMillis() - chillerOffTime) > chillerSetOffTime))
//     return 1;
//   else if ((temperatureF_ChillerIntake > setpointTemp) && (temperatureF_DisplayTank > setpointTemp) && (firstChillerRunCompleted == false))
//     return 1;
//   else
//     return 0;
// }

void getTemperature(){
  temperatureF_DisplayTank = requestTemp(0, temperatureF_DisplayTank); 
  temperatureF_ChillerIntake = requestTemp(2, temperatureF_ChillerIntake);
  temperatureF_ChillerOutput = requestTemp(1, temperatureF_ChillerOutput);
  temperatureF_Ambient = requestTemp(3, temperatureF_Ambient);
}

// setup using https://www.instructables.com/ThingSpeak-Using-ESP8266/
void thingSpeakUpdate(){
  
  // define local variables that match thingspeak channel's field names using global variables.
  float DisplayTempF    = temperatureF_DisplayTank;
  float ChillerInTempF  = temperatureF_ChillerIntake;
  float ChillerOutTempF = temperatureF_ChillerOutput;
  float AmbientTempF    = temperatureF_Ambient;
  bool ReturnPumpOn     = returnPumpRunning;

  // set the fields with the values
  ThingSpeak.setField(1, DisplayTempF);       
  ThingSpeak.setField(2, ChillerInTempF);     
  ThingSpeak.setField(3, ChillerOutTempF);    
  ThingSpeak.setField(4, AmbientTempF);       
  ThingSpeak.setField(5, ReturnPumpOn);      
  
  // set the status
  ThingSpeak.setStatus(myStatus); //not currently used. it will always be zero as it was set in the setup.
  
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}

long verifyMillis(){                  //since millis() resets to 0 after aprox. 50 days,                                   //
  if(loggedMillis > millis()){        //we need to take that into account by making sure it hasn't reset because if we didn't,  
    resetVariables();                 //the pump would start running (or not running) 50 days because startTime and chillerOffTime would still
  }                                   //be huge because they hadn't reset!  So verifyMillis() will reset those variables to their starting
  loggedMillis = millis();             //points if millis() resets.
  return millis();
}

void resetVariables(){
  //  chillerOffTime = 0;  
  //  startTime = 0;
  //  serialPrintLast = 0;
  //  firstChillerRunCompleted = false;
   lastThingSpeakUpdateTime = 0;
}
