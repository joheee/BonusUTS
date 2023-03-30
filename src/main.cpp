#include <Arduino.h>
#include <DHTesp.h>
#include <BH1750.h>

DHTesp dht;
BH1750 lightMeter;

#define LED_FLASH   4
#define LED_RED    15
#define LED_YELLOW 14
#define LED_GREEN  16
#define PIN_SW     0
#define PIN_DHT    13 

#define LED_BUILTIN_ON  0
#define LED_BUILTIN_OFF 1

#define LED_ON  1
#define LED_OFF 0

#define PIN_SDA 32
#define PIN_SCL 33

#define LED_COUNT 2
const uint8_t arLed[LED_COUNT] = {LED_RED, LED_GREEN};


int nCount=0;

void onReadDht(){
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  if (dht.getStatus()==DHTesp::ERROR_NONE){
    Serial.printf("Temperature: %.2f C\n", temperature);
    Serial.printf("Humidity: %.2f %%\n", humidity);
  }
}

void onReadLight(){
  float lux = lightMeter.readLightLevel();
  Serial.printf("Light: %.2f lx\n", lux);
}

void onReadSensors() {
  digitalWrite(arLed[nCount%LED_COUNT], LED_ON);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  onReadDht();
  onReadLight();
  
  vTaskDelay(100/portTICK_PERIOD_MS);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  digitalWrite(arLed[nCount%LED_COUNT], LED_OFF);
  nCount++;
}

IRAM_ATTR void onResetCounter() {
  nCount = 0;
}

void OnTimerReadDht(){
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  
  if (dht.getStatus()==DHTesp::ERROR_NONE){
    Serial.printf("Temperature: %.2f C\n", temperature);
    Serial.printf("Humidity: %.2f %%\n", humidity);
  }
  vTaskDelay(100/portTICK_PERIOD_MS);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
}

void Bh1750Trial() {
  for (uint8_t i=0; i<LED_COUNT; i++) pinMode(arLed[i], OUTPUT);
  
  pinMode(PIN_SW, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_SW), onResetCounter,FALLING); // extra checking
  dht.setup(PIN_DHT, DHTesp::DHT11);
  Wire.begin(PIN_SDA, PIN_SCL);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23,  &Wire);
}

void executeDHT(void *arg){
  for(;;){
    vTaskDelay(2*dht.getMinimumSamplingPeriod()/portTICK_PERIOD_MS);
    onReadSensors();
  }
}

void executeBH(void *arg){
  for(;;){
    vTaskDelay(1000/portTICK_PERIOD_MS);
    Bh1750Trial();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  xTaskCreatePinnedToCore(executeBH,"BH1750",2048,NULL,1,NULL,0);
  xTaskCreatePinnedToCore(executeDHT,"DHT",2048,NULL,2,NULL,0);
}

void loop() {

}