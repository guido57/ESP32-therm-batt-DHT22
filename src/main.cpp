#include <Arduino.h>
#include <HTTPClient.h>
#include <DHT.h>

#define SUCCESSFUL_DEEP_SLEEP_SECS  60
#define UNSUCCESSFUL_DEEP_SLEEP_SECS  60 
#define NO_CONN_DEEP_SLEEP_SECS  10 

//#define Console myMQTTlog // you can use Console.printf(...)
#define Console Serial // you can use Console.printf(...)

//============================================
// Power supply measurement
//============================================
int analog_input_pin = 35; 

//============================================
// DHT22 Temperature And Humidity Sensor
//============================================
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
const int DHT22_power_pin = 17; // the +3.3 of DS18B20 is connected here
#define DHTPIN 18     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
DHT dht(DHTPIN, DHTTYPE);

float readTemperature(){
  int attempts = 10;
  for(int i=0; i< attempts;i++){
    float temp = dht.readTemperature();
    if(isnan(temp) || temp == -127.0){
    }else 
      return temp;

    delay(300);  
  }  
  return -127.0;
}

float readHumidity(){
  int attempts = 10;
  for(int i=0; i< attempts;i++){
    float hum = dht.readHumidity();
    if(false == isnan(hum))
      return hum;
    delay(300);  
  }
  return -1.0;
}

float readVoltage(){
  uint16_t adv = analogRead(analog_input_pin);
  return 1.0746*2*3.3*float(adv)/4095.0; // yes, a resistive divider by two is used  
}

int sendTempVoltageToThingSpeak(){
      // https://api.thingspeak.com/update?api_key=20HZ9WTUWMM9E5AL&field1=13.3&field2=48.2&field3=3.8
      
      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      String url = "https://api.thingspeak.com/update?api_key=20HZ9WTUWMM9E5AL";
      url += "&field1=" + String(readTemperature(),1);
      url += "&field2=" + String(readHumidity(),1);
      url += "&field3=" + String(readVoltage(),1);
      http.begin(url);
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      Console.printf("HTTP Response code: %d after %lu millis\r\n",httpResponseCode,millis());
      // Free resources
      http.end();
      return httpResponseCode;
}

void post_to_homeassistant(WiFiClient * client_p, String state, String unit_of_meas, String name, String friendly_name){

  HTTPClient http_p;
  // Send to Home Assistant via rest api
  http_p.addHeader("Authorization","Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiI0MTkxYmE2MWVjMTk0ZTU4OTIzZmZjYjY4ZDBiNzdmMSIsImlhdCI6MTY5OTcxOTUwMCwiZXhwIjoyMDE1MDc5NTAwfQ.Ep4FyHd6cdKJyCQLMfVeGEiTc38FUNTWS3P8LqoJAGg",false,true);
  http_p.addHeader("Content-Type","application/json",false,true);
  // String httpRequestData = "{\"state\":\"31.4\", \"attributes\": {\"unit_of_measurement\": \"%\", \"friendly_name\": \"Remote Input 1\"}}";           
  String httpRequestData = "{\"state\":\"" + state + 
                            "\",\"attributes\":{\"unit_of_measurement\":\"" + unit_of_meas + 
                            "\", \"friendly_name\":\"" + friendly_name +
                            "\"}}";           
  Serial.printf("httpRequestData is:%s\r\n",httpRequestData.c_str());
  http_p.begin(*client_p, "http://homeassistant.local:8123/api/states/" + name);     
  int httpCode = http_p.POST(httpRequestData);
  Serial.printf("http POST result=%d\r\n",httpCode);
  //http_p.end(); //Free the resources

}

void sendTempVoltageToHomeAssistant(){
      
      WiFiClient * client  = new WiFiClient;;
      post_to_homeassistant(client, String(readTemperature(),1),"°C","sensor.esp32_wifi_therm_batt_temp","ESP32 WiFi Garden Thermometer - Temperature");
      post_to_homeassistant(client, String(readHumidity(),1),"%","sensor.esp32_wifi_therm_batt_hum","ESP32 WiFi Garden Thermometer - Humidity");
      post_to_homeassistant(client, String(readVoltage(),1),"V","sensor.esp32_wifi_therm_batt_volt","ESP32 WiFi Garden Thermometer - Voltage");
      post_to_homeassistant(client, String(random(1000000)),"","sensor.esp32_wifi_therm_batt_random","ESP32 WiFi Garden Thermometer - random");

}

