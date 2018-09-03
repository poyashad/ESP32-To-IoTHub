#include "NetworkManager.h"

NetworkManager::NetworkManager():
  radio(33, 27),  // RADIO CE-PINS
  network(radio),
  mesh(radio, network),
  client(server.c_str(), port, NULL, wifiClient),
  timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000)

{

  lastReconnectAttempt = 0;
  count = 0;
  status = WL_IDLE_STATUS;


}

void NetworkManager::init() {
  Serial.println(F("Master Node - Gateway"));
  mesh.setNodeID(0);
  Serial.print(F("NodeID: "));
  Serial.println(mesh.getNodeID());
  mesh.begin();
  connectWiFi();
  timeClient.begin();
  timeClient.update();
  setConnectionString(connectionString);
  connectCloud();

}

void NetworkManager::updateTime() {
  timeClient.update();
}

// ---- - MESH
void NetworkManager::updateMesh() {
  mesh.update();

}

void NetworkManager::updateDHCP() {
  mesh.DHCP();
}

void NetworkManager::printInfo() {
  Serial.println(F(" "));
  Serial.println(F("********WIFI INFO***********"));
  Serial.print(F("SSID: "));
  Serial.print(ssid);
  Serial.print(F(" ** IP: "));
  Serial.print(WiFi.localIP());
  Serial.println(" ** Time:" + timeClient.getFormattedTime());
  Serial.print(F("-----------------------------"));
  Serial.println(F(" "));
  Serial.println(F("********Mesh Network********"));
  Serial.println(" ");
  Serial.println(F("********Assigned Addresses********"));
  for (int i = 0; i < mesh.addrListTop; i++) {
    Serial.print("NodeID: ");
    Serial.print(mesh.addrList[i].nodeID);
    Serial.print(" RF24Network Address: 0");
    Serial.println(mesh.addrList[i].address, OCT);
  }
  Serial.println(F("**********************************"));
}

void NetworkManager::sendPayloadToCloud() {
  RF24NetworkHeader header;
  network.peek(header);
  String quote = "\"";

  switch (header.type) {
    case 'M':
      jsonString = "";
      network.read(header, &incommingDataArray, sizeof(incommingDataArray));

      nodeID = mesh.getNodeID(header.from_node);
      temp = incommingDataArray[0];
      hum = incommingDataArray[1];
      voc = incommingDataArray[2];
      c02 = incommingDataArray[3];
      motion = incommingDataArray[4];
      sound = incommingDataArray[5];
      error = incommingDataArray[6];
      count = count + 1;
      delay(500);

      jsonString =
        "{\"ID\":" + quote + nodeID + quote + "," +
        "\"t\":" + temp + "," +
        "\"h\":" + hum + "," +
        "\"v\":" + voc + "," +
        "\"c\":" + c02 + "," +
        "\"m\":" + motion + "," +
        "\"time\":" + timeClient.getEpochTime() + "," +
        "\"s\":" + sound + "}";

      delay(500);
      Serial.println(jsonString);
      if (client.publish(pubEndpoint.c_str(), jsonString.c_str())) {
        wifiClient.flush();
        Serial.println("Publish ok");

      } else {
        Serial.println("Publish failed");
        Serial.println(errorCodes(client.state()));
        if (client.state() < 0 || client.state() > 0) {
          wifiClient.flush();
          wifiClient.stop();
          yield();
          connectWiFi();
          connectCloud();
        }

      }
      break;

    default:
      network.read(header, 0, 0);
      Serial.println(header.type);
      break;
  }
}

// ------ WIFI

void NetworkManager::connectWiFi() {

  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid.c_str(), pass.c_str());

    // wait 10 seconds for connection:
    delay(10000);
    Serial.println("");

    if (WiFi.localIP() != 0) {
      Serial.println(F("WiFi connected"));
      Serial.println(F("IP address: "));
      Serial.println(WiFi.localIP());
      Serial.println();
    } else {
      Serial.println(F("Connection to WiFi failed...Reconnecting"));
    }

    delay(500);
  }

}

/// --- CLOUD FUNCTIONS ------------

void NetworkManager::connectCloud() {

  Serial.println(F("Connecting to Cloud"));
  Serial.println();

  client.setServer(server.c_str(), port);

  while (!client.connected()) {
    //Serial.println("Fastnat på clientconnect");
    // CREATE A NEW SAS TimeNow + 24 hours
    newSas = createSas(key, url);
    if (client.connect(clientId.c_str(), hub_user.c_str(), newSas.c_str())) { // Here is where the connection to the cloud starts

      Serial.println(F("Client Connected To Cloud"));
      client.subscribe(subEndpoint.c_str());
      yield();

    } else {
      Serial.println();
      Serial.println(F("Client Failed To Connect To Cloud"));
      Serial.print(F("Error Code: "));
      Serial.println(errorCodes(client.state()));
      Serial.println("Reconnecting....");
      delay(500);
    }
  }
}

String NetworkManager::errorCodes(int errorCode) {
  switch (errorCode) {
    case -4: return "The server didn't respond within the keepalive time.";
    case -3: return "The network connection was broken.";
    case -2: return "The network connection failed.";
    case -1: return "The client disconnected cleanly.";
    case 0: return "The client connected.";
    case 1: return "The server doesn't support the requested version of MQTT.";
    case 2: return "The server rejected the client identifier.";
    case 3: return "The server was unable to accept the connection.";
    case 4: return "The username/password were rejected.";
    case 5: return "The client was not authorized to connect.";
    default: break;
  }
}

String NetworkManager::createSas(char* key, String url) {

  sasExpiryTime = timeClient.getEpochTime() + sasExpiryPeriodInSeconds;
  String stringToSign = url + "\n" + sasExpiryTime;

  int keyLength = strlen(key);

  int decodedKeyLength = base64_dec_len(key, keyLength);
  char decodedKey[decodedKeyLength];  //allocate char array big enough for the base64 decoded key

  base64_decode(decodedKey, key, keyLength);  //decode key

  Sha256.initHmac((const uint8_t*)decodedKey, decodedKeyLength);
  Sha256.print(stringToSign);
  char* sign = (char*) Sha256.resultHmac();
  // END: Create signature

  // START: Get base64 of signature
  int encodedSignLen = base64_enc_len(HASH_LENGTH);
  char encodedSign[encodedSignLen];
  base64_encode(encodedSign, sign, HASH_LENGTH);

  // SharedAccessSignature
  return "SharedAccessSignature sr=" + url + "&sig=" + urlEncode(encodedSign) + "&se=" + sasExpiryTime;
  // END: create SAS
}

String NetworkManager::urlEncode(const char* msg)
{
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  while (*msg != '\0') {
    if ( ('a' <= *msg && *msg <= 'z')
         || ('A' <= *msg && *msg <= 'Z')
         || ('0' <= *msg && *msg <= '9') ) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}

const char* NetworkManager::GetStringValue(String value) {
  int len = value.length() + 1;
  char *temp = new char[len];
  value.toCharArray(temp, len);
  return temp;
}

void NetworkManager::setConnectionString(String cs) {
  host = GetStringValue(splitStringByIndex(splitStringByIndex(cs, ';', 0), '=', 1));
  deviceId = GetStringValue(splitStringByIndex(splitStringByIndex(cs, ';', 1), '=', 1));
  key = (char*)GetStringValue(splitStringByIndex(splitStringByIndex(cs, ';', 2), '=', 1));
}

String NetworkManager::splitStringByIndex(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


