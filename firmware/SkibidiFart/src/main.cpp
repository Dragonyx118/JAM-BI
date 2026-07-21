#include <esp_now.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define RELAY_PIN 4       // Pin del relè umidificatore
#define RXD2 27           // RX ESP32 <- TX DFPlayer
#define TXD2 26           // TX ESP32 -> RX DFPlayer (resistenza 1k in serie)

HardwareSerial mySerial(2);
DFRobotDFPlayerMini player;

bool umidificatoreOn = false;
uint16_t numeroTracce = 0;
bool dfplayerPronto = false;

// Comandi: action
// 1  = play traccia N (value = numero traccia)
// 2  = toggle umidificatore
// 3  = set volume (value = 0-30)
// 4  = stop
// 5  = pausa(0)/riprendi(1)
// 10 = richiesta conteggio tracce (master -> slave)
// 11 = risposta conteggio tracce (slave -> master, value = numero tracce)

typedef struct {
  uint8_t action;
  uint16_t value;
} Comando;

Comando cmdIn;
Comando cmdOut;

uint8_t masterMac[6];
bool masterMacNoto = false;

void inviaAlMaster(uint8_t action, uint16_t value) {
  if (!masterMacNoto) return;
  cmdOut.action = action;
  cmdOut.value = value;
  esp_now_send(masterMac, (uint8_t *)&cmdOut, sizeof(cmdOut));
}

void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (!masterMacNoto) {
    memcpy(masterMac, mac, 6);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (!esp_now_is_peer_exist(masterMac)) {
      esp_now_add_peer(&peerInfo);
    }
    masterMacNoto = true;
    Serial.println("Master registrato come peer");
  }

  memcpy(&cmdIn, data, sizeof(cmdIn));

  if (!dfplayerPronto && cmdIn.action != 10) {
    Serial.println("Comando ignorato: DFPlayer non pronto");
    return;
  }

  switch (cmdIn.action) {
    case 1:
      player.play(cmdIn.value);
      Serial.printf("Play traccia %d\n", cmdIn.value);
      break;

    case 2:
      umidificatoreOn = !umidificatoreOn;
      digitalWrite(RELAY_PIN, umidificatoreOn ? HIGH : LOW);
      Serial.printf("Umidificatore: %s\n", umidificatoreOn ? "ON" : "OFF");
      break;

    case 3:
      player.volume(cmdIn.value);
      Serial.printf("Volume: %d\n", cmdIn.value);
      break;

    case 4:
      player.stop();
      Serial.println("Stop");
      break;

    case 5:
      if (cmdIn.value == 0) player.pause();
      else player.start();
      break;

    case 10:
      if (dfplayerPronto) {
        numeroTracce = player.readFileCounts();
        Serial.printf("Numero tracce sulla SD: %d\n", numeroTracce);
        inviaAlMaster(11, numeroTracce);
      } else {
        Serial.println("Richiesta tracce ignorata: DFPlayer non pronto");
        inviaAlMaster(11, 0);
      }
      break;
  }
}

void stampaErroreDFPlayer(uint8_t type, int value) {
  Serial.print("Notifica DFPlayer - tipo: 0x");
  Serial.print(type, HEX);
  Serial.print(" valore: ");
  Serial.println(value);

  switch (type) {
    case DFPlayerCardInserted: Serial.println("-> SD inserita"); break;
    case DFPlayerCardRemoved: Serial.println("-> SD rimossa"); break;
    case DFPlayerCardOnline: Serial.println("-> SD online, pronta"); break;
    case DFPlayerError:
      Serial.print("-> ERRORE codice: ");
      Serial.println(value);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  delay(1000); // Tempo per l'avvio del modulo MP3-TF-16P

  Serial.println("Inizializzo DFPlayer...");

  if (!player.begin(mySerial, /*isACK=*/true, /*doReset=*/true)) {
    Serial.println("DFPlayer non trovato al primo tentativo, ritento con isACK=false...");
    delay(500);
    if (!player.begin(mySerial, /*isACK=*/false, /*doReset=*/true)) {
      Serial.println("DFPlayer non trovato, controlla i collegamenti");
      dfplayerPronto = false;
    } else {
      Serial.println("DFPlayer pronto (isACK=false)!");
      dfplayerPronto = true;
    }
  } else {
    Serial.println("DFPlayer pronto!");
    dfplayerPronto = true;
  }

  if (dfplayerPronto) {
    player.volume(20);
    delay(1000); // Delay per stabilizzare la lettura SD

    numeroTracce = player.readFileCounts();
    
    // Se fallisce, riprova un paio di volte
    int tentativi = 0;
    while (numeroTracce == 65535 && tentativi < 3) {
      Serial.println("Retry lettura conteggio tracce...");
      delay(500);
      numeroTracce = player.readFileCounts();
      tentativi++;
    }
    
    Serial.printf("Tracce trovate all'avvio: %d\n", numeroTracce);
  }

  WiFi.mode(WIFI_STA);
  Serial.print("MAC di questo slave: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore inizializzazione ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  // NOTA: Il blocco di test "player.play(1)" è stato rimosso.
  // Lo slave ora rimane in attesa dei comandi ESP-NOW inviati dal Master.
}

void loop() {
  // Gestisce le notifiche asincrone del DFPlayer
  if (player.available()) {
    stampaErroreDFPlayer(player.readType(), player.read());
  }
}