#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <esp_now.h>
#include <WiFi.h>
#include "logo_data.h"

uint8_t slaveMac[] = {0x8C, 0x94, 0xDF, 0x8B, 0x51, 0xA4};

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

// Menu Principale (Grafica Originale)
Pulsante menuPulsanti[4] = {
  {20, 170, 280, 50, "JAMMER"},        
  {20, 240, 280, 50, "HUMIDIFIER"},
  {20, 310, 280, 50, "MEDIA"},
  {20, 380, 280, 50, "HELP"}
};

// ==========================================
// GESTIONE TRACCE E PAGINAZIONE (100 TRACCE)
// ==========================================
// ==========================================
// GESTIONE TRACCE E PAGINAZIONE (119 TRACCE)
// ==========================================
const char* listaTracce[] = {
  "01. AAH 01", "02. DINOSAUR", "03. FART", "04. GAY 04", "05. MIBOMBOO 05",
  "06. NIGANIGANIGANE 06", "07. NIGGERS 07", "08. NIIIGEEEERR 08", "09. ALITLEBITOFMONIKA 09", "10. ALARME 10",
  "11. BRAINWAVE 11", "12. WIWIRIWIRIWU 12", "13. 67CHRISMAS 13", "14. SIXSEVEN 14", "15. AAAAAAAAA 15",
  "16. ALAWAAKBAR 16", "17. ALLERTAAURA 17", "18. ALLARMEFROCI 18", "19. SIGMAWOLF 19", "20. AMERICANFAT 20",
  "21. ARANSAMSAM 21", "22. VANDERLINDE 22", "23. BALLSBALLSINMYASS 23", "24. BASTARLDO 24", "25. MISCOPOUNACAPRA 25",
  "26. CREEPYMUSIC 26", "27. DIDDYSPARTY 27", "28. MINEITUP 28", "29. DJSALSAYOGURT 29", "30. NONFREGAUNCAZZO 30",
  "31. VIVAILDUCE 31", "32. EPSTINFUCKNIGGERS 32", "33. VIVAILDUCE 33", "34. FAHHHH 34", "35. PERRYL'ORNITORINCO 35",
  "36. FBIOPENUP 36", "37. FILIPPOSTAIZITTO 37", "38. FLASHBANG 38", "39. FLIPFLAPUBIDDODO 39", "40. WHATDAHELL 40",
  "41. TAKEMEFARAWAY 41", "42. COMINISMO 42", "43. TROMBA 43", "44. GRISSINBON 44", "45. AKUNAMATATARAGAZZI 45",
  "46. HASEGNATOMUSSOLINI 46", "47. HEAVEN 47", "48. HOMELANDERSONG 48", "49. RAGEBAITER 49", "50. IAMLOYALTOVILTRUM 50",
  "51. IFUAREDISABLED 51", "52. IJUSTHITTHEJACKPOT 52", "53. TRYTOGOON 53", "54. JAVASCRIPT 54", "55. IAMAGOONER 55",
  "56. JOKERNONHANINVASOILPACKISTAN 56", "57. WIGGLEINVINCIBLE 57", "58. CARSSONG 58", "59. IT'SMYMOVIE 59", "60. IGOTTHISFAHH 60",
  "61. JEFFèLUI 61", "62. TROMBANAVE 62", "63. PIOVONOPOLPETTESONG 63", "64. DIOYOGURT 64", "65. LOONEYTOONS 65",
  "66. WILSONLOSIENTO 66", "67. IMBOLLETPROOF 67", "68. TUTURUTURUTU 68", "69. MARIOKART 69", "70. MARZA 70",
  "71. KINGONJUNMASTERGOON 71", "72. METALPIPE 72", "73. MISSIONIMPOSSIBLE 73", "74. DI***CO 74", "75. IMSITTINGINTHECORNER 75",
  "76. RICKROLL 76", "77. JOHNSENA 77", "78. NIIIIIGGAAAAAA 78", "79. BENJAMIN NETANIAHU 79", "80. MILANOCENTROSOCIALE 80",
  "81. PARAPARAPA 81", "82. PARACADUTOSULLEAPALLE 82", "83. PARAPPAPARA 83", "84. TAKEMEFARAWAY 84", "85. PERRYORNITORINCO 85",
  "86. MENOHAMBURGER 86", "87. POVERICOMUNISTI 87", "88. PROGAMERMOVE 88", "89. RAAAARH 89", "90. TROPPOTRAPPITRAPPI 90",
  "91. MOUTHFART 91", "92. SAYWALLHI 92", "93. BOMBA 93", "94. SIGMAGOON18 94", "95. SKIBIDIBAMBAM 95",
  "96. 2016YOUTUBE 96", "97. SAYWALLAHI 97", "98. STEVEJOBSLIGMA 98", "99. AMBULANZA 99", "100. SOMETHINGINMYASS 100",
  "101. OHIOSONG 101", "102. PEPEPEPERE 102", "103. SLIPPINGBANANA 103", "104. ABUSODIPOTEREDI**NE 104", "105. DEJAAVU 105",
  "106. PERFECTCELL 106", "107. SONOUNESSEREUMANOTRANSFORMER 107", "108. TROMBAMILITARE 108", "109. TUNGTUNGSAHUR 109", "110. WE'LLBETHERE 110",
  "111. WETFART 111", "112. WHATISDIDDYBLUDDOING 112", "113. MASTEREPSTIEN 113", "114. DIDDYBLUD 114", "115. WHATINTHEACTUALFUCK 115",
  "116. JEWISH 116", "117. YOUWILLNEVERBREATHAGAIN 117"
};
#define TOT_TRACCE (sizeof(listaTracce) / sizeof(listaTracce[0]))

