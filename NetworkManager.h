/** NetworkManager Class -
    Configure Your Credentials Here
*/
#include <Arduino.h>
#include "Base64.h"
#include "sha256.h"
#include <WiFiClientSecure.h>
#include "Hash.h"
#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include "PubSubClient.h"
#include "NTPClient.h"
#include <WiFiUdp.h>
#include <SHA256.h>
#include <TimeLib.h>

class NetworkManager {

  public:
    NetworkManager();

    /* LIBRARY CONSTRUCTORS */
    RF24 radio;
    RF24Network network;
    RF24Mesh mesh;
    WiFiClientSecure wifiClient;
    PubSubClient client;
    NTPClient timeClient;
    WiFiUDP ntpUDP;
    SHA256 sha256;
    int status;
    int clientState;

    /* RADIO */
    int CE = 33;
    int CS = 27;

    /* CREDENTIALS ***/

    /**  ---------   AZURE  ---------- */

    // -- Server
    String server =  "Acando-IoT.azure-devices.net";
    int port = 8883;

    // Device INFO clientId = deviceID
    String clientId = "CLIENTID";
    String hub_user = "USER";
    const char* host;
    char* key;
    const char* deviceId;

    // default topic feed for publishing is "devices/<myCoolDevice>/messages/events/"
    String pubEndpoint = "devices/Acando-Stockholm/messages/events/";
    String subEndpoint = "devices/Acando-Stockholm/messages/devicebound/#";

    /**  ^^^^^^^   AZURE  ^^^^^^^^^^^ */

    /** NETWORK MANAGER FUNCTIONS **/
    void init();
    void sendPayloadToCloud();
    void connectWiFi();
    void connectCloud();
    void updateTime();
    void updateMesh();
    void updateDHCP();
    void printInfo();
    void callback(char* topic, byte* payload, unsigned int length);
    String errorCodes(int errorCode);
    void setConnectionString(String cs);
    time_t  sasExpiryTime = 0;
    int sasExpiryPeriodInSeconds = 60 * 60 * 24; // SAS TOKEN EXPIRY

  protected:

    String urlEncode(const char* msg);
    const char* GetStringValue(String value);

  private:

    const char* connectionString = "PUTCONNECTIONSTRING";
    bool generateSas();
    String splitStringByIndex(String data, char separator, int index);
    char* format(const char *input, const char *value);
    char* format(const char *input, const char *value1, const char *value2);
    String createSas(char* key, String url);
    String url = "URL";
    String newSas;
    const char* cUrl;

  
    /*** WIFI CREDENTIALS ***/
    String ssid = "SSID";       /*** your network SSID (name) ***/
    String pass = "PASS";   /*** your network password ***/
   
    ///// --- Payload
    float incommingDataArray[7];
    String nodeID;
    String temp;
    String hum;
    String voc;
    String c02;
    String motion;
    String sound;
    String error;
    String jsonString;
    String payload;
    int count = 0;
    long lastReconnectAttempt;

};
