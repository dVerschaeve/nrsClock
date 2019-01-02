#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ArduinoJson.h>


// WiFi
#define CONFIG_WIFI_SSID ""
#define CONFIG_WIFI_PASS ""

// MQTT
#define CONFIG_MQTT_HOST ""
#define CONFIG_MQTT_PORT 1883
#define CONFIG_MQTT_USER ""
#define CONFIG_MQTT_PASS ""
#define CONFIG_MQTT_CLIENT_ID "nrsClock" 

// LED Setup
#define NUM_LEDS 12
#define COLOR_ORDER GRB
#define DATA_PIN D4

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "stat/nrsClock/LED"
#define CONFIG_MQTT_TOPIC_SET "cmnd/nrsClock/LED"

#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

//Define VAriables
bool booted = true;
bool changeMode = false;
bool stateOn = false;
int brightness = 255;
bool countDownDone = false;
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);


//https://www.tweaking4all.nl/hardware/arduino/adruino-led-strip-effecten/

// Define the array of leds
CRGB leds[NUM_LEDS];



WiFiClient espClient;
PubSubClient client(espClient);

int REDVALUE = 0;
int GREENVALUE = 0;
int BLUEVALUE = 0;
String EFFECT = "color";



void setup() {
  
  Serial.begin(9600); 
  delay(3000); // sanity delay
  setup_wifi();

  //MQTT Setup
  client.setServer(CONFIG_MQTT_HOST, CONFIG_MQTT_PORT);
  client.setCallback(callback);

  //LED Setup
  FastLED.addLeds<WS2813, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {

  if (!client.connected()) {
    Serial.println("not connected");
    reconnect();
  }

  client.loop();

  if(booted == true){
    stateOn = false;
    setAll(0,0,0);
    sendOffCommand();
    booted = false;
  }
  //Serial.print("ChangeMode: ");
  //Serial.println(changeMode);
  //Serial.print("Effect: ");
  //Serial.println(EFFECT);
  if(changeMode == true){
      
      if(EFFECT == "color"){

        Serial.println("Setting strip to color mode");
        Serial.print("Red: ");
        Serial.println(REDVALUE);
        Serial.print("Green: ");
        Serial.println(GREENVALUE);
        Serial.print("Blue: ");
        Serial.println(BLUEVALUE);
        
        setAll(REDVALUE,GREENVALUE,BLUEVALUE);
        
        
      }

      if(stateOn == false){
        setAll(0,0,0);
      }
      
      changeMode = false;
  }

  if(stateOn && EFFECT == "CountDown"){
    Serial.println("Starting Countdown Animation");
    countDown();  
  }

  if(stateOn && EFFECT == "meteorRain"){
    Serial.println("Starting meteorRain Animation");
    meteorRain(0xff,0xff,0xff,10, 64, true, 30);
  }

  if(stateOn && EFFECT == "SnowSparkle"){
    Serial.println("STarting SnowSparkle Animation");
    SnowSparkle(0x10, 0x10, 0x10, 20, random(100,1000));
  }

  if(stateOn && EFFECT == "CylonBounce"){
    Serial.println("STarting Cylon Animation");
    CylonBounce(0xff, 0, 0, 2, 200, 200);
  }

  if(stateOn && EFFECT == "Strobe"){
    Serial.println("Starting Strobe Animation");
    Strobe(0xff, 0xff, 0xff, 10, 50, 1000);  
  }
  

  
  // put your main code here, to run repeatedly:
  //Serial.println("Setting all colors to RED");
  //setAll(0,0,255);
  //Serial.println("Begin CountDown");
  //countDown();
  //Twinkle(0xff, 0, 0, 10, 100, false);
  // Sparkle(0xff, 0xff, 0xff, 0);
  //SnowSparkle(0x10, 0x10, 0x10, 20, random(100,1000));
  //meteorRain(0xff,0xff,0xff,10, 64, true, 30);
}


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(CONFIG_WIFI_SSID);

  WiFi.mode(WIFI_STA); // Disable the built-in WiFi access point.
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();


  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  
    if (!processJson(message)) {
      return;
    }
    Serial.println("Sending state to MQTT Server");
    sendState();

}

