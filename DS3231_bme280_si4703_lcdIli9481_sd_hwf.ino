/*
 * 
 * lcd: ili9481 com lib modigicada mcu...9481
 *     janeiro 2018 agora o codigo cabe em um arduino uno
 *     
 * radio: si4703 com lib modificada para leitura do relogio
 * 
 * rtc: DS3231
 * 
 * clima: bme280 -> Forced Mode
 * 
 * todo:
 *   desligar si4703 
 *   ajuste do modo bme280 ok 
 *   alarme  24c02 256 bytes
 *   rdstime ok
 *   tea5757  
 *   pct2323 
 *   grafico da temperatura1, temperatura2, umidade, pressao
 *   tmrpcm lib
 */



// ------> V E R Y   V E R Y   I M P O R T A N T !  <------
// This exemaple works only with a DS3231 RTC, which has alarms
// It is not possible to have it working with a DS1307 which lacks that feature

 

// ------> V E R Y   I M P O R T A N T !  <------
// CONNECTIONS:
//
// DS3231 SDA --> SDA
// DS3231 SCL --> SCL
// DS3231 VCC --> 5V (can be also 3.3V for DS3231)
// DS3231 GND --> GND
// DS3231 SQW --> DIGITAL PIN 2 or any other pin supporting an interrupt (See table here below)

// ------> For this example you *MUST* connect the DS3231 SQW signal to an interrupt pin!!!

// +-------------------+-----------------------------------+
// |                   |  I N T E R R U P T   N U M B E R  |
// |     B O A R D     +-----+-----+-----+-----+-----+-----+
// |                   |  0  |  1  |  2  |  3  |  4  |  5  |
// +-------------------+-----+-----+-----+-----+-----+-----+
// |Uno, Ethernet pins |  2  |  3  |     |     |     |     |
// +-------------------+-----+-----+-----+-----+-----+-----+
// |Mega2560      pins |  2  |  3  | 21  | 20  | 19  | 18  |
// +-------------------+-----+-----+-----+-----+-----+-----+
// |Leonardo      pins |  3  |  2  |  0  |  1  |  7  |     |
// +-------------------+-----+-----+-----+-----+-----+-----+

#define INTERRUPT_PIN 18

// ------> I M P O R T A N T !  <------
// DEBUG - Uncomment/comment the define here below if you want to have more or less information about what's going on.
// Beware that when DEBUG is defined the RTC is reset with the compile time at every run.
// #define DEBUG


// ------> I M P O R T A N T !  <------
// Adjust SERIAL_SPEED to your needs:
// The default is 9600, but you probably can increase it in your IDE
// and set it here accordingly
#define SERIAL_SPEED 57600



// ------> I M P O R T A N T !  <------
// Uncomment the #define here below if you want to use
// the SoftwareWire library (... and remember to install it!)


//===============================================================================================================//


// Declared volatile so interrupt can safely modify them
// and other code can safely read and modify them
volatile bool interruptFlag = false;
unsigned long loops = 0;

// We NEED the standard C time library...
#include <Si4703_Breakout.h>
//#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <MCUFRIEND_kbv_9481.h> ///////////////////////////////////////////////////////////


#include "TouchScreen.h"

#define YP A8  // must be an analog pin, use "An" notation!
#define XM A9  // must be an analog pin, use "An" notation!
#define YM A10  // can be a digital pin
#define XP A11   // can be a digital pin


uint16_t TS_LEFT = 978;// 800; 978
uint16_t TS_RT  = 134; //196;  134

uint16_t TS_TOP = 840;// 978; 800
uint16_t TS_BOT = 236;// 134; 196

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

uint16_t pref[5][2];

#define USE_SDFAT
#include <SdFat.h>
//#include <SD.h>
SdFat SD;
#include <avr/sleep.h>
#include <avr/power.h>

#define SD_CS 53
File root;
char namebuf[32] = "/";
int pathlen;

uint8_t v=0;

uint8_t sec_t, min_t, hora_t,week_t;
int8_t tpRtc[480],tpBme[480];

