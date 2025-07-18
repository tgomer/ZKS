// The ZKS project with
// esp32 by Espressif Systems 2.0.14 -> 3.0.0 - aplha 1 will not work
// Adafruit NeoPixel 1.11.0
// ESP32Time by fbiego 2.0.4
// ESP32TimerInterrupt by Khoi Hoang 2.3.0
// Board: ESP32 Dev Module
// developed by Tobias Gomer

// Sensefactor K definition:
// U(50kN) = 2.5V * 923 * K = 2.3075 * K [V]  
// Prototype: K = 0.99268
// 2,2906091
// U[50kN] = 2365mV = K*2,3825 [V]
// U(F) = K*2,3825/50000 * F
// Assuming U(0) = 0 -> F(U) = a*U

// two RGB LEDs 
// Pixel 0 (middle of the board) BT connection  state:
#define BT_LED 0
//         Connected: Blue continuous
#define BT_CONNECTED 0
#define BT_CONNECTED_COLOR Color(0,0,255)
//         Data transfer: Blue binking
//         Data error: Red blinking (no idea how to detect this)
#define BT_ERRORCONNECTED 1
#define BT_ERRORCONNECTED_COLOR Color(255,0,0)
//         Not connected: Yellow blinking
#define BT_NOTCONNECTED 2
#define BT_NOTCONNECTED_COLOR Color(255,127,0)
#define LED_BLINK_NONE 0
#define LED_BLINK 1
#define LED_BLINK_SLOW 2

int bt_led = BT_NOTCONNECTED;
int bt_blink = LED_BLINK_NONE;

// Pixel 1 Power 
#define AKKU_LED 1
//         Akku full: Green
#define AKKU_FULL 0
#define AKKU_FULL_COLOR Color(0,255,0)
//         Akku half: Yellow 
#define AKKU_HALF 1
#define AKKU_HALF_COLOR Color(155,155,0)
//         Akku almost empty: Red
#define AKKU_ALMOST_EMPTY 2
#define AKKU_ALMOST_EMPTY_COLOR Color(255,0,0)
//         Akku almost empty: Red blinking
#define AKKU_EMPTY 3
#define AKKU_EMPTY_COLOR Color(255,0,0)
//         Loading: Yellow/Green
#define AKKU_LOADING 4
#define AKKU_LOADING_COLOR Color(155,155,0)
//         Loading: Yellow/Green
#define AKKU_LOADING_COMPLETED 5
#define AKKU_LOADING_COMPLETED_COLOR Color(0,150,0)

#define PIXEL_BRIGHTNES 128
#define PIXEL_LOADINGBRIGHTNES 30

#define AKKU_FULL_VOLTAGE  3.55
#define AKKU_EMPTY_VOLTAGE 2.90

int akku_led = AKKU_FULL;
int akku_blink = LED_BLINK;


// ADC
// Connected to HSPI - Chip select PIN 15
// Channel 0: Akku voltage
// Channel 1: Sensor voltage
// Channel 2: Reference voltage (Zero)
// During startup the force will be set to zero.
// AMC = MCLK OSR= 24576  0010 1100

#define POWERSUPPLYDIVIDER 10.0

// Sensor electronics
// Uv = 2.5V, Gain 923 -> without load: 30mV
// sensitivity: 0,99268 mV/V FS (50kN)
// 45,81 µV/N  or 449,4 µV / kg
// drift 458  µV/K
// Forcefactor default: 2164 bei .99268
// F = U*Forcefactor
// Forcefactor = 2148 / senseFactor

#define SWVersion "1.1"
#define HWVersion "4.0"

#define CFG_SENSFACTOR 1.0
#define CFG_PIN 929555
#define CFG_TIMEOUT 3600 
#define CFG_SERIALNO "4XXX"
#define CFG_DEVICENAME "DYNAFORCE"
#define CFG_LIMIT1 5000
#define CFG_LIMIT2 3000
#define CFG_SERCICELIMIT 300
#define CFG_PULLINGMINKG 500
#define CFG_PULLINGMINSEC 5

