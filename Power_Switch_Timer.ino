//-------------------------------------------------------------------------------------------------
//
//  File : Power_Switch_Timer.ino
//
//-------------------------------------------------------------------------------------------------
//
// Copyright(c) 2022 Alain Royer.
// Email: aroyer.qc@gmail.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
// AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Configuration(s)
//-------------------------------------------------------------------------------------------------

#define USE_ESP32
//#define USE_WIFI
//#define TIMER_INTERRUPT_DEBUG      1                  // For "ESP x TimerInterrupt.h"
//#define USE_NEO_PIXEL

//-------------------------------------------------------------------------------------------------
// Include file(s)
//-------------------------------------------------------------------------------------------------

#ifdef USE_ESP32
    #include <WiFi.h>
    #include "ESPAsyncWebServer.h"
#else
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
#endif

#ifdef USE_ESP32
    #include <ESP32TimerInterrupt.h>
    //#include <ESP32_ISR_Timer-Impl.h>
    //#include <ESP32_ISR_Timer.h>
#else
    #include <ESP8266TimerInterrupt.h>
    //#include <ESP8266_ISR_Timer-Impl.h>
    //#include <ESP8266_ISR_Timer.h>
#endif

#ifdef USE_NEO_PIXEL
#include <Adafruit_NeoPixel.h>
#endif

//-------------------------------------------------------------------------------------------------
// Define(s)
//-------------------------------------------------------------------------------------------------

#define PIN                     13           // Which pin on the ESP8266 is connected to the NeoPixels?

#ifdef USE_ESP32
#define LED_BUILTIN             2            // Led on ESP32
#endif

#define TIMER_INTERVAL_MS       1000
#define MAX_STEP                26
#define NUMBER_OF_COLOR         13
//#define MIN_RANDOM_TIMING       4000                 // 1000 = 1 sec
//#define MAX_RANDOM_TIMING       20000

//#define MIN_RANDOM_TIMING_CRAZY 100                 // 1000 = 1 sec
//#define MAX_RANDOM_TIMING_CRAZY 500

#define DelayHasEnded(DELAY)  ((millis() > DELAY) ? true : false)

// ****************************************************************************
// Typedefs 

typedef enum
{
    PIXEL_0,
    PIXEL_1,
    PIXEL_2,
    PIXEL_3,
    PIXEL_4,
    PIXEL_5,
    PIXEL_6,
    PIXEL_7,
    
    NUMBER_OF_PIXEL,
    FIRST_PIXEL = 0,
} Pixel_t;

typedef struct
{
  uint8_t Red;
  uint8_t Green;
  uint8_t Blue;
} Color_t;

// ****************************************************************************
// Variables 

#ifdef USE_WIFI
    const char* ssid = "Timer";   
    const char* password = "Magic";

    // Put IP Address details
    IPAddress local_ip(192,168,1,1);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);

    #ifdef USE_ESP32
  AsyncWebServer server(80);          // Set to port 80 as server
 #else
  ESP8266WebServer server(80);        // Set to port 80 as server
 #endif // USE_ESP32

#endif // USE_WIFI

uint32_t TickCounter;

 #ifdef USE_ESP32
  ESP32Timer Timer(0);
 #else
  ESP8266Timer Timer(1);
 #endif // USE_ESP32

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_OF_PIXEL, PIN, NEO_GRB + NEO_KHZ800);

const uint8_t PixPercent[MAX_STEP] =
{
    0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 100,   // Ascending 
    64, 48, 32, 24, 16, 12, 8, 6, 4, 2, 1, 0            // Descending
};

const Color_t Color[13]
{
  {255, 0,   0  },    // Red
  {0,   255, 0  },    // Lime
  {0,   0,   255},    // Blue
  {255, 255, 0  },    // Yellow
  {0,   255, 255},    // Cyan
  {255, 0,   255},    // Magenta
  {255, 255 ,255},    // White  
  {128, 0,   0  },    // Dark Yellow  
  {128, 128, 0  },    // Olive
  {0,   128, 0  },    // Green
  {0,   128, 128},    // Teal
  {0,   0,   128},    // Navy
  {128, 128 ,128},    // White  
};


bool IsIsTimeToUpdate = false;
bool RequestAllPixel  = false;
uint32_t GoCrazy  = 0;

uint8_t  PixRed[NUMBER_OF_PIXEL];
uint8_t  PixGreen[NUMBER_OF_PIXEL];
uint8_t  PixBlue[NUMBER_OF_PIXEL];
uint32_t PixTiming[NUMBER_OF_PIXEL];
uint32_t PixStep[NUMBER_OF_PIXEL];

