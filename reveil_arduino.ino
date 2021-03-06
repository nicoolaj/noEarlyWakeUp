/**
 * Exemple de code de lecture et d'ajustement de l'heure avec un module RTC DS1307.
 * Compatible Arduino 0023 et Arduino 1.x (et supérieur).
 */

/* Dépendances */
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "./DS1307.h"

#define TIME_SCREEN_ON 6
#define DISTANCE_MINIMUM 150 // distance in mm

#define BOARDLED 13
#define EXTERNLED 8
#define BUTTONPRESS 4

#define OLED_RESET 4

#define DEBUG 0

#define TRIGGER_PIN 2
#define ECHO_PIN 3

/* Constantes pour le timeout */
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s

/* Vitesse du son dans l'air en mm/us */
const float SOUND_SPEED = 340.0 / 1000;

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };


/* Rétro-compatibilité avec Arduino 1.x et antérieur */
#if ARDUINO >= 100
#define Wire_write(x) Wire.write(x)
#define Wire_read() Wire.read()
#else
#define Wire_write(x) Wire.send(x)
#define Wire_read() Wire.receive()
#endif

Adafruit_SSD1306 display(OLED_RESET);

int stayScreenOn=0;
int asAlreadyLooped=0;

/** Fonction de conversion BCD -> decimal */
byte bcd_to_decimal(byte bcd) {
  return (bcd / 16 * 10) + (bcd % 16); 
}

/** Fonction de conversion decimal -> BCD */
byte decimal_to_bcd(byte decimal) {
  return (decimal / 10 * 16) + (decimal % 10);
}

/** 
 * Fonction récupérant l'heure et la date courante à partir du module RTC.
 * Place les valeurs lues dans la structure passée en argument (par pointeur).
 * N.B. Retourne 1 si le module RTC est arrêté (plus de batterie, horloge arrêtée manuellement, etc.), 0 le reste du temps.
 */
byte read_current_datetime(DateTime_t *datetime) {
  
  /* Début de la transaction I2C */
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire_write((byte) 0); // Lecture mémoire à l'adresse 0x00
  Wire.endTransmission(); // Fin de la transaction I2C
 
  /* Lit 7 octets depuis la mémoire du module RTC */
  Wire.requestFrom(DS1307_ADDRESS, (byte) 7);
  byte raw_seconds = Wire_read();
  datetime->seconds = bcd_to_decimal(raw_seconds);
  datetime->minutes = bcd_to_decimal(Wire_read());
  byte raw_hours = Wire_read();
  if (raw_hours & 64) { // Format 12h
    datetime->hours = bcd_to_decimal(raw_hours & 31);
    datetime->is_pm = raw_hours & 32;
  } else { // Format 24h
    datetime->hours = bcd_to_decimal(raw_hours & 63);
    datetime->is_pm = 0;
  }
  datetime->day_of_week = bcd_to_decimal(Wire_read());
  datetime->days = bcd_to_decimal(Wire_read());
  datetime->months = bcd_to_decimal(Wire_read());
  datetime->year = bcd_to_decimal(Wire_read());
  
  /* Si le bit 7 des secondes == 1 : le module RTC est arrêté */
  return raw_seconds & 128;
}

/** 
 * Fonction ajustant l'heure et la date courante du module RTC à partir des informations fournies.
 * N.B. Redémarre l'horloge du module RTC si nécessaire.
 */
void adjust_current_datetime(DateTime_t *datetime) { 
  /* Début de la transaction I2C */
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire_write((byte) 0); // Ecriture mémoire à l'adresse 0x00
  Wire_write(decimal_to_bcd(datetime->seconds) & 127); // CH = 0
  Wire_write(decimal_to_bcd(datetime->minutes));
  Wire_write(decimal_to_bcd(datetime->hours) & 63); // Mode 24h
  Wire_write(decimal_to_bcd(datetime->day_of_week));
  Wire_write(decimal_to_bcd(datetime->days));
  Wire_write(decimal_to_bcd(datetime->months));
  Wire_write(decimal_to_bcd(datetime->year));
  Wire.endTransmission(); // Fin de transaction I2C
}

