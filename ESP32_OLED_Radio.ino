//*******************************
//  - nvsacbug library and integrated in this sketch.
//
// See http://www.internet-radio.com for suitable stations.  Add the stations of your choice
// to the preferences in either Esp32_rad********************************************************************
//*  ESP32_Radio -- Webradio receiver for ESP32, 1.8 color display and VS1053 MP3 module.           *
//*                 By Ed Smallenburg.                                                              *
//***************************************************************************************************
// ESP32 libraries used:
//  - WiFiMultiio_init.ino sketch or through the webinterface.
//
// Brief description of the program:
// First a suitable WiFi network is found and a connection is made.
// Then a connection will be made to a shoutcast server.  The server starts with some
// info in the header in readable ascii, ending with a double CRLF, like:
//  icy-name:Classic Rock Florida - SHE Radio
//  icy-genre:Classic Rock 60s 70s 80s Oldies Miami South Florida
//  icy-url:http://www.ClassicRockFLorida.com
//  content-type:audio/mpeg
//  - Adafruit_ST7735
//  - ArduinoOTA
//  - PubSubClientenc_dt_pin
//  - SD
//  - FS
// A library for the VS1053 (for ESP32) is not available (or not easy to find).  Therefore
// a class for this module is derived from the mani
//  icy-pub:1
//  icy-metaint:32768          - Metadata after 32768 bytes of MP3-data
//  icy-br:128                 - in kb/sec (for Ogg this is like "icy-br=Quality 2"
//
// After de double CRLF is received, the server starts sending mp3- or Ogg-data.  For mp3, this
// data may contain metadata (non mp3) after every "metaint" mp3 bytes.
// The metadata is empty in most cases, but if any is available the content will be
// presented on the TFT.
// Pushing an input button causes the player to execute a programmable command.
//
// The display used is a Chinese 1.8 color TFT module 128 x 160 pixels.
// Now there is room for 26 characters per line and 16 lines.
// Software will work without installing the display.
// The SD card interface of the module may be used to play mp3-tracks on the SD card.
//
// For configuration of the WiFi network(s): see the global data section further on.
//
// The VSPI interface is used for VS1053, TFT and SD.
//
// Wiring. Note that this is just an example.  Pins (except 18,19 and 23 of the SPI interface)
// can be configured in the config page of the web interface.
// ESP32dev Signal  Wired to LCD        Wired to VS1053      SDCARD   Wired to the rest
// -------- ------  --------------      -------------------  ------   ---------------
// GPIO16           -                   pin 1 XDCS            -       -
// GPIO5            -                   pin 2 XCS             -       -
// GPIO4            -                   pin 4 DREQ            -       -
// GPIO2            pin 3 D/C or A0     -                     -       -
// GPIO17           -                   -                     CS      -
// GPIO18   SCK     pin 5 CLK or SCK    pin 5 SCK             CLK     -
// GPIO19   MISO    -                   pin 7 MISO            MISO    -
// GPIO23   MOSI    pin 4 DIN or SDA    pin 6 MOSI            MOSI    -
// GPIO15           pin 2 CS            -                     -       -
// GPI03    RXD0    -                   -                     -       Reserved serial input
// GPIO1    TXD0    -                   -                     -       Reserved serial output
// GPIO34   -       -                   -                     -       Optional pull-up resistor
// GPIO35   -       -                   -                     -       Infrared receiver VS1838B
// GPIO25   -       -                   -                     -       Rotary encoder CLK
// GPIO26   -       -                   -                     -       Rotary encoder DT
// GPIO27   -       -                   -                     -       Rotary encoder SW
// -------  ------  ---------------     -------------------  ------   ----------------
// GND      -       pin 8 GND           pin 8 GND                     Power supply GND
// VCC 5 V  -       pin 7 BL            -                             Power supply
// VCC 5 V  -       pin 6 VCC           pin 9 5V                      Power supply
// EN       -       pin 1 RST           pin 3 XRST                    -
//
// 26-04-2017, ES: First set-up, derived from ESP8266 version.
// 08-05-2017, ES: Handling of preferences.
// 20-05-2017, ES: Handling input buttons and MQTT.
// 22-05-2017, ES: Save preset, volume and tone settings.
// 23-05-2017, ES: No more calls of non-iram functions on interrupts.
// 24-05-2017, ES: Support for featherboard.
// 26-05-2017, ES: Correction playing from .m3u playlist. Allow single hidden SSID.
// 30-05-2017, ES: Add SD card support (FAT format), volume indicator.
// 26-06-2017, ES: Correction: start in AP-mode if no WiFi networks configured.
// 28-06-2017, ES: Added IR interface.
// 30-06-2017, ES: Improved functions for SD card play.
// 03-07-2017, ES: Webinterface control page shows current settings.
// 04-07-2017, ES: Correction MQTT subscription. Keep playing during long operations.
// 08-07-2017, ES: More space for streamtitle on TFT.
// 18-07-2017, ES: Time Of Day on TFT.
// 19-07-2017, ES: Minor corrections.
// 26-07-2017, ES: Flexible pin assignment. Add rotary encoder switch.
// 27-07-2017, ES: Removed tinyXML library.
// 18-08-2017, Es: Minor corrections
// 28-08-2017, ES: Preferences for pins used for SPI bus,
//                 Corrected bug in handling programmable pins,
//                 Introduced touch pins.
// 30-08-2017, ES: Limit number of retries for MQTT connection.
//                 Added MDNS responder.
// 11-11-2017, ES: Increased ringbuffer.  Measure bit rate.
// 13-11-2017, ES: Forward declarations.
// 16-11-2017, ES: Replaced ringbuffer by FreeRTOS queue, play function on second CPU,
//                 Included improved rotary switch routines supplied by fenyvesi,
//                 Better IR sensitivity.
// 30-11-2017, ES: Hide passwords in config page.
// 01-12-2017, ES: Better handling of playlist.
// 07-12-2017, ES: Faster handling of config screen.
// 08-12-2017, ES: More MQTT items to publish, added pin_shutdown.
// 13-12-2017, ES: Correction clear LCD.
// 15-12-2017, ES: Correction defaultprefs.h.
// 18-12-2017, ES: Stop playing during config.
// 02-01-2018, ES: Stop/resume is same command.
// 22-01-2018, ES: Read ADC (GPIO36) and display as a battery capacity percentage.
// 13-02-2018, ES: Stop timer during NVS write.
// 15-02-2018, ES: Correction writing wifi credentials in NVS.
// 03-03-2018, ES: Correction bug IR pinnumber.
// 05-03-2018, ES: Improved rotary encoder interface.
// 10-03-2018, ES: Minor corrections.
// 13-04-2018, ES: Guard against empty string send to TFT, thanks to Andreas Spiess.
// 16-04-2018, ES: ID3 tags handling while playing from SD.
// 25-04-2018, ES: Choice of several display boards.
// 30-04-2018, ES: Bugfix: crash when no IR is configured, no display without VS1063.
// 08-05-2018, ES: 1602 LCD display support (limited).
// 11-05-2018, ES: Bugfix: incidental crash in isr_enc_turn().
// 30-05-2018, ES: Bugfix: Assigned DRAM to global variables used in timer ISR.
// 31-05-2018, ES: Bugfix: Crashed if I2C is used, but pins not defined.
// 01-06-2018, ES: Run Playtask on CPU 0.
// 04-06-2018, ES: Made handling of playlistdata more tolerant (NDR).
// 09-06-2018, ES: Typo in defaultprefs.h
// 10-06-2018, ES: Rotary encoder, interrupts on all 3 signals.
//
//
// Define the version number, also used for webserver as Last-Modified header:
#define VERSION "Sun, 10 June 2018 18:30:00 GMT"
//
// Define type of display.  See documentation.
//#define BLUETFT                        // Works also for RED TFT 128x160
#define OLED                         // 64x128 I2C OLED
//#define DUMMYTFT                     // Dummy display
 //#define LCD1602I2C                   // LCD 1602 display with I2C backpack
//
#include <nvs.h>
#include <PubSubClient.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <stdio.h>
#include <string.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <esp_partition.h>
#include <driver/adc.h>



//=== bring onother part ino file here ====//
#include "inputcontrol.h"
#include "localmp3.h"
#include "display.h"
#include "module.h"


                                  // For OLED I2C SD1306 64x128 display
// #include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Wire.h"
// SSD1306Wire  display(0x3c, 21, 22);

// Number of entries in the queue
#define QSIZ 400
// Debug buffer size
#define DEBUG_BUFFER_SIZE 150
// Access point name if connection to WiFi network fails.  Also the hostname for WiFi and OTA.
// Not that the password of an AP must be at least as long as 8 characters.
// Also used for other naming.
#define NAME "ESP32Radio"
// Maximum number of MQTT reconnects before give-up
#define MAXMQTTCONNECTS 5
// Adjust size of buffer to the longest expected string for nvsgetstr
#define NVSBUFSIZE 150
// Position (column) of time in topline relative to end
#define TIMEPOS -52
// SPI speed for SD card
#define SDSPEED 1000000
// Size of metaline buffer
#define METASIZ 1024
// Max. number of NVS keys in table
#define MAXKEYS 200
//
// Subscription topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
// by the the mqttprefix in the preferences.  The next definition will yield the topic
// "ESP32Radio/command" if mqttprefix is "ESP32Radio".
#define MQTT_SUBTOPIC     "command"           // Command to receive from MQTT
//
//**************************************************************************************************
// Forward declaration and prototypes of various functions.                                        *
//**************************************************************************************************
void        displaytime ( const char* str, uint16_t color = 0xFFFF ) ;
void        showstreamtitle ( const char* ml, bool full = false ) ;
void        handlebyte_ch ( uint8_t b ) ;
void        handleFSf ( const String& pagename ) ;
void        handleCmd()  ;
char*       dbgprint( const char* format, ... ) ;
const char* analyzeCmd ( const char* str ) ;
const char* analyzeCmd ( const char* par, const char* val ) ;
void        chomp ( String &str ) ;
String      httpheader ( String contentstype ) ;
bool        nvssearch ( const char* key ) ;
void        mp3loop() ;
//void        tftlog ( const char *str ) ;
void        playtask ( void * parameter ) ;       // Task to play the stream
void        spftask ( void * parameter ) ;        // Task for special functions
void        gettime() ;

//**************************************************************************************************
// Several structs.                                                                                *
//**************************************************************************************************
//

struct scrseg_struct                                  // For screen segments
{
  bool     update_req ;                               // Request update of screen
  uint16_t color ;                                    // Textcolor
  uint16_t y ;                                        // Begin of segment row
  uint16_t height ;                                   // Height of segment
  String   str ;                                      // String to be displayed
} ;

enum qdata_type { QDATA, QSTARTSONG, QSTOPSONG } ;    // datatyp in qdata_struct
struct qdata_struct
{
  int datatyp ;                                       // Identifier
  __attribute__((aligned(4))) uint8_t buf[32] ;       // Buffer for chunk
} ;

struct ini_struct
{
  String         mqttbroker ;                         // The name of the MQTT broker server
  String         mqttprefix ;                         // Prefix to use for topics
  uint16_t       mqttport ;                           // Port, default 1883
  String         mqttuser ;                           // User for MQTT authentication
  String         mqttpasswd ;                         // Password for MQTT authentication
  uint8_t        reqvol ;                             // Requested volume
  uint8_t        rtone[4] ;                           // Requested bass/treble settings
  int8_t         newpreset ;                          // Requested preset
  String         clk_server ;                         // Server to be used for time of day clock
  int8_t         clk_offset ;                         // Offset in hours with respect to UTC
  int8_t         clk_dst ;                            // Number of hours shift during DST
  int8_t         ir_pin ;                             // GPIO connected to output of IR decoder
  int8_t         enc_clk_pin ;                        // GPIO connected to CLK of rotary encoder
  int8_t         enc_dt_pin ;                         // GPIO connected to DT of rotary encoder
  int8_t         enc_sw_pin ;                         // GPIO connected to SW of rotary encoder
  int8_t         tft_cs_pin ;                         // GPIO connected to CS of TFT screen
  int8_t         tft_dc_pin ;                         // GPIO connected to D/C or A0 of TFT screen

  int8_t         tft_scl_pin ;                        // GPIO connected to SCL of i2c TFT screen
  int8_t         tft_sda_pin ;                        // GPIO connected to SDA of I2C TFT screen
  
  int8_t         sd_cs_pin ;                          // GPIO connected to CS of SD card
  int8_t         vs_cs_pin ;                          // GPIO connected to CS of VS1053
  int8_t         vs_dcs_pin ;                         // GPIO connected to DCS of VS1053
  int8_t         vs_dreq_pin ;                        // GPIO connected to DREQ of VS1053
  int8_t         vs_shutdown_pin ;                    // GPIO to shut down the amplifier
  int8_t         spi_sck_pin ;                        // GPIO connected to SPI SCK pin
  int8_t         spi_miso_pin ;                       // GPIO connected to SPI MISO pin
  int8_t         spi_mosi_pin ;                       // GPIO connected to SPI MOSI pin
  uint16_t       bat0 ;                               // ADC value for 0 percent battery charge
  uint16_t       bat100 ;                             // ADC value for 100 percent battery charge
} ;

struct WifiInfo_t                                     // For list with WiFi info
{
  uint8_t inx ;                                       // Index as in "wifi_00"
  char * ssid ;                                       // SSID for an entry
  char * passphrase ;                                 // Passphrase for an entry
} ;

struct nvs_entry
{
  uint8_t  Ns ;                                       // Namespace ID
  uint8_t  Type ;                                     // Type of value
  uint8_t  Span ;                                     // Number of entries used for this item
  uint8_t  Rvs ;                                      // Reserved, should be 0xFF
  uint32_t CRC ;                                      // CRC
  char     Key[16] ;                                  // Key in Ascii
  uint64_t Data ;                                     // Data in entry
} ;

struct nvs_page                                       // For nvs entries
{ // 1 page is 4096 bytes
  uint32_t  State ;
  uint32_t  Seqnr ;
  uint32_t  Unused[5] ;
  uint32_t  CRC ;
  uint8_t   Bitmap[32] ;
  nvs_entry Entry[126] ;
} ;

struct keyname_t                                      // For keys in NVS
{
  char      Key[16] ;                                 // Mac length is 15 plus delimeter
} ;

//**************************************************************************************************
// Global data section.                                                                            *
//**************************************************************************************************
// There is a block ini-data that contains some configuration.  Configuration data is              *
// saved in the preferences by the webinterface.  On restart the new data will                     *
// de read from these preferences.                                                                 *
// Items in ini_block can be changed by commands from webserver/MQTT/Serial.                       *
//**************************************************************************************************

enum datamode_t { INIT = 1, HEADER = 2, DATA = 4,        // State for datastream
                  METADATA = 8, PLAYLISTINIT = 16,
                  PLAYLISTHEADER = 32, PLAYLISTDATA = 64,
                  STOPREQD = 128, STOPPED = 256
                } ;

// Global variables
int               DEBUG = 1 ;                            // Debug on/off
int               numSsid ;                              // Number of available WiFi networks
WiFiMulti         wifiMulti ;                            // Possible WiFi networks
ini_struct        ini_block ;                            // Holds configurable data
WiFiServer        cmdserver ( 80 ) ;                     // Instance of embedded webserver, port 80
WiFiClient        mp3client ;                            // An instance of the mp3 client
WiFiClient        cmdclient ;                            // An instance of the client for commands
WiFiClient        wmqttclient ;                          // An instance for mqtt
PubSubClient      mqttclient ( wmqttclient ) ;           // Client for MQTT subscriber
TaskHandle_t      maintask ;                             // Taskhandle for main task
TaskHandle_t      xplaytask ;                            // Task handle for playtask
TaskHandle_t      xspftask ;                             // Task handle for special functions
SemaphoreHandle_t SPIsem = NULL ;                        // For exclusive SPI usage
hw_timer_t*       timer = NULL ;                         // For timer
char              timetxt[9] ;                           // Converted timeinfo
char              cmd[130] ;                             // Command from MQTT or Serial
QueueHandle_t     dataqueue ;                            // Queue for mp3 datastream
QueueHandle_t     spfqueue ;                             // Queue for special functions
qdata_struct      outchunk ;                             // Data to queue
qdata_struct      inchunk ;                              // Data from queue
uint8_t*          outqp = outchunk.buf ;                 // Pointer to buffer in outchunk
uint32_t          totalcount = 0 ;                       // Counter mp3 data
datamode_t        datamode ;                             // State of datastream
int               metacount ;                            // Number of bytes in metadata
int               datacount ;                            // Counter databytes before metadata
char              metalinebf[METASIZ + 1] ;              // Buffer for metaline/ID3 tags
int16_t           metalinebfx ;                          // Index for metalinebf
String            icystreamtitle ;                       // Streamtitle from metadata
String            icyname ;                              // Icecast station name
String            ipaddress ;                            // Own IP-address
int               bitrate ;                              // Bitrate in kb/sec
int               mbitrate ;                             // Measured bitrate
int               metaint = 0 ;                          // Number of databytes between metadata
int8_t            currentpreset = -1 ;                   // Preset station playing
String            host ;                                 // The URL to connect to or file to play
String            playlist ;                             // The URL of the specified playlist
bool              hostreq = false ;                      // Request for new host
bool              reqtone = false ;                      // New tone setting requested
bool              muteflag = false ;                     // Mute output
bool              resetreq = false ;                     // Request to reset the ESP32
bool              NetworkFound = false ;                 // True if WiFi network connected
bool              mqtt_on = false ;                      // MQTT in use
String            networks ;                             // Found networks in the surrounding
uint16_t          mqttcount = 0 ;                        // Counter MAXMQTTCONNECTS
int8_t            playingstat = 0 ;                      // 1 if radio is playing (for MQTT)
int8_t            playlist_num = 0 ;                     // Nonzero for selection from playlist
File              mp3file ;                              // File containing mp3 on SD card
uint32_t          mp3filelength ;                        // File length
bool              localfile = false ;                    // Play from local mp3-file or not
bool              chunked = false ;                      // Station provides chunked transfer
int               chunkcount = 0 ;                       // Counter for chunked transfer
String            http_getcmd ;                          // Contents of last GET command
String            http_rqfile ;                          // Requested file
bool              http_reponse_flag = false ;            // Response required
uint16_t          ir_value = 0 ;                         // IR code
struct tm         timeinfo ;                             // Will be filled by NTP server
bool              time_req = false ;                     // Set time requested
bool              SD_okay = false ;                      // True if SD card in place and readable
String            SD_nodelist ;                          // Nodes of mp3-files on SD
int               SD_nodecount = 0 ;                     // Number of nodes in SD_nodelist
String            SD_currentnode = "" ;                  // Node ID of song playing ("0" if random)
uint16_t          adcval ;                               // ADC value (battery voltage)
uint16_t          clength ;                              // Content length found in http header
int16_t           scanios ;                              // TEST*TEST*TEST
int16_t           scaniocount ;                          // TEST*TEST*TEST
std::vector<WifiInfo_t> wifilist ;                       // List with wifi_xx info
// nvs stuff
nvs_page                nvsbuf ;                         // Space for 1 page of NVS info
const esp_partition_t*  nvs ;                            // Pointer to partition struct
esp_err_t               nvserr ;                         // Error code from nvs functions
uint32_t                nvshandle = 0 ;                  // Handle for nvs access
uint8_t                 namespace_ID ;                   // Namespace ID found
char                    nvskeys[MAXKEYS][16] ;           // Space for NVS keys
std::vector<keyname_t> keynames ;                        // Keynames in NVS
// Rotary encoder stuff
#define sv DRAM_ATTR static volatile
sv uint16_t       clickcount = 0 ;                       // Incremented per encoder click
sv int16_t        rotationcount = 0 ;                    // Current position of rotary switch
sv uint16_t       enc_inactivity = 0 ;                   // Time inactive
sv bool           singleclick = false ;                  // True if single click detected
sv bool           doubleclick = false ;                  // True if double click detected
sv bool           tripleclick = false ;                  // True if triple click detected
sv bool           longclick = false ;                    // True if longclick detected
enum enc_menu_t { VOLUME, PRESET, TRACK } ;              // State for rotary encoder menu
enc_menu_t        enc_menu_mode = VOLUME ;               // Default is VOLUME mode


String tftsec2log;
String stasiun;
int curvsvol;
long sleepvalsec=0;
long sleepvalmin=0;
bool sleepalarm=false;
bool tolocalap=false;
bool isplaying=false;
int tmrinfo=0;
int numOfPreset=0;

int tc2, tc3, tc4, tc5, tc6,prevtc6, prevtc2,prevtc3,prevtc4,prevtc5;
int tmode=0;
int tmppreset;
int angka2=0;
int tlrate=1;
int pinleddummyload=32;
// bool startlisteningtouch=false;

#define touch1 14 //14
#define touch2 12  //13
#define touch3 27 //13
#define touch4 15 //14
#define led_connnected_in 2
bool flasing_connection=false;
// Include software for the right display
#ifdef BLUETFT
#include "bluetft.h"                                     // For ILI9163C or ST7735S 128x160 display
#endif
#ifdef OLED
#include "SSD1306.h"   
#endif
#ifdef LCD1602I2C
#include "LCD1602.h"                                     // For LCD 1602 display (I2C)

#endif
#ifdef DUMMYTFT
#include "Dummytft.h"                                    // For Dummy display
#endif

//
struct progpin_struct                                    // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
  // Example: "uppreset=1"
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
} ;

progpin_struct   progpin[] =                             // Input pins and programmed function
{
  {  0, false, false,  "", false },
  //{  1, true,  false,  "", false },                    // Reserved for TX Serial output
  {  2, false, false,  "", false },
  //{  3, true,  false,  "", false },                    // Reserved for RX Serial input
  {  4, false, false,  "", false },
  {  5, false, false,  "", false },
  //{  6, true,  false,  "", false },                    // Reserved for FLASH SCK
  //{  7, true,  false,  "", false },                    // Reserved for FLASH D0
  //{  8, true,  false,  "", false },                    // Reserved for FLASH D1
  //{  9, true,  false,  "", false },                    // Reserved for FLASH D2
  //{ 10, true,  false,  "", false },                    // Reserved for FLASH D3
  //{ 11, true,  false,  "", false },                    // Reserved for FLASH CMD
  { 12, false, false,  "", false },
  { 13, false, false,  "", false },
  { 14, false, false,  "", false },
  { 15, false, false,  "", false },
  { 16, false, false,  "", false },
  { 17, false, false,  "", false },
  { 18, false, false,  "", false },                      // Default for SPI CLK
  { 19, false, false,  "", false },                      // Default for SPI MISO
  //{ 20, true,  false,  "", false },                    // Not exposed on DEV board
  { 21, false, false,  "", false },                      // Also Wire SDA
  { 22, false, false,  "", false },                      // Also Wire SCL
  { 23, false, false,  "", false },                      // Default for SPI MOSI
  //{ 24, true,  false,  "", false },                    // Not exposed on DEV board
  { 25, false, false,  "", false },
  { 26, false, false,  "", false },
  { 27, false, false,  "", false },
  //{ 28, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 29, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 30, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 31, true,  false,  "", false },                    // Not exposed on DEV board
  { 32, false, false,  "", false },
  { 33, false, false,  "", false },
  { 34, false, false,  "", false },                      // Note, no internal pull-up
  { 35, false, false,  "", false },                      // Note, no internal pull-up
  { -1, false, false,  "", false }                       // End of list
} ;

struct touchpin_struct                                   // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number GPIO
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
  // Example: "uppreset=1"
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
  int16_t        count ;                                 // Counter number of times low level
} ;
touchpin_struct   touchpin[] =                           // Touch pins and programmed function
{
  {   4, false, false, "", false, 0 },                   // TOUCH0
  //{ 0, true,  false, "", false, 0 },                   // TOUCH1, reserved for BOOT button
  {   2, false, false, "", false, 0 },                   // TOUCH2
  {  15, false, false, "", false, 0 },                   // TOUCH3
  {  13, false, false, "", false, 0 },                   // TOUCH4
  {  12, false, false, "", false, 0 },                   // TOUCH5
  {  14, false, false, "", false, 0 },                   // TOUCH6
  {  27, false, false, "", false, 0 },                   // TOUCH7
  {  33, false, false, "", false, 0 },                   // TOUCH8
  {  32, false, false, "", false, 0 },                   // TOUCH9
  {  -1, false, false, "", false, 0 }
  // End of table
} ;


