#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <EasyButton.h>
#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
#include <TFT_eSPI.h>
#include <JsonListener.h>
#include <SPI.h>
#include <NTPClient.h>
#include "Free_Fonts.h"
#include "time.h"
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "User_Setup.h"
#define FS_NO_GLOBALS
#include <FS.h>
#include <LittleFS.h>
#include "alert.h"
#include "OpenWeatherMapOneCall.h"



//#define LOAD_GFXFF


//ringmeter
#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5
#define TFT_GREY 0x2104 // Dark grey 16 bit colour
#define BUTTON_PIN D1


EasyButton button(BUTTON_PIN);

uint32_t runTime = -99999;       // time for next update

int reading = 0; // Value to be displayed
int d = 0; // Variable used for the sinewave test waveform
boolean range_error = 0;
int8_t ramp = 1;


OpenWeatherMapCurrent client;
//OpenWeatherMapForecast client2;
OpenWeatherMapOneCall client2;
OpenWeatherMapOneCallData openWeatherMapOneCallData;

String OPEN_WEATHER_MAP_APP_ID = "05575ef18e43f6ac0519dadf9cf56621";
String OPEN_WEATHER_MAP_LOCATION_ID = "3054643";
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = true;
//uint8_t MAX_FORECASTS = 3;
  float OPEN_WEATHER_MAP_LOCATTION_LAT = 47.498; // Bp
  float OPEN_WEATHER_MAP_LOCATTION_LON = 19.0399; // Bp

bool startWeather = true;
bool startForeCast = true;
bool hwClear = true;
bool wClear = true;
TFT_eSPI tft = TFT_eSPI();

WiFiManager wifiManager;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, 3600);

const long interval = 1500000;
unsigned long previousMillis = 0;

const long interval2 = 3600000;
unsigned long previousMillis2 = 0;


String weekDays[7]={"Vasarnap", "Hetfo", "Kedd", "Szerda", "Csutortok", "Pentek", "Szombat"};


void onPressed()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setCursor(60, 70, 6);
  tft.print("Update...");
  //ESPhttpUpdate.update("192.168.0.129", 80, "/firmware.bin");
  //ESPhttpUpdate.updateSpiffs("192.168.0.129", 80, "/littlefs.bin");
  //Serial.println("Button pressed");
  
}




void setup() {
  //screen
  
  
  
  
  Serial.begin(9600);  
  if (!LittleFS.begin()) {  //SPIFFS initialize
    Serial.println("SPIFFS initialze error");
   while (1) yield(); // wait
  }  
   
  timeClient.begin();
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setCursor(60, 70, 4);
  tft.print("Waiting for WiFi");
  tft.setCursor(60, 130, 4);
  tft.print("Go to AutoConnectAP");
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("connected...yeey :)");
  delay(6000);
  
  //timeClient.forceUpdate();


  // Initialize the button.
  button.begin();
  // Add the callback function to be called when the button is pressed.
  button.onPressed(onPressed);
  timeClient.forceUpdate();
  
}




uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}


float sineWave(int phase) {
  return sin(phase * 0.0174532925);
}

#define BUFF_SIZE 64


void drawIcon(const unsigned short* icon, int16_t x, int16_t y, int8_t width, int8_t height) {

  uint16_t  pix_buffer[BUFF_SIZE];   // Pixel buffer (16 bits per pixel)

  tft.startWrite();

  // Set up a window the right size to stream pixels into
  tft.setAddrWindow(x, y, width, height);

  // Work out the number whole buffers to send
  uint16_t nb = ((uint16_t)height * width) / BUFF_SIZE;

  // Fill and send "nb" buffers to TFT
  for (int i = 0; i < nb; i++) {
    for (int j = 0; j < BUFF_SIZE; j++) {
      pix_buffer[j] = pgm_read_word(&icon[i * BUFF_SIZE + j]);
    }
    tft.pushColors(pix_buffer, BUFF_SIZE);
  }

  // Work out number of pixels not yet sent
  uint16_t np = ((uint16_t)height * width) % BUFF_SIZE;

  // Send any partial buffer left over
  if (np) {
    for (int i = 0; i < np; i++) pix_buffer[i] = pgm_read_word(&icon[nb * BUFF_SIZE + i]);
    tft.pushColors(pix_buffer, np);
  }

  tft.endWrite();
}
void drawAlert(int x, int y , int side, boolean draw)
{
  if (draw && !range_error) {
    drawIcon(alert, x - alertWidth/2, y - alertHeight/2, alertWidth, alertHeight);
    range_error = 1;
  }
  else if (!draw) {
    tft.fillRect(x - alertWidth/2, y - alertHeight/2, alertWidth, alertHeight, TFT_WHITE);
    range_error = 0;
  }
}


int ringMeter(int value, int vmin, int vmax, int x, int y, int r, const char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring

  int w = r / 3;    // Width of outer ring is 1/4 of radius
  
  int angle = 150;  // Half the sweep angle of meter (300 degrees)

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 3; // Segments are 3 degrees wide = 100 segments for 300 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Variable to save "value" text colour from scheme and set default
  int colour = TFT_BLACK;
 
  // Draw colour blocks every inc degrees
  for (int i = -angle+inc/2; i < angle-inc/2; i += inc) {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      switch (scheme) {
        case 0: colour = TFT_RED; break; // Fixed colour
        case 1: colour = TFT_GREEN; break; // Fixed colour
        case 2: colour = TFT_BLUE; break; // Fixed colour
        case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
        case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break; // Green to red (high temperature etc)
        case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
        default: colour = TFT_BLACK; break; // Fixed colour
      }
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      //text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREY);
    }
  }
  // Convert value to a string
  char buf[10];
  byte len = 3; if (value > 999) len = 5;
  dtostrf(value, len, 0, buf);
  buf[len] = ' '; buf[len+1] = 0; // Add blanking space and terminator, helps to centre text too!
  // Set the text colour to default
  tft.setTextSize(1);

  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  // Uncomment next line to set the text colour to the last segment value!
  tft.setTextColor(colour, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  // Print value, if the meter is large then use big font 8, othewise use 4
  if (r > 84) {
    tft.setTextPadding(55*3); // Allow for 3 digits each 55 pixels wide
    tft.drawString(buf, x, y, 8); // Value in middle
  }
  else {
    tft.setTextPadding(3 * 14); // Allow for 3 digits each 14 pixels wide
    tft.drawString(buf, x, y, 4); // Value in middle
  }
  tft.setTextSize(1);
  tft.setTextPadding(0);
  // Print units, if the meter is large then use big font 4, othewise use 2
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  if (r > 84) tft.drawString(units, x, y + 60, 4); // Units display
  else tft.drawString(units, x, y + 15, 2); // Units display

  // Calculate and return right hand side x coordinate
  return x + r;
}



