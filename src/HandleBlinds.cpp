#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <AutoConnect.h>
#include <Bounce2.h>
#include <Stepper.h>
#include <ArduinoJson.h>


#define CREDENTIAL_OFFSET 0

void handleRoot();
String viewCredential();
void handlePercent();
void handleStatus();
void handleGPIO();
void sendRedirect(String uri);
bool atDetect(IPAddress softapIP);
void deleteAllCredentials();
void blink();
void EEPROM_int_write(int addr, int num);
int getAngle();
int getMaxAngle();
void rotate(int angle);

const int ledled = 12;
int temp_percent = 0;

AutoConnect portal;
AutoConnectConfig Config; 

void handleRoot() {
  String page = PSTR(
"<html>"
 "<head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/> <style type=\"text/css\"> "
"body{-webkit-appearance: none; -moz-appearance: none; font-family: \"Arial\", sansÃ’-serif; text-align: center;}"
".menu > a:link{position: absolute; display: inline-block; right: 12px; padding: 0 6px; text-decoration: none;}"
".window_slider_container{margin: auto; padding: 30px 0; align-items: center; display: flex; flex-direction: column; z-index: 900;}"
"#window_slider_parent{background-color:  #333338; border-radius: 20px; position: relative; overflow: hidden; width: 100px; height: 300px; margin-bottom: 10px;}"
"#window_slider_cover{width: 100%; height: 100%; background-color: #72D5FF; position: absolute; text-align: center;}</style> </head> "
"<body><div class=\"menu\">" AUTOCONNECT_LINK(BAR_32) "</div>"
  "<div class=\"window_slider_container\"> <div id=\"window_slider_parent\"> <div id=\"window_slider_cover\"></div></div>"
"<span class=\"percent_value\" id=\"precent_value_id\"></span> <span class=\"error\" id=\"error_block\"></span> </div>"
"<script>function getBlindersStatus(){return fetch(\"/api/status\").then(response=> response.json()).then(response=>response.position);}"
"function setBlindersStatus(percent){return fetch(`/api/blinds?open=${percent}`).then(response=> response.json()).then(response=>response.position);}"
"function renderPosition(position){var elem=document.getElementById(\"window_slider_cover\"); var span_percent=document.getElementById(\"precent_value_id\"); var parentRect=document.getElementById(\"window_slider_parent\").getBoundingClientRect(); span_percent.textContent=`Open Percent ${position}%`; elem.style.top=`-${position * (parentRect.height/100)}px`}"
"document.addEventListener(\"DOMContentLoaded\", async function (){var elem=document.getElementById(\"window_slider_cover\"); dragElement(elem); var position=await getBlindersStatus(); renderPosition(position);}); "
"function dragElement(elmnt){var pos1=0, pos2=0; var parent=document.getElementById(\"window_slider_parent\"); document.getElementById( \"window_slider_parent\" ).onmousedown=dragMouseDown; "
 " function dragMouseDown(e){e=e || window.event; e.preventDefault();pos2=e.clientY; document.onmouseup=closeDragElement; document.onmousemove=elementDrag; document.touchend = closeDragElement; document.touchmove = elementDrag; document.touchstart = elementDrag;}"
 " function elementDrag(e){e=e || window.event; e.preventDefault(); pos1=pos2 - e.clientY; pos2=e.clientY;"
 "var parentRect=parent.getBoundingClientRect(); var elemRect=elmnt.getBoundingClientRect(); if ( parentRect.y >=elemRect.y - pos1 && parentRect.top <=elemRect.bottom - pos1 ){elmnt.style.top=elmnt.offsetTop - pos1 + \"px\";}}"
 "async function closeDragElement(){document.onmouseup=null; document.onmousemove=null; "
 "var parentRect=parent.getBoundingClientRect(); var elemRect=elmnt.getBoundingClientRect(); var elemTop=parseInt(window.getComputedStyle(elmnt).top);" 
 "var percent=Math.round((Math.abs(parentRect.height - Math.abs(elemTop))/ parentRect.height) * 100); var position=await setBlindersStatus(100 - percent); renderPosition(position);}}</script> </body></html>");
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
  String open_percent = server.arg("open");
  Serial.println(open_percent);
  int currentAngle = getAngle();
  int maxAngleLocal = getMaxAngle();

  StaticJsonDocument<100> responseDocument;
  temp_percent = open_percent.toInt();
  responseDocument["position"] = temp_percent;
  String response;
  serializeJson(responseDocument, response);
  portal.host().send(200, "application/json", response);
}


void handleStatus() {
  StaticJsonDocument<100> responseDocument;
  responseDocument["position"] = temp_percent;
  String response;
  serializeJson(responseDocument, response);
  portal.host().send(200, "application/json", response);
}


void handleGPIO() {
  Serial.println("handleGPIO");
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
      Serial.println("DEBUG " + String((int)currentAngle));
      myStepper.step(angle);
      currentAngle += angle;
      Serial.println("DEBUG 11 " + String((int)currentAngle));
      EEPROM_int_write(0, currentAngle);
      int newQwe = 0;
      EEPROM.get(0, newQwe);
      Serial.println("DEBUG2 " + String((int)currentAngle) + " DEBUGDEBUG " + String((int)newQwe));
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
  Config.psk = "bentcoyote80";
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
