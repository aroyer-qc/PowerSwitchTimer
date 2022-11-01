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
#define TZ    "UTC + 5" // configuration should be done in firmware

//-------------------------------------------------------------------------------------------------
// Include file(s)
//-------------------------------------------------------------------------------------------------

#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <ESP32TimerInterrupt.h>
//#include <ESP32_ISR_Timer-Impl.h>
//#include <ESP32_ISR_Timer.h>

//****************************************************************
// from susnset sunrise example
// Time library - https://github.com/PaulStoffregen/Time
#include "Time.h"
#include <math.h>

//****************************************************************

//-------------------------------------------------------------------------------------------------
// Define(s)
//-------------------------------------------------------------------------------------------------

// Sunrise stuff
#define PI 3.1415926
#define ZENITH -.83
/*
#define Stirling_Latitude 56.138607900000000000
#define Stirling_Longitude -3.944080100000064700
*/

// Lat long for your location
#define HERE_LATITUDE  56.1386079
#define HERE_LONGITUDE -3.9440801

// LED - blinks on trigger events
#define LED_BUILTIN             2
// Relay pins
#define RELAY_1                 9
#define RELAY_2                 10

#define TIMER_INTERVAL_MS       1000
//#define MIN_RANDOM_TIMING       4000                 // 1000 = 1 sec
//#define MAX_RANDOM_TIMING       20000

//#define MIN_RANDOM_TIMING_CRAZY 100                 // 1000 = 1 sec
//#define MAX_RANDOM_TIMING_CRAZY 500

#define DelayHasEnded(DELAY)  ((millis() > DELAY) ? true : false)

// ****************************************************************************
// Variables 

const char* ssid     = "AR_WIFI";   
const char* password = "the wifi password";

// Put IP Address details
IPAddress local_IP(192,168,1,1);
IPAddress Gateway(192,168,1,1);
IPAddress Subnet(255,255,255,0);
IPAddress primaryDNS(8, 8, 8, 8);   // put avahi RPI local DNS
IPAddress secondaryDNS(8, 8, 4, 4); // optional

AsyncWebServer server(80);          // Set to port 80 as server

uint32_t TickCounter;

ESP32Timer Timer(0);

//bool IsIsTimeToUpdate = false;
uint8_t Button;


int thisYear   = 2015;
int thisMonth  = 6;
int thisDay    = 27;
int lastDay    = 0;
int thisHour   = 0;
int thisMinute = 0;
int thisSecond = 0;

float thisLat  = HERE_LATITUDE;
float thisLong = HERE_LONGITUDE;
float thisLocalOffset = 0;
int   thisDaylightSavings = 1;


#if 0 // from sunset sunrise

// Create USB serial port
//USBSerial serial_debug;

// Digital clock variables
byte omm = 0;
//byte ss=0;
byte xcolon = 0;

#endif



// ****************************************************************************

void IRAM_ATTR TimerHandler(void)
{
    TickCounter++;
    static bool Toggle = false;

    if((TickCounter % 2000) == 0)
    {
        digitalWrite(LED_BUILTIN, Toggle);
        Toggle = !Toggle;
    }
}

// ****************************************************************************

void setup()
{
    // Should pickup actual time from the router
    
    Serial.begin(115200);
    Serial.println();
    Serial.println("Power Switch Timer");
    Serial.println("Initialiser serveur");
    SetupWifi();
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
  
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
//                if(DelayHasEnded(PixTiming[Pixel]))
//                {
//  Serial.printf("%d - %d \r\n", Pixel, PixPercent[PixStep[Pixel]]);
//                }
    
    //Serial.println("Timer millis() = " + String(millis()));
    //Serial.printf("%ld", TickCounter);

    // Show time, date, sunrise and sunset
    tt = rt.getTime();
    thisYear = year(tt);
    thisMonth = month(tt);
    thisDay = day(tt);
    thisHour = hour(tt);
    thisMinute = minute(tt);
    thisSecond = second(tt);

    //  showTime();
    // Send time to web page
    if(lastDay != thisDay)
    {
        //showDate();
        // Send date to web page
    }
    lastDay = thisDay;

    showSunrise();
    // Send Sunrise to web page

    showSunset();
    // Send Sunset to web page

    // serialCurrentTime();

}

// ****************************************************************************

