/* original from cbm80amiga
// BME280 mini weather station with ST7789 IPS
// Required RREFont, DigiFont and Adafruit_BME280
// (c) 2020 Pawel A. Hernik
// YouTube video: https://youtu.be/MBehHqti35w
// v.0 - Nicu FLORICA (niq_ro) changed to GY-21 sensor (HTU21/SHT21 & Si7021)
// v.0a - cleaned the sketch by artephacts
// v.0c - changed to AHT20 sensor
/*
 ST7789 240x240 IPS (without CS pin) connections (only 6 wires required):
 #01 GND -> GND
 #02 VCC -> VCC (3.3V only!)
 #03 SCL  --|==2k2==|--> D13/PA5/SCK
 #04 SDA ---|==2k2==|--> D11/PA7/MOSI
 #05 RES ---|==2k2==|--> D9 /PA0 or any digital
 #06 DC  ---|==2k2==|--> D10/PA1 or any digital
 #07 BLK -> NC
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#if (__STM32F1__) // bluepill
#define TFT_DC    PA1
#define TFT_RST   PA0
#define BUTTON    PB9
#define DHT11_PIN PA2
//#include <Arduino_ST7789_STM.h>
#else
#define TFT_DC    10
#define TFT_RST   9
#define BUTTON    3

#include <Arduino_ST7789_Fast.h>  // https://github.com/cbm80amiga/Arduino_ST7789_Fast
#endif

#define SCR_WD 240
#define SCR_HT 240
Arduino_ST7789 lcd = Arduino_ST7789(TFT_DC, TFT_RST);

// use only ENABLE_RRE_16B = 1 to save 4KB of flash (in RREFont.h, other ENABLEs should be 0 to save memory)
#include "RREFont.h"  // https://github.com/cbm80amiga/RREFont
#include "rre_term_10x16.h"
RREFont font;
// needed for RREFont library initialization, define your fillRect
void customRectRRE(int x, int y, int w, int h, int c) { return lcd.fillRect(x, y, w, h, c); }  

// -----------------
#include "DigiFont.h"
// needed for DigiFont library initialization, define your customLineH, customLineV and customRect
void customLineH(int x0,int x1, int y, int c) { lcd.drawFastHLine(x0,y,x1-x0+1,c); }
void customLineV(int x, int y0,int y1, int c) { lcd.drawFastVLine(x,y0,y1-y0+1,c); } 
void customRect(int x, int y,int w,int h, int c) { lcd.fillRect(x,y,w,h,c); } 
DigiFont digi(customLineH,customLineV,customRect);
// -----------------


#include <Wire.h>
/*
#include <Adafruit_Sensor.h>
#include <SHT21.h>  // https://github.com/SolderedElectronics/SHT21-Arduino-Library
SHT21 sht;   // i2c
*/

#include "AHT20.h"  //https://github.com/Seeed-Studio/Seeed_Arduino_AHT20
AHT20 AHT;

// --------------------------------------------------------------------------
int stateOld = HIGH;
long btDebounce    = 30;
long btDoubleClick = 600;
long btLongClick   = 700;
long btLongerClick = 2000;
long btTime = 0, btTime2 = 0;
int clickCnt = 1;

// 0=idle, 1,2,3=click, -1,-2=longclick
int checkButton()
{
  int state = digitalRead(BUTTON);
  if( state == LOW && stateOld == HIGH ) { btTime = millis(); stateOld = state; return 0; } // button just pressed
  if( state == HIGH && stateOld == LOW ) { // button just released
    stateOld = state;
    if( millis()-btTime >= btDebounce && millis()-btTime < btLongClick ) { 
      if( millis()-btTime2<btDoubleClick ) clickCnt++; else clickCnt=1;
      btTime2 = millis();
      return clickCnt; 
    } 
  }
  if( state == LOW && millis()-btTime >= btLongerClick ) { stateOld = state; return -2; }
  if( state == LOW && millis()-btTime >= btLongClick ) { stateOld = state; return -1; }
  return 0;
}
//-----------------------------------------------------------------------------
#define RGBIto565(r,g,b,i) ((((((r)*(i))/255) & 0xF8) << 8) | ((((g)*(i)/255) & 0xFC) << 3) | ((((b)*(i)/255) & 0xFC) >> 3)) 

