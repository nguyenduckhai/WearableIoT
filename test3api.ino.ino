// Use this code(3rd) to predict data
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LSM9DS0.h>
#include <Adafruit_Sensor.h> 

#include <Metro.h> 

// Create mutil tasking
// 100ms for collecting data
Metro blink1Metro = Metro(100);
// 5 seconds for sending to server 
Metro blink2Metro = Metro(5000);
// 5 seconds for predicting activities
//Metro blink3Metro = Metro(5000);


// Enter  Wifi 
//const char* ssid     = "New Riseplus Wifi";
//const char* password = "freewifi";

int i = 0;

String AllJsonBody; 
String AllJsonGBody; 

// i2c
Adafruit_LSM9DS0 lsm = Adafruit_LSM9DS0();

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;

void setupSensor()
{
  // 1.) Set the accelerometer range
  lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_2G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_4G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_6G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_8G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_16G);
  
  // 2.) Set the magnetometer sensitivity
  lsm.setupMag(lsm.LSM9DS0_MAGGAIN_2GAUSS);
  //lsm.setupMag(lsm.LSM9DS0_MAGGAIN_4GAUSS);
  //lsm.setupMag(lsm.LSM9DS0_MAGGAIN_8GAUSS);
  //lsm.setupMag(lsm.LSM9DS0_MAGGAIN_12GAUSS);

  // 3.) Setup the gyroscope
  lsm.setupGyro(lsm.LSM9DS0_GYROSCALE_245DPS);
  //lsm.setupGyro(lsm.LSM9DS0_GYROSCALE_500DPS);
  //lsm.setupGyro(lsm.LSM9DS0_GYROSCALE_2000DPS);
}

//----------------Function to create data buffer 
String putAcDatainBuffer(String formattedDate){
  String JsonBody;
  if(i == 0){
     JsonBody = get1stJsonBody((String)lsm.accelData.x,(String)lsm.accelData.y,(String)lsm.accelData.z,formattedDate);
  }else{ 
     JsonBody = getJsonBody((String)lsm.accelData.x,(String)lsm.accelData.y,(String)lsm.accelData.z,formattedDate);
  }
  return JsonBody;             
}


String putGDatainBuffer(String formattedDate){
  String JsonGBody;
  if(i == 0){
     JsonGBody = get1stJsonBody((String)lsm.gyroData.x,(String)lsm.gyroData.y,(String)lsm.gyroData.z,formattedDate);
  }else{ 
     JsonGBody = getJsonBody((String)lsm.gyroData.x,(String)lsm.gyroData.y,(String)lsm.gyroData.z,formattedDate);
  }
  return JsonGBody;             
}
//-----------------Function to reset data buffer 
void reset(){
    i = 0;
    AllJsonBody = "";
    AllJsonGBody = "";
    Serial.println("\n--------HAVE SENDED----------");
}

//-----------------Function to create JsonBody
String getJsonBody(String acc_x,String acc_y,String acc_z,String timest){
  String JsonBody = "\n,{   \n  \"wearable_label_id\":\"1\",\n  \"x_value\":\""+acc_x+"\",\n  \"y_value\":\""+acc_y+"\",\n  \"z_value\":\""+acc_z+"\",\n  \"timest\":\""+timest+"\"}";
  return JsonBody;
}

String get1stJsonBody(String acc_x,String acc_y,String acc_z,String timest){
  String JsonBody = "\n{   \n  \"wearable_label_id\":\"1\",\n  \"x_value\":\""+acc_x+"\",\n  \"y_value\":\""+acc_y+"\",\n  \"z_value\":\""+acc_z+"\",\n  \"timest\":\""+timest+"\"}";
  return JsonBody;
}


void setup() {
    // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { // CHeck for connection
    Serial.print("Connecting to Wifi");
  }
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
#ifndef ESP8266
  while (!Serial);     // will pause Zero, Leonardo, etc until serial console opens
#endif
  Serial.println("LSM raw read demo");
  if (!lsm.begin()){
    Serial.println("Oops ... unable to initialize the LSM9DS0. Check your wiring!");
    while (1);
  }
  Serial.println("Found LSM9DS0 9DOF");
  Serial.println("");
  Serial.println("");

  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);
}

void loop() {
  // put your main code here, to run repeatedly:
   if(WiFi.status()== WL_CONNECTED){   
    String Gyro_url = "http://192.168.10.104:3000/gyroscopes";
    String Accel_url = "http://192.168.10.104:3000/accelerometers";
    String Predic_url = "http://192.168.10.104:3000/prediction";
    lsm.read();
    while(!timeClient.update()) {
      timeClient.forceUpdate();
    }
    formattedDate = timeClient.getFormattedDate();
  
    if(blink1Metro.check()){
       Serial.print(".");
       String JsonGBody = putGDatainBuffer(formattedDate);
       String JsonBody = putAcDatainBuffer(formattedDate);
       AllJsonBody += JsonBody;
       AllJsonGBody += JsonGBody;
       ++i;
     }
    
     if(blink2Metro.check()){
       sendGyroData(Gyro_url,AllJsonGBody);
       sendAccelData(Accel_url,AllJsonBody);
       reset();
     }

//     if(blink3Metro.check()){
//       getPredicData(Predic_url);
//     }
     
  }else{
    Serial.println("Error in WiFi connection");   
  }
}

//------------------Function to call method----------
void sendAccelData(String url,String AllJsonBody){
  HTTPClient http;   
  http.begin(url);     //Specify destination for HTTP request
  http.addHeader("Content-Type", "application/json");             //Specify content-type header

  int httpResponseCode = http.POST("{\"accelerometers\":["+ AllJsonBody +"\n]}");
  if(httpResponseCode>0){
 
      String response = http.getString();                       //Get the response to the request
      Serial.println(httpResponseCode);   //Print return code/
//      Serial.println(response);           //Print request answer
      
    }else{

      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
   http.end();
}

void sendGyroData(String url,String AllJsonGBody){
  HTTPClient http;   
  http.begin(url);     //Specify destination for HTTP request
  http.addHeader("Content-Type", "application/json");             //Specify content-type header

  int httpResponseCode = http.POST("{\"gyroscopes\":["+ AllJsonGBody +"\n]}");
  if(httpResponseCode>0){
 
      String response = http.getString();                       //Get the response to the request
      Serial.println(httpResponseCode);   //Print return code/
//      Serial.println(response);           //Print request answer
      
    }else{

      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
   http.end();
}

void getPredicData(String url){
    HTTPClient http;   
    http.begin(url);
    int httpResponseCode = http.GET(); 
    if (httpResponseCode > 0) {
//      String payload = http.getString();   //Get the request response payload
      Serial.println(httpResponseCode);                     //Print the response payload
    }else{
      Serial.print("Error on getting GET: ");
      Serial.println(httpResponseCode);
    }
    http.end();
}
