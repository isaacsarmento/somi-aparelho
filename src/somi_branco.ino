#include <WiFi.h>           
#include <IOXhop_FirebaseESP32.h>
#include <NTPClient.h>
#include <DHT.h>
#include <WiFiManager.h>

#define SENSOR_FOGO_ESQ_PIN 34  // Sensor de chama 1 (esquerda)
#define SENSOR_FOGO_DIR_PIN 35  // Sensor de chama 2 (direita)
#define DHT_PIN 4               // Sensor DHT11
#define MQ2_PIN 32              // Sensor MQ-2

// Configurações Firebase
#define FIREBASE_HOST "https://somi-shieldnig-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "DyTH9sfQC2kcNH4PNN0I3oSWQuMpno2yf1WaQYyO"

// Configurações DHT11
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// Configurações NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

// Variáveis de tempo
unsigned long lastTime = 0;
const unsigned long timerDelay = 60000;  
#define SLEEP_TIME 60000000  

// Função para obter data e hora atual
void getFormattedTime(char* buffer) {
  timeClient.update();
  snprintf(buffer, 20, "%02d:%02d:%02d", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
}

const char* wifiManagerCSS = "<style>"
                        "body { background-color: orange; }"
                        "input[type=submit] { background-color: white; }"
                        "<h1>Configuração do Dispositivo SOMI Branco</h1>"
                        "</style>"; 

void setup() {
  Serial.begin(115200);

  // Configura WiFi Manager
  WiFiManager wifiManager;
  wifiManager.autoConnect("SOMIBranco", "12345678"); // Nome da rede temporária
  
  // Se chegou aqui, a conexão foi bem-sucedida
  Serial.println("WiFi Conectado.");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);   

  // Inicializa NTP e sensores
  timeClient.begin();
  dht.begin();
  pinMode(SENSOR_FOGO_ESQ_PIN, INPUT);
  pinMode(SENSOR_FOGO_DIR_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Acordou do Deep Sleep.");
  }

  esp_sleep_enable_timer_wakeup(SLEEP_TIME);  
}

void loop() {
  unsigned long currentTime = millis();
  
  // Verifica se é hora de coletar dados
  if (currentTime - lastTime >= timerDelay) {
    lastTime = currentTime;

    // Leitura dos sensores de chama
    float sensorFogoEsq = analogRead(SENSOR_FOGO_ESQ_PIN);
    float sensorFogoDir = analogRead(SENSOR_FOGO_DIR_PIN);
    
    // Leitura do DHT11 (temperatura e umidade)
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    
    // Leitura do MQ-2 (detecção de gases inflamáveis)
    float leituraMQ2 = analogRead(MQ2_PIN); 

    // Data e hora atual via NTP
    char dataHora[20];
    getFormattedTime(dataHora);

    // Enviando dados ao Firebase
    String path = "/SOMI_Branco/" + String(dataHora);
    Firebase.setFloat(path + "/temperatura", temperatura);
    Firebase.setFloat(path + "/umidade", umidade);
    Firebase.setFloat(path + "/leituraMQ2", leituraMQ2);
    Firebase.setFloat(path + "/sensorFogoEsq", sensorFogoEsq);
    Firebase.setFloat(path + "/sensorFogoDir", sensorFogoDir);

    Serial.println("Entrando em Deep Sleep por 1 minuto...");
    esp_deep_sleep_start();
  }
}
