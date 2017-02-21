#include<LiquidCrystal.h>                                                                                    //Header file for LCD display
#include<SoftwareSerial.h>                                                                                   //Header file to define additional serial input pins for GSM
SoftwareSerial mySerial(3, 4);                                                                               //Variable mySerial of type SoftwareSerial, syntax:(rxPin, txPin)
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);                                                                     //Variable lcd of type LiquidCrystal, syntax: (rs,enable,d4,d5,d6,d7)

int ReedPin = 5;		                                                                             //Reed Switch - detects presence of magnetic field
int PowerPin = 6;		                                                                             //Power Switch
int PirPin = 7;		                                                                                     //PirStatus - detects radiation from human body
int TempPin = A2; 		                                                                             //Temperature
int LightPin = A5;		                                                                             //Light

String GsmInput = "";                                                                                        //To store gsm input
String GsmResponse = "";                                                                                     //To store gsm response
float Celsius, Light;
String PirStatus, PowerSwitchStatus, ReedSwitchStatus, TemperatureString,LightString;                        //To store sensor value as string
String url;                                                                                                  //url for get request with sensor values
String SensorString, SensorStatus;

void setup()
{
  Serial.begin(9600);                                                                                        //Normal serial port
  mySerial.begin(9600);                                                                                      //Software serial ports
  lcd.begin(16, 2);                                                                                          //Initialize lcd number of rows and columns
  pinMode(PowerPin, INPUT);                                                                                  //Define Power switch as input
  pinMode(ReedPin, INPUT);                                                                                   //Define reed switch as input
  pinMode(PirPin, INPUT);                                                                                    //Define pir as input
}

void DisplayGSMResponse()                                                                                    
{
  if (mySerial.available())                                                                                  //If data is available at GSM serial port
  {
    String GsmInput = "";                                                                                    //Define GsmInput to store GSM response
    while (mySerial.available())                                                                             //When a response is present
    {
      GsmInput += (char)mySerial.read();                                                                     //Add current data bits to whatever is already saved
    }
    GsmInput.trim();                                                                                         //Removes spaces in the string    
    lcd.setCursor(0, 1);                                                                                     //Keep LCD cursor at column 0 , row 1
    lcd.print(GsmInput);                                                                                     //Print GSM input on LCD
    Serial.println(GsmInput);
    delay(1000);                                                                                             //Wait
  }
}

void gsmMessage(String atCommand, String LcdDescription, int commandDelay = 0){
  
  mySerial.println(atCommand);                                                                               //Start writing commands for the gsm module (attention gsm module!)
  Serial.println(LcdDescription);
  lcd.clear();
  lcd.print(LcdDescription);                                                                                 //Show current process on lcd
  delay(commandDelay);
  DisplayGSMResponse(); 
}

String getAnalogSensor(int sensorPin, String LcdDescription, String SerialDescription){
  Serial.println(SerialDescription);
  int PinInput = analogRead(sensorPin);
  delay(20);
  float InputMillivolts = (PinInput / 1023.0) * 5000;                                                        //1023 is resolution of ADC, 5V is applied voltage,
  float SensorReading = InputMillivolts / 10;                                                                //for 1 degree change in temp, 10mV difference in output
  char Buffer[10]; 
  SensorString = dtostrf(SensorReading, 5, 2, Buffer);                                                       //decimal to string conversion, dtostrf(float_val, width. precision,buffer)
                                                                                         
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(LcdDescription);
  lcd.print(":");
  lcd.print(SensorReading);
  Serial.print(SerialDescription);
  Serial.print(":");
  Serial.println(SensorReading);
  
  return SensorString;
}

String getDigitalSensor(int SensorPin, String pinLowStatus, String pinHighStatus, String lcdDescription, String serialDescription)
{
  if (digitalRead(SensorPin) == LOW)
  {
    SensorStatus = pinLowStatus;
  }
  else
  {
    SensorStatus = pinHighStatus;
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(lcdDescription);
  lcd.print(":");
  lcd.setCursor(0, 1);
  lcd.print(SensorStatus);
  Serial.print(serialDescription);
  Serial.print(":");
  Serial.println(SensorStatus);
  return SensorStatus;

}

void loop()
{
  gsmMessage("AT", "Attention");                                                                             //Start writing commands for the gsm module (attention gsm module!)
  gsmMessage("ATE0", "Disabling echo");                                                                      //Ensure command sent to the modem is not echoed back
  gsmMessage("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "GPRS Connection");                                        //Create a connection of GPRS type
  gsmMessage("AT+SAPBR=3,1,\"APN\",\"INTERNET\"", "Setting APN");                                            //Set access point name as INTERNET
  gsmMessage("AT+SAPBR=1,1", "Opening GPRS");                                                                //Open the connection
  gsmMessage("AT+SAPBR=2,1", "Sending query");                                                               //Send sample query through connection
  gsmMessage("AT+HTTPINIT", "HTTP Initialize");                                                              //HTTP initialization
  gsmMessage("AT+HTTPPARA=\"CID\",1", "Setting HTTP");                                                       //Sets HTTP to use the connection
  
  url = "AT+HTTPPARA=\"URL\",\"52.24.234.121/EnterBrowserValues.php?Temperature=";                           //Forming URL
  
  TemperatureString = getAnalogSensor(TempPin, "Temp", "Getting Temperature");
  url.concat(TemperatureString);
  delay(500);
  
  url.concat("&Light=");
  LightString = getAnalogSensor(LightPin, "Light", "Light in Lux");
  url.concat(LightString);
  delay(500);
  
  url.concat("&PirStatus=");
  PirStatus = getDigitalSensor(PirPin, "NO", "YES", "Pir status", "PIR Status");
  url.concat(PirStatus);
  delay(500);
  
  url.concat("&Reed=");
  ReedSwitchStatus = getDigitalSensor(ReedPin, "OPEN", "CLOSED", "Reed status", "Reed Switch Status");
  url.concat(ReedSwitchStatus);
  delay(500);
  
  url.concat("&Power=");
  PowerSwitchStatus = getDigitalSensor(PowerPin, "OFF", "ON", "Power status", "Power Switch Status"); 
  url.concat(PowerSwitchStatus); 
  url.concat("\"");
  Serial.println(url);
  
  gsmMessage(url, "Sending URL", 2000);                                                                      //Send URL
  gsmMessage("AT+HTTPACTION=0", "GET Command", 10000);                                                       //Set GET command
  gsmMessage("AT+ HTTPTERM", "Terminate HTTP", 2000);                                                        //Terminate HTTP Connection
  gsmMessage("AT+SAPBR=0,1", "Close Connection", 2000);                                                      //Close carrier connection
}
