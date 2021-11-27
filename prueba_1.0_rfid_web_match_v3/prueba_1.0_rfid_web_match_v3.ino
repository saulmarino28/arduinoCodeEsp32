///////////////////////////////////////////////////
////////////// LECTURA DE RFID ///////////////////
////////////// SAUL MARINO     //////////////////
///////////// FES ARAGON UNAM  /////////////////
//////////// ELIGIO ME LA PELA ////////////////
//////////////////////////////////////////////
 /*
 * Typical RFID pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   ESP 32     Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega                 Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D22         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D21        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D23        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D19        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D18        ICSP-3           15
 */
#include <WiFiClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
//#include <SPIFFS.h>
#include <SPI.h>
#include <MFRC522.h>

#define rst_pin 22  //Esp32
#define ss_pin 21
//#define rst_pin 9   //arduino
//#define ss_pin  10
MFRC522 mfrc522(ss_pin, rst_pin);

const char* ssid = "Internet";
const char* password = "@Colima1nternet#";

WebServer server(80);
int ledAccess = 2; 
int ledAccess2 = 6;  
int ledAccess3 = 8;  

int ledDeneg = 4;
String DNS = "esp32_saulma";
String MatchRfid = "";
String lectUid=""; 
 
// Variables will change:
int ledState = LOW;             // ledState used to set the LED
unsigned long previousMillis = 0;        // will store last time LED was updated
// constants won't change:
const long interval = 1000;           // interval at which to blink (milliseconds)

void setCrossOrigin(){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Credentials", "true");
    server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.sendHeader("Access-Control-Max-Age", "1000");
   
};
//
void setMatch() {
   Serial.println(F("Comparando"));
   setCrossOrigin();
   String postBody = server.arg("plain");
   Serial.println(postBody);
   DynamicJsonDocument doc(512);
   DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        // if the file didn't open, print an error:
        Serial.print(F("Error parsing JSON: "));
        Serial.println(error.c_str());
        String msg = error.c_str();
        server.send(400, F("text/html"), "Error in parsin json body! <br>"+msg);
    }
    else{
        JsonObject postObj = doc.as<JsonObject>();
        //Serial.print(F("HTTP Method: "));
        //Serial.println(server.method());
        if (server.method() == HTTP_POST) {
            if ((postObj.containsKey("match"))) {
                // Here you can open your file to store your config
                // Now I simulate It and set configFile a true
                    Serial.println(F("Hecho."));
                    String matched = postObj[F("match")];
                    Serial.print("match: ");
                    Serial.println(matched);
                    //funcion del led;
                    //y regresar 
                    if(matched == "true") {
                       digitalWrite(ledAccess, HIGH);
                       delay(1000);
                       digitalWrite(ledAccess, LOW);
                    }
                    else {
                       digitalWrite(ledDeneg, HIGH);
                       delay(1000);
                       digitalWrite(ledDeneg, LOW);
                    }
                    String msgSuccess = "prueba exitosa";
//                  server.sendHeader("Content-Length", String(postBody.length()));
                    server.send(201, F("application/json"), msgSuccess);
//                  Serial.println(F("Sent reset page"));
//                    delay(5000);
//                    ESP.restart();
//                    delay(2000);
            }
            else {
                server.send(204, F("text/html"), F("No data found, or incorrect!"));
            }
        }
    }
}
// Serving SendRFID
void sendRfid() {
      setCrossOrigin();
      DynamicJsonDocument doc(512);
      //String rfid = getrfid().c_str();
      String rfid = getrfid(); 
      doc["rfid"] = rfid;
      //doc["resp"] = true;
      Serial.println(F("leyendo"));
      String buf;
      serializeJson(doc, buf);
      server.send(200, "application/json", buf);
      //limpiar var 
      CleanLectRfid();
}
//encabezado para aceptar origenes cruzados
void sendCrossOriginHeader(){
    Serial.println(F("sendCORSHeader"));
    server.sendHeader(F("access-control-allow-credentials"), F("false"));
    setCrossOrigin();
    server.send(204);
}
// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    server.on(F("/sendRfid"), HTTP_OPTIONS, sendCrossOriginHeader);
    server.on(F("/sendRfid"), HTTP_GET, sendRfid);
 
    server.on(F("/getMatch"), HTTP_OPTIONS, sendCrossOriginHeader);
    server.on(F("/getMatch"), HTTP_POST, setMatch);
}
// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  // put your setup code here, to run once:
  //leds de acceso
  pinMode(ledAccess, OUTPUT);
  pinMode(ledDeneg, OUTPUT);
  
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane 
  if (MDNS.begin("esp32_saulma")) {
    Serial.println("MDNS responder started");
    Serial.println("Nombre DNS:" +DNS);
  }
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  //server.enableCORS(false);
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  
}

void CleanLectRfid() {
  if(lectUid != ""){
    /*delay(500); //delay .5 seg, 5 seg para prueba
    lectUid="";
    /*/
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time
      previousMillis = currentMillis;
      lectUid="";
    }//*/
     
  }     
}

String getrfid() {
     String saul= "invalid";
     if(! mfrc522.PICC_IsNewCardPresent())
      return String(saul);
     if(! mfrc522.PICC_ReadCardSerial())
      return String (saul);
     for(byte i=0; i<mfrc522.uid.size; i++){
      if(mfrc522.uid.uidByte[i]<0x10){
      Serial.print(" 0"); 
      lectUid += String ("0");
      } 
      else {
       Serial.print(" ");
       //lectUid += String (" ");  
        }
        Serial.print(mfrc522.uid.uidByte[i], HEX ); 
        //lectUid[i]=mfrc522.uid.uidByte[i];
        //Serial.print(lectUid[i]);
        
        lectUid += String (mfrc522.uid.uidByte[i], HEX);
      }
      Serial.println("");
      mfrc522.PICC_HaltA();
      return String(lectUid);
}

void LedEncender(int ledPin) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
}
  