void drawBmp(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = LittleFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
     // Serial.print("Loaded in "); Serial.print(millis() - startTime);
     // Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}



void printWeatherIcon(int id) {
 switch(id) {
  case 800: drawBmp("/sun2.bmp",190, 55); break;
  case 801: drawBmp("/sunny.bmp",190, 55); break;
  case 802: drawBmp("/sunny.bmp",190, 55); break;
  case 803: drawBmp("/cloud.bmp",190, 55); break;
  case 804: drawBmp("/cloud.bmp",190, 55); break;
  
  case 200: drawBmp("/storm.bmp",190, 55); break;
  case 201: drawBmp("/storm.bmp",190, 55); break;
  case 202: drawBmp("/storm.bmp",190, 55); break;
  case 210: drawBmp("/storm.bmp",190, 55); break;
  case 211: drawBmp("/storm.bmp",190, 55); break;
  case 212: drawBmp("/storm.bmp",190, 55); break;
  case 221: drawBmp("/storm.bmp",190, 55); break;
  case 230: drawBmp("/storm.bmp",190, 55); break;
  case 231: drawBmp("/storm.bmp",190, 55); break;
  case 232: drawBmp("/storm.bmp",190, 55); break;

  case 300: drawBmp("/rain2.bmp",190, 55); break;
  case 301: drawBmp("/rain2.bmp",190, 55); break;
  case 302: drawBmp("/rain2.bmp",190, 55); break;
  case 310: drawBmp("/rain2.bmp",190, 55); break;
  case 311: drawBmp("/rain2.bmp",190, 55); break;
  case 312: drawBmp("/rain2.bmp",190, 55); break;
  case 313: drawBmp("/rain2.bmp",190, 55); break;
  case 314: drawBmp("/rain2.bmp",190, 55); break;
  case 321: drawBmp("/rain2.bmp",190, 55); break;

  case 500: drawBmp("/moderate_rain.bmp",190, 55); break;
  case 501: drawBmp("/moderate_rain.bmp",190, 55); break;
  case 502: drawBmp("/moderate_rain.bmp",190, 55); break;
  case 503: drawBmp("/storm.bmp",190, 55); break;
  case 504: drawBmp("/storm.bmp",190, 55); break;
  case 511: drawBmp("/rain2.bmp",190, 55); break;
  case 520: drawBmp("/rain2.bmp",190, 55); break;
  case 521: drawBmp("/rain2.bmp",190, 55); break;
  case 522: drawBmp("/rain2.bmp",190, 55); break;
  case 531: drawBmp("/rain2.bmp",190, 55); break;

  case 600: drawBmp("/snow4.bmp",190, 55); break;
  case 601: drawBmp("/snow.bmp",190, 55); break;
  case 602: drawBmp("/snow.bmp",190, 55); break;
  case 611: drawBmp("/snow.bmp",190, 55); break;
  case 612: drawBmp("/snow4.bmp",190, 55); break;
  case 615: drawBmp("/snow.bmp",190, 55); break;
  case 616: drawBmp("/snow.bmp",190, 55); break;
  case 620: drawBmp("/snow4.bmp",190, 55); break;
  case 621: drawBmp("/snow.bmp",190, 55); break;
  case 622: drawBmp("/snow.bmp",190, 55); break;

  case 701: drawBmp("/fog.bmp",190, 55); break;
  case 711: drawBmp("/fog.bmp",190, 55); break;
  case 721: drawBmp("/fog.bmp",190, 55); break;
  case 731: drawBmp("/fog.bmp",190, 55); break;
  case 741: drawBmp("/fog.bmp",190, 55); break;
  case 751: drawBmp("/fog.bmp",190, 55); break;
  case 761: drawBmp("/fog.bmp",190, 55); break;
  case 762: drawBmp("/fog.bmp",190, 55); break;
  case 771: drawBmp("/fog.bmp",190, 55); break;
  case 781: drawBmp("/fog.bmp",190, 55); break;
  default:break; 
  }
}

void printWeatherIconNight(int id) {
 switch(id) {
  case 800: drawBmp("/sun2_night.bmp",190, 55); break;
  case 801: drawBmp("/sunny_night.bmp",190, 55); break;
  case 802: drawBmp("/sunny_night.bmp",190, 55); break;
  case 803: drawBmp("/cloud.bmp",190, 55); break;
  case 804: drawBmp("/cloud.bmp",190, 55); break;
  
  case 200: drawBmp("/storm.bmp",190, 55); break;
  case 201: drawBmp("/storm.bmp",190, 55); break;
  case 202: drawBmp("/storm.bmp",190, 55); break;
  case 210: drawBmp("/storm.bmp",190, 55); break;
  case 211: drawBmp("/storm.bmp",190, 55); break;
  case 212: drawBmp("/storm.bmp",190, 55); break;
  case 221: drawBmp("/storm.bmp",190, 55); break;
  case 230: drawBmp("/storm.bmp",190, 55); break;
  case 231: drawBmp("/storm.bmp",190, 55); break;
  case 232: drawBmp("/storm.bmp",190, 55); break;

  case 300: drawBmp("/rain2.bmp",190, 55); break;
  case 301: drawBmp("/rain2.bmp",190, 55); break;
  case 302: drawBmp("/rain2.bmp",190, 55); break;
  case 310: drawBmp("/rain2.bmp",190, 55); break;
  case 311: drawBmp("/rain2.bmp",190, 55); break;
  case 312: drawBmp("/rain2.bmp",190, 55); break;
  case 313: drawBmp("/rain2.bmp",190, 55); break;
  case 314: drawBmp("/rain2.bmp",190, 55); break;
  case 321: drawBmp("/rain2.bmp",190, 55); break;

  case 500: drawBmp("/moderate_rain_night.bmp",190, 55); break;
  case 501: drawBmp("/moderate_rain_night.bmp",190, 55); break;
  case 502: drawBmp("/moderate_rain_night.bmp",190, 55); break;
  case 503: drawBmp("/storm.bmp",190, 55); break;
  case 504: drawBmp("/storm.bmp",190, 55); break;
  case 511: drawBmp("/rain2.bmp",190, 55); break;
  case 520: drawBmp("/rain2.bmp",190, 55); break;
  case 521: drawBmp("/rain2.bmp",190, 55); break;
  case 522: drawBmp("/rain2.bmp",190, 55); break;
  case 531: drawBmp("/rain2.bmp",190, 55); break;

  case 600: drawBmp("/snow4_night.bmp",190, 55); break;
  case 601: drawBmp("/snow.bmp",190, 55); break;
  case 602: drawBmp("/snow.bmp",190, 55); break;
  case 611: drawBmp("/snow.bmp",190, 55); break;
  case 612: drawBmp("/snow4_night.bmp",190, 55); break;
  case 615: drawBmp("/snow.bmp",190, 55); break;
  case 616: drawBmp("/snow.bmp",190, 55); break;
  case 620: drawBmp("/snow4_night.bmp",190, 55); break;
  case 621: drawBmp("/snow.bmp",190, 55); break;
  case 622: drawBmp("/snow.bmp",190, 55); break;

  case 701: drawBmp("/fog.bmp",190, 55); break;
  case 711: drawBmp("/fog.bmp",190, 55); break;
  case 721: drawBmp("/fog.bmp",190, 55); break;
  case 731: drawBmp("/fog.bmp",190, 55); break;
  case 741: drawBmp("/fog.bmp",190, 55); break;
  case 751: drawBmp("/fog.bmp",190, 55); break;
  case 761: drawBmp("/fog.bmp",190, 55); break;
  case 762: drawBmp("/fog.bmp",190, 55); break;
  case 771: drawBmp("/fog.bmp",190, 55); break;
  case 781: drawBmp("/fog.bmp",190, 55); break;
  default:break; 
  }
}

void printWeatherIconForeCast0(int id) {
 switch(id) {
  case 800: drawBmp("/sun2.bmp",30, 210); break;
  case 801: drawBmp("/sunny.bmp",30, 210); break;
  case 802: drawBmp("/sunny.bmp",30, 210); break;
  case 803: drawBmp("/cloud.bmp",30, 210); break;
  case 804: drawBmp("/cloud.bmp",30, 210); break;
  
  case 200: drawBmp("/storm.bmp",30, 210); break;
  case 201: drawBmp("/storm.bmp",30, 210); break;
  case 202: drawBmp("/storm.bmp",30, 210); break;
  case 210: drawBmp("/storm.bmp",30, 210); break;
  case 211: drawBmp("/storm.bmp",30, 210); break;
  case 212: drawBmp("/storm.bmp",30, 210); break;
  case 221: drawBmp("/storm.bmp",30, 210); break;
  case 230: drawBmp("/storm.bmp",30, 210); break;
  case 231: drawBmp("/storm.bmp",30, 210); break;
  case 232: drawBmp("/storm.bmp",30, 210); break;

  case 300: drawBmp("/rain2.bmp",30, 210); break;
  case 301: drawBmp("/rain2.bmp",30, 210); break;
  case 302: drawBmp("/rain2.bmp",30, 210); break;
  case 310: drawBmp("/rain2.bmp",30, 210); break;
  case 311: drawBmp("/rain2.bmp",30, 210); break;
  case 312: drawBmp("/rain2.bmp",30, 210); break;
  case 313: drawBmp("/rain2.bmp",30, 210); break;
  case 314: drawBmp("/rain2.bmp",30, 210); break;
  case 321: drawBmp("/rain2.bmp",30, 210); break;

  case 500: drawBmp("/moderate_rain.bmp",30, 210); break;
  case 501: drawBmp("/moderate_rain.bmp",30, 210); break;
  case 502: drawBmp("/moderate_rain.bmp",30, 210); break;
  case 503: drawBmp("/storm.bmp",30, 210); break;
  case 504: drawBmp("/storm.bmp",30, 210); break;
  case 511: drawBmp("/rain2.bmp",30, 210); break;
  case 520: drawBmp("/rain2.bmp",30, 210); break;
  case 521: drawBmp("/rain2.bmp",30, 210); break;
  case 522: drawBmp("/rain2.bmp",30, 210); break;
  case 531: drawBmp("/rain2.bmp",30, 210); break;

  case 600: drawBmp("/snow4.bmp",30, 210); break;
  case 601: drawBmp("/snow.bmp",30, 210); break;
  case 602: drawBmp("/snow.bmp",30, 210); break;
  case 611: drawBmp("/snow.bmp",30, 210); break;
  case 612: drawBmp("/snow4.bmp",30, 210); break;
  case 615: drawBmp("/snow.bmp",30, 210); break;
  case 616: drawBmp("/snow.bmp",30, 210); break;
  case 620: drawBmp("/snow4.bmp",30, 210); break;
  case 621: drawBmp("/snow.bmp",30, 210); break;
  case 622: drawBmp("/snow.bmp",30, 210); break;

  case 701: drawBmp("/fog.bmp",30, 210); break;
  case 711: drawBmp("/fog.bmp",30, 210); break;
  case 721: drawBmp("/fog.bmp",30, 210); break;
  case 731: drawBmp("/fog.bmp",30, 210); break;
  case 741: drawBmp("/fog.bmp",30, 210); break;
  case 751: drawBmp("/fog.bmp",30, 210); break;
  case 761: drawBmp("/fog.bmp",30, 210); break;
  case 762: drawBmp("/fog.bmp",30, 210); break;
  case 771: drawBmp("/fog.bmp",30, 210); break;
  case 781: drawBmp("/fog.bmp",30, 210); break;
  default:break; 
  }
}

void printWeatherIconForeCast1(int id) {
 switch(id) {
  case 800: drawBmp("/sun2.bmp",190, 210); break;
  case 801: drawBmp("/sunny.bmp",190, 210); break;
  case 802: drawBmp("/sunny.bmp",190, 210); break;
  case 803: drawBmp("/cloud.bmp",190, 210); break;
  case 804: drawBmp("/cloud.bmp",190, 210); break;
  
  case 200: drawBmp("/storm.bmp",190, 210); break;
  case 201: drawBmp("/storm.bmp",190, 210); break;
  case 202: drawBmp("/storm.bmp",190, 210); break;
  case 210: drawBmp("/storm.bmp",190, 210); break;
  case 211: drawBmp("/storm.bmp",190, 210); break;
  case 212: drawBmp("/storm.bmp",190, 210); break;
  case 221: drawBmp("/storm.bmp",190, 210); break;
  case 230: drawBmp("/storm.bmp",190, 210); break;
  case 231: drawBmp("/storm.bmp",190, 210); break;
  case 232: drawBmp("/storm.bmp",190, 210); break;

  case 300: drawBmp("/rain2.bmp",190, 210); break;
  case 301: drawBmp("/rain2.bmp",190, 210); break;
  case 302: drawBmp("/rain2.bmp",190, 210); break;
  case 310: drawBmp("/rain2.bmp",190, 210); break;
  case 311: drawBmp("/rain2.bmp",190, 210); break;
  case 312: drawBmp("/rain2.bmp",190, 210); break;
  case 313: drawBmp("/rain2.bmp",190, 210); break;
  case 314: drawBmp("/rain2.bmp",190, 210); break;
  case 321: drawBmp("/rain2.bmp",190, 210); break;

  case 500: drawBmp("/moderate_rain.bmp",190, 210); break;
  case 501: drawBmp("/moderate_rain.bmp",190, 210); break;
  case 502: drawBmp("/moderate_rain.bmp",190, 210); break;
  case 503: drawBmp("/storm.bmp",190, 210); break;
  case 504: drawBmp("/storm.bmp",190, 210); break;
  case 511: drawBmp("/rain2.bmp",190, 210); break;
  case 520: drawBmp("/rain2.bmp",190, 210); break;
  case 521: drawBmp("/rain2.bmp",190, 210); break;
  case 522: drawBmp("/rain2.bmp",190, 210); break;
  case 531: drawBmp("/rain2.bmp",190, 210); break;

  case 600: drawBmp("/snow4.bmp",190, 210); break;
  case 601: drawBmp("/snow.bmp",190, 210); break;
  case 602: drawBmp("/snow.bmp",190, 210); break;
  case 611: drawBmp("/snow.bmp",190, 210); break;
  case 612: drawBmp("/snow4.bmp",190, 210); break;
  case 615: drawBmp("/snow.bmp",190, 210); break;
  case 616: drawBmp("/snow.bmp",190, 210); break;
  case 620: drawBmp("/snow4.bmp",190, 210); break;
  case 621: drawBmp("/snow.bmp",190, 210); break;
  case 622: drawBmp("/snow.bmp",190, 210); break;

  case 701: drawBmp("/fog.bmp",190, 210); break;
  case 711: drawBmp("/fog.bmp",190, 210); break;
  case 721: drawBmp("/fog.bmp",190, 210); break;
  case 731: drawBmp("/fog.bmp",190, 210); break;
  case 741: drawBmp("/fog.bmp",190, 210); break;
  case 751: drawBmp("/fog.bmp",190, 210); break;
  case 761: drawBmp("/fog.bmp",190, 210); break;
  case 762: drawBmp("/fog.bmp",190, 210); break;
  case 771: drawBmp("/fog.bmp",190, 210); break;
  case 781: drawBmp("/fog.bmp",190, 210); break;
  default:break; 
  }
}

void printWeatherIconForeCast2(int id) {
 switch(id) {
  case 800: drawBmp("/sun2.bmp",350, 210); break;
  case 801: drawBmp("/sunny.bmp",350, 210); break;
  case 802: drawBmp("/sunny.bmp",350, 210); break;
  case 803: drawBmp("/cloud.bmp",350, 210); break;
  case 804: drawBmp("/cloud.bmp",350, 210); break;
  
  case 200: drawBmp("/storm.bmp",350, 210); break;
  case 201: drawBmp("/storm.bmp",350, 210); break;
  case 202: drawBmp("/storm.bmp",350, 210); break;
  case 210: drawBmp("/storm.bmp",350, 210); break;
  case 211: drawBmp("/storm.bmp",350, 210); break;
  case 212: drawBmp("/storm.bmp",350, 210); break;
  case 221: drawBmp("/storm.bmp",350, 210); break;
  case 230: drawBmp("/storm.bmp",350, 210); break;
  case 231: drawBmp("/storm.bmp",350, 210); break;
  case 232: drawBmp("/storm.bmp",350, 210); break;

  case 300: drawBmp("/rain2.bmp",350, 210); break;
  case 301: drawBmp("/rain2.bmp",350, 210); break;
  case 302: drawBmp("/rain2.bmp",350, 210); break;
  case 310: drawBmp("/rain2.bmp",350, 210); break;
  case 311: drawBmp("/rain2.bmp",350, 210); break;
  case 312: drawBmp("/rain2.bmp",350, 210); break;
  case 313: drawBmp("/rain2.bmp",350, 210); break;
  case 314: drawBmp("/rain2.bmp",350, 210); break;
  case 321: drawBmp("/rain2.bmp",350, 210); break;

  case 500: drawBmp("/moderate_rain.bmp",350, 210); break;
  case 501: drawBmp("/moderate_rain.bmp",350, 210); break;
  case 502: drawBmp("/moderate_rain.bmp",350, 210); break;
  case 503: drawBmp("/storm.bmp",350, 210); break;
  case 504: drawBmp("/storm.bmp",350, 210); break;
  case 511: drawBmp("/rain2.bmp",350, 210); break;
  case 520: drawBmp("/rain2.bmp",350, 210); break;
  case 521: drawBmp("/rain2.bmp",350, 210); break;
  case 522: drawBmp("/rain2.bmp",350, 210); break;
  case 531: drawBmp("/rain2.bmp",350, 210); break;

  case 600: drawBmp("/snow4.bmp",350, 210); break;
  case 601: drawBmp("/snow.bmp",350, 210); break;
  case 602: drawBmp("/snow.bmp",350, 210); break;
  case 611: drawBmp("/snow.bmp",350, 210); break;
  case 612: drawBmp("/snow4.bmp",350, 210); break;
  case 615: drawBmp("/snow.bmp",350, 210); break;
  case 616: drawBmp("/snow.bmp",350, 210); break;
  case 620: drawBmp("/snow4.bmp",350, 210); break;
  case 621: drawBmp("/snow.bmp",350, 210); break;
  case 622: drawBmp("/snow.bmp",350, 210); break;

  case 701: drawBmp("/fog.bmp",350, 210); break;
  case 711: drawBmp("/fog.bmp",350, 210); break;
  case 721: drawBmp("/fog.bmp",350, 210); break;
  case 731: drawBmp("/fog.bmp",350, 210); break;
  case 741: drawBmp("/fog.bmp",350, 210); break;
  case 751: drawBmp("/fog.bmp",350, 210); break;
  case 761: drawBmp("/fog.bmp",350, 210); break;
  case 762: drawBmp("/fog.bmp",350, 210); break;
  case 771: drawBmp("/fog.bmp",350, 210); break;
  case 781: drawBmp("/fog.bmp",350, 210); break;
  default:break; 
  }
}

void forecast(){
  unsigned long currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 >= interval2 || startForeCast == true) {
    previousMillis2 = currentMillis2;
  
  
    OpenWeatherMapOneCall *oneCallClient = new OpenWeatherMapOneCall();

    oneCallClient->setMetric(IS_METRIC);
  oneCallClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);

  //long executionStart = millis();
  oneCallClient->update(&openWeatherMapOneCallData, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);
  delete oneCallClient;
  //oneCallClient = nullptr;

    //uint8_t foundForecasts = client2.updateForecastsById(data2, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
    //time_t time;

    for (uint8_t i = 0; i < 4; i++) {
      if(openWeatherMapOneCallData.daily[i].dt > 0) {
        tft.setCursor(65, 190, 2);
        
        String foreDay0 = weekDays[((openWeatherMapOneCallData.daily[1].dt / 86400L) + 4 ) % 7];
        tft.println(foreDay0);
        tft.print("       ");
        printWeatherIconForeCast0(openWeatherMapOneCallData.daily[1].weatherId);
        tft.setCursor(45, 280, 2);
        tft.print((int)openWeatherMapOneCallData.daily[1].tempMin);
        tft.print(" `C"); 
        tft.print("/"); 
        tft.print((int)openWeatherMapOneCallData.daily[1].tempMax);
        tft.print(" `C   "); 
        tft.setCursor(230, 190, 2);
        String foreDay1 = weekDays[((openWeatherMapOneCallData.daily[2].dt / 86400L) + 4 ) % 7];
        tft.println(foreDay1);
        tft.print("       ");
        printWeatherIconForeCast1(openWeatherMapOneCallData.daily[2].weatherId);
        tft.setCursor(205, 280, 2);
        tft.print((int)openWeatherMapOneCallData.daily[2].tempMin);
        tft.print(" `C"); 
        tft.print("/"); 
        tft.print((int)openWeatherMapOneCallData.daily[2].tempMax);
        tft.print(" `C     "); 
        tft.setCursor(380, 190, 2);
        String foreDay2 = weekDays[((openWeatherMapOneCallData.daily[3].dt / 86400L) + 4 ) % 7];
        tft.println(foreDay2);
        tft.print("       ");
        printWeatherIconForeCast2(openWeatherMapOneCallData.daily[3].weatherId);
        tft.setCursor(365, 280, 2);
        tft.print((int)openWeatherMapOneCallData.daily[3].tempMin);
        tft.print(" `C"); 
        tft.print("/"); 
        tft.print((int)openWeatherMapOneCallData.daily[3].tempMax);
        tft.print(" `C    "); 
        Serial.print("forecast futott");
      }
    }
  }
  startForeCast = false;

} 



  
void getWeather(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || startWeather == true) {
    previousMillis = currentMillis;
  OpenWeatherMapCurrentData data;
  client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  client.setMetric(IS_METRIC);
  client.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setCursor(10, 70, 4);
  tft.print(data.country.c_str());
  tft.setCursor(10, 100, 4);
  tft.print(data.cityName.c_str());
  tft.setCursor(10, 130, 4);
  tft.print(data.description.c_str());
  tft.print("                     ");
  tft.setCursor(320, 70, 4);
  tft.print((int)data.temp);
  tft.print(" `C");
  tft.print("    ");
  tft.setCursor(320, 100, 4);
  tft.print((int)data.tempMin);
  tft.print(" `C"); 
  tft.print(" / "); 
  tft.print((int)data.tempMax);
  tft.print(" `C   "); 
  drawBmp("/windicon.bmp", 315, 128);
  tft.setCursor(343, 130, 4);
  tft.print(data.windSpeed);tft.print(" km/h");
  
  drawBmp("/half.bmp",0, 170);
  //Serial.print(data.weatherId);
  
  int sunrise = ((data.sunrise  % 86400L) / 3600) % 24;
  int sunset = ((data.sunset  % 86400L) / 3600) % 24;

  if(timeClient.getHours() > sunset ||  timeClient.getHours() < sunrise ){
      printWeatherIconNight(data.weatherId);

  }else
  {
    printWeatherIcon(data.weatherId);
    
  }
  Serial.print("currentWeather futott");
  forecast();
  
  
  
startWeather = false;
}
}
 