#include <BluetoothSerial.h>
#include <esp_bt.h>

#include <ESP32Time.h>
#include <ESP32TimerInterrupt.h>

ESP32Timer ITimer0(0);
ESP32Timer ITimer1(1);

//ESP32Time rtc;
ESP32Time rtc(0);

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
// The RGB LEDs connected to Pin 4
#define PIN        4 
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 2 
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// Charge control
#define CHARGEPIN 16
#define STANDBYPIN 17

#define FULLLOADEDTIME 1800        /* Time for begin blinking after voltage > 3.55 in seconds */ 

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */

// Shutdown control
#define SHUTDOWNPIN 18

#include <Preferences.h>

#include "ZKSConfig.h"
#include "ADC.h"
#include "cli.h"

BluetoothSerial SerialBT;
ZKSConfig cConfig;
ADC cAdc;

#define TIMER0_INTERVAL_MS 500
#define TIMER1_INTERVAL_MS 10000   // Check Akku Level

#define DATA_TIMEOUT_MS 3000
#define DATA_TIMEOUT_COUNT (DATA_TIMEOUT_MS/TIMER0_INTERVAL_MS)

volatile int updatePixels = 0; // Zugriff im Timer0-Interrupt (atomar)
volatile int dataTimeout = 0;  // Zugriff im Timer0-Interrupt (atomar)
volatile bool do_it = false;   // Zugriff im Timer0-Interrupt (atomar)
volatile bool check_akkulevel = false; // Zugriff im Timer1-Interrupt (atomar)

double akkuVoltage = 0.0;
double startVoltage = 0.0;   // The voltage at startup time considered to be zero
                             // assuming to be 0.03 when the sensor seems to be   
                             // under tension
double currentForce = 0.0;
double maxForcePerPull = 0.0;
int akku = 0;
unsigned long pullstart = 0;
unsigned long fullloadedsince = 0;
int currentPull = 0;
bool limit1topped = false;
bool limit2topped = false;
unsigned long lastaction;
/*
struct {
  double startVoltage = 0.0;   // The voltage at startup time considered to be zero
} pull;
*/
bool IRAM_ATTR TimerHandler0(void * timerNo)
{
  do_it = true;
  updatePixels++;
  if (updatePixels == 4)
    updatePixels = 0;
  if (dataTimeout) dataTimeout--;
	return true;
}

bool IRAM_ATTR TimerHandler1(void * timerNo)
{
  check_akkulevel = true;
	return true;
}

void setup() {
/*  digitalWrite(SHUTDOWNPIN, HIGH);
  pinMode(SHUTDOWNPIN, OUTPUT);
  digitalWrite(SHUTDOWNPIN, HIGH); */

  Serial.begin(115200);
  cConfig.begin();
  cAdc.begin();
  for (int i=0; i<10; i++)
    startVoltage += cAdc.getValue(1, true);
  startVoltage = startVoltage/10;
  Serial.print("startVoltage:");
  Serial.println(startVoltage);
  if (startVoltage > 0.08){
    Serial.print("startVoltage corrected to 0.03");
    startVoltage = 0.03;
  }
  SerialBT.begin(cConfig.getDevicename() + "_" + cConfig.getSerialno()); //Name des ESP32
  esp_bredr_tx_power_set(ESP_PWR_LVL_P9,ESP_PWR_LVL_P9);
  Serial.println("ESP32 ready. Bluetooth initialized.");
  pixels.begin();
  pixels.clear(); // Set all pixel colors to 'off'

  if (ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, TimerHandler0))
	{
		Serial.print(F("Starting  ITimer0 OK, millis() = "));
		Serial.println(millis());
	}
	else
		Serial.println(F("Can't set ITimer0. Select another freq. or timer"));
  
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS * 1000, TimerHandler1))
	{
		Serial.print(F("Starting  ITimer1 OK, millis() = "));
		Serial.println(millis());
	}
	else
		Serial.println(F("Can't set ITimer1. Select another freq. or timer"));


  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    pixels.setPixelColor(i, pixels.Color(0, 150, 0));
    pixels.show();   // Send the updated pixel colors to the hardware.
  }
  pixels.show();
  if (SerialBT.connected()) {
    pixels.setPixelColor(BT_LED, pixels.BT_CONNECTED_COLOR);
  }
  else
    pixels.setPixelColor(BT_LED, pixels.BT_NOTCONNECTED_COLOR);  
  pixels.show();
  //akku = getAkkuLevel(cAdc.getValue(0) * POWERSUPPLYDIVIDER);
  //Serial.print("AKKU:");
  //Serial.println(cAdc.getValue(0) * POWERSUPPLYDIVIDER);
  pinMode(CHARGEPIN, INPUT);
  pinMode(STANDBYPIN, INPUT);
  int val = digitalRead(CHARGEPIN); 
  Serial.print("CHARGEPIN:");
  Serial.println(val);
  val = digitalRead(STANDBYPIN); 
  Serial.print("STANDBYPIN:");
  Serial.println(val);
  Serial.println(cConfig.setTime(""));
  lastaction = millis();
  delay(500);
  akkuVoltage = cAdc.getValue(0) * POWERSUPPLYDIVIDER;
  akku = getAkkuLevel(akkuVoltage);
  Serial.print("AKKU:");
  Serial.println(akkuVoltage);
}