#define TRACCE_PER_PAGINA 5   // 5 tracce visibili per pagina
int scrollOffset = 0;         // Indice della prima traccia della pagina corrente
int tracciaSelezionata = -1;  // Traccia attiva (-1 nessuna)
int volumeAttuale = 20;       // Livello volume (0 - 30)

// Pulsanti Controlli Audio (Riga 1)
Pulsante btnVolGiu = {20, 325, 80, 40, "VOL -"};
Pulsante btnStop   = {120, 325, 80, 40, "STOP"};
Pulsante btnVolSu  = {220, 325, 80, 40, "VOL +"};

// Pulsanti Navigazione Pagine (Riga 2)
Pulsante btnSu            = {20, 380, 80, 40, "SU"};
Pulsante btnGiu           = {120, 380, 80, 40, "GIU"};
Pulsante btnIndietroMedia = {220, 380, 80, 40, "BACK"};

// Pulsante universale per HELP
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

// --- RENDERING SCHERMATE ---

void mostraMenuPrincipale() {
  tft.fillScreen(TFT_BLACK);
  int miniLogoX = (320 - (LOGO_COLS * 2)) / 2;
  drawMiniLogo(miniLogoX, 10);

  tft.setTextDatum(MC_DATUM); 

  for (int i = 0; i < 4; i++) {
    uint16_t colore = (i == 1 && humidifierAttivo) ? TFT_GREEN : TFT_WHITE;
    
    tft.drawRect(menuPulsanti[i].x, menuPulsanti[i].y, menuPulsanti[i].w, menuPulsanti[i].h, colore);
    
    int centroX = menuPulsanti[i].x + (menuPulsanti[i].w / 2);
    int centroY = menuPulsanti[i].y + (menuPulsanti[i].h / 2) - 3; 
    
    tft.setTextColor(colore, TFT_BLACK);
    tft.drawCentreString(menuPulsanti[i].etichetta, centroX, centroY, 4); 
  }
}

