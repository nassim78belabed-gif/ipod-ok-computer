// iPod "OK Computer Edition" - PCB de test V7 - ESP32-S3-DevKitC-1 N16R8
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

// Entrees (internes en INPUT_PULLUP, boutons vers GND)
#define BTN_MENU   42
#define BTN_PLAY   41
#define ENC_A      40
#define ENC_B      39
#define ENC_SW     38

// Mesure batterie (pont 100k/100k sur VSW -> lire x2)
#define VBAT_ADC   1
