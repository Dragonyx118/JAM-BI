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

// Stati del sistema: 0=Boot, 1=Menu Principale, 2=Menu Media, 3=Menu Help, 4=Jammer, 5=Humidifier
int sistemaStato = 0; 
bool humidifierAttivo = false; 
int bananaPotenza = 0; // 0=OFF, 1=Potenza1, 2=Potenza2+, 3=Potenza Massima
float faseOnda = 0.0;          // Fase corrente dell'onda (per l'animazione)
unsigned long ultimoTremore = 0; // Timestamp ultimo aggiornamento tremolio

// --- VARIABILI SCHERMATA HUMIDIFIER ---
// --- VARIABILI SCHERMATA HUMIDIFIER ---
int humidifierTimerIndex = 0;                 // Indice del preset timer selezionato
const int humidifierPresets[5] = {0, 5, 10, 15, 30}; // Secondi ridotti (0 = infinito)
const char* humidifierPresetLabel[5] = {"INFINITO", "5 sec", "10 sec", "15 sec", "30 sec"};
int humidifierTimerSec = 0;                   // Secondi correnti (0 = nessuno spegnimento automatico)
unsigned long humidifierAccensioneMillis = 0; // Timestamp di accensione (per countdown)
unsigned long ultimaParticella = 0;            // Timestamp ultimo aggiornamento animazione particelle

#define NUM_PARTICELLE 10
struct Particella {
  float x, y;
  float velocita;
  bool attiva;
};
Particella particelle[NUM_PARTICELLE];

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

// --- PULSANTI SCHERMATA HUMIDIFIER ---
Pulsante btnHumOn    = {30, 260, 120, 55, "ON"};
Pulsante btnHumOff   = {170, 260, 120, 55, "OFF"};
Pulsante btnHumTimer = {30, 325, 260, 50, "TIMER"};
Pulsante btnHumBack  = {30, 390, 260, 50, "INDIETRO"};

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

// --- ELEMENTI PAGINA BANANA (ONDA + CONTROLLO POTENZA) ---
Pulsante rettangoloOnda = {20, 50, 280, 200, ""}; // Riquadro dell'onda (circa meta' schermo)

// 4 pulsanti potenza in una riga sotto il riquadro onda
Pulsante bananaBtn[4] = {
  {20,  280, 64, 60, "OFF"},
  {92,  280, 64, 60, "POT 1"},
  {164, 280, 64, 60, "POT 2+"},
  {236, 280, 64, 60, "MAX"}
};

Pulsante btnIndietroBanana = {20, 360, 280, 50, "INDIETRO"};

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
  tft.setTextFont(1);      // Font pixel GLCD, stesso stile del logo
  tft.setTextSize(3);      // Ingrandito per abbinarsi alle dimensioni dei pulsanti

  for (int i = 0; i < 4; i++) {
    uint16_t colore = (i == 1 && humidifierAttivo) ? TFT_GREEN : TFT_WHITE;
    
    tft.drawRect(menuPulsanti[i].x, menuPulsanti[i].y, menuPulsanti[i].w, menuPulsanti[i].h, colore);
    
    int centroX = menuPulsanti[i].x + (menuPulsanti[i].w / 2);
    int centroY = menuPulsanti[i].y + (menuPulsanti[i].h / 2) - 3; 
    
    tft.setTextColor(colore, TFT_BLACK);
    tft.drawCentreString(menuPulsanti[i].etichetta, centroX, centroY, 1); 
  }
  tft.setTextSize(1);      // Ripristino la scala di default
}

// ==========================================================
// SCHERMATA HUMIDIFIER: icona pixel-art + ON/OFF + TIMER
// ==========================================================

// ---- Coordinate dell'icona (facili da ritoccare a mano se sul display reale
//      qualcosa risultasse leggermente storto o sovrapposto) ----
#define HUM_BASE_X   50
#define HUM_BASE_Y   208
#define HUM_BASE_W   220
#define HUM_BASE_H   16

#define HUM_PCB_X    122
#define HUM_PCB_Y    182
#define HUM_PCB_W    72
#define HUM_PCB_H    30

#define HUM_BODY_X   140
#define HUM_BODY_Y   118
#define HUM_BODY_W   50
#define HUM_BODY_H   45