const int dw1=23,dh1=23*2,dw2=56,dh2=56*2-10,sp=3,th1=7,th2=15;
 int yBig = dh1+13;
const int ySmall = 0;
const int yGraph = SCR_HT-65;

const uint16_t v1Col1 = RGBIto565(100,250,100,255);
const uint16_t v1Col2 = RGBIto565(100,250,100,200);
const uint16_t v1Col0 = RGBIto565(100,250,100,50);
const uint16_t v2Col1 = RGBIto565(255,250,100,255);
const uint16_t v2Col2 = RGBIto565(255,250,100,200);
const uint16_t v2Col0 = RGBIto565(255,250,100,50);
const uint16_t v3Col1 = RGBIto565(120,250,250,255);
const uint16_t v3Col2 = RGBIto565(120,250,250,200);
const uint16_t v3Col0 = RGBIto565(120,255,250,50);
const uint16_t v4Col1 = RGBIto565(255,120,120,255);
const uint16_t v4Col2 = RGBIto565(255,120,120,200);
const uint16_t v4Col0 = RGBIto565(255,120,120,50);

char txt[20];
int numVal,numAvg;
#define NUM_VAL 48

class value {
public:
  value() { for(int i=0;i<NUM_VAL;i++) tab[i]=0; };
  //~value() {};
  void init(float v, uint16_t _c0, uint16_t _c1, uint16_t _c2) { cur=avg=min=max=v; c0=_c0; c1=_c1; c2=_c2; }
  float cur;
  float avg;  // average value from last 3600 measurements (60 minutes)
  float min,max; // min/max from last 3600 values (60 minutes)
  uint16_t c0,c1,c2;
  float tab[NUM_VAL]={0};

  void store(float v, bool store)
  {
    cur = v;
    if(cur < min) min=cur;
    if(cur > max) max=cur;
    avg = 0.99*avg + 0.01*cur;
    if(store) {
      for(int i=NUM_VAL-1;i--;) tab[i+1]=tab[i];
      tab[0] = avg;
      avg = min = max = cur;
    }
  }

  void debug(int x, int y, int h, int d=4)
  {
    dtostrf(avg,d,1,txt); digi.printNumber7(txt,x,y);
    dtostrf(min,d,1,txt); digi.printNumber7(txt,x,y+h*1);
    dtostrf(max,d,1,txt); digi.printNumber7(txt,x,y+h*2);
  }

  void graph()
  {
    int i, x=1, y=yGraph;
    int wd=4*48, ht=64;
    float min,max;
    min=max=numVal>0 ? tab[0] : avg;
    for(i=1;i<NUM_VAL;i++) {
      if(tab[i]<=0) continue;
      if(tab[i]<min) min=tab[i];
      if(tab[i]>max) max=tab[i];
    }
    //Serial.println(min); Serial.println(max);
    min = floor(min);
    max = ceil(max);
    //Serial.println(min); Serial.println(max);
    int yv,dy=max-min;
    lcd.drawRect(x-1,y-1,wd+2,ht+2,RGBto565(150,150,150));
    lcd.drawLine(x+wd/2,y,x+wd/2,y+ht,RGBto565(100,100,100));
    digi.setColors(c1,c0);
    digi.setSize7(9,18,3,0);
    digi.setSpacing(2);
    int w = digi.numberWidth("8888");
    dtostrf(max,4,0,txt); digi.printNumber7(txt,SCR_WD-w,y-1);
    dtostrf(min,4,0,txt); digi.printNumber7(txt,SCR_WD-w,y+ht-18+1);
    for(i=0;i<NUM_VAL;i++) {
      if(tab[i]<=0) continue;
      yv=(tab[i]-min)*ht/dy;
      //Serial.print(i);  Serial.print("  ");  Serial.println(yv);
      if(ht-yv>0) lcd.fillRect(x+(NUM_VAL-1-i)*4,y,4,ht-yv,BLACK);
      if(yv>0)    lcd.fillRect(x+(NUM_VAL-1-i)*4,y+ht-yv,4,yv,(i&1)?c1:c2);
    }
  }
};