void parsing(){
  //Use ArduinoJson Assistant for json generate
if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;  //Object of class HTTPClient
    http.useHTTP10(true);
    http.begin("http://192.168.0.129:8085/data.json");
    int httpCode = http.GET();
    
    //Check the returning code                                                                  
    if (httpCode == 200) {
      if (hwClear){
        tft.fillScreen(TFT_BLACK);
        hwClear = false;
        wClear = true;
      }
    
  // Parsing

      const size_t capacity = 82*JSON_ARRAY_SIZE(0) + 12*JSON_ARRAY_SIZE(1) + 5*JSON_ARRAY_SIZE(2) + 3*JSON_ARRAY_SIZE(3) + 3*JSON_ARRAY_SIZE(4) + JSON_ARRAY_SIZE(5) + JSON_ARRAY_SIZE(6) + 5*JSON_ARRAY_SIZE(7) + JSON_ARRAY_SIZE(10) + JSON_ARRAY_SIZE(14) + 114*JSON_OBJECT_SIZE(7) + 6660;
DynamicJsonDocument doc(capacity);


deserializeJson(doc, http.getStream(), DeserializationOption::NestingLimit(12));

JsonObject Children_0 = doc["Children"][0];
//int Children_0_id = Children_0["id"]; // 1
//const char* Children_0_Text = Children_0["Text"]; // "DESKTOP-RGO67HR"

JsonArray Children_0_Children = Children_0["Children"];

JsonObject Children_0_Children_0 = Children_0_Children[0];
//int Children_0_Children_0_id = Children_0_Children_0["id"]; // 2
//const char* Children_0_Children_0_Text = Children_0_Children_0["Text"]; // "MSI B360M MORTAR TITANIUM (MS-7B23)"

JsonObject Children_0_Children_0_Children_0 = Children_0_Children_0["Children"][0];
//int Children_0_Children_0_Children_0_id = Children_0_Children_0_Children_0["id"]; // 3
//const char* Children_0_Children_0_Children_0_Text = Children_0_Children_0_Children_0["Text"]; // "Nuvoton NCT6797D"

JsonArray Children_0_Children_0_Children_0_Children = Children_0_Children_0_Children_0["Children"];

JsonObject Children_0_Children_0_Children_0_Children_0 = Children_0_Children_0_Children_0_Children[0];
//int Children_0_Children_0_Children_0_Children_0_id = Children_0_Children_0_Children_0_Children_0["id"]; // 4
//const char* Children_0_Children_0_Children_0_Children_0_Text = Children_0_Children_0_Children_0_Children_0["Text"]; // "Voltages"

//JsonArray Children_0_Children_0_Children_0_Children_0_Children = Children_0_Children_0_Children_0_Children_0["Children"];

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_0 = Children_0_Children_0_Children_0_Children_0_Children[0];
//int Children_0_Children_0_Children_0_Children_0_Children_0_id = Children_0_Children_0_Children_0_Children_0_Children_0["id"]; // 5
//const char* Children_0_Children_0_Children_0_Children_0_Children_0_Text = Children_0_Children_0_Children_0_Children_0_Children_0["Text"]; // "CPU VCore"

//const char* Children_0_Children_0_Children_0_Children_0_Children_0_Min = Children_0_Children_0_Children_0_Children_0_Children_0["Min"]; // "0,720 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_0_Value = Children_0_Children_0_Children_0_Children_0_Children_0["Value"]; // "0,848 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_0_Max = Children_0_Children_0_Children_0_Children_0_Children_0["Max"]; // "1,008 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_0_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_1 = Children_0_Children_0_Children_0_Children_0_Children[1];
//int Children_0_Children_0_Children_0_Children_0_Children_1_id = Children_0_Children_0_Children_0_Children_0_Children_1["id"]; // 6
//const char* Children_0_Children_0_Children_0_Children_0_Children_1_Text = Children_0_Children_0_Children_0_Children_0_Children_1["Text"]; // "Voltage #2"

//const char* Children_0_Children_0_Children_0_Children_0_Children_1_Min = Children_0_Children_0_Children_0_Children_0_Children_1["Min"]; // "1,008 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_1_Value = Children_0_Children_0_Children_0_Children_0_Children_1["Value"]; // "1,008 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_1_Max = Children_0_Children_0_Children_0_Children_0_Children_1["Max"]; // "1,016 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_1_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_2 = Children_0_Children_0_Children_0_Children_0_Children[2];
//int Children_0_Children_0_Children_0_Children_0_Children_2_id = Children_0_Children_0_Children_0_Children_0_Children_2["id"]; // 7
//const char* Children_0_Children_0_Children_0_Children_0_Children_2_Text = Children_0_Children_0_Children_0_Children_0_Children_2["Text"]; // "AVCC"

//const char* Children_0_Children_0_Children_0_Children_0_Children_2_Min = Children_0_Children_0_Children_0_Children_0_Children_2["Min"]; // "3,344 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_2_Value = Children_0_Children_0_Children_0_Children_0_Children_2["Value"]; // "3,344 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_2_Max = Children_0_Children_0_Children_0_Children_0_Children_2["Max"]; // "3,344 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_2_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_3 = Children_0_Children_0_Children_0_Children_0_Children[3];
//int Children_0_Children_0_Children_0_Children_0_Children_3_id = Children_0_Children_0_Children_0_Children_0_Children_3["id"]; // 8
//const char* Children_0_Children_0_Children_0_Children_0_Children_3_Text = Children_0_Children_0_Children_0_Children_0_Children_3["Text"]; // "3VCC"

//const char* Children_0_Children_0_Children_0_Children_0_Children_3_Min = Children_0_Children_0_Children_0_Children_0_Children_3["Min"]; // "3,312 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_3_Value = Children_0_Children_0_Children_0_Children_0_Children_3["Value"]; // "3,328 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_3_Max = Children_0_Children_0_Children_0_Children_0_Children_3["Max"]; // "3,328 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_3_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_3["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_4 = Children_0_Children_0_Children_0_Children_0_Children[4];
//int Children_0_Children_0_Children_0_Children_0_Children_4_id = Children_0_Children_0_Children_0_Children_0_Children_4["id"]; // 9
//const char* Children_0_Children_0_Children_0_Children_0_Children_4_Text = Children_0_Children_0_Children_0_Children_0_Children_4["Text"]; // "Voltage #5"

//const char* Children_0_Children_0_Children_0_Children_0_Children_4_Min = Children_0_Children_0_Children_0_Children_0_Children_4["Min"]; // "1,000 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_4_Value = Children_0_Children_0_Children_0_Children_0_Children_4["Value"]; // "1,000 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_4_Max = Children_0_Children_0_Children_0_Children_0_Children_4["Max"]; // "1,008 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_4_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_4["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_5 = Children_0_Children_0_Children_0_Children_0_Children[5];
//int Children_0_Children_0_Children_0_Children_0_Children_5_id = Children_0_Children_0_Children_0_Children_0_Children_5["id"]; // 10
//const char* Children_0_Children_0_Children_0_Children_0_Children_5_Text = Children_0_Children_0_Children_0_Children_0_Children_5["Text"]; // "Voltage #6"

//const char* Children_0_Children_0_Children_0_Children_0_Children_5_Min = Children_0_Children_0_Children_0_Children_0_Children_5["Min"]; // "0,152 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_5_Value = Children_0_Children_0_Children_0_Children_0_Children_5["Value"]; // "0,152 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_5_Max = Children_0_Children_0_Children_0_Children_0_Children_5["Max"]; // "0,152 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_5_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_5["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_6 = Children_0_Children_0_Children_0_Children_0_Children[6];
//int Children_0_Children_0_Children_0_Children_0_Children_6_id = Children_0_Children_0_Children_0_Children_0_Children_6["id"]; // 11
//const char* Children_0_Children_0_Children_0_Children_0_Children_6_Text = Children_0_Children_0_Children_0_Children_0_Children_6["Text"]; // "Voltage #7"

//const char* Children_0_Children_0_Children_0_Children_0_Children_6_Min = Children_0_Children_0_Children_0_Children_0_Children_6["Min"]; // "0,944 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_6_Value = Children_0_Children_0_Children_0_Children_0_Children_6["Value"]; // "0,944 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_6_Max = Children_0_Children_0_Children_0_Children_0_Children_6["Max"]; // "0,952 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_6_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_6["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_7 = Children_0_Children_0_Children_0_Children_0_Children[7];
//int Children_0_Children_0_Children_0_Children_0_Children_7_id = Children_0_Children_0_Children_0_Children_0_Children_7["id"]; // 12
//const char* Children_0_Children_0_Children_0_Children_0_Children_7_Text = Children_0_Children_0_Children_0_Children_0_Children_7["Text"]; // "3VSB"

//const char* Children_0_Children_0_Children_0_Children_0_Children_7_Min = Children_0_Children_0_Children_0_Children_0_Children_7["Min"]; // "3,344 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_7_Value = Children_0_Children_0_Children_0_Children_0_Children_7["Value"]; // "3,344 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_7_Max = Children_0_Children_0_Children_0_Children_0_Children_7["Max"]; // "3,344 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_7_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_7["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_8 = Children_0_Children_0_Children_0_Children_0_Children[8];
//int Children_0_Children_0_Children_0_Children_0_Children_8_id = Children_0_Children_0_Children_0_Children_0_Children_8["id"]; // 13
//const char* Children_0_Children_0_Children_0_Children_0_Children_8_Text = Children_0_Children_0_Children_0_Children_0_Children_8["Text"]; // "VTT"

//const char* Children_0_Children_0_Children_0_Children_0_Children_8_Min = Children_0_Children_0_Children_0_Children_0_Children_8["Min"]; // "1,048 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_8_Value = Children_0_Children_0_Children_0_Children_0_Children_8["Value"]; // "1,056 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_8_Max = Children_0_Children_0_Children_0_Children_0_Children_8["Max"]; // "1,056 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_8_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_8["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_9 = Children_0_Children_0_Children_0_Children_0_Children[9];
//int Children_0_Children_0_Children_0_Children_0_Children_9_id = Children_0_Children_0_Children_0_Children_0_Children_9["id"]; // 14
//const char* Children_0_Children_0_Children_0_Children_0_Children_9_Text = Children_0_Children_0_Children_0_Children_0_Children_9["Text"]; // "Voltage #11"

//const char* Children_0_Children_0_Children_0_Children_0_Children_9_Min = Children_0_Children_0_Children_0_Children_0_Children_9["Min"]; // "1,048 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_9_Value = Children_0_Children_0_Children_0_Children_0_Children_9["Value"]; // "1,056 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_9_Max = Children_0_Children_0_Children_0_Children_0_Children_9["Max"]; // "1,056 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_9_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_9["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_10 = Children_0_Children_0_Children_0_Children_0_Children[10];
//int Children_0_Children_0_Children_0_Children_0_Children_10_id = Children_0_Children_0_Children_0_Children_0_Children_10["id"]; // 15
//const char* Children_0_Children_0_Children_0_Children_0_Children_10_Text = Children_0_Children_0_Children_0_Children_0_Children_10["Text"]; // "Voltage #12"

//const char* Children_0_Children_0_Children_0_Children_0_Children_10_Min = Children_0_Children_0_Children_0_Children_0_Children_10["Min"]; // "0,832 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_10_Value = Children_0_Children_0_Children_0_Children_0_Children_10["Value"]; // "0,856 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_10_Max = Children_0_Children_0_Children_0_Children_0_Children_10["Max"]; // "0,872 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_10_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_10["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_11 = Children_0_Children_0_Children_0_Children_0_Children[11];
//int Children_0_Children_0_Children_0_Children_0_Children_11_id = Children_0_Children_0_Children_0_Children_0_Children_11["id"]; // 16
//const char* Children_0_Children_0_Children_0_Children_0_Children_11_Text = Children_0_Children_0_Children_0_Children_0_Children_11["Text"]; // "Voltage #13"

//const char* Children_0_Children_0_Children_0_Children_0_Children_11_Min = Children_0_Children_0_Children_0_Children_0_Children_11["Min"]; // "1,048 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_11_Value = Children_0_Children_0_Children_0_Children_0_Children_11["Value"]; // "1,048 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_11_Max = Children_0_Children_0_Children_0_Children_0_Children_11["Max"]; // "1,048 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_11_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_11["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_12 = Children_0_Children_0_Children_0_Children_0_Children[12];
//int Children_0_Children_0_Children_0_Children_0_Children_12_id = Children_0_Children_0_Children_0_Children_0_Children_12["id"]; // 17
//const char* Children_0_Children_0_Children_0_Children_0_Children_12_Text = Children_0_Children_0_Children_0_Children_0_Children_12["Text"]; // "Voltage #14"

//const char* Children_0_Children_0_Children_0_Children_0_Children_12_Min = Children_0_Children_0_Children_0_Children_0_Children_12["Min"]; // "0,600 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_12_Value = Children_0_Children_0_Children_0_Children_0_Children_12["Value"]; // "0,608 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_12_Max = Children_0_Children_0_Children_0_Children_0_Children_12["Max"]; // "0,608 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_12_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_12["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_0_Children_13 = Children_0_Children_0_Children_0_Children_0_Children[13];
//int Children_0_Children_0_Children_0_Children_0_Children_13_id = Children_0_Children_0_Children_0_Children_0_Children_13["id"]; // 18
//const char* Children_0_Children_0_Children_0_Children_0_Children_13_Text = Children_0_Children_0_Children_0_Children_0_Children_13["Text"]; // "Voltage #15"

//const char* Children_0_Children_0_Children_0_Children_0_Children_13_Min = Children_0_Children_0_Children_0_Children_0_Children_13["Min"]; // "1,504 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_13_Value = Children_0_Children_0_Children_0_Children_0_Children_13["Value"]; // "1,504 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_13_Max = Children_0_Children_0_Children_0_Children_0_Children_13["Max"]; // "1,512 V"
//const char* Children_0_Children_0_Children_0_Children_0_Children_13_ImageURL = Children_0_Children_0_Children_0_Children_0_Children_13["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_0_Children_0_Children_0_Min = Children_0_Children_0_Children_0_Children_0["Min"]; // ""
//const char* Children_0_Children_0_Children_0_Children_0_Value = Children_0_Children_0_Children_0_Children_0["Value"]; // ""
//const char* Children_0_Children_0_Children_0_Children_0_Max = Children_0_Children_0_Children_0_Children_0["Max"]; // ""
//const char* Children_0_Children_0_Children_0_Children_0_ImageURL = Children_0_Children_0_Children_0_Children_0["ImageURL"]; // "images_icon/voltage.png"

JsonObject Children_0_Children_0_Children_0_Children_1 = Children_0_Children_0_Children_0_Children[1];
//int Children_0_Children_0_Children_0_Children_1_id = Children_0_Children_0_Children_0_Children_1["id"]; // 19
//const char* Children_0_Children_0_Children_0_Children_1_Text = Children_0_Children_0_Children_0_Children_1["Text"]; // "Temperatures"

//JsonArray Children_0_Children_0_Children_0_Children_1_Children = Children_0_Children_0_Children_0_Children_1["Children"];

//JsonObject Children_0_Children_0_Children_0_Children_1_Children_0 = Children_0_Children_0_Children_0_Children_1_Children[0];
//int Children_0_Children_0_Children_0_Children_1_Children_0_id = Children_0_Children_0_Children_0_Children_1_Children_0["id"]; // 20
//const char* Children_0_Children_0_Children_0_Children_1_Children_0_Text = Children_0_Children_0_Children_0_Children_1_Children_0["Text"]; // "CPU Core"

//const char* Children_0_Children_0_Children_0_Children_1_Children_0_Min = Children_0_Children_0_Children_0_Children_1_Children_0["Min"]; // "35,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_0_Value = Children_0_Children_0_Children_0_Children_1_Children_0["Value"]; // "37,5 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_0_Max = Children_0_Children_0_Children_0_Children_1_Children_0["Max"]; // "61,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_0_ImageURL = Children_0_Children_0_Children_0_Children_1_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_1_Children_1 = Children_0_Children_0_Children_0_Children_1_Children[1];
//int Children_0_Children_0_Children_0_Children_1_Children_1_id = Children_0_Children_0_Children_0_Children_1_Children_1["id"]; // 21
//const char* Children_0_Children_0_Children_0_Children_1_Children_1_Text = Children_0_Children_0_Children_0_Children_1_Children_1["Text"]; // "Temperature #1"

//const char* Children_0_Children_0_Children_0_Children_1_Children_1_Min = Children_0_Children_0_Children_0_Children_1_Children_1["Min"]; // "40,5 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_1_Value = Children_0_Children_0_Children_0_Children_1_Children_1["Value"]; // "42,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_1_Max = Children_0_Children_0_Children_0_Children_1_Children_1["Max"]; // "46,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_1_ImageURL = Children_0_Children_0_Children_0_Children_1_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_1_Children_2 = Children_0_Children_0_Children_0_Children_1_Children[2];
//int Children_0_Children_0_Children_0_Children_1_Children_2_id = Children_0_Children_0_Children_0_Children_1_Children_2["id"]; // 22
//const char* Children_0_Children_0_Children_0_Children_1_Children_2_Text = Children_0_Children_0_Children_0_Children_1_Children_2["Text"]; // "Temperature #2"

//const char* Children_0_Children_0_Children_0_Children_1_Children_2_Min = Children_0_Children_0_Children_0_Children_1_Children_2["Min"]; // "42,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_2_Value = Children_0_Children_0_Children_0_Children_1_Children_2["Value"]; // "43,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_2_Max = Children_0_Children_0_Children_0_Children_1_Children_2["Max"]; // "44,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_2_ImageURL = Children_0_Children_0_Children_0_Children_1_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_1_Children_3 = Children_0_Children_0_Children_0_Children_1_Children[3];
//int Children_0_Children_0_Children_0_Children_1_Children_3_id = Children_0_Children_0_Children_0_Children_1_Children_3["id"]; // 23
//const char* Children_0_Children_0_Children_0_Children_1_Children_3_Text = Children_0_Children_0_Children_0_Children_1_Children_3["Text"]; // "Temperature #3"

//const char* Children_0_Children_0_Children_0_Children_1_Children_3_Min = Children_0_Children_0_Children_0_Children_1_Children_3["Min"]; // "29,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_3_Value = Children_0_Children_0_Children_0_Children_1_Children_3["Value"]; // "29,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_3_Max = Children_0_Children_0_Children_0_Children_1_Children_3["Max"]; // "29,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_3_ImageURL = Children_0_Children_0_Children_0_Children_1_Children_3["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_1_Children_4 = Children_0_Children_0_Children_0_Children_1_Children[4];
//int Children_0_Children_0_Children_0_Children_1_Children_4_id = Children_0_Children_0_Children_0_Children_1_Children_4["id"]; // 24
//const char* Children_0_Children_0_Children_0_Children_1_Children_4_Text = Children_0_Children_0_Children_0_Children_1_Children_4["Text"]; // "Temperature #4"

//const char* Children_0_Children_0_Children_0_Children_1_Children_4_Min = Children_0_Children_0_Children_0_Children_1_Children_4["Min"]; // "23,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_4_Value = Children_0_Children_0_Children_0_Children_1_Children_4["Value"]; // "23,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_4_Max = Children_0_Children_0_Children_0_Children_1_Children_4["Max"]; // "23,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_4_ImageURL = Children_0_Children_0_Children_0_Children_1_Children_4["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_1_Children_5 = Children_0_Children_0_Children_0_Children_1_Children[5];
//int Children_0_Children_0_Children_0_Children_1_Children_5_id = Children_0_Children_0_Children_0_Children_1_Children_5["id"]; // 25
//const char* Children_0_Children_0_Children_0_Children_1_Children_5_Text = Children_0_Children_0_Children_0_Children_1_Children_5["Text"]; // "Temperature #5"

//const char* Children_0_Children_0_Children_0_Children_1_Children_5_Min = Children_0_Children_0_Children_0_Children_1_Children_5["Min"]; // "33,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_5_Value = Children_0_Children_0_Children_0_Children_1_Children_5["Value"]; // "34,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_5_Max = Children_0_Children_0_Children_0_Children_1_Children_5["Max"]; // "35,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_5_ImageURL = Children_0_Children_0_Children_0_Children_1_Children_5["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_1_Children_6 = Children_0_Children_0_Children_0_Children_1_Children[6];
//int Children_0_Children_0_Children_0_Children_1_Children_6_id = Children_0_Children_0_Children_0_Children_1_Children_6["id"]; // 26
//const char* Children_0_Children_0_Children_0_Children_1_Children_6_Text = Children_0_Children_0_Children_0_Children_1_Children_6["Text"]; // "Temperature #6"

//const char* Children_0_Children_0_Children_0_Children_1_Children_6_Min = Children_0_Children_0_Children_0_Children_1_Children_6["Min"]; // "-1,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_6_Value = Children_0_Children_0_Children_0_Children_1_Children_6["Value"]; // "-1,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_6_Max = Children_0_Children_0_Children_0_Children_1_Children_6["Max"]; // "-1,0 °C"
//const char* Children_0_Children_0_Children_0_Children_1_Children_6_ImageURL = Children_0_Children_0_Children_0_Children_1_Children_6["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_0_Children_0_Children_1_Min = Children_0_Children_0_Children_0_Children_1["Min"]; // ""
//const char* Children_0_Children_0_Children_0_Children_1_Value = Children_0_Children_0_Children_0_Children_1["Value"]; // ""
//const char* Children_0_Children_0_Children_0_Children_1_Max = Children_0_Children_0_Children_0_Children_1["Max"]; // ""
//const char* Children_0_Children_0_Children_0_Children_1_ImageURL = Children_0_Children_0_Children_0_Children_1["ImageURL"]; // "images_icon/temperature.png"

JsonObject Children_0_Children_0_Children_0_Children_2 = Children_0_Children_0_Children_0_Children[2];
//int Children_0_Children_0_Children_0_Children_2_id = Children_0_Children_0_Children_0_Children_2["id"]; // 27
//const char* Children_0_Children_0_Children_0_Children_2_Text = Children_0_Children_0_Children_0_Children_2["Text"]; // "Fans"

//JsonArray Children_0_Children_0_Children_0_Children_2_Children = Children_0_Children_0_Children_0_Children_2["Children"];

//JsonObject Children_0_Children_0_Children_0_Children_2_Children_0 = Children_0_Children_0_Children_0_Children_2_Children[0];
//int Children_0_Children_0_Children_0_Children_2_Children_0_id = Children_0_Children_0_Children_0_Children_2_Children_0["id"]; // 28
//const char* Children_0_Children_0_Children_0_Children_2_Children_0_Text = Children_0_Children_0_Children_0_Children_2_Children_0["Text"]; // "Fan #2"

//const char* Children_0_Children_0_Children_0_Children_2_Children_0_Min = Children_0_Children_0_Children_0_Children_2_Children_0["Min"]; // "2206 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_0_Value = Children_0_Children_0_Children_0_Children_2_Children_0["Value"]; // "2217 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_0_Max = Children_0_Children_0_Children_0_Children_2_Children_0["Max"]; // "2292 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_0_ImageURL = Children_0_Children_0_Children_0_Children_2_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_2_Children_1 = Children_0_Children_0_Children_0_Children_2_Children[1];
//int Children_0_Children_0_Children_0_Children_2_Children_1_id = Children_0_Children_0_Children_0_Children_2_Children_1["id"]; // 29
//const char* Children_0_Children_0_Children_0_Children_2_Children_1_Text = Children_0_Children_0_Children_0_Children_2_Children_1["Text"]; // "Fan #3"

//const char* Children_0_Children_0_Children_0_Children_2_Children_1_Min = Children_0_Children_0_Children_0_Children_2_Children_1["Min"]; // "550 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_1_Value = Children_0_Children_0_Children_0_Children_2_Children_1["Value"]; // "577 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_1_Max = Children_0_Children_0_Children_0_Children_2_Children_1["Max"]; // "1120 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_1_ImageURL = Children_0_Children_0_Children_0_Children_2_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_2_Children_2 = Children_0_Children_0_Children_0_Children_2_Children[2];
//int Children_0_Children_0_Children_0_Children_2_Children_2_id = Children_0_Children_0_Children_0_Children_2_Children_2["id"]; // 30
//const char* Children_0_Children_0_Children_0_Children_2_Children_2_Text = Children_0_Children_0_Children_0_Children_2_Children_2["Text"]; // "Fan #5"

//const char* Children_0_Children_0_Children_0_Children_2_Children_2_Min = Children_0_Children_0_Children_0_Children_2_Children_2["Min"]; // "1006 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_2_Value = Children_0_Children_0_Children_0_Children_2_Children_2["Value"]; // "1014 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_2_Max = Children_0_Children_0_Children_0_Children_2_Children_2["Max"]; // "2061 RPM"
//const char* Children_0_Children_0_Children_0_Children_2_Children_2_ImageURL = Children_0_Children_0_Children_0_Children_2_Children_2["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_0_Children_0_Children_2_Min = Children_0_Children_0_Children_0_Children_2["Min"]; // ""
//const char* Children_0_Children_0_Children_0_Children_2_Value = Children_0_Children_0_Children_0_Children_2["Value"]; // ""
//const char* Children_0_Children_0_Children_0_Children_2_Max = Children_0_Children_0_Children_0_Children_2["Max"]; // ""
//const char* Children_0_Children_0_Children_0_Children_2_ImageURL = Children_0_Children_0_Children_0_Children_2["ImageURL"]; // "images_icon/fan.png"

JsonObject Children_0_Children_0_Children_0_Children_3 = Children_0_Children_0_Children_0_Children[3];
//int Children_0_Children_0_Children_0_Children_3_id = Children_0_Children_0_Children_0_Children_3["id"]; // 31
//const char* Children_0_Children_0_Children_0_Children_3_Text = Children_0_Children_0_Children_0_Children_3["Text"]; // "Controls"

JsonArray Children_0_Children_0_Children_0_Children_3_Children = Children_0_Children_0_Children_0_Children_3["Children"];

JsonObject Children_0_Children_0_Children_0_Children_3_Children_0 = Children_0_Children_0_Children_0_Children_3_Children[0];
//int Children_0_Children_0_Children_0_Children_3_Children_0_id = Children_0_Children_0_Children_0_Children_3_Children_0["id"]; // 32
//const char* Children_0_Children_0_Children_0_Children_3_Children_0_Text = Children_0_Children_0_Children_0_Children_3_Children_0["Text"]; // "Fan Control #1"

//const char* Children_0_Children_0_Children_0_Children_3_Children_0_Min = Children_0_Children_0_Children_0_Children_3_Children_0["Min"]; // "100,0 %"
String Children_0_Children_0_Children_0_Children_3_Children_0_Value = Children_0_Children_0_Children_0_Children_3_Children_0["Value"]; // "100,0 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_0_Max = Children_0_Children_0_Children_0_Children_3_Children_0["Max"]; // "100,0 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_0_ImageURL = Children_0_Children_0_Children_0_Children_3_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_3_Children_1 = Children_0_Children_0_Children_0_Children_3_Children[1];
//int Children_0_Children_0_Children_0_Children_3_Children_1_id = Children_0_Children_0_Children_0_Children_3_Children_1["id"]; // 33
//const char* Children_0_Children_0_Children_0_Children_3_Children_1_Text = Children_0_Children_0_Children_0_Children_3_Children_1["Text"]; // "Fan Control #2"

//const char* Children_0_Children_0_Children_0_Children_3_Children_1_Min = Children_0_Children_0_Children_0_Children_3_Children_1["Min"]; // "12,9 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_1_Value = Children_0_Children_0_Children_0_Children_3_Children_1["Value"]; // "12,9 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_1_Max = Children_0_Children_0_Children_0_Children_3_Children_1["Max"]; // "47,5 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_1_ImageURL = Children_0_Children_0_Children_0_Children_3_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_3_Children_2 = Children_0_Children_0_Children_0_Children_3_Children[2];
//int Children_0_Children_0_Children_0_Children_3_Children_2_id = Children_0_Children_0_Children_0_Children_3_Children_2["id"]; // 34
//const char* Children_0_Children_0_Children_0_Children_3_Children_2_Text = Children_0_Children_0_Children_0_Children_3_Children_2["Text"]; // "Fan Control #3"

//const char* Children_0_Children_0_Children_0_Children_3_Children_2_Min = Children_0_Children_0_Children_0_Children_3_Children_2["Min"]; // "24,7 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_2_Value = Children_0_Children_0_Children_0_Children_3_Children_2["Value"]; // "24,7 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_2_Max = Children_0_Children_0_Children_0_Children_3_Children_2["Max"]; // "54,9 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_2_ImageURL = Children_0_Children_0_Children_0_Children_3_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_3_Children_3 = Children_0_Children_0_Children_0_Children_3_Children[3];
//int Children_0_Children_0_Children_0_Children_3_Children_3_id = Children_0_Children_0_Children_0_Children_3_Children_3["id"]; // 35
//const char* Children_0_Children_0_Children_0_Children_3_Children_3_Text = Children_0_Children_0_Children_0_Children_3_Children_3["Text"]; // "Fan Control #4"

//const char* Children_0_Children_0_Children_0_Children_3_Children_3_Min = Children_0_Children_0_Children_0_Children_3_Children_3["Min"]; // "57,6 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_3_Value = Children_0_Children_0_Children_0_Children_3_Children_3["Value"]; // "57,6 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_3_Max = Children_0_Children_0_Children_0_Children_3_Children_3["Max"]; // "57,6 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_3_ImageURL = Children_0_Children_0_Children_0_Children_3_Children_3["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_3_Children_4 = Children_0_Children_0_Children_0_Children_3_Children[4];
//int Children_0_Children_0_Children_0_Children_3_Children_4_id = Children_0_Children_0_Children_0_Children_3_Children_4["id"]; // 36
//const char* Children_0_Children_0_Children_0_Children_3_Children_4_Text = Children_0_Children_0_Children_0_Children_3_Children_4["Text"]; // "Fan Control #5"

//const char* Children_0_Children_0_Children_0_Children_3_Children_4_Min = Children_0_Children_0_Children_0_Children_3_Children_4["Min"]; // "57,6 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_4_Value = Children_0_Children_0_Children_0_Children_3_Children_4["Value"]; // "57,6 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_4_Max = Children_0_Children_0_Children_0_Children_3_Children_4["Max"]; // "57,6 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_4_ImageURL = Children_0_Children_0_Children_0_Children_3_Children_4["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_3_Children_5 = Children_0_Children_0_Children_0_Children_3_Children[5];
//int Children_0_Children_0_Children_0_Children_3_Children_5_id = Children_0_Children_0_Children_0_Children_3_Children_5["id"]; // 37
//const char* Children_0_Children_0_Children_0_Children_3_Children_5_Text = Children_0_Children_0_Children_0_Children_3_Children_5["Text"]; // "Fan Control #6"

//const char* Children_0_Children_0_Children_0_Children_3_Children_5_Min = Children_0_Children_0_Children_0_Children_3_Children_5["Min"]; // "49,8 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_5_Value = Children_0_Children_0_Children_0_Children_3_Children_5["Value"]; // "49,8 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_5_Max = Children_0_Children_0_Children_0_Children_3_Children_5["Max"]; // "49,8 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_5_ImageURL = Children_0_Children_0_Children_0_Children_3_Children_5["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_0_Children_0_Children_3_Children_6 = Children_0_Children_0_Children_0_Children_3_Children[6];
//int Children_0_Children_0_Children_0_Children_3_Children_6_id = Children_0_Children_0_Children_0_Children_3_Children_6["id"]; // 38
//const char* Children_0_Children_0_Children_0_Children_3_Children_6_Text = Children_0_Children_0_Children_0_Children_3_Children_6["Text"]; // "Fan Control #7"

//const char* Children_0_Children_0_Children_0_Children_3_Children_6_Min = Children_0_Children_0_Children_0_Children_3_Children_6["Min"]; // "49,8 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_6_Value = Children_0_Children_0_Children_0_Children_3_Children_6["Value"]; // "49,8 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_6_Max = Children_0_Children_0_Children_0_Children_3_Children_6["Max"]; // "49,8 %"
//const char* Children_0_Children_0_Children_0_Children_3_Children_6_ImageURL = Children_0_Children_0_Children_0_Children_3_Children_6["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_0_Children_0_Children_3_Min = Children_0_Children_0_Children_0_Children_3["Min"]; // ""
//const char* Children_0_Children_0_Children_0_Children_3_Value = Children_0_Children_0_Children_0_Children_3["Value"]; // ""
//const char* Children_0_Children_0_Children_0_Children_3_Max = Children_0_Children_0_Children_0_Children_3["Max"]; // ""
//const char* Children_0_Children_0_Children_0_Children_3_ImageURL = Children_0_Children_0_Children_0_Children_3["ImageURL"]; // "images_icon/control.png"

//const char* Children_0_Children_0_Children_0_Min = Children_0_Children_0_Children_0["Min"]; // ""
//const char* Children_0_Children_0_Children_0_Value = Children_0_Children_0_Children_0["Value"]; // ""
//const char* Children_0_Children_0_Children_0_Max = Children_0_Children_0_Children_0["Max"]; // ""
//const char* Children_0_Children_0_Children_0_ImageURL = Children_0_Children_0_Children_0["ImageURL"]; // "images_icon/chip.png"

//const char* Children_0_Children_0_Min = Children_0_Children_0["Min"]; // ""
//const char* Children_0_Children_0_Value = Children_0_Children_0["Value"]; // ""
//const char* Children_0_Children_0_Max = Children_0_Children_0["Max"]; // ""
//const char* Children_0_Children_0_ImageURL = Children_0_Children_0["ImageURL"]; // "images_icon/mainboard.png"

JsonObject Children_0_Children_1 = Children_0_Children[1];
//int Children_0_Children_1_id = Children_0_Children_1["id"]; // 39
//const char* Children_0_Children_1_Text = Children_0_Children_1["Text"]; // "Intel Core i7-8700"

JsonArray Children_0_Children_1_Children = Children_0_Children_1["Children"];

JsonObject Children_0_Children_1_Children_0 = Children_0_Children_1_Children[0];
//int Children_0_Children_1_Children_0_id = Children_0_Children_1_Children_0["id"]; // 40
//const char* Children_0_Children_1_Children_0_Text = Children_0_Children_1_Children_0["Text"]; // "Clocks"

//JsonArray Children_0_Children_1_Children_0_Children = Children_0_Children_1_Children_0["Children"];

//JsonObject Children_0_Children_1_Children_0_Children_0 = Children_0_Children_1_Children_0_Children[0];
//int Children_0_Children_1_Children_0_Children_0_id = Children_0_Children_1_Children_0_Children_0["id"]; // 41
//const char* Children_0_Children_1_Children_0_Children_0_Text = Children_0_Children_1_Children_0_Children_0["Text"]; // "Bus Speed"

//const char* Children_0_Children_1_Children_0_Children_0_Min = Children_0_Children_1_Children_0_Children_0["Min"]; // "99,8 MHz"
//const char* Children_0_Children_1_Children_0_Children_0_Value = Children_0_Children_1_Children_0_Children_0["Value"]; // "99,8 MHz"
//const char* Children_0_Children_1_Children_0_Children_0_Max = Children_0_Children_1_Children_0_Children_0["Max"]; // "99,8 MHz"
//const char* Children_0_Children_1_Children_0_Children_0_ImageURL = Children_0_Children_1_Children_0_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_0_Children_1 = Children_0_Children_1_Children_0_Children[1];
//int Children_0_Children_1_Children_0_Children_1_id = Children_0_Children_1_Children_0_Children_1["id"]; // 42
//const char* Children_0_Children_1_Children_0_Children_1_Text = Children_0_Children_1_Children_0_Children_1["Text"]; // "CPU Core #1"

//const char* Children_0_Children_1_Children_0_Children_1_Min = Children_0_Children_1_Children_0_Children_1["Min"]; // "798,0 MHz"
//const char* Children_0_Children_1_Children_0_Children_1_Value = Children_0_Children_1_Children_0_Children_1["Value"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_1_Max = Children_0_Children_1_Children_0_Children_1["Max"]; // "4289,4 MHz"
//const char* Children_0_Children_1_Children_0_Children_1_ImageURL = Children_0_Children_1_Children_0_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_0_Children_2 = Children_0_Children_1_Children_0_Children[2];
//int Children_0_Children_1_Children_0_Children_2_id = Children_0_Children_1_Children_0_Children_2["id"]; // 43
//const char* Children_0_Children_1_Children_0_Children_2_Text = Children_0_Children_1_Children_0_Children_2["Text"]; // "CPU Core #2"

//const char* Children_0_Children_1_Children_0_Children_2_Min = Children_0_Children_1_Children_0_Children_2["Min"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_2_Value = Children_0_Children_1_Children_0_Children_2["Value"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_2_Max = Children_0_Children_1_Children_0_Children_2["Max"]; // "4289,4 MHz"
//const char* Children_0_Children_1_Children_0_Children_2_ImageURL = Children_0_Children_1_Children_0_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_0_Children_3 = Children_0_Children_1_Children_0_Children[3];
//int Children_0_Children_1_Children_0_Children_3_id = Children_0_Children_1_Children_0_Children_3["id"]; // 44
//const char* Children_0_Children_1_Children_0_Children_3_Text = Children_0_Children_1_Children_0_Children_3["Text"]; // "CPU Core #3"

//const char* Children_0_Children_1_Children_0_Children_3_Min = Children_0_Children_1_Children_0_Children_3["Min"]; // "798,0 MHz"
//const char* Children_0_Children_1_Children_0_Children_3_Value = Children_0_Children_1_Children_0_Children_3["Value"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_3_Max = Children_0_Children_1_Children_0_Children_3["Max"]; // "4289,4 MHz"
//const char* Children_0_Children_1_Children_0_Children_3_ImageURL = Children_0_Children_1_Children_0_Children_3["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_0_Children_4 = Children_0_Children_1_Children_0_Children[4];
//int Children_0_Children_1_Children_0_Children_4_id = Children_0_Children_1_Children_0_Children_4["id"]; // 45
//const char* Children_0_Children_1_Children_0_Children_4_Text = Children_0_Children_1_Children_0_Children_4["Text"]; // "CPU Core #4"

//const char* Children_0_Children_1_Children_0_Children_4_Min = Children_0_Children_1_Children_0_Children_4["Min"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_4_Value = Children_0_Children_1_Children_0_Children_4["Value"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_4_Max = Children_0_Children_1_Children_0_Children_4["Max"]; // "4289,4 MHz"
//const char* Children_0_Children_1_Children_0_Children_4_ImageURL = Children_0_Children_1_Children_0_Children_4["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_0_Children_5 = Children_0_Children_1_Children_0_Children[5];
//int Children_0_Children_1_Children_0_Children_5_id = Children_0_Children_1_Children_0_Children_5["id"]; // 46
//const char* Children_0_Children_1_Children_0_Children_5_Text = Children_0_Children_1_Children_0_Children_5["Text"]; // "CPU Core #5"

//const char* Children_0_Children_1_Children_0_Children_5_Min = Children_0_Children_1_Children_0_Children_5["Min"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_5_Value = Children_0_Children_1_Children_0_Children_5["Value"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_5_Max = Children_0_Children_1_Children_0_Children_5["Max"]; // "4289,4 MHz"
//const char* Children_0_Children_1_Children_0_Children_5_ImageURL = Children_0_Children_1_Children_0_Children_5["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_0_Children_6 = Children_0_Children_1_Children_0_Children[6];
//int Children_0_Children_1_Children_0_Children_6_id = Children_0_Children_1_Children_0_Children_6["id"]; // 47
//const char* Children_0_Children_1_Children_0_Children_6_Text = Children_0_Children_1_Children_0_Children_6["Text"]; // "CPU Core #6"

//const char* Children_0_Children_1_Children_0_Children_6_Min = Children_0_Children_1_Children_0_Children_6["Min"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_6_Value = Children_0_Children_1_Children_0_Children_6["Value"]; // "4289,3 MHz"
//const char* Children_0_Children_1_Children_0_Children_6_Max = Children_0_Children_1_Children_0_Children_6["Max"]; // "4289,4 MHz"
//const char* Children_0_Children_1_Children_0_Children_6_ImageURL = Children_0_Children_1_Children_0_Children_6["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_1_Children_0_Min = Children_0_Children_1_Children_0["Min"]; // ""
//const char* Children_0_Children_1_Children_0_Value = Children_0_Children_1_Children_0["Value"]; // ""
//const char* Children_0_Children_1_Children_0_Max = Children_0_Children_1_Children_0["Max"]; // ""
//const char* Children_0_Children_1_Children_0_ImageURL = Children_0_Children_1_Children_0["ImageURL"]; // "images_icon/clock.png"

JsonObject Children_0_Children_1_Children_1 = Children_0_Children_1_Children[1];
//int Children_0_Children_1_Children_1_id = Children_0_Children_1_Children_1["id"]; // 48
//const char* Children_0_Children_1_Children_1_Text = Children_0_Children_1_Children_1["Text"]; // "Temperatures"

JsonArray Children_0_Children_1_Children_1_Children = Children_0_Children_1_Children_1["Children"];

JsonObject Children_0_Children_1_Children_1_Children_0 = Children_0_Children_1_Children_1_Children[0];
//int Children_0_Children_1_Children_1_Children_0_id = Children_0_Children_1_Children_1_Children_0["id"]; // 49
//const char* Children_0_Children_1_Children_1_Children_0_Text = Children_0_Children_1_Children_1_Children_0["Text"]; // "CPU Core #1"

//const char* Children_0_Children_1_Children_1_Children_0_Min = Children_0_Children_1_Children_1_Children_0["Min"]; // "33,0 °C"
String Children_0_Children_1_Children_1_Children_0_Value = Children_0_Children_1_Children_1_Children_0["Value"]; // "37,0 °C"
//const char* Children_0_Children_1_Children_1_Children_0_Max = Children_0_Children_1_Children_1_Children_0["Max"]; // "58,0 °C"
//const char* Children_0_Children_1_Children_1_Children_0_ImageURL = Children_0_Children_1_Children_1_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_1_Children_1 = Children_0_Children_1_Children_1_Children[1];
//int Children_0_Children_1_Children_1_Children_1_id = Children_0_Children_1_Children_1_Children_1["id"]; // 50
//const char* Children_0_Children_1_Children_1_Children_1_Text = Children_0_Children_1_Children_1_Children_1["Text"]; // "CPU Core #2"

//const char* Children_0_Children_1_Children_1_Children_1_Min = Children_0_Children_1_Children_1_Children_1["Min"]; // "33,0 °C"
//const char* Children_0_Children_1_Children_1_Children_1_Value = Children_0_Children_1_Children_1_Children_1["Value"]; // "39,0 °C"
//const char* Children_0_Children_1_Children_1_Children_1_Max = Children_0_Children_1_Children_1_Children_1["Max"]; // "62,0 °C"
//const char* Children_0_Children_1_Children_1_Children_1_ImageURL = Children_0_Children_1_Children_1_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_1_Children_2 = Children_0_Children_1_Children_1_Children[2];
//int Children_0_Children_1_Children_1_Children_2_id = Children_0_Children_1_Children_1_Children_2["id"]; // 51
//const char* Children_0_Children_1_Children_1_Children_2_Text = Children_0_Children_1_Children_1_Children_2["Text"]; // "CPU Core #3"

//const char* Children_0_Children_1_Children_1_Children_2_Min = Children_0_Children_1_Children_1_Children_2["Min"]; // "33,0 °C"
//const char* Children_0_Children_1_Children_1_Children_2_Value = Children_0_Children_1_Children_1_Children_2["Value"]; // "37,0 °C"
//const char* Children_0_Children_1_Children_1_Children_2_Max = Children_0_Children_1_Children_1_Children_2["Max"]; // "62,0 °C"
//const char* Children_0_Children_1_Children_1_Children_2_ImageURL = Children_0_Children_1_Children_1_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_1_Children_3 = Children_0_Children_1_Children_1_Children[3];
//int Children_0_Children_1_Children_1_Children_3_id = Children_0_Children_1_Children_1_Children_3["id"]; // 52
//const char* Children_0_Children_1_Children_1_Children_3_Text = Children_0_Children_1_Children_1_Children_3["Text"]; // "CPU Core #4"

//const char* Children_0_Children_1_Children_1_Children_3_Min = Children_0_Children_1_Children_1_Children_3["Min"]; // "31,0 °C"
//const char* Children_0_Children_1_Children_1_Children_3_Value = Children_0_Children_1_Children_1_Children_3["Value"]; // "36,0 °C"
//const char* Children_0_Children_1_Children_1_Children_3_Max = Children_0_Children_1_Children_1_Children_3["Max"]; // "61,0 °C"
//const char* Children_0_Children_1_Children_1_Children_3_ImageURL = Children_0_Children_1_Children_1_Children_3["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_1_Children_4 = Children_0_Children_1_Children_1_Children[4];
//int Children_0_Children_1_Children_1_Children_4_id = Children_0_Children_1_Children_1_Children_4["id"]; // 53
//const char* Children_0_Children_1_Children_1_Children_4_Text = Children_0_Children_1_Children_1_Children_4["Text"]; // "CPU Core #5"

//const char* Children_0_Children_1_Children_1_Children_4_Min = Children_0_Children_1_Children_1_Children_4["Min"]; // "31,0 °C"
//const char* Children_0_Children_1_Children_1_Children_4_Value = Children_0_Children_1_Children_1_Children_4["Value"]; // "35,0 °C"
//const char* Children_0_Children_1_Children_1_Children_4_Max = Children_0_Children_1_Children_1_Children_4["Max"]; // "60,0 °C"
//const char* Children_0_Children_1_Children_1_Children_4_ImageURL = Children_0_Children_1_Children_1_Children_4["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_1_Children_5 = Children_0_Children_1_Children_1_Children[5];
//int Children_0_Children_1_Children_1_Children_5_id = Children_0_Children_1_Children_1_Children_5["id"]; // 54
//const char* Children_0_Children_1_Children_1_Children_5_Text = Children_0_Children_1_Children_1_Children_5["Text"]; // "CPU Core #6"

//const char* Children_0_Children_1_Children_1_Children_5_Min = Children_0_Children_1_Children_1_Children_5["Min"]; // "32,0 °C"
//const char* Children_0_Children_1_Children_1_Children_5_Value = Children_0_Children_1_Children_1_Children_5["Value"]; // "41,0 °C"
//const char* Children_0_Children_1_Children_1_Children_5_Max = Children_0_Children_1_Children_1_Children_5["Max"]; // "62,0 °C"
//const char* Children_0_Children_1_Children_1_Children_5_ImageURL = Children_0_Children_1_Children_1_Children_5["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_1_Children_6 = Children_0_Children_1_Children_1_Children[6];
//int Children_0_Children_1_Children_1_Children_6_id = Children_0_Children_1_Children_1_Children_6["id"]; // 55
//const char* Children_0_Children_1_Children_1_Children_6_Text = Children_0_Children_1_Children_1_Children_6["Text"]; // "CPU Package"

//const char* Children_0_Children_1_Children_1_Children_6_Min = Children_0_Children_1_Children_1_Children_6["Min"]; // "34,0 °C"
//const char* Children_0_Children_1_Children_1_Children_6_Value = Children_0_Children_1_Children_1_Children_6["Value"]; // "40,0 °C"
//const char* Children_0_Children_1_Children_1_Children_6_Max = Children_0_Children_1_Children_1_Children_6["Max"]; // "62,0 °C"
//const char* Children_0_Children_1_Children_1_Children_6_ImageURL = Children_0_Children_1_Children_1_Children_6["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_1_Children_1_Min = Children_0_Children_1_Children_1["Min"]; // ""
//const char* Children_0_Children_1_Children_1_Value = Children_0_Children_1_Children_1["Value"]; // ""
//const char* Children_0_Children_1_Children_1_Max = Children_0_Children_1_Children_1["Max"]; // ""
//const char* Children_0_Children_1_Children_1_ImageURL = Children_0_Children_1_Children_1["ImageURL"]; // "images_icon/temperature.png"

JsonObject Children_0_Children_1_Children_2 = Children_0_Children_1_Children[2];
//int Children_0_Children_1_Children_2_id = Children_0_Children_1_Children_2["id"]; // 56
//const char* Children_0_Children_1_Children_2_Text = Children_0_Children_1_Children_2["Text"]; // "Load"

JsonArray Children_0_Children_1_Children_2_Children = Children_0_Children_1_Children_2["Children"];

JsonObject Children_0_Children_1_Children_2_Children_0 = Children_0_Children_1_Children_2_Children[0];
//int Children_0_Children_1_Children_2_Children_0_id = Children_0_Children_1_Children_2_Children_0["id"]; // 57
//const char* Children_0_Children_1_Children_2_Children_0_Text = Children_0_Children_1_Children_2_Children_0["Text"]; // "CPU Total"

//const char* Children_0_Children_1_Children_2_Children_0_Min = Children_0_Children_1_Children_2_Children_0["Min"]; // "0,1 %"
String Children_0_Children_1_Children_2_Children_0_Value = Children_0_Children_1_Children_2_Children_0["Value"]; 
ringMeter(Children_0_Children_1_Children_2_Children_0_Value.toInt(),0,100, 30,25,52," Load",GREEN2RED);
//const char* Children_0_Children_1_Children_2_Children_0_Max = Children_0_Children_1_Children_2_Children_0["Max"]; // "100,0 %"
//const char* Children_0_Children_1_Children_2_Children_0_ImageURL = Children_0_Children_1_Children_2_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_2_Children_1 = Children_0_Children_1_Children_2_Children[1];
//int Children_0_Children_1_Children_2_Children_1_id = Children_0_Children_1_Children_2_Children_1["id"]; // 58
//const char* Children_0_Children_1_Children_2_Children_1_Text = Children_0_Children_1_Children_2_Children_1["Text"]; // "CPU Core #1"

//const char* Children_0_Children_1_Children_2_Children_1_Min = Children_0_Children_1_Children_2_Children_1["Min"]; // "0,0 %"
//const char* Children_0_Children_1_Children_2_Children_1_Value = Children_0_Children_1_Children_2_Children_1["Value"]; // "6,2 %"
//const char* Children_0_Children_1_Children_2_Children_1_Max = Children_0_Children_1_Children_2_Children_1["Max"]; // "100,0 %"
//const char* Children_0_Children_1_Children_2_Children_1_ImageURL = Children_0_Children_1_Children_2_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_2_Children_2 = Children_0_Children_1_Children_2_Children[2];
//int Children_0_Children_1_Children_2_Children_2_id = Children_0_Children_1_Children_2_Children_2["id"]; // 59
//const char* Children_0_Children_1_Children_2_Children_2_Text = Children_0_Children_1_Children_2_Children_2["Text"]; // "CPU Core #2"

//const char* Children_0_Children_1_Children_2_Children_2_Min = Children_0_Children_1_Children_2_Children_2["Min"]; // "0,0 %"
//const char* Children_0_Children_1_Children_2_Children_2_Value = Children_0_Children_1_Children_2_Children_2["Value"]; // "13,8 %"
//const char* Children_0_Children_1_Children_2_Children_2_Max = Children_0_Children_1_Children_2_Children_2["Max"]; // "100,0 %"
//const char* Children_0_Children_1_Children_2_Children_2_ImageURL = Children_0_Children_1_Children_2_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_2_Children_3 = Children_0_Children_1_Children_2_Children[3];
//int Children_0_Children_1_Children_2_Children_3_id = Children_0_Children_1_Children_2_Children_3["id"]; // 60
//const char* Children_0_Children_1_Children_2_Children_3_Text = Children_0_Children_1_Children_2_Children_3["Text"]; // "CPU Core #3"

//const char* Children_0_Children_1_Children_2_Children_3_Min = Children_0_Children_1_Children_2_Children_3["Min"]; // "0,0 %"
//const char* Children_0_Children_1_Children_2_Children_3_Value = Children_0_Children_1_Children_2_Children_3["Value"]; // "3,8 %"
//const char* Children_0_Children_1_Children_2_Children_3_Max = Children_0_Children_1_Children_2_Children_3["Max"]; // "100,0 %"
//const char* Children_0_Children_1_Children_2_Children_3_ImageURL = Children_0_Children_1_Children_2_Children_3["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_2_Children_4 = Children_0_Children_1_Children_2_Children[4];
//int Children_0_Children_1_Children_2_Children_4_id = Children_0_Children_1_Children_2_Children_4["id"]; // 61
//const char* Children_0_Children_1_Children_2_Children_4_Text = Children_0_Children_1_Children_2_Children_4["Text"]; // "CPU Core #4"

//const char* Children_0_Children_1_Children_2_Children_4_Min = Children_0_Children_1_Children_2_Children_4["Min"]; // "0,0 %"
//const char* Children_0_Children_1_Children_2_Children_4_Value = Children_0_Children_1_Children_2_Children_4["Value"]; // "5,4 %"
//const char* Children_0_Children_1_Children_2_Children_4_Max = Children_0_Children_1_Children_2_Children_4["Max"]; // "100,0 %"
//const char* Children_0_Children_1_Children_2_Children_4_ImageURL = Children_0_Children_1_Children_2_Children_4["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_2_Children_5 = Children_0_Children_1_Children_2_Children[5];
//int Children_0_Children_1_Children_2_Children_5_id = Children_0_Children_1_Children_2_Children_5["id"]; // 62
//const char* Children_0_Children_1_Children_2_Children_5_Text = Children_0_Children_1_Children_2_Children_5["Text"]; // "CPU Core #5"

//const char* Children_0_Children_1_Children_2_Children_5_Min = Children_0_Children_1_Children_2_Children_5["Min"]; // "0,0 %"
//const char* Children_0_Children_1_Children_2_Children_5_Value = Children_0_Children_1_Children_2_Children_5["Value"]; // "6,2 %"
//const char* Children_0_Children_1_Children_2_Children_5_Max = Children_0_Children_1_Children_2_Children_5["Max"]; // "100,0 %"
//const char* Children_0_Children_1_Children_2_Children_5_ImageURL = Children_0_Children_1_Children_2_Children_5["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_2_Children_6 = Children_0_Children_1_Children_2_Children[6];
//int Children_0_Children_1_Children_2_Children_6_id = Children_0_Children_1_Children_2_Children_6["id"]; // 63
//const char* Children_0_Children_1_Children_2_Children_6_Text = Children_0_Children_1_Children_2_Children_6["Text"]; // "CPU Core #6"

//const char* Children_0_Children_1_Children_2_Children_6_Min = Children_0_Children_1_Children_2_Children_6["Min"]; // "0,0 %"
//const char* Children_0_Children_1_Children_2_Children_6_Value = Children_0_Children_1_Children_2_Children_6["Value"]; // "6,9 %"
//const char* Children_0_Children_1_Children_2_Children_6_Max = Children_0_Children_1_Children_2_Children_6["Max"]; // "100,0 %"
//const char* Children_0_Children_1_Children_2_Children_6_ImageURL = Children_0_Children_1_Children_2_Children_6["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_1_Children_2_Min = Children_0_Children_1_Children_2["Min"]; // ""
//const char* Children_0_Children_1_Children_2_Value = Children_0_Children_1_Children_2["Value"]; // ""
//const char* Children_0_Children_1_Children_2_Max = Children_0_Children_1_Children_2["Max"]; // ""
//const char* Children_0_Children_1_Children_2_ImageURL = Children_0_Children_1_Children_2["ImageURL"]; // "images_icon/load.png"

JsonObject Children_0_Children_1_Children_3 = Children_0_Children_1_Children[3];
//int Children_0_Children_1_Children_3_id = Children_0_Children_1_Children_3["id"]; // 64
//const char* Children_0_Children_1_Children_3_Text = Children_0_Children_1_Children_3["Text"]; // "Powers"

//JsonArray Children_0_Children_1_Children_3_Children = Children_0_Children_1_Children_3["Children"];

//JsonObject Children_0_Children_1_Children_3_Children_0 = Children_0_Children_1_Children_3_Children[0];
//int Children_0_Children_1_Children_3_Children_0_id = Children_0_Children_1_Children_3_Children_0["id"]; // 65
//const char* Children_0_Children_1_Children_3_Children_0_Text = Children_0_Children_1_Children_3_Children_0["Text"]; // "CPU Package"

//const char* Children_0_Children_1_Children_3_Children_0_Min = Children_0_Children_1_Children_3_Children_0["Min"]; // "7,3 W"
//const char* Children_0_Children_1_Children_3_Children_0_Value = Children_0_Children_1_Children_3_Children_0["Value"]; // "17,2 W"
//const char* Children_0_Children_1_Children_3_Children_0_Max = Children_0_Children_1_Children_3_Children_0["Max"]; // "75,8 W"
//const char* Children_0_Children_1_Children_3_Children_0_ImageURL = Children_0_Children_1_Children_3_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_3_Children_1 = Children_0_Children_1_Children_3_Children[1];
//int Children_0_Children_1_Children_3_Children_1_id = Children_0_Children_1_Children_3_Children_1["id"]; // 66
//const char* Children_0_Children_1_Children_3_Children_1_Text = Children_0_Children_1_Children_3_Children_1["Text"]; // "CPU Cores"

//const char* Children_0_Children_1_Children_3_Children_1_Min = Children_0_Children_1_Children_3_Children_1["Min"]; // "5,8 W"
//const char* Children_0_Children_1_Children_3_Children_1_Value = Children_0_Children_1_Children_3_Children_1["Value"]; // "15,7 W"
//const char* Children_0_Children_1_Children_3_Children_1_Max = Children_0_Children_1_Children_3_Children_1["Max"]; // "73,7 W"
//const char* Children_0_Children_1_Children_3_Children_1_ImageURL = Children_0_Children_1_Children_3_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_3_Children_2 = Children_0_Children_1_Children_3_Children[2];
//int Children_0_Children_1_Children_3_Children_2_id = Children_0_Children_1_Children_3_Children_2["id"]; // 67
//const char* Children_0_Children_1_Children_3_Children_2_Text = Children_0_Children_1_Children_3_Children_2["Text"]; // "CPU Graphics"

//const char* Children_0_Children_1_Children_3_Children_2_Min = Children_0_Children_1_Children_3_Children_2["Min"]; // "0,0 W"
//const char* Children_0_Children_1_Children_3_Children_2_Value = Children_0_Children_1_Children_3_Children_2["Value"]; // "0,0 W"
//const char* Children_0_Children_1_Children_3_Children_2_Max = Children_0_Children_1_Children_3_Children_2["Max"]; // "0,0 W"
//const char* Children_0_Children_1_Children_3_Children_2_ImageURL = Children_0_Children_1_Children_3_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_1_Children_3_Children_3 = Children_0_Children_1_Children_3_Children[3];
//int Children_0_Children_1_Children_3_Children_3_id = Children_0_Children_1_Children_3_Children_3["id"]; // 68
//const char* Children_0_Children_1_Children_3_Children_3_Text = Children_0_Children_1_Children_3_Children_3["Text"]; // "CPU DRAM"

//const char* Children_0_Children_1_Children_3_Children_3_Min = Children_0_Children_1_Children_3_Children_3["Min"]; // "0,5 W"
//const char* Children_0_Children_1_Children_3_Children_3_Value = Children_0_Children_1_Children_3_Children_3["Value"]; // "0,8 W"
//const char* Children_0_Children_1_Children_3_Children_3_Max = Children_0_Children_1_Children_3_Children_3["Max"]; // "2,9 W"
//const char* Children_0_Children_1_Children_3_Children_3_ImageURL = Children_0_Children_1_Children_3_Children_3["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_1_Children_3_Min = Children_0_Children_1_Children_3["Min"]; // ""
//const char* Children_0_Children_1_Children_3_Value = Children_0_Children_1_Children_3["Value"]; // ""
//const char* Children_0_Children_1_Children_3_Max = Children_0_Children_1_Children_3["Max"]; // ""
//const char* Children_0_Children_1_Children_3_ImageURL = Children_0_Children_1_Children_3["ImageURL"]; // "images_icon/power.png"

//const char* Children_0_Children_1_Min = Children_0_Children_1["Min"]; // ""
//const char* Children_0_Children_1_Value = Children_0_Children_1["Value"]; // ""
//const char* Children_0_Children_1_Max = Children_0_Children_1["Max"]; // ""
//const char* Children_0_Children_1_ImageURL = Children_0_Children_1["ImageURL"]; // "images_icon/cpu.png"

JsonObject Children_0_Children_2 = Children_0_Children[2];
//int Children_0_Children_2_id = Children_0_Children_2["id"]; // 69
//const char* Children_0_Children_2_Text = Children_0_Children_2["Text"]; // "Generic Memory"

//JsonObject Children_0_Children_2_Children_0 = Children_0_Children_2["Children"][0];
//int Children_0_Children_2_Children_0_id = Children_0_Children_2_Children_0["id"]; // 70
//const char* Children_0_Children_2_Children_0_Text = Children_0_Children_2_Children_0["Text"]; // "Load"

//JsonObject Children_0_Children_2_Children_0_Children_0 = Children_0_Children_2_Children_0["Children"][0];
//int Children_0_Children_2_Children_0_Children_0_id = Children_0_Children_2_Children_0_Children_0["id"]; // 71
//const char* Children_0_Children_2_Children_0_Children_0_Text = Children_0_Children_2_Children_0_Children_0["Text"]; // "Memory"

//const char* Children_0_Children_2_Children_0_Children_0_Min = Children_0_Children_2_Children_0_Children_0["Min"]; // "38,8 %"
//const char* Children_0_Children_2_Children_0_Children_0_Value = Children_0_Children_2_Children_0_Children_0["Value"]; // "41,9 %"
//const char* Children_0_Children_2_Children_0_Children_0_Max = Children_0_Children_2_Children_0_Children_0["Max"]; // "48,4 %"
//const char* Children_0_Children_2_Children_0_Children_0_ImageURL = Children_0_Children_2_Children_0_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_2_Children_0_Min = Children_0_Children_2_Children_0["Min"]; // ""
//const char* Children_0_Children_2_Children_0_Value = Children_0_Children_2_Children_0["Value"]; // ""
//const char* Children_0_Children_2_Children_0_Max = Children_0_Children_2_Children_0["Max"]; // ""
//const char* Children_0_Children_2_Children_0_ImageURL = Children_0_Children_2_Children_0["ImageURL"]; // "images_icon/load.png"

JsonObject Children_0_Children_2_Children_1 = Children_0_Children_2["Children"][1];
//int Children_0_Children_2_Children_1_id = Children_0_Children_2_Children_1["id"]; // 72
//const char* Children_0_Children_2_Children_1_Text = Children_0_Children_2_Children_1["Text"]; // "Data"

JsonObject Children_0_Children_2_Children_1_Children_0 = Children_0_Children_2_Children_1["Children"][0];
//int Children_0_Children_2_Children_1_Children_0_id = Children_0_Children_2_Children_1_Children_0["id"]; // 73
//const char* Children_0_Children_2_Children_1_Children_0_Text = Children_0_Children_2_Children_1_Children_0["Text"]; // "Used Memory"

//const char* Children_0_Children_2_Children_1_Children_0_Min = Children_0_Children_2_Children_1_Children_0["Min"]; // "6,2 GB"
String Children_0_Children_2_Children_1_Children_0_Value = Children_0_Children_2_Children_1_Children_0["Value"]; // "6,7 GB"
//const char* Children_0_Children_2_Children_1_Children_0_Max = Children_0_Children_2_Children_1_Children_0["Max"]; // "7,7 GB"
//const char* Children_0_Children_2_Children_1_Children_0_ImageURL = Children_0_Children_2_Children_1_Children_0["ImageURL"]; // "images/transparent.png"

JsonObject Children_0_Children_2_Children_1_Children_1 = Children_0_Children_2_Children_1["Children"][1];
//int Children_0_Children_2_Children_1_Children_1_id = Children_0_Children_2_Children_1_Children_1["id"]; // 74
//const char* Children_0_Children_2_Children_1_Children_1_Text = Children_0_Children_2_Children_1_Children_1["Text"]; // "Available Memory"

//const char* Children_0_Children_2_Children_1_Children_1_Min = Children_0_Children_2_Children_1_Children_1["Min"]; // "8,2 GB"
const char* Children_0_Children_2_Children_1_Children_1_Value = Children_0_Children_2_Children_1_Children_1["Value"]; // "9,3 GB"
//const char* Children_0_Children_2_Children_1_Children_1_Max = Children_0_Children_2_Children_1_Children_1["Max"]; // "9,8 GB"
//const char* Children_0_Children_2_Children_1_Children_1_ImageURL = Children_0_Children_2_Children_1_Children_1["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_2_Children_1_Min = Children_0_Children_2_Children_1["Min"]; // ""
//const char* Children_0_Children_2_Children_1_Value = Children_0_Children_2_Children_1["Value"]; // ""
//const char* Children_0_Children_2_Children_1_Max = Children_0_Children_2_Children_1["Max"]; // ""
//const char* Children_0_Children_2_Children_1_ImageURL = Children_0_Children_2_Children_1["ImageURL"]; // "images_icon/power.png"

//const char* Children_0_Children_2_Min = Children_0_Children_2["Min"]; // ""
//const char* Children_0_Children_2_Value = Children_0_Children_2["Value"]; // ""
//const char* Children_0_Children_2_Max = Children_0_Children_2["Max"]; // ""
//const char* Children_0_Children_2_ImageURL = Children_0_Children_2["ImageURL"]; // "images_icon/ram.png"

JsonObject Children_0_Children_3 = Children_0_Children[3];
//int Children_0_Children_3_id = Children_0_Children_3["id"]; // 75
//const char* Children_0_Children_3_Text = Children_0_Children_3["Text"]; // "NVIDIA GeForce GTX 1080"

JsonArray Children_0_Children_3_Children = Children_0_Children_3["Children"];

JsonObject Children_0_Children_3_Children_0 = Children_0_Children_3_Children[0];
//int Children_0_Children_3_Children_0_id = Children_0_Children_3_Children_0["id"]; // 76
//const char* Children_0_Children_3_Children_0_Text = Children_0_Children_3_Children_0["Text"]; // "Clocks"

//JsonArray Children_0_Children_3_Children_0_Children = Children_0_Children_3_Children_0["Children"];

//JsonObject Children_0_Children_3_Children_0_Children_0 = Children_0_Children_3_Children_0_Children[0];
//int Children_0_Children_3_Children_0_Children_0_id = Children_0_Children_3_Children_0_Children_0["id"]; // 77
//const char* Children_0_Children_3_Children_0_Children_0_Text = Children_0_Children_3_Children_0_Children_0["Text"]; // "GPU Core"

//const char* Children_0_Children_3_Children_0_Children_0_Min = Children_0_Children_3_Children_0_Children_0["Min"]; // "1607,0 MHz"
//const char* Children_0_Children_3_Children_0_Children_0_Value = Children_0_Children_3_Children_0_Children_0["Value"]; // "1607,0 MHz"
//const char* Children_0_Children_3_Children_0_Children_0_Max = Children_0_Children_3_Children_0_Children_0["Max"]; // "1607,0 MHz"
//const char* Children_0_Children_3_Children_0_Children_0_ImageURL = Children_0_Children_3_Children_0_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_0_Children_1 = Children_0_Children_3_Children_0_Children[1];
//int Children_0_Children_3_Children_0_Children_1_id = Children_0_Children_3_Children_0_Children_1["id"]; // 78
//const char* Children_0_Children_3_Children_0_Children_1_Text = Children_0_Children_3_Children_0_Children_1["Text"]; // "GPU Memory"

//const char* Children_0_Children_3_Children_0_Children_1_Min = Children_0_Children_3_Children_0_Children_1["Min"]; // "5005,8 MHz"
//const char* Children_0_Children_3_Children_0_Children_1_Value = Children_0_Children_3_Children_0_Children_1["Value"]; // "5005,8 MHz"
//const char* Children_0_Children_3_Children_0_Children_1_Max = Children_0_Children_3_Children_0_Children_1["Max"]; // "5005,8 MHz"
//const char* Children_0_Children_3_Children_0_Children_1_ImageURL = Children_0_Children_3_Children_0_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_0_Children_2 = Children_0_Children_3_Children_0_Children[2];
//int Children_0_Children_3_Children_0_Children_2_id = Children_0_Children_3_Children_0_Children_2["id"]; // 79
//const char* Children_0_Children_3_Children_0_Children_2_Text = Children_0_Children_3_Children_0_Children_2["Text"]; // "GPU Shader"

//const char* Children_0_Children_3_Children_0_Children_2_Min = Children_0_Children_3_Children_0_Children_2["Min"]; // "3214,0 MHz"
//const char* Children_0_Children_3_Children_0_Children_2_Value = Children_0_Children_3_Children_0_Children_2["Value"]; // "3214,0 MHz"
//const char* Children_0_Children_3_Children_0_Children_2_Max = Children_0_Children_3_Children_0_Children_2["Max"]; // "3214,0 MHz"
//const char* Children_0_Children_3_Children_0_Children_2_ImageURL = Children_0_Children_3_Children_0_Children_2["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_3_Children_0_Min = Children_0_Children_3_Children_0["Min"]; // ""
//const char* Children_0_Children_3_Children_0_Value = Children_0_Children_3_Children_0["Value"]; // ""
//const char* Children_0_Children_3_Children_0_Max = Children_0_Children_3_Children_0["Max"]; // ""
//const char* Children_0_Children_3_Children_0_ImageURL = Children_0_Children_3_Children_0["ImageURL"]; // "images_icon/clock.png"

JsonObject Children_0_Children_3_Children_1 = Children_0_Children_3_Children[1];
//int Children_0_Children_3_Children_1_id = Children_0_Children_3_Children_1["id"]; // 80
//const char* Children_0_Children_3_Children_1_Text = Children_0_Children_3_Children_1["Text"]; // "Temperatures"

JsonObject Children_0_Children_3_Children_1_Children_0 = Children_0_Children_3_Children_1["Children"][0];
//int Children_0_Children_3_Children_1_Children_0_id = Children_0_Children_3_Children_1_Children_0["id"]; // 81
//const char* Children_0_Children_3_Children_1_Children_0_Text = Children_0_Children_3_Children_1_Children_0["Text"]; // "GPU Core"

//const char* Children_0_Children_3_Children_1_Children_0_Min = Children_0_Children_3_Children_1_Children_0["Min"]; // "51,0 °C"
String Children_0_Children_3_Children_1_Children_0_Value = Children_0_Children_3_Children_1_Children_0["Value"]; // "55,0 °C"
//const char* Children_0_Children_3_Children_1_Children_0_Max = Children_0_Children_3_Children_1_Children_0["Max"]; // "57,0 °C"
//const char* Children_0_Children_3_Children_1_Children_0_ImageURL = Children_0_Children_3_Children_1_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_3_Children_1_Min = Children_0_Children_3_Children_1["Min"]; // ""
//const char* Children_0_Children_3_Children_1_Value = Children_0_Children_3_Children_1["Value"]; // ""
//const char* Children_0_Children_3_Children_1_Max = Children_0_Children_3_Children_1["Max"]; // ""
//const char* Children_0_Children_3_Children_1_ImageURL = Children_0_Children_3_Children_1["ImageURL"]; // "images_icon/temperature.png"

JsonObject Children_0_Children_3_Children_2 = Children_0_Children_3_Children[2];
//int Children_0_Children_3_Children_2_id = Children_0_Children_3_Children_2["id"]; // 82
//const char* Children_0_Children_3_Children_2_Text = Children_0_Children_3_Children_2["Text"]; // "Load"

JsonArray Children_0_Children_3_Children_2_Children = Children_0_Children_3_Children_2["Children"];

JsonObject Children_0_Children_3_Children_2_Children_0 = Children_0_Children_3_Children_2_Children[0];
//int Children_0_Children_3_Children_2_Children_0_id = Children_0_Children_3_Children_2_Children_0["id"]; // 83
//const char* Children_0_Children_3_Children_2_Children_0_Text = Children_0_Children_3_Children_2_Children_0["Text"]; // "GPU Core"

//const char* Children_0_Children_3_Children_2_Children_0_Min = Children_0_Children_3_Children_2_Children_0["Min"]; // "0,0 %"
String Children_0_Children_3_Children_2_Children_0_Value = Children_0_Children_3_Children_2_Children_0["Value"]; // "3,0 %"
//const char* Children_0_Children_3_Children_2_Children_0_Max = Children_0_Children_3_Children_2_Children_0["Max"]; // "32,0 %"
//const char* Children_0_Children_3_Children_2_Children_0_ImageURL = Children_0_Children_3_Children_2_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_2_Children_1 = Children_0_Children_3_Children_2_Children[1];
//int Children_0_Children_3_Children_2_Children_1_id = Children_0_Children_3_Children_2_Children_1["id"]; // 84
//const char* Children_0_Children_3_Children_2_Children_1_Text = Children_0_Children_3_Children_2_Children_1["Text"]; // "GPU Frame Buffer"

//const char* Children_0_Children_3_Children_2_Children_1_Min = Children_0_Children_3_Children_2_Children_1["Min"]; // "2,0 %"
//const char* Children_0_Children_3_Children_2_Children_1_Value = Children_0_Children_3_Children_2_Children_1["Value"]; // "2,0 %"
//const char* Children_0_Children_3_Children_2_Children_1_Max = Children_0_Children_3_Children_2_Children_1["Max"]; // "19,0 %"
//const char* Children_0_Children_3_Children_2_Children_1_ImageURL = Children_0_Children_3_Children_2_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_2_Children_2 = Children_0_Children_3_Children_2_Children[2];
//int Children_0_Children_3_Children_2_Children_2_id = Children_0_Children_3_Children_2_Children_2["id"]; // 85
//const char* Children_0_Children_3_Children_2_Children_2_Text = Children_0_Children_3_Children_2_Children_2["Text"]; // "GPU Video Engine"

//const char* Children_0_Children_3_Children_2_Children_2_Min = Children_0_Children_3_Children_2_Children_2["Min"]; // "0,0 %"
//const char* Children_0_Children_3_Children_2_Children_2_Value = Children_0_Children_3_Children_2_Children_2["Value"]; // "0,0 %"
//const char* Children_0_Children_3_Children_2_Children_2_Max = Children_0_Children_3_Children_2_Children_2["Max"]; // "0,0 %"
//const char* Children_0_Children_3_Children_2_Children_2_ImageURL = Children_0_Children_3_Children_2_Children_2["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_2_Children_3 = Children_0_Children_3_Children_2_Children[3];
//int Children_0_Children_3_Children_2_Children_3_id = Children_0_Children_3_Children_2_Children_3["id"]; // 86
//const char* Children_0_Children_3_Children_2_Children_3_Text = Children_0_Children_3_Children_2_Children_3["Text"]; // "GPU Bus Interface"

//const char* Children_0_Children_3_Children_2_Children_3_Min = Children_0_Children_3_Children_2_Children_3["Min"]; // "0,0 %"
//const char* Children_0_Children_3_Children_2_Children_3_Value = Children_0_Children_3_Children_2_Children_3["Value"]; // "2,0 %"
//const char* Children_0_Children_3_Children_2_Children_3_Max = Children_0_Children_3_Children_2_Children_3["Max"]; // "24,0 %"
//const char* Children_0_Children_3_Children_2_Children_3_ImageURL = Children_0_Children_3_Children_2_Children_3["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_2_Children_4 = Children_0_Children_3_Children_2_Children[4];
//int Children_0_Children_3_Children_2_Children_4_id = Children_0_Children_3_Children_2_Children_4["id"]; // 87
//const char* Children_0_Children_3_Children_2_Children_4_Text = Children_0_Children_3_Children_2_Children_4["Text"]; // "GPU Memory"

//const char* Children_0_Children_3_Children_2_Children_4_Min = Children_0_Children_3_Children_2_Children_4["Min"]; // "16,6 %"
//const char* Children_0_Children_3_Children_2_Children_4_Value = Children_0_Children_3_Children_2_Children_4["Value"]; // "19,0 %"
//const char* Children_0_Children_3_Children_2_Children_4_Max = Children_0_Children_3_Children_2_Children_4["Max"]; // "20,5 %"
//const char* Children_0_Children_3_Children_2_Children_4_ImageURL = Children_0_Children_3_Children_2_Children_4["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_3_Children_2_Min = Children_0_Children_3_Children_2["Min"]; // ""
//const char* Children_0_Children_3_Children_2_Value = Children_0_Children_3_Children_2["Value"]; // ""
//const char* Children_0_Children_3_Children_2_Max = Children_0_Children_3_Children_2["Max"]; // ""
//const char* Children_0_Children_3_Children_2_ImageURL = Children_0_Children_3_Children_2["ImageURL"]; // "images_icon/load.png"

//JsonObject Children_0_Children_3_Children_3 = Children_0_Children_3_Children[3];
//int Children_0_Children_3_Children_3_id = Children_0_Children_3_Children_3["id"]; // 88
//const char* Children_0_Children_3_Children_3_Text = Children_0_Children_3_Children_3["Text"]; // "Fans"

//JsonObject Children_0_Children_3_Children_3_Children_0 = Children_0_Children_3_Children_3["Children"][0];
//int Children_0_Children_3_Children_3_Children_0_id = Children_0_Children_3_Children_3_Children_0["id"]; // 89
//const char* Children_0_Children_3_Children_3_Children_0_Text = Children_0_Children_3_Children_3_Children_0["Text"]; // "GPU"

//const char* Children_0_Children_3_Children_3_Children_0_Min = Children_0_Children_3_Children_3_Children_0["Min"]; // "1460 RPM"
//const char* Children_0_Children_3_Children_3_Children_0_Value = Children_0_Children_3_Children_3_Children_0["Value"]; // "1480 RPM"
//const char* Children_0_Children_3_Children_3_Children_0_Max = Children_0_Children_3_Children_3_Children_0["Max"]; // "1483 RPM"
//const char* Children_0_Children_3_Children_3_Children_0_ImageURL = Children_0_Children_3_Children_3_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_3_Children_3_Min = Children_0_Children_3_Children_3["Min"]; // ""
//const char* Children_0_Children_3_Children_3_Value = Children_0_Children_3_Children_3["Value"]; // ""
//const char* Children_0_Children_3_Children_3_Max = Children_0_Children_3_Children_3["Max"]; // ""
//const char* Children_0_Children_3_Children_3_ImageURL = Children_0_Children_3_Children_3["ImageURL"]; // "images_icon/fan.png"

JsonObject Children_0_Children_3_Children_4 = Children_0_Children_3_Children[4];
//int Children_0_Children_3_Children_4_id = Children_0_Children_3_Children_4["id"]; // 90
//const char* Children_0_Children_3_Children_4_Text = Children_0_Children_3_Children_4["Text"]; // "Controls"

JsonObject Children_0_Children_3_Children_4_Children_0 = Children_0_Children_3_Children_4["Children"][0];
//int Children_0_Children_3_Children_4_Children_0_id = Children_0_Children_3_Children_4_Children_0["id"]; // 91
//const char* Children_0_Children_3_Children_4_Children_0_Text = Children_0_Children_3_Children_4_Children_0["Text"]; // "GPU Fan"

//const char* Children_0_Children_3_Children_4_Children_0_Min = Children_0_Children_3_Children_4_Children_0["Min"]; // "37,0 %"
const char* Children_0_Children_3_Children_4_Children_0_Value = Children_0_Children_3_Children_4_Children_0["Value"]; // "37,0 %"
//const char* Children_0_Children_3_Children_4_Children_0_Max = Children_0_Children_3_Children_4_Children_0["Max"]; // "37,0 %"
//const char* Children_0_Children_3_Children_4_Children_0_ImageURL = Children_0_Children_3_Children_4_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_3_Children_4_Min = Children_0_Children_3_Children_4["Min"]; // ""
//const char* Children_0_Children_3_Children_4_Value = Children_0_Children_3_Children_4["Value"]; // ""
//const char* Children_0_Children_3_Children_4_Max = Children_0_Children_3_Children_4["Max"]; // ""
//const char* Children_0_Children_3_Children_4_ImageURL = Children_0_Children_3_Children_4["ImageURL"]; // "images_icon/control.png"

JsonObject Children_0_Children_3_Children_5 = Children_0_Children_3_Children[5];
//int Children_0_Children_3_Children_5_id = Children_0_Children_3_Children_5["id"]; // 92
//const char* Children_0_Children_3_Children_5_Text = Children_0_Children_3_Children_5["Text"]; // "Data"

JsonArray Children_0_Children_3_Children_5_Children = Children_0_Children_3_Children_5["Children"];

JsonObject Children_0_Children_3_Children_5_Children_0 = Children_0_Children_3_Children_5_Children[0];
//int Children_0_Children_3_Children_5_Children_0_id = Children_0_Children_3_Children_5_Children_0["id"]; // 93
//const char* Children_0_Children_3_Children_5_Children_0_Text = Children_0_Children_3_Children_5_Children_0["Text"]; // "GPU Memory Free"

//const char* Children_0_Children_3_Children_5_Children_0_Min = Children_0_Children_3_Children_5_Children_0["Min"]; // "6513,7 MB"
const char* Children_0_Children_3_Children_5_Children_0_Value = Children_0_Children_3_Children_5_Children_0["Value"]; // "6639,4 MB"
//const char* Children_0_Children_3_Children_5_Children_0_Max = Children_0_Children_3_Children_5_Children_0["Max"]; // "6834,8 MB"
//const char* Children_0_Children_3_Children_5_Children_0_ImageURL = Children_0_Children_3_Children_5_Children_0["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_5_Children_1 = Children_0_Children_3_Children_5_Children[1];
//int Children_0_Children_3_Children_5_Children_1_id = Children_0_Children_3_Children_5_Children_1["id"]; // 94
//const char* Children_0_Children_3_Children_5_Children_1_Text = Children_0_Children_3_Children_5_Children_1["Text"]; // "GPU Memory Used"

//const char* Children_0_Children_3_Children_5_Children_1_Min = Children_0_Children_3_Children_5_Children_1["Min"]; // "1357,2 MB"
//const char* Children_0_Children_3_Children_5_Children_1_Value = Children_0_Children_3_Children_5_Children_1["Value"]; // "1552,6 MB"
//const char* Children_0_Children_3_Children_5_Children_1_Max = Children_0_Children_3_Children_5_Children_1["Max"]; // "1678,3 MB"
//const char* Children_0_Children_3_Children_5_Children_1_ImageURL = Children_0_Children_3_Children_5_Children_1["ImageURL"]; // "images/transparent.png"

//JsonObject Children_0_Children_3_Children_5_Children_2 = Children_0_Children_3_Children_5_Children[2];
//int Children_0_Children_3_Children_5_Children_2_id = Children_0_Children_3_Children_5_Children_2["id"]; // 95
//const char* Children_0_Children_3_Children_5_Children_2_Text = Children_0_Children_3_Children_5_Children_2["Text"]; // "GPU Memory Total"

//const char* Children_0_Children_3_Children_5_Children_2_Min = Children_0_Children_3_Children_5_Children_2["Min"]; // "8192,0 MB"
//const char* Children_0_Children_3_Children_5_Children_2_Value = Children_0_Children_3_Children_5_Children_2["Value"]; // "8192,0 MB"
//const char* Children_0_Children_3_Children_5_Children_2_Max = Children_0_Children_3_Children_5_Children_2["Max"]; // "8192,0 MB"
//const char* Children_0_Children_3_Children_5_Children_2_ImageURL = Children_0_Children_3_Children_5_Children_2["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_3_Children_5_Min = Children_0_Children_3_Children_5["Min"]; // ""
//const char* Children_0_Children_3_Children_5_Value = Children_0_Children_3_Children_5["Value"]; // ""
//const char* Children_0_Children_3_Children_5_Max = Children_0_Children_3_Children_5["Max"]; // ""
//const char* Children_0_Children_3_Children_5_ImageURL = Children_0_Children_3_Children_5["ImageURL"]; // "images_icon/power.png"

//const char* Children_0_Children_3_Min = Children_0_Children_3["Min"]; // ""
//const char* Children_0_Children_3_Value = Children_0_Children_3["Value"]; // ""
//const char* Children_0_Children_3_Max = Children_0_Children_3["Max"]; // ""
//const char* Children_0_Children_3_ImageURL = Children_0_Children_3["ImageURL"]; // "images_icon/nvidia.png"

JsonObject Children_0_Children_4 = Children_0_Children[4];
//int Children_0_Children_4_id = Children_0_Children_4["id"]; // 96
//const char* Children_0_Children_4_Text = Children_0_Children_4["Text"]; // "WDC WD20PURX-64P6ZY0"

//JsonObject Children_0_Children_4_Children_0 = Children_0_Children_4["Children"][0];
//int Children_0_Children_4_Children_0_id = Children_0_Children_4_Children_0["id"]; // 97
//const char* Children_0_Children_4_Children_0_Text = Children_0_Children_4_Children_0["Text"]; // "Temperatures"

//JsonObject Children_0_Children_4_Children_0_Children_0 = Children_0_Children_4_Children_0["Children"][0];
//int Children_0_Children_4_Children_0_Children_0_id = Children_0_Children_4_Children_0_Children_0["id"]; // 98
//const char* Children_0_Children_4_Children_0_Children_0_Text = Children_0_Children_4_Children_0_Children_0["Text"]; // "Temperature"

//const char* Children_0_Children_4_Children_0_Children_0_Min = Children_0_Children_4_Children_0_Children_0["Min"]; // "37,0 °C"
//const char* Children_0_Children_4_Children_0_Children_0_Value = Children_0_Children_4_Children_0_Children_0["Value"]; // "37,0 °C"
//const char* Children_0_Children_4_Children_0_Children_0_Max = Children_0_Children_4_Children_0_Children_0["Max"]; // "37,0 °C"
//const char* Children_0_Children_4_Children_0_Children_0_ImageURL = Children_0_Children_4_Children_0_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_4_Children_0_Min = Children_0_Children_4_Children_0["Min"]; // ""
//const char* Children_0_Children_4_Children_0_Value = Children_0_Children_4_Children_0["Value"]; // ""
//const char* Children_0_Children_4_Children_0_Max = Children_0_Children_4_Children_0["Max"]; // ""
//const char* Children_0_Children_4_Children_0_ImageURL = Children_0_Children_4_Children_0["ImageURL"]; // "images_icon/temperature.png"

//JsonObject Children_0_Children_4_Children_1 = Children_0_Children_4["Children"][1];
//int Children_0_Children_4_Children_1_id = Children_0_Children_4_Children_1["id"]; // 99
//const char* Children_0_Children_4_Children_1_Text = Children_0_Children_4_Children_1["Text"]; // "Load"

//JsonObject Children_0_Children_4_Children_1_Children_0 = Children_0_Children_4_Children_1["Children"][0];
//int Children_0_Children_4_Children_1_Children_0_id = Children_0_Children_4_Children_1_Children_0["id"]; // 100
//const char* Children_0_Children_4_Children_1_Children_0_Text = Children_0_Children_4_Children_1_Children_0["Text"]; // "Used Space"

//const char* Children_0_Children_4_Children_1_Children_0_Min = Children_0_Children_4_Children_1_Children_0["Min"]; // "86,9 %"
//const char* Children_0_Children_4_Children_1_Children_0_Value = Children_0_Children_4_Children_1_Children_0["Value"]; // "86,9 %"
//const char* Children_0_Children_4_Children_1_Children_0_Max = Children_0_Children_4_Children_1_Children_0["Max"]; // "86,9 %"
//const char* Children_0_Children_4_Children_1_Children_0_ImageURL = Children_0_Children_4_Children_1_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_4_Children_1_Min = Children_0_Children_4_Children_1["Min"]; // ""
//const char* Children_0_Children_4_Children_1_Value = Children_0_Children_4_Children_1["Value"]; // ""
//const char* Children_0_Children_4_Children_1_Max = Children_0_Children_4_Children_1["Max"]; // ""
//const char* Children_0_Children_4_Children_1_ImageURL = Children_0_Children_4_Children_1["ImageURL"]; // "images_icon/load.png"

//const char* Children_0_Children_4_Min = Children_0_Children_4["Min"]; // ""
//const char* Children_0_Children_4_Value = Children_0_Children_4["Value"]; // ""
//const char* Children_0_Children_4_Max = Children_0_Children_4["Max"]; // ""
//const char* Children_0_Children_4_ImageURL = Children_0_Children_4["ImageURL"]; // "images_icon/hdd.png"

JsonObject Children_0_Children_5 = Children_0_Children[5];
//int Children_0_Children_5_id = Children_0_Children_5["id"]; // 101
//const char* Children_0_Children_5_Text = Children_0_Children_5["Text"]; // "WDC WDS250G1B0B-00AS40"

//JsonObject Children_0_Children_5_Children_0 = Children_0_Children_5["Children"][0];
//int Children_0_Children_5_Children_0_id = Children_0_Children_5_Children_0["id"]; // 102
//const char* Children_0_Children_5_Children_0_Text = Children_0_Children_5_Children_0["Text"]; // "Temperatures"

//JsonObject Children_0_Children_5_Children_0_Children_0 = Children_0_Children_5_Children_0["Children"][0];
//int Children_0_Children_5_Children_0_Children_0_id = Children_0_Children_5_Children_0_Children_0["id"]; // 103
//const char* Children_0_Children_5_Children_0_Children_0_Text = Children_0_Children_5_Children_0_Children_0["Text"]; // "Temperature"

//const char* Children_0_Children_5_Children_0_Children_0_Min = Children_0_Children_5_Children_0_Children_0["Min"]; // "50,0 °C"
//const char* Children_0_Children_5_Children_0_Children_0_Value = Children_0_Children_5_Children_0_Children_0["Value"]; // "52,0 °C"
//const char* Children_0_Children_5_Children_0_Children_0_Max = Children_0_Children_5_Children_0_Children_0["Max"]; // "52,0 °C"
//const char* Children_0_Children_5_Children_0_Children_0_ImageURL = Children_0_Children_5_Children_0_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_5_Children_0_Min = Children_0_Children_5_Children_0["Min"]; // ""
//const char* Children_0_Children_5_Children_0_Value = Children_0_Children_5_Children_0["Value"]; // ""
//const char* Children_0_Children_5_Children_0_Max = Children_0_Children_5_Children_0["Max"]; // ""
//const char* Children_0_Children_5_Children_0_ImageURL = Children_0_Children_5_Children_0["ImageURL"]; // "images_icon/temperature.png"

//JsonObject Children_0_Children_5_Children_1 = Children_0_Children_5["Children"][1];
//int Children_0_Children_5_Children_1_id = Children_0_Children_5_Children_1["id"]; // 104
//const char* Children_0_Children_5_Children_1_Text = Children_0_Children_5_Children_1["Text"]; // "Load"

//JsonObject Children_0_Children_5_Children_1_Children_0 = Children_0_Children_5_Children_1["Children"][0];
//int Children_0_Children_5_Children_1_Children_0_id = Children_0_Children_5_Children_1_Children_0["id"]; // 105
//const char* Children_0_Children_5_Children_1_Children_0_Text = Children_0_Children_5_Children_1_Children_0["Text"]; // "Used Space"

//const char* Children_0_Children_5_Children_1_Children_0_Min = Children_0_Children_5_Children_1_Children_0["Min"]; // "55,9 %"
//const char* Children_0_Children_5_Children_1_Children_0_Value = Children_0_Children_5_Children_1_Children_0["Value"]; // "56,0 %"
//const char* Children_0_Children_5_Children_1_Children_0_Max = Children_0_Children_5_Children_1_Children_0["Max"]; // "56,0 %"
//const char* Children_0_Children_5_Children_1_Children_0_ImageURL = Children_0_Children_5_Children_1_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_5_Children_1_Min = Children_0_Children_5_Children_1["Min"]; // ""
//const char* Children_0_Children_5_Children_1_Value = Children_0_Children_5_Children_1["Value"]; // ""
//const char* Children_0_Children_5_Children_1_Max = Children_0_Children_5_Children_1["Max"]; // ""
//const char* Children_0_Children_5_Children_1_ImageURL = Children_0_Children_5_Children_1["ImageURL"]; // "images_icon/load.png"

//const char* Children_0_Children_5_Min = Children_0_Children_5["Min"]; // ""
//const char* Children_0_Children_5_Value = Children_0_Children_5["Value"]; // ""
//const char* Children_0_Children_5_Max = Children_0_Children_5["Max"]; // ""
//const char* Children_0_Children_5_ImageURL = Children_0_Children_5["ImageURL"]; // "images_icon/hdd.png"

JsonObject Children_0_Children_6 = Children_0_Children[6];
//int Children_0_Children_6_id = Children_0_Children_6["id"]; // 106
//const char* Children_0_Children_6_Text = Children_0_Children_6["Text"]; // "Patriot Burst"

//JsonObject Children_0_Children_6_Children_0 = Children_0_Children_6["Children"][0];
//int Children_0_Children_6_Children_0_id = Children_0_Children_6_Children_0["id"]; // 107
//const char* Children_0_Children_6_Children_0_Text = Children_0_Children_6_Children_0["Text"]; // "Temperatures"

//JsonObject Children_0_Children_6_Children_0_Children_0 = Children_0_Children_6_Children_0["Children"][0];
//int Children_0_Children_6_Children_0_Children_0_id = Children_0_Children_6_Children_0_Children_0["id"]; // 108
//const char* Children_0_Children_6_Children_0_Children_0_Text = Children_0_Children_6_Children_0_Children_0["Text"]; // "Temperature"

//const char* Children_0_Children_6_Children_0_Children_0_Min = Children_0_Children_6_Children_0_Children_0["Min"]; // "33,0 °C"
//const char* Children_0_Children_6_Children_0_Children_0_Value = Children_0_Children_6_Children_0_Children_0["Value"]; // "33,0 °C"
//const char* Children_0_Children_6_Children_0_Children_0_Max = Children_0_Children_6_Children_0_Children_0["Max"]; // "33,0 °C"
//const char* Children_0_Children_6_Children_0_Children_0_ImageURL = Children_0_Children_6_Children_0_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_6_Children_0_Min = Children_0_Children_6_Children_0["Min"]; // ""
//const char* Children_0_Children_6_Children_0_Value = Children_0_Children_6_Children_0["Value"]; // ""
//const char* Children_0_Children_6_Children_0_Max = Children_0_Children_6_Children_0["Max"]; // ""
//const char* Children_0_Children_6_Children_0_ImageURL = Children_0_Children_6_Children_0["ImageURL"]; // "images_icon/temperature.png"

//JsonObject Children_0_Children_6_Children_1 = Children_0_Children_6["Children"][1];
//int Children_0_Children_6_Children_1_id = Children_0_Children_6_Children_1["id"]; // 109
//const char* Children_0_Children_6_Children_1_Text = Children_0_Children_6_Children_1["Text"]; // "Load"

//JsonObject Children_0_Children_6_Children_1_Children_0 = Children_0_Children_6_Children_1["Children"][0];
//int Children_0_Children_6_Children_1_Children_0_id = Children_0_Children_6_Children_1_Children_0["id"]; // 110
//const char* Children_0_Children_6_Children_1_Children_0_Text = Children_0_Children_6_Children_1_Children_0["Text"]; // "Used Space"

//const char* Children_0_Children_6_Children_1_Children_0_Min = Children_0_Children_6_Children_1_Children_0["Min"]; // "18,0 %"
//const char* Children_0_Children_6_Children_1_Children_0_Value = Children_0_Children_6_Children_1_Children_0["Value"]; // "18,0 %"
//const char* Children_0_Children_6_Children_1_Children_0_Max = Children_0_Children_6_Children_1_Children_0["Max"]; // "18,0 %"
//const char* Children_0_Children_6_Children_1_Children_0_ImageURL = Children_0_Children_6_Children_1_Children_0["ImageURL"]; // "images/transparent.png"

//const char* Children_0_Children_6_Children_1_Min = Children_0_Children_6_Children_1["Min"]; // ""
//const char* Children_0_Children_6_Children_1_Value = Children_0_Children_6_Children_1["Value"]; // ""
//const char* Children_0_Children_6_Children_1_Max = Children_0_Children_6_Children_1["Max"]; // ""
//const char* Children_0_Children_6_Children_1_ImageURL = Children_0_Children_6_Children_1["ImageURL"]; // "images_icon/load.png"

//const char* Children_0_Children_6_Min = Children_0_Children_6["Min"]; // ""
//const char* Children_0_Children_6_Value = Children_0_Children_6["Value"]; // ""
//const char* Children_0_Children_6_Max = Children_0_Children_6["Max"]; // ""
//const char* Children_0_Children_6_ImageURL = Children_0_Children_6["ImageURL"]; // "images_icon/hdd.png"

//JsonObject Children_0_Children_7 = Children_0_Children[7];
//int Children_0_Children_7_id = Children_0_Children_7["id"]; // 111
//const char* Children_0_Children_7_Text = Children_0_Children_7["Text"]; // "Generic Hard Disk"

//const char* Children_0_Children_7_Min = Children_0_Children_7["Min"]; // ""
//const char* Children_0_Children_7_Value = Children_0_Children_7["Value"]; // ""
//const char* Children_0_Children_7_Max = Children_0_Children_7["Max"]; // ""
//const char* Children_0_Children_7_ImageURL = Children_0_Children_7["ImageURL"]; // "images_icon/hdd.png"

//JsonObject Children_0_Children_8 = Children_0_Children[8];
//int Children_0_Children_8_id = Children_0_Children_8["id"]; // 112
//const char* Children_0_Children_8_Text = Children_0_Children_8["Text"]; // "Generic Hard Disk"

//const char* Children_0_Children_8_Min = Children_0_Children_8["Min"]; // ""
//const char* Children_0_Children_8_Value = Children_0_Children_8["Value"]; // ""
//const char* Children_0_Children_8_Max = Children_0_Children_8["Max"]; // ""
//const char* Children_0_Children_8_ImageURL = Children_0_Children_8["ImageURL"]; // "images_icon/hdd.png"

//JsonObject Children_0_Children_9 = Children_0_Children[9];
//int Children_0_Children_9_id = Children_0_Children_9["id"]; // 113
//const char* Children_0_Children_9_Text = Children_0_Children_9["Text"]; // "Generic Hard Disk"

//const char* Children_0_Children_9_Min = Children_0_Children_9["Min"]; // ""
//const char* Children_0_Children_9_Value = Children_0_Children_9["Value"]; // ""
//const char* Children_0_Children_9_Max = Children_0_Children_9["Max"]; // ""
//const char* Children_0_Children_9_ImageURL = Children_0_Children_9["ImageURL"]; // "images_icon/hdd.png"

//const char* Children_0_Min = Children_0["Min"]; // ""
//const char* Children_0_Value = Children_0["Value"]; // ""
//const char* Children_0_Max = Children_0["Max"]; // ""
//const char* Children_0_ImageURL = Children_0["ImageURL"]; // "images_icon/computer.png"

//const char* Min = doc["Min"]; // "Min"
//const char* Value = doc["Value"]; // "Value"
//const char* Max = doc["Max"]; // "Max"
//const char* ImageURL = doc["ImageURL"]; // ""

  
  tft.setCursor(60, 290, 4);
  tft.print("Cpu"); 
  tft.setCursor(215, 290, 4);
  tft.print("Gpu");
  tft.setCursor(370, 290, 4);
  tft.print("Mem");

  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  
  int currentMonth = ptm->tm_mon+1;
  String cMonth;
  String mDay;
  String plusNull = "0";
  
  int currentYear = ptm->tm_year+1900;

  String weekDay = weekDays[timeClient.getDay()];

  if(currentMonth < 10){
     cMonth = String(plusNull + currentMonth);
     
  }else
  {
     cMonth = String(currentMonth);
  }
  
  if(monthDay < 10){
     mDay = String(plusNull + monthDay);
  }else
  {
     mDay = String(monthDay);
  }
  
  

  
  String currentDate = String(currentYear) + "." + cMonth + "." + mDay;
    timeClient.forceUpdate();
  tft.setCursor(140, 0, 2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  tft.print(timeClient.getFormattedTime());
  tft.print("  ");
  tft.print(currentDate);
  tft.print("  ");
  tft.print(weekDay);
  tft.print("              ");
  
  
  
  
    // Comment out above meters, then uncomment the next line to show large meter
    //cpu
   ringMeter(Children_0_Children_1_Children_2_Children_0_Value.toInt(),0,100, 30,25,52," Load",GREEN2RED); // Draw analogue meter
    //gpu
   ringMeter(Children_0_Children_3_Children_2_Children_0_Value.toInt(),0,100, 185,25,52," Load",GREEN2RED);
    //memo
   ringMeter(Children_0_Children_2_Children_1_Children_0_Value.toInt(),0,16, 340,25,52," Used GB",GREEN2GREEN);
  //cpu
  ringMeter(Children_0_Children_1_Children_1_Children_0_Value.toInt(),0,100, 30,160,52," Temp",BLUE2RED); 
    //gpu
   ringMeter(Children_0_Children_3_Children_1_Children_0_Value.toInt(),0,100, 185,160,52," Temp",BLUE2RED);
    //memo
   ringMeter(Children_0_Children_0_Children_0_Children_3_Children_0_Value.toInt(),0,16, 340,160,52," CpuFan",GREEN2GREEN);


  delay(300);

       

  }else {
    if (wClear){
        tft.fillScreen(TFT_BLACK);
        wClear = false;
        hwClear = true;
      }
    
  timeClient.forceUpdate();  
  tft.setCursor(10, 10, 6);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  tft.print(timeClient.getFormattedTime());

  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  
  int currentMonth = ptm->tm_mon+1;
  String cMonth;
  String mDay;
  String plusNull = "0";
  
  int currentYear = ptm->tm_year+1900;

  String weekDay = weekDays[timeClient.getDay()];

  if(currentMonth < 10){
     cMonth = String(plusNull + currentMonth);
     
  }else
  {
     cMonth = String(currentMonth);
  }
  
  if(monthDay < 10){
     mDay = String(plusNull + monthDay);
  }else
  {
     mDay = String(monthDay);
  }
  
  

  
  String currentDate = String(currentYear) + "." + cMonth + "." + mDay;
  tft.setCursor(211, 8, 4);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  
  tft.print(weekDay);
  tft.print("            ");
  tft.setCursor(211, 32, 4);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  
  tft.print(currentDate);
  
  

  
  getWeather();
  
  delay(300);
  }
  
}


};






void loop() {
  
  
  parsing();
  button.read();

}