int8_t loopp;

String diaSemana[7] = {"Dom","Seg","Ter","Qua","Qui","Sex","Sab"};

char tcdigitado = '\0';

MCUFRIEND_kbv tft;

Si4703_Breakout radio(49, 20, 21);

int channel;
int volume;
int8_t rdsBuffer[10];
uint8_t adj=0;

#include <Wire.h>
#define myWire TwoWire
#define I2C Wire

#define SEALEVELPRESSURE_HPA (1013.25)
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define PIN_SD_CS 10 // Adafruit SD shields and modules: pin 10

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Here where we instantiate our "Rtc" object
#include <RtcDS3231Smpl.h> //////////////////////////////////////////////////////////////////////
RtcDS3231<myWire> Rtc(I2C);

Adafruit_BME280 bme; // I2C

// Scheduler:
// We will have our loop() "freerunning" and we will print out our time only when an alarm is triggered.
// Having the time printed will thus be a demonstartion of alarms working.

// Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
// If using the shield, all control and data lines are fixed, and
// a simpler declaration can optionally be used:
// Adafruit_TFTLCD tft;


// SETUP
void setup() {

  // Setup Serial
  Serial.begin(SERIAL_SPEED);
  
  for (int x=0;x<480;x++){
    tpRtc[x]=(x%120)-40;tpBme[x]=(x%80)-40;
  }

  bmeSetup();
  SdSetup();
  radioSetup();
  lcdSetup();
  rtcSetup();

  
  //set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //set_sleep_mode(SLEEP_MODE_IDLE);

  tftTimeRefresh();

    
  // End of Setup
}

void bmeSetup(){
  
  if (! bme.begin())  Serial.println("errBME, chk wiring!");
  
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
  
}

void SdSetup(){
  bool good = SD.begin(SD_CS);
  if (!good) {
    Serial.print(F("cannot start SD"));
  }
  root = SD.open(namebuf);
  pathlen = strlen(namebuf);
}

void radioSetup(){
  radio.powerOn();
  radio.setVolume(10);
  
}

void lcdSetup(){
  tft.reset();
  tft.begin(0x9481);
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.invertDisplay(true);
  tft.clrScr(BLACK);
  tft.invertDisplay(true);
  }

void rtcSetup(){
  // Setup RTC
  Rtc.Begin();

  // Now we check the health of our RTC and in case we try to "fix it"
  // Common causes for it being invalid are:
  //    1) first time you ran and the device wasn't running yet
  //    2) the battery on the device is low or even missing

  // Check if the RTC clock is running (Yes, it can be stopped, if you wish!)
  if (!Rtc.GetIsRunning())
  {
    Serial.println(F("W1C"));// RTC was not actively running, starting it now.
    Rtc.SetIsRunning(true);
  }


  // Turn off DS3231 32KHz pin
  Rtc.Enable32kHzPin(false);

  // Set the DS3231 SQW pin to be triggered whenever one of the two alarms expires
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmBoth);

  // Alarm 1 can be triggered at most once per second:
  // we will use it to print a timestamp every second
  // DS3231AlarmOne alarm1(0, 0, 0, 0, DS3231AlarmOneControl_OncePerSecond);
  // Rtc.SetAlarmOne(alarm1);

  // Alarm 2 can be triggered at most once per minute:
  // we will use it to print ambient temperature every minute (D3231 has a temperature sensor!)
  DS3231AlarmTwo alarm2(0, 0, 0, DS3231AlarmTwoControl_OncePerMinute);
  Rtc.SetAlarmTwo(alarm2);

  // Throw away any old alarm state before we run
  Rtc.LatchAlarmsTriggeredFlags();

  // Associate the interrupt service routine to a FALLING edge on the board pin connected to the DS3231 SQW pin
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), InterruptServiceRoutine, FALLING);


  /* Check if the time is valid.
#ifndef DEBUG
  if (!Rtc.IsDateTimeValid())
#endif
  {
#ifndef DEBUG
    Serial.println(F("WARNING: RTC invalid time, setting RTC with compile time!"));
#else
    Serial.println(F("DEBUG mode: ALWAYS setting RTC with compile time!"));
#endif
    Rtc.SetTime(&compiled_time_t);
  }*/
  
}


