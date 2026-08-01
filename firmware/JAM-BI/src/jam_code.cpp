#include <SPI.h>
#include "RF24.h"

// 1. Dichiarazione GLOBALE: ora 'radio' è visibile in TUTTO il codice
RF24 radio(22, 21); // Pin CE, CSN

// 2. Dichiarazione del canale (i)
uint8_t i = 45; // Numero del canale radio (0-125)

void initAntennaAndJamming() {
  // Inizializza il modulo RF24
  if (radio.begin()) {
    Serial.println("Modulo RF24 trovato!");
  } else {
    Serial.println("ERRORE: Modulo RF24 non risponde.");
  }
  
  delay(1000);

  // Configurazione dei parametri RF24
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setPayloadSize(5);
  radio.setAddressWidth(3);
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setDataRate(RF24_2MBPS);
  radio.setCRCLength(RF24_CRC_DISABLED);

  Serial.println("Antenna configurata.");
}

void startJamming() {
  // Ora 'radio' e 'i' sono visibili correttamente
  radio.startConstCarrier(RF24_PA_MAX, i); 
  Serial.println("Portante continua avviata...");
}

void stopJamming() {
  // Funzione per fermare la trasmissione continua
  radio.stopConstCarrier(); 
  Serial.println("Portante continua fermata.");
}

void setup() {
  Serial.begin(115200);
  initAntennaAndJamming();
}

void loop() {
  // Logica di test
}