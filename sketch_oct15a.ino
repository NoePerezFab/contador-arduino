#include "WiFi.h"

#include "HTTPClient.h"

#include "conection.h"

#include <ArduinoJson.h>

#include "WebSocketsClient.h"

#include "StompClient.h"

const char* ssid = SECRET_SSID;

const char* password = SECRET_PSW;
const int sensor = 35;

bool useWSS                      = false;

const char* ws_host              = "tt-server.ddns.net";

const int ws_port                = 8081;

const char* ws_baseurl           = "/contador-patadas/ws/"; 

bool in_rutina                    = false;

unsigned long tiempo_rutina       = 0;

unsigned long tiempo_envio        = 0;

unsigned long id_rutina        = 0;

WebSocketsClient webSocket;

Stomp::StompClient stomper(webSocket, ws_host, ws_port, ws_baseurl, true);

void setup() {

  Serial.begin(115200);
  pinMode(35, INPUT_PULLDOWN);    
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.println("Connecting to WiFi..");

  }

  Serial.println("Connected to the WiFi network");

   stomper.onConnect(subscribe);

  stomper.onError(error);

  if (useWSS) {

   stomper.beginSSL();

   } else {
   
   stomper.begin();

  }

}

void subscribe(Stomp::StompCommand cmd) {

 Serial.println("Connected to STOMP broker");

 stomper.subscribe("/call/message/2", Stomp::CLIENT, handleGreetingsMessage);


}

Stomp::Stomp_Ack_t handleGreetingsMessage(const Stomp::StompCommand cmd) {

 Serial.println("Got a message!");
  String json = "";
  for(int i=0;i<cmd.body.length();i++){
    if(cmd.body.charAt(i) != '\\'){
      json = json + cmd.body.charAt(i);
    }
  }
  Serial.println(json);
  StaticJsonDocument<256> doc;
DeserializationError e = deserializeJson(doc,json);
Serial.println(e.f_str());
in_rutina = true;
tiempo_rutina = doc["tiempo"];
tiempo_envio = doc["tiempoEnvio"];
id_rutina = doc["id"];
Serial.println(tiempo_rutina); 
Serial.println(tiempo_envio);  
 return Stomp::CONTINUE;

}

void error(const Stomp::StompCommand cmd) {

 Serial.println("ERROR: " + cmd.body);

}

void loop() {
  bool valorSensor;
  webSocket.loop();
  while(in_rutina){
    unsigned long tiempo_inicio = millis();
    int patadas = 0;
    StaticJsonDocument<1250> doc;
    JsonArray array = doc.to<JsonArray>();
    unsigned long tiempo_transcurrido_envio = millis();
    String array_json;
    Serial.println("======Inicia rutina======");
    while(millis() < tiempo_inicio+(tiempo_rutina)){
      valorSensor = digitalRead(sensor);
  webSocket.loop();
  if(valorSensor == 1){
    Serial.println("=====Patada detectada============");
    delay(50);
    while(valorSensor == 1){
      
      valorSensor = digitalRead(sensor);
    }
    addPatada(array,millis()-tiempo_inicio);
    Serial.println("=====Patada Guardada============");
    Serial.println("=====Patada Guardada============");

    }
   if(millis() > tiempo_transcurrido_envio + tiempo_envio){
        tiempo_transcurrido_envio = millis();
        Serial.println("=======Enviando patadas=========");
        serializeJson(doc,array_json);
        Serial.println(array_json);
        addPatadas(array_json);
        array.clear();
        array = doc.to<JsonArray>();
        array_json = "";
    }

  }
  Serial.println("=======Enviando patadas=========");
        serializeJson(doc,array_json);
        Serial.println(array_json);
        addPatadas(array_json);
        array.clear();
        array = doc.to<JsonArray>();
        array_json = "";
  Serial.println("======Termina rutina======");
tiempo_rutina = 0;
in_rutina = false;  
  
  }


  
}

String ApiHost = "http://tt-server.ddns.net:8081/contador-patadas/api/rutina";

void processResponse(int httpCode, HTTPClient& http){
   if (httpCode > 0) {
      Serial.printf("Response code: %d\t", httpCode);
      if (httpCode == HTTP_CODE_OK) {
         String payload = http.getString();
         Serial.println(payload);
      }
   }
   else {
      Serial.printf("Request failed, error: %s\n", http.errorToString(httpCode).c_str());
   }
   http.end();
}


void addPatada (JsonArray array,unsigned long tiempo){

    JsonObject nested = array.createNestedObject();
    nested["tiempo"] = tiempo;
}

void addPatadas(String patadas){
  HTTPClient http;
   http.begin(ApiHost + "/add-patadas/"+id_rutina);
   http.addHeader("mode", "cors");
   Serial.println(ApiHost + "/add-patadas/"+id_rutina);
   Serial.println(patadas);
   http.addHeader("Content-Type", "application/json");

   int httpCode = http.PUT(patadas);
   processResponse(httpCode, http);
}

