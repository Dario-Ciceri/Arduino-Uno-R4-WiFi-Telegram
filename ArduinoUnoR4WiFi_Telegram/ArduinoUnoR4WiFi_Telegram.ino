// Dario Ciceri
// Codice per controllare schede Arduino Uno R4 WiFi tramite Telegram
// Profilo Instragram: https://www.instagram.com/_dario.ciceri_/
// Pagina GitHub: https://github.com/Dario-Ciceri
// Canale YouTube: https://www.youtube.com/channel/UCuPuHsNjWX7huiztYu9ROQA

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <UniversalTelegramBot.h>
#include "si7021.h"
#include "PIR.h"

// Dimensioni della matrice LED
#define MAX_Y 8
#define MAX_X 12

// Oggetto per la gestione della matrice LED
ArduinoLEDMatrix matrix;

// Matrice bidimensionale per rappresentare lo stato dei LED sulla matrice
uint8_t grid[MAX_Y][MAX_X] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

// Credenziali di rete WiFi
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PSK"

// Token del bot Telegram (ottenuto da Botfather)
#define BOT_API_TOKEN "YOUR_BOT_TOKEN"

// Pin per LED e relè
#define LED_PIN 9
#define RELAY_PIN 10

// Variabili per gestire l'ID della chat, l'ID del messaggio e il nome del mittente
String chat_id = "";
int message_id = -1;
String from_name = "Unknown";

// Variabile booleana per ignorare lo stato del sensore PIR
bool ignorePirState = false;

// Costante per l'intervallo di pausa tra le ricezioni dei messaggi
const unsigned long interval = 1000;

// Variabile per la gestione del tempo
unsigned long last_call = 0;

// Oggetto per la gestione del client WiFi sicuro
WiFiSSLClient securedClient;

// Oggetto per la gestione del bot Telegram
UniversalTelegramBot bot(BOT_API_TOKEN, securedClient);

// Stringa JSON per la tastiera inline
String inlineKeyboardJson = "";

// Funzione per gestire i messaggi della chat Telegram
void handleMessages(int num_new_messages) {
  for (int i = 0; i < num_new_messages; i++) {
    // Ottenimento dell'ID e del testo del messaggio
    message_id = bot.messages[i].message_id;
    chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    from_name = bot.messages[i].from_name;

    // Se il nome del mittente è vuoto, impostalo su "Unknown"
    if (from_name == "") {
      from_name = "Unknown";
    }

    // Gestione dei comandi ricevuti
    if (text == "/start") {
      // Aggiorna la tastiera inline
      updateInlineKeyboard();

      // Invia il messaggio di benvenuto con la tastiera inline
      bot.sendMessageWithInlineKeyboard(chat_id, "Ciao, " + (from_name == "Unknown" ? "" : from_name) + ", scegli una delle opzioni qui sotto", "Markdown", inlineKeyboardJson);
    } else if (text == "/toggleLED") {
      // Funzione per invertire lo stato del LED
      toggleLED();
    } else if (text == "/reportTempAndHum" && hasTHSensor) {
      // Aggiorna la tastiera inline
      updateInlineKeyboard();

      // Invia il messaggio con la tastiera inline aggiornata
      bot.sendMessageWithInlineKeyboard(chat_id, "Ciao, " + from_name + ", scegli una delle opzioni qui sotto", "Markdown", inlineKeyboardJson, message_id);
    } else if (text == "/toggleRelay") {
      // Funzione per invertire lo stato del relè
      toggleRelay();
    }

    // Stampa dei dettagli sulla console seriale per il debug
    Serial.println(from_name);
    Serial.println(chat_id);
    Serial.println(text);
  }
}

