
#include "BLEDevice.h"
#include <driver/adc.h>

//#include "WiFi.h" 
//#include "driver/adc.h"
//#include <esp_wifi.h>
//#include <esp_bt.h>

//#include "BLEScan.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("09fc95c0-c111-11e3-9904-0002a5d5c51b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("11fac9e0-c111-11e3-9246-0002a5d5c51b");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
BLEClient*  pClient = NULL;
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds) */

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.println(myDevice->getAddress().toString().c_str());
    
    pClient  = BLEDevice::createClient();

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remote BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }


    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  //adc_power_on();
  //WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(35, false);
} // End of setup.

int read_humidity_sensor(){

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_11db);
  return adc1_get_raw(ADC1_CHANNEL_0);
  
}

void go_to_sleep(){

  //btStop();
  //adc_power_off();
  //esp_wifi_stop();
  //esp_bt_controller_disable();
  
  esp_deep_sleep_start();
  
}
// This is the Arduino main loop function.
void loop() {
  int counter = 0;
  while (counter < 5 ) {
      counter ++;
      
      // If the flag "doConnect" is true then we have scanned for and found the desired
      // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
      // connected we set the connected flag to be true.
      if (doConnect == true) {
        if (! connectToServer()) {
          go_to_sleep();
        } 
        doConnect = false;
      }
    
      // If we are connected to a peer BLE Server, update the characteristic each time we are reached
      // with the current time since boot.
      if (connected) {
           // Read the value of the characteristic.
          std::string humidity_string_value = pRemoteCharacteristic->readValue();
          int oldHumidityValue = atoi( humidity_string_value.c_str() );
          int newHumidityValue = read_humidity_sensor();
          if ( abs(oldHumidityValue - newHumidityValue) < 50){
            pClient -> disconnect();
            go_to_sleep();

          }
          
          // Set the characteristic's value to be the array of bytes that is actually a string.
          pRemoteCharacteristic->writeValue(String(newHumidityValue).c_str(), true);
        
      }else if(doScan){
        BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
      }
      
      delay(1000); // Delay a second between loops.
  }
  go_to_sleep();
} // End of loop