void InterruptServiceRoutine()
{
  // Since this interrupted any other running code, so we keep the code here at minimum
  // and especially avoid any communications calls within this routine
  // We just set a flag that will be checked later in the main loop (where real action will take place)
  // and increment a counter to be displayed there
  interruptFlag = true;
}

void displayInfo()
{
   Serial.print("Ch:Vol:");
   Serial.print(channel); 
   Serial.println(volume); 
}

void loopRadio()
{
  if (Serial.available())
  {
    Serial.println("=");
    char ch = Serial.read();
    channel=0;
    if (ch == 'u') 
    {
      channel = radio.seekUp();
      displayInfo();
    } 
    else if (ch == 'd') 
    {
      channel = radio.seekDown();
      displayInfo();
    } 
    else if (ch == '+') 
    {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == '-') 
    {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == '1')
    {
      channel = 903; // band news  fm +++ ++ muito bom rds time
      displayInfo();
    }
    else if (ch == '2')
    {
      channel = 925; // cbn 
      displayInfo();
    }
    else if (ch == '3')
    {
      channel = 949; // mpb
      displayInfo();
    }
    else if (ch == '4')
    {
      channel = 981; // globo fm
      displayInfo();
    }
    else if (ch == '5')
    {
      channel = 999; // JB 
      displayInfo();
    }
    else if (ch == '6')
    {
      channel = 1013; // transamerica fm +++ ++ muito bom rds time
      displayInfo();
    }
    else if (ch == 'h')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(--hora, minuto, segundo, dia, mes, ano, semana);  
      tftTimeRefresh();
    }
     else if (ch == 'H')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(++hora, minuto, segundo, dia, mes, ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 'm')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, --minuto, segundo, dia, mes, ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 'M')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, ++minuto, segundo, dia, mes, ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 's')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo, dia, mes, ano, --semana);
      tftTimeRefresh();
    }
    else if (ch == 'S')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo, dia, mes, ano, ++semana);
      tftTimeRefresh();
    }
    else if (ch == 'c')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo,--dia, mes, ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 'C')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo, ++dia, mes, ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 't')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo,dia, --mes, ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 'T')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo, dia, ++mes, ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 'a')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo,dia, mes, --ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 'A')
    {
      uint8_t hora, minuto, segundo, dia, mes, ano, semana;
      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana );
      Rtc.SetTime(hora, minuto, segundo, dia, mes, ++ano, semana);
      tftTimeRefresh();
    }
    else if (ch == 'r')
    {
      Serial.println("RDS lst");
      radio.readRDStime(rdsBuffer, 15000);
      Serial.print("RDS h:");
      //Serial.println(rdsBuffer);
      if (rdsBuffer[0]!=0){
        //rds_time_t = utl2Time(rdsBuffer[3],rdsBuffer[4],0,rdsBuffer[0],rdsBuffer[1],rdsBuffer[2]);
        //Rtc.SetTime(&rds_time_t);
        Serial.print("Set");
        uint8_t nulo, semana;
        Rtc.GetTime(&nulo, &nulo, &nulo, &nulo, &nulo, &nulo, &semana );
        Rtc.SetTime(rdsBuffer[3]&0xff,
                    rdsBuffer[4]&0xff,
                    0,
                    rdsBuffer[0]&0xff, 
                    rdsBuffer[1]&0xff,
                    rdsBuffer[2]&0xff, semana);
      }
      for (char xx=0;xx<8;xx++){
        Serial.print("-");
        Serial.print(rdsBuffer[xx]&0xff);
      }
      tftTimeRefresh();
    }
    else if (ch == '=')
    {
      Serial.println("RDS list");
      radio.readRDStime(rdsBuffer, 15000);
      Serial.print("RDS h:");
      
      if (rdsBuffer[0]!=0){
        Serial.print("Sec>");
        //Serial.println(Rtc.GetTime());
      }
      for (char xx=0;xx<8;xx++){
        Serial.print("-");
        Serial.println(rdsBuffer[xx]&0xff);
      }
    }
  }
  if (channel != 0) radio.setChannel(channel);
}