void blinkSequence(int ledPin){
  for(int a=0; a<5; a++){
    digitalWrite(ledPin, HIGH);
    delay(400);
    for(int b=0; b<a ; b++){
      digitalWrite(ledPin, LOW);
      delay(50 - a);
      digitalWrite(ledPin, HIGH);
      delay(50 - a);
    }
  }
  digitalWrite(ledPin, LOW);
}

void oled_night(){
  display.clearDisplay();
  display.drawBitmap(10, 10,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(40, 16,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(70, 10,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(100, 16,  logo16_glcd_bmp, 16, 16, 1);
  display.startscrollleft(0x00, 0x04);
  display.display();
}

void oled_initUp(int color, int ledPin){
  int ledState=LOW;
  for(int16_t i=0; i<display.width(); i=i+2){
    display.drawFastVLine(i, 0, 7, color),
    display.display();
    //delay(1);
    if(i%5 == 0){
      ledState = (ledState?LOW:HIGH);
      digitalWrite(ledPin, ledState);
    }
  }
  digitalWrite(ledPin, LOW);
}

void oled_initDown1(int color, int ledPin){
  int ledState=LOW;
  for(int16_t i=display.height(); i>7; i--){
    display.drawFastHLine(0, i, display.width(), color);
    display.display();
    //delay(1);
    if(i%5 == 0){
      ledState = (ledState?LOW:HIGH);
      digitalWrite(ledPin, ledState);
    }
  }
  digitalWrite(ledPin, LOW);
}
void oled_initDown2(int color, int ledPin){
  int ledState=LOW;
  for(int16_t i=8; i<display.height(); i++){
    display.drawFastHLine(0, i, display.width(), color);
    display.display();
    //delay(1);
    if(i%5 == 0){
      ledState = (ledState?LOW:HIGH);
      digitalWrite(ledPin, ledState);
    }
  }
  digitalWrite(ledPin, LOW);
}

void oled_sunrise(void) {
  boolean switchDisplay;
  for (int16_t i=0; i<display.width()/2; i+=3) {
    display.drawCircle(display.width()/2, 7, i, WHITE);
    display.fillRect(0, 8, display.width(), display.height(), 0);
    display.display();
    delay(5);
  }
  /*
  for(int16_t i=0;i<20;i++){
    if((i%2)==0){
      switchDisplay=true;
    }else{
      switchDisplay=false;
    }
    display.invertDisplay(switchDisplay);
    display.fillRect(0, 8, display.width(), display.height(), WHITE);
    delay(500);
  }
  */
}

void screenOff(){
  display.clearDisplay();
  display.display();
}

/** Fonction setup() */
void setup() {
  pinMode(BOARDLED, OUTPUT);
  pinMode(EXTERNLED, OUTPUT);
  pinMode(2,INPUT_PULLUP);

  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW); // La broche TRIGGER doit être à LOW au repos
  pinMode(ECHO_PIN, INPUT);

  /* Initialise le port série */
  if(DEBUG){
    Serial.begin(9600);
    Serial.print(F("la valeur de arduino est : "));
    Serial.print(ARDUINO);
    Serial.println(F("."));
  }
  /* Initialise le port I2C */
  Wire.begin();
  
  /* Compter 2 secondes pour initialiser l'horloge suite à un reset */
  
  /* Vérifie si le module RTC est initialisé */
  DateTime_t now;
  /*
   * Switch the 2 IF condition in order to set the clock
   */
  if (read_current_datetime(&now)) {
  //if (1) {
    if(DEBUG){
      Serial.println(F("L'horloge du module RTC n'est pas active !"));
    }
    // Reconfiguration avec une date et heure en dure (pour l'exemple)
    now.seconds = 2;
    now.minutes = 31;
    now.hours = 19; // 12h 0min 0sec
    now.is_pm = 1; 
    now.day_of_week = 0;
    now.days = 28;
    now.months = 10;
    now.year = 18; // 1 dec 2016
    adjust_current_datetime(&now);
  }
  if(DEBUG){
    Serial.println(F("Sequence d'initialisation des LED"));
  }

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  // Clear the buffer.
  screenOff();

  oled_sunrise();
  delay(10000);
  /*
  oled_initUp(WHITE,BOARDLED);
  oled_initDown1(WHITE,EXTERNLED);
  display.dim(true);
  oled_initDown1(BLACK,EXTERNLED);
  oled_initDown2(WHITE,EXTERNLED);
  oled_initDown2(BLACK,EXTERNLED);
  display.dim(false);
  oled_initUp(BLACK,BOARDLED);
  */
  screenOff();

  /*  
  blinkSequence(BOARDLED);
  blinkSequence(EXTERNLED);
  */
}

char* nomJour(int numJourSemaine){
  switch(numJourSemaine){
    case 0:
    case 7:
      return (char*) "Dimanche";
    case 1:
      return (char*) "Lundi";
    case 2:
      return (char*) "Mardi";
    case 3:
      return (char*) "Mercredi";
    case 4:
      return (char*) "Jeudi";
    case 5:
      return (char*) "Vendredi";
    case 6:
      return (char*) "Samedi";
  }
  return (char*) "";
}

char* nomMois(int numMois){
  switch(numMois){
    case 1:
      return (char*) "Janvier";
    case 2:
      return (char*) "Fevrier";
    case 3:
      return (char*) "Mars";
    case 4:
      return (char*) "Avril";
    case 5:
      return (char*) "Mai";
    case 6:
      return (char*) "Juin";
    case 7:
      return (char*) "Juillet";
    case 8:
      return (char*) "Aout";
    case 9:
      return (char*) "Septembre";
    case 10:
      return (char*) "Octobre";
    case 11:
      return (char*) "Novembre";
    case 12:
      return (char*) "Décembre";
  }
  return (char*) "";
}

void showDigitalClock(){
  DateTime_t now;
  if (read_current_datetime(&now)) {
    if(DEBUG){
      Serial.println(F("L'horloge du module RTC n'est pas active !"));
    }
  }else{
    char heure[5];
    char jour[20];
    sprintf(heure, "%02d:%02d", now.hours, now.minutes);
    sprintf(jour, "%s %02d %s", nomJour(now.day_of_week), now.days, nomMois(now.months));

    display.clearDisplay();

    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(10,10);
    display.print(heure);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.print(jour);
    display.display();
    delay(2000);
  }
}


/* Retourne l'heure et les minutes au format (100*Heures + Minutes) */
int wakeUpTimeByDay(int dayOfWeek){
    switch(dayOfWeek){
    case 0://Dimanche
    case 7:
      return 720;
      break;
    case 1://Lundi
      return 645;
      break;
    case 2://Mardi
      return 700;
      break;
    case 3://Mercredi
      return 645;
      break;
    case 4://Jeudi
      return 645;
      break;
    case 5://Vendredi
      return 700;
      break;
    case 6://Samedi
      return 720;
      break;
  }
}

int isWakeUpAllowed(int dayOfWeek, int nowHour, int nowMinute){
  int strangeFormat=(100*nowHour)+nowMinute;
  if(DEBUG){
    Serial.println(strangeFormat);
  }

  if(strangeFormat > 2000){
    if(DEBUG){
      Serial.println(F("Il est trop tard pour allumer"));
    }
    return 0;
  }else if(strangeFormat < wakeUpTimeByDay(dayOfWeek)){
    if(DEBUG){
      Serial.println(F("Il est trop tot pour allumer"));
    }
    return 0;
  }else{
    if(DEBUG){
      Serial.println(F("Période authorisée de réveil"));
    }
    return 1;
  }


}

void drawMark(int h){
  // Source : http://www.rinkydinkelectronics.com
  float x1, y1, x2, y2;
  int decalCenterX = 45;

  h=h*30;
  h=h+270;
  
  x1=19*cos(h*0.0175) + decalCenterX;
  y1=19*sin(h*0.0175);
  x2=16*cos(h*0.0175) + decalCenterX;
  y2=16*sin(h*0.0175);
  
  display.drawLine(x1+(display.width()/2), y1+(display.height()/2), x2+(display.width()/2), y2+(display.height()/2),WHITE);
}
void drawMin(int m){
  // Source : http://www.rinkydinkelectronics.com
  float x1, y1, x2, y2, x3, y3, x4, y4;
  int decalCenterX = 45;

  m=m*6;
  m=m+270;
  
  x1=25*cos(m*0.0175) + decalCenterX;
  y1=25*sin(m*0.0175);
  x2=3*cos(m*0.0175) + decalCenterX;
  y2=3*sin(m*0.0175);
  x3=10*cos((m+8)*0.0175) + decalCenterX;
  y3=10*sin((m+8)*0.0175);
  x4=10*cos((m-8)*0.0175) + decalCenterX;
  y4=10*sin((m-8)*0.0175);
  
  display.drawLine(x1+(display.width()/2), y1+(display.height()/2), x3+(display.width()/2), y3+(display.height()/2),WHITE);
  display.drawLine(x3+(display.width()/2), y3+(display.height()/2), x2+(display.width()/2), y2+(display.height()/2),WHITE);
  display.drawLine(x2+(display.width()/2), y2+(display.height()/2), x4+(display.width()/2), y4+(display.height()/2),WHITE);
  display.drawLine(x4+(display.width()/2), y4+(display.height()/2), x1+(display.width()/2), y1+(display.height()/2),WHITE);
}
void drawHour(int h, int m){
  // Source : http://www.rinkydinkelectronics.com
  float x1, y1, x2, y2, x3, y3, x4, y4, x5, y5;
  int decalCenterX = 45;

  h=(h*30)+(m/2);
  h=h+270;
  
  x1=20*cos(h*0.0175) + decalCenterX;
  y1=20*sin(h*0.0175);
  x2=3*cos(h*0.0175) + decalCenterX;
  y2=3*sin(h*0.0175);
  x3=8*cos((h+12)*0.0175) + decalCenterX;
  y3=8*sin((h+12)*0.0175);
  x4=8*cos((h-12)*0.0175) + decalCenterX;
  y4=8*sin((h-12)*0.0175);
  x5=15*cos((h-12)*0.0175) + decalCenterX;
  y5=15*sin((h-12)*0.0175);


  display.drawLine(x1+(display.width()/2), y1+(display.height()/2), x3+(display.width()/2), y3+(display.height()/2),WHITE);
  display.drawLine(x3+(display.width()/2), y3+(display.height()/2), x2+(display.width()/2), y2+(display.height()/2),WHITE);
  display.drawLine(x2+(display.width()/2), y2+(display.height()/2), x4+(display.width()/2), y4+(display.height()/2),WHITE);
  display.drawLine(x4+(display.width()/2), y4+(display.height()/2), x1+(display.width()/2), y1+(display.height()/2),WHITE);
  //display.drawLine(x2+(display.width()/2), y2+(display.height()/2), x5+(display.width()/2), y5+(display.height()/2),WHITE);
  //display.drawLine(x5+(display.width()/2), y5+(display.height()/2), x1+(display.width()/2), y1+(display.height()/2),WHITE);
}
void drawClockFace(int hour, int minute){
  // Source : http://www.rinkydinkelectronics.com
  display.stopscroll();
  for (int i=0; i<2; i++)
  {
    display.drawCircle((display.width()/2)+45, (display.height()/2), (display.height()/2)-i, WHITE);
  }
  /*
  // Center
  for (int i=0; i<3; i++)
  {
    display.drawCircle((display.width()/2), (display.height()/2), i, WHITE);
  }
  */
  // Draw a small mark for every hour
  for (int i=1; i<12; i++)
  {
    drawMark(i);
  }

  //drawMin(minute);
  drawHour(hour, minute);

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(10,10);
  display.print(hour);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(55,17);
  display.print(minute);

  display.display();
  delay(5000);
  //display.drawCircle(display.width()/2, ((display.height()-10)/2)+8, 12, WHITE);
  //display.display();
  //delay(3000);
}

void main_action(){
    /* Lit la date et heure courante */
  DateTime_t now;
  if (read_current_datetime(&now)) {
    if(DEBUG){
      Serial.println(F("L'horloge du module RTC n'est pas active !"));
    }
  }else{
      if(DEBUG){
      /* Affiche la date et heure courante */
        Serial.print(F("Date : "));
        Serial.print(now.days);
        Serial.print(F("/"));
        Serial.print(now.months);
        Serial.print(F("/"));
        Serial.print(now.year + 2000);
        Serial.print(F("  Heure : "));
        Serial.print(now.hours);
        Serial.print(F(":"));
        Serial.print(now.minutes);
        Serial.print(F(":"));
        Serial.println(now.seconds);
      }
      if(isWakeUpAllowed(now.day_of_week, now.hours, now.minutes)){
        digitalWrite(EXTERNLED, HIGH);   // turn the LED on (HIGH is the voltage level)
        display.dim(false); //Do not dim the display
        oled_sunrise();
      }else{
        digitalWrite(EXTERNLED, LOW);    // turn the LED off by making the voltage LOW
        display.dim(true); //Dim the display
        oled_night();
        delay(4500); 
        screenOff();
        drawClockFace(now.hours,now.minutes);
      }
  }
  
  
  /* Rafraichissement une fois par seconde */ 
  //delay(1000);  
}

int distance(){
  int retour=1;
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long measure = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  float distance_mm = measure / 2.0 * SOUND_SPEED;
  
  if(distance_mm>3){
    if(distance_mm < DISTANCE_MINIMUM){
      if(DEBUG){
        Serial.print(F("Range is correct !"));
      }
      retour = 0;
    }
    if(DEBUG >=3 ){
      Serial.print(F("Distance: "));
      Serial.print(distance_mm / 10.0, 2);
      Serial.println(F("cm, "));
    }
  }else{
    if(DEBUG >=3 ){
      Serial.print(F("NO ! : "));
      Serial.println(distance_mm);    
    }
  }
  return(retour);
}

/** Fonction loop() */
void loop() {
    int sensor1Value = distance();
    int sensor2Value = digitalRead(BUTTONPRESS);
    if(!asAlreadyLooped){
      sensor2Value = LOW;
      asAlreadyLooped++;
    }
    
    if(DEBUG){
      Serial.print(F("Button is "));
    }
    if (sensor2Value == LOW){
      if(DEBUG){
        Serial.println(F("Pressed"));
      }
      showDigitalClock();
    }else{
      if(DEBUG){
        Serial.println(F("Released"));
      }
    }
    if(DEBUG){
      Serial.print(F("Distance is "));
    }
    if (sensor1Value == LOW) {
      if(DEBUG){
        Serial.print(F("Between range"));
        digitalWrite(BOARDLED, LOW);
      }
      main_action();
      stayScreenOn = TIME_SCREEN_ON;
      if(DEBUG){
        Serial.print(F(" stayScreenON set to "));
        Serial.println(TIME_SCREEN_ON);
      }
    } else {
      if(DEBUG){
        Serial.print(F("Released"));
        digitalWrite(BOARDLED, HIGH);
      }
      if(stayScreenOn <= 0){
        stayScreenOn = 0;
        screenOff();
        digitalWrite(BOARDLED,LOW);
      }else{
        stayScreenOn--;
        if(DEBUG >=3 ){
          Serial.print(F("  wait"));
        }
        delay(1000);
        if(DEBUG >=3 ){
          Serial.println(F(" 1 sec"));
        }
      }
    }
    if(DEBUG){
      Serial.print(F(" stayScreenON ="));
      Serial.println(stayScreenOn);
    }
}
