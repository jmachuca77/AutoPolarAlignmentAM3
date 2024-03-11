#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer_Generic.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>

// WIFI definitions
const char* ssid = "James05";          // your SSID (wifi network name)
const char* password = "1977041500";    //wifi passwprd
const char* ap_ssid = "PA_Control";     // Wifi ssid when in access point mode
const char* ap_password = "1977041500"; // Wifi password for access point
const char* mDNS_Hostname = "AM3polar"; // name of the web adress. http://AM3polar.local


const int AZ_STEPS_PER_DEGREE = 2489;
const int ALT_STEPS_PER_DEGREE = 5880;

#define motor1dirPin 1        // to be changed according to the microcontroller used
#define motor1stepPin 3       // to be changed according to the microcontroller used
#define motor1enablePin 4    // to be changed according to the microcontroller used
#define motor2dirPin 14       // to be changed according to the microcontroller used
#define motor2stepPin 12      // to be changed according to the microcontroller used
#define motor2enablePin 13    // to be changed according to the microcontroller used
//#define stepsPerRevolution 16000

AccelStepper motorAZ(1,motor1stepPin,motor1dirPin); 
AccelStepper motorALT(1,motor2stepPin,motor2dirPin);
long dist = 0;

// motor state
bool motorInMiscare = false;
bool fromWEB = false;

//login credentials
const char* www_username = "Jaime";  //username to access the web interface
const char* www_password = "G0P0J1ily";  //password

// Web server object
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Websocket Update Delay Timer
// ========================================================================
const uint8_t updateTimeDelay = 100;      // How long to wait for updates to process (ms) - Default 100 ms

uint32_t updateTimeStart;
uint32_t updateTimeNow;
uint32_t updateTimeElapsed;
bool updateNeeded = false;
bool motorsEnabled = false;


// Function that sends messages to the WebSocket - combine data in the form "Nume_|_Valoare"
// ========================================================================
// uses: webSocketSend("Type", String(Data));
void webSocketSend(String msgType, String msgData) {
  String delimiter = "_|_";
  String fullMessage = msgType + delimiter + msgData;
  webSocket.broadcastTXT(fullMessage);
}


// Update clients - function can be called whenever needed
// ========================================================================
void updateClients() {
  if(motorsEnabled) {
    webSocketSend("motorChange", "1");
  }
  else {
    webSocketSend("motorChange", "0");
  }
}

// Function to handle WebSocket events
// ========================================================================

void webSocketEvent(const uint8_t& num, const WStype_t& type, uint8_t * payload, const size_t& length)
{
  (void) length;

  switch (type)
  {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      // send message to client
      webSocketSend("Connected", "1");
      updateClients();
    }
    break;

    case WStype_TEXT:
    {
      Serial.printf("Message Received: %s\n", payload);
      char *stringToSplit = (char*) payload;
      const char delim[4] = "_|_";                      // _|_ message delimiter incoming/outgoing
      char *msgType = strtok(stringToSplit, delim);     // The first part of the message is msgType (variable)
      char *msgData = strtok(NULL, delim);              // The second part of the message is msgData (variable value)
      char *msgDir = strtok(NULL, delim);            // The third part is msgDir (motor direction)
      
      if ((msgType != NULL) && (msgData != NULL)) {     // Check if the message contains two parts
        processWebSocketMessage(msgType, msgData, msgDir);      // Send the data to the function that can process the message
      } else {
        Serial.println("Invalid message - Cannot be processed!");
      }      
    }
    break;
    default:
      break;
  }
}



void moveALT(char * distance, char * direction) {
  
  if (motorALT.distanceToGo() == 0)
  {
    int distanceValue = atoi(distance);
    int directionValue = atoi(direction);

    // Convert distance from arcseconds to degrees
    float angleInDegrees = static_cast<float>(distanceValue) / 3600.0;

    // Determine the number of steps based on the motor's steps per degree
    long stepsToMove = static_cast<long>(angleInDegrees * ALT_STEPS_PER_DEGREE);

    if(directionValue) {
      motorALT.move(stepsToMove);
    } else {
      motorALT.move(-stepsToMove);
    }

    motorInMiscare = true;
  }
  // webSocketSend("motorRunningALT", distance); // debug message
  updateClients();
}

void moveAZ(char * distance, char * direction) {
  if (motorAZ.distanceToGo() == 0)
  {
    int distanceValue = atoi(distance);
    int directionValue = atoi(direction);

    // Convert distance from arcseconds to degrees
    float angleInDegrees = static_cast<float>(distanceValue) / 3600.0;

    // Determine the number of steps based on the motor's steps per degree
    long stepsToMove = static_cast<long>(angleInDegrees * AZ_STEPS_PER_DEGREE);

    if(directionValue) {
      motorAZ.move(-stepsToMove);
    } else {
      motorAZ.move(stepsToMove);
    }

    motorInMiscare = true;
  }
  // webSocketSend("motorRunningAZ", distance); // debug message
  updateClients();
}


void toggleMotors() {
  if(!motorsEnabled) {
    motorAZ.enableOutputs();
    motorALT.enableOutputs();
    digitalWrite(motor1enablePin, LOW);
    digitalWrite(motor2enablePin, LOW);
    motorsEnabled = true;
  } else {
    motorAZ.disableOutputs();
    motorALT.disableOutputs();
    digitalWrite(motor1enablePin, HIGH);
    digitalWrite(motor2enablePin, HIGH);
    motorsEnabled = false;
  }
  updateClients();
}