void showWeek(uint8_t week){
  
  if (week != week_t){
    uint8_t dd,c=40;
    tft.fillRect(0,0,480,40, 0x08);
    
    for (dd=0;dd<7;dd++){
      tft.setTextSize(2);
      tft.setCursor(c+dd *55, 14);
      tft.setTextColor(28);
      if (dd+1 == week) {
        tft.setTextColor(WHITE);
        tft.setTextSize(4);
        tft.setCursor(c+dd*55, 6);
        c+=40;
      } 
      tft.print(diaSemana[dd]);
    }
    week_t = week;
  }
  
  
}

void showTime(uint8_t hr ,uint8_t mn){

  //  showBMP 1110 - 1250
  //  showBMF 271 - 380
  //  showfnb sdfat 88 - 120

  char namef[10];
  uint8_t slc,pos=55;
  min_t = mn;

  //uint32_t start = millis();

  if (hr != hora_t){
    tft.fillRect(0,pos,220,114,0);
  
    slc = (hr%100)/10;
    sprintf(namef, "/n%d.fnb",slc);
    showBMFC(namef, 15, pos);

    slc = (hr%100)%10;
    sprintf(namef, "/n%d.fnb", slc);
    showBMFC(namef, 110, pos);

    showBMFC("/dp.fnb", 220, pos);

    hora_t = hr;
  }

  tft.fillRect(260,pos,480,114,0);
  slc = (mn%100)/10;
  sprintf(namef, "/n%d.fnb", slc);
  showBMFC(namef, 260, pos);

  slc = (mn%100)%10;
  sprintf(namef, "/n%d.fnb", slc);
  showBMFC(namef, 360, pos);

  //Serial.print("tm = ");
  //Serial.println(millis() - start);
  
}


void showGraph(float tpRt, float tpBm){

    int p = ((hora_t * 60) + min_t)/3;
    
    tft.drawRect(0,257,479,62, 255);
    tft.fillRect(1,258,477,20, 32);
    tft.fillRect(1,278,477,20, 1);
    tft.fillRect(1,298,477,20, 8);

    tft.fillRect(p-1,258,3,60, 0);
    
    tpRtc[p] = int(tpRt*2);
    tpBme[p] = int(tpBm*2);

    for (int v=0 ;v<480;v++){
      tft.drawPixel(v,(596-tpRtc[v])/2,0xFe8F);
      tft.drawPixel(v,(596-tpBme[v])/2,0x8FEF);
      //tft.drawPixel(v,298-tpRtc[v]/2,0xFFFF);
      //tft.drawPixel(v,298-tpBme[v]/2,0xFFFF);
    }
  
}

uint8_t showBMFC(char *nm, int x, int y)
{
    File bmpFile;
    uint8_t bmpWidth, bmpHeight;    // W+H in pixels
    uint8_t w, h;
    uint8_t ret;
    uint8_t sdbuffer[100];
    uint8_t bitmask,row,col;
    bool first = true;
    int fileSize = 0;
    int rest = 95;

    bmpFile = SD.open(nm);      // Parse BMP header
    fileSize = bmpFile.size()-2;
    bmpWidth = bmpFile.read();
    bmpHeight = bmpFile.read();

    Serial.println(fileSize);
    Serial.println(bmpWidth);
    Serial.println(bmpHeight);
    
    w = bmpWidth;
    h = bmpHeight;
    
    if ((x + w) >= tft.width())       // Crop area to be loaded
        w = tft.width() - x;
    if ((y + h) >= tft.height())      //
        h = tft.height() - y;

    // Set TFT address window to clipped image bounds
    tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
    int lcdidx, lcdleft;
    uint16_t color;
    uint8_t  b;
    bmpFile.seek(2);
    while (bmpFile.available()) { 

        if (fileSize < rest) rest = fileSize; 
        
        bmpFile.read(sdbuffer,rest);
        tft.pushColorsX(sdbuffer, rest, first);
        first = false;
        fileSize -= rest;
        
    }               // end rows
    tft.setAddrWindow(0, 0, tft.width() - 1, tft.height() - 1); //restore full screen
    ret = 0;        // good render
    bmpFile.close();
    return (ret);
}