//**************************************************************************************************
// Pages, CSS and data for the webinterface.                                                       *
//**************************************************************************************************
#include "about_html.h"
#include "config_html.h"
#include "index_html.h"
#include "mp3play_html.h"
#include "radio_css.h"
#include "favicon_ico.h"
#include "defaultprefs.h"

//**************************************************************************************************
// End of global data section.                                                                     *
//**************************************************************************************************




//**************************************************************************************************
//                                     M Q T T P U B _ C L A S S                                   *
//**************************************************************************************************
// ID's for the items to publish to MQTT.  Is index in amqttpub[]
enum { MQTT_IP,     MQTT_ICYNAME, MQTT_STREAMTITLE, MQTT_NOWPLAYING,
       MQTT_PRESET, MQTT_VOLUME, MQTT_PLAYING
     } ;
enum { MQSTRING, MQINT8 } ;                              // Type of variable to publish

class mqttpubc                                           // For MQTT publishing
{
    struct mqttpub_struct
    {
      const char*    topic ;                             // Topic as partial string (without prefix)
      uint8_t        type ;                              // Type of payload
      void*          payload ;                           // Payload for this topic
      bool           topictrigger ;                      // Set to true to trigger MQTT publish
    } ;
    // Publication topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
    // by the the mqttprefix in the preferences.
  protected:
    mqttpub_struct amqttpub[8] =                   // Definitions of various MQTT topic to publish
    { // Index is equal to enum above
      { "ip",              MQSTRING, &ipaddress,        false }, // Definition for MQTT_IP
      { "icy/name",        MQSTRING, &icyname,          false }, // Definition for MQTT_ICYNAME
      { "icy/streamtitle", MQSTRING, &icystreamtitle,   false }, // Definition for MQTT_STREAMTITLE
      { "nowplaying",      MQSTRING, &ipaddress,        false }, // Definition for MQTT_NOWPLAYING
      { "preset" ,         MQINT8,   &currentpreset,    false }, // Definition for MQTT_PRESET
      { "volume" ,         MQINT8,   &ini_block.reqvol, false }, // Definition for MQTT_VOLUME
      { "playing",         MQINT8,   &playingstat,      false }, // Definition for MQTT_PLAYING
      { NULL,              0,        NULL,              false }  // End of definitions
    } ;
  public:
    void          trigger ( uint8_t item ) ;                      // Trigger publishig for one item
    void          publishtopic() ;                                // Publish triggerer items
} ;


//**************************************************************************************************
// MQTTPUB  class implementation.                                                                  *
//**************************************************************************************************

//**************************************************************************************************
//                                            T R I G G E R                                        *
//**************************************************************************************************
// Set request for an item to publish to MQTT.                                                     *
//**************************************************************************************************
void mqttpubc::trigger ( uint8_t item )                    // Trigger publishig for one item
{
  amqttpub[item].topictrigger = true ;                     // Request re-publish for an item
}

//**************************************************************************************************
//                                     P U B L I S H T O P I C                                     *
//**************************************************************************************************
// Publish a topic to MQTT broker.                                                                 *
//**************************************************************************************************
void mqttpubc::publishtopic()
{
  int         i = 0 ;                                         // Loop control
  char        topic[40] ;                                     // Topic to send
  const char* payload ;                                       // Points to payload
  char        intvar[10] ;                                    // Space for integer parameter
  while ( amqttpub[i].topic )
  {
    if ( amqttpub[i].topictrigger )                           // Topic ready to send?
    {
      amqttpub[i].topictrigger = false ;                      // Success or not: clear trigger
      sprintf ( topic, "%s/%s", ini_block.mqttprefix.c_str(),
                amqttpub[i].topic ) ;                         // Add prefix to topic
      switch ( amqttpub[i].type )                             // Select conversion method
      {
        case MQSTRING :
          payload = ((String*)amqttpub[i].payload)->c_str() ;
          //payload = pstr->c_str() ;                           // Get pointer to payload
          break ;
        case MQINT8 :
          sprintf ( intvar, "%d",
                    *(int8_t*)amqttpub[i].payload ) ;         // Convert to array of char
          payload = intvar ;                                  // Point to this array
          break ;
        default :
          continue ;                                          // Unknown data type
      }
      dbgprint ( "Publish to topic %s : %s",                  // Show for debug
                 topic, payload ) ;
      if ( !mqttclient.publish ( topic, payload ) )           // Publish!
      {
        dbgprint ( "MQTT publish failed!" ) ;                 // Failed
      }
      return ;                                                // Do the rest later
    }
    i++ ;                                                     // Next entry
  }
}

mqttpubc         mqttpub ;                                    // Instance for mqttpubc


//
//**************************************************************************************************
// VS1053 stuff.  Based on maniacbug library.                                                      *
//**************************************************************************************************
// VS1053 class definition.                                                                        *
//**************************************************************************************************
class VS1053
{
  private:
    int8_t       cs_pin ;                         // Pin where CS line is connected
    int8_t       dcs_pin ;                        // Pin where DCS line is connected
    int8_t       dreq_pin ;                       // Pin where DREQ line is connected
    int8_t       shutdown_pin ;                   // Pin where the shutdown line is connected
    uint8_t       curvol ;                        // Current volume setting 0..100%
    const uint8_t vs1053_chunk_size = 32 ;
    // SCI Register
    const uint8_t SCI_MODE          = 0x0 ;
    const uint8_t SCI_BASS          = 0x2 ;
    const uint8_t SCI_CLOCKF        = 0x3 ;
    const uint8_t SCI_AUDATA        = 0x5 ;
    const uint8_t SCI_WRAM          = 0x6 ;
    const uint8_t SCI_WRAMADDR      = 0x7 ;
    const uint8_t SCI_AIADDR        = 0xA ;
    const uint8_t SCI_VOL           = 0xB ;
    const uint8_t SCI_AICTRL0       = 0xC ;
    const uint8_t SCI_AICTRL1       = 0xD ;
    const uint8_t SCI_num_registers = 0xF ;
    // SCI_MODE bits
    const uint8_t SM_SDINEW         = 11 ;        // Bitnumber in SCI_MODE always on
    const uint8_t SM_RESET          = 2 ;         // Bitnumber in SCI_MODE soft reset
    const uint8_t SM_CANCEL         = 3 ;         // Bitnumber in SCI_MODE cancel song
    const uint8_t SM_TESTS          = 5 ;         // Bitnumber in SCI_MODE for tests
    const uint8_t SM_LINE1          = 14 ;        // Bitnumber in SCI_MODE for Line input
    SPISettings   VS1053_SPI ;                    // SPI settings for this slave
    uint8_t       endFillByte ;                   // Byte to send when stopping song
    bool          okay              = true ;      // VS1053 is working
  protected:
    inline void await_data_request() const
    {
       while ( ( dreq_pin >= 0 ) &&
               ( !digitalRead ( dreq_pin ) ) )
      {
        NOP() ;                                   // Very short delay
      }
    }

    inline void control_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      digitalWrite ( cs_pin, LOW ) ;
    }

    inline void control_mode_off() const
    {
      digitalWrite ( cs_pin, HIGH ) ;             // End control mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    inline void data_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      //digitalWrite ( cs_pin, HIGH ) ;             // Bring slave in data mode
      digitalWrite ( dcs_pin, LOW ) ;
    }

    inline void data_mode_off() const
    {
      digitalWrite ( dcs_pin, HIGH ) ;            // End data mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    uint16_t    read_register ( uint8_t _reg ) const ;
    void        write_register ( uint8_t _reg, uint16_t _value ) const ;
    inline bool sdi_send_buffer ( uint8_t* data, size_t len ) ;
    void        sdi_send_fillers ( size_t length ) ;
    void        wram_write ( uint16_t address, uint16_t data ) ;
    uint16_t    wram_read ( uint16_t address ) ;

  public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    VS1053 ( int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin, int8_t _shutdown_pin ) ;
    void     begin() ;                                   // Begin operation.  Sets pins correctly,
                                                         // and prepares SPI bus.
    void     startSong() ;                               // Prepare to start playing. Call this each
                                                         // time a new song starts.
    inline bool playChunk ( uint8_t* data,               // Play a chunk of data.  Copies the data to
                            size_t len ) ;               // the chip.  Blocks until complete.
                                                         // Returns true if more data can be added
                                                         // to fifo
    void     stopSong() ;                                // Finish playing a song. Call this after
                                                         // the last playChunk call.
    void     setVolume ( uint8_t vol ) ;                 // Set the player volume.Level from 0-100,
                                                         // higher is louder.
    void     setTone ( uint8_t* rtone ) ;                // Set the player baas/treble, 4 nibbles for
                                                         // treble gain/freq and bass gain/freq
    inline uint8_t  getVolume() const                    // Get the current volume setting.
    {                                                    // higher is louder.
      return curvol ;
    }
    void     printDetails ( const char *header ) ;       // Print config details to serial output
    void     softReset() ;                               // Do a soft reset
    bool     testComm ( const char *header ) ;           // Test communication with module
    inline bool data_request() const
    {
      return ( digitalRead ( dreq_pin ) == HIGH ) ;
    }
} ;

//**************************************************************************************************
// VS1053 class implementation.                                                                    *
//**************************************************************************************************

VS1053::VS1053 ( int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin, int8_t _shutdown_pin ) :
  cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin), shutdown_pin(_shutdown_pin)
{
}

uint16_t VS1053::read_register ( uint8_t _reg ) const
{
  uint16_t result ;

  control_mode_on() ;
  SPI.write ( 3 ) ;                                // Read operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  // Note: transfer16 does not seem to work
  result = ( SPI.transfer ( 0xFF ) << 8 ) |        // Read 16 bits data
           ( SPI.transfer ( 0xFF ) ) ;
  await_data_request() ;                           // Wait for DREQ to be HIGH again
  control_mode_off() ;
  return result ;
}

void VS1053::write_register ( uint8_t _reg, uint16_t _value ) const
{
  control_mode_on( );
  SPI.write ( 2 ) ;                                // Write operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  SPI.write16 ( _value ) ;                         // Send 16 bits data
  await_data_request() ;
  control_mode_off() ;
}

bool VS1053::sdi_send_buffer ( uint8_t* data, size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    await_data_request() ;                         // Wait for space available
    SPI.writeBytes ( data, chunk_length ) ;
    data += chunk_length ;
  }
  data_mode_off() ;
  return data_request() ;                          // True if more data can de stored in fifo
}

void VS1053::sdi_send_fillers ( size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    await_data_request() ;                         // Wait for space available
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    while ( chunk_length-- )
    {
      SPI.write ( endFillByte ) ;
    }
  }
  data_mode_off();
}

void VS1053::wram_write ( uint16_t address, uint16_t data )
{
  write_register ( SCI_WRAMADDR, address ) ;
  write_register ( SCI_WRAM, data ) ;
}

uint16_t VS1053::wram_read ( uint16_t address )
{
  write_register ( SCI_WRAMADDR, address ) ;            // Start reading from WRAM
  return read_register ( SCI_WRAM ) ;                   // Read back result
}

bool VS1053::testComm ( const char *header )
{
  // Test the communication with the VS1053 module.  The result wille be returned.
  // If DREQ is low, there is problably no VS1053 connected.  Pull the line HIGH
  // in order to prevent an endless loop waiting for this signal.  The rest of the
  // software will still work, but readbacks from VS1053 will fail.
  int       i ;                                         // Loop control
  uint16_t  r1, r2, cnt = 0 ;
  uint16_t  delta = 300 ;                               // 3 for fast SPI

  dbgprint ( header ) ;                                 // Show a header
  if ( !digitalRead ( dreq_pin ) )
  {
    dbgprint ( "VS1053 not properly installed!" ) ;
    // Allow testing without the VS1053 module
    pinMode ( dreq_pin,  INPUT_PULLUP ) ;               // DREQ is now input with pull-up
    return false ;                                      // Return bad result
  }
  // Further TESTING.  Check if SCI bus can write and read without errors.
  // We will use the volume setting for this.
  // Will give warnings on serial output if DEBUG is active.
  // A maximum of 20 errors will be reported.
  if ( strstr ( header, "Fast" ) )
  {
    delta = 3 ;                                         // Fast SPI, more loops
  }
  for ( i = 0 ; ( i < 0xFFFF ) && ( cnt < 20 ) ; i += delta )
  {
    write_register ( SCI_VOL, i ) ;                     // Write data to SCI_VOL
    r1 = read_register ( SCI_VOL ) ;                    // Read back for the first time
    r2 = read_register ( SCI_VOL ) ;                    // Read back a second time
    if  ( r1 != r2 || i != r1 || i != r2 )              // Check for 2 equal reads
    {
      dbgprint ( "VS1053 error retry SB:%04X R1:%04X R2:%04X", i, r1, r2 ) ;
      cnt++ ;
      delay ( 10 ) ;
    }
  }
  okay = ( cnt == 0 ) ;                                 // True if working correctly
  return ( okay ) ;                                     // Return the result
}

void VS1053::begin()
{
  pinMode      ( dreq_pin,  INPUT ) ;                   // DREQ is an input
  pinMode      ( cs_pin,    OUTPUT ) ;                  // The SCI and SDI signals
  pinMode      ( dcs_pin,   OUTPUT ) ;
  digitalWrite ( dcs_pin,   HIGH ) ;                    // Start HIGH for SCI en SDI
  digitalWrite ( cs_pin,    HIGH ) ;
  if ( shutdown_pin >= 0 )                              // Shutdown in use?
  {
    pinMode ( shutdown_pin,   OUTPUT ) ;
    digitalWrite ( shutdown_pin, HIGH ) ;              // Shut down audio output
  }
  delay ( 100 ) ;
  // Init SPI in slow mode ( 0.2 MHz )
  VS1053_SPI = SPISettings ( 200000, MSBFIRST, SPI_MODE0 ) ;
  SPI.setDataMode ( SPI_MODE0 ) ;
  SPI.setBitOrder ( MSBFIRST ) ;
  //printDetails ( "Right after reset/startup" ) ;
  delay ( 20 ) ;
  //printDetails ( "20 msec after reset" ) ;
  if ( testComm ( "Slow SPI, Testing VS1053 read/write registers..." ) )
  {
    // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
    // when playing MP3.  You can modify the board, but there is a more elegant way:
    wram_write ( 0xC017, 3 ) ;                            // GPIO DDR = 3
    wram_write ( 0xC019, 0 ) ;                            // GPIO ODATA = 0
    delay ( 100 ) ;
    //printDetails ( "After test loop" ) ;
    softReset() ;                                         // Do a soft reset
    // Switch on the analog parts
    write_register ( SCI_AUDATA, 44100 + 1 ) ;            // 44.1kHz + stereo
    // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
    write_register ( SCI_CLOCKF, 6 << 12 ) ;              // Normal clock settings
                                                          // multiplyer 3.0 = 12.2 MHz
    //SPI Clock to 4 MHz. Now you can set high speed SPI clock.
    VS1053_SPI = SPISettings ( 5000000, MSBFIRST, SPI_MODE0 ) ;
    write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_LINE1 ) ) ;
    //testComm ( "Fast SPI, Testing VS1053 read/write registers again..." ) ;
    delay ( 10 ) ;
    await_data_request() ;
    endFillByte = wram_read ( 0x1E06 ) & 0xFF ;
    //dbgprint ( "endFillByte is %X", endFillByte ) ;
    //printDetails ( "After last clocksetting" ) ;
    delay ( 100 ) ;
  }
}

void VS1053::setVolume ( uint8_t vol )
{
  // Set volume.  Both left and right.
  // Input value is 0..100.  100 is the loudest.
  // Clicking reduced by using 0xf8 to 0x00 as limits.
  uint16_t value ;                                      // Value to send to SCI_VOL

  if ( vol != curvol )
  {
    curvol = vol ;                                      // Save for later use
    value = map ( vol, 0, 100, 0xF8, 0x00 ) ;           // 0..100% to one channel
    value = ( value << 8 ) | value ;
    write_register ( SCI_VOL, value ) ;                 // Volume left and right
  }
}

void VS1053::setTone ( uint8_t *rtone )                 // Set bass/treble (4 nibbles)
{
  // Set tone characteristics.  See documentation for the 4 nibbles.
  uint16_t value = 0 ;                                  // Value to send to SCI_BASS
  int      i ;                                          // Loop control

  for ( i = 0 ; i < 4 ; i++ )
  {
    value = ( value << 4 ) | rtone[i] ;                 // Shift next nibble in
  }
  write_register ( SCI_BASS, value ) ;                  // Volume left and right
}

void VS1053::startSong()
{
  sdi_send_fillers ( 10 ) ;
  if ( shutdown_pin >= 0 )                              // Shutdown in use?
  {
    digitalWrite ( shutdown_pin, LOW ) ;                // Enable audio output
  }

}

bool VS1053::playChunk ( uint8_t* data, size_t len )
{
  return okay && sdi_send_buffer ( data, len ) ;        // True if more data can be added to fifo
}

void VS1053::stopSong()
{
  uint16_t modereg ;                                    // Read from mode register
  int      i ;                                          // Loop control

  sdi_send_fillers ( 2052 ) ;
  if ( shutdown_pin >= 0 )                              // Shutdown in use?
  {
    digitalWrite ( shutdown_pin, HIGH ) ;               // Disable audio output
  }
  delay ( 10 ) ;
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_CANCEL ) ) ;
  for ( i = 0 ; i < 200 ; i++ )
  {
    sdi_send_fillers ( 32 ) ;
    modereg = read_register ( SCI_MODE ) ;  // Read status
    if ( ( modereg & _BV ( SM_CANCEL ) ) == 0 )
    {
      sdi_send_fillers ( 2052 ) ;
      //dbgprint ( "Song stopped correctly after %d msec", i * 10 ) ;
      return ;
    }
    delay ( 10 ) ;
  }
  printDetails ( "Song stopped incorrectly!" ) ;
}

void VS1053::softReset()
{
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_RESET ) ) ;
  delay ( 10 ) ;
  await_data_request() ;
}

void VS1053::printDetails ( const char *header )
{
  uint16_t     regbuf[16] ;
  uint8_t      i ;

  dbgprint ( header ) ;
  dbgprint ( "REG   Contents" ) ;
  dbgprint ( "---   -----" ) ;
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    regbuf[i] = read_register ( i ) ;
  }
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    delay ( 5 ) ;
    dbgprint ( "%3X - %5X", i, regbuf[i] ) ;
  }
}

// The object for the MP3 player
VS1053* vs1053player ;

//**************************************************************************************************
// End VS1053 stuff.                                                                               *
//**************************************************************************************************




//**************************************************************************************************
//                                      C L A I M S P I                                            *
//**************************************************************************************************
// Claim the SPI bus.  Uses FreeRTOS semaphores.                                                   *
//**************************************************************************************************
void claimSPI ( const char* p )
{
  const        TickType_t ctry = 10 ;                       // Time to wait for semaphore
  uint32_t     count = 0 ;                                  // Wait time in ticks

  while ( xSemaphoreTake ( SPIsem, ctry ) != pdTRUE  ) ;    // Claim SPI bus
  {
    if ( count++ > 10 )
    {
      dbgprint ( "SPI semaphore not taken within %d ticks by CPU %d, id %s",
                 count * ctry,
                 xPortGetCoreID(),
                 p ) ;
    }
  }
}


//**************************************************************************************************
//                                   R E L E A S E S P I                                           *
//**************************************************************************************************
// Free the the SPI bus.  Uses FreeRTOS semaphores.                                                *
//**************************************************************************************************
void releaseSPI()
{
  xSemaphoreGive ( SPIsem ) ;                            // Release SPI bus
}


//**************************************************************************************************
//                                      Q U E U E F U N C                                          *
//**************************************************************************************************
// Queue a special function for the play task.                                                     *
//**************************************************************************************************
void queuefunc ( int func )
{
  qdata_struct     specchunk ;                          // Special function to queue

  specchunk.datatyp = func ;                            // Put function in datatyp
  xQueueSend ( dataqueue, &specchunk, 200 ) ;           // Send to queue
}


//**************************************************************************************************
//                                      N V S S E A R C H                                          *
//**************************************************************************************************
// Check if key exists in nvs.                                                                     *
//**************************************************************************************************
bool nvssearch ( const char* key )
{
  size_t        len = NVSBUFSIZE ;                      // Length of the string

  nvsopen() ;                                           // Be sure to open nvs
  nvserr = nvs_get_str ( nvshandle, key, NULL, &len ) ; // Get length of contents
  return ( nvserr == ESP_OK ) ;                         // Return true if found
}


//**************************************************************************************************
//                                      L I S T S D T R A C K S                                    *
//**************************************************************************************************
// Search all MP3 files on directory of SD card.  Return the number of files found.                *
// A "node" of max. 4 levels ( the subdirectory level) will be generated for every file.           *
// The numbers within the node-array is the sequence number of the file/directory in that          *
// subdirectory.                                                                                   *
// A node ID is a string like "2,1,4,0", which means the 4th file in the first directory           *
// of the second directory.                                                                        *
// The list will be send to the webinterface if parameter "send"is true.                           *
//**************************************************************************************************
int listsdtracks ( const char * dirname, int level = 0, bool send = true )
{
  const  uint16_t SD_MAXDEPTH = 4 ;                     // Maximum depts.  Note: see mp3play_html.
  static uint16_t fcount, oldfcount ;                   // Total number of files
  static uint16_t SD_node[SD_MAXDEPTH + 1] ;            // Node ISs, max levels deep
  static String   SD_outbuf ;                           // Output buffer for cmdclient
  uint16_t        ldirname ;                            // Length of dirname to remove from filename
  File            root, file ;                          // Handle to root and directory entry
  String          filename ;                            // Copy of filename for lowercase test
  uint16_t        i ;                                   // Loop control to compute single node id
  String          tmpstr ;                              // Tijdelijke opslag node ID

  if ( strcmp ( dirname, "/" ) == 0 )                   // Are we at the root directory?
  {
    fcount = 0 ;                                        // Yes, reset count
    memset ( SD_node, 0, sizeof(SD_node) ) ;            // And sequence counters
    SD_outbuf = String() ;                              // And output buffer
    SD_nodelist = String() ;                            // And nodelist
    if ( !SD_okay )                                     // See if known card
    {
      if ( send )
      {
        cmdclient.println ( "0/No tracks found" ) ;     // No SD card, emppty list
      }
      return 0 ;
    }
  }
  oldfcount = fcount ;                                  // To see if files found in this directory
  //dbgprint ( "SD directory is %s", dirname ) ;        // Show current directory
  ldirname = strlen ( dirname ) ;                       // Length of dirname to remove from filename
  claimSPI ( "sdopen2" ) ;                              // Claim SPI bus
  root = SD.open ( dirname ) ;                          // Open the current directory level
  releaseSPI() ;                                        // Release SPI bus
  if ( !root || !root.isDirectory() )                   // Success?
  {
    dbgprint ( "%s is not a directory or not root",     // No, print debug message
               dirname ) ;
    return fcount ;                                     // and return
  }
  while ( true )                                        // Find all mp3 files
  {
    claimSPI ( "opennextf" ) ;                          // Claim SPI bus
    file = root.openNextFile() ;                        // Try to open next
    releaseSPI() ;                                      // Release SPI bus
    if ( !file )
    {
      break ;                                           // End of list
    }
    SD_node[level]++ ;                                  // Set entry sequence of current level
    if ( file.name()[0] == '.' )                        // Skip hidden directories
    {
      continue ;
    }
    if ( file.isDirectory() )                           // Is it a directory?
    {
      if ( level < SD_MAXDEPTH )                        // Yes, dig deeper
      {
        listsdtracks ( file.name(), level + 1, send ) ; // Note: called recursively
        SD_node[level + 1] = 0 ;                        // Forget counter for one level up
      }
    }
    else
    {
      filename = String ( file.name() ) ;               // Copy filename
      filename.toLowerCase() ;                          // Force lowercase
      if ( filename.endsWith ( ".mp3" ) )               // It is a file, but is it an MP3?
      {
        fcount++ ;                                      // Yes, count total number of MP3 files
        tmpstr = String() ;                             // Empty
        for ( i = 0 ; i < SD_MAXDEPTH ; i++ )           // Add a line containing the node to SD_outbuf
        {
          if ( i )                                      // Need to add separating comma?
          {
            tmpstr += String ( "," ) ;                  // Yes, add comma
          }
          tmpstr += String ( SD_node[i] ) ;             // Add sequence number
        }
        if ( send )                                     // Need to add to string for webinterface?
        {
          SD_outbuf += tmpstr +                         // Form line for mp3play_html page
                       utf8ascii ( file.name() +        // Filename starts after directoryname
                                   ldirname ) +
                       String ( "\n" ) ;
        }
        SD_nodelist += tmpstr + String ( "\n" ) ;       // Add to nodelist
        //dbgprint ( "Track: %s",                       // Show debug info
        //           file.name() + ldirname ) ;
        if ( SD_outbuf.length() > 1000 )                // Buffer full?
        {
          cmdclient.print ( SD_outbuf ) ;               // Yes, send it
          SD_outbuf = String() ;                        // Clear buffer
        }
      }
    }
    if ( send )
    {
      mp3loop() ;                                       // Keep playing
    }
  }
  if ( fcount != oldfcount )                            // Files in this directory?
  {
    SD_outbuf += String ( "-1/ \n" ) ;                  // Spacing in list
  }
  if ( SD_outbuf.length() )                             // Flush buffer if not empty
  {
    cmdclient.print ( SD_outbuf ) ;                     // Filled, send it
    SD_outbuf = String() ;                              // Continue with empty buffer
  }
  return fcount ;                                       // Return number of MP3s (sofar)
}


