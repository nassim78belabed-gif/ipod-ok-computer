// iPod "OK Computer Edition" - PCB V8 mini (sans batterie/molette) - ESP32-S3-DevKitC-1 N16R8
// Brochage EXACT du PCB (ne pas changer sans refaire le PCB).
// GPIO interdits sur N16R8 (PSRAM octale) : 26-37. Strapping evites : 0, 3, 45, 46.

// Bus SPI partage ecran + SD
#define SPI_SCK    8
#define SPI_MOSI   17
#define SPI_MISO   18      // = MISO de la SD uniquement (SDO de l'ecran NON connecte)

// Chip select / controle
#define SD_CS      16
#define TFT_CS     11
#define TFT_DC     9
#define TFT_RST    10

// DAC PCM5102A (I2S)
#define I2S_BCK    7
#define I2S_DIN    6
#define I2S_LRCK   5

// Boutons (internes en INPUT_PULLUP, boutons vers GND)
#define BTN_MENU     42
#define BTN_PLAY     41
#define BTN_PREV     40
#define BTN_NEXT     39
#define BTN_VOLUP    38
#define BTN_VOLDOWN  1