void sleepNow() {
  sleep_enable();
  sleep_mode();
  
  sleep_disable();
  power_all_enable();
  //Serial.begin(SERIAL_SPEED); 
}

String senhaDigitada = "";

void getTouch(uint16_t *x, uint16_t *y){
  TSPoint tp = ts.getPoint();
    if (tp.z > ts.pressureThreshhold) {

      *x = map(tp.x, TS_LEFT, TS_RT, 0, tft.width());
      *y = map(tp.y, TS_TOP, TS_BOT, 0, tft.height());
      Serial.print("X = "); Serial.print(tp.x);
      Serial.print("\tY = "); Serial.print(tp.y);
      Serial.print("\tPressure = "); Serial.println(tp.z);
    }
    
}



char telaSenhaC[] = {'1','2','3','4','5','6','7','8','9','<','0','>'};

char telaSenha( uint16_t tcx, uint16_t tcy){
  int x,y;
  uint8_t c=0;
  tft.setTextColor(255);
  tft.setTextSize(3);
  tft.fillRect(150,220,177,40,8);
  tft.setCursor(155,230);
  tft.print(senhaDigitada);
  
  for (y=20;y<200;y+=180/4){
    for(x=150;x<330;x+=180/3){
      if (tcx>500){
        tft.fillRect(x,y,180/3-3,180/4-3,2);
        tft.setCursor(x+15,y+10);
        tft.print(telaSenhaC[c]);
        
      }else{
        if (tcx>x && tcy>y && tcx< (x+180/3-3) && tcy<(y+180/4-3) ){
          Serial.print("--------------");
          Serial.println(c);
          return telaSenhaC[c];
        } 
      }
      c+=1;
    }
  }

  if (tcx<500){
    Serial.println("--####-");
  }
  return '\0';
  
}




// Main loop
void loop() {
  //sleepNow();
  loopRadio();
  telaSenha(700,0);
  uint16_t px=999,py=999;
  //testColor();
  //tft.clrScr(v++);
  
  //tft.fillRect(455,166,32,1, loopp++);

  getTouch(&px,&py);
  char digito = telaSenha(px,py);

  if (tcdigitado != digito){
    if (digito != '\0'){
      senhaDigitada += digito;
    }
    tcdigitado = digito;
  }
  
  tft.fillRect(px,py,10,10, 255);
  
   
  if (interruptFlag)  // Check if InterruptServiceRoutine() has set the flag
  {
    // Yes, An interrupt has occoured!

    // Reset the flag
    interruptFlag = false;

    // Get the DS3231 Alarm Flag
    // then allows for others to trigger again
    DS3231AlarmFlag alarm_flag = Rtc.LatchAlarmsTriggeredFlags();

    // See if an Alarm2 was triggered. If yes, we print out the temperature
    if (alarm_flag & DS3231AlarmFlag_Alarm2){

      tftTimeRefresh();
      tftClimaRefresh();
      
    }

    // See if an Alarm1 was triggered. If yes, we print out a timestamp
    if (alarm_flag & DS3231AlarmFlag_Alarm1)
    {
      uint8_t nulo, segundo;
      Rtc.GetTime(&nulo, &nulo, &segundo, &nulo, &nulo, &nulo, &nulo );
      tft.setTextColor(1);
      tft.setTextSize(2);
      tft.fillRect(455,150,32,16, 0);
      tft.setCursor(455,150);
      tft.print(segundo); 
      
    }
      /* // But first, to play it safe, we check if the clock is reliable (it should, as we got an alarm...)
      if (Rtc.IsDateTimeValid())
      {
      // We print how many interrupts (Alarms) have happened so far...
      // And how many loops since the previous interrupt.
      }
      else
      {
        // Battery on the device is low or even missing and the power line was disconnected
        Serial.println(F("E1C"));//RTC clock has failed!
      }
    }*/

     
  }

  // End of the loop
}