//**************************************************************************************************
//                                     G E T E N C R Y P T I O N T Y P E                           *
//**************************************************************************************************
// Read the encryption type of the network and return as a 4 byte name                             *
//**************************************************************************************************
const char* getEncryptionType ( wifi_auth_mode_t thisType )
{
  switch ( thisType )
  {
    case WIFI_AUTH_OPEN:
      return "OPEN" ;
    case WIFI_AUTH_WEP:
      return "WEP" ;
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK" ;
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK" ;
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK" ;
    case WIFI_AUTH_MAX:
      return "MAX" ;
    default:
      break ;
  }
  return "????" ;
}


//**************************************************************************************************
//                                        L I S T N E T W O R K S                                  *
//**************************************************************************************************
// List the available networks.                                                                    *
// Acceptable networks are those who have an entry in the preferences.                             *
// SSIDs of available networks will be saved for use in webinterface.                              *
//**************************************************************************************************
void listNetworks()
{
  WifiInfo_t       winfo ;            // Entry from wifilist
  wifi_auth_mode_t encryption ;       // TKIP(WPA), WEP, etc.
  const char*      acceptable ;       // Netwerk is acceptable for connection
  int              i, j ;             // Loop control

  dbgprint ( "Scan Networks" ) ;                         // Scan for nearby networks
  tftsec2log="Scan Networks";
  tftlog(tftsec2log.c_str());
  //oledshow(2,tftsec2log);
  // dsp_println("Scan Networks");
  // dsp_update();
  numSsid = WiFi.scanNetworks() ;
  tftsec2log= tftsec2log+"\nScan completed";
  tftlog(tftsec2log.c_str());
  //oledshow(2,tftsec2log);
  dbgprint ( "Scan completed" ) ;
  if ( numSsid <= 0 )
  {
    dbgprint ( "Couldn't get a wifi connection" ) ;
    oledshow(2,"no remembered Accces Point available");
    return ;

  }
  // print the list of networks seen:
  dbgprint ( "Number of available networks: %d",
             numSsid ) ;
  // String nssid="available AP = "+String(numSsid);
  String nssid=String(numSsid);
  tftsec2log=tftsec2log+ "\nFound "+nssid+ " available AP";
  tftlog(tftsec2log.c_str());
  //oledshow(2, tftsec2log);
  //dsp_update();
  // Print the network number and name for each network found and
  for ( i = 0 ; i < numSsid ; i++ )
  {
    acceptable = "" ;                                    // Assume not acceptable
    for ( j = 0 ; j < wifilist.size() ; j++ )            // Search in wifilist
    {
      winfo = wifilist[j] ;                              // Get one entry
      if ( WiFi.SSID(i).indexOf ( winfo.ssid ) == 0 )    // Is this SSID acceptable?
      {
        acceptable = "Acceptable" ;
        break ;
      }
    }
    // encryption = WiFi.encryptionType ( i ) ;
    dbgprint ( "%2d - %-25s Signal: %3d dBm",
               i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i)) ;
    // Remember this network for later use
    networks += WiFi.SSID(i) + String ( ", " ) ;        
  }
  dbgprint ( "End of list" ) ;
  tftsec2log+="\n"+networks;
  tftlog(tftsec2log.c_str());     
}

int countlocalap=8;
//**************************************************************************************************
//                                          T I M E R 1 0 S E C                                    *
//**************************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                                       *
// If totalcount has not been changed, there is a problem and playing will stop.                   *
// Note that calling timely procedures within this routine or in called functions will             *
// cause a crash!                                                                                  *
//**************************************************************************************************
void IRAM_ATTR timer10sec()
{
  static uint32_t oldtotalcount = 7321 ;          // Needed for change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails
  uint32_t        bytesplayed ;                   // Bytes send to MP3 converter

  if ( datamode & ( INIT | HEADER | DATA |        // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    bytesplayed = totalcount - oldtotalcount ;    // Number of bytes played in the 10 seconds
    oldtotalcount = totalcount ;                  // Save for comparison in next cycle
    if ( bytesplayed == 0 )                       // Still playing?
    {
      if ( morethanonce > 10 )                    // No! Happened too many times?
      {
        try
        {          
          WiFi.disconnect() ;                     // attempt to disconnect before restart         }
        }catch(int e){
          
        }
        ESP.restart() ;                           // Reset the CPU, probably no return
      }
      if ( datamode & ( PLAYLISTDATA |            // In playlist mode?
                        PLAYLISTINIT |
                        PLAYLISTHEADER ) )
      {
        playlist_num = 0 ;                        // Yes, end of playlist
      }
      if ( ( morethanonce > 0 ) ||                // Happened more than once?
           ( playlist_num > 0 ) )                 // Or playlist active?
      {
        oledshow(2, "Unable to play!\nTry to play next station....");
        isplaying=false;
        datamode = STOPREQD ;                     // Stop player
        ini_block.newpreset++ ;                   // Yes, try next channel
      }
      morethanonce++ ;                            // Count the fails
    }
    else
    {
      //                                          // Data has been send to MP3 decoder
      // Bitrate in kbits/s is bytesplayed / 10 / 1000 * 8
      mbitrate = ( bytesplayed + 625 ) / 1250 ;   // Measured bitrate
      morethanonce = 0 ;                          // Data seen, reset failcounter
    }
  }
  if(tolocalap==true){
    countlocalap--;
    String counts;
    counts = "Timeout : ";
    counts+=String(countlocalap);
    oledshow(3, counts);
    dbgprint ("countlocalap is counting" ) ;  
    if(countlocalap<1){
      {
        ESP.restart();
      }
    }
  }


}

// int tc2, tc3, tc4, tc5, tc6,prevtc6, prevtc2,prevtc3,prevtc4,prevtc5;
// int tmode=0;
// int tmppreset;
// int angka2=0;
// int tlrate=1;
// bool startlisteningtouch=false;





void touchs_listener(){
  if(digitalRead(touch1)==1){
      if(tolocalap){                                      //extra function to avoid timeout apmode
        tolocalap=false;
        oledshow(3,"Timeout dismissed!");
      }else{
        ini_block.reqvol -=2;
        if(ini_block.reqvol<50){ini_block.reqvol=50;}
        oledshow ( 3, "Volume is now : "+ String(ini_block.reqvol))  ;
        tlrate=3;
        tmrinfo=0;
      }
  }else{tlrate=1;}
  if(digitalRead(touch2)==1){
      if(tolocalap){
        ESP.restart();
      }else{
        ini_block.reqvol +=2;
        if(ini_block.reqvol>100){ini_block.reqvol=100;}
        oledshow ( 3, "Volume is now : "+ String(ini_block.reqvol))  ;
        tlrate=3;
        tmrinfo=0;
      }
    
  }else{tlrate=1;}
  if(digitalRead(touch3)==1){
    ini_block.newpreset--;
        if (ini_block.newpreset<=(-1)){ini_block.newpreset=numOfPreset;}
        oledshow ( 3, "Preset is now : "+  String(ini_block.newpreset))  ;
        tlrate=3;
        tmrinfo=0;
  }else{tlrate=1;}
  if(digitalRead(touch4)==1){
        ini_block.newpreset++;
        if(ini_block.newpreset>numOfPreset){ini_block.newpreset=0;}
        oledshow ( 3, "Preset is now : "+  String(ini_block.newpreset))  ;
        tlrate=3;
      tmrinfo=0; 
  }else{tlrate=1;}
}



//**************************************************************************************************
//                                          T I M E R 1 0 0                                        *
//**************************************************************************************************
// Called every 100 msec on interrupt level, so must be in IRAM and no lengthy operations          *
// allowed.                                                                                        *
//**************************************************************************************************
void IRAM_ATTR timer100()
{
  sv int16_t   count10sec = 0 ;                   // Counter for activatie 10 seconds process
  sv int16_t   eqcount = 0 ;                      // Counter for equal number of clicks
  sv int16_t   oldclickcount = 0 ;                // To detect difference

angka2++;
if (angka2>tlrate){
  touchs_listener();
  angka2=0;
}

   if(++count10sec==10){
      
   }
  if ( ++count10sec == 100  )                     // 10 seconds passed?
  {
    timer10sec() ;                                // Yes, do 10 second procedure
    count10sec = 0 ;                              // Reset count
  //  if(startlisteningtouch==0){startlisteningtouch=1;} // start to listening touch after 10 secs boot
    
  }
  if ( ( count10sec % 10 ) == 0 )                 // One second over?
  {
    //dynamicinfo();
    scaniocount = scanios ;                       // TEST*TEST*TEST
    scanios = 0 ;
    if ( ++timeinfo.tm_sec >= 60 )                // Yes, update number of seconds
    {
      timeinfo.tm_sec = 0 ;                       // Wrap after 60 seconds
      if ( ++timeinfo.tm_min >= 60 )
      {
        timeinfo.tm_min = 0 ;                     // Wrap after 60 minutes
        if ( ++timeinfo.tm_hour >= 24 )
        {
          timeinfo.tm_hour = 0 ;                  // Wrap after 24 hours
        }
      }
    }
    time_req = true ;                             // Yes, show current time request


   
  }

  // // Handle rotary encoder. Inactivity counter will be reset by encoder interrupt
  // if ( ++enc_inactivity == 36000 )                // Count inactivity time
  // {
  //   enc_inactivity = 1000 ;                       // Prevent wrap
  // }
  // // Now detection of single/double click of rotary encoder switch
  // if ( clickcount )                               // Any click?
  // {
  //   if ( oldclickcount == clickcount )            // Yes, stable situation?
  //   {
  //     if ( ++eqcount == 4 )                       // Long time stable?
  //     {
  //       eqcount = 0 ;
  //       if ( clickcount > 2 )                     // Triple click?
  //       {
  //         tripleclick = true ;                    // Yes, set result
  //       }
  //       else if ( clickcount == 2 )               // Double click?
  //       {
  //         doubleclick = true ;                    // Yes, set result
  //       }
  //       else
  //       {
  //         singleclick = true ;                    // Just one click seen
  //       }
  //       clickcount = 0 ;                          // Reset number of clicks
  //     }
  //   }
  //   else
  //   {
  //     oldclickcount = clickcount ;                // To detect change
  //     eqcount = 0 ;                               // Not stable, reset count
  //   }
  // }
}


//**************************************************************************************************
//                                          I S R _ I R                                            *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// Intervals are 640 or 1640 microseconds for data.  syncpulses are 3400 micros or longer.         *
// Input is complete after 65 level changes.                                                       *
// Only the last 32 level changes are significant and will be handed over to common data.          *
//**************************************************************************************************
void IRAM_ATTR isr_IR()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  sv uint32_t      ir_locvalue = 0 ;                 // IR code
  sv int           ir_loccount = 0 ;                 // Length of code
  uint32_t         t1, intval ;                      // Current time and interval since last change
  uint32_t         mask_in = 2 ;                     // Mask input for conversion
  uint16_t         mask_out = 1 ;                    // Mask output for conversion

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare
  if ( ( intval > 300 ) && ( intval < 800 ) )        // Short pulse?
  {
    ir_locvalue = ir_locvalue << 1 ;                 // Shift in a "zero" bit
    ir_loccount++ ;                                  // Count number of received bits
  }
  else if ( ( intval > 1500 ) && ( intval < 1800 ) ) // Long pulse?
  {
    ir_locvalue = ( ir_locvalue << 1 ) + 1 ;         // Shift in a "one" bit
    ir_loccount++ ;                                  // Count number of received bits
  }
  else if ( ir_loccount == 65 )                      // Value is correct after 65 level changes
  {
    while ( mask_in )                                // Convert 32 bits to 16 bits
    {
      if ( ir_locvalue & mask_in )                   // Bit set in pattern?
      {
        ir_value |= mask_out ;                       // Set set bit in result
      }
      mask_in <<= 2 ;                                // Shift input mask 2 positions
      mask_out <<= 1 ;                               // Shift output mask 1 position
    }
    ir_loccount = 0 ;                                // Ready for next input
  }
  else
  {
    ir_locvalue = 0 ;                                // Reset decoding
    ir_loccount = 0 ;
  }
}


//**************************************************************************************************
//                                          I S R _ E N C _ S W I T C H                            *
//**************************************************************************************************
// Interrupts received from rotary encoder switch.                                                 *
//**************************************************************************************************
void IRAM_ATTR isr_enc_switch()
{
  sv uint32_t     oldtime = 0 ;                            // Time in millis previous interrupt
  sv bool         sw_state ;                               // True is pushed (LOW)
  bool            newstate ;                               // Current state of input signal
  uint32_t        newtime ;                                // Current timestamp
  char*           reply;
  // Read current state of SW pin
  newstate = ( digitalRead ( ini_block.enc_sw_pin ) == LOW ) ;
  newtime = millis() ;
  if ( newtime == oldtime )                                // Debounce
  {
    return ;
  }
  if ( newstate != sw_state )                              // State changed?
  {
    sw_state = newstate ;                                  // Yes, set current (new) state
    if ( !sw_state )                                       // SW released?
    {
      if ( ( newtime - oldtime ) > 1000 )                  // More than 1 second?
      {
        longclick = true ;                                 // Yes, register longclick
      }
      else
      {
        clickcount++ ;                                     // Yes, click detected
      }
      enc_inactivity = 0 ;                                 // Not inactive anymore
    }
  }
  oldtime = newtime ;                                      // For next compare
}

String oldradioinfo;
String radioinfo;

//**************************************************************************************************
//                                S H O W S T R E A M T I T L E                                    *
//**************************************************************************************************
// Show artist and songtitle if present in metadata.                                               *
// Show always if full=true.                                                                       *
//**************************************************************************************************
void showstreamtitle ( const char *ml, bool full )
{
  char*             p1 ;
  char*             p2 ;
  char              streamtitle[150] ;           // Streamtitle from metadata
  const char*             reply;
  if ( strstr ( ml, "StreamTitle=" ) )
  {
    dbgprint ( "Streamtitle found, %d bytes", strlen ( ml ) ) ;
    dbgprint ( ml ) ;
    p1 = (char*)ml + 12 ;                       // Begin of artist and title
    if ( ( p2 = strstr ( ml, ";" ) ) )          // Search for end of title
    {
      if ( *p1 == '\'' )                        // Surrounded by quotes?
      {
        p1++ ;
        p2-- ;
      }
      *p2 = '\0' ;                              // Strip the rest of the line
    }
    // Save last part of string as streamtitle.  Protect against buffer overflow
    strncpy ( streamtitle, p1, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else if ( full )
  {
    // Info probably from playlist
    strncpy ( streamtitle, ml, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else
  {
    icystreamtitle = "" ;                       // Unknown type
    return ;                                    // Do not show
  }
  // Save for status request from browser and for MQTT
  icystreamtitle = streamtitle ;
  if ( ( p1 = strstr ( streamtitle, " - " ) ) ) // look for artist/title separator
  {
    *p1++ = '\n' ;                              // Found: replace 3 characters by newline
    p2 = p1 + 2 ;
    if ( *p2 == ' ' )                           // Leading space in title?
    {
      p2++ ;
    }
    strcpy ( p1, p2 ) ;                         // Shift 2nd part of title 2 or 3 places
  }
  //String tt=" - "+String (streamtitle);
  String tt=String (streamtitle);
  //stasiun=stasiun+tt;

  //stasiun=stasiun+"\n"+tt;
    radioinfo=stasiun+"\n"+tt;

  // dsp_fillRect ( 0, 16,                             // clear sector 0,12,128,24 for new text
  //                    dsp_getwidth(), 56-16, BLACK ) ;    
  // dsp_update();
  // tftset(1,radioinfo);
    oledshow(2,radioinfo);
    http_reponse_flag=true;
          httpheader ( String ( "text/html" ) ) ;           // Send header
          // the content of the HTTP response follows the header:
          // cmdclient.println ( "Dummy response\n" ) ;        // Text ending with double newline
    cmdclient.print("test send");

    //====not work====
  // if(radioinfo!=oldradioinfo){
  //   if(cmdclient.connected()){

  //         httpheader ( String ( "text/html" ) ) ;           // Send header
  //         // the content of the HTTP response follows the header:
  //         // cmdclient.println ( "Dummy response\n" ) ;        // Text ending with double newline
  //     // cmdclient.print(radioinfo);

  //       reply = analyzeCmd ( "status" ) ;             // Analyze command and handle it
  //       dbgprint ( reply ) ;                     // Result for debugging
  //     oldradioinfo=radioinfo;
  //   }

  // }
 //  tftset ( 1, streamtitle ) ;                   // Set screen segment text middle part
}


//**************************************************************************************************
//                                    S T O P _ M P 3 C L I E N T                                  *
//**************************************************************************************************
// Disconnect from the server.                                                                     *
//**************************************************************************************************
void stop_mp3client ()
{

  while ( mp3client.connected() )
  {
    dbgprint ( "Stopping client" ) ;               // Stop connection to host
    mp3client.flush() ;
    mp3client.stop() ;
    delay ( 500 ) ;
  }
  mp3client.flush() ;                              // Flush stream client
  mp3client.stop() ;                               // Stop stream client
}


//**************************************************************************************************
//                                    C O N N E C T T O H O S T                                    *
//**************************************************************************************************
// Connect to the Internet radio server specified by newpreset.                                    *
//**************************************************************************************************
bool connecttohost()
{
  int         inx ;                                 // Position of ":" in hostname
  uint16_t    port = 80 ;                           // Port number for host
  String      extension = "/" ;                     // May be like "/mp3" in "skonto.ls.lv:8002/mp3"
  String      hostwoext = host ;                    // Host without extension and portnumber

  stop_mp3client() ;                                // Disconnect if still connected
  dbgprint ( "Connect to new host %s", host.c_str() ) ;
  //tftset ( 0, "ESP32-Radio" ) ;                     // Set screen segment text top line
  displaytime ( "" ) ;                              // Clear time on TFT screen
  datamode = INIT ;                                 // Start default in metamode
  chunked = false ;                                 // Assume not chunked
  if ( host.endsWith ( ".m3u" ) )                   // Is it an m3u playlist?
  {
    playlist = host ;                               // Save copy of playlist URL
    datamode = PLAYLISTINIT ;                       // Yes, start in PLAYLIST mode
    if ( playlist_num == 0 )                        // First entry to play?
    {
      playlist_num = 1 ;                            // Yes, set index
    }
    dbgprint ( "Playlist request, entry %d", playlist_num ) ;
  }
  // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
  inx = host.indexOf ( "/" ) ;                      // Search for begin of extension
  if ( inx > 0 )                                    // Is there an extension?
  {
    extension = host.substring ( inx ) ;            // Yes, change the default
    hostwoext = host.substring ( 0, inx ) ;         // Host without extension
  }
  // In the host there may be a portnumber
  inx = hostwoext.indexOf ( ":" ) ;                 // Search for separator
  if ( inx >= 0 )                                   // Portnumber available?
  {
    port = host.substring ( inx + 1 ).toInt() ;     // Get portnumber as integer
    hostwoext = host.substring ( 0, inx ) ;         // Host without portnumber
  }
  dbgprint ( "Connect to %s on port %d, extension %s",
             hostwoext.c_str(), port, extension.c_str() ) ;
  if ( mp3client.connect ( hostwoext.c_str(), port ) )
  {
    dbgprint ( "Connected to server" ) ;
    // This will send the request to the server. Request metadata.
    mp3client.print ( String ( "GET " ) +
                      extension +
                      String ( " HTTP/1.1\r\n" ) +
                      String ( "Host: " ) +
                      hostwoext +
                      String ( "\r\n" ) +
                      String ( "Icy-MetaData:1\r\n" ) +
                      String ( "Connection: close\r\n\r\n" ) ) ;
    return true ;
  }
  dbgprint ( "Request %s failed!", host.c_str() ) ;
  return false ;
}


//**************************************************************************************************
//                                      S S C O N V                                                *
//**************************************************************************************************
// Convert an array with 4 "synchsafe integers" to a number.                                       *
// There are 7 bits used per byte.                                                                 *
//**************************************************************************************************
uint32_t ssconv ( const uint8_t* bytes )
{
  uint32_t res = 0 ;                                      // Result of conversion
  uint8_t  i ;                                            // Counter number of bytes to convert

  for ( i = 0 ; i < 4 ; i++ )                             // Handle 4 bytes
  {
    res = res * 128 + bytes[i] ;                          // Convert next 7 bits
  }
  return res ;                                            // Return the result
}


//**************************************************************************************************
//                                       C O N N E C T T O F I L E                                 *
//**************************************************************************************************
// Open the local mp3-file.                                                                        *
//**************************************************************************************************
bool connecttofile()
{
  String path ;                                           // Full file spec

  tftset ( 0, "ESP32 MP3 Player" ) ;                      // Set screen segment top line
  displaytime ( "" ) ;                                    // Clear time on TFT screen
  path = host.substring ( 9 ) ;                           // Path, skip the "localhost" part
  claimSPI ( "sdopen3" ) ;                                // Claim SPI bus
  handle_ID3 ( path ) ;                                   // See if there are ID3 tags in this file
  mp3filelength = mp3file.available() ;                   // Get length
  releaseSPI() ;                                          // Release SPI bus
  if ( !mp3file )
  {
    dbgprint ( "Error opening file %s", path.c_str() ) ;  // No luck
    return false ;
  }
  mqttpub.trigger ( MQTT_STREAMTITLE ) ;                  // Request publishing to MQTT
  icyname = "" ;                                          // No icy name yet
  chunked = false ;                                       // File not chunked
  metaint = 0 ;                                           // No metadata
  return true ;
}


//**************************************************************************************************
//                                       C O N N E C T W I F I                                     *
//**************************************************************************************************
// Connect to WiFi using the SSID's available in wifiMulti.                                        *
// If only one AP if found in preferences (i.e. wifi_00) the connection is made without            *
// using wifiMulti.                                                                                *
// If connection fails, an AP is created and the function returns false.                           *
//**************************************************************************************************
bool connectwifi()
{
  char*      pfs ;                                      // Pointer to formatted string
  char*      pfs2 ;                                     // Pointer to formatted string
  bool       localAP = false ;                          // True if only local AP is left

  WifiInfo_t winfo ;                                    // Entry from wifilist

  WiFi.disconnect() ;                                   // After restart the router could
  delay(1000);
  WiFi.softAPdisconnect(true) ;                         // still keep the old connection
  if ( wifilist.size()  )                               // Any AP defined?
  {
    if ( wifilist.size() == 1 )                         // Just one AP defined in preferences?
    {
      winfo = wifilist[0] ;                             // Get this entry
      WiFi.begin ( winfo.ssid, winfo.passphrase ) ;     // Connect to single SSID found in wifi_xx
      dbgprint ( "Try WiFi %s", winfo.ssid ) ;          // Message to show during WiFi connect
      pfs2=dbgprint ( "Try WiFi %s", winfo.ssid ) ;  
        tftlog ( pfs2 ) ;
        //oledshow(1,pfs2);
      //tftlog(pfs2 );
    }
    else                                                // More AP to try
    {
      wifiMulti.run() ;                                 // Connect to best network
    }
    if (  WiFi.waitForConnectResult() != WL_CONNECTED ) // Try to connect
    {
      localAP = true ;                                  // Error, setup own AP
      tolocalap=true;

    dbgprint ("tolocalap=true" ) ;  
    }
  }
  else
  {
    localAP = true ;                                    // Not even a single AP defined
    tolocalap=true;

    dbgprint ("tolocalap=true" ) ;  
  }
  if ( localAP )                                        // Must setup local AP?
  {
    dbgprint ( "WiFi Failed!  Trying to setup AP with name %s and password %s.", NAME, NAME ) ;
    WiFi.softAP ( NAME, NAME ) ;                        // This ESP will be an AP
    pfs = dbgprint ( "IP = 192.168.4.1" ) ;             // Address for AP
    oledshow(1,"Station mode");
    oledshow(2, "IP = 192.168.4.1\npass = ESP32Radio\nvoldown to avoid timeout\nvolup to force restart");
  }
  else
  {
   //ipaddress = "Wlan0 : "+WiFi.SSID() +"\n"+"IP    : "+WiFi.localIP().toString() ;             // Form IP address
   ipaddress = "Wlan0 : "+WiFi.SSID()  ;             // Form IP address
    pfs2 = dbgprint ( "Connected to %s", WiFi.SSID().c_str() ) ;
    // display.drawString(0,10, String(pfs2));
     //tftset ( 1, (WiFi.SSID().c_str()) ) ;                     // Set screen segment text top line
  tftlog ( pfs2 ) ;
   oledshow(3, ipaddress);
   //pfs = dbgprint ( "IP = %s", ipaddress.c_str() ) ;   // String to dispay on TFT
   //pfs = dbgprint (  ipaddress.c_str() ) ;   // String to dispay on TFT
 //char infox= ("SSID : "&(WiFi.SSID().c_str()) );
  //char infox2= " - IP : " & (ipaddress.c_str());
  char* sssid=(pfs ,pfs2);
  //tftset ( 1, ("SSID : "&(WiFi.SSID().c_str()) & " - IP : " & (ipaddress.c_str()))) ;  
  //const char sx= (" - ");
  //const char arrayx[2]={pfs, pfs2};
  //tftset(1,"%s\n",arrayx);
  //tftset(0,ipaddress);
  oledshow(0,ipaddress);

    // display.drawString(0,21, String(pfs));
  }
 //tftlog ( pfs ) ;                                      // Show IP
     // display.drawString(0,20, String(pfs));

  // delay ( 3000 ) ;                                      // Allow user to read this
  return ( localAP == false ) ;                         // Return result of connection
}


//**************************************************************************************************
//                                           O T A S T A R T                                       *
//**************************************************************************************************
// Update via WiFi has been started by Arduino IDE.                                                *
//**************************************************************************************************
void otastart()
{
  char* p ;

  p = dbgprint ( "OTA update Started" ) ;
  tftset ( 2, p ) ;                                   // Set screen segment bottom part
}


//**************************************************************************************************
//                                  R E A D H O S T F R O M P R E F                                *
//**************************************************************************************************
// Read the mp3 host from the preferences specified by the parameter.                              *
// The host will be returned.                                                                      *
//**************************************************************************************************
String readhostfrompref ( int8_t preset )
{
  char           tkey[12] ;                            // Key as an array of chars

  sprintf ( tkey, "preset_%02d", preset ) ;            // Form the search key
  if ( nvssearch ( tkey ) )                            // Does it exists?
  {
    // Get the contents
    return nvsgetstr ( tkey ) ;                        // Get the station (or empty sring)
  }
  else
  {
    return String ( "" ) ;                             // Not found
  }
}


//**************************************************************************************************
//                                  R E A D H O S T F R O M P R E F                                *
//**************************************************************************************************
// Search for the next mp3 host in preferences specified newpreset.                                *
// The host will be returned.  newpreset will be updated                                           *
//**************************************************************************************************
String readhostfrompref()
{
  String contents = "" ;                                // Result of search
  int    maxtry = 0 ;                                   // Limit number of tries

  while ( ( contents = readhostfrompref ( ini_block.newpreset ) ) == "" )
  {
    if ( ++ maxtry > 99 )
    {
      return "" ;
    }
    if ( ++ini_block.newpreset > 99 )                   // Next or wrap to 0
    {
      ini_block.newpreset = 0 ;
    }
  }
  // Get the contents
  return contents ;                                     // Return the station
}


//**************************************************************************************************
//                                       R E A D P R O G B U T T O N S                             *
//**************************************************************************************************
// Read the preferences for the programmable input pins and the touch pins.                        *
//**************************************************************************************************
void readprogbuttons()
{
  char        mykey[20] ;                                   // For numerated key
  int8_t      pinnr ;                                       // GPIO pinnumber to fill
  int         i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry

  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan for all programmable pins
  {
    sprintf ( mykey, "gpio_%02d", i ) ;                 // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !progpin[i].reserved )                         // Do not use reserved pins
        {
          progpin[i].avail = true ;                         // This one is active now
          progpin[i].command = val ;                        // Set command
          dbgprint ( "gpio_%02d will execute %s",           // Show result
                     i, val.c_str() ) ;
          dbgprint ( "Level is now %d",
                     touchRead ( pinnr ) ) ;                // Sample the pin
  
        }
      }
    }
  }
  // Now for the touch pins 0..9, identified by their GPIO pin number
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan for all programmable pins
  {
    sprintf ( mykey, "touch_%02d", pinnr ) ;                // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !touchpin[i].reserved )                        // Do not use reserved pins
        {
          touchpin[i].avail = true ;                        // This one is active now
          touchpin[i].command = val ;                       // Set command
          //pinMode ( touchpin[i].gpio,  INPUT ) ;          // Free floating input
          dbgprint ( "touch_%02d will execute %s",          // Show result
                     pinnr, val.c_str() ) ;
        }
        else
        {
          dbgprint ( "touch_%02d pin (GPIO%02d) is reserved for I/O!",
                     i, pinnr ) ;

        }
      }
    }
  }
}


