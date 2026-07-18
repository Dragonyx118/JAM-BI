# JAM-BI 🛰️

![GitHub repo size](https://img.shields.io/github/repo-size/Dragonyx118/JAM-BI?color=blue)
![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)
![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-teal.svg)
![Protocol: ESP--NOW](https://img.shields.io/badge/Protocol-ESP--NOW-orange.svg)
![Status: Initial Testing](https://img.shields.io/badge/Status-Initial%20Testing-green.svg)

**JAM-BI** è un ecosistema firmware basato su microcontrollori **ESP32** progettato per il controllo remoto wireless di periferiche multimediali e attuatori. Il sistema sfrutta il protocollo ad alte prestazioni **ESP-NOW** per far comunicare un'unità centrale (Master) dotata di interfaccia grafica Touchscreen con uno o più nodi periferici (Client/Slave).

## 📁 Architettura del Progetto

Il cuore del codice si trova all'interno della cartella `firmware/`, divisa nelle seguenti componenti principali per i test iniziali:

### 1. JAM-BI Base Master (Interfaccia Utente)
Il Master gestisce un display **TFT con controller grafico XPT2046 Touchscreen** (connesso tramite bus SPI/HSPI).
* **Boot Animation**: All'avvio mostra un'animazione custom pixel-art che renderizza il logo aziendale/di progetto.
* **UI Custom**: Utilizza un motore di rendering del testo pixel-by-pixel ottimizzato per far entrare stringhe lunghe nei pulsanti hardware.
* **Menu Touch**: Fornisce un'interfaccia a 4 pulsanti principali:
  * `JAMMER`
  * `UMIDIFICATORE`
  * `MEDIA`
  * `HELP`
* **Calibrazione**: Mappatura millimetrica degli assi del touchscreen per la modalità verticale (`Rotation 2`).

### 2. SkibidiFart Base Client (Periferica Attuatrice)
Lo Slave riceve i pacchetti dati strutturati dal Master e mappa le azioni sull'hardware locale. Gestisce:
* **Audio DFPlayer Mini**: Controllo completo di un lettore MP3-TF-16P via seriale hardware (`HardwareSerial 2`), con gestione del volume, play/pausa, stop, inserimento/rimozione della scheda SD e tracciamento dinamico del numero di tracce presenti.
* **Relè Umidificatore**: Controllo dello stato On/Off di un carico a relè (configurato sul pin `GPIO 4`).
* **Auto-Pairing**: Registrazione automatica del MAC Address del Master alla ricezione del primo pacchetto utile.

---

## 📡 Protocollo di Comunicazione (ESP-NOW)

I due moduli si scambiano una struttura dati fissa chiamata `Comando`:

```cpp
typedef struct {
  uint8_t action;   // ID del Comando (es. 1=Play, 2=Toggle Relè, 3=Volume...)
  uint16_t value;   // Valore numerico associato (es. ID traccia, livello volume)
} Comando;