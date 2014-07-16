
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

#define RELAY  6   // Digital pin for the relay

// commands that can be called through rest api : arduino_ip/arduino/$COMMAND/
// make sure REST api security is configured with password in the Yun web panel
const String DOOR_CMD = "door";
const String ENABLE_CMD = "enable-door";
const String DISABLE_CMD = "disable-door";
const String STATUS_CMD = "status";

const String ALL_COMMANDS = "door, enable-door, disable-door, status";
const String DISABLE_OK_MSG = "OK door disabled";
const String ENABLE_OK_MSG = "OK door enabled";
const String DOOR_OK_MSG = "OK Opening door for 2 seconds";
const String DOOR_KO_MSG = "OK door enabled";


boolean door_enabled = true;

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;
YunClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing...");
  pinMode(RELAY, OUTPUT);                           // Initialise the Arduino data pins for OUTPUT  (Relay)
  pinMode(13, OUTPUT);                              // Initialise the Arduino data pins for OUTPUT  (Led)
  digitalWrite(13, LOW);                            // Turns led off
  digitalWrite(RELAY, LOW);                         // Turns relay off
  
  Serial.println("Starting Server...");   
  Bridge.begin();        
  server.noListenOnLocalhost();                      // Listen for incoming connection not only from localhost
  server.begin();  
}

void loop() {
   // Get clients coming from server
  YunClient client = server.accept();

  // There is a new client?
  if (client) {  
    Serial.println("Processing client request...");
    // Process request
    process(client);

    // Close connection and free resources.
    client.stop();
  }
  delay(50); // Poll every 50ms
}


 // read the command, REST must be called with url ending with '/'
 // ex http://arduino.local/arduino/status/
void process(YunClient client) {
  String method = client.readStringUntil('/');   // GET or POST
  String root = client.readStringUntil('/');     // arduino
  String command = client.readStringUntil('/');  // door, status, enable-door, disable-door
  
  Serial.print("Processing command: ");
  Serial.println(command);
  
  if (command == DOOR_CMD) {
    if(door_enabled) {
     openDoor();
     println(DOOR_OK_MSG);
    } else {
     println(DOOR_KO_MSG);
    }
  } else if(command == ENABLE_CMD) {
    door_enabled = true;
    println(ENABLE_OK_MSG);
  } else if(command == DISABLE_CMD) {
    door_enabled = false;
    println(DISABLE_OK_MSG);
  } else if(command == STATUS_CMD) {
    print("OK door status is ");
    println(door_enabled ? "Enabled" : "Disabled");
  } else {
    print("KO Unknown command: '");
    print(command);
    print("', available commands are: ");
    println(ALL_COMMANDS);
  }
}

/*
* Print method to print on both serial & http client response
*/
void print(String message) {
   client.print(message);
   Serial.print(message); 
}

/*
* Print method to print on both serial & http client response
*/
void println(String message) {
   client.println(message);
   Serial.println(message); 
}

/*
* Turns relay on for 2s, then turns it off. Turn board led on/off 
* at the same time as a visual feedback
*/
void openDoor() {
  digitalWrite(13, HIGH);   // Turns led on
  digitalWrite(RELAY, HIGH); // Turns relay on
  delay(2000); 
  digitalWrite(RELAY, LOW); // Turns relay off
  digitalWrite(13, LOW);   // Turns led off
}
