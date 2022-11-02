#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "donotuse"
#define STAPSK  "66666663"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

const int MPU = 0x68;
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
float gForceX, gForceY, gForceZ, rotX, rotY, rotZ, NowTemp;



void setup() 
{
  Serial.begin(115200);
  Wire.begin(0,2);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) 
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/inline", []() {
  server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Accelerometer: \t X \t Y \t Z  Gyroscope:\t X \t Y \t Z");
  pinMode(1,OUTPUT);

  /*
    test OK .It can use esp8266 rx and tx pin to be an IO output/input
  */
//  digitalWrite(1,HIGH);
//  delay(1000);
//  digitalWrite(1,LOW);
//  delay(5000);
//  digitalWrite(1,HIGH);
//  delay(1000);
//  digitalWrite(1,LOW);
//  delay(5000);
//  digitalWrite(1,HIGH);
}

void loop() 
{
  server.handleClient();
  MDNS.update();
  dataReceiver();
  debugFunction();
//  dataAcc();
//  dataGy();
  
}


//---------------------------------MPU6050
void dataReceiver()
{
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,14,1);  // request a total of 14 registers
  AcX = Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY = Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  NowTemp = Wire.read()<<8|Wire.read();
  GyX = Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY = Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ = Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  processData();
}
 
void processData()
{
  gForceX = AcX / 16384.0;
  gForceY = AcY / 16384.0; 
  gForceZ = AcZ / 16384.0;
  NowTemp = NowTemp / 100;
  rotX = GyX / 131.0;
  rotY = GyY / 131.0; 
  rotZ = GyZ / 131.0;
}
void debugFunction()
{
//  Serial.print("\t"); Serial.print(gForceX);
//  Serial.print("\t"); Serial.print(gForceY);
//  Serial.print("\t"); Serial.print(gForceZ);  
//  Serial.print("\t"); Serial.print(rotX);
//  Serial.print("\t"); Serial.print(rotY);
//  Serial.print("\t"); Serial.print(rotZ);
  Serial.print("\t"); Serial.println(NowTemp);
}
char* init(float val)
{
  
  char buff[100];
 
  for (int i = 0; i < 100; i++) {
      dtostrf(val, 4, 2, buff);  //4 is mininum width, 6 is precision
  }
   return buff;
 
}
void dataAcc()
{
 
  char mpu6050X[100]= "";   
  strcat(mpu6050X,init(gForceX));
 
  char mpu6050Y[100]= "";   
  strcat(mpu6050Y,init(gForceY));
 
  char mpu6050Z[100]= "";   
  strcat(mpu6050Z,init(gForceZ));
 
}
void dataGy()
{
 
  char mpu6050X[100]= "";
  strcat(mpu6050X,init(rotX));
 
  char mpu6050Y[100]= "";
  strcat(mpu6050Y,init(rotY));
 
  char mpu6050Z[100]= "";
  strcat(mpu6050Z,init(rotZ));
  
}

//---------------------------------http server
void handleRoot() 
{
  char temp[400];

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='0.5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>ACCEL: %02f    %02f    %02f</p>\
    <p>GYRO: %02f    %02f    %02f</p>\
    <p>Temp: %02f</p>\
  </body>\
</html>",

           gForceX , gForceY , gForceZ , rotX , rotY , rotZ , NowTemp
          );
  server.send(200, "text/html", temp);
}

void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}
