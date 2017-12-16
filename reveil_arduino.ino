/**
 * Exemple de code de lecture et d'ajustement de l'heure avec un module RTC DS1307.
 * Compatible Arduino 0023 et Arduino 1.x (et supérieur).
 */

/* Dépendances */
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "DS1307.h"

#define OLED_RESET 4

#define DEBUG 1

/* Rétro-compatibilité avec Arduino 1.x et antérieur */
#if ARDUINO >= 100
#define Wire_write(x) Wire.write(x)
#define Wire_read() Wire.read()
#else
#define Wire_write(x) Wire.send(x)
#define Wire_read() Wire.receive()
#endif

int boardLed = 13;
int externLed = 8;
Adafruit_SSD1306 display(OLED_RESET);


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
  display.display();
}


void oled_sunrise(void) {
  for (int16_t i=0; i<display.width()/2; i+=3) {
    display.drawCircle(display.width()/2, 7, i, WHITE);
  }
  display.fillRect(0, 8, display.width(), display.height(), 0);
  display.display();
  //delay(1000);
}


/** Fonction setup() */
void setup() {
  pinMode(boardLed, OUTPUT);
  pinMode(externLed, OUTPUT);
  /* Initialise le port série */
  if(DEBUG){
    Serial.begin(115200);
  }
  /* Initialise le port I2C */
  Wire.begin();

  /* Compter 2 secondes pour initialiser l'horloge suite à un reset */
  
  /* Vérifie si le module RTC est initialisé */
  DateTime_t now;
  if (read_current_datetime(&now)) {
  //if (1) {
    if(DEBUG){
      Serial.println(F("L'horloge du module RTC n'est pas active !"));
    }
    // Reconfiguration avec une date et heure en dure (pour l'exemple)
    now.seconds = 2;
    now.minutes = 0;
    now.hours = 19; // 12h 0min 0sec
    now.is_pm = 0; 
    now.day_of_week = 1;
    now.days = 11;
    now.months = 12;
    now.year = 17; // 1 dec 2016
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
  display.clearDisplay();
  display.display();

  blinkSequence(boardLed);
  blinkSequence(externLed);
}

/* Retourne l'heure et les minutes au format (100*Heures + Minutes) */
int wakeUpTimeByDay(int dayOfWeek){
    switch(dayOfWeek){
    case 0://Dimanche
    case 7:
      return 730;
      break;
    case 1://Lundi
      return 645;
      break;
    case 2://Mardi
      return 710;
      break;
    case 3://Mercredi
      return 645;
      break;
    case 4://Jeudi
      return 645;
      break;
    case 5://Vendredi
      return 710;
      break;
    case 6://Samedi
      return 730;
      break;
  }
}

int isWakeUpAllowed(int dayOfWeek, int nowHour, int nowMinute){
  int strangeFormat=(100*nowHour)+nowMinute;
  Serial.println(strangeFormat);

  if(strangeFormat > 1302){
    Serial.println(F("Il est trop tard pour allumer"));
    return 0;
  }else if(strangeFormat < wakeUpTimeByDay(dayOfWeek)){
    Serial.println(F("Il est trop tot pour allumer"));
    return 0;
  }else{
    Serial.println(F("Période authorisée de réveil"));
    return 1;
  }


}

/** Fonction loop() */
void loop() {
  
  /* Lit la date et heure courante */
  DateTime_t now;
  if (read_current_datetime(&now)) {
    if(DEBUG){
      Serial.println(F("L'horloge du module RTC n'est pas active !"));
    }
  }else{
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
    if(isWakeUpAllowed(now.day_of_week, now.hours, now.minutes)){
      //digitalWrite(boardLed, LOW);   // turn the LED on (HIGH is the voltage level)
      digitalWrite(externLed, HIGH);   // turn the LED on (HIGH is the voltage level)
      oled_sunrise();
    }else{
      //digitalWrite(boardLed, HIGH);    // turn the LED off by making the voltage LOW
      digitalWrite(externLed, LOW);    // turn the LED off by making the voltage LOW
      oled_night();
    }
  }
  
  
  /* Rafraichissement une fois par seconde */ 
  delay(1000);
}
