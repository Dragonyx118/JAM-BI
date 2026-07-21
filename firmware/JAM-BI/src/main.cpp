#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <esp_now.h>
#include <WiFi.h>
#include "logo_data.h"

// ==========================================
// ⚠️ CONFIGURAZIONE MAC ADDRESS SLAVE
// ==========================================
uint8_t slaveMac[] = {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x00}; 

// Pin del Touch (Bus HSPI)
#define TOUCH_CS_PIN  27
#define TOUCH_TDO_PIN 12
#define TOUCH_TDI_PIN 13
#define TOUCH_CLK_PIN 14

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

// Stati del sistema: 0=Boot, 1=Menu Principale, 2=Menu Media, 3=Menu Help
int sistemaStato = 0; 
bool humidifierAttivo = false; 

// Struttura dati per i Comandi ESP-NOW
typedef struct {
  uint8_t action;
  uint16_t value;
} Comando;

Comando cmdOut;
Comando cmdIn;

struct Pulsante {
  int x;
  int y;
  int w;
  int h;
  const char* etichetta;
};

// Menu Principale
Pulsante menuPulsanti[4] = {
  {20, 170, 280, 50, "JAMMER"},        
  {20, 240, 280, 50, "HUMIDIFIER"},
  {20, 310, 280, 50, "MEDIA"},
  {20, 380, 280, 50, "HELP"}
};

// ==========================================
// GESTIONE TRACCE E SCROLL (MENU MEDIA)
// ==========================================
// Puoi aggiungere fino a 100 o più tracce qui dentro. L'interfaccia le gestirà automaticamente.
const char* listaTracce[] = {
  "01. TITOLO CANZONE MOLTO LUNGO TRACCIA A",
  "02. SOUND TRACCIA B",
  "03. TEST AUDIO NUMERO TRE",
  "04. TRACCIA 04 VOLUME MAX",
  "05. EFFETTO SONORO SPECIALE 05",
  "06. TRACCIA EXTRA 06",
  "07. SOUND TRACCIA G",
  "08. AUDIO TRACK NUMERO OTTO",
  "09. PLAYLIST SOUND 09",
  "10. ULTIMA TRACCIA DI TEST 10"
};
#define TOT_TRACCE (sizeof(listaTracce) / sizeof(listaTracce[0]))

// Configurazione layout griglia visibile per i Media
#define TRACCE_VISIBILI 5     // Quante tracce mostrare contemporaneamente a schermo
int scrollOffset = 0;         // Indice della prima traccia visibile a schermo
int tracciaSelezionata = -1;  // Traccia attualmente in riproduzione (-1 nessuno)

// Pulsanti di navigazione per lo Scroll
Pulsante btnSu = {20, 390, 80, 40, "SU"};
Pulsante btnGiu = {120, 390, 80, 40, "GIU"};
Pulsante btnIndietroMedia = {220, 390, 80, 40, "BACK"};

// Pulsante universale per la schermata HELP
Pulsante pulsanteIndietroHelp = {20, 400, 280, 50, "INDIETRO"};

// --- FUNZIONI DI COMUNICAZIONE ---
void inviaAlSlave(uint8_t action, uint16_t value) {
  cmdOut.action = action;
  cmdOut.value = value;
  esp_now_send(slaveMac, (uint8_t *)&cmdOut, sizeof(cmdOut));
}

void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  memcpy(&cmdIn, data, sizeof(cmdIn));
  Serial.printf("[ESP-NOW] Dati dallo Slave: Action %d, Value %d\n", cmdIn.action, cmdIn.value);
}

// --- FUNZIONI GRAFICHE ---
void drawAnimatedLogo() {
  tft.fillScreen(TFT_BLACK);
  int scala = 3; 
  int offsetX = (320 - (LOGO_COLS * scala)) / 2;
  int offsetY = (480 - (LOGO_ROWS * scala)) / 2 - 20;

  for (int r = 0; r < LOGO_ROWS; r++) {
    for (int c = 0; c < LOGO_COLS; c++) {
      uint8_t pixelType = pgm_read_byte(&logoData[r * LOGO_COLS + c]);
      if (pixelType != 0) {
        uint16_t colore = (pixelType == 1) ? tft.color565(100, 100, 100) : TFT_GREEN;
        tft.fillRect(offsetX + (c * scala), offsetY + (r * scala), scala, scala, colore);
      }
    }
    delay(12); 
  }
}

void drawMiniLogo(int offsetX, int offsetY) {
  int scala = 2;
  for (int r = 0; r < LOGO_ROWS; r++) {
    for (int c = 0; c < LOGO_COLS; c++) {
      uint8_t pixelType = pgm_read_byte(&logoData[r * LOGO_COLS + c]);
      if (pixelType != 0) {
        uint16_t colore = (pixelType == 1) ? tft.color565(100, 100, 100) : TFT_GREEN;
        tft.fillRect(offsetX + (c * scala), offsetY + (r * scala), scala, scala, colore);
      }
    }
  }
}