void updatePixelsFunc(){
  /*if (isCharging()){
    esp_bredr_tx_power_set(ESP_PWR_LVL_N12,ESP_PWR_LVL_N12);
    pixels.setPixelColor(AKKU_LED, pixels.Color(0,0,0));
    pixels.setPixelColor(BT_LED, pixels.Color(0,0,0));
    return;
  } */
  if (akku_led == AKKU_FULL)
      pixels.setPixelColor(AKKU_LED, pixels.AKKU_FULL_COLOR);
  if (akku_led == AKKU_HALF)
      pixels.setPixelColor(AKKU_LED, pixels.AKKU_HALF_COLOR);
  if (akku_led == AKKU_ALMOST_EMPTY)
      pixels.setPixelColor(AKKU_LED, pixels.AKKU_ALMOST_EMPTY_COLOR);
  if (akku_led == AKKU_EMPTY)
      pixels.setPixelColor(AKKU_LED, pixels.AKKU_EMPTY_COLOR);
  if (akku_led == AKKU_LOADING)
      pixels.setPixelColor(AKKU_LED, pixels.AKKU_LOADING_COLOR);
  if (akku_blink == LED_BLINK && (updatePixels == 1 || updatePixels == 3))
    pixels.setPixelColor(AKKU_LED, pixels.Color(0,0,0));
  if (akku_blink == LED_BLINK_SLOW && (updatePixels == 2 || updatePixels == 3))
    pixels.setPixelColor(AKKU_LED, pixels.Color(0,0,0));
  if (bt_led == BT_ERRORCONNECTED)
      pixels.setPixelColor(BT_LED, pixels.BT_ERRORCONNECTED_COLOR);
  if (bt_led == BT_CONNECTED)
      pixels.setPixelColor(BT_LED, pixels.BT_CONNECTED_COLOR);
  if (bt_led == BT_NOTCONNECTED)
      pixels.setPixelColor(BT_LED, pixels.BT_NOTCONNECTED_COLOR);
  if (bt_blink == LED_BLINK && (updatePixels == 1 || updatePixels == 3))
    pixels.setPixelColor(BT_LED, pixels.Color(0,0,0));
  if (bt_blink == LED_BLINK_SLOW && (updatePixels == 2 || updatePixels == 3))
    pixels.setPixelColor(BT_LED, pixels.Color(0,0,0));
  if (isCharging() || isStandby()){
    pixels.setBrightness(PIXEL_LOADINGBRIGHTNES);
//    esp_bredr_tx_power_set(ESP_PWR_LVL_N12,ESP_PWR_LVL_N12);
  } else {
    pixels.setBrightness(PIXEL_BRIGHTNES);
//    esp_bredr_tx_power_set(ESP_PWR_LVL_P9,ESP_PWR_LVL_P9);
  }
  if (isCharging()){
      if( akku_led == AKKU_LOADING_COMPLETED ){
        pixels.setPixelColor(AKKU_LED, pixels.AKKU_LOADING_COMPLETED_COLOR);
      }
      else{
        pixels.setPixelColor(AKKU_LED, pixels.AKKU_HALF_COLOR);
        if (updatePixels == 1 || updatePixels == 3)
          pixels.setPixelColor(AKKU_LED, pixels.Color(0,0,0));
      }
      pixels.setPixelColor(BT_LED, pixels.Color(0,0,0));
  }
  pixels.show();
};