// ****************************************************************************

void IRAM_ATTR TimerHandler(void)
{
    TickCounter++;

  #ifdef USE_ESP32
    static bool Toggle = false;
  
    if((TickCounter % 2000) == 0)
    {
        digitalWrite(LED_BUILTIN, Toggle);
        Toggle = !Toggle;
        // RequestAllPixel = true;
        GoCrazy = 2000;
    }
  #endif  
}

// ****************************************************************************

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Lucioles Magiques");
    Serial.println("Initialiser serveur");

  #ifdef USE_WIFI
    SetupWifi();
  #endif // USE_WIFI

  #ifdef USE_ESP32
    pinMode(LED_BUILTIN, OUTPUT);
  #endif // USE_ESP32
    
    pixels.begin(); // This initializes the NeoPixel library.

    // if analog input pin 0 is unconnected, random analog
    // noise will cause the call to randomSeed() to generate
    // different seed numbers each time the sketch runs.
    // randomSeed() will then shuffle the random function.
    randomSeed(analogRead(0));
  
    // Randomize all value for pixel
    for(int Pixel = (int)FIRST_PIXEL; Pixel < (int)NUMBER_OF_PIXEL; Pixel++)
    {
        SetNextPixel(Pixel);
    }

    // Interval in microsecs
    if(Timer.attachInterruptInterval(TIMER_INTERVAL_MS * 16, TimerHandler))
    {
        Serial.println("Starting  Timer OK, millis() = " + String(millis()));
    }
    else
    {
        Serial.println("Can't set Timer correctly. Select another freq. or interval");
    }
}

// ****************************************************************************

void loop()
{
    if(RequestAllPixel == true)
    {
        for(int Pixel = (int)FIRST_PIXEL; Pixel < (int)NUMBER_OF_PIXEL; Pixel++)
        {
            PixStep[Pixel] = 1;
            IsIsTimeToUpdate = true;
        }
        
        RequestAllPixel = false;
    }
    else
    {
        for(int Pixel = (int)FIRST_PIXEL; Pixel < (int)NUMBER_OF_PIXEL; Pixel++)
        {
            if(PixStep[Pixel] != 0)
            {
               PixStep[Pixel]++;
    
               if(PixStep[Pixel] == MAX_STEP)
               {
                  PixStep[Pixel] = 0;
                  SetNextPixel(Pixel);
               }
               else
               {
                  IsIsTimeToUpdate = true;
               //   Serial.printf("%d - %d \r\n", Pixel, PixPercent[PixStep[Pixel]]);
               }
            }
            else
            {      
                if(DelayHasEnded(PixTiming[Pixel]))
                {
                    PixStep[Pixel]++;
                    IsIsTimeToUpdate = true;
                 //  Serial.printf("%d - %d \r\n", Pixel, PixPercent[PixStep[Pixel]]);
                }
            }
        }
    }
  /*
   IsIsTimeToUpdate = true;
   PixStep[0]++;
   if(PixStep[0] >= MAX_STEP)
   {
      SetNextPixel(0);
     PixStep[0] = 0;
   }
   //delay(840);
  */
    if(IsIsTimeToUpdate == true)
    {
        uint8_t Red;
        uint8_t Green;
        uint8_t Blue;

        IsIsTimeToUpdate = false;
        
        // Update Pixel
        for(int Pixel = (int)FIRST_PIXEL; Pixel < (int)NUMBER_OF_PIXEL; Pixel++)
        {

            Red   = (uint8_t)(((uint32_t)PixPercent[PixStep[Pixel]] * PixRed  [Pixel]) / 100);
            Green = (uint8_t)(((uint32_t)PixPercent[PixStep[Pixel]] * PixGreen[Pixel]) / 100);
            Blue  = (uint8_t)(((uint32_t)PixPercent[PixStep[Pixel]] * PixBlue [Pixel]) / 100);

            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            pixels.setPixelColor(Pixel, pixels.Color(Red, Green, Blue));
        }
 
        pixels.show(); // This sends the updated pixel color to the hardware.
    }

    delay(8);
    if(GoCrazy != 0)
    {
        if(GoCrazy == 200)
        {
            Serial.println("Go Crazy");
            for(int Pixel = (int)FIRST_PIXEL; Pixel < (int)NUMBER_OF_PIXEL; Pixel++)
            {
                if(PixStep[Pixel] == 0)
                {
                    SetNextPixel(Pixel);
                }
            }
        }
        GoCrazy--;
        if(GoCrazy == 0)Serial.println("Go Normal");
    }
    
    //Serial.println("Timer millis() = " + String(millis()));
    //Serial.printf("%ld", TickCounter);

  #ifdef USE_WIFI
   #ifndef USE_ESP32
    server.handleClient();
   #endif // USE_ESP32
  #endif // USE_WIFI
}