// --- RENDERING DELLE SCHERMATE ---

void mostraMenuPrincipale() {
  tft.fillScreen(TFT_BLACK);
  int miniLogoX = (320 - (LOGO_COLS * 2)) / 2;
  drawMiniLogo(miniLogoX, 10);

  tft.setTextDatum(MC_DATUM); 

  for (int i = 0; i < 4; i++) {
    uint16_t colore = (i == 1 && humidifierAttivo) ? TFT_GREEN : TFT_WHITE;
    
    tft.drawRect(menuPulsanti[i].x, menuPulsanti[i].y, menuPulsanti[i].w, menuPulsanti[i].h, colore);
    
    int centroX = menuPulsanti[i].x + (menuPulsanti[i].w / 2);
    // Corretto offset verticale (-3) per centrare perfettamente il Font 4 nel rettangolo
    int centroY = menuPulsanti[i].y + (menuPulsanti[i].h / 2) - 3; 
    
    tft.setTextColor(colore, TFT_BLACK);
    tft.drawCentreString(menuPulsanti[i].etichetta, centroX, centroY, 4); 
  }
}

void mostraMenuMedia() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("MEDIA SELECT", 160, 30, 4);
  tft.drawFastHLine(20, 60, 280, TFT_GREEN);

  // Disegna le 5 tracce visibili in base allo scrollOffset attuale
  int startY = 80;
  int boxH = 50;
  int spacing = 10;

  for (int i = 0; i < TRACCE_VISIBILI; i++) {
    int tracciaIndice = scrollOffset + i;
    
    // Se la lista è finita (es. ci sono solo 7 tracce totali), non disegnare i box successivi
    if (tracciaIndice >= TOT_TRACCE) break; 

    int currentY = startY + i * (boxH + spacing);
    
    // Se questa è la traccia attualmente attiva/selezionata, illumina il box di verde
    uint16_t coloreBox = (tracciaIndice == tracciaSelezionata) ? TFT_GREEN : TFT_WHITE;
    uint16_t coloreTesto = (tracciaIndice == tracciaSelezionata) ? TFT_GREEN : TFT_WHITE;

    tft.drawRect(20, currentY, 280, boxH, coloreBox);
    
    // Rimpicciolito a Font 2 per titoli lunghi e spostato a sinistra (TL_DATUM) per non tagliare le scritte
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(coloreTesto, TFT_BLACK);
    
    // Taglia la stringa se è troppo lunga per il display, inserendola a X=30 e Y centrata nel box
    tft.drawString(listaTracce[tracciaIndice], 30, currentY + (boxH / 2) - 6, 2);
  }

  // Disegna i pulsanti di controllo in basso (SU, GIÙ, BACK)
  tft.setTextDatum(MC_DATUM);
  
  tft.drawRect(btnSu.x, btnSu.y, btnSu.w, btnSu.h, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(btnSu.etichetta, btnSu.x + (btnSu.w/2), btnSu.y + (btnSu.h/2) - 2, 2);

  tft.drawRect(btnGiu.x, btnGiu.y, btnGiu.w, btnGiu.h, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(btnGiu.etichetta, btnGiu.x + (btnGiu.w/2), btnGiu.y + (btnGiu.h/2) - 2, 2);

  tft.drawRect(btnIndietroMedia.x, btnIndietroMedia.y, btnIndietroMedia.w, btnIndietroMedia.h, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawCentreString(btnIndietroMedia.etichetta, btnIndietroMedia.x + (btnIndietroMedia.w/2), btnIndietroMedia.y + (btnIndietroMedia.h/2) - 2, 2);
}

void mostraMenuHelp() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("HELP & INFO", 160, 40, 4);
  tft.drawFastHLine(20, 75, 280, TFT_YELLOW);

  tft.setTextDatum(TL_DATUM); 
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GUIDA DI UTILIZZO:", 20, 100, 2);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("- HUMIDIFIER:", 20, 140, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("  Attiva/Disattiva il rele' Slave.", 20, 165, 2);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("- MEDIA:", 20, 205, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("  Scegli le tracce audio dalla SD.", 20, 230, 2);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("- JAMMER:", 20, 270, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("  Opzione futura di controllo.", 20, 295, 2);

  // Bottone Indietro
  tft.setTextDatum(MC_DATUM);
  tft.drawRect(pulsanteIndietroHelp.x, pulsanteIndietroHelp.y, pulsanteIndietroHelp.w, pulsanteIndietroHelp.h, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawCentreString(pulsanteIndietroHelp.etichetta, pulsanteIndietroHelp.x + (pulsanteIndietroHelp.w/2), pulsanteIndietroHelp.y + (pulsanteIndietroHelp.h/2) - 3, 4);
}

// --- SETUP E LOOP ---

void setup() {
  Serial.begin(115200);
  delay(500);

  tft.init();
  tft.setRotation(2); 
  
  drawAnimatedLogo();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Loading JAM-BI...", 160, 430, 2);
  delay(1500);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;

  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, slaveMac, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  touchSPI.begin(TOUCH_CLK_PIN, TOUCH_TDO_PIN, TOUCH_TDI_PIN, TOUCH_CS_PIN);
  ts.begin(touchSPI);

  sistemaStato = 1;
  mostraMenuPrincipale();
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    int x_mappato = map(p.y, 200, 3800, 320, 0); 
    int y_mappato = map(p.x, 200, 3800, 0, 480); 

    // ==========================================
    // LOGICA STATO 1: MENU PRINCIPALE
    // ==========================================
    if (sistemaStato == 1) {
      for (int i = 0; i < 4; i++) {
        if (x_mappato >= menuPulsanti[i].x && x_mappato <= (menuPulsanti[i].x + menuPulsanti[i].w) &&
            y_mappato >= menuPulsanti[i].y && y_mappato <= (menuPulsanti[i].y + menuPulsanti[i].h)) {
          
          if (i == 0) {
            Serial.println("JAMMER premuto (No action)");
          } 
          else if (i == 1) {
            humidifierAttivo = !humidifierAttivo;
            inviaAlSlave(2, humidifierAttivo ? 1 : 0); 
            mostraMenuPrincipale();
          }
          else if (i == 2) {
            sistemaStato = 2;
            scrollOffset = 0; // Reset scroll all'apertura
            mostraMenuMedia();
          }
          else if (i == 3) {
            sistemaStato = 3;
            mostraMenuHelp();
          }
          break; 
        }
      }
    }
    
    // ==========================================
    // LOGICA STATO 2: MENU MEDIA (CON SCROLL)
    // ==========================================
    else if (sistemaStato == 2) {
      
      // 1. Controlla pressione sulle 5 tracce visibili
      int startY = 80;
      int boxH = 50;
      int spacing = 10;
      bool tracciaPremuta = false;

      for (int i = 0; i < TRACCE_VISIBILI; i++) {
        int tracciaIndice = scrollOffset + i;
        if (tracciaIndice >= TOT_TRACCE) break;

        int currentY = startY + i * (boxH + spacing);

        if (x_mappato >= 20 && x_mappato <= 300 && y_mappato >= currentY && y_mappato <= (currentY + boxH)) {
          tracciaSelezionata = tracciaIndice;
          Serial.printf("Riproduco traccia SD: %d (%s)\n", tracciaIndice, listaTracce[tracciaIndice]);
          
          // Invia allo slave l'indice corretto della traccia (es: da 0 a 99)
          inviaAlSlave(1, tracciaIndice); 
          
          // Aggiorna la grafica per mostrare la selezione verde
          mostraMenuMedia();
          tracciaPremuta = true;
          break;
        }
      }

      if (!tracciaPremuta) {
        // 2. Controlla pulsante SU (Scorri in alto)
        if (x_mappato >= btnSu.x && x_mappato <= (btnSu.x + btnSu.w) &&
            y_mappato >= btnSu.y && y_mappato <= (btnSu.y + btnSu.h)) {
          if (scrollOffset > 0) {
            scrollOffset--;
            mostraMenuMedia();
          }
        }
        
        // 3. Controlla pulsante GIU (Scorri in basso)
        else if (x_mappato >= btnGiu.x && x_mappato <= (btnGiu.x + btnGiu.w) &&
                 y_mappato >= btnGiu.y && y_mappato <= (btnGiu.y + btnGiu.h)) {
          // Permette lo scroll solo se ci sono altre tracce sotto da mostrare
          if (scrollOffset + TRACCE_VISIBILI < TOT_TRACCE) {
            scrollOffset++;
            mostraMenuMedia();
          }
        }

        // 4. Controlla pulsante BACK
        else if (x_mappato >= btnIndietroMedia.x && x_mappato <= (btnIndietroMedia.x + btnIndietroMedia.w) &&
                 y_mappato >= btnIndietroMedia.y && y_mappato <= (btnIndietroMedia.y + btnIndietroMedia.h)) {
          sistemaStato = 1;
          mostraMenuPrincipale();
        }
      }
    }

    // ==========================================
    // LOGICA STATO 3: MENU HELP
    // ==========================================
    else if (sistemaStato == 3) {
      if (x_mappato >= pulsanteIndietroHelp.x && x_mappato <= (pulsanteIndietroHelp.x + pulsanteIndietroHelp.w) &&
          y_mappato >= pulsanteIndietroHelp.y && y_mappato <= (pulsanteIndietroHelp.y + pulsanteIndietroHelp.h)) {
        sistemaStato = 1;
        mostraMenuPrincipale();
      }
    }

    delay(200); // Anti-rimbalzo per il tocco
  }
}