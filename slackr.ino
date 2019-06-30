#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
 
// WiFi details
const String SSID = "YOUR_SSID";
const String PASSWORD = "YOUR_PASSWORD";
 
// Web/Server address to read/write from 
const String HOST = "hooks.slack.com";
const int HTTPS_PORT = 443;
 
// SHA1 fingerprint of certificate
// Copy your Slack webhook in your browser, go to certificate and copy the fingerprint
const char FINGERPRINT[] PROGMEM = "YOUR_FINGERPRINT";

// Slack details
const String ROUTE = "YOUR_WEBHOOK";
const String REQUEST_BODY = "{\"text\": \"<!channel> TEST\"}";
const int CONTENT_LENGTH = REQUEST_BODY.length();
const int MAX_RETRIES = 20;

//HTTPS client
WiFiClientSecure httpsClient;

// Button
int current_button_state = 0;
int previous_button_state = 0;
bool posting = false;

void setup () {
  pinMode(D1, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  // Prevents reconnection issue (taking too long to connect)
  WiFi.mode(WIFI_OFF);
  delay(1000);
  // Don't use the ESP as a hotspot
  WiFi.mode(WIFI_STA);

  // Connect to WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");

  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to: ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

  // HTTPS client
  Serial.println(HOST);
  Serial.printf("Using fingerprint '%s'\n", FINGERPRINT);
  httpsClient.setFingerprint(FINGERPRINT);
  delay(1000);
  Serial.println("HTTPS Connecting");
  int retry_counter = 0;

  while ( (!httpsClient.connect(HOST, HTTPS_PORT)) && (retry_counter < MAX_RETRIES) ) {
    delay(100);
    Serial.print(".");
    retry_counter++;
  }

  if(retry_counter == MAX_RETRIES) {
    Serial.println("Connection failed");
    digitalWrite(LED_BUILTIN, LOW);
  }
  else {
    Serial.println("Connected to web");
    blink();
  }
}

void loop () {
  current_button_state = digitalRead(D1);

  if (current_button_state != previous_button_state) {
    if (current_button_state == HIGH) {
      Serial.println("ON");
      if (!httpsClient.connected()) {
        httpsClient.connect(HOST, HTTPS_PORT);
        delay(200);
      }
      if (!posting) {
        post_message(httpsClient);
      }      
    }
    else {
      Serial.println("OFF");
    }
    delay(100);
  }

  previous_button_state = current_button_state;
}

void post_message(WiFiClientSecure httpsClient) {
  posting = true;
  digitalWrite(LED_BUILTIN, LOW);

  httpsClient.print(String("POST ") + ROUTE + " HTTP/1.1\r\n" +
                           "Host: " + HOST + "\r\n" +
                           "Content-Type: application/json"+ "\r\n" +
                           "Content-Length: " + CONTENT_LENGTH + "\r\n\r\n" +
                           REQUEST_BODY + "\r\n" +
                           "Connection: close\r\n\r\n");
                            
  while (httpsClient.connected()) {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers received");
      break;
    }
  }

  Serial.println("Response:");
  Serial.println("==========");
  String line;
  while(httpsClient.available()){        
    line = httpsClient.readStringUntil('\n');
    Serial.println(line);
  }

  posting = false;
  digitalWrite(LED_BUILTIN, HIGH);
}

void blink() {
  int i = 3;

  while (i > 0) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    i--;
  }
}