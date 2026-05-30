#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <AESLib.h>

// ── Pinos ───────────────────────────────────────────────────
#define PIN_POT1  34
#define PIN_POT2  35
#define PIN_POT3  32
#define PIN_LED   25

// ── WiFi ────────────────────────────────────────────────────
const char* ssid     = "agents";
const char* password = "QgC9O8VucAByqvVu5Rruv1zdpqM66cd23KG4ElV7vZiJND580bzYvaHqz5k07G2";

// ── MQTT ────────────────────────────────────────────────────
const char* mqtt_server  = "broker.mqtt-dashboard.com";
const char* TOPIC_PUB    = "IPB/IoT/a52965/filtro/dados";
const char* TOPIC_THRESH = "IPB/IoT/a52965/filtro/threshold";

// ── Chave AES-128 (16 bytes) — igual no Node-RED ─────────────
const char* AES_KEY = "IPB_IoT_a52965!!";  // 16 caracteres exatos

// ── Limiares ─────────────────────────────────────────────────
float DP_THRESHOLD = 1.5;
float Q_MIN        = 20.0;

// ── Objetos globais ──────────────────────────────────────────
WiFiClient   espClient;
PubSubClient client(espClient);
AESLib       aesLib;
long lastTime = 0;

// ── Média ADC ────────────────────────────────────────────────
int lerADC(int pino) {
  long soma = 0;
  for (int i = 0; i < 16; i++) {
    soma += analogRead(pino);
    delay(2);
  }
  return soma / 16;
}

float adcToPressure(int raw) { return (raw / 4095.0) * 5.0; }
float adcToFlow(int raw)     { return (raw / 4095.0) * 100.0; }

// ── Cifrar com AES-128 CBC → Base64 ─────────────────────────
String encryptAES(String plaintext) {
  byte key[16];
  memcpy(key, AES_KEY, 16);

  // IV fixo para simplicidade — podes tornar aleatório depois
  byte iv[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};

  // Padding PKCS7 para múltiplo de 16
  int len = plaintext.length();
  int padded = ((len / 16) + 1) * 16;
  byte data[padded];
  memset(data, padded - len, padded);  // PKCS7
  memcpy(data, plaintext.c_str(), len);

  aesLib.encrypt(data, padded, data, key, 128, iv);

  // Codificar em Base64
  String result = "";
  const char* b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i = 0;
  while (i < padded) {
    uint32_t n = ((uint32_t)data[i] << 16);
    if (i+1 < padded) n |= ((uint32_t)data[i+1] << 8);
    if (i+2 < padded) n |= data[i+2];
    result += b64chars[(n >> 18) & 63];
    result += b64chars[(n >> 12) & 63];
    result += (i+1 < padded) ? b64chars[(n >> 6) & 63] : '=';
    result += (i+2 < padded) ? b64chars[n & 63] : '=';
    i += 3;
  }
  return result;
}

// ── WiFi ─────────────────────────────────────────────────────
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected — IP: " + WiFi.localIP().toString());
}

// ── Callback MQTT ────────────────────────────────────────────
void callback(char* topic, byte* payload, unsigned int length) {
  String topico = String(topic);
  String valor  = "";
  for (int i = 0; i < length; i++) valor += (char)payload[i];

  if (topico == TOPIC_THRESH) {
    StaticJsonDocument<100> doc;
    if (!deserializeJson(doc, valor)) {
      if (doc.containsKey("dp_threshold")) DP_THRESHOLD = doc["dp_threshold"];
      if (doc.containsKey("q_min"))        Q_MIN        = doc["q_min"];
      Serial.printf("Threshold atualizado → ΔP>%.2f kPa | Q<%.1f%%\n", DP_THRESHOLD, Q_MIN);
    }
  }
}

// ── Reconnect MQTT ───────────────────────────────────────────
void reconnect() {
  while (!client.connected()) {
    Serial.print("MQTT: ligando...");
    String clientId = "ESP32Filtro-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" OK");
      client.subscribe(TOPIC_THRESH);
    } else {
      Serial.printf(" falhou rc=%d, a tentar em 5s\n", client.state());
      delay(5000);
    }
  }
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  aesLib.set_paddingmode(paddingMode::CMS);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ── Loop ─────────────────────────────────────────────────────
void loop() {
  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  if (!client.connected()) reconnect();
  client.loop();

  long now = millis();
  if (now - lastTime > 2000) {
    lastTime = now;

    int rawP1   = lerADC(PIN_POT1);
    int rawP2   = lerADC(PIN_POT2);
    int rawFlow = lerADC(PIN_POT3);

    float p1   = adcToPressure(rawP1);
    float p2   = adcToPressure(rawP2);
    float dp   = p1 - p2;
    float flow = adcToFlow(rawFlow);
    bool  sat  = (dp > DP_THRESHOLD) || (flow < Q_MIN);

    digitalWrite(PIN_LED, sat ? HIGH : LOW);

    // Montar JSON
    StaticJsonDocument<200> doc;
    doc["p1_kpa"]    = round(p1   * 100) / 100.0;
    doc["p2_kpa"]    = round(p2   * 100) / 100.0;
    doc["dp_kpa"]    = round(dp   * 100) / 100.0;
    doc["flow_pct"]  = round(flow * 10)  / 10.0;
    doc["saturado"]  = sat;
    doc["estado"]    = sat ? "SATURADO" : "OK";
    doc["dp_thresh"] = DP_THRESHOLD;
    doc["q_min"]     = Q_MIN;

    char jsonBuf[200];
    serializeJson(doc, jsonBuf);

    // Cifrar e publicar
    String encrypted = encryptAES(String(jsonBuf));
    client.publish(TOPIC_PUB, encrypted.c_str());

    Serial.printf("[%lus] P1=%.2f P2=%.2f ΔP=%.2f Q=%.1f%% → %s\n",
                  now/1000, p1, p2, dp, flow, sat ? "SATURADO" : "OK");
    Serial.println("Publicado cifrado: " + encrypted);
  }
}
