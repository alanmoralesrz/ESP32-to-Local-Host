#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "heartRate.h"
#include <Adafruit_MLX90614.h>

#include <WiFi.h>
#include <HTTPClient.h>

String URL = "http://computer_ip/project_folder/test_data.php";

const char* ssid = "INFINITUM625C";
const char* password = "Mput9dausK";

// variables
float temperature;
int32_t spo2;
int beatAvg;

//***********************SPO2 VARIABLES**************************//
MAX30105 particleSensor;
#define MAX_BRIGHTNESS 255
uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t bufferLength;
//int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

//***********************BPM SENSOR VARIABLES**************************//
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
//int beatAvg;

//***********************TEMP SENSOR VARIABLES**************************//
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); //Se declara una variable u objeto para el sensor  

unsigned long lastPrintTime = 0; 
const unsigned long printInterval = 20000;
int state = 3;

void setup()
{
  Serial.begin(115200);
  connectWiFi();

  // Inicializar el sensor de temperatura
  //Serial.println("Iniciando MLX90614...");
  if (!mlx.begin()) {
    Serial.println("MLX90614 not found");
    while (1);
  }

}

void loop()
{
  if(WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  String postData = "temperature=" + String(temperature) + "&spo2=" + String(spo2) + "&bpm=" + String(beatAvg);

  HTTPClient http;
  http.begin(URL);
  
  int httpCode = http.POST(postData);
  String payload = http.getString();
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  Serial.print("URL : "); Serial.println(URL); 
  Serial.print("Data: "); Serial.println(postData);
  Serial.print("httpCode: "); Serial.println(httpCode);
  Serial.print("payload : "); Serial.println(payload);
  Serial.println("--------------------------------------------------");
  delay(5000);



  switch (state)
  {
    case 1:
      initSPO2Sensor(); // Llama a la función para inicializar el sensor de temperatura
      SPO2_measure();
      break;
    
    case 2:
      BPM_measure();
      break;
      
    case 3:
      tempMeasure();    // Llama la función para medir la temperatura
      state = 1;
      break;
  }
}

//***********************SPO2 CODE****************************//

void SPO2_measure()
{
    bufferLength = 100;

    // Leer las primeras 100 muestras
    for (byte i = 0; i < bufferLength; i++){
      while (particleSensor.available() == false)
        particleSensor.check();
        
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
    }
    
    // Calcular frecuencia cardíaca y SpO2
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    // Mover las muestras y tomar nuevas
    for (byte i = 25; i < 100; i++) {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }
    
    for (byte i = 75; i < 100; i++) {
      while (particleSensor.available() == false)
        particleSensor.check();
        
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
    }

    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    
    if (validSPO2 == 0){
      Serial.println("Invalid value");
    }

    unsigned long currentTime = millis();
    if (currentTime - lastPrintTime >= printInterval) {
      lastPrintTime = currentTime;
      if (validSPO2 == 1) {
        Serial.print(F("SPO2="));
        Serial.println(spo2, DEC);
        state = 2;
      }
    }
}

void BPM_measure()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; 
      rateSpot %= RATE_SIZE; 

      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  if (irValue < 50000)
    Serial.println("Invalid value");
  
  unsigned long currentTime = millis();
  if (currentTime - lastPrintTime >= printInterval) {
    lastPrintTime = currentTime;
    if (irValue >= 50000) {
      Serial.print("BPM=");
      Serial.println(beatAvg);
      resetProgram();   // Reinicia el programa
    }
  }
}

//***********************TEMP SENSOR FUNCTIONS**************************//

void initSPO2Sensor() {
  /*
  // Finalizar el sensor anterior
  Serial.println("Apagando MLX...");
  mlx.shutDown();
  delay(3000);
  */

  //***********************BPM/SPO2 SENSOR INITIALIZATION**************************//
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)){
    Serial.println("MAX30105 not found");
    while (1);
  }   
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;
  byte sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);             
  particleSensor.setPulseAmplitudeRed(0x0A);  
  particleSensor.setPulseAmplitudeGreen(0);  

}

void tempMeasure() {
  // Medir temperatura
  //Serial.print("Temp. . .");
  delay(3000);
  Serial.print("Temperatura= "); 
  temperature = mlx.readObjectTempC() + 2;
  Serial.print(temperature); // Compensación de +2°C
  Serial.println(" °C"); 
  delay(3000);
}

void resetProgram() {
  Serial.println("Reiniciando");
  delay(1000);
  ESP.restart(); // Reinicia el ESP32
}

void connectWiFi() {
  WiFi.mode(WIFI_OFF);
  delay(1000);
  //This line hides the viewing of ESP as wifi hotspot
  WiFi.mode(WIFI_STA);
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
    
  Serial.print("connected to : "); Serial.println(ssid);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
}

