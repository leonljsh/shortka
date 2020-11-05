
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <AutoConnect.h>
#include <Bounce2.h>
#include <Stepper.h>


#define CREDENTIAL_OFFSET 0

const int ledled = 12;

AutoConnect portal;
AutoConnectConfig Config; 

void handleRoot() {
  String page = PSTR(
"<html>"
"<head>"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<style type=\"text/css\">"
    "body {"
    "-webkit-appearance:none;"
    "-moz-appearance:none;"
    "font-family:'Arial',sansÒ-serif;"
    "text-align:center;"
    "}"
    ".menu > a:link {"
    "position: absolute;"
    "display: inline-block;"
    "right: 12px;"
    "padding: 0 6px;"
    "text-decoration: none;"
    "}"
    ".button {"
    "display:inline-block;"
    "border-radius:7px;"
    "background:#73ad21;"
    "margin:0 10px 0 10px;"
    "padding:10px 20px 10px 20px;"
    "text-decoration:none;"
    "color:#000000;"
    "}"
  "</style>"
"</head>"
"<body>"
  "<div class=\"menu\">" AUTOCONNECT_LINK(BAR_32) "</div>"
  "BUILT-IN LED<br>"
  "GPIO(");
  page += String(ledled);
  page += String(F(") : <span style=\"font-weight:bold;color:"));
  page += digitalRead(ledled) ? String("Tomato\">HIGH") : String("SlateBlue\">LOW");
  page += String(F("</span>"));
  page += String(F("<p><a class=\"button\" href=\"/io?v=low\">low</a><a class=\"button\" href=\"/io?v=high\">high</a></p>"));
  page += String(F("</body></html>"));
  portal.host().send(200, "text/html", page);
}

String viewCredential() {
  AutoConnectCredential  ac(CREDENTIAL_OFFSET);
  station_config_t  entry;
  String content = "";
  uint8_t  count = ac.entries();          // Get number of entries.

  for (int8_t i = 0; i < count; i++) {    // Loads all entries.
    ac.load(i, &entry);
    // Build a SSID line of an HTML.
    content += String("<li>") + String((char *)entry.ssid) + String((char *)entry.password) + String("</li>");
  }

  // Returns the '<li>SSID</li>' container.
  return content;
}



void handlePercent() {
  Serial.println("percent");
  Serial.println(viewCredential());
  WebServerClass& server = portal.host();
  String qweqwe = server.arg("open");
  Serial.println(qweqwe);
  int currentAngle = getAngle();
  int maxAngleLocal = getMaxAngle();

  String page = "{position=POSITION}";
  portal.host().send(200, "text/html", page);
}


void handleStatus() {
  String page = "{position=POSITION}";
  Serial.println("status");
  portal.host().send(200, "text/html", page);
}


void handleGPIO() {
  Serial.println("qweqe");
  Serial.println(viewCredential());
  WebServerClass& server = portal.host();
  if (server.arg("v") == "low")
    digitalWrite(ledled, LOW);
  else if (server.arg("v") == "high")
    digitalWrite(ledled, HIGH);
  sendRedirect("/");
}

void sendRedirect(String uri) {
  WebServerClass& server = portal.host();
  server.sendHeader("Location", uri, true);
  server.send(302, "text/plain", "");
  server.client().stop();
}

bool atDetect(IPAddress softapIP) {
  Serial.println("Captive portal started, SoftAP IP:" + softapIP.toString());
  return true;
}


void deleteAllCredentials() {
  AutoConnectCredential credential;
  station_config_t config;
  uint8_t ent = credential.entries();

  while (ent--) {
    credential.load(ent, &config);
    credential.del((const char*)&config.ssid[0]);
  }
}


const int upLed = 14;
const int downLed = 13;
const int upButton = 35;
const int downButton = 34;
Bounce debouncerUp = Bounce(); 
Bounce debouncerDown = Bounce(); 
unsigned long doublePressTime;
int prevDoublePressState = LOW;
int setupStatus = 0;
bool setupLock = false;


const int stepsPerRevolution = 200; 
Stepper myStepper(stepsPerRevolution, 27, 25,26, 33);

int maxAngle;

void blink() {
          digitalWrite(downLed, HIGH);
          digitalWrite(upLed, HIGH);
          delay(500);
          digitalWrite(downLed, LOW);
          digitalWrite(upLed, LOW);
          delay(1000);
          digitalWrite(downLed, HIGH);
          digitalWrite(upLed, HIGH);
          delay(500);
  }

// запись
void EEPROM_int_write(int addr, int num) {
  EEPROM.put(addr, num);
  EEPROM.commit();
}


int getAngle() {
    int currentAngle = 0;
    EEPROM.get(0, currentAngle);
    return currentAngle;
  }