void testColor(){
  
  int x,y;
  uint8_t c=0;
  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  for(x=0;x<240;x+=480/16){
    for (y=0;y<320;y+=320/16){ 
       tft.fillRect(x,y,480/16,320/16,c);
       tft.setCursor(x,y);
       tft.print(c);

       //if (c>=254) c=1;
       c+=1;
       
    }
    c=c+16;
  }
  c=16;
  for(x=240;x<480;x+=480/16){
    for (y=0;y<320;y+=320/16){ 
       tft.fillRect(x,y,480/16,320/16,c);
       tft.setCursor(x,y);
       tft.print(c);

       //if (c>=254) c=1;
       c+=1;
       
    }
    c=c+16;
  }

  
}

void tftTimeRefresh(){

      uint8_t dia,mes,ano,hora,minuto,segundo,semana;
   
      //tft.clrScr(BLACK);
      //tft.fillScreen(BLACK);
      
      tft.setTextColor(WHITE);

      Rtc.GetTime(&hora, &minuto, &segundo, &dia, &mes, &ano, &semana ); 

      showWeek(semana);
      showTime(hora,minuto);

      tft.fillRect(0,186,479,52, 1);
      
      tft.setCursor(0, 190);
      tft.setTextSize(4);

      tft.print(dia);
      tft.print("/");
      tft.print(mes);
      tft.print("/");
      tft.print(ano-100 );
  
}

void tftClimaRefresh(){  

  float tRtc = Rtc.GetTemperature();
  bme.takeForcedMeasurement();
  float tBme = bme.readTemperature();

  tft.fillRect(0,298,479,20, 8);

  int16_t cx,cy;

  tft.setTextSize(2);
  tft.setCursor(200,190);
  
  tft.print(F(" T"));
  tft.setTextSize(4);
  tft.print(int(tRtc));
  tft.setTextSize(2);
  cx = tft.getCursorX();
  cy = tft.getCursorY();
  tft.print(F("o "));
  tft.setCursor(cx,cy+16);
  tft.print(int((tRtc-int(tRtc))*100));
  tft.setCursor(tft.getCursorX(),cy);

  tft.print(F(" S"));
  tft.setTextSize(4);
  tft.print(int(tBme));
  tft.setTextSize(2);
  cx = tft.getCursorX();
  cy = tft.getCursorY();
  tft.print(F("o "));
  tft.setCursor(cx,cy+16);
  tft.println(int((tBme-int(tBme))*100));
     
  tft.print("P=");

  tft.print(bme.readPressure() / 100.0F);
  tft.print(" hPa ");

  tft.print("Hu=");
  tft.print(bme.readHumidity());
  tft.println(" %");

  showGraph(tRtc,tBme);

}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(BLACK);
  tft.fillScreen(BLACK);
  return micros() - start;
}

void testText() {
  tft.fillScreen(BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE);  tft.setTextSize(1);
  tft.println("M");
  return;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(BLACK);

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  tft.fillScreen(BLACK);

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(BLACK);

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(BLACK);

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);

  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(BLACK);
  start = micros();
  for(y=0; y<h; y+=5) tft.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for(i=2; i<n; i+=6) {
    i2 = i / 2;
    tft.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  n = min(tft.width(), tft.height());
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                w = tft.width()  + radius,
                h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for(x=0; x<w; x+=r2) {
    for(y=0; y<h; y+=r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  n     = min(cx, cy);
  start = micros();
  for(i=0; i<n; i+=5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(0, 0, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  start = micros();
  for(i=min(cx,cy); i>10; i-=5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(0, i, i));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(i, i, 0));
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for(i=0; i<w; i+=6) {
    i2 = i / 2;
    tft.drawRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  start = micros();
  for(i=min(tft.width(), tft.height()); i>20; i-=6) {
    i2 = i / 2;
    tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(0, i, 0));
  }

  return micros() - start;
}
