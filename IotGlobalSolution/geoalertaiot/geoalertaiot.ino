#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// --- Config WiFi ---
const char* ssid = "";//nome da rede
const char* password = "";//senha da rede

// --- URL do Google Script ---
const char* scriptURL = "https://script.google.com/macros/s/AKfycbw0_2xdbqbYvo1NkBCf8yXT8vkSVZ-vcgPJa7dYGbft2l6fwnAtCja_myL-LxfgcXsg/exec";

// --- API Key do ThingSpeak ---
const char* thingspeakServer = "http://api.thingspeak.com/update";
const char* thingspeakAPIKey = "9AALAMNJK4RL0851";

// --- Sensores ---
#define DHTPIN 4       // DHT11 no pino D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//potenciometros
#define SOIL_PIN 34    
#define CHUVA_PIN 35 

unsigned long previousMillis = 0;
const long interval = 1200; 

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  dht.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("WiFi conectado");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float umidadeSoloRaw = analogRead(SOIL_PIN);
    float umidadeSolo = map(umidadeSoloRaw, 0, 4095, 0, 100);
    float umidadeAr = dht.readHumidity();

    float chuvaRaw = analogRead(CHUVA_PIN);
    int chuva = (chuvaRaw > 2000) ? 1 : 0; 

    // Cálculo de risco
    String aviso = "Baixo Risco";
    if (umidadeSolo > 70 && umidadeAr > 75 && chuva == 1) {
      aviso = "Alto Risco";
    } else if (umidadeSolo > 50 && umidadeAr > 65) {
      aviso = "Risco Moderado";
    }

    Serial.println("Enviando dados...");
    enviarParaGoogleSheets(umidadeSolo, umidadeAr, chuva, aviso);
    enviarParaThingSpeak(umidadeSolo, umidadeAr, chuva);
  }
}

// --- Google Sheets ---
void enviarParaGoogleSheets(float solo, float ar, int chuva, String aviso) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(scriptURL);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"umidade_solo\":" + String(solo, 2) +
                     ",\"umidade_ar\":" + String(ar, 2) +
                     ",\"chuva\":" + String(chuva) +
                     ",\"aviso\":\"" + aviso + "\"}";

    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) {
      Serial.println("Google Sheets OK: " + String(httpResponseCode));
    } else {
      Serial.println("Erro Google Sheets: " + String(httpResponseCode));
    }

    http.end();
  }
}

// --- ThingSpeak ---
void enviarParaThingSpeak(float solo, float ar, int chuva) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(thingspeakServer) + "?api_key=" + thingspeakAPIKey +
                 "&field1=" + String(solo, 2) +
                 "&field2=" + String(ar, 2) +
                 "&field3=" + String(chuva);

    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("ThingSpeak OK: " + String(httpResponseCode));
    } else {
      Serial.println("Erro ThingSpeak: " + String(httpResponseCode));
    }
    http.end();
  }
}