//**************************************************************************************************
//                                       R E S E R V E P I N                                       *
//**************************************************************************************************
// Set I/O pin to "reserved".                                                                      *
// The pin is than not available for a programmable function.                                      *
//**************************************************************************************************
void reservepin ( int8_t rpinnr )
{
  uint8_t i = 0 ;                                           // Index in progpin/touchpin array
  int8_t  pin ;                                             // Pin number in progpin array

  while ( ( pin = progpin[i].gpio ) >= 0 )                  // Find entry for requested pin
  {
    if ( pin == rpinnr )                                    // Entry found?
    {
      //dbgprint ( "GPIO%02d unavailabe for 'gpio_'-command", pin ) ;
      progpin[i].reserved = true ;                          // Yes, pin is reserved now
      break ;                                               // No need to continue
    }
    i++ ;                                                   // Next entry
  }
  // Also reserve touchpin numbers
  i = 0 ;
  while ( ( pin = touchpin[i].gpio ) >= 0 )                 // Find entry for requested pin
  {
    if ( pin == rpinnr )                                    // Entry found?
    {
      //dbgprint ( "GPIO%02d unavailabe for 'touch'-command", pin ) ;
      touchpin[i].reserved = true ;                         // Yes, pin is reserved now
      break ;                                               // No need to continue
    }
    i++ ;                                                   // Next entry
  }
}


//**************************************************************************************************
//                                       R E A D I O P R E F S                                     *
//**************************************************************************************************
// Scan the preferences for IO-pin definitions.                                                    *
//**************************************************************************************************
void readIOprefs()
{
  struct iosetting
  {
    const char* gname ;                                   // Name in preferences
    int8_t*     gnr ;                                     // GPIO pin number
    int8_t      pdefault ;                                // Default pin
  };
  struct iosetting klist[] = {                            // List of I/O related keys
    { "pin_ir",       &ini_block.ir_pin,          -1          },
    { "pin_enc_clk",  &ini_block.enc_clk_pin,     -1          },
    { "pin_enc_dt",   &ini_block.enc_dt_pin,      -1          },
    { "pin_enc_sw",   &ini_block.enc_sw_pin,      -1          },
    { "pin_tft_cs",   &ini_block.tft_cs_pin,      -1          },   // Display SPI version
    { "pin_tft_dc",   &ini_block.tft_dc_pin,      -1          },   // Display SPI version
    { "pin_tft_scl",  &ini_block.tft_scl_pin,     -1          },   // Display I2C version
    { "pin_tft_sda",  &ini_block.tft_sda_pin,     -1          },   // Display I2C version
    { "pin_sd_cs",    &ini_block.sd_cs_pin,       -1          },
    { "pin_vs_cs",    &ini_block.vs_cs_pin,       -1          },
    { "pin_vs_dcs",   &ini_block.vs_dcs_pin,      -1          },
    { "pin_vs_dreq",  &ini_block.vs_dreq_pin,     -1          },
    { "pin_shutdown", &ini_block.vs_shutdown_pin, -1          },
    { "pin_spi_sck",  &ini_block.spi_sck_pin,     18          },
    { "pin_spi_miso", &ini_block.spi_miso_pin,    19          },
    { "pin_spi_mosi", &ini_block.spi_mosi_pin,    23          },
    { NULL,           NULL,                       0           }    // End of list
  } ;
  int         i ;                                         // Loop control
  int         count = 0 ;                                 // Number of keys found
  String      val ;                                       // Contents of preference entry
  int8_t      ival ;                                      // Value converted to integer
  int8_t*     p ;                                         // Points to variable

  for ( i = 0 ; klist[i].gname ; i++ )                    // Loop trough all I/O related keys
  {
    p = klist[i].gnr ;                                    // Point to target variable
    ival = klist[i].pdefault ;                            // Assume pin number to be the default
    if ( nvssearch ( klist[i].gname ) )                   // Does it exist?
    {
      val = nvsgetstr ( klist[i].gname ) ;                // Read value of key
      if ( val.length() )                                 // Parameter in preference?
      {
        count++ ;                                         // Yes, count number of filled keys
        ival = val.toInt() ;                              // Convert value to integer pinnumber
        reservepin ( ival ) ;                             // Set pin to "reserved"
      }
    }
    *p = ival ;                                           // Set pinnumber in ini_block
    dbgprint ( "%s set to %d",                            // Show result
               klist[i].gname,
               ival ) ;
  }
}


//**************************************************************************************************
//                                       R E A D P R E F S                                         *
//**************************************************************************************************
// Read the preferences and interpret the commands.                                                *
// If output == true, the key / value pairs are returned to the caller as a String.                *
//**************************************************************************************************
String readprefs ( bool output )
{
  uint16_t    i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry
  String      cmd ;                                         // Command for analyzCmd
  String      outstr = "" ;                                 // Outputstring
  char*       key ;                                         // Point to nvskeys[i]
  uint8_t     winx ;                                        // Index in wifilist
  uint16_t    last2char = 0 ;                               // To detect paragraphs

  i = 0 ;
  while ( *( key = nvskeys[i] ) )                           // Loop trough all available keys
  {
    val = nvsgetstr ( key ) ;                               // Read value of this key
    cmd = String ( key ) +                                  // Yes, form command
          String ( " = " ) +
          val ;
    if ( strstr ( key, "wifi_"  ) )                         // Is it a wifi ssid/password?
    {
      winx = atoi ( key + 5 ) ;                             // Get index in wifilist
      if ( ( winx < wifilist.size() ) &&                    // Existing wifi spec in wifilist?
           ( val.indexOf ( wifilist[winx].ssid ) == 0 ) )
      {
        val = String ( wifilist[winx].ssid ) +              // Yes, hide password
              String ( "/*******" ) ;
      }
      cmd = String ( "" ) ;                                 // Do not analyze this
    }
    else if ( strstr ( key, "mqttpasswd"  ) )               // Is it a MQTT password?
    {
      val = String ( "*******" ) ;                          // Yes, hide it
    }
    if ( output )
    {
      if ( ( i > 0 ) &&
           ( *(uint16_t*)key != last2char ) )               // New paragraph?
      {
        outstr += String ( "#\n" ) ;                        // Yes, add separator
      }
      last2char = *(uint16_t*)key ;                         // Save 2 chars for next compare
      outstr += String ( key ) +                            // Add to outstr
                String ( " = " ) +
                val +
                String ( "\n" ) ;                           // Add newline
    }
    else
    {
      analyzeCmd ( cmd.c_str() ) ;                          // Analyze it
    }
    i++ ;                                                   // Next key
  }
  if ( i == 0 )
  {
    outstr = String ( "No preferences found.\n"
                      "Use defaults or run Esp32_radio_init first.\n" ) ;
  }
  return outstr ;
}


//**************************************************************************************************
//                                    M Q T T R E C O N N E C T                                    *
//**************************************************************************************************
// Reconnect to broker.                                                                            *
//**************************************************************************************************
bool mqttreconnect()
{
  static uint32_t retrytime = 0 ;                         // Limit reconnect interval
  bool            res = false ;                           // Connect result
  char            clientid[20] ;                          // Client ID
  char            subtopic[20] ;                          // Topic to subscribe

  if ( ( millis() - retrytime ) < 5000 )                  // Don't try to frequently
  {
    return res ;
  }
  retrytime = millis() ;                                  // Set time of last try
  if ( mqttcount > MAXMQTTCONNECTS )                      // Tried too much?
  {
    mqtt_on = false ;                                     // Yes, switch off forever
    return res ;                                          // and quit
  }
  mqttcount++ ;                                           // Count the retries
  dbgprint ( "(Re)connecting number %d to MQTT %s",       // Show some debug info
             mqttcount,
             ini_block.mqttbroker.c_str() ) ;
  sprintf ( clientid, "%s-%04d",                          // Generate client ID
            NAME, (int) random ( 10000 ) % 10000 ) ;
  res = mqttclient.connect ( clientid,                    // Connect to broker
                             ini_block.mqttuser.c_str(),
                             ini_block.mqttpasswd.c_str()
                           ) ;
  if ( res )
  {
    sprintf ( subtopic, "%s/%s",                          // Add prefix to subtopic
              ini_block.mqttprefix.c_str(),
              MQTT_SUBTOPIC ) ;
    res = mqttclient.subscribe ( subtopic ) ;             // Subscribe to MQTT
    if ( !res )
    {
      dbgprint ( "MQTT subscribe failed!" ) ;             // Failure
    }
    mqttpub.trigger ( MQTT_IP ) ;                         // Publish own IP
  }
  else
  {
    dbgprint ( "MQTT connection failed, rc=%d",
               mqttclient.state() ) ;

  }
  return res ;
}


//**************************************************************************************************
//                                    O N M Q T T M E S S A G E                                    *
//**************************************************************************************************
// Executed when a subscribed message is received.                                                 *
// Note that message is not delimited by a '\0'.                                                   *
// Note that cmd buffer is shared with serial input.                                               *
//**************************************************************************************************
void onMqttMessage ( char* topic, byte* payload, unsigned int len )
{
  const char*  reply ;                                // Result from analyzeCmd

  if ( strstr ( topic, MQTT_SUBTOPIC ) )              // Check on topic, maybe unnecessary
  {
    if ( len >= sizeof(cmd) )                         // Message may not be too long
    {
      len = sizeof(cmd) - 1 ;
    }
    strncpy ( cmd, (char*)payload, len ) ;            // Make copy of message
    cmd[len] = '\0' ;                                 // Take care of delimeter
    dbgprint ( "MQTT message arrived [%s], lenght = %d, %s", topic, len, cmd ) ;
    reply = analyzeCmd ( cmd ) ;                      // Analyze command and handle it
    dbgprint ( reply ) ;                              // Result for debugging
  }
}


//**************************************************************************************************
//                                     S C A N S E R I A L                                         *
//**************************************************************************************************
// Listen to commands on the Serial inputline.                                                     *
//**************************************************************************************************
void scanserial()
{
  static String serialcmd ;                      // Command from Serial input
  char          c ;                              // Input character
  const char*   reply ;                          // Reply string froma analyzeCmd
  uint16_t      len ;                            // Length of input string

  while ( Serial.available() )                   // Any input seen?
  {
    c =  (char)Serial.read() ;                   // Yes, read the next input character
    //Serial.write ( c ) ;                       // Echo
    len = serialcmd.length() ;                   // Get the length of the current string
    if ( ( c == '\n' ) || ( c == '\r' ) )
    {
      if ( len )
      {
        strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
        reply = analyzeCmd ( cmd ) ;             // Analyze command and handle it
        dbgprint ( reply ) ;                     // Result for debugging
        serialcmd = "" ;                         // Prepare for new command
      }
    }
    if ( c >= ' ' )                              // Only accept useful characters
    {
      serialcmd += c ;                           // Add to the command
    }
    if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
    {
      serialcmd = "" ;                           // Too long, reset
    }
  }
}


//**************************************************************************************************
//                                     S C A N D I G I T A L                                       *
//**************************************************************************************************
// Scan digital inputs.                                                                            *
//**************************************************************************************************
void  scandigital()
{
  static uint32_t oldmillis = 5000 ;                        // To compare with current time
  int             i ;                                       // Loop control
  int8_t          pinnr ;                                   // Pin number to check
  bool            level ;                                   // Input level
  const char*     reply ;                                   // Result of analyzeCmd
  int16_t         tlevel ;                                  // Level found by touch pin
  const int16_t   THRESHOLD = 30 ;                          // Threshold or touch pins

  if ( ( millis() - oldmillis ) < 100 )                     // Debounce
  {
    return ;
  }
  scanios++ ;                                               // TEST*TEST*TEST
  oldmillis = millis() ;                                    // 100 msec over
  /**
  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan all inputs
  {
    if ( !progpin[i].avail || progpin[i].reserved )         // Skip unused and reserved pins
    {
      continue ;
    }
    level = ( digitalRead ( pinnr ) == HIGH ) ;             // Sample the pin
    if ( level != progpin[i].cur )                          // Change seen?
    {
      progpin[i].cur = level ;                              // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        dbgprint ( "GPIO_%02d is now LOW, execute %s",
                   pinnr, progpin[i].command.c_str() ) ;
        reply = analyzeCmd ( progpin[i].command.c_str() ) ; // Analyze command and handle it
        dbgprint ( reply ) ;                                // Result for debugging
      }
    }
  }
  // Now for the touch pins
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan all inputs
  {
    if ( !touchpin[i].avail || touchpin[i].reserved )       // Skip unused and reserved pins
    {
      continue ;
    }
    tlevel = ( touchRead ( pinnr ) ) ;                      // Sample the pin
    oledshow(2, String(tlevel));
    dbgprint(String(tlevel).c_str());
    level = ( tlevel >= 30 ) ;                              // True if below threshold
    dbgprint(" touch level = " + tlevel);
    if ( level )                                            // Level HIGH?
    {
      touchpin[i].count = 0 ;                               // Reset count number of times
    }
    else
    {
      if ( ++touchpin[i].count < 3 )                        // Count number of times LOW
      {
        level = true ;                                      // Not long enough: handle as HIGH
      }
    }
    if ( level != touchpin[i].cur )                         // Change seen?
    {
      touchpin[i].cur = level ;                             // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        dbgprint ( "TOUCH_%02d is now %d ( < %d ), execute %s",
                   pinnr, tlevel, THRESHOLD,
                   touchpin[i].command.c_str() ) ;
        reply = analyzeCmd ( touchpin[i].command.c_str() ); // Analyze command and handle it
        dbgprint ( reply ) ;                                // Result for debugging
      }
    }
  }
touch_listener();
**/
}



/**
//==========handle touchcontrol
//function for touch_listener
void touch_listener(){                    
    
   if(startlisteningtouch==1){
      tc4=touchRead(T4);
      tc5=touchRead(T5);   
      tc6=touchRead(T3);
      // tc2=touchRead(T2);
   }


  // dbgprint(("   "+String(tc6)).c_str());

    // dbgprint(String(tc2).c_str());


if(tc6>78){


  if(prevtc6<65&&prevtc6>0){
        
      if(tmode==0){tmode=1;oledshow ( 3, "touchmode : preset");
        tmppreset=currentpreset;
      }else{tmode=0;oledshow ( 3, "touchmode : volume");}  
      
      dbgprint(("   "+String(tc6)).c_str());
      dbgprint(String(tc6).c_str());   
      tlrate=5;
  }
}else{tlrate=1;}

prevtc6=tc6;




//adjust ++ 
//===touch4==GPIO12
if(tc4<75){


  if(prevtc4<75&&prevtc4>0){
        
      if(tmode==0){
          curvsvol = vs1053player->getVolume() ;    
          curvsvol+=2;
          if((curvsvol+2)>90){curvsvol=90;}
          ini_block.reqvol =curvsvol;
          muteflag = false ;
        // oledshow(3, "     " +String(tc4));    
          oledshow ( 3, "Volume is now : "+ String(curvsvol))  ;
          oledshow(1, "preset : " +  String(ini_block.newpreset)+" | volume : " +  String(curvsvol));
          // dbgprint(   (("TC5 = "+String(tc5)).c_str())   +(("TC4 = "+String(tc4)).c_str()));
          dbgprint(("             TC4   "+String(tc4)).c_str());
          dbgprint(String(tc5).c_str());   
          tlrate=5;

      }else{
        // datamode = STOPREQD ; 
        ini_block.newpreset++;
        tmppreset+=1;
        if(tmppreset>17){tmppreset=0;}
        currentpreset=ini_block.newpreset;
        oledshow ( 3, "Preset is now : "+  String(ini_block.newpreset))  ;
        // oledshow(1, "preset : " +  String(ini_block.newpreset)+" | volume : " +  String(curvsvol));
        tlrate=5;

      }


      tmrinfo=0;
      dbgprint(("tlrate =  "+String(tlrate)).c_str());

      
  }
}else{tlrate=1;}

prevtc4=tc4;



//adjust--
//===touch5==GPIO12
if(tc5<80){
  if(prevtc5<80&&prevtc5>0){


    if(tmode==0){
        curvsvol = vs1053player->getVolume() ;    
        curvsvol-=2;
        if((curvsvol-2)<50){curvsvol=50;}
        ini_block.reqvol =curvsvol;
        muteflag = false ;
      // oledshow(3, String(tc5));    
        oledshow ( 3, "Volume is now : "+String(curvsvol))  ;
        oledshow(1, "preset : " +  String(ini_block.newpreset)+" | volume : " +  String(curvsvol));
          dbgprint(   ("TC4 = "+String(tc4)).c_str());
        dbgprint(String(tc5).c_str());
        tlrate=5;

      }else{
//        datamode = STOPREQD ; 
        ini_block.newpreset--;
        tmppreset-=1;
        if(tmppreset<0){tmppreset=17;}
        currentpreset=ini_block.newpreset;

        oledshow ( 3, "Preset is now : "+  String(ini_block.newpreset))  ;
        oledshow(1, "preset : " +  String(ini_block.newpreset)+" | volume : " +  String(curvsvol));
        tlrate=5;

      }

      tmrinfo=0;
      dbgprint(("tlrate =  "+String(tlrate)).c_str());
  }
}else{tlrate=1;}
prevtc5=tc5;

}   
**/



