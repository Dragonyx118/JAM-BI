#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "logo_data.h"

// Pin del Touch (Bus HSPI)
#define TOUCH_CS_PIN  27
#define TOUCH_TDO_PIN 12
#define TOUCH_TDI_PIN 13
#define TOUCH_CLK_PIN 14

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

// Variabile per gestire lo stato del programma
// 0 = Boot/Animazione, 1 = Menu Principale
int sistemaStato = 0; 

// Struttura geometrica per definire l'area cliccabile di un Pulsante
struct Pulsante {
  int x;
  int y;
  int w;
  int h;
  const char* etichetta;
};

// Definiamo i 4 pulsanti centrati su schermo largo 320px
// X = 20, Larghezza = 280 (lascia 20px di margine a destra e sinistra)
// Sostituisci la vecchia definizione dei pulsanti con questa:
Pulsante menuPulsanti[4] = {
  {20, 170, 280, 50, "JAMMER"},        
  {20, 240, 280, 50, "UMIDIFICATORE"},
  {20, 310, 280, 50, "MEDIA"},
  {20, 380, 280, 50, "HELP"}
};

// 1. Animazione iniziale: Logo grande (Scala 3) dall'alto in basso
void drawAnimatedLogo() {
  tft.fillScreen(TFT_BLACK);
  int scala = 3; 
  int offsetX = (320 - (LOGO_COLS * scala)) / 2;
  int offsetY = (480 - (LOGO_ROWS * scala)) / 2 - 20; // Leggermente sollevato

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

// 2. Disegna il Logo rimpicciolito in alto (Scala 2) senza animazione
void drawMiniLogo(int offsetX, int offsetY) {
  int scala = 2; // Scala ridotta per farlo stare in alto
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

// Funzione artigianale per disegnare lettere in pixel-art (Infallibile)
void disegnaLetteraCustom(char c, int x, int y, uint16_t colore) {
  switch (toUpperCase(c)) {
    case 'J':
      tft.fillRect(x+8, y, 4, 12, colore);
      tft.fillRect(x, y+12, 12, 4, colore);
      tft.fillRect(x, y+8, 4, 4, colore);
      break;
    case 'A':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+10, y, 4, 16, colore);
      tft.fillRect(x+4, y, 6, 4, colore);
      tft.fillRect(x+4, y+8, 6, 4, colore);
      break;
    case 'M':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+12, y, 4, 16, colore);
      tft.fillRect(x+4, y+4, 4, 4, colore);
      tft.fillRect(x+8, y+4, 4, 4, colore);
      break;
    case 'U':
      tft.fillRect(x, y, 4, 14, colore);
      tft.fillRect(x+10, y, 4, 14, colore);
      tft.fillRect(x+4, y+12, 6, 4, colore);
      break;
    case 'I':
      tft.fillRect(x+4, y, 4, 16, colore);
      break;
    case 'D':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+4, y, 6, 4, colore);
      tft.fillRect(x+4, y+12, 6, 4, colore);
      tft.fillRect(x+10, y+4, 4, 8, colore);
      break;
    case 'F':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+4, y, 10, 4, colore);
      tft.fillRect(x+4, y+7, 8, 4, colore);
      break;
    case 'C':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+4, y, 10, 4, colore);
      tft.fillRect(x+4, y+12, 10, 4, colore);
      break;
    case 'T':
      tft.fillRect(x, y, 14, 4, colore);
      tft.fillRect(x+5, y+4, 4, 12, colore);
      break;
    case 'O':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+10, y, 4, 16, colore);
      tft.fillRect(x+4, y, 6, 4, colore);
      tft.fillRect(x+4, y+12, 6, 4, colore);
      break;
    case 'R':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+4, y, 8, 4, colore);
      tft.fillRect(x+4, y+7, 8, 4, colore);
      tft.fillRect(x+10, y+4, 4, 3, colore);
      tft.fillRect(x+10, y+11, 4, 5, colore);
      break;
    case 'E':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+4, y, 10, 4, colore);
      tft.fillRect(x+4, y+6, 8, 4, colore);
      tft.fillRect(x+4, y+12, 10, 4, colore);
      break;
    case 'H':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+10, y, 4, 16, colore);
      tft.fillRect(x+4, y+6, 6, 4, colore);
      break;
    case 'L':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+4, y+12, 10, 4, colore);
      break;
    case 'P':
      tft.fillRect(x, y, 4, 16, colore);
      tft.fillRect(x+4, y, 8, 4, colore);
      tft.fillRect(x+4, y+7, 8, 4, colore);
      tft.fillRect(x+10, y+4, 4, 3, colore);
      break;
  }
}