void SetupWifi()
{
    WiFi.mode(WIFI_STA);

    // Configures static IP address
    if (WiFi.config(Local_IP, Gateway, Subnet, PrimaryDNS, SecondaryDNS) == 0)
    {
        Serial.println("STA Failed to configure");
    }

    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi ..");
    
    while(WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(500);
    }
    
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC:  ");
    Serial.println(WiFi.macAddress());
    Serial.print("RRSI: ");
    Serial.println(WiFi.RSSI());
    Serial.println("");
  
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Status 1: OFF | Status 2: OFF");
        request->send(200, "/text.html",  SendHTML(0));
    });

    server.on("/BT1", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Button 1");
        request->send(200, "/text.html",  SendHTML(BUTTON_1));
    });

    server.on("/BT2", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Button 2");
        request->send(200, "/text.html",  SendHTML(BUTTON_2));
    });

    server.on("/BT", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Button 3");
        request->send(200, "/text.html",  SendHTML(BUTTON_3));
    });

    server.begin();
    Serial.println("Serveur HTTP Prêt");
}

// ****************************************************************************

String SendHTML(uint8_t Buttons)
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
    ptr +="<h1>Power Switch Timer</h1>\n";
    ptr +="<h3>Copyright (c) Alain Royer 2020</h3>\n";
  
    if(Buttons & BUTTON_1) { ptr +="<p>Buttons 1: ON  </p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n"; }
    else                   { ptr +="<p>Buttons 1: OFF </p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";    }
    if(Buttons & BUTTON_2) { ptr +="<p>Buttons 2: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";   }
    else                   { ptr +="<p>Buttons 2: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";     }
    if(Buttons & BUTTON_3) { ptr +="<p>Buttons 3: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";   }
    else                   { ptr +="<p>Buttons 3: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";     }

    ptr +="</body>\n";
    ptr +="</html>\n";
    return ptr;
}

// ****************************************************************************

float calculateSunrise(int year, int month, int day, float lat, float lng, int localOffset, int daylightSavings)
{
    boolean rise = 1;
    return calculateSunriseSunset(year, month, day, lat, lng, localOffset, daylightSavings, rise) ;
}

// ****************************************************************************

float calculateSunset(int year, int month, int day, float lat, float lng, int localOffset, int daylightSavings)
{
    boolean rise = 0;
    return calculateSunriseSunset(year, month, day, lat, lng, localOffset, daylightSavings, rise) ;
}

// ****************************************************************************

float calculateSunriseSunset(int year, int month, int day, float lat, float lng, int localOffset, int daylightSavings, boolean rise)
{
    // localOffset will be <0 for western hemisphere and >0 for eastern hemisphere
    // daylightSavings should be 1 if it is in effect during the summer otherwise it should be 0

    // 1. first calculate the day of the year
    float N1 = floor(275 * month / 9);
    float N2 = floor((month + 9) / 12);
    float N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    float N = N1 - (N2 * N3) + day - 30;

    // 2. convert the longitude to hour value and calculate an approximate time
    float lngHour = lng / 15.0;

    float t = 0;
    
    if(rise != 0)
    {
        t = N + ((6 - lngHour) / 24);   //if rising time is desired:
    }
    else
    {
        t = N + ((18 - lngHour) / 24);   //if setting time is desired:
    }
    
    // 3. calculate the Sun's mean anomaly
    float M = (0.9856 * t) - 3.289;

    // 4. calculate the Sun's true longitude
    float L = fmod(M + (1.916 * sin((PI / 180) * M)) + (0.020 * sin(2 * (PI / 180) * M)) + 282.634, 360.0);

    // 5a. calculate the Sun's right ascension
    float RA = fmod(180 / PI * atan(0.91764 * tan((PI / 180) * L)), 360.0);

    // 5b. right ascension value needs to be in the same quadrant as L
    float Lquadrant  = floor( L / 90) * 90;
    float RAquadrant = floor(RA / 90) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    // 5c. right ascension value needs to be converted into hours
    RA = RA / 15;

    // 6. calculate the Sun's declination
    float sinDec = 0.39782 * sin((PI / 180) * L);
    float cosDec = cos(asin(sinDec));

    // 7a. calculate the Sun's local hour angle
    float cosH = (sin((PI / 180) * ZENITH) - (sinDec * sin((PI / 180) * lat))) / (cosDec * cos((PI / 180) * lat));
  
    /*
    if (cosH >  1)
    the sun never rises on this location (on the specified date)
    if (cosH < -1)
    the sun never sets on this location (on the specified date)
    */

    // 7b. finish calculating H and convert into hours
    float H = 0;
  
    if(rise != 0)
    {
        //Serial.print("#sunrise");
        H = 360 - (180 / PI) * acos(cosH); //   if if rising time is desired:
        //Serial.println(H);
    }
    else
    {
        //Serial.print("# sunset ");
        H = (180 / PI) * acos(cosH); // if setting time is desired:
        //Serial.println(H);
    }
  
    //float H = (180/PI)*acos(cosH) // if setting time is desired:
    H = H / 15;

    //8. calculate local mean time of rising/setting
    float T = H + RA - (0.06571 * t) - 6.622;

    // 9. adjust back to UTC
    float UT = fmod(T - lngHour, 24.0);

    // 10. convert UT value to local time zone of latitude/longitude
    return UT + localOffset + daylightSavings;
}