//**************************************************************************************************
//                                           M K _ L S A N                                         *
//**************************************************************************************************
// Make al list of acceptable networks in preferences.                                             *
// Will be called only once by setup().                                                            *
// The result will be stored in wifilist.                                                          *
// Not that the last found SSID and password are kept in common data.  If only one SSID is         *
// defined, the connect is made without using wifiMulti.  In this case a connection will           *
// be made even if de SSID is hidden.                                                              *
//**************************************************************************************************
void  mk_lsan()
{
  uint8_t     i ;                                        // Loop control
  char        key[10] ;                                  // For example: "wifi_03"
  String      buf ;                                      // "SSID/password"
  String      lssid, lpw ;                               // Last read SSID and password from nvs
  int         inx ;                                      // Place of "/"
  WifiInfo_t  winfo ;                                    // Element to store in list

  dbgprint ( "Create list with acceptable WiFi networks" ) ;
  for ( i = 0 ; i < 100 ; i++ )                          // Examine wifi_00 .. wifi_99
  {
    sprintf ( key, "wifi_%02d", i ) ;                    // Form key in preferences
    if ( nvssearch ( key  ) )                            // Does it exists?
    {
      buf = nvsgetstr ( key ) ;                          // Get the contents
      inx = buf.indexOf ( "/" ) ;                        // Find separator between ssid and password
      if ( inx > 0 )                                     // Separator found?
      {
        lpw = buf.substring ( inx + 1 ) ;                // Isolate password
        lssid = buf.substring ( 0, inx ) ;               // Holds SSID now
        dbgprint ( "Added %s to list of networks",
                   lssid.c_str() ) ;
        winfo.inx = i ;                                  // Create new element for wifilist ;
        winfo.ssid = strdup ( lssid.c_str() ) ;          // Set ssid of element
        winfo.passphrase = strdup ( lpw.c_str() ) ;
        wifilist.push_back ( winfo ) ;                   // Add to list
        wifiMulti.addAP ( winfo.ssid,                    // Add to wifi acceptable network list
                          winfo.passphrase ) ;
      }
    }
  }
  dbgprint ( "End adding networks" ) ; ////
}


//**************************************************************************************************
//                                     G E T R A D I O S T A T U S                                 *
//**************************************************************************************************
// Return preset-, tone- and volume status.                                                        *
// Included are the presets, the current station, the volume and the tone settings.                *
//**************************************************************************************************
String getradiostatus()
{
  char                pnr[3] ;                           // Preset as 2 character, i.e. "03"

  sprintf ( pnr, "%02d", ini_block.newpreset ) ;         // Current preset
  return String ( "preset=" ) +                          // Add preset setting
         String ( pnr ) +
         String ( "\nvolume=" ) +                        // Add volume setting
         String ( String ( ini_block.reqvol ) ) +
         String ( "\ntoneha=" ) +                        // Add tone setting HA
         String ( ini_block.rtone[0] ) +
         String ( "\ntonehf=" ) +                        // Add tone setting HF
         String ( ini_block.rtone[1] ) +
         String ( "\ntonela=" ) +                        // Add tone setting LA
         String ( ini_block.rtone[2] ) +
         String ( "\ntonelf=" ) +                        // Add tone setting LF
         String ( ini_block.rtone[3] ) ;
 //          display.clear();

 // // display.drawString(0,0, streamtitle);
 // display.drawStringMaxWidth(0, 0, 128,pnr);
 // display.display();
}


//**************************************************************************************************
//                                     G E T S E T T I N G S                                       *
//**************************************************************************************************
// Send some settings to the webserver.                                                            *
// Included are the presets, the current station, the volume and the tone settings.                *
//**************************************************************************************************
void getsettings()
{
  String              val ;                              // Result to send
  String              statstr ;                          // Station string
  int                 inx ;                              // Position of search char in line
  int                 i ;                                // Loop control, preset number
  char                tkey[12] ;                         // Key for preset preference

  for ( i = 0 ; i < 100 ; i++ )                          // Max 99 presets
  {
    sprintf ( tkey, "preset_%02d", i ) ;                 // Preset plus number
    if ( nvssearch ( tkey ) )                            // Does it exists?
    {
      // Get the contents
      statstr = nvsgetstr ( tkey ) ;                     // Get the station
      // Show just comment if available.  Otherwise the preset itself.
      inx = statstr.indexOf ( "#" ) ;                    // Get position of "#"
      if ( inx > 0 )                                     // Hash sign present?
      {
        statstr.remove ( 0, inx + 1 ) ;                  // Yes, remove non-comment part
      }
      chomp ( statstr ) ;                                // Remove garbage from description
      val += String ( tkey ) +
             String ( "=" ) +
             statstr +
             String ( "\n" ) ;                           // Add delimeter
      if ( val.length() > 1000 )                         // Time to flush?
      {
        cmdclient.print ( val ) ;                        // Yes, send
        val = "" ;                                       // Start new string
      }
    }
  }
  val += getradiostatus() +                              // Add radio setting
         String ( "\n\n" ) ;                             // End of reply
  cmdclient.print ( val ) ;                              // And send
}





//**************************************************************************************************
//                                   F I N D N S I D                                               *
//**************************************************************************************************
// Find the namespace ID for the namespace passed as parameter.                                    *
//**************************************************************************************************
uint8_t FindNsID ( const char* ns )
{
  esp_err_t                 result = ESP_OK ;                 // Result of reading partition
  uint32_t                  offset = 0 ;                      // Offset in nvs partition
  uint8_t                   i ;                               // Index in Entry 0..125
  uint8_t                   bm ;                              // Bitmap for an entry
  uint8_t                   res = 0xFF ;                      // Function result

  while ( offset < nvs->size )
  {
    result = esp_partition_read ( nvs, offset,                // Read 1 page in nvs partition
                                  &nvsbuf,
                                  sizeof(nvsbuf) ) ;
    if ( result != ESP_OK )
    {
      dbgprint ( "Error reading NVS!" ) ;
      break ;
    }
    i = 0 ;
    while ( i < 126 )
    {

      bm = ( nvsbuf.Bitmap[i / 4] >> ( ( i % 4 ) * 2 ) ) ;    // Get bitmap for this entry,
      bm &= 0x03 ;                                            // 2 bits for one entry
      if ( ( bm == 2 ) &&
           ( nvsbuf.Entry[i].Ns == 0 ) &&
           ( strcmp ( ns, nvsbuf.Entry[i].Key ) == 0 ) )
      {
        res = nvsbuf.Entry[i].Data & 0xFF ;                   // Return the ID
        offset = nvs->size ;                                  // Stop outer loop as well
        break ;
      }
      else
      {
        if ( bm == 2 )
        {
          i += nvsbuf.Entry[i].Span ;                         // Next entry
        }
        else
        {
          i++ ;
        }
      }
    }
    offset += sizeof(nvs_page) ;                              // Prepare to read next page in nvs
  }
  return res ;
}


//**************************************************************************************************
//                            B U B B L E S O R T K E Y S                                          *
//**************************************************************************************************
// Bubblesort the nvskeys.                                                                         *
//**************************************************************************************************
void bubbleSortKeys ( uint16_t n )
{
  uint16_t i, j ;                                             // Indexes in nvskeys
  char     tmpstr[16] ;                                       // Temp. storage for a key

  for ( i = 0 ; i < n - 1 ; i++ )                             // Examine all keys
  {
    for ( j = 0 ; j < n - i - 1 ; j++ )                       // Compare to following keys
    {
      if ( strcmp ( nvskeys[j], nvskeys[j + 1] ) > 0 )        // Next key out of order?
      {
        strcpy ( tmpstr, nvskeys[j] ) ;                       // Save current key a while
        strcpy ( nvskeys[j], nvskeys[j + 1] ) ;               // Replace current with next key
        strcpy ( nvskeys[j + 1], tmpstr ) ;                   // Replace next with saved current
      }
    }
  }
}


//**************************************************************************************************
//                                      F I L L K E Y L I S T                                      *
//**************************************************************************************************
// File the list of all relevant keys in NVS.                                                      *
// The keys will be sorted.                                                                        *
//**************************************************************************************************
void fillkeylist()
{
  esp_err_t    result = ESP_OK ;                                // Result of reading partition
  uint32_t     offset = 0 ;                                     // Offset in nvs partition
  uint16_t     i ;                                              // Index in Entry 0..125.
  uint8_t      bm ;                                             // Bitmap for an entry
  uint16_t     nvsinx = 0 ;                                     // Index in nvskey table

  keynames.clear() ;                                            // Clear the list
  while ( offset < nvs->size )
  {
    result = esp_partition_read ( nvs, offset,                  // Read 1 page in nvs partition
                                  &nvsbuf,
                                  sizeof(nvsbuf) ) ;
    if ( result != ESP_OK )
    {
      dbgprint ( "Error reading NVS!" ) ;
      break ;
    }
    i = 0 ;
    while ( i < 126 )
    {


      bm = ( nvsbuf.Bitmap[i / 4] >> ( ( i % 4 ) * 2 ) ) ;      // Get bitmap for this entry,
      bm &= 0x03 ;                                              // 2 bits for one entry
      if ( bm == 2 )                                            // Entry is active?
      {
        if ( nvsbuf.Entry[i].Ns == namespace_ID )               // Namespace right?
        {
          strcpy ( nvskeys[nvsinx], nvsbuf.Entry[i].Key ) ;     // Yes, save in table
          if ( ++nvsinx == MAXKEYS )
          {
            nvsinx-- ;                                          // Prevent excessive index
          }
        }
        i += nvsbuf.Entry[i].Span ;                             // Next entry
      }
      else
      {
        i++ ;
      }
    }
    offset += sizeof(nvs_page) ;                                // Prepare to read next page in nvs
  }
  nvskeys[nvsinx][0] = '\0' ;                                   // Empty key at the end
  dbgprint ( "Read %d keys from NVS", nvsinx ) ;
  bubbleSortKeys ( nvsinx ) ;                                   // Sort the keys
}

//**************************************************************************************************
//                                           S E T U P                                             *
//**************************************************************************************************
// Setup for the program.                                                                          *
//**************************************************************************************************
void setup()
{
  int                       i ;                          // Loop control
  int                       pinnr ;                      // Input pinnumber
  const char*               p ;
  byte                      mac[6] ;                     // WiFi mac address
  char                      tmpstr[20] ;                 // For version and Mac address
  const char*               partname = "nvs" ;           // Partition with NVS info
  esp_partition_iterator_t  pi ;                         // Iterator for find
  const char*               wvn = "Include file %s_html has the wrong version number!"
                                  "Replace header file." ;

  Serial.begin ( 115200 ) ;                              // For debug
  Serial.println() ;

  // Version tests for some vital include files
  if ( about_html_version   < 170626 ) dbgprint ( wvn, "about" ) ;
  if ( config_html_version  < 171207 ) dbgprint ( wvn, "config" ) ;
  if ( index_html_version   < 180102 ) dbgprint ( wvn, "index" ) ;
  if ( mp3play_html_version < 170626 ) dbgprint ( wvn, "mp3play" ) ;
  if ( defaultprefs_version < 190609 ) dbgprint ( wvn, "defaultprefs" ) ;
  // Print some memory and sketch info
  dbgprint ( "Starting ESP32-radio running on CPU %d at %d MHz.  Version %s.  Free memory %d",
             xPortGetCoreID(),
             ESP.getCpuFreqMHz(),
             VERSION,
             ESP.getFreeHeap() ) ;                       // Normally about 170 kB
  maintask = xTaskGetCurrentTaskHandle() ;               // My taskhandle
  SPIsem = xSemaphoreCreateMutex(); ;                    // Semaphore for SPI bus
  pi = esp_partition_find ( ESP_PARTITION_TYPE_DATA,     // Get partition iterator for
                            ESP_PARTITION_SUBTYPE_ANY,   // the NVS partition
                            partname ) ;
  if ( pi )
  {
    nvs = esp_partition_get ( pi ) ;                     // Get partition struct
    esp_partition_iterator_release ( pi ) ;              // Release the iterator
    dbgprint ( "Partition %s found, %d bytes",
               partname,
               nvs->size ) ;
  }
  else
  {
    dbgprint ( "Partition %s not found!", partname ) ;   // Very unlikely...
    while ( true ) ;                                     // Impossible to continue
  }
  namespace_ID = FindNsID ( NAME ) ;                     // Find ID of our namespace in NVS
  fillkeylist() ;                                        // Fill keynames with all keys
  memset ( &ini_block, 0, sizeof(ini_block) ) ;          // Init ini_block
  ini_block.mqttport = 1883 ;                            // Default port for MQTT
  ini_block.mqttprefix = "" ;                            // No prefix for MQTT topics seen yet
  ini_block.clk_server = "id.pool.ntp.org" ;                // Default server for NTP
  ini_block.clk_offset = 7 ;                             // Default Amsterdam time zone
  ini_block.clk_dst = 0 ;                                // DST is +1 hour
  ini_block.bat0 = 0 ;                                   // Battery ADC levels not yet defined
  ini_block.bat100 = 0 ;
  readIOprefs() ;                                        // Read pins used for SPI, TFT, VS1053, IR,
                                                         // Rotary encoder

  // for(int i=1; i<5; i++){
  //     pinMode("touch"+String(i), INPUT);
  // }
  pinMode(touch1, INPUT);
  pinMode(touch2, INPUT);
  pinMode(touch3, INPUT);
  pinMode(touch4, INPUT);
  pinMode(pinleddummyload, OUTPUT);
  pinMode(led_connnected_in, OUTPUT);
  
  // for ( i = 0 ; (pinnr = progpin[i].gpio) >= 0 ; i++ )   // Check programmable input pins
  // {
  //   pinMode ( pinnr, INPUT_PULLUP ) ;                    // Input for control button
  //   delay ( 10 ) ;
  //   // Check if pull-up active
  //   if ( ( progpin[i].cur = digitalRead ( pinnr ) ) == HIGH )
  //   {
  //     p = "HIGH" ;
  //   }
  //   else
  //   {
  //     p = "LOW, probably no PULL-UP" ;                   // No Pull-up
  //   }
  //   dbgprint ( "GPIO%d is %s", pinnr, p ) ;
  // }
  // readprogbuttons() ;                                    // Program the free input pins


  SPI.begin ( ini_block.spi_sck_pin,                     // Init VSPI bus with default or modified pins
              ini_block.spi_miso_pin,
              ini_block.spi_mosi_pin ) ;
  vs1053player = new VS1053 ( ini_block.vs_cs_pin,       // Make instance of player
                              ini_block.vs_dcs_pin,
                              ini_block.vs_dreq_pin,
                              ini_block.vs_shutdown_pin ) ;
  if ( ini_block.ir_pin >= 0 )
  {
    dbgprint ( "Enable pin %d for IR",
               ini_block.ir_pin ) ;
    pinMode ( ini_block.ir_pin, INPUT ) ;                // Pin for IR receiver VS1838B
    attachInterrupt ( ini_block.ir_pin,                  // Interrupts will be handle by isr_IR
                      isr_IR, CHANGE ) ;
  }
  if ( ( ini_block.tft_cs_pin >= 0  ) ||                 // Display configured?
       ( ini_block.tft_scl_pin >= 0 ) )
  {
    dbgprint ( "Start display" ) ;
    if ( dsp_begin() )                                   // Init display
    {
      // dsp_setRotation() ;                                // Use landscape format
      dsp_erase() ;                                      // Clear screen
      // dsp_setTextSize ( 2 ) ;                            // Small character font
      dsp_setTextColor ( WHITE ) ;                       // Info in white
      dsp_setCursor ( 0, 0 ) ;                           // Top of screen
      //dsp_print ( "Starting..." "\n" "Version:" ) ;
      //strncpy ( tmpstr, VERSION, 16 ) ;                  // Limit version length
      //dsp_println ( tmpstr ) ;
      //dsp_println ( "By Ed Smallenburg" ) ;
      
      dsp_fillRect ( 0, 16,
                     dsp_getwidth(), 1, RED ) ;                    // Paint red part
      dsp_println("ESP32 Webradio");
      dsp_print("initializing...");
      dsp_update() ;                                     // Show on physical screen

    }   
  }
  if ( ini_block.sd_cs_pin >= 0 )                        // SD configured?
  {
    if ( !SD.begin ( ini_block.sd_cs_pin, SPI,           // Yes,
                     SDSPEED ) )                         // try to init SD card driver
    {
      p = dbgprint ( "SD Card Mount Failed!" ) ;         // No success, check formatting (FAT)
      //tftlog ( p ) ;                                     // Show error on TFT as well
    }
    else
    {
      SD_okay = ( SD.cardType() != CARD_NONE ) ;         // See if known card
      if ( !SD_okay )
      {
        p = dbgprint ( "No SD card attached" ) ;         // Card not readable
        // tftlog ( p ) ;                                   // Show error on TFT as well
      }
      else
      {
        dbgprint ( "Locate mp3 files on SD, may take a while..." ) ;
        tftlog ( "Read SD card" ) ;
        SD_nodecount = listsdtracks ( "/", 0, false ) ;  // Build nodelist
        p = dbgprint ( "%d tracks on SD", SD_nodecount ) ;
        tftlog ( p ) ;                                   // Show number of tracks on TFT
      }
    }
  }
//  oledshow(2, "work on networking.. " );
  if(tft){
  oledshow(0,"ESP32 Webradio");
  oledshow(1,"starting");

  }
  //handle_tft_txt() ;
  mk_lsan() ;                                            // Make all list of acceptable networks
                                                         // in preferences.
  WiFi.mode ( WIFI_STA ) ;                               // This ESP is a station
  WiFi.persistent ( false ) ;                            // Do not save SSID and password
  WiFi.disconnect() ;                                    // After restart router could still
  delay ( 1000 ) ;                                        // keep old connection
  listNetworks() ;                                       // Search for WiFi networks
  readprefs ( false ) ;                                  // Read preferences
  tcpip_adapter_set_hostname ( TCPIP_ADAPTER_IF_STA, NAME ) ;
  delay(20);
  delay(10);
  //oledshow(2, "Connect to WiFi" );
      // dsp_println("Connect to WiFi");
      // dsp_update();
  p = dbgprint ( "Connect to WiFi" ) ;                   // Show progress
  //oledshow(3, "Try connecting to Access Point" );
  
  vs1053player->begin() ;                                 // Initialize VS1053 player
  //tftlog ( p ) ;                                         // On TFT too
  NetworkFound = connectwifi() ;                         // Connect to WiFi network
  dbgprint ( "Start server for commands" ) ;

  cmdserver.begin() ;                                    // Start http server
  if ( NetworkFound )                                    // OTA and MQTT only if Wifi network found
  {
    dbgprint ( "Network found. Starting mqbbhtt and OTA" ) ;
  
    oledshow(3, "Start playing" );
    isplaying =true;
    flasing_connection=true;                                  // tell if connected to network
    if(flasing_connection){  dbgprint ( "Flashing started!  ") ;  }
    
    mqtt_on = ( ini_block.mqttbroker.length() > 0 ) &&   // Use MQTT if broker specified
              ( ini_block.mqttbroker != "none" ) ;
    ArduinoOTA.setHostname ( NAME ) ;                    // Set the hostname
    ArduinoOTA.onStart ( otastart ) ;
    ArduinoOTA.begin() ;                                 // Allow update over the air

    if ( mqtt_on )                                       // Broker specified?
    {
      if ( ( ini_block.mqttprefix.length() == 0 ) ||     // No prefix?
           ( ini_block.mqttprefix == "none" ) )
      {
        WiFi.macAddress ( mac ) ;                        // Get mac-adress
        sprintf ( tmpstr, "P%02X%02X%02X%02X",           // Generate string from last part
                  mac[3], mac[2],
                  mac[1], mac[0] ) ;
        ini_block.mqttprefix = String ( tmpstr ) ;       // Save for further use
      }
      dbgprint ( "MQTT uses prefix %s", ini_block.mqttprefix.c_str() ) ;
      dbgprint ( "Init MQTT" ) ;
      mqttclient.setServer(ini_block.mqttbroker.c_str(), // Specify the broker
                           ini_block.mqttport ) ;        // And the port
      mqttclient.setCallback ( onMqttMessage ) ;         // Set callback on receive
    }
    if ( MDNS.begin ( NAME ) )                           // Start MDNS transponder
    {
      dbgprint ( "MDNS responder started" ) ;
    }
    else
    {
      dbgprint ( "Error setting up MDNS responder!" ) ;
    }
  }
  else
  {
    currentpreset = ini_block.newpreset ;                // No network: do not start radio
    oledshow(3, "Network failed!" );
  }

  delay(100);
  timer = timerBegin ( 0, 80, true ) ;                   // User 1st timer with prescaler 80
  timerAttachInterrupt ( timer, &timer100, true ) ;      // Call timer100() on timer alarm
  timerAlarmWrite ( timer, 100000, true ) ;              // Alarm every 100 msec
  timerAlarmEnable ( timer ) ;                           // Enable the timer
  delay ( 1000 ) ;                                       // Show IP for a while
  configTime ( ini_block.clk_offset * 3600,
               ini_block.clk_dst * 3600,
               ini_block.clk_server.c_str() ) ;          // GMT offset, daylight offset in seconds
  timeinfo.tm_year = 0 ;                                 // Set TOD to illegal
  // Init settings for rotary switch (if existing).
  if ( ( ini_block.enc_clk_pin + ini_block.enc_dt_pin + ini_block.enc_sw_pin ) > 2 )
  {
    attachInterrupt ( ini_block.enc_clk_pin, isr_enc_turn,   CHANGE ) ;
    attachInterrupt ( ini_block.enc_dt_pin,  isr_enc_turn,   CHANGE ) ;
    attachInterrupt ( ini_block.enc_sw_pin,  isr_enc_switch, CHANGE ) ;
    dbgprint ( "Rotary encoder is enabled" ) ;
  }
  else
  {
    dbgprint ( "Rotary encoder is disabled (%d/%d/%d)",
               ini_block.enc_clk_pin,
               ini_block.enc_dt_pin,
               ini_block.enc_sw_pin) ;
  }
  if ( NetworkFound )
  {
    gettime() ;                                           // Sync time
  }
  countpreset();
  if ( tft )
  // {
  //   dsp_fillRect ( 0, 8,                                  // Clear most of the screen
  //                  dsp_getwidth(),
  //                  dsp_getheight() - 8, BLACK ) ;
  // }
  //tftset ( 1, "ESP32 Webradio" ) ;
  // tftset ( 2, "starting.....") ;
  outchunk.datatyp = QDATA ;                              // This chunk dedicated to QDATA
  adc1_config_width ( ADC_WIDTH_12Bit ) ;
  adc1_config_channel_atten ( ADC1_CHANNEL_0, ADC_ATTEN_0db ) ;
  dataqueue = xQueueCreate ( QSIZ,                        // Create queue for communication
                             sizeof ( qdata_struct ) ) ;
  xTaskCreatePinnedToCore (
    playtask,                                             // Task to play data in dataqueue.
    "Playtask",                                           // name of task.
    1600,                                                 // Stack size of task
    NULL,                                                 // parameter of the task
    2,                                                    // priority of the task
    &xplaytask,                                           // Task handle to keep track of created task
    0 ) ;                                                 // Run on CPU 0
  xTaskCreate (
    spftask,                                              // Task to handle special functions.
    "Spftask",                                            // name of task.
    2048,                                                 // Stack size of task
    NULL,                                                 // parameter of the task
    1,                                                    // priority of the task
    &xspftask ) ;                                         // Task handle to keep track of created task
}
void countpreset (){
  String tmppresetc;
  for(int i=0; i<101; i++){
           tmppresetc=readhostfrompref(i).c_str();       //get station name 
          tmppresetc+="\n";                              //add next station as newline
          int in=tmppresetc.indexOf("#");                //get comment sign
          if(in>0){

            numOfPreset+=1;
              // tmppresetc=tmppresetc.substring(in);        //get comment only
          }else{break;}
        //  allpreset=allpreset+ tmppreset;                //list all station available in one packet string
      }
      numOfPreset-=1;
      dbgprint ( "totalcount preset %d", numOfPreset ) ;
}