int getMaxAngle() {
    int maxAngleLocal = 0;
    EEPROM.get(4, maxAngleLocal);
    return maxAngleLocal;
  }


void rotate(int angle) {
    int currentAngle = getAngle();
    int maxAngleLocal = getMaxAngle();

    Serial.println("loh " + String((int)maxAngleLocal));

    if (maxAngleLocal != 0 && maxAngleLocal < currentAngle + angle) {
      Serial.println("SOSOSOS ");
      }
    else {
      Serial.println("PIDR1 " + String((int)currentAngle));
      myStepper.step(angle);
      currentAngle += angle;
      Serial.println("PIDR 11 " + String((int)currentAngle));
      EEPROM_int_write(0, currentAngle);
      int newQwe = 0;
      EEPROM.get(0, newQwe);
      Serial.println("PIDR2 " + String((int)currentAngle) + " PIDRPIDR " + String((int)newQwe));
    }
  }

void setup() {
  Serial.begin(115200);
  Serial.println();
  EEPROM.begin(512);
  Serial.println("LOOOOOAD");

  maxAngle = 0;
  EEPROM.get(4, maxAngle);

  Serial.println(maxAngle);

  Serial.println("LOOOOOAD");
  pinMode(ledled, OUTPUT);
  pinMode(upLed, OUTPUT);
  pinMode(upButton, INPUT_PULLUP);
  debouncerUp.attach(upButton);
  debouncerUp.interval(50); // interval in ms

  pinMode(downLed, OUTPUT);
  pinMode(downButton, INPUT_PULLUP);
  debouncerDown.attach(downButton);
  debouncerDown.interval(50); // interval in ms

  myStepper.setSpeed(100);

//  Config.autoReconnect = true;
  Config.apid = "markusik";
  Config.psk = "gb3141828";
  portal.config(Config);
  portal.onDetect(atDetect);
  if (portal.begin()) {
    WebServerClass& server = portal.host();
    server.on("/", handleRoot);
    server.on("/io", handleGPIO);
    server.on("/api/blinds", handlePercent);
    server.on("/api/status", handleStatus);
    Serial.println("Started, IP:" + WiFi.localIP().toString());
  }
  else {
    Serial.println("Connection failed.");
    while (true) { yield(); }
  }

}

void loop() {
  debouncerUp.update();
  debouncerDown.update();
  portal.handleClient();

  if (WiFi.status() == WL_IDLE_STATUS) {
    Serial.println("restart");
    ESP.restart();
    delay(1000);
    Serial.println("delay ended");
  }

  int upValue = debouncerUp.read();
  int downValue = debouncerDown.read();

if (upValue == HIGH) {
    digitalWrite(upLed, HIGH);
  }
  else {
    digitalWrite(upLed, LOW);
    }
  
  if (downValue == HIGH) {
    digitalWrite(downLed, HIGH);
  }
  else {
    digitalWrite(downLed, LOW);
    }


  if ( upValue == HIGH && downValue == HIGH ) {
    Serial.println("1");
    if (prevDoublePressState == HIGH) {
      doublePressTime = millis();
      }
     unsigned long millisPressed = millis() - doublePressTime;
     if (millisPressed > 20000) {
        Serial.println("HUT");
        blink();
        EEPROM_int_write(4, 0);
        EEPROM_int_write(0, 0);
        blink();
        deleteAllCredentials();
        delay(1000);
        WiFi.disconnect(false, true);
        delay(1000);
        while (WiFi.status() == WL_CONNECTED) {
          delay(10);
          yield();
        }

        delay(1000);
        Serial.println("restart");
        ESP.restart();
        delay(1000);
        Serial.println("delay ended");
      }
      else if (millisPressed > 5000 && setupLock == false) {
        if (setupStatus == 0) {
          setupStatus = 1;
          Serial.println("setup up");
          blink();
          EEPROM_int_write(4, 0);
          EEPROM_int_write(0, 0);
          setupLock = true;
          }
      else if (setupStatus == 1) {
          setupStatus = 2;
          Serial.println("setup down");
          blink();
          setupLock = true;
          }
 
      else if (setupStatus == 2) {
          setupStatus = 0;
          Serial.println("setup finished");
          blink();
          maxAngle = getAngle();
          EEPROM_int_write(4, maxAngle);
          setupLock = true;
          }
   
        Serial.println("ZXC");
        delay(1000);
        }

    prevDoublePressState = LOW;
  } 
  else {
    prevDoublePressState = HIGH;
    doublePressTime = 0;
    setupLock = false;
  }
 
  if (upValue == HIGH && downValue == LOW) {
    Serial.println("2");
    rotate(100);
  }
  if (upValue == LOW && downValue == HIGH) {
    Serial.println("3");
    rotate(-100);

  }
}