int sendTempVoltageToThermostat(){

      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      String url = "http://192.168.1.69:2048/";
      //String url = "http://"  + thermostat_IP.toString() + ":" + "3000" + "/";
      http.begin(url);
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");
      // Data to send with HTTP POST
      String data = "{\"temp\":\""         + String(readTemperature(),1)
                  + "\",\"humidity\":\""   + String(readHumidity(),1)
                  + "\",\"macAddress\":\"" + WiFi.macAddress() 
                  + "\",\"voltage\":\""    + String(readVoltage(),1) 
                  + "\"}";
      String httpRequestData = data;           
      // Send HTTP POST request
      Console.printf("Sending %s to url %s after %lu millis\r\n",data.c_str(),url.c_str(),millis());
      int httpResponseCode = http.POST(httpRequestData);
      Console.printf("HTTP Response code: %d after %lu millis\r\n",httpResponseCode,millis());
      // Free resources
      http.end();
      return httpResponseCode;
}

//  Go to deep sleep for sleep_time_seconds
void deep_sleep(int sleep_time_seconds){
  
  int final_message_id = Console.printf("goto sleep after %lu millis! Wake up in %d seconds\r\n", millis(), sleep_time_seconds);
  esp_sleep_enable_timer_wakeup(sleep_time_seconds * 1000000);
  //delay(1000); // let MQTT to send log

  Serial.printf("esp_deep_sleep_start after %lu millis! Wake up in %d seconds\r\n", 
           millis(), sleep_time_seconds);
  esp_deep_sleep_start();
};


//==============================================
void setup() {
       
    //   power supply DHT22 
    pinMode(DHT22_power_pin, OUTPUT);
    digitalWrite(DHT22_power_pin, HIGH);
    dht.begin();

    // setup serial port
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.printf("Setup started at %lu millis\r\n",millis()); 
    
    // begin connecting
    Serial.println("Trying to connect to sallynet...");
    WiFi.begin("sallynet", "sally866");
    WiFi.waitForConnectResult(10000);
    if(WiFi.isConnected()){
      Serial.printf("WiFi connected to sallynet at %lu millis\r\n",millis()); 
      Serial.print("Local IP is "); Serial.println(WiFi.localIP());
    }else{
       WiFi.persistent(false);
       WiFi.disconnect();
       WiFi.mode(WIFI_OFF);
       WiFi.mode(WIFI_STA);  
       Serial.println("Trying to connect to TIM-18373419...");
       WiFi.begin("TIM-18373419", "K9giYCTW4ryRS1MT26oIs7BG");
       WiFi.waitForConnectResult(10000);
       if(WiFi.isConnected()){
         Serial.printf("WiFi connected to TIM-18373419 at %lu millis\r\n",millis()); 
         Serial.print("Local IP is "); Serial.println(WiFi.localIP());
       }else {
         Console.printf("No WiFi connection-> goto deep sleep for %d seconds after %lu msecs \r\n", 
                      UNSUCCESSFUL_DEEP_SLEEP_SECS, millis());    
         deep_sleep(UNSUCCESSFUL_DEEP_SLEEP_SECS);
      }
    }  
}


// loop() variables
unsigned long last_send_temp_msecs = 0L; 
bool send_temp_OK = false;
unsigned long send_temp_interval_msecs = 60000L;
void loop()
{

  if(WiFi.isConnected()){

    // send Voltage and Temperature to the Thermostat every send_temp_interval_msecs   
    if(last_send_temp_msecs == 0L || millis() > last_send_temp_msecs + send_temp_interval_msecs)
    {
    
      // sendTempVoltageToThingSpeak();

      int http_resp = -1;
      http_resp = sendTempVoltageToThermostat();
      send_temp_OK = (http_resp == 200);
      Console.printf("Send_temp_OK=%d after %lu msecs\r\n", 
             send_temp_OK,millis());  

      sendTempVoltageToHomeAssistant();

      last_send_temp_msecs = millis();
    }

    if(send_temp_OK){
        Console.printf("Send_temp_OK -> goto deep sleep for %d seconds after %lu msecs \r\n", 
                      SUCCESSFUL_DEEP_SLEEP_SECS, millis());    
        deep_sleep(SUCCESSFUL_DEEP_SLEEP_SECS); 
    }else{
        Console.printf("Send_temp_KO -> goto deep sleep for %d seconds after %lu msecs \r\n", 
                      UNSUCCESSFUL_DEEP_SLEEP_SECS, millis());    
        deep_sleep(UNSUCCESSFUL_DEEP_SLEEP_SECS); 
    }

  
  }else{
      Console.printf("Send_temp_KO -> goto deep sleep for %d seconds after %lu msecs \r\n", 
                      UNSUCCESSFUL_DEEP_SLEEP_SECS, millis());    
      deep_sleep(UNSUCCESSFUL_DEEP_SLEEP_SECS); 
  }
}