void mostraMenuMedia() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  
  // Calcolo pagina corrente e totale pagine
  int paginaCorrente = (scrollOffset / TRACCE_PER_PAGINA) + 1;
  int pagineTotali = (TOT_TRACCE + TRACCE_PER_PAGINA - 1) / TRACCE_PER_PAGINA;
  
  char titolo[30];
  snprintf(titolo, sizeof(titolo), "MEDIA (%d/%d)", paginaCorrente, pagineTotali);
  tft.drawCentreString(titolo, 160, 20, 4);
  tft.drawFastHLine(20, 42, 280, TFT_GREEN);

  // Disegna le 5 tracce della pagina
  int startY = 50;
  int boxH = 44;
  int spacing = 8;

  for (int i = 0; i < TRACCE_PER_PAGINA; i++) {
    int tracciaIndice = scrollOffset + i;
    if (tracciaIndice >= TOT_TRACCE) break; 

    int currentY = startY + i * (boxH + spacing);
    
    uint16_t coloreBox = (tracciaIndice == tracciaSelezionata) ? TFT_GREEN : TFT_WHITE;
    uint16_t coloreTesto = (tracciaIndice == tracciaSelezionata) ? TFT_GREEN : TFT_WHITE;

    tft.drawRect(20, currentY, 280, boxH, coloreBox);
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(coloreTesto, TFT_BLACK);
    tft.drawString(listaTracce[tracciaIndice], 30, currentY + (boxH / 2) - 6, 2);
  }

  tft.setTextDatum(MC_DATUM);

  // Riga 1 Controlli: VOL -, STOP, VOL +
  tft.drawRect(btnVolGiu.x, btnVolGiu.y, btnVolGiu.w, btnVolGiu.h, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString(btnVolGiu.etichetta, btnVolGiu.x + (btnVolGiu.w/2), btnVolGiu.y + (btnVolGiu.h/2) - 2, 2);

  tft.drawRect(btnStop.x, btnStop.y, btnStop.w, btnStop.h, TFT_ORANGE);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawCentreString(btnStop.etichetta, btnStop.x + (btnStop.w/2), btnStop.y + (btnStop.h/2) - 2, 2);

  tft.drawRect(btnVolSu.x, btnVolSu.y, btnVolSu.w, btnVolSu.h, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString(btnVolSu.etichetta, btnVolSu.x + (btnVolSu.w/2), btnVolSu.y + (btnVolSu.h/2) - 2, 2);

  // Riga 2 Navigazione: SU, GIU, BACK
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
  tft.setRotation(2); // Rotazione display originale
  
  drawAnimatedLogo();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Loading JAM-BI...", 160, 430, 2);
  delay(1500);

  // --- OPTIMIZATION RANGE TRANSMISSION (TX POWER MAX) ---
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Imposta la massima potenza del chip radio ESP32

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

    // FILTRO TOUCH: Ignora tocchi troppo leggeri (rumore elettrico)
    if (p.z > 300 && p.z < 3800) {
      
      // MEDIA CAMPIONATA SU 3 LETTURE: Elimina salti imprevedibili del cursore
      int sumX = p.x, sumY = p.y;
      for (int k = 0; k < 2; k++) {
        TS_Point pTemp = ts.getPoint();
        sumX += pTemp.x;
        sumY += pTemp.y;
      }
      int xGrezzo = sumY / 3; 
      int yGrezzo = sumX / 3;

      // MAPPATURA ORIENTAMENTO DISPLAY
      int x_mappato = map(xGrezzo, 200, 3800, 320, 0); 
      int y_mappato = map(yGrezzo, 200, 3800, 0, 480); 

      // Mantiene le coordinate nei limiti fisici dello schermo
      x_mappato = constrain(x_mappato, 0, 320);
      y_mappato = constrain(y_mappato, 0, 480);

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
              scrollOffset = 0; 
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
      // LOGICA STATO 2: MENU MEDIA (A PAGINE)
      // ==========================================
      else if (sistemaStato == 2) {
        
        // 1. Controlla selezione tracce visibili
        int startY = 50;
        int boxH = 44;
        int spacing = 8;
        bool azionato = false;

        for (int i = 0; i < TRACCE_PER_PAGINA; i++) {
          int tracciaIndice = scrollOffset + i;
          if (tracciaIndice >= TOT_TRACCE) break;

          int currentY = startY + i * (boxH + spacing);

          if (x_mappato >= 20 && x_mappato <= 300 && y_mappato >= currentY && y_mappato <= (currentY + boxH)) {
            tracciaSelezionata = tracciaIndice;
            Serial.printf("Riproduco traccia lista: %d (%s)\n", tracciaIndice, listaTracce[tracciaIndice]);
            
            // OFFSET +2: Invia valore 2 allo Slave per la prima traccia
            inviaAlSlave(1, tracciaIndice + 2); 
            
            mostraMenuMedia();
            azionato = true;
            break;
          }
        }

        if (!azionato) {
          // 2. Controlla VOL -
          if (x_mappato >= btnVolGiu.x && x_mappato <= (btnVolGiu.x + btnVolGiu.w) &&
              y_mappato >= btnVolGiu.y && y_mappato <= (btnVolGiu.y + btnVolGiu.h)) {
            if (volumeAttuale > 0) {
              volumeAttuale -= 2;
              if (volumeAttuale < 0) volumeAttuale = 0;
              inviaAlSlave(3, volumeAttuale); // Action 3: Volume
              Serial.printf("Volume inviato: %d\n", volumeAttuale);
            }
          }
          
          // 3. Controlla STOP
          else if (x_mappato >= btnStop.x && x_mappato <= (btnStop.x + btnStop.w) &&
                   y_mappato >= btnStop.y && y_mappato <= (btnStop.y + btnStop.h)) {
            tracciaSelezionata = -1;
            inviaAlSlave(4, 0); // Action 4: Stop
            Serial.println("Comando STOP inviato");
            mostraMenuMedia();
          }

          // 4. Controlla VOL +
          else if (x_mappato >= btnVolSu.x && x_mappato <= (btnVolSu.x + btnVolSu.w) &&
                   y_mappato >= btnVolSu.y && y_mappato <= (btnVolSu.y + btnVolSu.h)) {
            if (volumeAttuale < 30) {
              volumeAttuale += 2;
              if (volumeAttuale > 30) volumeAttuale = 30;
              inviaAlSlave(3, volumeAttuale); // Action 3: Volume
              Serial.printf("Volume inviato: %d\n", volumeAttuale);
            }
          }

          // 5. Controlla SU (Cambia Pagina Indietro)
          else if (x_mappato >= btnSu.x && x_mappato <= (btnSu.x + btnSu.w) &&
                   y_mappato >= btnSu.y && y_mappato <= (btnSu.y + btnSu.h)) {
            if (scrollOffset >= TRACCE_PER_PAGINA) {
              scrollOffset -= TRACCE_PER_PAGINA;
              mostraMenuMedia();
            }
          }
          
          // 6. Controlla GIU (Cambia Pagina Avanti)
          else if (x_mappato >= btnGiu.x && x_mappato <= (btnGiu.x + btnGiu.w) &&
                   y_mappato >= btnGiu.y && y_mappato <= (btnGiu.y + btnGiu.h)) {
            if (scrollOffset + TRACCE_PER_PAGINA < TOT_TRACCE) {
              scrollOffset += TRACCE_PER_PAGINA;
              mostraMenuMedia();
            }
          }

          // 7. Controlla BACK
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

      delay(180); // Ritardo anti-rimbalzo ottimizzato
    }
  }
}