//-----------------------------------------------------------------------------

value temp, hum;
unsigned long storeTime, ms = 0;
int mode=2,lastMode=-1;
int lastDebugMode=0, debugMode=0;
int wd,x,y;

float h0, h, t;
int ret;

void setup() 
{
  Serial.begin(9600);
  Serial.println("------");
  pinMode(BUTTON, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(400000);  // faster
  lcd.init(SCR_WD, SCR_HT);
  font.init(customRectRRE, SCR_WD, SCR_HT); // custom fillRect function and screen width and height values 
  font.setFont(&rre_term_10x16);
  font.setScale(2);

/*
  temp.init(sht.getTemperature(),v1Col0,v1Col1,v1Col2);
  hum.init(sht.getHumidity(),v2Col0,v2Col1,v2Col2);
*/ 

  AHT.begin(); 
  delay(2000);
  ret = AHT.getSensor(&h0, &t);   
    if(ret)     // GET DATA OK
    {
        Serial.print("humidity: ");
        h = h0*100.;
        Serial.print(h);
        Serial.print("%\t temperature: ");
        Serial.println(t);
        temp.init(t,v1Col0,v1Col1,v1Col2);
        hum.init(h,v2Col0,v2Col1,v2Col2);
    }
    else        // GET DATA FAIL
    {
        Serial.println("GET DATA FROM AHT20 FAIL");
    }
  numVal = numAvg = 0;
  storeTime = millis();
}

void showVal(float v, int x, int y, int w, uint16_t col1, uint16_t col2, uint16_t col0)
{
  dtostrf(v,w,1,txt);
  digi.setColors(col1,col2,col0);
  digi.printNumber7(txt,x,y);
}

/*
void readGY21()
{
  float t = sht.getTemperature();
  float h = sht.getHumidity();
  //bool store = ( millis()-storeTime>2L*1000L );  // store value every 2 seconds
  //bool store = ( millis()-storeTime>60L*60L*1000L );  // store value every 60 minutes
  bool store = ( millis()-storeTime>30L*60L*1000L );  // store value every 30 minutes
  //bool store = ( millis()-storeTime>120*1000L ); // store value every 120 seconds
  if(t>-20 && t<100) temp.store(t,store);
  if(h>0 && h<100) hum.store(h,store);
  numAvg++;
  if(store) {
    storeTime = millis();
    if(++numVal>=NUM_VAL) numVal=NUM_VAL;
    numAvg = 0;
  }
}
*/

void readAHT20()
{
  ret = AHT.getSensor(&h0, &t);   
    if(ret)     // GET DATA OK
    {
        Serial.print("humidity: ");
        h = h0*100;
        Serial.print(h);
        Serial.print("%\t temperature: ");
        Serial.println(t);
    }
    else        // GET DATA FAIL
    {
        Serial.println("GET DATA FROM AHT20 FAIL");
    }
  //bool store = ( millis()-storeTime>2L*1000L );  // store value every 2 seconds
  //bool store = ( millis()-storeTime>60L*60L*1000L );  // store value every 60 minutes
  bool store = ( millis()-storeTime>30L*60L*1000L );  // store value every 30 minutes
  //bool store = ( millis()-storeTime>120*1000L ); // store value every 120 seconds
  if(t>-20 && t<100) temp.store(t,store);
  if(h>0 && h<100) hum.store(h,store);
  numAvg++;
  if(store) {
    storeTime = millis();
    if(++numVal>=NUM_VAL) numVal=NUM_VAL;
    numAvg = 0;
  }
}


void bigT()
{
  digi.setSize7(dw2,dh2,th2,th2/2);
  digi.setSpacing(4);
  wd = digi.numberWidth("88.8");
  x = (SCR_WD-wd-24)/2;
  if(temp.cur<0 || temp.cur>99) return;
  showVal(temp.cur, x,yBig, 4,v1Col1,v1Col2,v1Col0);
  lcd.fillCircle(x+wd+4+12,yBig+12,12,v1Col1);
  lcd.fillCircle(x+wd+4+12,yBig+12,6,v1Col0);
  font.setScale(3,5);
  font.setColor(v1Col0); font.printStr(x+wd+4+3,yBig+3+30,"C");
  font.setColor(v1Col1); font.printStr(x+wd+4,yBig+30,"C");
}

void bigH()
{
  digi.setSize7(dw2,dh2,th2,th2/2);
  digi.setSpacing(4);
  wd = digi.numberWidth("88.8");
  x = (SCR_WD-wd-8*3-3)/2;
  if(hum.cur<0 || hum.cur>99) return;
  showVal(hum.cur, x,yBig, 4,v2Col1,v2Col2,v2Col0);
  font.setScale(3,3);
  font.setColor(v2Col0); font.printStr(x+wd+4+3,yBig+3,"%");
  font.setColor(v2Col1); font.printStr(x+wd+4,yBig,"%");
}

void smallT()
{
  showVal(temp.cur, 0,y, 4,v1Col1,v1Col1,v1Col0);
  font.setScale(2);
  font.setColor(v1Col0); font.printStr(wd+3+3,y+3,"'C");
  font.setColor(v1Col1); font.printStr(wd+3,y,"'C");
}

void smallH(int x)
{
  if(x) {
    wd = digi.numberWidth("88.8");
    x = SCR_WD-wd-8*2-3;
  }
  showVal(hum.cur, x,y, 4,v2Col1,v2Col1,v2Col0);
  font.setScale(2);
  font.setColor(v2Col0); font.printStr(x+wd+3+3,y+3,"%");
  font.setColor(v2Col1); font.printStr(x+wd+3,y,"%");
}

void smallCommon()
{
  //digi.setSize7(dw1,dw1*2,th1,th1/2);
  digi.setSize7(dw1,dw1*2,7,2);
  digi.setSpacing(3);
  wd = digi.numberWidth("88.8");
  y = ySmall;
}

void debug()
{
  digi.setColors(RED,RGBto565(100,0,0));
  digi.setSize7(11,20,3,1);
  digi.setSpacing(2);
  y = yGraph;
  temp.debug(0,y,23,4);
  hum.debug(95-4,y,23,4);
  dtostrf(numAvg,4,0,txt); digi.printNumber7(txt,190,y);  // https://www.programmingelectronics.com/dtostrf/
  dtostrf(numVal,4,0,txt); digi.printNumber7(txt,190,y+23);
}

void mode0()
{
  bigT();
  smallCommon();
  smallH(0);
  debugMode ? debug() : temp.graph();
}

void mode1()
{
  bigH();
  smallCommon();
  smallT();
  debugMode ? debug() : hum.graph();
}

void mode2()
{
  if (!debugMode)
  {
  yBig = 13;
  bigT();
  yBig = 2*dh1+39;
  bigH();
  smallCommon();
  }
  else
  {
 smallCommon();
 smallT();
 smallH(1);
 smallCommon();
 debug();
  }
  yBig = dh1+13;
}

void drawScreen()
{
  if(mode==0) mode0(); else
  if(mode==1) mode1(); else
  if(mode==2) mode2();
}

void loop()
{
  if(mode!=lastMode || debugMode!=lastDebugMode) {
    lastMode = mode;
    lastDebugMode = debugMode;
    lcd.fillScreen(BLACK);
    drawScreen();
  }

  if(millis()-ms>5000) {  // read the sensor
    ms = millis();
    // readGY21();
    readAHT20();
    drawScreen();
  }

  int st = checkButton();
  if(st>0) { if(++mode>2) mode=0; }
  if(st<0) { debugMode=!debugMode; while(digitalRead(BUTTON)==LOW); }
}