#define HUM_ROD_STARTX 178
#define HUM_ROD_STARTY 130
#define HUM_ROD_STEPS  8
#define HUM_ROD_DX     -7.2
#define HUM_ROD_DY     -8.7
#define HUM_ROD_SIZE   15

#define HUM_CAP_R      15

// Disegna l'icona del diffusore ispirata alla foto: base in legno, schedina con
// filo rosso, corpo nero, bastoncino bianco a gradini e cappuccio nero del
// trasduttore ben visibile in cima.
void disegnaIconaUmidificatore() {
  // Base scura (mobile/legno) su cui poggia il diffusore
  tft.fillRect(HUM_BASE_X, HUM_BASE_Y, HUM_BASE_W, HUM_BASE_H, tft.color565(35, 35, 35));

  // Schedina elettronica (come nella foto), con due fori ramati e un connettore bianco
  tft.fillRoundRect(HUM_PCB_X, HUM_PCB_Y, HUM_PCB_W, HUM_PCB_H, 4, tft.color565(15, 60, 15));
  tft.fillCircle(HUM_PCB_X + 14, HUM_PCB_Y + 22, 3, tft.color565(190, 150, 70));
  tft.fillCircle(HUM_PCB_X + 40, HUM_PCB_Y + 22, 3, tft.color565(190, 150, 70));
  tft.fillRect(HUM_PCB_X + 46, HUM_PCB_Y + 8, 20, 12, TFT_WHITE);

  // Filo rosso che sale dalla schedina fino al corpo nero del diffusore
  tft.drawLine(HUM_PCB_X + 20, HUM_PCB_Y, HUM_BODY_X + 12, HUM_BODY_Y + HUM_BODY_H, TFT_RED);
  tft.drawLine(HUM_PCB_X + 22, HUM_PCB_Y, HUM_BODY_X + 14, HUM_BODY_Y + HUM_BODY_H, TFT_RED);

  // Corpo nero arrotondato del diffusore
  tft.fillRoundRect(HUM_BODY_X, HUM_BODY_Y, HUM_BODY_W, HUM_BODY_H, 10, TFT_BLACK);

  // Bastoncino bianco diagonale "a gradini" stile pixel-art
  int lastPx = 0, lastPy = 0;
  for (int i = 0; i < HUM_ROD_STEPS; i++) {
    int px = (int)(HUM_ROD_STARTX + HUM_ROD_DX * i);
    int py = (int)(HUM_ROD_STARTY + HUM_ROD_DY * i);
    tft.fillRect(px, py, HUM_ROD_SIZE, HUM_ROD_SIZE, TFT_WHITE);
    lastPx = px;
    lastPy = py;
  }

  // --- Il "cosino nero" in cima (il trasduttore piezoelettrico della foto) ---
  // Posizionato esattamente alla fine del bastoncino bianco, reso piu' grande
  // e definito rispetto alla versione precedente cosi' che si veda bene.
  int capX = lastPx + (HUM_ROD_SIZE / 2);
  int capY = lastPy + (HUM_ROD_SIZE / 2);

  tft.fillCircle(capX, capY, HUM_CAP_R, tft.color565(55, 55, 55));  // bordo grigio scuro
  tft.fillCircle(capX, capY, HUM_CAP_R - 5, TFT_BLACK);             // centro nero
  tft.drawCircle(capX, capY, HUM_CAP_R, tft.color565(95, 95, 95));  // rilievo esterno
  tft.drawCircle(capX, capY, HUM_CAP_R - 5, tft.color565(80, 80, 80)); // rilievo interno
}