bool cliresult = true;
  
bool cli_execute(String command, Stream & device);

void loop() {
  if (Serial.available()) {
    // Hinweis: Timeout 1s
    String val = Serial.readStringUntil('\r');
    val.trim();
    cliresult = cli_execute(val, Serial);
    if (cliresult) dataTimeout = DATA_TIMEOUT_COUNT;
  }

  if (SerialBT.available()) {
    //pixels.setPixelColor(BT_LED, pixels.Color(0,0,0));
    //pixels.show();
    // Hinweis: Timeout 1s
    String val = SerialBT.readStringUntil('\r');
    val.trim();
    cliresult = cli_execute(val, SerialBT);
    if (cliresult) dataTimeout = DATA_TIMEOUT_COUNT;
  }

  if (SerialBT.connected()){
    if (!cliresult){
      bt_led = BT_ERRORCONNECTED;
      bt_blink = LED_BLINK;
    }
    else
    {
      bt_led = BT_CONNECTED;
      if (dataTimeout){
        bt_blink = LED_BLINK;
      }
      else{
        bt_blink = LED_BLINK_NONE;
      }        
    }
  }
  else {
    bt_led = BT_NOTCONNECTED;
    bt_blink = LED_BLINK_NONE;
  }

  if (do_it){
    updatePixelsFunc();
    do_it = false;
  }

  if (check_akkulevel){
    check_akkulevel = false;
    akkuVoltage = cAdc.getValue(0) * POWERSUPPLYDIVIDER;
    akku = getAkkuLevel(akkuVoltage);
    if (akku > 5){
      akku_led = AKKU_FULL;
      akku_blink = LED_BLINK_NONE;
    } else if (akku > 2){
      akku_led = AKKU_HALF;
      akku_blink = LED_BLINK_NONE;
    } else if (akku == 2){
      akku_led = AKKU_ALMOST_EMPTY;
      akku_blink = LED_BLINK_NONE;
    } else if (akku == 1){
      akku_led = AKKU_ALMOST_EMPTY;
      akku_blink = LED_BLINK_SLOW;
    } else{
      akku_led = AKKU_EMPTY;
      akku_blink = LED_BLINK;
      Serial.print("AKKU_EMPTY ");
    }
  }
  // Build mean Value ?
  currentForce = getForce();
  checkPullStatus();

  unsigned long idletime = (millis() - lastaction)/1000;
  if (!isCharging()){
    if (idletime > cConfig.getTimeout())
      shutdown();
    else if (idletime > cConfig.getTimeout() - 120)
      bt_blink = LED_BLINK;
    
    if ((akkuVoltage < AKKU_EMPTY_VOLTAGE) && (akkuVoltage > 0)){
      shutdown();
    }
  }
  else {
    if (akkuVoltage > AKKU_FULL_VOLTAGE){
      if (!fullloadedsince)
        fullloadedsince = millis();
      if ( (millis() - fullloadedsince) / 1000 > FULLLOADEDTIME){
        akku_led = AKKU_LOADING_COMPLETED;
        akku_blink = LED_BLINK;
      }
    }         
  }  
}

bool isStandby(){
  return !digitalRead(STANDBYPIN);   
}

bool isCharging(){
  return !digitalRead(CHARGEPIN);   
}
