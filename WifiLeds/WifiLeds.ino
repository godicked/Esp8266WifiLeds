#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <EEPROM.h>

#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#define NUM_LEDS 6
#define DATA_PIN 4

// Define the array of leds
CRGB leds[NUM_LEDS];

CRGB color1 = CRGB(170, 0, 0);
CRGB color2 = CRGB(0,0,255);
float counter = 0;

//  Wifi and WebServer
WiFiManager wifiManager;
ESP8266WebServer server(80);
String webPage = "";

//  default custom static IP
char static_ip[16] = "192.168.0.60";
char static_gw[16] = "192.168.0.1";
char static_sn[16] = "255.255.255.0";


void htmlColor(int r, int g, int b, char *hex)
{
    char hexcol[7];

    snprintf(hexcol, sizeof hexcol, "%02x%02x%02x", r, g, b);
    for(int i = 0; i < 6; i++)
    {
        hex[i] = hexcol[i];
    }
}


void rgbColor(const char *hex, uint8_t &r, uint8_t &g, uint8_t &b)
{
    int c = strtol(hex, NULL, 16);
    
    r = c >> 16;
    g = (c - (r << 16)) >> 8;
    b = (c - (r << 16) - (g << 8));
}

String htmlpage()
{
  char c1[7];
  char c2[7];

  htmlColor(color1.r, color1.g, color1.b, c1);
  htmlColor(color2.r, color2.g, color2.b, c2);

  c1[6] = '\0';
  c2[6] = '\0';
  
  String page = "";
  page += "<div><input type=\"color\" id=\"c1\" style=\"height: 50%; width: 100%;\"/></div>";
  page += "<div><input type=\"color\" id=\"c2\" style=\"height: 50%; width: 100%;\"/></div>";

  page += "<script> var color1 = '#" + String(c1) + "';";
  page += "var color2 = '#" + String(c2) + "';";
  page += "function post(path, params) {var xhttp = new XMLHttpRequest();path += '?';if( params.c1 ) {path += 'c1=' + params.c1 + '&';}if( params.c2 ) {path += 'c2=' + params.c2;}xhttp.open('POST', path, true);xhttp.send();}var c1 = document.getElementById('c1');var c2 = document.getElementById('c2');c1.value = color1;c2.value = color2;c1.addEventListener('change', (event) => {var color = event.target.value.slice(1);post('/color', {c1: color});}, false);c2.addEventListener('change', (event) => {var color = event.target.value.slice(1);post('/color', {c2: color});}, false);</script>";
  
  return page;
}


void animation()
{
   for(int i = 0; i < NUM_LEDS; i++) 
   {
    float xi = i * (1.0/NUM_LEDS) * 255 * 2;
    xi = (xi + counter);
    leds[i].r = color1.r * wave(xi) + color2.r * (1-wave(xi));
    leds[i].g = color1.g * wave(xi) + color2.g * (1-wave(xi));
    leds[i].b = color1.b * wave(xi) + color2.b * (1-wave(xi));
  }
  counter += 0.07; 
}

float wave(float t)
{
  return cubicwave8(t) / 255.0;
}

/*
void saveColor(CRGB color, int id)
{
  id *= 3;

  Serial.println(color.r);
  Serial.println(color.g);
  Serial.println(color.b);
  
  EEPROM.write(id, color.r);
  EEPROM.write(id+1, color.g);
  EEPROM.write(id+2, color.b);
}
*/
/*
void loadColors()
{

  Serial.println(EEPROM.read(0));
  Serial.println(EEPROM.read(1));
  Serial.println(EEPROM.read(2));

  Serial.println((byte)255);
  
  color1.r = EEPROM.read(0);
  color1.g = EEPROM.read(1);
  color1.b = EEPROM.read(2);

  color2.r = EEPROM.read(3);
  color2.g = EEPROM.read(4);
  color2.b = EEPROM.read(5);
}
*/

void saveColor(CRGB &color, int id)
{
  EEPROM.put(id * sizeof(CRGB), color);   
  EEPROM.commit();
}

void loadColors()
{
  EEPROM.get(0, color1);
  EEPROM.get(sizeof(CRGB), color2);
}
void setup()
{
  EEPROM.begin(512);
  delay(100);
  
  IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  wifiManager.autoConnect("Licht");

  Serial.begin(115200);
  Serial.println(WiFi.localIP());
  
  // initialize digital pin 13 as an output.
  pinMode(2, OUTPUT);

  loadColors();

  server.on("/", [](){
    server.send(200, "text/html", htmlpage());
    Serial.println("end");
    
  });

  server.on("/color", [](){
    
    if(server.arg("c1") != "")
    {
      rgbColor(server.arg("c1").c_str(), color1.r, color1.g, color1.b);
      saveColor(color1, 0);
    }
    if(server.arg("c2") != "")
    {
      rgbColor(server.arg("c2").c_str(), color2.r, color2.g, color2.b);
      saveColor(color2, 1);
    }

    server.send(200, "", "");
  });

  server.begin();

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void loop() 
{
  server.handleClient();
  animation();
  FastLED.show();
}