// ****************************************************************************

void SetNextPixel(int PixelID)
{
    uint8_t NewColor;
  
    // randomize all value for pixel driving
    NewColor = random(NUMBER_OF_COLOR);
    PixRed  [PixelID]  = Color[NewColor].Red;
    PixGreen[PixelID]  = Color[NewColor].Green;
    PixBlue [PixelID]  = Color[NewColor].Blue;
    
    if(GoCrazy != 0)
    {
        PixTiming[PixelID] = millis() + random(MIN_RANDOM_TIMING_CRAZY, MAX_RANDOM_TIMING_CRAZY);
    }
    else
    {
        PixTiming[PixelID] = millis() + random(MIN_RANDOM_TIMING, MAX_RANDOM_TIMING);
    }
}

// ****************************************************************************

#ifdef USE_WIFI

// ****************************************************************************

void SetupWifi()
{
  #ifdef USE_ESP32
    WiFi.mode(WIFI_AP);
  #endif // USE_ESP32
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(1000);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP Address IP: ");
    Serial.println(IP);

    // Print ESP8266 Local IP Address
    Serial.println(WiFi.localIP());
   
  #ifdef USE_ESP32
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("GPIO7 Status: OFF | GPIO6 Status: OFF");
        request->send(200, "/text.html",  SendHTML(0));
    });

    server.on("/All", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Toutes les Lucioles sweep");
        request->send(200, "/text.html",  SendHTML(LUCIOLE_1 | LUCIOLE_2 | LUCIOLE_3));
    });

    server.on("/luc1", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Luciole 1 sweep");
        request->send(200, "/text.html",  SendHTML(LUCIOLE_1));
    });

    server.on("/luc2", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Luciole 2 sweep");
        request->send(200, "/text.html",  SendHTML(LUCIOLE_2));
    });

    server.on("/luc3", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Luciole 3 sweep");
        request->send(200, "/text.html",  SendHTML(LUCIOLE_3));
    });
  #else // !USE_ESP32
    server.on("/",          HandleOnConnect);
    server.on("/All",       HandleAll);
    server.on("/luc1",      HandleLuciole_1);
    server.on("/luc2",      HandleLuciole_2);
    server.on("/luc3",      HandleLuciole_3);
    server.onNotFound(HandleNotFound);
  #endif // !USE_ESP32

    server.begin();
    Serial.println("Serveur HTTP Prêt");
}

// ****************************************************************************

#ifndef USE_ESP32

void HandleOnConnect()
{
    Serial.println("GPIO7 Status: OFF | GPIO6 Status: OFF");
    server.send(200, "text/html", SendHTML(0)); 
}

void HandleAll()
{
    Serial.println("Toutes les Lucioles sweep");
    server.send(200, "text/html", SendHTML(LUCIOLE_1 | LUCIOLE_2 | LUCIOLE_3)); 
}

void HandleLuciole_1()
{
    Serial.println("Luciole 1 sweep");
    server.send(200, "text/html", SendHTML(LUCIOLE_1)); 
}

void HandleLuciole_2()
{
    Serial.println("Luciole 2 sweep");
    server.send(200, "text/html", SendHTML(LUCIOLE_2)); 
}

void HandleLuciole_3()
{
    Serial.println("Luciole 3 sweep");
    server.send(200, "text/html", SendHTML(LUCIOLE_3)); 
}

void HandleNotFound()
{
    server.send(404, "text/plain", "404, Non Trouvé");
}
#endif // !USE_ESP32

// ****************************************************************************