//**************************************************************************************************
//                                        R I N B Y T                                              *
//**************************************************************************************************
// Read next byte from http inputbuffer.  Buffered for speed reasons.                              *
//**************************************************************************************************
uint8_t rinbyt ( bool forcestart )
{
  static uint8_t  buf[1024] ;                           // Inputbuffer
  static uint16_t i ;                                   // Pointer in inputbuffer
  static uint16_t len ;                                 // Number of bytes in buf
  uint16_t        tlen ;                                // Number of available bytes
  uint16_t        trycount = 0 ;                        // Limit max. time to read

  if ( forcestart || ( i == len ) )                     // Time to read new buffer
  {
    while ( cmdclient.connected() )                     // Loop while the client's connected
    {
      tlen = cmdclient.available() ;                    // Number of bytes to read from the client
      len = tlen ;                                      // Try to read whole input
      if ( len == 0 )                                   // Any input available?
      {
        if ( ++trycount > 3 )                           // Not for a long time?
        {
          dbgprint ( "HTTP input shorter than expected" ) ;
          return '\n' ;                                 // Error! No input
        }
        delay ( 10 ) ;                                  // Give communication some time
        continue ;                                      // Next loop of no input yet
      }
      if ( len > sizeof(buf) )                          // Limit number of bytes
      {
        len = sizeof(buf) ;
      }
      len = cmdclient.read ( buf, len ) ;               // Read a number of bytes from the stream
      i = 0 ;                                           // Pointer to begin of buffer
      break ;
    }
  }
  return buf[i++] ;
}


//**************************************************************************************************
//                                        W R I T E P R E F S                                      *
//**************************************************************************************************
// Update the preferences.  Called from the web interface.                                         *
//**************************************************************************************************
void writeprefs()
{
  int        inx ;                                            // Position in inputstr
  uint8_t    winx ;                                           // Index in wifilist
  char       c ;                                              // Input character
  String     inputstr = "" ;                                  // Input regel
  String     key, contents ;                                  // Pair for Preferences entry
  String     dstr ;                                           // Contents for debug

  timerAlarmDisable ( timer ) ;                               // Disable the timer
  nvsclear() ;                                                // Remove all preferences
  while ( true )
  {
    c = rinbyt ( false ) ;                                    // Get next inputcharacter
    if ( c == '\n' )                                          // Newline?
    {
      if ( inputstr.length() == 0 )
      {
        dbgprint ( "End of writing preferences" ) ;
        break ;                                               // End of contents
      }
      if ( !inputstr.startsWith ( "#" ) )                     // Skip pure comment lines
      {
        inx = inputstr.indexOf ( "=" ) ;
        if ( inx >= 0 )                                       // Line with "="?
        {
          key = inputstr.substring ( 0, inx ) ;               // Yes, isolate the key
          key.trim() ;
          contents = inputstr.substring ( inx + 1 ) ;         // and contents
          contents.trim() ;
          dstr = contents ;                                   // Copy for debug
          if ( ( key.indexOf ( "wifi_" ) == 0 ) )             // Sensitive info?
          {
            winx = key.substring(5).toInt() ;                 // Get index in wifilist
            if ( ( winx < wifilist.size() ) &&                // Existing wifi spec in wifilist?
                 ( contents.indexOf ( wifilist[winx].ssid ) == 0 ) &&
                 ( contents.indexOf ( "/****" ) > 0 ) )       // Hidden password?
            {
              contents = String ( wifilist[winx].ssid ) +     // Retrieve ssid and password
                         String ( "/" ) +
                         String ( wifilist[winx].passphrase ) ;
              dstr = String ( wifilist[winx].ssid ) +
                     String ( "/*******" ) ;                  // Hide in debug line
            }
          }
          if ( ( key.indexOf ( "mqttpasswd" ) == 0 ) )        // Sensitive info?
          {
            if ( contents.indexOf ( "****" ) == 0 )           // Hidden password?
            {
              contents = ini_block.mqttpasswd ;               // Retrieve mqtt password
            }
            dstr = String ( "*******" ) ;                     // Hide in debug line
          }
          dbgprint ( "writeprefs setstr %s = %s",
                     key.c_str(), dstr.c_str() ) ;
          nvssetstr ( key.c_str(), contents ) ;               // Save new pair
        }
      }
      inputstr = "" ;
    }
    else
    {
      if ( c != '\r' )                                        // Not newline.  Is is a CR?
      {
        inputstr += String ( c ) ;                            // No, normal char, add to string
      }
    }
  }
  timerAlarmEnable ( timer ) ;                                // Enable the timer
  fillkeylist() ;                                             // Update list with keys
}


//**************************************************************************************************
//                                        H A N D L E H T T P R E P L Y                            *
//**************************************************************************************************
// Handle the output after an http request.                                                        *
//**************************************************************************************************
void handlehttpreply()
{
  const char*   p ;                                         // Pointer to reply if command
  String        sndstr = "" ;                               // String to send
  int           n ;                                         // Number of files on SD card

  if ( http_reponse_flag )
  {
    http_reponse_flag = false ;
    if ( cmdclient.connected() )
    {
      if ( http_rqfile.length() == 0 &&                     // An empty "GET"?
           http_getcmd.length() == 0 )
      {
        if ( NetworkFound )                                 // Yes, check network
        {
          handleFSf ( String( "index.html") ) ;             // Okay, send the startpage
        }
        else
        {
          handleFSf ( String( "config.html") ) ;            // Or the configuration page if in AP mode
        }
      }
      else
      {
        if ( http_getcmd.length() )                         // Command to analyze?
        {
          dbgprint ( "Send reply for %s", http_getcmd.c_str() ) ;
          sndstr = httpheader ( String ( "text/html" ) ) ;  // Set header
          if ( http_getcmd.startsWith ( "getprefs" ) )      // Is it a "Get preferences"?
          {
            if ( datamode != STOPPED )                      // Still playing?
            {
              datamode = STOPREQD ;                         // Stop playing
            }
            sndstr += readprefs ( true ) ;                  // Read and send
          }
          else if ( http_getcmd.startsWith ( "getdefs" ) )  // Is it a "Get default preferences"?
          {
            sndstr += String ( defprefs_txt + 1 ) ;         // Yes, read initial values
          }
          else if ( http_getcmd.startsWith ("saveprefs") )  // Is is a "Save preferences"
          {
            writeprefs() ;                                  // Yes, handle it
          }
          else if ( http_getcmd.startsWith ( "mp3list" ) )  // Is is a "Get SD MP3 tracklist"?
          {
            if ( datamode != STOPPED )                      // Still playing?
            {
              datamode = STOPREQD ;                         // Stop playing
            }
            cmdclient.print ( sndstr ) ;                    // Yes, send header
            n = listsdtracks ( "/" ) ;                      // Handle it
            dbgprint ( "%d tracks found on SD card", n ) ;
            return ;                                        // Do not send empty line
          }
          else if ( http_getcmd.startsWith ( "settings" ) ) // Is is a "Get settings" (like presets and tone)?
          {
            cmdclient.print ( sndstr ) ;                    // Yes, send header
            getsettings() ;                                 // Handle settings request
            return ;                                        // Do not send empty line
          }
          else
          {
            p = analyzeCmd ( http_getcmd.c_str() ) ;        // Yes, do so
            sndstr += String ( p ) ;                        // Content of HTTP response follows the header
          }
          sndstr += String ( "\n" ) ;                       // The HTTP response ends with a blank line
          cmdclient.print ( sndstr ) ;
        }
        else if ( http_rqfile.length() )                    // File requested?
        {
          dbgprint ( "Start file reply for %s",
                     http_rqfile.c_str() ) ;
          handleFSf ( http_rqfile ) ;                       // Yes, send it
        }
        else
        {
          httpheader ( String ( "text/html" ) ) ;           // Send header
          // the content of the HTTP response follows the header:
          cmdclient.println ( "Dummy response\n" ) ;        // Text ending with double newline
          dbgprint ( "Dummy response sent" ) ;
        }
      }
    }
  }
}


//**************************************************************************************************
//                                        H A N D L E H T T P                                      *
//**************************************************************************************************
// Handle the input of an http request.                                                            *
//**************************************************************************************************
void handlehttp()
{
  bool        first = true ;                                 // First call to rinbyt()
  char        c ;                                            // Next character from http input
  int         inx0, inx ;                                    // Pos. of search string in currenLine
  String      currentLine = "" ;                             // Build up to complete line
  bool        reqseen = false ;                              // No GET seen yet

  if ( !cmdclient.connected() )                              // Action if client is connected
  {
    return ;                                                 // No client active
  }
  dbgprint ( "handlehttp started" ) ;
  while ( true )                                             // Loop till command/file seen
  {
    c = rinbyt ( first ) ;                                   // Get a byte
    first = false ;                                          // No more first call
    if ( c == '\n' )
    {
      // If the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if ( currentLine.length() == 0 )
      {
        http_reponse_flag = reqseen ;                        // Response required or not
        break ;
      }
      else
      {
        // Newline seen, remember if it is like "GET /xxx?y=2&b=9 HTTP/1.1"
        if ( currentLine.startsWith ( "GET /" ) )            // GET request?
        {
          inx0 = 5 ;                                         // Start search at pos 5
        }
        else if ( currentLine.startsWith ( "POST /" ) )      // POST request?
        {
          inx0 = 6 ;
        }
        else
        {
          inx0 = 0 ;                                         // Not GET nor POST
        }
        if ( inx0 )                                          // GET or POST request?
        {
          reqseen = true ;                                   // Request seen
          inx = currentLine.indexOf ( "&" ) ;                // Search for 2nd parameter
          if ( inx < 0 )
          {
            inx = currentLine.indexOf ( " HTTP" ) ;          // Search for end of GET command
          }
          // Isolate the command
          http_getcmd = currentLine.substring ( inx0, inx ) ;
          inx = http_getcmd.indexOf ( "?" ) ;                // Search for command
          if ( inx == 0 )                                    // Arguments only?
          {
            http_getcmd = http_getcmd.substring ( 1 ) ;      // Yes, get rid of question mark
            http_rqfile = "" ;                               // No file
          }
          else if ( inx > 0 )                                // Filename present?
          {
            http_rqfile = http_getcmd.substring ( 0, inx ) ; // Remember filename
            http_getcmd = http_getcmd.substring ( inx + 1 ) ; // Remove filename from GET command

          }
          else
          {
            http_rqfile = http_getcmd ;                      // No parameters, set filename
            http_getcmd = "" ;
          }
          if ( http_getcmd.length() )
          {
            dbgprint ( "Get command is: %s",                 // Show result
                       http_getcmd.c_str() ) ;
                       // tftset ( 0, http_getcmd.c_str() ) ;
          }
          if ( http_rqfile.length() )
          {
            dbgprint ( "Filename is: %s",                    // Show requested file
                       http_rqfile.c_str() ) ;
          }
        }
        currentLine = "" ;
      }
    }
    else if ( c != '\r' )                                    // No LINFEED.  Is it a CR?
    {
      currentLine += c ;                                     // No, add normal char to currentLine
    }
  }
  //cmdclient.stop() ;
}


//**************************************************************************************************
//                                          X M L P A R S E                                        *
//**************************************************************************************************
// Parses line with XML data and put result in variable specified by parameter.                    *
//**************************************************************************************************
void xmlparse ( String &line, const char *selstr, String &res )
{
  String sel = "</" ;                                  // Will be like "</status-code"
  int    inx ;                                         // Position of "</..." in line

  sel += selstr ;                                      // Form searchstring
  if ( line.endsWith ( sel ) )                         // Is this the line we are looking for?
  {
    inx = line.indexOf ( sel ) ;                       // Get position of end tag
    res = line.substring ( 0, inx ) ;                  // Set result
  }
}


//**************************************************************************************************
//                                          X M L G E T H O S T                                    *
//**************************************************************************************************
// Parses streams from XML data.                                                                   *
// Example URL for XML Data Stream:                                                                *
// http://playerservices.streamtheworld.com/api/livestream?version=1.5&mount=IHR_TRANAAC&lang=en   *
//**************************************************************************************************
String xmlgethost  ( String mount )
{
  const char* xmlhost = "playerservices.streamtheworld.com" ;  // XML data source
  const char* xmlget =  "GET /api/livestream"                  // XML get parameters
                        "?version=1.5"                         // API Version of IHeartRadio
                        "&mount=%sAAC"                         // MountPoint with Station Callsign
                        "&lang=en" ;                           // Language

  String   stationServer = "" ;                     // Radio stream server
  String   stationPort = "" ;                       // Radio stream port
  String   stationMount = "" ;                      // Radio stream Callsign
  uint16_t timeout = 0 ;                            // To detect time-out
  String   sreply = "" ;                            // Reply from playerservices.streamtheworld.com
  String   statuscode = "200" ;                     // Assume good reply
  char     tmpstr[200] ;                            // Full GET command, later stream URL
  String   urlout ;                                 // Result URL

  stop_mp3client() ; // Stop any current wificlient connections.
  dbgprint ( "Connect to new iHeartRadio host: %s", mount.c_str() ) ;
  datamode = INIT ;                                   // Start default in metamode
  chunked = false ;                                   // Assume not chunked
  sprintf ( tmpstr, xmlget, mount.c_str() ) ;         // Create a GET commmand for the request
  dbgprint ( "%s", tmpstr ) ;
  if ( mp3client.connect ( xmlhost, 80 ) )            // Connect to XML stream
  {
    dbgprint ( "Connected to %s", xmlhost ) ;
    mp3client.print ( String ( tmpstr ) + " HTTP/1.1\r\n"
                      "Host: " + xmlhost + "\r\n"
                      "User-Agent: Mozilla/5.0\r\n"
                      "Connection: close\r\n\r\n" ) ;
    while ( mp3client.available() == 0 )
    {
      delay ( 200 ) ;                                 // Give server some time
      if ( ++timeout > 25 )                           // No answer in 5 seconds?
      {
        dbgprint ( "Client Timeout !" ) ;
      }
    }
    dbgprint ( "XML parser processing..." ) ;
    while ( mp3client.available() )
    {
      sreply = mp3client.readStringUntil ( '>' ) ;
      sreply.trim() ;
      // Search for relevant info in in reply and store in variable
      xmlparse ( sreply, "status-code", statuscode ) ;
      xmlparse ( sreply, "ip",          stationServer ) ;
      xmlparse ( sreply, "port",        stationPort ) ;
      xmlparse ( sreply, "mount",       stationMount ) ;
      if ( statuscode != "200" )                      // Good result sofar?
      {
        dbgprint ( "Bad xml status-code %s",         // No, show and stop interpreting
                   statuscode.c_str() ) ;
        tmpstr[0] = '\0' ;                           // Clear result
        break ;
      }
    }
    if ( ( stationServer != "" ) &&                   // Check if all station values are stored
         ( stationPort != "" ) &&
         ( stationMount != "" ) )
    {
      sprintf ( tmpstr, "%s:%s/%s_SC",                // Build URL for ESP-Radio to stream.
                stationServer.c_str(),
                stationPort.c_str(),
                stationMount.c_str() ) ;
      dbgprint ( "Found: %s", tmpstr ) ;
    }
  }
  else
  {
    dbgprint ( "Can't connect to XML host!" ) ;       // Connection failed
    tmpstr[0] = '\0' ;
  }
  mp3client.stop() ;
  return String ( tmpstr ) ;                          // Return final streaming URL.
}


//**************************************************************************************************
//                                      H A N D L E S A V E R E Q                                  *
//**************************************************************************************************
// Handle save volume/preset/tone.  This will save current settings every 10 minutes to            *
// the preferences.  On the next restart these values will be loaded.                              *
// Note that saving prefences will only take place if contents has changed.                        *
//**************************************************************************************************
void handleSaveReq()
{
  static uint32_t savetime = 0 ;                          // Limit save to once per 10 minutes

  if ( ( millis() - savetime ) < 600000 )                 // 600 sec is 10 minutes
  {
    return ;
  }
  savetime = millis() ;                                   // Set time of last save
  nvssetstr ( "preset", String ( currentpreset )  ) ;     // Save current preset
  nvssetstr ( "volume", String ( ini_block.reqvol ) );    // Save current volue
  nvssetstr ( "toneha", String ( ini_block.rtone[0] ) ) ; // Save current toneha
  nvssetstr ( "tonehf", String ( ini_block.rtone[1] ) ) ; // Save current tonehf
  nvssetstr ( "tonela", String ( ini_block.rtone[2] ) ) ; // Save current tonela
  nvssetstr ( "tonelf", String ( ini_block.rtone[3] ) ) ; // Save current tonelf
}
//function for forcesavepref
void forcesavepref(){                    
    
  nvssetstr ( "preset", String ( currentpreset )  ) ;     // Save current preset
  nvssetstr ( "volume", String ( ini_block.reqvol ) );    // Save current volue
  nvssetstr ( "toneha", String ( ini_block.rtone[0] ) ) ; // Save current toneha
  nvssetstr ( "tonehf", String ( ini_block.rtone[1] ) ) ; // Save current tonehf
  nvssetstr ( "tonela", String ( ini_block.rtone[2] ) ) ; // Save current tonela
  nvssetstr ( "tonelf", String ( ini_block.rtone[3] ) ) ; // Save current tonelf
}   