// Aggiorna l'animazione delle particelle facendole uscire dal cappuccio nero,
// senza mai intaccare l'icona statica sottostante (zona di pulizia ristretta
// alla sola area sopra il cappuccio).
void aggiornaParticelleUmidificatore() {
  // Punto di nascita del vapore: appena sopra il cappuccio nero
  const int nascitaX = (int)(HUM_ROD_STARTX + HUM_ROD_DX * (HUM_ROD_STEPS - 1)) + (HUM_ROD_SIZE / 2);
  const int nascitaY = (int)(HUM_ROD_STARTY + HUM_ROD_DY * (HUM_ROD_STEPS - 1)) + (HUM_ROD_SIZE / 2) - HUM_CAP_R - 4;
  const int fadeY = nascitaY - 14; // altezza a cui le particelle svaniscono

  // Pulisce solo la fascia sopra il cappuccio (non tocca mai il resto dell'icona)
  tft.fillRect(nascitaX - 45, fadeY - 4, 90, (nascitaY - fadeY) + 12, TFT_BLACK);

  if (humidifierAttivo) {
    for (int i = 0; i < NUM_PARTICELLE; i++) {
      if (!particelle[i].attiva) {
        if (random(0, 100) < 25) {
          particelle[i].x = nascitaX + random(-6, 7);
          particelle[i].y = nascitaY;
          particelle[i].velocita = 0.6f + (random(0, 50) / 100.0f);
          particelle[i].attiva = true;
        }
      } else {
        particelle[i].y -= particelle[i].velocita;
        particelle[i].x += (random(-10, 11) / 12.0f); // Leggera oscillazione laterale

        if (particelle[i].y < fadeY) {
          particelle[i].attiva = false;
        } else {
          uint16_t coloreVapore = tft.color565(200, 230, 255);
          tft.fillCircle((int)particelle[i].x, (int)particelle[i].y, 2, coloreVapore);
        }
      }
    }
  }
}