void stampaTestoCustom(const char* testo, int centroX, int centroY, uint16_t colore) {
  int len = strlen(testo);
  
  // Se la parola è UMIDIFICATORE o JAMMER, possiamo stringere la spaziatura a 14 o 16 pixel 
  // per farla stare comodamente nel rettangolo da 280px!
  int spaziaturaLettera = (len > 8) ? 14 : 18; 
  
  int larghezzaTotale = len * spaziaturaLettera - 4;
  int startX = centroX - (larghezzaTotale / 2);
  int startY = centroY - 8;
  
  for (int i = 0; i < len; i++) {
    disegnaLetteraCustom(testo[i], startX + (i * spaziaturaLettera), startY, colore);
  }
}

void mostraMenuPrincipale() {
  tft.fillScreen(TFT_BLACK);

  // Disegna il mini logo in alto
  int miniLogoX = (320 - (LOGO_COLS * 2)) / 2;
  drawMiniLogo(miniLogoX, 10);

  // Disegna i pulsanti e le relative scritte geometriche
  for (int i = 0; i < 4; i++) {
    // Rettangolo bianco esterno
    tft.drawRect(menuPulsanti[i].x, menuPulsanti[i].y, menuPulsanti[i].w, menuPulsanti[i].h, TFT_WHITE);
    
    // Calcola il centro del rettangolo
    int centroX = menuPulsanti[i].x + (menuPulsanti[i].w / 2);
    int centroY = menuPulsanti[i].y + (menuPulsanti[i].h / 2);
    
    // Disegna il testo pixel per pixel
    stampaTestoCustom(menuPulsanti[i].etichetta, centroX, centroY, TFT_WHITE);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  tft.init();
  tft.setRotation(2); // Schermo verticale orientato correttamente
  
  // Esegue l'animazione di avvio dello stato 0
  drawAnimatedLogo();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("Loading JAM-BI...", 160, 430, 2);
  
  delay(2500); // Mostra la schermata di caricamento per 2.5 secondi

  // Inizializza il Touch indipendente sui pin HSPI
  touchSPI.begin(TOUCH_CLK_PIN, TOUCH_TDO_PIN, TOUCH_TDI_PIN, TOUCH_CS_PIN);
  if (!ts.begin(touchSPI)) {
    Serial.println("Errore hardware Touch!");
  }

  // Passa ufficialmente allo stato Menu
  sistemaStato = 1;
  mostraMenuPrincipale();
}

void loop() {
  // Gestiamo il touch solo se siamo dentro la schermata del Menu (Stato 1)
  if (sistemaStato == 1 && ts.touched()) {
    TS_Point p = ts.getPoint();

    // Mappatura calibrata al millimetro per la modalità verticale (Rotation 2)
    int x_mappato = map(p.y, 200, 3800, 320, 0); 
    int y_mappato = map(p.x, 200, 3800, 0, 480); 

    // Controlla se il tocco rientra in uno dei 4 pulsanti
    for (int i = 0; i < 4; i++) {
      if (x_mappato >= menuPulsanti[i].x && x_mappato <= (menuPulsanti[i].x + menuPulsanti[i].w) &&
          y_mappato >= menuPulsanti[i].y && y_mappato <= (menuPulsanti[i].y + menuPulsanti[i].h)) {
        
        // --- FUNZIONE CLICK INTERCETTATA ---
        Serial.printf("Pulsante premuto: %s\n", menuPulsanti[i].etichetta);
        
        // Feedback grafico veloce (inverte il rettangolo in verde quando lo premi)
        tft.drawRect(menuPulsanti[i].x, menuPulsanti[i].y, menuPulsanti[i].w, menuPulsanti[i].h, TFT_GREEN);
        delay(150); // Piccolo debounce visivo
        tft.drawRect(menuPulsanti[i].x, menuPulsanti[i].y, menuPulsanti[i].w, menuPulsanti[i].h, TFT_WHITE);
        
        // Qui in futuro metterai i cambi di schermata, ad esempio:
        // if(i == 0) { apriSchermataJam(); }
        // if(i == 1) { apriSchermataUmidificatore(); }
        
        break; // Esci dal ciclo una volta trovato il pulsante premuto
      }
    }
    delay(100); // Evita letture multiple involontarie (Debounce)
  }
}