//**************************************************************************************************
//                                      H A N D L E I P P U B                                      *
//**************************************************************************************************
// Handle publish op IP to MQTT.  This will happen every 10 minutes.                               *
//**************************************************************************************************
void handleIpPub()
{
  static uint32_t pubtime = 300000 ;                       // Limit save to once per 10 minutes

  if ( ( millis() - pubtime ) < 600000 )                   // 600 sec is 10 minutes
  {
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  mqttpub.trigger ( MQTT_IP ) ;                            // Request re-publish IP
}


//**************************************************************************************************
//                                      H A N D L E V O L P U B                                    *
//**************************************************************************************************
// Handle publish of Volume to MQTT.  This will happen max every 10 seconds.                       *
//**************************************************************************************************
void handleVolPub()
{
  static uint32_t pubtime = 10000 ;                        // Limit save to once per 10 seconds
  static uint8_t  oldvol = -1 ;                            // For comparison

  if ( ( millis() - pubtime ) < 10000 )                    // 10 seconds
  {
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  if ( ini_block.reqvol != oldvol )                        // Volume change?
  {
    mqttpub.trigger ( MQTT_VOLUME ) ;                      // Request publish VOLUME
    oldvol = ini_block.reqvol ;                            // Remember publishe volume
  }
}



//**************************************************************************************************
//                                           C H K _ E N C                                         *
//**************************************************************************************************
// See if rotary encoder is activated and perform its functions.                                   *
//**************************************************************************************************
void chk_enc()
{
  static int8_t  enc_preset ;                                 // Selected preset
  static String  enc_nodeID ;                                 // Node of selected track
  static String  enc_filename ;                               // Filename of selected track
  String         tmp ;                                        // Temporary string
  int16_t        inx ;                                        // Position in string

  if ( enc_menu_mode != VOLUME )                              // In default mode?
  {
    if ( enc_inactivity > 40 )                                // No, more than 4 seconds inactive
    {
      enc_inactivity = 0 ;
      enc_menu_mode = VOLUME ;                                // Return to VOLUME mode
      dbgprint ( "Encoder mode back to VOLUME" ) ;
      tftset ( 2, (char*)NULL ) ;                             // Restore original text at bottom
    }
  }
  if ( tripleclick )                                          // First handle triple click
  {
    dbgprint ( "Triple click") ;
    tripleclick = false ;
    if ( SD_nodecount )                                       // Tracks on SD?
    {
      enc_menu_mode = TRACK ;                                 // Swich to TRACK mode
      dbgprint ( "Encoder mode set to TRACK" ) ;
      tftset ( 3, "Turn to select track\n"                    // Show current option
               "Press to confirm" ) ;
      enc_nodeID = selectnextSDnode ( SD_currentnode, +1 ) ;  // Start with next file on SD
      if ( enc_nodeID == "" )                                 // Current track available?
      {
        inx = SD_nodelist.indexOf ( "\n" ) ;                  // No, find first
        enc_nodeID = SD_nodelist.substring ( 0, inx ) ;
      }
      // Stop playing as reading filenames saturates SD I/O.
      if ( datamode != STOPPED )
      {
        datamode = STOPREQD ;                                 // Request STOP
      }
    }
  }
  if ( doubleclick )                                          // Handle the doubleclick
  {
    dbgprint ( "Double click") ;
    doubleclick = false ;
    enc_menu_mode = PRESET ;                                  // Swich to PRESET mode
    dbgprint ( "Encoder mode set to PRESET" ) ;
    tftset ( 3, "Turn to select station\n"                    // Show current option
             "Press to confirm" ) ;
    enc_preset = ini_block.newpreset + 1 ;                    // Start with current preset + 1
  }
  if ( singleclick )
  {
    dbgprint ( "Single click") ;
    singleclick = false ;
    switch ( enc_menu_mode )                                  // Which mode (VOLUME, PRESET, TRACK)?
    {
      case VOLUME :
        if ( muteflag )
        {
          tftset ( 3, "" ) ;                                  // Clear text
        }
        else
        {
          tftset ( 3, "Mute" ) ;
        }
        muteflag = !muteflag ;                                // Mute/unmute
        break ;
      case PRESET :
        currentpreset = -1 ;                                  // Make sure current is different
        ini_block.newpreset = enc_preset ;                    // Make a definite choice
        enc_menu_mode = VOLUME ;                              // Back to default mode
        tftset ( 3, "" ) ;                                    // Clear text
        break ;
      case TRACK :
        host = enc_filename ;                                 // Selected track as new host
        hostreq = true ;                                      // Request this host
        enc_menu_mode = VOLUME ;                              // Back to default mode
        tftset ( 3, "" ) ;                                    // Clear text
        break ;
    }
  }
  if ( longclick )                                            // Check for long click
  {
    dbgprint ( "Long click") ;
    if ( datamode != STOPPED )
    {
      datamode = STOPREQD ;                                   // Request STOP, do not touch logclick flag
    }
    else
    {
      longclick = false ;                                     // Reset condition
      dbgprint ( "Long click detected" ) ;
      if ( SD_nodecount )                                     // Tracks on SD?
      {
        host = getSDfilename ( "0" ) ;                        // Get random track
        hostreq = true ;                                      // Request this host
      }
      muteflag = false ;                                      // Be sure muteing is off
    }
  }
  if ( rotationcount == 0 )                                   // Any rotation?
  {
    return ;                                                  // No, return
  }
  dbgprint ( "Rotation count %d", rotationcount ) ;
  switch ( enc_menu_mode )                                    // Which mode (VOLUME, PRESET, TRACK)?
  {
    case VOLUME :
      if ( ( ini_block.reqvol + rotationcount ) < 0 )         // Limit volume
      {
        ini_block.reqvol = 0 ;                                // Limit to normal values
      }
      else if ( ( ini_block.reqvol + rotationcount ) > 100 )
      {
        ini_block.reqvol = 100 ;                              // Limit to normal values
      }
      else
      {
        ini_block.reqvol += rotationcount ;
      }
      muteflag = false ;                                      // Mute off
      break ;
    case PRESET :
      if ( ( enc_preset + rotationcount ) < 0 )               // Negative not allowed
      {
        enc_preset = 0 ;                                      // Stay at 0
      }
      else
      {
        enc_preset += rotationcount ;                         // Next preset
      }
      tmp = readhostfrompref ( enc_preset ) ;                 // Get host spec and possible comment
      if ( tmp == "" )                                        // End of presets?
      {
        enc_preset = 0 ;                                      // Yes, wrap
        tmp = readhostfrompref ( enc_preset ) ;               // Get host spec and possible comment
      }
      dbgprint ( "Preset is %d", enc_preset ) ;
      // Show just comment if available.  Otherwise the preset itself.
      inx = tmp.indexOf ( "#" ) ;                             // Get position of "#"
      if ( inx > 0 )                                          // Hash sign present?
      {
        tmp.remove ( 0, inx + 1 ) ;                           // Yes, remove non-comment part
      }
      chomp ( tmp ) ;                                         // Remove garbage from description
      tftset ( 3, tmp ) ;                                     // Set screen segment bottom part
      break ;
    case TRACK :
      enc_nodeID = selectnextSDnode ( enc_nodeID,
                                      rotationcount ) ;       // Select the next file on SD
      enc_filename = getSDfilename ( enc_nodeID ) ;           // Set new filename
      tmp = enc_filename ;                                    // Copy for display
      dbgprint ( "Select %s", tmp.c_str() ) ;
      while ( ( inx = tmp.indexOf ( "/" ) ) >= 0 )            // Search for last slash
      {
        tmp.remove ( 0, inx + 1 ) ;                           // Remove before the slash
      }
      dbgprint ( "Simplified %s", tmp.c_str() ) ;
      tftset ( 3, tmp ) ;         
      // Set screen segment bottom part
    default :
      break ;
  }
  rotationcount = 0 ;                                         // Reset
}


//**************************************************************************************************
//                                           M P 3 L O O P                                         *
//**************************************************************************************************
// Called from the mail loop() for the mp3 functions.                                              *
// A connection to an MP3 server is active and we are ready to receive data.                       *
// Normally there is about 2 to 4 kB available in the data stream.  This depends on the sender.    *
//**************************************************************************************************
void mp3loop()
{
  static uint8_t  tmpbuff[6000] ;                        // Input buffer for mp3 stream
  uint32_t        maxchunk ;                             // Max number of bytes to read
  int             res = 0 ;                              // Result reading from mp3 stream
  uint32_t        av = 0 ;                               // Available in stream
  String          nodeID ;                               // Next nodeID of track on SD

  // Try to keep the Queue to playtask filled up by adding as much bytes as possible
  if ( datamode & ( INIT | HEADER | DATA |               // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    maxchunk = sizeof(tmpbuff) ;                         // Reduce byte count for this mp3loop()
    if ( localfile )                                     // Playing file from SD card?
    {
      av = mp3filelength ;                               // Bytes left in file
      if ( av < maxchunk )                               // Reduce byte count for this mp3loop()
      {
        maxchunk = av ;
      }
      if ( maxchunk )                                    // Anything to read?
      {
        claimSPI ( "sdread" ) ;                          // Claim SPI bus
        res = mp3file.read ( tmpbuff, maxchunk ) ;       // Read a block of data
        releaseSPI() ;                                   // Release SPI bus
        mp3filelength -= res ;                           // Number of bytes left
      }
    }
    else
    {
      av = mp3client.available() ;                       // Available from stream
      if ( av < maxchunk )                               // Limit read size
      {
        maxchunk = av ;
      }
      if ( maxchunk )                                    // Anything to read?
      {
        res = mp3client.read ( tmpbuff, maxchunk ) ;     // Read a number of bytes from the stream
      }
      else
      {
        if ( datamode == PLAYLISTDATA )                  // End of playlist
        {
          playlist_num = 0 ;                             // And reset
          dbgprint ( "End of playlist seen" ) ;
          datamode = STOPPED ;
          ini_block.newpreset++ ;                        // Go to next preset
        }
      }
    }
    for ( int i = 0 ; i < res ; i++ )
    {
      handlebyte_ch ( tmpbuff[i] ) ;                     // Handle one byte
    }
  }
  if ( datamode == STOPREQD )                            // STOP requested?
  {
    dbgprint ( "STOP requested" ) ;
    if ( localfile )
    {
      claimSPI ( "close" ) ;                             // Claim SPI bus
      mp3file.close() ;
      releaseSPI() ;                                     // Release SPI bus
    }
    else
    {
      stop_mp3client() ;                                 // Disconnect if still connected
    }
    chunked = false ;                                    // Not longer chunked
    datacount = 0 ;                                      // Reset datacount
    outqp = outchunk.buf ;                               // and pointer
    queuefunc ( QSTOPSONG ) ;                            // Queue a request to stop the song
    metaint = 0 ;                                        // No metaint known now
    datamode = STOPPED ;                                 // Yes, state becomes STOPPED
    return ;
  }
  if ( localfile )                                       // Playing from SD?
  {
    if ( datamode & DATA )                               // Test op playing
    {
      if ( av == 0 )                                     // End of mp3 data?
      {
        datamode = STOPREQD ;                            // End of local mp3-file detected
        nodeID = selectnextSDnode ( SD_currentnode, +1 ) ; // Select the next file on SD
        host = getSDfilename ( nodeID ) ;
        hostreq = true ;                                 // Request this host
      }
    }
  }
  if ( ini_block.newpreset != currentpreset )            // New station or next from playlist requested?
  {
    if ( datamode != STOPPED )                           // Yes, still busy?
    {
      datamode = STOPREQD ;                              // Yes, request STOP
    }
    else
    {
      if ( playlist_num )                                 // Playing from playlist?
      { // Yes, retrieve URL of playlist
        playlist_num += ini_block.newpreset -
                        currentpreset ;                   // Next entry in playlist
        ini_block.newpreset = currentpreset ;             // Stay at current preset
      }
      else
      {
        host = readhostfrompref() ;                       // Lookup preset in preferences
        chomp ( host ) ;                                  // Get rid of part after "#"
      }
      dbgprint ( "New preset/file requested (%d/%d) from %s",
                 ini_block.newpreset, playlist_num, host.c_str() ) ;
      if ( host != ""  )                                  // Preset in ini-file?
      {
        hostreq = true ;                                  // Force this station as new preset
      }
      else
      {
        // This preset is not available, return to preset 0, will be handled in next mp3loop()
        dbgprint ( "No host for this preset" ) ;
        ini_block.newpreset = 0 ;                         // Wrap to first station
      }
    }
  }
  if ( hostreq )                                          // New preset or station?
  {
    hostreq = false ;
    currentpreset = ini_block.newpreset ;                 // Remember current preset
    mqttpub.trigger ( MQTT_PRESET ) ;                     // Request publishing to MQTT
    // Find out if this URL is on localhost (SD).
    localfile = ( host.indexOf ( "localhost/" ) >= 0 ) ;
    if ( localfile )                                      // Play file from localhost?
    {
      if ( connecttofile() )                              // Yes, open mp3-file
      {
        datamode = DATA ;                                 // Start in DATA mode
      }
    }
    else
    {
      if ( host.startsWith ( "ihr/" ) )                   // iHeartRadio station requested?
      {
        host = host.substring ( 4 ) ;                     // Yes, remove "ihr/"
        host = xmlgethost ( host ) ;                      // Parse the xml to get the host
      }
      connecttohost() ;                                   // Switch to new host
    }
  }
}
int prevmil;


//**************************************************************************************************
//                                           L O O P                                               *
//**************************************************************************************************
// Main loop of the program.                                                                       *
//**************************************************************************************************
void loop()
{
  mp3loop() ;                                               // Do mp3 related actions
  if ( resetreq )                                           // Reset requested?
  {
    delay ( 1000 ) ;                                        // Yes, wait some time
    ESP.restart() ;                                         // Reboot
  }
  ledflash();
  // scanserial() ;                                            // Handle serial input
  // scandigital() ;                                           // Scan digital inputs
  //scanIR() ;                                                // See if IR input
  // ArduinoOTA.handle() ;                                     // Check for OTA
  mp3loop() ;                                               // Do more mp3 related actions
  handlehttpreply() ;
  cmdclient = cmdserver.available() ;                       // Check Input from client?
  if ( cmdclient )                                          // Client connected?
  {
    dbgprint ( "Command client available" ) ;
    handlehttp() ;
  }
  // Handle MQTT.
  if ( mqtt_on )
  {
    mqttclient.loop() ;                                     // Handling of MQTT connection
  }
  handleSaveReq() ;                                         // See if time to save settings
  handleIpPub() ;                                           // See if time to publish IP
  handleVolPub() ;                                          // See if time to publish volume
  //chk_enc() ;                                               // Check rotary encoder functions

  // unsigned long curmil=0;
  
}

// flasing for connected indicator
void ledflash (){
  if (flasing_connection){
    if (millis()-prevmil>5000){
        digitalWrite(led_connnected_in, HIGH);
    }
    if (millis()-prevmil>5100){
      digitalWrite(led_connnected_in, LOW);
      if (  wifiMulti.run() != WL_CONNECTED ) {
        ESP.restart();
      }
      prevmil=millis();
    }
  }
}

//**************************************************************************************************
//                                    C H K H D R L I N E                                          *
//**************************************************************************************************
// Check if a line in the header is a reasonable headerline.                                       *
// Normally it should contain something like "icy-xxxx:abcdef".                                    *
//**************************************************************************************************
bool chkhdrline ( const char* str )
{
  char    b ;                                         // Byte examined
  int     len = 0 ;                                   // Lengte van de string

  while ( ( b = *str++ ) )                            // Search to end of string
  {
    len++ ;                                           // Update string length
    if ( ! isalpha ( b ) )                            // Alpha (a-z, A-Z)
    {
      if ( b != '-' )                                 // Minus sign is allowed
      {
        if ( b == ':' )                               // Found a colon?
        {
          return ( ( len > 5 ) && ( len < 50 ) ) ;    // Yes, okay if length is okay
        }
        else
        {
          return false ;                              // Not a legal character
        }
      }
    }
  }
  return false ;                                      // End of string without colon
}


//**************************************************************************************************
//                            S C A N _ C O N T E N T _ L E N G T H                                *
//**************************************************************************************************
// If the line contains content-length information: set clength (content length counter).          *
//**************************************************************************************************
void scan_content_length ( const char* metalinebf )
{
  if ( strstr ( metalinebf, "Content-Length" ) )        // Line contains content length
  {
    clength = atoi ( metalinebf + 15 ) ;                // Yes, set clength
    dbgprint ( "Content-Length is %d", clength ) ;      // Show for debugging purposes
  }
}

String bitrateinfo="";
String radioinfo2="";
String oldradioinfo2="";
//**************************************************************************************************
//                                   H A N D L E B Y T E _ C H                                     *
//**************************************************************************************************
// Handle the next byte of data from server.                                                       *
// Chunked transfer encoding aware. Chunk extensions are not supported.                            *
//**************************************************************************************************
void handlebyte_ch ( uint8_t b )
{
  static int       chunksize = 0 ;                      // Chunkcount read from stream
  static uint16_t  playlistcnt ;                        // Counter to find right entry in playlist
  static int       LFcount ;                            // Detection of end of header
  static bool      ctseen = false ;                     // First line of header seen or not

  if ( chunked &&
       ( datamode & ( DATA |                           // Test op DATA handling
                      METADATA |
                      PLAYLISTDATA ) ) )
  {
    if ( chunkcount == 0 )                             // Expecting a new chunkcount?
    {
      if ( b == '\r' )                                 // Skip CR
      {
        return ;
      }
      else if ( b == '\n' )                            // LF ?
      {
        chunkcount = chunksize ;                       // Yes, set new count
        chunksize = 0 ;                                // For next decode
        return ;
      }
      // We have received a hexadecimal character.  Decode it and add to the result.
      b = toupper ( b ) - '0' ;                        // Be sure we have uppercase
      if ( b > 9 )
      {
        b = b - 7 ;                                    // Translate A..F to 10..15
      }
      chunksize = ( chunksize << 4 ) + b ;
      return  ;
    }
    chunkcount-- ;                                     // Update count to next chunksize block
  }
  if ( datamode == DATA )                              // Handle next byte of MP3/Ogg data
  {
    *outqp++ = b ;
    if ( outqp == ( outchunk.buf + sizeof(outchunk.buf) ) ) // Buffer full?
    {
      // Send data to playtask queue.  If the buffer cannot be placed within 200 ticks,
      // the queue is full, while the sender tries to send more.  The chunk will be dis-
      // carded it that case.
      xQueueSend ( dataqueue, &outchunk, 200 ) ;       // Send to queue
      outqp = outchunk.buf ;                           // Item empty now
    }
    if ( metaint )                                     // No METADATA on Ogg streams or mp3 files
    {
      if ( --datacount == 0 )                          // End of datablock?
      {
        datamode = METADATA ;
        metalinebfx = -1 ;                             // Expecting first metabyte (counter)
      }
    }
    return ;
  }
  if ( datamode == INIT )                              // Initialize for header receive
  {
    ctseen = false ;                                   // Contents type not seen yet
    metaint = 0 ;                                      // No metaint found
    LFcount = 0 ;                                      // For detection end of header
    bitrate = 0 ;                                      // Bitrate still unknown
    dbgprint ( "Switch to HEADER" ) ;
    datamode = HEADER ;                                // Handle header
    totalcount = 0 ;                                   // Reset totalcount
    metalinebfx = 0 ;                                  // No metadata yet
    metalinebf[0] = '\0' ;
  }
  if ( datamode == HEADER )                            // Handle next byte of MP3 header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      metalinebf[metalinebfx] = '\0' ;                 // Take care of delimiter
      if ( chkhdrline ( metalinebf ) )                 // Reasonable input?
      {
        dbgprint ( "Headerline: %s",                   // Show headerline
                   metalinebf ) ;
        String metaline = String ( metalinebf ) ;      // Convert to string
        String lcml = metaline ;                       // Use lower case for compare
        lcml.toLowerCase() ;
        if ( lcml.startsWith ( "location: http://" ) ) // Redirection?
        {
          host = metaline.substring ( 17 ) ;           // Yes, get new URL
          hostreq = true ;                             // And request this one
        }
        if ( lcml.indexOf ( "content-type" ) >= 0)     // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true ;                              // Yes, remember seeing this
          String ct = metaline.substring ( 13 ) ;      // Set contentstype. Not used yet
          ct.trim() ;
          dbgprint ( "%s seen.", ct.c_str() ) ;
          radioinfo2+="type : " +metaline.substring ( 19 )+"\n";
        }
        if ( lcml.startsWith ( "icy-genre:" ))     // Line with "icy-genre: xxxx/yyy"
        {
         radioinfo2+="genre : " +metaline.substring(10)+"\n";
        }
        
        if ( lcml.startsWith ( "icy-br:" ) )
        {
          bitrate = metaline.substring(7).toInt() ;    // Found bitrate tag, read the bitrate
          radioinfo2+="bitrate : "+metaline.substring(7)+ " kbps"+"\n";
          if ( bitrate == 0 )                          // For Ogg br is like "Quality 2"
          {
            bitrate = 87 ;                             // Dummy bitrate
          }
        }
        else if ( lcml.startsWith ("icy-metaint:" ) )
        {
          metaint = metaline.substring(12).toInt() ;   // Found metaint tag, read the value
        }
        else if ( lcml.startsWith ( "icy-name:" ) )
        {
          icyname = metaline.substring(9) ;            // Get station name
         
          radioinfo2="";
          icyname.trim() ;                             // Remove leading and trailing spaces
         //tftset ( 1, icyname ) ;                      // Set screen segment bottom part
         stasiun=icyname;
          mqttpub.trigger ( MQTT_ICYNAME ) ;           // Request publishing to MQTT
        }
        else if ( lcml.startsWith ( "transfer-encoding:" ) )
        {
          // Station provides chunked transfer
          if ( lcml.endsWith ( "chunked" ) )
          {
            chunked = true ;                           // Remember chunked transfer mode
            chunkcount = 0 ;                           // Expect chunkcount in DATA
          }
        }
      }
      metalinebfx = 0 ;                                // Reset this line
      if ( ( LFcount == 2 ) && ctseen )                // Content type seen and a double LF?
      {
        dbgprint ( "Switch to DATA, bitrate is %d"     // Show bitrate
                   ", metaint is %d",                  // and metaint
                   bitrate, metaint ) ;
        datamode = DATA ;                              // Expecting data now
        datacount = metaint ;                          // Number of bytes before first metadata
        queuefunc ( QSTARTSONG ) ;                     // Queue a request to start song
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;            // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
    // oldradioinfo2=radioinfo2;
    isplaying=true;
    return ;
  }
  if ( datamode == METADATA )                          // Handle next byte of metadata
  {
    if ( metalinebfx < 0 )                             // First byte of metadata?
    {
      metalinebfx = 0 ;                                // Prepare to store first character
      metacount = b * 16 + 1 ;                         // New count for metadata including length byte
      if ( metacount > 1 )
      {
        dbgprint ( "Metadata block %d bytes",
                   metacount - 1 ) ;                   // Most of the time there are zero bytes of metadata
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;            // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
    }
    if ( --metacount == 0 )
    {
      metalinebf[metalinebfx] = '\0' ;                 // Make sure line is limited
      if ( strlen ( metalinebf ) )                     // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showstreamtitle ( metalinebf ) ;               // Show artist and title if present in metadata
        mqttpub.trigger ( MQTT_STREAMTITLE ) ;         // Request publishing to MQTT
      }
      if ( metalinebfx  > ( METASIZ - 10 ) )           // Unlikely metaline length?
      {
        dbgprint ( "Metadata block too long! Skipping all Metadata from now on." ) ;
        metaint = 0 ;                                  // Probably no metadata
      }
      datacount = metaint ;                            // Reset data count
      //bufcnt = 0 ;                                     // Reset buffer count
      datamode = DATA ;                                // Expecting data
    }
  }
  if ( datamode == PLAYLISTINIT )                      // Initialize for receive .m3u file
  {
    // We are going to use metadata to read the lines from the .m3u file
    // Sometimes this will only contain a single line
    metalinebfx = 0 ;                                  // Prepare for new line
    LFcount = 0 ;                                      // For detection end of header
    datamode = PLAYLISTHEADER ;                        // Handle playlist data
    playlistcnt = 1 ;                                  // Reset for compare
    totalcount = 0 ;                                   // Reset totalcount
    clength = 0xFFFF ;                                 // Content-length unknown
    dbgprint ( "Read from playlist" ) ;
  }
  if ( datamode == PLAYLISTHEADER )                    // Read header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      return ;                                         // Quick return
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      metalinebf[metalinebfx] = '\0' ;                 // Take care of delimeter
      dbgprint ( "Playlistheader: %s",                 // Show playlistheader
                 metalinebf ) ;
      scan_content_length ( metalinebf ) ;             // Check if it is a content-length line
      metalinebfx = 0 ;                                // Ready for next line
      if ( LFcount == 2 )
      {
        dbgprint ( "Switch to PLAYLISTDATA, "          // For debug
                   "search for entry %d",
                   playlist_num ) ;
        datamode = PLAYLISTDATA ;                      // Expecting data now
        return ;
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;            // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
  }
  if ( datamode == PLAYLISTDATA )                      // Read next byte of .m3u file data
  {
    clength-- ;                                        // Decrease content length by 1
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    if ( b != '\n' )                                   // Linefeed?
    {                                                  // No, normal character in playlistdata,
      metalinebf[metalinebfx++] = (char)b ;            // add it to metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
    }
    if ( ( b == '\n' ) ||                              // linefeed ?
         ( clength == 0 ) )                            // Or end of playlist data contents
    {
      int inx ;                                        // Pointer in metaline
      metalinebf[metalinebfx] = '\0' ;                 // Take care of delimeter
      dbgprint ( "Playlistdata: %s",                   // Show playlistheader
                 metalinebf ) ;
      if ( strlen ( metalinebf ) < 5 )                 // Skip short lines
      {
        metalinebfx = 0 ;                              // Flush line
        metalinebf[0] = '\0' ;
        return ;
      }
      String metaline = String ( metalinebf ) ;        // Convert to string
      if ( metaline.indexOf ( "#EXTINF:" ) >= 0 )      // Info?
      {
        if ( playlist_num == playlistcnt )             // Info for this entry?
        {
          inx = metaline.indexOf ( "," ) ;             // Comma in this line?
          if ( inx > 0 )
          {
            // Show artist and title if present in metadata
            showstreamtitle ( metaline.substring ( inx + 1 ).c_str(), true ) ;
            mqttpub.trigger ( MQTT_STREAMTITLE ) ;     // Request publishing to MQTT
          }
        }
      }
      if ( metaline.startsWith ( "#" ) )               // Commentline?
      {
        metalinebfx = 0 ;                              // Yes, ignore
        return ;                                       // Ignore commentlines
      }
      // Now we have an URL for a .mp3 file or stream.  Is it the rigth one?
      dbgprint ( "Entry %d in playlist found: %s", playlistcnt, metalinebf ) ;
      if ( playlist_num == playlistcnt  )
      {
        inx = metaline.indexOf ( "http://" ) ;         // Search for "http://"
        if ( inx >= 0 )                                // Does URL contain "http://"?
        {
          host = metaline.substring ( inx + 7 ) ;      // Yes, remove it and set host
        }
        else
        {
          host = metaline ;                            // Yes, set new host
        }
        connecttohost() ;                              // Connect to it
      }
      metalinebfx = 0 ;                                // Prepare for next line
      host = playlist ;                                // Back to the .m3u host
      playlistcnt++ ;                                  // Next entry in playlist
    }
  }
}


//**************************************************************************************************
//                                     G E T C O N T E N T T Y P E                                 *
//**************************************************************************************************
// Returns the contenttype of a file to send.                                                      *
//**************************************************************************************************
String getContentType ( String filename )
{
  if      ( filename.endsWith ( ".html" ) ) return "text/html" ;
  else if ( filename.endsWith ( ".png"  ) ) return "image/png" ;
  else if ( filename.endsWith ( ".gif"  ) ) return "image/gif" ;
  else if ( filename.endsWith ( ".jpg"  ) ) return "image/jpeg" ;
  else if ( filename.endsWith ( ".ico"  ) ) return "image/x-icon" ;
  else if ( filename.endsWith ( ".css"  ) ) return "text/css" ;
  else if ( filename.endsWith ( ".zip"  ) ) return "application/x-zip" ;
  else if ( filename.endsWith ( ".gz"   ) ) return "application/x-gzip" ;
  else if ( filename.endsWith ( ".mp3"  ) ) return "audio/mpeg" ;
  else if ( filename.endsWith ( ".pw"   ) ) return "" ;              // Passwords are secret
  return "text/plain" ;
}


//**************************************************************************************************
//                                        H A N D L E F S F                                        *
//**************************************************************************************************
// Handling of requesting pages from the PROGMEM. Example: favicon.ico                             *
//**************************************************************************************************
void handleFSf ( const String& pagename )
{
  String                 ct ;                           // Content type
  const char*            p ;
  int                    l ;                            // Size of requested page
  int                    TCPCHUNKSIZE = 1024 ;          // Max number of bytes per write

  dbgprint ( "FileRequest received %s", pagename.c_str() ) ;
  ct = getContentType ( pagename ) ;                    // Get content type
  if ( ( ct == "" ) || ( pagename == "" ) )             // Empty is illegal
  {
    cmdclient.println ( "HTTP/1.1 404 Not Found" ) ;
    cmdclient.println ( "" ) ;
    return ;
  }
  else
  {
    if ( pagename.indexOf ( "index.html" ) >= 0 )       // Index page is in PROGMEM
    {
      p = index_html ;
      l = sizeof ( index_html ) ;
    }
    else if ( pagename.indexOf ( "radio.css" ) >= 0 )   // CSS file is in PROGMEM
    {
      p = radio_css + 1 ;
      l = sizeof ( radio_css ) ;
    }
    else if ( pagename.indexOf ( "config.html" ) >= 0 ) // Config page is in PROGMEM
    {
      p = config_html ;
      l = sizeof ( config_html ) ;
    }
    else if ( pagename.indexOf ( "mp3play.html" ) >= 0 ) // Mp3player page is in PROGMEM
    {
      p = mp3play_html ;
      l = sizeof ( mp3play_html ) ;
    }
    else if ( pagename.indexOf ( "about.html" ) >= 0 )  // About page is in PROGMEM
    {
      p = about_html ;
      l = sizeof ( about_html ) ;
    }
    else if ( pagename.indexOf ( "favicon.ico" ) >= 0 ) // Favicon icon is in PROGMEM
    {
      p = (char*)favicon_ico ;
      l = sizeof ( favicon_ico ) ;
    }
    else
    {
      p = index_html ;
      l = sizeof ( index_html ) ;
    }
    if ( *p == '\n' )                                   // If page starts with newline:
    {
      p++ ;                                             // Skip first character
      l-- ;
    }
    dbgprint ( "Length of page is %d", strlen ( p ) ) ;
    cmdclient.print ( httpheader ( ct ) ) ;             // Send header
    // The content of the HTTP response follows the header:
    if ( l < 10 )
    {
      cmdclient.println ( "Testline<br>" ) ;
    }
    else
    {
      while ( l )                                       // Loop through the output page
      {
        if ( l <= TCPCHUNKSIZE )                        // Near the end?
        {
          cmdclient.write ( p, l ) ;                    // Yes, send last part
          l = 0 ;
        }
        else
        {
          cmdclient.write ( p, TCPCHUNKSIZE ) ;         // Send part of the page
          p += TCPCHUNKSIZE ;                           // Update startpoint and rest of bytes
          l -= TCPCHUNKSIZE ;
        }
      }
    }
    // The HTTP response ends with another blank line:
    cmdclient.println() ;
    dbgprint ( "Response send" ) ;
  }
}


//**************************************************************************************************
//                                         C H O M P                                               *
//**************************************************************************************************
// Do some filtering on de inputstring:                                                            *
//  - String comment part (starting with "#").                                                     *
//  - Strip trailing CR.                                                                           *
//  - Strip leading spaces.                                                                        *
//  - Strip trailing spaces.                                                                       *
//**************************************************************************************************
void chomp ( String &str )
{
  int   inx ;                                         // Index in de input string

  if ( ( inx = str.indexOf ( "#" ) ) >= 0 )           // Comment line or partial comment?
  {
    str.remove ( inx ) ;                              // Yes, remove
  }
  str.trim() ;                                        // Remove spaces and CR
}


//**************************************************************************************************
//                                     A N A L Y Z E C M D                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                         *
// Version for handling string with: <parameter>=<value>                                           *
//**************************************************************************************************
const char* analyzeCmd ( const char* str )
{
  char*        value ;                           // Points to value after equalsign in command
  const char*  res ;                             // Result of analyzeCmd

  value = strstr ( str, "=" ) ;                  // See if command contains a "="
  if ( value )
  {
    *value = '\0' ;                              // Separate command from value
    res = analyzeCmd ( str, value + 1 ) ;        // Analyze command and handle it
    *value = '=' ;                               // Restore equal sign
  }
  else
  {
    res = analyzeCmd ( str, "0" ) ;              // No value, assume zero
  }
  return res ;
}

String voll;
//**************************************************************************************************
//                                     A N A L Y Z E C M D                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, serial or MQTT.                         *
// par holds the parametername and val holds the value.                                            *
// "wifi_00" and "preset_00" may appear more than once, like wifi_01, wifi_02, etc.                *
// Examples with available parameters:                                                             *
//   preset     = 12                        // Select start preset to connect to                   *
//   preset_00  = <mp3 stream>              // Specify station for a preset 00-99 *)               *
//   volume     = 95                        // Percentage between 0 and 100                        *
//   upvolume   = 2                         // Add percentage to current volume                    *
//   downvolume = 2                         // Subtract percentage from current volume             *
//   toneha     = <0..15>                   // Setting treble gain                                 *
//   tonehf     = <0..15>                   // Setting treble frequency                            *
//   tonela     = <0..15>                   // Setting bass gain                                   *
//   tonelf     = <0..15>                   // Setting treble frequency                            *
//   station    = <mp3 stream>              // Select new station (will not be saved)              *
//   station    = <URL>.mp3                 // Play standalone .mp3 file (not saved)               *
//   station    = <URL>.m3u                 // Select playlist (will not be saved)                 *
//   stop                                   // Stop playing                                        *
//   resume                                 // Resume playing                                      *
//   mute                                   // Mute/unmute the music (toggle)                      *
//   wifi_00    = mySSID/mypassword         // Set WiFi SSID and password *)                       *
//   mqttbroker = mybroker.com              // Set MQTT broker to use *)                           *
//   mqttprefix = XP93g                     // Set MQTT broker to use                              *
//   mqttport   = 1883                      // Set MQTT port to use, default 1883 *)               *
//   mqttuser   = myuser                    // Set MQTT user for authentication *)                 *
//   mqttpasswd = mypassword                // Set MQTT password for authentication *)             *
//   clk_server = pool.ntp.org              // Time server to be used *)                           *
//   clk_offset = <-11..+14>                // Offset with respect to UTC in hours *)              *
//   clk_dst    = <1..2>                    // Offset during daylight saving time in hours *)      *
//   mp3track   = <nodeID>                  // Play track from SD card, nodeID 0 = random          *
//   settings                               // Returns setting like presets and tone               *
//   status                                 // Show current URL to play                            *
//   test                                   // For test purposes                                   *
//   debug      = 0 or 1                    // Switch debugging on or off                          *
//   reset                                  // Restart the ESP32                                   *
//   bat0       = 2318                      // ADC value for an empty battery                      *
//   bat100     = 2916                      // ADC value for a fully charged battery               *
//  Commands marked with "*)" are sensible during initialization only                              *
//**************************************************************************************************
const char* analyzeCmd ( const char* par, const char* val )
{
  String             argument ;                       // Argument as string
  String             value ;                          // Value of an argument as a string
  int                ivalue ;                         // Value of argument as an integer
  static char        reply[180] ;                     // Reply to client, will be returned
  uint8_t            oldvol ;                         // Current volume
  bool               relative ;                       // Relative argument (+ or -)
  String             tmpstr ;                         // Temporary for value
  uint32_t           av ;                             // Available in stream/file

  strcpy ( reply, "Command accepted" ) ;              // Default reply
  argument = String ( par ) ;                         // Get the argument
  chomp ( argument ) ;                                // Remove comment and useless spaces
  if (argument!=""){
    if(tolocalap){

    dbgprint("localAP timeout dismiss ");
    oledshow(3,"Timeout dismissed!");
    tolocalap=false;
    }
  }
  if ( argument.length() == 0 )                       // Lege commandline (comment)?
  {
    return reply ;                                    // Ignore
  }
  argument.toLowerCase() ;                            // Force to lower case
  value = String ( val ) ;                            // Get the specified value
  chomp ( value ) ;                                   // Remove comment and extra spaces
  ivalue = value.toInt() ;                            // Also as an integer
  ivalue = abs ( ivalue ) ;                           // Make positive
  relative = argument.indexOf ( "up" ) == 0 ;         // + relative setting?
  if ( argument.indexOf ( "down" ) == 0 )             // - relative setting?
  {
    relative = true ;                                 // It's relative
    ivalue = - ivalue ;                               // But with negative value
  }
  if ( value.startsWith ( "http://" ) )               // Does (possible) URL contain "http://"?
  {
    value.remove ( 0, 7 ) ;                           // Yes, remove it
  }
  if ( value.length() )
  {
    tmpstr = value ;                                  // Make local copy of value
    if ( argument.indexOf ( "passw" ) >= 0 )          // Password in value?
    {
      tmpstr = String ( "*******" ) ;                 // Yes, hide it
    }
    dbgprint ( "Command: %s with parameter %s",
               argument.c_str(), tmpstr.c_str() ) ;
  }
  else
  {
    dbgprint ( "Command: %s (without parameter)",
               argument.c_str() ) ;
  }
  if ( argument.indexOf ( "volume" ) >= 0 )           // Volume setting?
  {
    // Volume may be of the form "upvolume", "downvolume" or "volume" for relative or absolute setting
    curvsvol = vs1053player->getVolume() ;              // Get current volume
    if ( relative )                                   // + relative setting?
    {
      ini_block.reqvol = curvsvol + ivalue ;            // Up/down by 0.5 or more dB
    }
    else
    {
      ini_block.reqvol = ivalue ;                     // Absolue setting
    }
    if ( ini_block.reqvol > 127 )                     // Wrapped around?
    {
      ini_block.reqvol = 0 ;                          // Yes, keep at zero
    }
    if ( ini_block.reqvol > 100 )
    {
      ini_block.reqvol = 100 ;                        // Limit to normal values
    }
    muteflag = false ;                                // Stop possibly muting
    sprintf ( reply, "ESP respond: Volume is now %d",              // Reply new volume
              ini_block.reqvol ) ;
    //  tftset(3,((char*)ini_block.reqvol));
     voll= String(ini_block.reqvol);
     oledshow ( 3, "Volume is now : "+ voll ) ;
     oledshow(1, "preset: " +  String(ini_block.newpreset)+" | volume: " + voll);
    //oledshow(2, voll);    
  }
  else if ( argument == "mute" )                      // Mute/unmute request
  {
    muteflag = !muteflag ;                            // Request volume to zero/normal
    oledshow ( 3, "Radio muted" ) ;
  }
  else if ( argument.indexOf ( "ir_" ) >= 0 )         // Ir setting?
  { // Do not handle here
  }
  else if ( argument.indexOf ( "preset_" ) >= 0 )     // Enumerated preset?
  { // Do not handle here
  }
  else if ( argument.indexOf ( "preset" ) >= 0 )      // (UP/DOWN)Preset station?
  {
    if ( relative )                                   // Relative argument?
    {
      ini_block.newpreset += ivalue ;                 // Yes, adjust currentpreset
    }
    else
    {
      ini_block.newpreset = ivalue ;                  // Otherwise set station
      playlist_num = 0 ;                              // Absolute, reset playlist
    }
    sprintf ( reply, "ESP respond: Preset is now %d",              // Reply new preset
              ini_block.newpreset ) ;
    oledshow(3,"Preset is now :" + String(ini_block.newpreset)              // Reply new preset
               ) ;
    oledshow(1, "preset: " + String(ini_block.newpreset)+" | volume: " +  voll);
  }
  else if(argument.indexOf("sleep")>=0)
  {
      if(ivalue>5){
        sleepvalsec=(ivalue*60);
        sleepalarm=true;
        sprintf(reply, "sleep alarm set to %d", ivalue);
      }else{
        sprintf(reply, "You must to set sleep time at least more then 5 minutes");
      }
  }
  else if ( argument == "stop" )                      // (un)Stop requested?
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )

    {
      datamode = STOPREQD ;                           // Request STOP
    }
    else
    {
      hostreq = true ;                                // Request UNSTOP
    }
  }
  else if ( ( value.length() > 0 ) &&
            ( ( argument == "mp3track" ) ||           // Select a track from SD card?
              ( argument == "station" ) ) )           // Station in the form address:port
  {
    if ( argument.startsWith ( "mp3" ) )              // MP3 track to search for
    {
      if ( !SD_okay )                                 // SD card present?
      {
        strcpy ( reply, "Command not accepted!" ) ;   // Error reply
        return reply ;
      }
      value = getSDfilename ( value ) ;               // like "localhost/........"
    }
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )
    {
      datamode = STOPREQD ;                           // Request STOP
    }
    host = value ;                                    // Save it for storage and selection later
    hostreq = true ;                                  // Force this station as new preset
    sprintf ( reply,
              "Playing %s",                           // Format reply
              host.c_str() ) ;
    utf8ascii ( reply ) ;                             // Remove possible strange characters
    oledshow(2, "Playing: " + String(host.c_str()));
  }
  else if ( argument == "status" )                    // Status request
  {
    if ( datamode == STOPPED )
    {
      sprintf ( reply, "Player stopped" ) ;           // Format reply
      oledshow ( 3, "Radio stopped" ) ;
    }
    else
    {
      // oledshow ( 2, "" ) ;             // clear the bottom side // airasz added
      if(icystreamtitle== "") {

       sprintf ( reply, "%s", icyname.c_str() ) ;            // Streamtitle from metadata
      }else{
        sprintf ( reply, "%s - %s", icyname.c_str(),
                icystreamtitle.c_str() ) ;            // Streamtitle from metadata
      }
    }
  }
  else if ( argument.startsWith ( "reset" ) )         // Reset request
  {
    resetreq = true ;                                 // Reset all
  }
  else if ( argument == "restart"){
    // ESP.restart();

    sprintf ( reply, "Radio restarting" ) ;           // Format reply
    resetreq = true ; 
  }
  else if ( argument == "reboot"){
    // ESP.restart();
    
    sprintf ( reply, "Radio restarting" ) ;           // Format reply
    resetreq = true ;
  }
  else if ( argument == "test" )                      // Test command
  {
    if ( localfile )
    {
      av = mp3filelength ;                            // Available bytes in file
    }
    else
    {
      av = mp3client.available() ;                    // Available in stream
    }
    sprintf ( reply, "Free memory is %d, chunks in queue %d, stream %d, bitrate %d kbps",
              ESP.getFreeHeap(),
              uxQueueMessagesWaiting ( dataqueue ),
              av,
              mbitrate ) ;
    dbgprint ( "Stack maintask is %d", uxTaskGetStackHighWaterMark ( maintask ) ) ;
    dbgprint ( "Stack playtask is %d", uxTaskGetStackHighWaterMark ( xplaytask ) ) ;
    dbgprint ( "Stack spftask  is %d", uxTaskGetStackHighWaterMark ( xspftask ) ) ;
    dbgprint ( "ADC reading is %d", adcval ) ;
    dbgprint ( "scaniocount is %d", scaniocount ) ;
  }
  // Commands for bass/treble control
  else if ( argument.startsWith ( "tone" ) )          // Tone command
  {
    if ( argument.indexOf ( "ha" ) > 0 )              // High amplitue? (for treble)
    {
      ini_block.rtone[0] = ivalue ;                   // Yes, prepare to set ST_AMPLITUDE
    }
    if ( argument.indexOf ( "hf" ) > 0 )              // High frequency? (for treble)
    {
      ini_block.rtone[1] = ivalue ;                   // Yes, prepare to set ST_FREQLIMIT
    }
    if ( argument.indexOf ( "la" ) > 0 )              // Low amplitue? (for bass)
    {
      ini_block.rtone[2] = ivalue ;                   // Yes, prepare to set SB_AMPLITUDE
    }
    if ( argument.indexOf ( "lf" ) > 0 )              // High frequency? (for bass)
    {
      ini_block.rtone[3] = ivalue ;                   // Yes, prepare to set SB_FREQLIMIT
    }
    reqtone = true ;                                  // Set change request
    sprintf ( reply, "Parameter for bass/treble %s set to %d",
              argument.c_str(), ivalue ) ;
  }
  else if ( argument.startsWith ( "mqtt" ) )          // Parameter fo MQTT?
  {
    strcpy ( reply, "MQTT broker parameter changed. Save and restart to have effect" ) ;
    if ( argument.indexOf ( "broker" ) > 0 )          // Broker specified?
    {
      ini_block.mqttbroker = value ;                  // Yes, set broker accordingly
    }
    else if ( argument.indexOf ( "prefix" ) > 0 )     // Port specified?
    {
      ini_block.mqttprefix = value ;                  // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "port" ) > 0 )       // Port specified?
    {
      ini_block.mqttport = ivalue ;                   // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "user" ) > 0 )       // User specified?
    {
      ini_block.mqttuser = value ;                    // Yes, set user accordingly
    }
    else if ( argument.indexOf ( "passwd" ) > 0 )     // Password specified?
    {
      ini_block.mqttpasswd = value.c_str() ;          // Yes, set broker password accordingly
    }
  }
  else if ( argument == "debug" )                     // debug on/off request?
  {
    DEBUG = ivalue ;                                  // Yes, set flag accordingly
  }
  else if ( argument == "getnetworks" )               // List all WiFi networks?
  {
    sprintf ( reply, networks.c_str() ) ;             // Reply is SSIDs
  }
  else if ( argument.startsWith ( "clk_" ) )          // TOD parameter?
  {
    if ( argument.indexOf ( "server" ) > 0 )          // Yes, NTP server spec?
    {
      ini_block.clk_server = value ;                  // Yes, set server
    }
    if ( argument.indexOf ( "offset" ) > 0 )          // Offset with respect to UTC spec?
    {
      ini_block.clk_offset = ivalue ;                 // Yes, set offset
    }
    if ( argument.indexOf ( "dst" ) > 0 )             // Offset duringe DST spec?
    {
      ini_block.clk_dst = ivalue ;                    // Yes, set DST offset
    }
  }
  else if ( argument.startsWith ( "bat" ) )           // Battery ADC value?
  {
    if ( argument.indexOf ( "100" ) == 3 )            // 100 percent value?
    {
      ini_block.bat100 = ivalue ;                     // Yes, set it
    }
    else if ( argument.indexOf ( "0" ) == 3 )         // 0 percent value?
    {
      ini_block.bat0 = ivalue ;                       // Yes, set it
    }
  }
  else if(argument.startsWith ("ping"))
  {
      sprintf(reply, "ESP");
  } 

  else if(argument.startsWith ("savepref"))
  {
      forcesavepref();  
      sprintf(reply, "preferences has been successfully saved");
  }

  else if(argument == "populated")
  {   
      String allpreset="list\n";   
      String tmppreset="";
      for(int i=0; i<101; i++){
        numOfPreset+=1;
           tmppreset=readhostfrompref(i).c_str();       //get station name 
          tmppreset+="\n";                              //add next station as newline
          int in=tmppreset.indexOf("#");                //get comment sign
          if(in>0){
              tmppreset=tmppreset.substring(in);        //get comment only
          }else{break;}
         allpreset=allpreset+ tmppreset;                //list all station available in one packet string
      }                                                 //spatrated by newline
      sprintf(reply,allpreset.c_str());
  } 
  else if (argument=="getsync"){
    String datasync="";
    if ( datamode == STOPPED )
    {
      sprintf ( reply, "Player stopped" ) ;           // Format reply
      oledshow ( 3, "Radio stopped" ) ;
    }
    else
    {
      // oledshow ( 2, "" ) ;             // clear the bottom side // airasz added
      if(icystreamtitle== "") {

       // sprintf ( reply, "%s", icyname.c_str() ) ;            // Streamtitle from metadata
      datasync="sync\n";
      datasync=datasync+icyname.c_str()+"\n";
      datasync+=String(ini_block.newpreset)+"\n";
      datasync+=String(ini_block.reqvol)+"\n";
      sprintf(reply,datasync.c_str());
      }else{
        // sprintf ( reply, "%s - %s", icyname.c_str(),
        //         icystreamtitle.c_str() ) ;            // Streamtitle from metadata

      datasync="sync\n";
      datasync=datasync+icyname.c_str()+" - " + icystreamtitle.c_str()+"\n";
      datasync+=String(ini_block.newpreset)+"\n";
      datasync+=String(ini_block.reqvol);

      sprintf(reply,datasync.c_str());
      }
    }
  }
  else if (argument.indexOf("reqp")>=0) {

    String pr=readhostfrompref(ivalue).c_str();
    int in=pr.indexOf("#");
    // pr.remove(0,in);
    // chomp(pr);
    pr=pr.substring(in);
    //sprintf(reply, readhostfrompref(0).c_str());
    // pr.c_str();
    // pr=pr+"resp";
    // sprintf(reply, "resp%s",pr.c_str());

    sprintf(reply, pr.c_str());
     
  }
  else
  {
    sprintf ( reply, "%s called with illegal parameter: %s",
              NAME, argument.c_str() ) ;
  }
  return reply ;                                      // Return reply to the caller
}


//**************************************************************************************************
//                                     H T T P H E A D E R                                         *
//**************************************************************************************************
// Set http headers to a string.                                                                   *
//**************************************************************************************************
String httpheader ( String contentstype )
{
  return String ( "HTTP/1.1 200 OK\nContent-type:" ) +
         contentstype +
         String ( "\n"
                  "Server: " NAME "\n"
                  "Cache-Control: " "max-age=3600\n"
                  "Last-Modified: " VERSION "\n\n" ) ;
}

int GMT=7;            //depending to your country
//**************************************************************************************************
//                                         G E T T I M E                                           *
//**************************************************************************************************
// Retrieve the local time from NTP server and convert to string.                                  *
// Will be called every second.                                                                    *
//**************************************************************************************************
void gettime()
{
  static int16_t delaycount = 0 ;                           // To reduce number of NTP requests
  static int16_t retrycount = 100 ;

  if ( tft )                                                // TFT used?
  {
    if ( timeinfo.tm_year )                                 // Legal time found?
    {
      // sprintf ( timetxt, "%02d:%02d:%02d",                  // Yes, format to a string
      //           timeinfo.tm_hour,
      //           timeinfo.tm_min,
      //           timeinfo.tm_sec ) ;

      sprintf ( timetxt, "%02d:%02d",                  // Yes, format to a string
                timeinfo.tm_hour,
                timeinfo.tm_min) ;
    }
    if ( --delaycount <= 0 )                                // Sync every few hours
    {
      delaycount = 7200 ;                                   // Reset counter
      if ( timeinfo.tm_year )                               // Legal time found?
      {
        dbgprint ( "Sync TOD, old value is %s", timetxt ) ;
      }
      dbgprint ( "Sync TOD" ) ;
      if ( !getLocalTime ( &timeinfo ) )                    // Read from NTP server
      {
        dbgprint ( "Failed to obtain time!" ) ;             // Error
        timeinfo.tm_year = 0 ;                              // Set current time to illegal
        if ( retrycount )                                   // Give up syncing?
        {
          retrycount-- ;                                    // No try again
          delaycount = 5 ;                                  // Retry after 5 seconds
        }
      }
      else
      {
        // sprintf ( timetxt, "%02d:%02d:%02d",                // Format new time to a string
        //           timeinfo.tm_hour,
        //           timeinfo.tm_min,
        //           timeinfo.tm_sec ) ;

      sprintf ( timetxt, "%02d:%02d",                  // Yes, format to a string
                timeinfo.tm_hour,
                timeinfo.tm_min) ;
        dbgprint ( "Sync TOD, new value is %s", timetxt ) ;
      }
    }
  }
}

//**************************************************************************************************
//                                     P L A Y T A S K                                             *
//**************************************************************************************************
// Play stream data from input queue.                                                              *
// Handle all I/O to VS1053B during normal playing.                                                *
// Handles display of text, time and volume on TFT as well.                                        *
//**************************************************************************************************
void playtask ( void * parameter )
{
  while ( true )
  {
    if ( xQueueReceive ( dataqueue, &inchunk, 5 ) )
    {
      while ( !vs1053player->data_request() )                       // If FIFO is full..
      {
        vTaskDelay ( 1 ) ;                                          // Yes, take a break
      }
      switch ( inchunk.datatyp )                                    // What kind of chunk?
      {
        case QDATA:
          claimSPI ( "chunk" ) ;                                    // Claim SPI bus
          vs1053player->playChunk ( inchunk.buf,                    // DATA, send to player
                                    sizeof(inchunk.buf) ) ;
          releaseSPI() ;                                            // Release SPI bus
          totalcount += sizeof(inchunk.buf) ;                       // Count the bytes
          break ;
        case QSTARTSONG:
          playingstat = 1 ;                                         // Status for MQTT
          mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
          claimSPI ( "startsong" ) ;                                // Claim SPI bus
          vs1053player->startSong() ;                               // START, start player
          releaseSPI() ;                                            // Release SPI bus
          break ;
        case QSTOPSONG:
          playingstat = 0 ;                                         // Status for MQTT
          mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
          claimSPI ( "stopsong" ) ;                                 // Claim SPI bus
          vs1053player->setVolume ( 0 ) ;                           // Mute
          vs1053player->stopSong() ;                                // STOP, stop player
          releaseSPI() ;                                            // Release SPI bus
          vTaskDelay ( 500 / portTICK_PERIOD_MS ) ;                 // Pause for a short time
          break ;
        default:
          break ;
      }
    }
    //esp_task_wdt_reset() ;                                        // Protect against idle cpu
  }
  //vTaskDelete ( NULL ) ;                                          // Will never arrive here
}