bool processJson(char* message){
      StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }


  if (root.containsKey("state")) {

    if (strcmp(root["state"], CONFIG_MQTT_PAYLOAD_ON) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], CONFIG_MQTT_PAYLOAD_OFF) == 0) {
      stateOn = false;
      Serial.println("Setting stateOn to false");
      changeMode = true;
      EFFECT = "color";
    }
  }

  if (root.containsKey("brightness")){
    brightness = root["brightness"];
  } 

  if (root.containsKey("effect")){
    EFFECT = root["effect"].as<String>();
    changeMode = false;
  } else if (root.containsKey("color")) {
      REDVALUE = root["color"]["r"];
      GREENVALUE = root["color"]["g"];
      BLUEVALUE = root["color"]["b"];
      EFFECT = "color";
      changeMode = true;
  } else {
    EFFECT = "color";
    changeMode = true;
    if(stateOn == true && REDVALUE == 0 && GREENVALUE == 0 & BLUEVALUE == 0){
        REDVALUE = 255;
        GREENVALUE = 255;
        BLUEVALUE = 255;
      }
  }
  
  return true;
}

void sendState() {
  Serial.print("Sending State:");
  Serial.println(stateOn);
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF;

  if(EFFECT == "color"){
    JsonObject& color = root.createNestedObject("color");
    color["r"] = REDVALUE;
    color["g"] = GREENVALUE;
    color["b"] = BLUEVALUE;  
  } else {
    root["effect"] = EFFECT;
  }
  
  
  //root["brightness"] = brightness;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(CONFIG_MQTT_TOPIC_STATE, buffer, true);
}

void sendOffCommand() {
  Serial.print("Sending State:");
  Serial.print(stateOn);
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF;
  
  JsonObject& color = root.createNestedObject("color");
  color["r"] = REDVALUE;
  color["g"] = GREENVALUE;
  color["b"] = BLUEVALUE;
  
  root["brightness"] = brightness;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(CONFIG_MQTT_TOPIC_SET, buffer, true);
}


void sendSnowSparkleCommand() {
  Serial.print("Sending State:");
  Serial.print(stateOn);
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = CONFIG_MQTT_PAYLOAD_ON;
  root["effect"] = "SnowSparkle";

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(CONFIG_MQTT_TOPIC_SET, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASS)) {
      Serial.println("connected");
      client.subscribe(CONFIG_MQTT_TOPIC_SET);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//FrameWork Definitions
void showStrip() {
   FastLED.setBrightness(brightness);
   FastLED.show();
   
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
   leds[Pixel].r = red;
   leds[Pixel].g = green;
   leds[Pixel].b = blue;
}

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue); 
  }
  showStrip();
}

//Sparkle
void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = random(NUM_LEDS);
  setPixel(Pixel,red,green,blue);
  showStrip();
  delay(SpeedDelay);
  setPixel(Pixel,0,0,0);
}

//SnowSparkle
void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  setAll(red,green,blue);
  
  int Pixel = random(NUM_LEDS);
  setPixel(Pixel,0xff,0xff,0xff);
  showStrip();
  delay(SparkleDelay);
  setPixel(Pixel,red,green,blue);
  showStrip();
  delay(SpeedDelay);
}

//Twinkle
void Twinkle(byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0,0,0);
  
  for (int i=0; i<Count; i++) {
     setPixel(random(NUM_LEDS),red,green,blue);
     showStrip();
     delay(SpeedDelay);
     if(OnlyOne) { 
       setAll(0,0,0); 
     }
   }
  
  delay(SpeedDelay);
}

void Strobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause){
  for(int j = 0; j < StrobeCount; j++) {
    setAll(red,green,blue);
    showStrip();
    delay(FlashDelay);
    setAll(0,0,0);
    showStrip();
    delay(FlashDelay);
  }
 
 delay(EndPause);
}

//Meteor
void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
  setAll(0,0,0);
  
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
    
    
    // fade brightness all LEDs one step
    for(int j=0; j<NUM_LEDS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
    
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
        setPixel(i-j, red, green, blue);
      } 
    }
   
    showStrip();
    delay(SpeedDelay);
  }
}


void fadeToBlack(int ledNo, byte fadeValue) {
 #ifdef ADAFRUIT_NEOPIXEL_H 
    // NeoPixel
    uint32_t oldColor;
    uint8_t r, g, b;
    int value;
    
    oldColor = strip.getPixelColor(ledNo);
    r = (oldColor & 0x00ff0000UL) >> 16;
    g = (oldColor & 0x0000ff00UL) >> 8;
    b = (oldColor & 0x000000ffUL);

    r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
    
    strip.setPixelColor(ledNo, r,g,b);
 #endif
 #ifndef ADAFRUIT_NEOPIXEL_H
   // FastLED
   leds[ledNo].fadeToBlackBy( fadeValue );
 #endif  
}

void countDown(){
  int timer = 1000;
  countDownDone = false;
  setAll(0,0,255);
  int currentStep = NUM_LEDS;
    for (int i=0; i <= 122; i++){
       Serial.print("Current i:");
       Serial.println(i); 
       
       if(i < 108){
        
        switch(i){
            case 10:  setPixel(NUM_LEDS - 1, 255, 0, 0); showStrip(); Serial.println("Setting pixel 0 to Red"); break;
            case 20:  setPixel(NUM_LEDS - 2, 255, 0, 0); showStrip(); Serial.println("Setting pixel 1 to Red"); break;
            case 30:  setPixel(NUM_LEDS - 3, 255, 0, 0); showStrip(); Serial.println("Setting pixel 2 to Red"); break;
            case 40:  setPixel(NUM_LEDS - 4, 255, 0, 0); showStrip(); Serial.println("Setting pixel 3 to Red"); break;
            case 50:  setPixel(NUM_LEDS - 5, 255, 0, 0); showStrip(); Serial.println("Setting pixel 4 to Red"); break;
            case 60:  setPixel(NUM_LEDS - 6, 255, 0, 0); showStrip(); Serial.println("Setting pixel 5 to Red"); break;
            case 70:  setPixel(NUM_LEDS - 7, 255, 0, 0); showStrip(); Serial.println("Setting pixel 6 to Red"); break;
            case 80:  setPixel(NUM_LEDS - 8, 255, 0, 0); showStrip(); Serial.println("Setting pixel 7 to Red"); break;
            case 90:  setPixel(NUM_LEDS - 9, 255, 0, 0); showStrip(); Serial.println("Setting pixel 8 to Red"); break;
            case 100:  setPixel(NUM_LEDS - 10, 255, 0, 0); showStrip(); Serial.println("Setting pixel 9 to Red"); break;
          }
          delay(timer);
       }

       if(i == 108){
          setAll(255,0,0);
        }

       if(i > 108){
         delay(timer);
         Serial.print("CurrentStep:");
         Serial.println(currentStep);
         setPixel(currentStep, 0, 255,0);
         showStrip();
         currentStep = currentStep - 1;
  
         if(currentStep == -1){
            currentStep = NUM_LEDS;
          }
       }
       
    }  
    
    Strobe(0xff, 0xff, 0xff, 10, 50, 1000);
    meteorRain(0xff,0xff,0xff,10, 64, true, 30);
    
    if (!client.connected()) {
      Serial.println("not connected");
      reconnect();
    }
    sendSnowSparkleCommand();
    EFFECT = "SnowSparkle";
    countDownDone = true;
}

//Cylon
void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){

  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }

  delay(ReturnDelay);

  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }
  
  delay(ReturnDelay);
}