//-----------------------------------------------------------------------------

void showSunrise()
{
    float localT = calculateSunrise(thisYear, thisMonth, thisDay, thisLat, thisLong, thisLocalOffset, thisDaylightSavings);
    double hours;
    float minutes = modf(localT, &hours) * 60;
    //TFT.print("Sunrise ");
    //TFT.print(uint(hours));
    //TFT.print(":");
    
    if(uint(minutes) < 10)
    {
        //TFT.print("0");
    }
    
    //TFT.print(uint(minutes));
}

//-----------------------------------------------------------------------------

void showSunset()
{
    float localT = calculateSunset(thisYear, thisMonth, thisDay, thisLat, thisLong, thisLocalOffset, thisDaylightSavings);
    double hours;
    float minutes = modf(24 + localT, &hours) * 60;
    //TFT.print("Sunset ");
    //TFT.print(uint(24 + localT));
    //TFT.print(":");
  
    if(uint(minutes) < 10)
    {
        //TFT.print("0");
    }
    //TFT.print(uint(minutes));
}

//-----------------------------------------------------------------------------

void setCurrentTime()
{
    char *arg;
    arg = sCmd.next();
    String thisArg = arg;
    Serial.print("# Time command [");
    Serial.print(thisArg.toInt() );
    Serial.println("]");
    setTime(thisArg.toInt());
    time_t tt = now();
    rt.setTime(tt);
    //serialCurrentTime();
}

//-----------------------------------------------------------------------------

void serialCurrentTime()
{
    Serial.print("# Current time - ");
  
    if(hour(tt) < 10)
    {
        Serial.print("0");
    }
    
    Serial.print(hour(tt));
    Serial.print(":");
    
    if(minute(tt) < 10)
    {
        Serial.print("0");
    }
    
    Serial.print(minute(tt));
    Serial.print(":");
    
    if(second(tt) < 10)
    {
        Serial.print("0");
    }
  
    Serial.print(second(tt));
    Serial.print(" ");
    Serial.print(day(tt));
    Serial.print("/");
    Serial.print(month(tt));
    Serial.print("/");
    Serial.print(year(tt));
    //Serial.println("("TZ")");
}

//-----------------------------------------------------------------------------

void unrecognized(const char *command) {
    Serial.print("# Unknown Command.[");
    Serial.print(command);
    Serial.println("]");
}

//-----------------------------------------------------------------------------

void showTime()
{
    tt = rt.getTime();
    byte mm = minute(tt);
    byte hh = hour(tt);
    byte ss = second(tt);

/*
    if (ss % 2)  // Flash the colon
    {
        TFT.drawChar(':', xcolon, ypos, 7);
        relayOneOff();
        relayTwoOn();
    }
    else
    {
        TFT.drawChar(':', xcolon, ypos, 7);
        relayOneOn();
        relayTwoOff();
    }
*/  
}

//-----------------------------------------------------------------------------

void showDate()
{
    // Show RTC Time.
    tt = rt.getTime();
    //  TFT.print(day(tt));
    //  TFT.print("-");
    //  TFT.print(month(tt));
    //  TFT.print("-");
    //  TFT.print(year(tt));
}

#endif