String SendHTML(uint8_t Lucioles)
{
    String ptr = "<!DOCTYPE html> <html>\n";
    ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
    ptr +="<title>Contrôle des lucioles</title>\n";
    ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    
    
    ptr +="<div class=\"container\">";
    ptr +="<div class=\"led-box\"><div class=\"led-green\"></div><p>Green LED</p></div>";
    ptr +="<div class=\"led-box\"><div class=\"led-yellow\"></div><p>Yellow LED</p></div>";
    ptr +="<div class=\"led-box\"><div class=\"led-red\"></div><p>Red LED</p></div>";
    ptr +="<div class=\"led-box\"><div class=\"led-blue\"></div><p>Blue LED</p></div>";
    ptr +="</div>";    
    
    // CSS
    ptr +=".container {";
    ptr +="background-size: cover;";
    ptr +="background: rgb(226,226,226);"; /* Old browsers */
    ptr +="background: -moz-linear-gradient(top,  rgba(226,226,226,1) 0%, rgba(219,219,219,1) 50%, rgba(209,209,209,1) 51%, rgba(254,254,254,1) 100%);"; /* FF3.6+ */
    ptr +="       background: -webkit-gradient(linear, left top, left bottom, color-stop(0%,rgba(226,226,226,1)), color-stop(50%,rgba(219,219,219,1)), color-stop(51%,rgba(209,209,209,1)), color-stop(100%,rgba(254,254,254,1)));"; /* Chrome,Safari4+ */
    ptr +="       background: -webkit-linear-gradient(top,  rgba(226,226,226,1) 0%,rgba(219,219,219,1) 50%,rgba(209,209,209,1) 51%,rgba(254,254,254,1) 100%);"; /* Chrome10+,Safari5.1+ */
    ptr +="       background: -o-linear-gradient(top,  rgba(226,226,226,1) 0%,rgba(219,219,219,1) 50%,rgba(209,209,209,1) 51%,rgba(254,254,254,1) 100%);"; /* Opera 11.10+ */
    ptr +="       background: -ms-linear-gradient(top,  rgba(226,226,226,1) 0%,rgba(219,219,219,1) 50%,rgba(209,209,209,1) 51%,rgba(254,254,254,1) 100%);"; /* IE10+ */
    ptr +="       background: linear-gradient(to bottom,  rgba(226,226,226,1) 0%,rgba(219,219,219,1) 50%,rgba(209,209,209,1) 51%,rgba(254,254,254,1) 100%);"; /* W3C */
    ptr +="filter: progid:DXImageTransform.Microsoft.gradient( startColorstr='#e2e2e2', endColorstr='#fefefe',GradientType=0 );"; /* IE6-9 */
    ptr +="padding: 20px;";
    ptr +="}";

    ptr +=".led-box {";
    ptr +="height: 30px;";
    ptr +="width: 25%;";
    ptr +="margin: 10px 0;";
    ptr +="float: left;";
    ptr +="}";

    ptr +=".led-box p {";
    ptr +="font-size: 12px;";
    ptr +="text-align: center;";
    ptr +="margin: 1em;";
    ptr +="}";

    ptr +=".led-red {";
    ptr +="margin: 0 auto;";
    ptr +="width: 24px;";
    ptr +="height: 24px;";
    ptr +="background-color: #F00;";
    ptr +="border-radius: 50%;";
    ptr +="box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #441313 0 -1px 9px, rgba(255, 0, 0, 0.5) 0 2px 12px;";
    ptr +="-webkit-animation: blinkRed 0.5s infinite;";
    ptr +="-moz-animation: blinkRed 0.5s infinite;";
    ptr +="-ms-animation: blinkRed 0.5s infinite;";
    ptr +="-o-animation: blinkRed 0.5s infinite;";
    ptr +="animation: blinkRed 0.5s infinite;";
    ptr +="}";

    ptr +="@-webkit-keyframes blinkRed {";
    ptr +="from { background-color: #F00; }";
    ptr +="50% { background-color: #A00; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #441313 0 -1px 9px, rgba(255, 0, 0, 0.5) 0 2px 0;}";
    ptr +="to { background-color: #F00; }";
    ptr +="}";
    ptr +="@-moz-keyframes blinkRed {";
    ptr +="from { background-color: #F00; }";
    ptr +="50% { background-color: #A00; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #441313 0 -1px 9px, rgba(255, 0, 0, 0.5) 0 2px 0;}";
    ptr +="to { background-color: #F00; }";
    ptr +="}";
    ptr +="@-ms-keyframes blinkRed {";
    ptr +="from { background-color: #F00; }";
    ptr +="50% { background-color: #A00; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #441313 0 -1px 9px, rgba(255, 0, 0, 0.5) 0 2px 0;}";
    ptr +="to { background-color: #F00; }";
    ptr +="}";
    ptr +="@-o-keyframes blinkRed {";
    ptr +="from { background-color: #F00; }";
    ptr +="50% { background-color: #A00; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #441313 0 -1px 9px, rgba(255, 0, 0, 0.5) 0 2px 0;}";
    ptr +="to { background-color: #F00; }";
    ptr +="}";
    ptr +="@keyframes blinkRed {";
    ptr +="from { background-color: #F00; }";
    ptr +="50% { background-color: #A00; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #441313 0 -1px 9px, rgba(255, 0, 0, 0.5) 0 2px 0;}";
    ptr +="to { background-color: #F00; }";
    ptr +="}";

    ptr +=".led-yellow {";
    ptr +="margin: 0 auto;";
    ptr +="width: 24px;";
    ptr +="height: 24px;";
    ptr +="background-color: #FF0;";
    ptr +="border-radius: 50%;";
    ptr +="box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #808002 0 -1px 9px, #FF0 0 2px 12px;";
    ptr +="-webkit-animation: blinkYellow 1s infinite;";
    ptr +="-moz-animation: blinkYellow 1s infinite;";
    ptr +="-ms-animation: blinkYellow 1s infinite;";
    ptr +="-o-animation: blinkYellow 1s infinite;";
    ptr +="animation: blinkYellow 1s infinite;";
    ptr +="}";

    ptr +="@-webkit-keyframes blinkYellow {";
    ptr +="from { background-color: #FF0; }";
    ptr +="50% { background-color: #AA0; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #808002 0 -1px 9px, #FF0 0 2px 0; }";
    ptr +="to { background-color: #FF0; }";
    ptr +="}";
    ptr +="@-moz-keyframes blinkYellow {";
    ptr +="from { background-color: #FF0; }";
    ptr +="50% { background-color: #AA0; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #808002 0 -1px 9px, #FF0 0 2px 0; }";
    ptr +="to { background-color: #FF0; }";
    ptr +="}";
    ptr +="@-ms-keyframes blinkYellow {";
    ptr +="from { background-color: #FF0; }";
    ptr +="50% { background-color: #AA0; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #808002 0 -1px 9px, #FF0 0 2px 0; }";
    ptr +="to { background-color: #FF0; }";
    ptr +="}";
    ptr +="@-o-keyframes blinkYellow {";
    ptr +="from { background-color: #FF0; }";
    ptr +="50% { background-color: #AA0; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #808002 0 -1px 9px, #FF0 0 2px 0; }";
    ptr +="to { background-color: #FF0; }";
    ptr +="}";
    ptr +="@keyframes blinkYellow {";
    ptr +="from { background-color: #FF0; }";
    ptr +="50% { background-color: #AA0; box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #808002 0 -1px 9px, #FF0 0 2px 0; }";
    ptr +="to { background-color: #FF0; }";
    ptr +="}";

    ptr +=".led-green {";
    ptr +="margin: 0 auto;";
    ptr +="width: 24px;";
    ptr +="height: 24px;";
    ptr +="background-color: #ABFF00;";
    ptr +="border-radius: 50%;";
    ptr +="box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #304701 0 -1px 9px, #89FF00 0 2px 12px;";
    ptr +="}";

    ptr +=".led-blue {";
    ptr +="margin: 0 auto;";
    ptr +="width: 24px;";
    ptr +="height: 24px;";
    ptr +="background-color: #24E0FF;";
    ptr +="border-radius: 50%;";
    ptr +="box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #006 0 -1px 9px, #3F8CFF 0 2px 14px;";
    ptr +="}";
    
    // JS
    ptr +="$( function() {";
    ptr +="var $winHeight = $( window ).height()";
    ptr +="$( '.container' ).height( $winHeight );";
    ptr +="});";

    // Button
    ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
    ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
    ptr +=".button-on {background-color: #1abc9c;}\n";
    ptr +=".button-on:active {background-color: #16a085;}\n";
    ptr +=".button-off {background-color: #34495e;}\n";
    ptr +=".button-off:active {background-color: #2c3e50;}\n";
    ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
    ptr +="</style>\n";
    ptr +="</head>\n";
    ptr +="<body>\n";
    ptr +="<h1>Lucioles Magiques</h1>\n";
    ptr +="<h3>Copyright (c) Alain Royer 2020</h3>\n";
  
    if(Lucioles & LUCIOLE_1) { ptr +="<p>Lucioles 1: ON  </p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n"; }
    else                     { ptr +="<p>Lucioles 1: OFF </p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";    }
    if(Lucioles & LUCIOLE_2) { ptr +="<p>Lucioles 2: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";   }
    else                     { ptr +="<p>Lucioles 2: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";     }
    if(Lucioles & LUCIOLE_3) { ptr +="<p>Lucioles 3: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";   }
    else                     { ptr +="<p>Lucioles 3: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";     }

    ptr +="</body>\n";
    ptr +="</html>\n";
    return ptr;
}

// ****************************************************************************

#endif // USE_WIFI