void mostraMenuHumidifier() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawCentreString("HUMIDIFIER", 160, 20, 1);
  tft.drawFastHLine(20, 42, 280, TFT_CYAN);
  tft.setTextSize(1);

  // Icona statica del diffusore
  disegnaIconaUmidificatore();

  // Reset di tutte le particelle ad ogni ingresso in questa schermata
  for (int i = 0; i < NUM_PARTICELLE; i++) particelle[i].attiva = false;

  // Testo di stato
  tft.setTextSize(2);
  tft.setTextColor(humidifierAttivo ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawCentreString(humidifierAttivo ? "STATO: ACCESO" : "STATO: SPENTO", 160, 232, 1);
  tft.setTextSize(1);

  tft.setTextSize(3);

  // Bottone ON
  tft.drawRect(btnHumOn.x, btnHumOn.y, btnHumOn.w, btnHumOn.h, TFT_GREEN);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("ON", btnHumOn.x + (btnHumOn.w / 2), btnHumOn.y + (btnHumOn.h / 2) - 3, 1);

  // Bottone OFF
  tft.drawRect(btnHumOff.x, btnHumOff.y, btnHumOff.w, btnHumOff.h, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawCentreString("OFF", btnHumOff.x + (btnHumOff.w / 2), btnHumOff.y + (btnHumOff.h / 2) - 3, 1);

  // Bottone TIMER (mostra il preset corrente, tocco per cambiarlo)
  char timerLabel[24];
  snprintf(timerLabel, sizeof(timerLabel), "TIMER: %s", humidifierPresetLabel[humidifierTimerIndex]);
  tft.drawRect(btnHumTimer.x, btnHumTimer.y, btnHumTimer.w, btnHumTimer.h, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCentreString(timerLabel, btnHumTimer.x + (btnHumTimer.w / 2), btnHumTimer.y + (btnHumTimer.h / 2) - 3, 1);
  tft.setTextSize(3);

  // Bottone BACK
  tft.drawRect(btnHumBack.x, btnHumBack.y, btnHumBack.w, btnHumBack.h, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("INDIETRO", btnHumBack.x + (btnHumBack.w / 2), btnHumBack.y + (btnHumBack.h / 2) - 3, 1);

  tft.setTextSize(1);
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
  tft.setTextSize(3);              // Titolo grande in font pixel
  tft.drawCentreString(titolo, 160, 20, 1);
  tft.drawFastHLine(20, 42, 280, TFT_GREEN);

  tft.setTextSize(1);              // Testo piu' piccolo in font pixel per elenco e pulsanti

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
    tft.drawString(listaTracce[tracciaIndice], 30, currentY + (boxH / 2) - 6, 1);
  }

  tft.setTextDatum(MC_DATUM);

  // Riga 1 Controlli: VOL -, STOP, VOL +
  tft.drawRect(btnVolGiu.x, btnVolGiu.y, btnVolGiu.w, btnVolGiu.h, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString(btnVolGiu.etichetta, btnVolGiu.x + (btnVolGiu.w/2), btnVolGiu.y + (btnVolGiu.h/2) - 2, 1);

  tft.drawRect(btnStop.x, btnStop.y, btnStop.w, btnStop.h, TFT_ORANGE);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawCentreString(btnStop.etichetta, btnStop.x + (btnStop.w/2), btnStop.y + (btnStop.h/2) - 2, 1);

  tft.drawRect(btnVolSu.x, btnVolSu.y, btnVolSu.w, btnVolSu.h, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString(btnVolSu.etichetta, btnVolSu.x + (btnVolSu.w/2), btnVolSu.y + (btnVolSu.h/2) - 2, 1);

  // Riga 2 Navigazione: SU, GIU, BACK
  tft.drawRect(btnSu.x, btnSu.y, btnSu.w, btnSu.h, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(btnSu.etichetta, btnSu.x + (btnSu.w/2), btnSu.y + (btnSu.h/2) - 2, 1);

  tft.drawRect(btnGiu.x, btnGiu.y, btnGiu.w, btnGiu.h, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(btnGiu.etichetta, btnGiu.x + (btnGiu.w/2), btnGiu.y + (btnGiu.h/2) - 2, 1);

  tft.drawRect(btnIndietroMedia.x, btnIndietroMedia.y, btnIndietroMedia.w, btnIndietroMedia.h, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawCentreString(btnIndietroMedia.etichetta, btnIndietroMedia.x + (btnIndietroMedia.w/2), btnIndietroMedia.y + (btnIndietroMedia.h/2) - 2, 1);

  tft.setTextSize(1);      // Ripristino la scala di default
}

void mostraMenuHelp() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(3);              // Titolo grande in font pixel
  tft.drawCentreString("HELP & INFO", 160, 40, 1);
  tft.drawFastHLine(20, 75, 280, TFT_YELLOW);

  tft.setTextSize(2);              // Testo del corpo, piu' piccolo, sempre in font pixel
  tft.setTextDatum(TL_DATUM); 
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GUIDA DI UTILIZZO:", 20, 100, 1);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("- HUMIDIFIER:", 20, 140, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("  ON/OFF + timer di spegnimento.", 20, 165, 1);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("- MEDIA:", 20, 205, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("  Scegli le tracce audio dalla SD.", 20, 230, 1);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("- JAMMER:", 20, 270, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("  Opzione futura di controllo.", 20, 295, 1);

  // --- SCRITTA GIGANTE IN PIXEL ---
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(7);               // Ingrandimento x7 = lettere enormi e blocchettose
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("SUCA", 160, 350, 1);

  tft.setTextSize(3);               // Torno alla dimensione grande per il pulsante
  tft.setTextDatum(MC_DATUM);
  tft.drawRect(pulsanteIndietroHelp.x, pulsanteIndietroHelp.y, pulsanteIndietroHelp.w, pulsanteIndietroHelp.h, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawCentreString(pulsanteIndietroHelp.etichetta, pulsanteIndietroHelp.x + (pulsanteIndietroHelp.w/2), pulsanteIndietroHelp.y + (pulsanteIndietroHelp.h/2) - 3, 1);

  tft.setTextSize(1);                // Ripristino la scala di default per le altre schermate
}

// --- PAGINA BANANA: ONDA OSCILLOSCOPIO + CONTROLLO POTENZA ---

// Disegna solo l'onda dentro il riquadro (senza ridisegnare tutta la pagina)
void disegnaOnda() {
  int x = rettangoloOnda.x;
  int y = rettangoloOnda.y;
  int w = rettangoloOnda.w;
  int h = rettangoloOnda.h;

  // Pulisce l'interno del riquadro lasciando intatto il bordo
  tft.fillRect(x + 2, y + 2, w - 4, h - 4, TFT_BLACK);

  int centroY = y + (h / 2);

  if (bananaPotenza == 0) {
    // OFF: linea piatta al centro
    tft.drawFastHLine(x + 4, centroY, w - 8, TFT_DARKGREY);
    return;
  }

  // L'ampiezza cresce con la potenza: a MAX occupa quasi tutta la meta' del riquadro
  float ampiezzaMax = (h / 2.0) - 8;
  float ampiezza = ampiezzaMax * (bananaPotenza / 3.0);

  // Anche la frequenza aumenta leggermente per un effetto piu' "vivo" alle potenze alte
  float frequenza = 1.5 + (bananaPotenza * 0.5);

  uint16_t colore;
  if (bananaPotenza == 1) colore = TFT_YELLOW;
  else if (bananaPotenza == 2) colore = TFT_ORANGE;
  else colore = TFT_RED;

  int prevX = x + 4;
  int prevY = centroY;
  for (int px = x + 4; px <= x + w - 4; px++) {
    float t = (float)(px - x) / w;

    // Onda base + tremolio: piccolo rumore casuale che varia ad ogni aggiornamento
    float tremore = (random(-100, 101) / 100.0) * (ampiezzaMax * 0.06);
    float valore = sin(t * frequenza * 2 * PI + faseOnda) * ampiezza + tremore;

    int py = centroY + (int)valore;
    tft.drawLine(prevX, prevY, px, py, colore);
    prevX = px;
    prevY = py;
  }
}

// Disegna i 4 pulsanti di potenza, evidenziando in verde quello attivo
void disegnaPulsantiBanana() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  for (int i = 0; i < 4; i++) {
    uint16_t colore = (i == bananaPotenza) ? TFT_GREEN : TFT_WHITE;
    tft.drawRect(bananaBtn[i].x, bananaBtn[i].y, bananaBtn[i].w, bananaBtn[i].h, colore);
    tft.setTextColor(colore, TFT_BLACK);
    tft.drawCentreString(bananaBtn[i].etichetta, bananaBtn[i].x + (bananaBtn[i].w / 2), bananaBtn[i].y + (bananaBtn[i].h / 2) - 4, 1);
  }
  tft.setTextSize(1);
}

// Schermata completa della pagina Banana
void mostraMenuBanana() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawCentreString("JAMMER", 160, 20, 1);
  tft.drawFastHLine(20, 42, 280, TFT_GREEN);
  tft.setTextSize(1);

  // Riquadro contenitore dell'onda
  tft.drawRect(rettangoloOnda.x, rettangoloOnda.y, rettangoloOnda.w, rettangoloOnda.h, TFT_WHITE);
  disegnaOnda();

  // Pulsanti potenza
  disegnaPulsantiBanana();

  // Pulsante BACK
  tft.setTextSize(3);
  tft.drawRect(btnIndietroBanana.x, btnIndietroBanana.y, btnIndietroBanana.w, btnIndietroBanana.h, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawCentreString(btnIndietroBanana.etichetta, btnIndietroBanana.x + (btnIndietroBanana.w / 2), btnIndietroBanana.y + (btnIndietroBanana.h / 2) - 3, 1);
  tft.setTextSize(1);
}

// --- SETUP E LOOP ---

void setup() {
  Serial.begin(115200);
  delay(500);
  randomSeed(analogRead(0)); // seme casuale per l'animazione delle particelle

  tft.init();
  tft.setRotation(2); // Rotazione display originale
  
  drawAnimatedLogo();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);      // Font pixel GLCD ingrandito, coerente col resto della UI
  tft.drawCentreString("Loading JAM-BI...", 160, 430, 1);
  tft.setTextSize(1);      // Ripristino la scala di default
  delay(1500);

  
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); 

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
  // Spegnimento automatico dell'humidifier se il timer e' scaduto (attivo indipendentemente
  // dalla schermata in cui ci si trova, per sicurezza).
  if (humidifierAttivo && humidifierTimerSec > 0) {
    if (millis() - humidifierAccensioneMillis >= (unsigned long)humidifierTimerSec * 1000UL) {
      humidifierAttivo = false;
      inviaAlSlave(2, 0);
      Serial.println("Humidifier spento automaticamente (timer scaduto)");
      if (sistemaStato == 5) mostraMenuHumidifier();
    }
  }

  // Animazione tremolio onda Banana: si aggiorna da sola quando la potenza e' attiva,
  // indipendentemente dal touch, cosi' l'onda "vive" mentre e' accesa.
  if (sistemaStato == 4 && bananaPotenza != 0) {
    if (millis() - ultimoTremore > 90) {
      faseOnda += 0.35;
      disegnaOnda();
      ultimoTremore = millis();
    }
  }

  // Animazione particelle di vapore nella schermata Humidifier
  if (sistemaStato == 5) {
    if (millis() - ultimaParticella > 120) {
      aggiornaParticelleUmidificatore();
      ultimaParticella = millis();
    }
  }

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
              Serial.println("JAMMER PREMUTO");
              sistemaStato = 4;
              mostraMenuBanana();
            } 
            else if (i == 1) {
              // Apre la nuova schermata dedicata all'humidifier invece di attivarlo direttamente
              sistemaStato = 5;
              mostraMenuHumidifier();
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

      // ==========================================
      // LOGICA STATO 4: PAGINA BANANA (ONDA + POTENZA)
      // ==========================================
      else if (sistemaStato == 4) {
        bool azionato = false;

        // 1. Controlla i 4 pulsanti di potenza
        for (int i = 0; i < 4; i++) {
          if (x_mappato >= bananaBtn[i].x && x_mappato <= (bananaBtn[i].x + bananaBtn[i].w) &&
              y_mappato >= bananaBtn[i].y && y_mappato <= (bananaBtn[i].y + bananaBtn[i].h)) {
            bananaPotenza = i; // 0=OFF, 1=POT1, 2=POT2+, 3=MAX
            inviaAlSlave(5, bananaPotenza); // Action 5: Potenza Banana
            Serial.printf("JAMMER potenza impostata: %d\n", bananaPotenza);

            disegnaPulsantiBanana(); // Aggiorna evidenziazione pulsante attivo
            disegnaOnda();           // Aggiorna l'onda in base alla nuova potenza

            azionato = true;
            break;
          }
        }

        // 2. Controlla BACK
        if (!azionato) {
          if (x_mappato >= btnIndietroBanana.x && x_mappato <= (btnIndietroBanana.x + btnIndietroBanana.w) &&
              y_mappato >= btnIndietroBanana.y && y_mappato <= (btnIndietroBanana.y + btnIndietroBanana.h)) {
            sistemaStato = 1;
            mostraMenuPrincipale();
          }
        }
      }

      // ==========================================
      // LOGICA STATO 5: SCHERMATA HUMIDIFIER (ON/OFF + TIMER)
      // ==========================================
      else if (sistemaStato == 5) {
        // 1. Bottone ON
        if (x_mappato >= btnHumOn.x && x_mappato <= (btnHumOn.x + btnHumOn.w) &&
            y_mappato >= btnHumOn.y && y_mappato <= (btnHumOn.y + btnHumOn.h)) {
          humidifierAttivo = true;
          humidifierAccensioneMillis = millis();
          inviaAlSlave(2, 1);
          Serial.println("Humidifier ACCESO");
          mostraMenuHumidifier();
        }

        // 2. Bottone OFF
        else if (x_mappato >= btnHumOff.x && x_mappato <= (btnHumOff.x + btnHumOff.w) &&
                 y_mappato >= btnHumOff.y && y_mappato <= (btnHumOff.y + btnHumOff.h)) {
          humidifierAttivo = false;
          inviaAlSlave(2, 0);
          Serial.println("Humidifier SPENTO");
          mostraMenuHumidifier();
        }

        // 3. Bottone TIMER (cicla tra i preset)
        else if (x_mappato >= btnHumTimer.x && x_mappato <= (btnHumTimer.x + btnHumTimer.w) &&
                 y_mappato >= btnHumTimer.y && y_mappato <= (btnHumTimer.y + btnHumTimer.h)) {
          humidifierTimerIndex = (humidifierTimerIndex + 1) % 5;
          humidifierTimerSec = humidifierPresets[humidifierTimerIndex];
          // Se e' gia' acceso, il countdown riparte da zero con il nuovo timer scelto
          if (humidifierAttivo) humidifierAccensioneMillis = millis();
          Serial.printf("Timer humidifier impostato: %s\n", humidifierPresetLabel[humidifierTimerIndex]);
          mostraMenuHumidifier();
        }

        // 4. Bottone BACK
        else if (x_mappato >= btnHumBack.x && x_mappato <= (btnHumBack.x + btnHumBack.w) &&
                 y_mappato >= btnHumBack.y && y_mappato <= (btnHumBack.y + btnHumBack.h)) {
          sistemaStato = 1;
          mostraMenuPrincipale();
        }
      }

      delay(180); // Ritardo anti-rimbalzo ottimizzato
    }
  }
}