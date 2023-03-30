#include <Arduino.h>
#include <DHTesp.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

DHTesp dht;
BH1750 lightMeter;

#define LED_FLASH   4
#define LED_RED    16
#define LED_YELLOW 17
#define LED_GREEN  5
#define PIN_SW     0
#define PIN_DHT    13 
#define LED_BUILTIN_ON  0
#define LED_BUILTIN_OFF 1
#define LED_ON  1
#define LED_OFF 0
#define PIN_SDA 32
#define PIN_SCL 33
#define LED_COUNT 2

#define WIFI_SSID "eeBotak punya hp"
#define WIFI_PASSWORD "satuperxdx"
#define MQTT_BROKER  "broker.emqx.io"
#define MQTT_TOPIC_PUBLISH   "esp32_johevin/2502036262/data"
#define MQTT_TOPIC_SUBSCRIBE "esp32_johevin/2502036262/data"  

float globalHumidity = 0, globalTemp = 0, globalLux = 0;

Ticker timerPublish;
char g_szDeviceId[30];
Ticker timerMqtt;
WiFiClient espClient;
PubSubClient mqtt(espClient);
boolean mqttConnect();
void WifiConnect();

void sendLux()
{
  char szMsg[50];
  static int nMsgCount=0;
  sprintf(szMsg, "Current Lux Value: %.2f ", globalLux);
  mqtt.publish(MQTT_TOPIC_PUBLISH, szMsg);
}

void sendTemp()
{
  char szMsg[50];
  static int nMsgCount=0;
  sprintf(szMsg, "Current Temp Value: %.2f ", globalTemp);
  mqtt.publish(MQTT_TOPIC_PUBLISH, szMsg);
}

void sendHumidity()
{
  char szMsg[50];
  static int nMsgCount=0;
  sprintf(szMsg, "Current Humidity Value: %.2f ", globalHumidity);
  mqtt.publish(MQTT_TOPIC_PUBLISH, szMsg);
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();
}
boolean mqttConnect() {
  sprintf(g_szDeviceId, "esp32_%08X",(uint32_t)ESP.getEfuseMac());
  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(mqttCallback);
  Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);

  boolean fMqttConnected = false;
  for (int i=0; i<3 && !fMqttConnected; i++) {
    Serial.print("Connecting to mqtt broker...");
    fMqttConnected = mqtt.connect(g_szDeviceId);
    if (fMqttConnected == false) {
      Serial.print(" fail, rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }

  if (fMqttConnected)
  {
    Serial.println(" success");
    mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
    Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);
    sendLux();
    sendTemp();
    sendHumidity();
  }
  return mqtt.connected();
}

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}

// BOUNDARIES
int nCount=0;

void displayTempHumidity(float humidity, float temperature){
    Serial.printf("Temperature: %.2f C\n", temperature);
    Serial.printf("Humidity: %.2f %%\n", humidity);
    if(temperature > 28 && humidity >= 80) {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
    }
    else if(temperature > 28 && (humidity > 60 && humidity < 80)) {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_GREEN, LOW);
    }
    else if(temperature < 28 && humidity <= 60) {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, HIGH);
    }
    globalHumidity = humidity;
    globalTemp = temperature; 
}

void displayLight(float lux){
  Serial.printf("Light: %.2f lx\n", lux);
  if(lux > 400) Serial.println("pintu kebuka bos");
  else Serial.println("pintu ketutup bang");
    globalLux = lux;
}

void onReadDht(){
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  if (dht.getStatus()==DHTesp::ERROR_NONE){
    displayTempHumidity(humidity,temperature);
  }
}

void onReadLight(){
  float lux = lightMeter.readLightLevel();
  displayLight(lux);
}

void onReadSensors() {
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  onReadDht();
  onReadLight();
  
  vTaskDelay(100/portTICK_PERIOD_MS);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
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
    displayTempHumidity(humidity,temperature);

  }
  vTaskDelay(100/portTICK_PERIOD_MS);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
}

void Bh1750Trial() {
  pinMode(PIN_SW, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_SW), onResetCounter,FALLING);
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
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  WifiConnect();
  mqttConnect();
  timerPublish.attach_ms(4000, sendLux);
  timerPublish.attach_ms(5000, sendTemp);
  timerPublish.attach_ms(6000, sendHumidity);

  xTaskCreatePinnedToCore(executeBH,"BH1750",2048,NULL,1,NULL,0);
  xTaskCreatePinnedToCore(executeDHT,"DHT",2048,NULL,2,NULL,0);
}

void loop()
{
  mqtt.loop();
}