// Funzione di inizializzazione
void setup() {
  // Inizializzazione della comunicazione seriale
  Serial.begin(115200);

  // Impostazione dei pin per LED e relè come output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Inizializzazione della matrice LED
  matrix.begin();

  // Inizializzazione dei sensori Si7021 e PIR
  initSi7120();
  initPIR();

  // Connessione alla rete WiFi
  connectToWiFi();

  // Stampa dell'indirizzo IP una volta connesso
  Serial.print("WiFi connected. IP Address: ");
  Serial.println(WiFi.localIP());

  // Configurazione della matrice LED con un testo statico
  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  const char text[] = "ON";
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText();
  matrix.endDraw();
}

// Loop principale
void loop() {
  // Esecuzione delle operazioni ogni intervallo di tempo
  if (millis() - last_call > interval) {
    // Aggiornamento della tastiera inline e invio del messaggio principale
    updateInlineKeyboard();
    bot.sendMessageWithInlineKeyboard(chat_id, "Ciao, " + from_name + ", scegli una delle opzioni qui sotto", "Markdown", inlineKeyboardJson, message_id);

    // Aggiornamento della lista dei messaggi e gestione dei nuovi messaggi
    int num_new_messages = bot.getUpdates(bot.last_message_received + 1);
    while (num_new_messages) {
      Serial.println("Message received");
      handleMessages(num_new_messages);
      num_new_messages = bot.getUpdates(bot.last_message_received + 1);
    }

    // Aggiornamento del tempo dell'ultima esecuzione
    last_call = millis();
  }
}

// Funzione per connettersi alla rete WiFi
void connectToWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi network ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Impostazione del certificato di sicurezza per la connessione a Telegram
  securedClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  byte i = 0, j = 0, dir = 1;

  // Attendi la connessione alla rete WiFi
  while (WiFi.status() != WL_CONNECTED) {
    grid[i][j] = !grid[i][j];
    j += dir;
    if (j > 11 || j < 0) {
      dir *= -1;
      j += dir;
      i++;
    }
    if (i > 7) {
      i = 0;
      if (dir == 1) {
        dir = -1;
        j = 11;
      } else {
        dir = 1;
        j = 0;
      }
    }
    matrix.renderBitmap(grid, 8, 12);
    delay(100);
  }
}

// Funzione per aggiornare la tastiera inline
void updateInlineKeyboard() {
  inlineKeyboardJson =
    "["
      "[{ \"text\" : " + String((digitalRead(LED_PIN) ? "\"Spegni LED\"" : "\"Accendi LED\"")) + ", \"callback_data\" : \"/toggleLED\" }],"
      "[{ \"text\" : " + String((digitalRead(RELAY_PIN) ? "\"Spegni relè\"" : "\"Accendi relè\"")) + ", \"callback_data\" : \"/toggleRelay\" }],"
      "[{ \"text\" : \"Temperatura: " + String(readTemperature()) + "°C\\nUmidità: " + String(readHumidity()) + "%\", \"callback_data\" : \"/reportTempAndHum\" }],"
      "[{ \"text\" : \"Movimenti rilevati: " + String(readPIR() ? "sì!" : "no!") + "\", \"callback_data\" : \"/motionStatus\" }]"
    "]";
}

// Funzione per invertire lo stato del LED
void toggleLED() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  // Aggiorna la tastiera inline con lo stato del LED
  updateInlineKeyboard();
  // Invia il messaggio con la tastiera inline aggiornata
  bot.sendMessageWithInlineKeyboard(chat_id, "Ciao, " + from_name + ", scegli una delle opzioni qui sotto", "Markdown", inlineKeyboardJson, message_id);
}

// Funzione per invertire lo stato del relè
void toggleRelay() {
  digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
  // Aggiorna la tastiera inline con lo stato del relè
  updateInlineKeyboard();
  // Invia il messaggio con la tastiera inline aggiornata
  bot.sendMessageWithInlineKeyboard(chat_id, "Ciao, " + from_name + ", scegli una delle opzioni qui sotto", "Markdown", inlineKeyboardJson, message_id);
}