// Function for processing incoming messages via WebSocket
// ========================================================================
void processWebSocketMessage(char * msgType, char * msgData, char * msgDir) {
  Serial.printf("msgType: %s - msgData: %s - msgDir: %s\n", msgType, msgData, msgDir);
  if (strcmp(msgType, "goALT") == 0) { 
    moveALT(msgData, msgDir);
  } else if (strcmp(msgType, "goAZ") == 0) { 
   moveAZ(msgData, msgDir);
  } else if (strcmp(msgType, "toggleMotors") == 0) { 
   toggleMotors();
  } else {
    Serial.print("Un-recognized command: ");
    Serial.println(msgType);
    return;
  }
  // We set an update flag and start a timer to prevent excessive updates
  // ---------------------------------------------------------------------------
  updateNeeded = true;
  updateTimeStart = millis();
}

// Function for determining the type of file sent to the webserver
// ========================================================================
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

uint16_t listAllFilesInDir(String indent, String path)
{
	uint16_t dirCount = 0;
  Dir dir = LittleFS.openDir(path);
  while (dir.next())
  {
    ++dirCount;
    if (dir.isDirectory())
    {
      Serial.printf_P(PSTR("%s%s [Dir]\n"), indent.c_str(), dir.fileName().c_str());
      dirCount += listAllFilesInDir(indent + "  ", path + dir.fileName() + "/");
    }
    else
      Serial.printf_P(PSTR("%s%-16s (%ld Bytes)\n"), indent.c_str(), dir.fileName().c_str(), (uint32_t)dir.fileSize());
  }
  return dirCount;
}

// Function to read files from controller flash
// ========================================================================
bool handleFileRead(String httpPathRequested) {
  Serial.println("handleFileRead: " + httpPathRequested);
  if (httpPathRequested.endsWith("/")) {            // if don't ask for a specific file we reply with index.html
    httpPathRequested = "index.html";
  }
  String contentType = getContentType(httpPathRequested);
  
  if (LittleFS.exists(httpPathRequested)) {
    File file = LittleFS.open(httpPathRequested, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;
}

void setup(void) {
  // Initialisation of pins
  Serial.begin(115200);

  pinMode(motor1stepPin, OUTPUT);
  pinMode(motor1dirPin, OUTPUT);
  pinMode(motor1enablePin, OUTPUT);
  pinMode(motor2stepPin, OUTPUT);
  pinMode(motor2dirPin, OUTPUT);
  pinMode(motor2enablePin, OUTPUT);

  digitalWrite(motor1enablePin, HIGH);
  digitalWrite(motor2enablePin, HIGH);

  // Connecting to WiFi
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Serial.print("\nWaiting for WiFi to connect...");
  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print(".");    
  // }  
  // IPAddress IP = WiFi.localIP();
  // Serial.println("\n\nWiFi connected! IP Address: " + (IP).toString());

  // Start File System
  // ---------------------------------------------------------------------------
  if (LittleFS.begin()) {
    Serial.println("\n\n LittleFS initialization");

    // useful when debugging to see if files are present in flash

    Serial.printf_P(PSTR("Listing contents...\n"));
    uint16_t fileCount = listAllFilesInDir("", "/");
    Serial.printf_P(PSTR("%d files/dirs total\n"), fileCount);
  }

  // Create a script and add URL for websocket connection based on current IP Address
  // ---------------------------------------------------------------------------
  File textIPFile = LittleFS.open("wsConnection.js", "w");
  textIPFile.print("wsConnection = \"ws://" + IP.toString() + ":81\";\n");
  textIPFile.close();

  if (MDNS.begin(mDNS_Hostname))
  {
    Serial.println("MDNS responder started");
  }

  // Configure and Start webServer and authentication
  // ---------------------------------------------------------------------------
  // If client requests any URI, send it, otherwise respond with 404
  
  server.onNotFound([]() {
    if(!server.authenticate(www_username, www_password))
   return server.requestAuthentication();
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "404: Not Found");
  });


  // Start WebSocket
  // ---------------------------------------------------------------------------

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);    
  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  Serial.println("HTTP server started");

  motorAZ.setMaxSpeed(2000);
  motorAZ.setSpeed(100);
  motorAZ.setAcceleration(800);
  motorALT.setMaxSpeed(2000);
  motorALT.setSpeed(100);
  motorALT.setAcceleration(800);

  Serial.println("Motor settings updated");

  Serial.println("Setup Complete =)");
}

void loop() {
  // Function to delay Websocket Updates after status change
  // ---------------------------------------------------------------------------
  if (updateNeeded == true) {
    updateTimeNow = millis();
    updateTimeElapsed = updateTimeNow - updateTimeStart;
    if (updateTimeElapsed >= updateTimeDelay) {
      updateNeeded = false;
      updateClients();
    }
  }

  webSocket.loop();
  server.handleClient();

  //Check for MDNS Stuff
  // ---------------------------------------------------------------------------
  MDNS.update();

  if (motorAZ.distanceToGo() == 0 && motorALT.distanceToGo() == 0)
  {
    motorInMiscare = false;
  }

  if(motorsEnabled) {
    motorAZ.run();
    motorALT.run();
  }
}
