//Steuerung für Gewächshaus aus Make 2_20
//Dr.-Ing. Maximilian Sixt
//Stand: 12.01.2020

#include <Wire.h>
#include <Time.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <DHT.h>
#include <math.h>

//Debug Interface
//#define DEBUG//"Schalter" zum aktivieren

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

//Display
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
volatile boolean HintergrundBeleuchtung = false;

//Lichtsensor
int BH1750_Device = 0x23; 

//EEPROM
int address = 0;

//Sonderzeichen
byte TempInnenLCD[8] = { //a
  0b11100,
  0b01000,
  0b01000,
  0b01111,
  0b01010,
  0b00010,
  0b00010,
  0b00111
};

byte TempAussenLCD[8] = { //b
  0b11100,
  0b01000,
  0b01000,
  0b01010,
  0b01101,
  0b00111,
  0b00101,
  0b00101
};

byte FeuchtInnenLCD[8] = { //c
  0b11100,
  0b01000,
  0b11100,
  0b01000,
  0b01111,
  0b00010,
  0b00010,
  0b00111
};

byte FeuchtAussenLCD[8] = { //d
  0b11100,
  0b01000,
  0b11100,
  0b01000,
  0b01010,
  0b00101,
  0b00111,
  0b00101
};

byte BewaesserungLCD[8] = { //e
  0b00100,
  0b11111,
  0b00010,
  0b01000,
  0b00010,
  0b01000,
  0b00010,
  0b01000
};

byte HeizungLCD[8] = { //f
  0b01010,
  0b01110,
  0b01010,
  0b00000,
  0b01010,
  0b10101,
  0b01010,
  0b10101
};
char TiLCD = 0;
char TaLCD = 1;
char FiLCD = 2;
char FaLCD = 3;
char BeLCD = 4;
char HeLCD = 5;

//Temperatur
float TempInnen;
float TempAussen;
float TempElektronik;
float TempSoll = 27;

#define DHTAussenPIN 43
#define DHTInnenPIN 41
#define DHTElektronikPIN 45
#define DHTTYPE DHT22
DHT dhtInnen(DHTInnenPIN, DHTTYPE);
DHT dhtAussen(DHTAussenPIN, DHTTYPE);
DHT dhtElektronik(DHTElektronikPIN, DHTTYPE);

#define HEIZUNG 5

//Menue
#define BUTTON 19 //INT4
#define ENCODERA 2 //INT0
#define ENCODERB 3 //INT1
#define BUZZER 48
volatile boolean Enc_A = false;
volatile boolean Enc_B = false;
volatile boolean pressed = false;
volatile boolean rotating = false;
boolean MenueStatus = false;
volatile boolean A_set = false;
volatile boolean B_set = false;
volatile boolean MenueStart = false;
volatile boolean EncBFlag = false;
volatile boolean EncAFlag = false;

//Luftfeuchtigkeit
float RHInnen;
float RHAussen;
float RHElektronik;
float RHSoll = 55;

//Bodenfeuchtigkeit
int SoilMoisture[6] = {};
bool SoilMoistureStatus[6] = {false, false, false, false, false, false};
int SMUpperThreshold[6] = {0, 0, 0, 0, 0, 0};

#define SM0 A0
#define SM1 A1
#define SM2 A2
#define SM3 A3
#define SM4 A4
#define SM5 A5

#define PUMPE 6
#define LECKAGEPIN A6

#define VENT0 23
#define VENT1 25
#define VENT2 27
#define VENT3 29
#define VENT4 31
#define VENT5 33

//Blende
#define BLENDE1 40
#define BLENDE2 42
#define BLENDE3 44
#define BLENDE4 38

//Luefter
#define FAN 4

//Licht
#define LED 7
int Licht = 50; //0-99

//Datum
const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
int tag;
String Tage[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
tmElements_t tm;

//Zeitprogramme: So, Mo, Di, Mi, Do, Fr, Sa
int LichtAn[7] = {540, 420, 420, 420, 420, 420, 540};
int LichtAus[7] = {1140, 1140, 1140, 1140, 1140, 1140, 1140};
bool LichtSteuerung[7] = {true, true, true, true, true, true, true};
int PumpeAn[7] = {540, 390, 390, 390, 390, 390, 540};
int PumpeAus[7] = {1200, 1200, 1200, 1200, 1200, 1200, 1200};
bool Bewaesserung[7] = {true, true, true, true, true, true, true};
int TempAn[7] = {360, 360, 360, 360, 360, 360, 360};
int TempAus[7] = {1140, 1140, 1140, 1140, 1140, 1140, 1140};
bool TempRegelung[7] = {true, true, true, true, true, true, true};
int BlendeAn[7] = {540, 420, 420, 420, 420, 420, 540}; //=Fan
int BlendeAus[7] = {1200, 1200, 1200, 1200, 1200, 1200, 1200};; //=Fan
bool RHRegelung[7] = {true, true, true, true, true, true, true};

void Button(void);
void doEncoderA(void);
void doEncoderB(void);

float Verbrauch[4] = {0, 0, 0, 0}; //Stromverbrauch aktuell, letzte 24h, letzte 48h, letzte 72h

void setup(void) {
  pinMode(BLENDE1, OUTPUT);
  pinMode(BLENDE2, OUTPUT);
  pinMode(BLENDE3, OUTPUT);
  pinMode(BLENDE4, OUTPUT);
  pinMode(VENT0, OUTPUT);
  pinMode(VENT1, OUTPUT);
  pinMode(VENT2, OUTPUT);
  pinMode(VENT3, OUTPUT);
  pinMode(VENT4, OUTPUT);
  pinMode(VENT5, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(ENCODERA, INPUT);
  pinMode(ENCODERB, INPUT);
  pinMode(LECKAGEPIN, INPUT);
  digitalWrite(BLENDE1, LOW);
  digitalWrite(BLENDE2, LOW);
  digitalWrite(BLENDE3, LOW);
  digitalWrite(BLENDE4, LOW);
  digitalWrite(VENT0, LOW);
  digitalWrite(VENT1, LOW);
  digitalWrite(VENT2, LOW);
  digitalWrite(VENT3, LOW);
  digitalWrite(VENT4, LOW);
  digitalWrite(VENT5, LOW);
  pinMode(SM0, INPUT);
  pinMode(SM1, INPUT);
  pinMode(SM2, INPUT);
  pinMode(SM3, INPUT);
  pinMode(SM4, INPUT);
  pinMode(SM5, INPUT);
  pinMode(FAN, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(PUMPE, OUTPUT);
  pinMode(HEIZUNG, OUTPUT);
  digitalWrite(FAN, LOW);
  analogWrite(LED, 0);
  digitalWrite(PUMPE, LOW);
  analogWrite(HEIZUNG, 0);
  TCCR3B = (TCCR3B & 0xF8) | 0x05 ; //PWM Frequenz von Pin 5 (Heizung niedirg)
  TCCR4B = (TCCR4B & 0xF8) | 0x01 ; //PWM Frequenz von Pin 7 (LED hoch)
  attachInterrupt(digitalPinToInterrupt(BUTTON), Button, HIGH);
  attachInterrupt(digitalPinToInterrupt(ENCODERA), doEncoderA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODERB), doEncoderB, CHANGE);
  Wire.beginTransmission (BH1750_Device);
  Wire.write(0x10); // Set resolution to 1 Lux
  Wire.endTransmission ();

  lcd.begin(20, 4);
  lcd.createChar(TiLCD, TempInnenLCD);
  lcd.createChar(TaLCD, TempAussenLCD);
  lcd.createChar(FiLCD, FeuchtInnenLCD);
  lcd.createChar(FaLCD, FeuchtAussenLCD);
  lcd.createChar(BeLCD, BewaesserungLCD);
  lcd.createChar(HeLCD, HeizungLCD);
  dhtInnen.begin();
  dhtAussen.begin();
  dhtElektronik.begin();
  Serial.begin(9600);
  setSyncProvider(RTC.get);
}//setup

void save(int wert) {
  //Funktion zum Speichern in den EEPROM. Wird aufgerufen von "Speichern". 
  //Daten werden in zwei Bytes zerlegt, da das Programm die An- bzw. Abschaltzeiten nur als Minuten (max. 24*60 = 1440) kennt und diese in zwei Bytes gespeichert werden müssen. 
  //Daten die nur ein Byte belegen werden im low-Byte gespeichert um Code einfach zu gestalten.
  byte high;
  byte low;
  low = wert % 255;
  high = (wert - low) / 255;
  EEPROM.write(address, low);
  address = address + 1;
  EEPROM.write(address, high);
  address = address + 1;
}//save

int restore(void) {
	//Lädt die Werte des Menüs aus dem EEPROM. Wird aufgerufen von "Wiederherstellen".
  int high;
  int low;
  int value;
  byte EEPROMhigh;
  byte EEPROMlow;
  low = EEPROM.read(address);
  address = address + 1;
  high = 255 * EEPROM.read(address);
  address = address + 1;
  value = high + low;
  return value;
}//restore

void Speichern(void) {
	//Speichert die Menüeinträge in den EEPROM, wird beim Beenden des Menüs automatisch aufgerufen, sie Datei EEPROM.pdf
  address = 0;
  //Temp
  for (int i = 0; i <= 6; i++) {
    save(TempAn[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(TempAus[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(TempRegelung[i]);
  }
  //Licht
  for (int i = 0; i <= 6; i++) {
    save(LichtAn[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(LichtAus[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(LichtSteuerung[i]);
  }
  //Bewaesserung
  for (int i = 0; i <= 6; i++) {
    save(PumpeAn[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(PumpeAus[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(Bewaesserung[i]);
  }
  //Blende
  for (int i = 0; i <= 6; i++) {
    save(BlendeAn[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(BlendeAus[i]);
  }
  for (int i = 0; i <= 6; i++) {
    save(RHRegelung[i]);
  }
  //SM Voreinstellungen
  for (int i = 0; i <= 5; i++) {
    save(SMUpperThreshold[i]);
  }
  save(Licht);
  save(RHSoll);
  save(TempSoll);
}

void Wiederherstellen(void) {
	//Startet das Wiederherstellen der Menüeinträge, siehe auch Datei EEPROM.pdf
  address = 0;
  //Temp
  for (int i = 0; i <= 6; i++) {
    (TempAn[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (TempAus[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (TempRegelung[i]) = restore();
  }
  //Licht
  for (int i = 0; i <= 6; i++) {
    (LichtAn[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (LichtAus[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (LichtSteuerung[i]) = restore();
  }
  //Bewaesserung
  for (int i = 0; i <= 6; i++) {
    (PumpeAn[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (PumpeAus[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (Bewaesserung[i]) = restore();
  }
  //Blende
  for (int i = 0; i <= 6; i++) {
    (BlendeAn[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (BlendeAus[i]) = restore();
  }
  for (int i = 0; i <= 6; i++) {
    (RHRegelung[i]) = restore();
  }
  //SM Voreinstellungen
  for (int i = 0; i <= 5; i++) {
    (SMUpperThreshold[i]) = restore();
  }
  Licht = restore();
  RHSoll = restore();
  TempSoll = restore();
}

void Menue(void) {
	//Menü zur Einstellung der Parameter des Gewächshauses
  String HauptMenueEntry[] = {"Info", "Temperatur>", "Feuchtigkeit>", "Bewaesserung>", "Blende>", "Licht>", "Programme>", "System>"};
  String TempMenueEntry[] = {"Menue", "Sollwert:", "Istwert:"};
  String FeuchtMenueEntry[] = {"Menue", "Sollwert:", "Istwert:"};
  String BewaesserungMenueEntry[] = {"Menue", "Sollwert1:", "Istwert1:", "Kanal1 ", "Sollwert2:", "Istwert2:", "Kanal2 ", "Sollwert3:", "Istwert3:", "Kanal3 ", "Sollwert4:", "Istwert4:", "Kanal4 ", "Sollwert5:", "Istwert5:", "Kanal5 ", "Sollwert6:", "Istwert6:", "Kanal6 "};
  String LichtMenueEntry[] = {"Menue", "Leistung: "};
  String BlendeMenueEntry[] = {"Menue", "Home>"};
  String ProgrammeMenueEntry[] = {"Menue", "Sonntag>", "Montag>", "Dienstag>", "Mittwoch>", "Donnerstag>", "Freitag>", "Samstag>" , "Wiederherstellen"};
  String TagMenueEntry[] = {"Luefter an : ", "Luefter aus: ", "Luefter: ", "Pumpe an : ", "Pumpe aus: ", "Pumpe: ", "Licht an : ", "Licht aus: ", "Licht: ", "Heizung an : ", "Heizung aus: ", "Heizung: "};
  String SystemMenueEntry[] = {"Menue", "TempElektronik:", "Feucht.Elek:", "Uhrzeit ", "Verbr. akt: ", "Verbr. 24h: ", "Verbr. 48h: ", "Verbr. 72h: "};
  int MAXHAUPTMENUE = 7;
  int MAXTEMPMENUE = 1;
  int MAXFEUCHTMENUE = 1;
  int MAXBEWAESSERUNGMENUE = 12;
  int MAXBLENDEMENUE = 1;
  int MAXLICHTMENUE = 1;
  int MAXPROGRAMMEMENUE = 8;
  int MAXTAGMENUE = 12; //Eigentlich 11, einer geht aber für den Wochentag drauf!
  int MAXSYSTEMMENUE = 1;
  int MAXEBENE = 30;
  int counter[MAXEBENE] = {0};
  int MAXCOUNTER[] = {MAXHAUPTMENUE, MAXTEMPMENUE, MAXFEUCHTMENUE, MAXBEWAESSERUNGMENUE, MAXBLENDEMENUE, MAXLICHTMENUE, MAXPROGRAMMEMENUE, MAXSYSTEMMENUE};
  int counterWert = 0;
  int Ebene = 0;
  int counter60 = 0; //Montag
  int counterStunde = tm.Hour;
  int counterMinute = tm.Minute;;
  int LayerIndex = 0;
  int TagIndex = 0;

  MenueStatus = true;
  lcd.backlight();

  digitalWrite(PUMPE, LOW);
  digitalWrite(VENT0, LOW);
  digitalWrite(VENT1, LOW);
  digitalWrite(VENT2, LOW);
  digitalWrite(VENT3, LOW);
  digitalWrite(VENT4, LOW);
  digitalWrite(VENT5, LOW);

  lcd.clear();
  while (Ebene != -1) {
    rotating = false; //Debouncer resetten
    switch (Ebene) {
      case 0: //Hauptmenue Ebene 0 (Ebene)
        if (Enc_A == true) {
          counter[0] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[0] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[0] > MAXCOUNTER[0]) {
          counter[0] = 0;
          lcd.clear();
        }
        if (counter[0] < 0) {
          counter[0] = MAXCOUNTER[0];
          lcd.clear();
        }
        switch (counter[0]) {//Durch Hauptmenü scrollen
          case 0: //Eintrag 0 (Hauptmenü, Zurück) counter[0]
            for (int i = 0; i <= 3; i++) {
              lcd.setCursor(0, i);
              if (counter[0] == i) {
                lcd.write("|");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.write(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = -1; //Zurück zum Infobildschrim
              lcd.clear();
            }
            break;
          case 1: //Eintrag 1 (Hauptmenü, Temperatur) counter[0]
            for (int i = 0; i <= 3; i++) {
              lcd.setCursor(0, i);
              if (counter[0] == i) {
                lcd.write(">");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.write(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = 1; //Temperatur Untermenü
              lcd.clear();
            }
            break;
          case 2: //Eintrag 2 (Hauptmenü, Feuchtigkeit) counter[0]
            for (int i = 0; i <= 3; i++) {
              lcd.setCursor(0, i);
              if (counter[0] == i) {
                lcd.write(">");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.write(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = 2; //Feuchtigkeit Untermenü
              lcd.clear();
            }
            break;
          case 3: //Eintrag 3 (Hauptmenü, Bewässerung) counter[0]
            for (int i = 0; i <= 3; i++) {
              lcd.setCursor(0, i);
              if (counter[0] == i) {
                lcd.write(">");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.print(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = 3; //Bewässerung Untermenü
              lcd.clear();
            }
            break;
          case 4: //Eintrag 4 (Hauptmenü, Blende) counter[0]
            for (int i = 4; i <= 7; i++) {
              lcd.setCursor(0, i - 4);
              if (counter[0] == i) {
                lcd.write(">");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.write(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = 4; //Blende Untermenü
              lcd.clear();
            }
            break;
          case 5: //Eintrag 5 (Hauptmenü, Licht) counter[0]
            for (int i = 4; i <= 7; i++) {
              lcd.setCursor(0, i - 4);
              if (counter[0] == i) {
                lcd.write(">");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.write(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = 5; //Licht Untermenü
              lcd.clear();
            }
            break;
          case 6: //Eintrag 6 (Hauptmenü, Programme) counter[0]
            for (int i = 4; i <= 7; i++) {
              lcd.setCursor(0, i - 4);
              if (counter[0] == i) {
                lcd.write(">");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.write(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = 6; //Programme Untermenü
              lcd.clear();
            }
            break;
          case 7: //Eintrag 7 (Hauptmenü, System) counter[0]
            for (int i = 4; i <= 7; i++) {
              lcd.setCursor(0, i - 4);
              if (counter[0] == i) {
                lcd.write(">");
                lcd.print(HauptMenueEntry[i]);
              }
              else {
                lcd.write(" ");
                lcd.print(HauptMenueEntry[i]);
              }
            }//for
            if (pressed == true) {
              pressed = false;
              Ebene = 7; //System Untermenü
              lcd.clear();
            }
            break;
        }//switch counter[0]
        break;//switch Ebene 0
      //////////////////TEMPERATUR/////////////////////////
      case 1: //Temperatur Untermenü [Ebene]
        if (Enc_A == true) {
          counter[1] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[1] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[1] > MAXCOUNTER[1]) {
          counter[1] = 0;
          lcd.clear();
        }
        if (counter[1] < 0) {
          counter[1] = MAXCOUNTER[1];
          lcd.clear();
        }
        switch (counter[1]) {
          case 0: //Eintrag 0 (Temperatur, Zurück zu Hauptmenü) counter[1]
            lcd.setCursor(0, 0);
            lcd.write("|");
            lcd.print(TempMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.write(" ");
            lcd.print(TempMenueEntry[1] + int(TempSoll) + "C"); //Sollwert
            lcd.setCursor(0, 2);
            lcd.write(" ");
            lcd.print(TempMenueEntry[2] + int(TempInnen) + "C"); //Istwert
            if (pressed == true) {
              pressed = false;
              Ebene = 0; //Hauptmenü
              lcd.clear();
            }
            break;//case0 counter[1]
          case 1: //Eintrag 1 (Temperatur) counter[1]
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.print(TempMenueEntry[0]); //Menü
            lcd.setCursor(0, 1);
            lcd.write(">");
            lcd.print(TempMenueEntry[1] + int(TempSoll) + "C"); //>Sollwert
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.print(TempMenueEntry[2] + int(TempInnen) + "C"); //Istwert
            if (pressed == true) {
              pressed = false;
              Ebene = 8; //Sollwert Temperatur
              lcd.clear();
            }
            break; //case1 counter[1]
        }//switch counter[1]
        break; //case 1 (Ebene)
      /////////////FEUCHTIGKEIT/////////////////////
      case 2: //Feuchtigkeit Ebene
        if (Enc_A == true) {
          counter[2] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[2] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[2] > MAXCOUNTER[2]) {
          counter[2] = 0;
          lcd.clear();
        }
        if (counter[1] < 0) {
          counter[2] = MAXCOUNTER[2];
          lcd.clear();
        }
        switch (counter[2]) { //Feuchtigkeit
          case 0: //Eintrag 0 (Feuchtigkeit, Zurück zu Hauptmenü) counter[2]
            lcd.setCursor(0, 0);
            lcd.print("|" + FeuchtMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(" " + FeuchtMenueEntry[1] + int(RHSoll) + "%"); //Sollwert
            lcd.setCursor(0, 2);
            lcd.print(" " + FeuchtMenueEntry[2] + int(RHInnen) + "%"); //Istwert
            if (pressed == true) {
              pressed = false;
              Ebene = 0; //Zurück zu Hauptmenü
              lcd.clear();
            }
            break;//case0 counter[2]
          case 1: //Eintrag 1 (Feuchtigkeit, Sollwert) counter[2]
            lcd.setCursor(0, 0);
            lcd.print(" " + FeuchtMenueEntry[0]); //Menü
            lcd.setCursor(0, 1);
            lcd.print(">" + FeuchtMenueEntry[1] + int(RHSoll) + "%"); //>Sollwert
            lcd.setCursor(0, 2);
            lcd.print(" " + FeuchtMenueEntry[2] + int(RHInnen) + "%"); //Istwert
            if (pressed == true) {
              pressed = false;
              Ebene = 9; //Feuchtigkeit Untermenü
              lcd.clear();
            }
            break;//case1 counter[2]
        }//switch counter[2]
        break;//case2 (Ebene)
      //////////////BEWÄSSERUNG//////////////////////
      case 3: //Bewässerung Untermenü [Ebene]
        if (Enc_A == true) {
          counter[3] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[3] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[3] > MAXCOUNTER[3]) {
          counter[3] = 0;
          lcd.clear();
        }
        if (counter[3] < 0) {
          counter[3] = MAXCOUNTER[3];
          lcd.clear();
        }
        switch (counter[3]) {
          case 0: //Eintrag 0 (Bewässerung, Zurück zu Hauptmenü) counter[3]
            lcd.setCursor(0, 0);
            lcd.print("|" + BewaesserungMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[1] + int(SMUpperThreshold[0]) + "%"); //Sollwert1
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[2] + int(SoilMoisture[0]) + "%"); //Istwert1
            lcd.setCursor(0, 3);
            lcd.print(" " + BewaesserungMenueEntry[3] + SoilMoistureStatus[0] + " /Manuell"); //Status1/Manuell
            if (pressed == true) {
              pressed = false;
              Ebene = 0; //Hauptmenü
              lcd.clear();
            }
            break;//case0 counter[3]
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%Kanal 1%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          case 1: //Eintrag 1 (Bewässerung, Sollwert1) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(" " + BewaesserungMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(">" + BewaesserungMenueEntry[1] + int(SMUpperThreshold[0]) + "%"); //Sollwert1
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[2] + int(SoilMoisture[0]) + "%"); //Istwert1
            lcd.setCursor(0, 3);
            lcd.print(" " + BewaesserungMenueEntry[3] + SoilMoistureStatus[0] + " /Manuell"); //Status1/Manuell
            if (pressed == true) {
              pressed = false;
              Ebene = 50; //Sollwert1 einstellen
              lcd.clear();
            }
            break;//case1 counter[3]
          case 2: //Eintrag 2 (Bewässerung, Kanal 1 Manuell) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(" " + BewaesserungMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[1] + int(SMUpperThreshold[0]) + "%"); //Sollwert1
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[2] + int(SoilMoisture[0]) + "%"); //Istwert1
            lcd.setCursor(0, 3);
            lcd.print(">" + BewaesserungMenueEntry[3] + SoilMoistureStatus[0] + " /Manuell"); //Status1
            if (pressed == true) {
              pressed = false;
              digitalWrite(VENT0, HIGH);
              delay(500);
              digitalWrite(PUMPE, HIGH);
              DEBUG_PRINTLN("Ventil 1 aktiv");
              delay(3000);
              digitalWrite(PUMPE, LOW);
              digitalWrite(VENT0, LOW);
            }
            break;//case2 counter[3]
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%Kanal 2%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          case 3: //Eintrag 3 (Bewässerung, Sollwert2) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(">" + BewaesserungMenueEntry[4] + int(SMUpperThreshold[1]) + "%"); //Sollwert2
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[5] + int(SoilMoisture[1]) + "%"); //Istwert2
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[6] + SoilMoistureStatus[1] + " /Manuell"); //Status2
            if (pressed == true) {
              pressed = false;
              Ebene = 51; //Sollwert2 einstellen
              lcd.clear();
            }
            break;//case3 counter[3]
          case 4: //Eintrag 4 (Bewässerung, Kanal 2 Manuell) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(" " + BewaesserungMenueEntry[4] + int(SMUpperThreshold[1]) + "%"); //Sollwert2
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[5] + int(SoilMoisture[1]) + "%"); //Istwert2
            lcd.setCursor(0, 2);
            lcd.print(">" + BewaesserungMenueEntry[6] + SoilMoistureStatus[1] + " /Manuell"); //Status2
            if (pressed == true) {
              pressed = false;
              digitalWrite(VENT1, HIGH);
              delay(500);
              digitalWrite(PUMPE, HIGH);
              DEBUG_PRINTLN("Ventil 2 aktiv");
              delay(3000);
              digitalWrite(PUMPE, LOW);
              digitalWrite(VENT1, LOW);
            }
            break;//case4 counter[3]
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%Kanal 3%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          case 5: //Eintrag 5 (Bewässerung, Sollwert3) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(">" + BewaesserungMenueEntry[7] + int(SMUpperThreshold[2]) + "%"); //Sollwert3
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[8] + int(SoilMoisture[2]) + "%"); //Istwert3
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[9] + SoilMoistureStatus[2] + " /Manuell"); //Status3
            if (pressed == true) {
              pressed = false;
              Ebene = 52; //Sollwert3 einstellen
              lcd.clear();
            }
            break;//case5 counter[3]
          case 6: //Eintrag 6 (Bewässerung, Kanal3  Manuell) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(" " + BewaesserungMenueEntry[7] + int(SMUpperThreshold[2]) + "%"); //Sollwert3
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[8] + int(SoilMoisture[2]) + "%"); //Istwert3
            lcd.setCursor(0, 2);
            lcd.print(">" + BewaesserungMenueEntry[9] + SoilMoistureStatus[2] + " /Manuell"); //Status3
            if (pressed == true) {
              pressed = false;
              digitalWrite(VENT2, HIGH);
              delay(500);
              digitalWrite(PUMPE, HIGH);
              DEBUG_PRINTLN("Ventil 3 aktiv");
              delay(3000);
              digitalWrite(PUMPE, LOW);
              digitalWrite(VENT2, LOW);
            }
            break;//case6 counter[3]
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%Kanal 4%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          case 7: //Eintrag 7 (Bewässerung, Sollwert4) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(">" + BewaesserungMenueEntry[10] + int(SMUpperThreshold[3]) + "%"); //Sollwert4
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[11] + int(SoilMoisture[3]) + "%"); //Istwert4
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[12] + SoilMoistureStatus[3] + " /Manuell"); //Status4
            if (pressed == true) {
              pressed = false;
              Ebene = 53; //Sollwert3 einstellen
              lcd.clear();
            }
            break;//case7 counter[3]
          case 8: //Eintrag 8 (Bewässerung, Kanal4 Manuell) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(" " + BewaesserungMenueEntry[10] + int(SMUpperThreshold[3]) + "%"); //Sollwert4
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[11] + int(SoilMoisture[3]) + "%"); //Istwert4
            lcd.setCursor(0, 2);
            lcd.print(">" + BewaesserungMenueEntry[12] + SoilMoistureStatus[3] + " /Manuell"); //Status4
            if (pressed == true) {
              pressed = false;
              digitalWrite(VENT3, HIGH);
              delay(500);
              digitalWrite(PUMPE, HIGH);
              DEBUG_PRINTLN("Ventil 4 aktiv");
              delay(3000);
              digitalWrite(PUMPE, LOW);
              digitalWrite(VENT3, LOW);
            }
            break;//case8 counter[3]
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%Kanal 5%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          case 9: //Eintrag 9 (Bewässerung, Sollwert5) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(">" + BewaesserungMenueEntry[13] + int(SMUpperThreshold[4]) + "%"); //Sollwert5
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[14] + int(SoilMoisture[4]) + "%"); //Istwert5
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[15] + SoilMoistureStatus[4] + " /Manuell"); //Status5
            if (pressed == true) {
              pressed = false;
              Ebene = 54; //Sollwert4 einstellen
              lcd.clear();
            }
            break;//case9 counter[3]
          case 10: //Eintrag 10 (Bewässerung, Kanal 5 Manuell) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(" " + BewaesserungMenueEntry[13] + int(SMUpperThreshold[4]) + "%"); //Sollwert5
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[14] + int(SoilMoisture[4]) + "%"); //Istwert5
            lcd.setCursor(0, 2);
            lcd.print(">" + BewaesserungMenueEntry[15] + SoilMoistureStatus[4] + " /Manuell"); //Status5
            if (pressed == true) {
              pressed = false;
              digitalWrite(VENT4, HIGH);
              delay(500);
              digitalWrite(PUMPE, HIGH);
              DEBUG_PRINTLN("Ventil 5 aktiv");
              delay(3000);
              digitalWrite(PUMPE, LOW);
              digitalWrite(VENT4, LOW);
            }
            break;//case10 counter[3]
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%Kanal 6%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          case 11: //Eintrag 11 (Bewässerung, Sollwert6) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(">" + BewaesserungMenueEntry[16] + int(SMUpperThreshold[5]) + "%"); //Sollwert6
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[17] + int(SoilMoisture[5]) + "%"); //Istwert6
            lcd.setCursor(0, 2);
            lcd.print(" " + BewaesserungMenueEntry[18] + SoilMoistureStatus[5] + " /Manuell"); //Status6
            if (pressed == true) {
              pressed = false;
              Ebene = 55; //Sollwert5 einstellen
              lcd.clear();
            }
            break;//case11 counter[3]
          case 12: //Eintrag 12 (Bewässerung, Kanal6 Manuell) counter[3]
            lcd.setCursor(0, 0);
            lcd.print(" " + BewaesserungMenueEntry[16] + int(SMUpperThreshold[5]) + "%"); //Sollwert6
            lcd.setCursor(0, 1);
            lcd.print(" " + BewaesserungMenueEntry[17] + int(SoilMoisture[5]) + "%"); //Istwert6
            lcd.setCursor(0, 2);
            lcd.print(">" + BewaesserungMenueEntry[18] + SoilMoistureStatus[5] + " /Manuell"); //Status6
            if (pressed == true) {
              pressed = false;
              digitalWrite(VENT5, HIGH);
              delay(500);
              digitalWrite(PUMPE, HIGH);
              DEBUG_PRINTLN("Ventil 6 aktiv");
              delay(3000);
              digitalWrite(PUMPE, LOW);
              digitalWrite(VENT5, LOW);
            }
            break;//case12 counter[3]
        }//switch counter[3]
        break; //case 3: //Bewässerung Menü
      //////////////////////BLENDE//////////////////////
      case 4: //Blende Untermenü [Ebene]
        if (Enc_A == true) {
          counter[4] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[4] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[4] > MAXCOUNTER[4]) {
          counter[4] = 0;
          lcd.clear();
        }
        if (counter[4] < 0) {
          counter[4] = MAXCOUNTER[4];
          lcd.clear();
        }
        switch (counter[4]) {
          case 0: //Eintrag 0 (Blende, Zurück zu Hauptmenü) counter[4]
            lcd.setCursor(0, 0);
            lcd.write("|");
            lcd.print(BlendeMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.write(" ");
            lcd.print(BlendeMenueEntry[1]); //Home
            if (pressed == true) {
              pressed = false;
              Ebene = 0; //Hauptmenü
              lcd.clear();
            }
            break;//case0 counter[51]
          case 1: //Eintrag 1 (HOme) counter[5]
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.print(BlendeMenueEntry[0]); //Menü
            lcd.setCursor(0, 1);
            lcd.write(">");
            lcd.print(BlendeMenueEntry[1]); //>PWM
            if (pressed == true) {
              pressed = false;
              Blende(10); //Home
              lcd.clear();
            }
            break; //case1 counter[5]
        }//switch counter[4]
        break; //case 5 (Ebene)
      ////////////////////////////////LICHT//////////////////////////
      case 5: //Licht Untermenü [Ebene]
        if (Enc_A == true) {
          counter[5] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[5] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[5] > MAXCOUNTER[5]) {
          counter[5] = 0;
          lcd.clear();
        }
        if (counter[5] < 0) {
          counter[5] = MAXCOUNTER[5];
          lcd.clear();
        }
        switch (counter[5]) {
          case 0: //Eintrag 0 (Licht, Zurück zu Hauptmenü) counter[5]
            lcd.setCursor(0, 0);
            lcd.write("|");
            lcd.print(LichtMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.write(" ");
            lcd.print(LichtMenueEntry[1] + int(Licht) + "%"); //PWM
            if (pressed == true) {
              pressed = false;
              Ebene = 0; //Hauptmenü
              lcd.clear();
            }
            break;//case0 counter[51]
          case 1: //Eintrag 1 (Licht PWM) counter[5]
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.print(LichtMenueEntry[0]); //Menü
            lcd.setCursor(0, 1);
            lcd.write(">");
            lcd.print(LichtMenueEntry[1] + int(Licht) + "%"); //>PWM
            if (pressed == true) {
              pressed = false;
              Ebene = 10; //LichtPWM
              lcd.clear();
            }
            break; //case1 counter[5]
        }//switch counter[5]
        break; //case 5 (Ebene)
      ////////////////////////////////PROGRAMME//////////////////////////////////////
      case 6: //Programme Untermenü [Ebene]
        if (Enc_A == true) {
          counter[6] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[6] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[6] > MAXCOUNTER[6]) {
          counter[6] = 0;
          lcd.clear();
        }
        if (counter[6] < 0) {
          counter[6] = MAXCOUNTER[6];
          lcd.clear();
        }
        switch (counter[6]) {
          case 0: //Eintrag 0 (Programme, Zurück zu Hauptmenü) counter[6]
            lcd.setCursor(0, 0);
            lcd.print("|" + ProgrammeMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(" " + ProgrammeMenueEntry[1]); //Montag
            lcd.setCursor(0, 2);
            lcd.print(" " + ProgrammeMenueEntry[2]); //Dienstag
            lcd.setCursor(0, 3);
            lcd.print(" " + ProgrammeMenueEntry[3]); //Mittwoch
            if (pressed == true) {
              pressed = false;
              Ebene = 0; //Hauptmenü
              lcd.clear();
            }
            break;//case0 counter[6]
          case 1: //Eintrag 1 (Programme, Montag) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(">" + ProgrammeMenueEntry[1]); //Montag
            lcd.setCursor(0, 2);
            lcd.print(" " + ProgrammeMenueEntry[2]); //Dienstag
            lcd.setCursor(0, 3);
            lcd.print(" " + ProgrammeMenueEntry[3]); //Mittwoch
            if (pressed == true) {
              pressed = false;
              Ebene = 60; //Montagmenü
              TagIndex = 0;
              lcd.clear();
            }
            break;//case1 counter[6]
          case 2: //Eintrag 2 (Programme, Dienstag) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(" " + ProgrammeMenueEntry[1]); //Montag
            lcd.setCursor(0, 2);
            lcd.print(">" + ProgrammeMenueEntry[2]); //Dienstag
            lcd.setCursor(0, 3);
            lcd.print(" " + ProgrammeMenueEntry[3]); //Mittwoch
            if (pressed == true) {
              pressed = false;
              Ebene = 60; //Dienstagmenü
              TagIndex = 1;
              lcd.clear();
            }
            break;//case2 counter[6]
          case 3: //Eintrag 3 (Programme, Mittwoch) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(" " + ProgrammeMenueEntry[1]); //Montag
            lcd.setCursor(0, 2);
            lcd.print(" " + ProgrammeMenueEntry[2]); //Dienstag
            lcd.setCursor(0, 3);
            lcd.print(">" + ProgrammeMenueEntry[3]); //Mittwoch
            if (pressed == true) {
              pressed = false;
              Ebene = 60; //Mittwochmenü
              TagIndex = 2;
              lcd.clear();
            }
            break;//case3 counter[6]
          case 4: //Eintrag 4 (Programme, Donnerstag) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(">" + ProgrammeMenueEntry[4]); //Donnerstag
            lcd.setCursor(0, 1);
            lcd.print(" " + ProgrammeMenueEntry[5]); //Freitag
            lcd.setCursor(0, 2);
            lcd.print(" " + ProgrammeMenueEntry[6]); //Samstag
            lcd.setCursor(0, 3);
            lcd.print(" " + ProgrammeMenueEntry[7]); //Sonntag
            if (pressed == true) {
              pressed = false;
              Ebene = 60; //Donnerstagmenü
              TagIndex = 3;
              lcd.clear();
            }
            break;//case4 counter[6]
          case 5: //Eintrag 5 (Programme, Freitag) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[4]); //Donnerstag
            lcd.setCursor(0, 1);
            lcd.print(">" + ProgrammeMenueEntry[5]); //Freitag
            lcd.setCursor(0, 2);
            lcd.print(" " + ProgrammeMenueEntry[6]); //Samstag
            lcd.setCursor(0, 3);
            lcd.print(" " + ProgrammeMenueEntry[7]); //Sonntag
            if (pressed == true) {
              pressed = false;
              Ebene = 60; //Freitagmenü
              TagIndex = 4;
              lcd.clear();
            }
            break;//case5 counter[6]
          case 6: //Eintrag 6 (Programme, Samstag) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[4]); //Donnerstag
            lcd.setCursor(0, 1);
            lcd.print(" " + ProgrammeMenueEntry[5]); //Freitag
            lcd.setCursor(0, 2);
            lcd.print(">" + ProgrammeMenueEntry[6]); //Samstag
            lcd.setCursor(0, 3);
            lcd.print(" " + ProgrammeMenueEntry[7]); //Sonntag
            if (pressed == true) {
              pressed = false;
              Ebene = 60; //Samstagmenü
              TagIndex = 5;
              lcd.clear();
            }
            break;//case6 counter[6]
          case 7: //Eintrag 7 (Programme, Sonntag) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[4]); //Donnerstag
            lcd.setCursor(0, 1);
            lcd.print(" " + ProgrammeMenueEntry[5]); //Freitag
            lcd.setCursor(0, 2);
            lcd.print(" " + ProgrammeMenueEntry[6]); //Samstag
            lcd.setCursor(0, 3);
            lcd.print(">" + ProgrammeMenueEntry[7]); //Sonntag
            if (pressed == true) {
              pressed = false;
              Ebene = 60; //Sonntagmenü
              TagIndex = 6;
              lcd.clear();
            }
            break;//case7 counter[6]
          case 8: //Eintrag 7 (Programme, Wiederherstellen) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(">" + ProgrammeMenueEntry[8]); //Wiederherstellen
            if (pressed == true) {
              pressed = false;
              Wiederherstellen();
              lcd.clear();
              lcd.print("Erledigt");
              delay(300);
              lcd.clear();
            }
            break;//case8 counter[6]
        }//switch counter[6]
        break; //case 6 (Ebene)

      case 7: //System [Ebene]
        if (Enc_A == true) {
          counter[7] += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter[7] -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter[7] > MAXCOUNTER[7]) {
          counter[7] = 0;
          lcd.clear();
        }
        if (counter[7] < 0) {
          counter[7] = MAXCOUNTER[7];
          lcd.clear();
        }
        switch (counter[7]) {
          case 0: //Eintrag 0 (System, Zurück zu Hauptmenü) counter[5]
            lcd.setCursor(0, 0);
            lcd.print("|" + SystemMenueEntry[0]); //>Menü
            lcd.setCursor(0, 1);
            lcd.print(" " + SystemMenueEntry[1] + int(TempElektronik) + "C"); //Temp Elektronik
            lcd.setCursor(0, 2);
            lcd.print(" " + SystemMenueEntry[2] + int(RHElektronik) + "%"); //RH Elektronik
            lcd.setCursor(0, 3);
            lcd.print(" " + SystemMenueEntry[3] + Tage[tag] + " " + int(tm.Hour) + ":" + int(tm.Minute));
            if (pressed == true) {
              pressed = false;
              Ebene = 0; //Hauptmenü
              lcd.clear();
            }
            break;//case0 counter[7]
          case 1: //Eintrag 10 (System, Verbrauchsanzeige) counter[5]
            lcd.setCursor(0, 0);
            lcd.print(" " + SystemMenueEntry[4] + int(Verbrauch[0]) + "Wh"); //aktueller Verbrauch
            lcd.setCursor(0, 1);
            lcd.print(" " + SystemMenueEntry[5] + int(Verbrauch[1]) + "Wh"); //Verbrauch letzte 24 h
            lcd.setCursor(0, 2);
            lcd.print(" " + SystemMenueEntry[6] + int(Verbrauch[2]) + "Wh"); //Verbrauch letzte 48 h
            lcd.setCursor(0, 3);
            lcd.print(" " + SystemMenueEntry[7] + int(Verbrauch[3]) + "Wh"); //Verbrauch letzte 72 h
            break;//case0 counter[7]
        }//switch counter[7]
        break; //case 5 (Ebene)
      case 8: //Temp Sollwert (Ebene)
        counterWert = TempSoll;
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 40) {
          counterWert = 40;
        }
        if (counterWert < 0) {
          counterWert = 40;
        }
        lcd.setCursor(0, 1);
        lcd.print(" " + TempMenueEntry[1] + int(counterWert) + "C");
        TempSoll = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 1; //Zurück ins Temperaturmenü
          lcd.clear();
        }
        break;//case8 (Ebene)
      case 9: //Feucht Sollwert (Ebene)
        counterWert = RHSoll;
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 80) {
          counterWert = 80;
        }
        if (counterWert < 10) {
          counterWert = 10;
        }
        lcd.setCursor(0, 1);
        lcd.print(" " + FeuchtMenueEntry[1] + int(counterWert) + "%");
        RHSoll = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 2; //Zurück ins Feuchtigkeitsmenü
          lcd.clear();
        }
        break;//case9 (Ebene)
      case 10: //Licht PWM (Ebene)
        counterWert = Licht;
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 99) {
          counterWert = 0;
        }
        if (counterWert < 0) {
          counterWert = 99;
        }
        lcd.setCursor(0, 1);
        lcd.print(" " + LichtMenueEntry[1] + int(counterWert) + "%");
        Licht = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 5; //Zurück ins Feuchtigkeitsmenü
          lcd.clear();
        }
        break;//case9 (Ebene)

      case 50: //Bewässerung Sollwert0 (Ebene)
        counterWert = SMUpperThreshold[0];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 99) {
          counterWert = 99;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 1);
        lcd.print(" " + BewaesserungMenueEntry[1] + int(counterWert) + "%");
        SMUpperThreshold[0] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 3; //Zurück ins Bewässerungsmenü
          lcd.clear();
        }
        break;//case50 (Ebene)
      case 51: //Bewässerung Sollwert1 (Ebene)
        counterWert = SMUpperThreshold[1];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 99) {
          counterWert = 99;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(" " + BewaesserungMenueEntry[4] + int(counterWert) + "%");
        SMUpperThreshold[1] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 3; //Zurück ins Bewässerungsmenü
          lcd.clear();
        }
        break;//case51 (Ebene)
      case 52: //Bewässerung Sollwert2 (Ebene)
        counterWert = SMUpperThreshold[2];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 99) {
          counterWert = 99;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(" " + BewaesserungMenueEntry[7] + int(counterWert) + "%");
        SMUpperThreshold[2] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 3; //Zurück ins Bewässerungsmenü
          lcd.clear();
        }
        break;//case52 (Ebene)
      case 53: //Bewässerung Sollwert3 (Ebene)
        counterWert = SMUpperThreshold[3];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 99) {
          counterWert = 99;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(" " + BewaesserungMenueEntry[10] + int(counterWert) + "%");
        SMUpperThreshold[3] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 3; //Zurück ins Bewässerungsmenü
          lcd.clear();
        }
        break;//case53 (Ebene)
      case 54: //Bewässerung Sollwert4 (Ebene)
        counterWert = SMUpperThreshold[4];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 99) {
          counterWert = 99;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(" " + BewaesserungMenueEntry[13] + int(counterWert) + "%");
        SMUpperThreshold[4] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 3; //Zurück ins Bewässerungsmenü
          lcd.clear();
        }
        break;//case54 (Ebene)
      case 55: //Bewässerung Sollwert5 (Ebene)
        counterWert = SMUpperThreshold[5];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 99) {
          counterWert = 99;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(" " + BewaesserungMenueEntry[16] + int(counterWert) + "%");
        SMUpperThreshold[5] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = 3; //Zurück ins Bewässerungsmenü
          lcd.clear();
        }
        break;//case55 (Ebene)
      //////////////////////Programme für Montag////////////////
      case 60: //Programm Untermenü, Tage [Ebene]
        LayerIndex = 60;
        if (Enc_A == true) {
          counter60 += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counter60 -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counter60 > MAXTAGMENUE) {
          counter60 = 0;
          lcd.clear();
        }
        if (counter60 < 0) {
          counter60 = MAXTAGMENUE;
          lcd.clear();
        }
        switch (counter60) {
          case 0: //Eintrag 0 (Zurück zu Programmemenü) counter60
            lcd.setCursor(0, 0);
            lcd.print("|" + ProgrammeMenueEntry[TagIndex + 1]); //Montag (Zurück)
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[0] + (BlendeAn[TagIndex] - BlendeAn[TagIndex] % 60) / 60 + ":" + BlendeAn[TagIndex] % 60); //Lüfter an
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[1] + (BlendeAus[TagIndex] - BlendeAus[TagIndex] % 60) / 60 + ":" + BlendeAus[TagIndex] % 60); //Lüfter aus
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[2] + RHRegelung[TagIndex]); //Lüfter on/off
            if (pressed == true) {
              pressed = false;
              Ebene = 6; //Programme
              lcd.clear();
            }
            break;//case0 counter60
          case 1: //Eintrag 1 (Lufter an) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[TagIndex + 1]);
            lcd.setCursor(0, 1);
            lcd.print(">" + TagMenueEntry[0] + (BlendeAn[TagIndex] - BlendeAn[TagIndex] % 60) / 60 + ":" + BlendeAn[TagIndex] % 60); //Lüfter an
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[1] + (BlendeAus[TagIndex] - BlendeAus[TagIndex] % 60) / 60 + ":" + BlendeAus[TagIndex] % 60); //Lüfter aus
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[2] + RHRegelung[TagIndex]); //Lüfter on/off
            if (pressed == true) {
              pressed = false;
              Ebene = 600; //LüfterAn Untermenü
              lcd.clear();
            }
            break;//case1 counter60
          case 2: //Eintrag 1 (Lufter aus) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[TagIndex + 1]);
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[0] + (BlendeAn[TagIndex] - BlendeAn[TagIndex] % 60) / 60 + ":" + BlendeAn[TagIndex] % 60); //Lüfter an
            lcd.setCursor(0, 2);
            lcd.print(">" + TagMenueEntry[1] + (BlendeAus[TagIndex] - BlendeAus[TagIndex] % 60) / 60 + ":" + BlendeAus[TagIndex] % 60); //Lüfter aus
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[2] + RHRegelung[TagIndex]); //Lüfter on/off
            if (pressed == true) {
              pressed = false;
              Ebene = 601; //LüfterAus Untermenü
              lcd.clear();
            }
            break;//case2 counter60
          case 3: //Eintrag 1 (Lufter Regelung) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + ProgrammeMenueEntry[TagIndex + 1]);
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[0] + (BlendeAn[TagIndex] - BlendeAn[TagIndex] % 60) / 60 + ":" + BlendeAn[TagIndex] % 60); //Lüfter an
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[1] + (BlendeAus[TagIndex] - BlendeAus[TagIndex] % 60) / 60 + ":" + BlendeAus[TagIndex] % 60); //Lüfter aus
            lcd.setCursor(0, 3);
            lcd.print(">" + TagMenueEntry[2] + RHRegelung[TagIndex]); //Lüfter on/off
            if (pressed == true) {
              pressed = false;
              Ebene = 602; //Lüfter Status Untermenü
              lcd.clear();
            }
            break;//case3 counter60
          case 4: //Eintrag 4 (Pumpe an) counter[6]
            lcd.setCursor(0, 0);
            lcd.print(">" + TagMenueEntry[3] + (PumpeAn[TagIndex] - PumpeAn[TagIndex] % 60) / 60 + ":" + PumpeAn[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[4] + (PumpeAus[TagIndex] - PumpeAus[TagIndex] % 60) / 60 + ":" + PumpeAus[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[5] + Bewaesserung[TagIndex]); //Pumpe Status
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[6] + (LichtAn[TagIndex] - LichtAn[TagIndex] % 60) / 60 + ":" + LichtAn[TagIndex] % 60); //Licht an
            if (pressed == true) {
              pressed = false;
              Ebene = 603; //Pumpe An Menü
              lcd.clear();
            }
            break;//case4 counter60
          case 5: //Eintrag 5 (Pumpe aus) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + TagMenueEntry[3] + (PumpeAn[TagIndex] - PumpeAn[TagIndex] % 60) / 60 + ":" + PumpeAn[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 1);
            lcd.print(">" + TagMenueEntry[4] + (PumpeAus[TagIndex] - PumpeAus[TagIndex] % 60) / 60 + ":" + PumpeAus[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[5] + Bewaesserung[TagIndex]); //Pumpe Status
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[6] + (LichtAn[TagIndex] - LichtAn[TagIndex] % 60) / 60 + ":" + LichtAn[TagIndex] % 60); //Licht an
            if (pressed == true) {
              pressed = false;
              Ebene = 604; //Pumpe Aus Menü
              lcd.clear();
            }
            break;//case5 counter60
          case 6: //Eintrag 6 (Pumpe Status) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + TagMenueEntry[3] + (PumpeAn[TagIndex] - PumpeAn[TagIndex] % 60) / 60 + ":" + PumpeAn[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[4] + (PumpeAus[TagIndex] - PumpeAus[TagIndex] % 60) / 60 + ":" + PumpeAus[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 2);
            lcd.print(">" + TagMenueEntry[5] + Bewaesserung[TagIndex]); //Pumpe Status
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[6] + (LichtAn[TagIndex] - LichtAn[TagIndex] % 60) / 60 + ":" + LichtAn[TagIndex] % 60); //Licht an
            if (pressed == true) {
              pressed = false;
              Ebene = 605; //Pumpe Status
              lcd.clear();
            }
            break;//case6 counter60
          case 7: //Eintrag 7 (Licht an) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + TagMenueEntry[3] + (PumpeAn[TagIndex] - PumpeAn[TagIndex] % 60) / 60 + ":" + PumpeAn[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[4] + (PumpeAus[TagIndex] - PumpeAus[TagIndex] % 60) / 60 + ":" + PumpeAus[TagIndex] % 60); //Pumpe an
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[5] + Bewaesserung[TagIndex]); //Pumpe Status
            lcd.setCursor(0, 3);
            lcd.print(">" + TagMenueEntry[6] + (LichtAn[TagIndex] - LichtAn[TagIndex] % 60) / 60 + ":" + LichtAn[TagIndex] % 60); //Licht an
            if (pressed == true) {
              pressed = false;
              Ebene = 606; //Licht an
              lcd.clear();
            }
            break;//case7 counter60
          case 8: //Eintrag 8 (Licht aus) counter60
            lcd.setCursor(0, 0);
            lcd.print(">" + TagMenueEntry[7] + (LichtAus[TagIndex] - LichtAus[TagIndex] % 60) / 60 + ":" + LichtAus[TagIndex] % 60); //Licht aus
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[8] + LichtSteuerung[TagIndex]); //Licht Status
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[9] + (TempAn[TagIndex] - TempAn[TagIndex] % 60) / 60 + ":" + TempAn[TagIndex] % 60); //Temp an
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[10] + (TempAus[TagIndex] - TempAus[TagIndex] % 60) / 60 + ":" + TempAus[TagIndex] % 60); //Temp aus
            if (pressed == true) {
              pressed = false;
              Ebene = 607; //Licht aus
              lcd.clear();
            }
            break;//case8 counter60
          case 9: //Eintrag 9 (Licht Status) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + TagMenueEntry[7] + (LichtAus[TagIndex] - LichtAus[TagIndex] % 60) / 60 + ":" + LichtAus[TagIndex] % 60); //Licht aus
            lcd.setCursor(0, 1);
            lcd.print(">" + TagMenueEntry[8] + LichtSteuerung[TagIndex]); //Licht Status
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[9] + (TempAn[TagIndex] - TempAn[TagIndex] % 60) / 60 + ":" + TempAn[TagIndex] % 60); //Temp an
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[10] + (TempAus[TagIndex] - TempAus[TagIndex] % 60) / 60 + ":" + TempAus[TagIndex] % 60); //Temp aus
            if (pressed == true) {
              pressed = false;
              Ebene = 608; //Licht Status
              lcd.clear();
            }
            break;//case9 counter60
          case 10: //Eintrag 10 (TempAn) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + TagMenueEntry[7] + (LichtAus[TagIndex] - LichtAus[TagIndex] % 60) / 60 + ":" + LichtAus[TagIndex] % 60); //Licht aus
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[8] + LichtSteuerung[TagIndex]); //Licht Status
            lcd.setCursor(0, 2);
            lcd.print(">" + TagMenueEntry[9] + (TempAn[TagIndex] - TempAn[TagIndex] % 60) / 60 + ":" + TempAn[TagIndex] % 60); //Temp an
            lcd.setCursor(0, 3);
            lcd.print(" " + TagMenueEntry[10] + (TempAus[TagIndex] - TempAus[TagIndex] % 60) / 60 + ":" + TempAus[TagIndex] % 60); //Temp aus
            if (pressed == true) {
              pressed = false;
              Ebene = 609; //TempAn
              lcd.clear();
            }
            break;//case10 counter60
          case 11: //Eintrag 11 (TempAus) counter60
            lcd.setCursor(0, 0);
            lcd.print(" " + TagMenueEntry[7] + (LichtAus[TagIndex] - LichtAus[TagIndex] % 60) / 60 + ":" + LichtAus[TagIndex] % 60); //Licht aus
            lcd.setCursor(0, 1);
            lcd.print(" " + TagMenueEntry[8] + LichtSteuerung[TagIndex]); //Licht Status
            lcd.setCursor(0, 2);
            lcd.print(" " + TagMenueEntry[9] + (TempAn[TagIndex] - TempAn[TagIndex] % 60) / 60 + ":" + TempAn[TagIndex] % 60); //Temp an
            lcd.setCursor(0, 3);
            lcd.print(">" + TagMenueEntry[10] + (TempAus[TagIndex] - TempAus[TagIndex] % 60) / 60 + ":" + TempAus[TagIndex] % 60); //Temp aus
            if (pressed == true) {
              pressed = false;
              Ebene = 610; //TempAn
              lcd.clear();
            }
            break;//case11 counter60
          case 12: //Eintrag 12 (TempStatus) counter60
            lcd.setCursor(0, 0);
            lcd.print(">" + TagMenueEntry[11] + TempRegelung[TagIndex]); //Licht Status
            if (pressed == true) {
              pressed = false;
              Ebene = 611; //TempAn
              lcd.clear();
            }
            break;//case11 counter60
        }//switch counter[60]
        break; //case 6 (Ebene)
      //////////////Zeit einstellen////////////
      case 70: //Einstellung Zeit
        if (Enc_A == true) {
          counterMinute += 30;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 3);
        lcd.print(">" + SystemMenueEntry[3] + Tage[tag] + " " + int(counterStunde) + ":" + int(counterMinute));
        setTime(counterStunde, counterMinute, 0, day(), month(), year());
        if (pressed == true) {
          pressed = false;
          Ebene = 7; //Zurück zu Sytem:
          lcd.clear();
        }
        break; //case70 (Ebene)


      case 600: //Einstellung LüfterAn
        counterStunde = (BlendeAn[TagIndex] - BlendeAn[TagIndex] % 60) / 60;
        counterMinute = BlendeAn[TagIndex] % 60;

        if (Enc_A == true) {
          counterMinute += 15; //Encoder rechts drehen --> +15 min (schnell hochzählen)
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1; //Encoder links drehen --> -1 min (genaues einstellen der gewünschten Zeit)
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[0] + int(counterStunde) + ":" + int(counterMinute));
        BlendeAn[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Lüfter an:
          lcd.clear();
        }
        break; //case600 (Ebene)

      case 601: //Einstellung LüfterAus
        counterStunde = (BlendeAus[TagIndex] - BlendeAus[TagIndex] % 60) / 60;
        counterMinute = BlendeAus[TagIndex] % 60;
        if (Enc_A == true) {
          counterMinute += 15;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[1] + int(counterStunde) + ":" + int(counterMinute));
        BlendeAus[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Lüfter aus:
          lcd.clear();
        }
        break; //case601 (Ebene)

      case 602: //Einstellung Lüfter Status
        counterWert = RHRegelung[TagIndex];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 1) {
          counterWert = 0;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[2] + int(counterWert));
        RHRegelung[TagIndex] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Lüfter aus:
          lcd.clear();
        }
        break; //case602 (Ebene)
      /////////////////
      case 603: //Einstellung PumpeAn
        counterStunde = (PumpeAn[TagIndex] - PumpeAn[TagIndex] % 60) / 60;
        counterMinute = PumpeAn[TagIndex] % 60;

        if (Enc_A == true) {
          counterMinute += 15;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[3] + int(counterStunde) + ":" + int(counterMinute));
        PumpeAn[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe an:
          lcd.clear();
        }
        break; //case603 (Ebene)

      case 604: //Einstellung PumpeAus
        counterStunde = (PumpeAus[TagIndex] - PumpeAus[TagIndex] % 60) / 60;
        counterMinute = PumpeAus[TagIndex] % 60;
        if (Enc_A == true) {
          counterMinute += 15;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[4] + int(counterStunde) + ":" + int(counterMinute));
        PumpeAus[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe aus:
          lcd.clear();
        }
        break; //case604 (Ebene)

      case 605: //Einstellung Pumpe Status
        counterWert = Bewaesserung[TagIndex];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 1) {
          counterWert = 0;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[5] + int(counterWert));
        Bewaesserung[TagIndex] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe Status
          lcd.clear();
        }
        break; //case605 (Ebene)
      ////////
      case 606: //Einstellung LichtAn
        counterStunde = (LichtAn[TagIndex] - LichtAn[TagIndex] % 60) / 60;
        counterMinute = LichtAn[TagIndex] % 60;

        if (Enc_A == true) {
          counterMinute += 15;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[6] + int(counterStunde) + ":" + int(counterMinute));
        LichtAn[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe an:
          lcd.clear();
        }
        break; //case606 (Ebene)

      case 607: //Einstellung LichtAus
        counterStunde = (LichtAus[TagIndex] - LichtAus[TagIndex] % 60) / 60;
        counterMinute = LichtAus[TagIndex] % 60;
        if (Enc_A == true) {
          counterMinute += 15;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[7] + int(counterStunde) + ":" + int(counterMinute));
        LichtAus[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe aus:
          lcd.clear();
        }
        break; //case604 (Ebene)

      case 608: //Einstellung Licht Status
        counterWert = LichtSteuerung[TagIndex];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 1) {
          counterWert = 0;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[8] + int(counterWert));
        LichtSteuerung[TagIndex] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe Status
          lcd.clear();
        }
        break; //case608 (Ebene)
      /////////

      case 609: //Einstellung TempAn
        counterStunde = (TempAn[TagIndex] - TempAn[TagIndex] % 60) / 60;
        counterMinute = TempAn[TagIndex] % 60;

        if (Enc_A == true) {
          counterMinute += 15;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[9] + int(counterStunde) + ":" + int(counterMinute));
        TempAn[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe an:
          lcd.clear();
        }
        break; //case609 (Ebene)

      case 610: //Einstellung LichtAus
        counterStunde = (TempAus[TagIndex] - TempAus[TagIndex] % 60) / 60;
        counterMinute = TempAus[TagIndex] % 60;
        if (Enc_A == true) {
          counterMinute += 15;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterMinute -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterMinute > 59) {
          counterMinute = 0;
          counterStunde += 1;
        }
        if (counterMinute < 0) {
          counterMinute = 59;
          counterStunde -= 1;
        }
        if (counterStunde > 23) {
          counterStunde = 0;
        }
        if (counterStunde < 0) {
          counterStunde = 23;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[10] + int(counterStunde) + ":" + int(counterMinute));
        TempAus[TagIndex] = counterStunde * 60 + counterMinute;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe aus:
          lcd.clear();
        }
        break; //case610 (Ebene)

      case 611: //Einstellung Temp Status
        counterWert = TempRegelung[TagIndex];
        if (Enc_A == true) {
          counterWert += 1;
          Enc_A = false;
          lcd.clear();
        }
        if (Enc_B == true) {
          counterWert -= 1;
          Enc_B = false;
          lcd.clear();
        }
        if (counterWert > 1) {
          counterWert = 0;
        }
        if (counterWert < 0) {
          counterWert = 0;
        }
        lcd.setCursor(0, 0);
        lcd.print(">" + TagMenueEntry[11] + int(counterWert));
        TempRegelung[TagIndex] = counterWert;
        if (pressed == true) {
          pressed = false;
          Ebene = LayerIndex; //Zurück zu Pumpe Status
          lcd.clear();
        }
        break; //case611 (Ebene)
    }//switch Ebene
  }//while Ebene != 0
  delay(10);
  MenueStatus = false;
  HintergrundBeleuchtung = true;
  Speichern();
}//Menue


void alarm(const char *Meldung) {
  //Allgemeine Alarmfunktion, zeigt an ob Leckage erkannt wurde bzw. ob Sensoren nicht mehr erreichbar sind.
  //Zusätzlich wird die Heizung und die Pumpe deaktiviert.
  analogWrite(HEIZUNG, 0);
  digitalWrite(PUMPE, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(Meldung);
  for (int i = 0; i <= 2; i++) {
    lcd.backlight();
    delay(300);
    lcd.noBacklight();
    delay(200);
  }
  lcd.backlight();
  lcd.clear();
}//alarm


void Blende(float RegelAbweichung) {
  //Richtet die Blende anhand der Regelabweichung aus, links bzw. rechtslauf durch Hysterese voneinander getrennt, damit Motor am Schaltpunkt nicht hin und her fährt
  const int Schrittfolge[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
  };
  const int Gesamtschritte = 84; //Anzahl an Durchläufen bis 100% offen
  static int Position = 0;
  int Verfahrweg = 0;
  const int Hysterese = 3;
  const int Maximalwert = 10;
  const int delaytime = 3;
  int pos = 0;

  float Sollposition = RegelAbweichung / Maximalwert * 100;

  if (Sollposition <= 0) {
    pos = 0;
  }
  if (Sollposition > 100) {
    pos = 100;

  }
  else {
    Verfahrweg = Sollposition - Position;
    if (Verfahrweg >= 0) { //weiter auf, rechtsrum drehen
      if (Sollposition >= 75) {
        pos = 100;
      }
      if (Sollposition >= 50 && Sollposition < 75) {
        pos = 75;
      }
      if (Sollposition >= 25 && Sollposition < 50) {
        pos = 50;
      }
      if (Sollposition >= 10 && Sollposition < 25) {
        pos = 25;
      }
      if (Sollposition >= 0 && Sollposition < 10) {
        pos = 0;
      }
    }//if
    else { //weiter zu, linksrum drehen
      if (Sollposition <= (10 - Hysterese) && Sollposition >= 0) {
        pos = 0;
      }
      if (Sollposition <= (25 - Hysterese) && Sollposition > (10 - Hysterese)) {
        pos = 25;
      }
      if (Sollposition <= (50 - Hysterese) && Sollposition > (25 - Hysterese)) {
        pos = 50;
      }
      if (Sollposition <= (75 - Hysterese) && Sollposition > (50 - Hysterese)) {
        pos = 75;
      }
      if (Sollposition >= (75 - Hysterese)) {
        pos = 100;
      }
    }//else
    DEBUG_PRINTLN("Position:\t" + String(pos));
  }
  float Schritte = Gesamtschritte * (pos - Position) / 100;
  if (Schritte >= 0) {
    for (int i = 0; i < int(abs(Schritte)); i++) {
      for (int a = 0; a <= 7; a++) {
        digitalWrite(BLENDE1, Schrittfolge[a][0]);
        digitalWrite(BLENDE2, Schrittfolge[a][1]);
        digitalWrite(BLENDE3, Schrittfolge[a][2]);
        digitalWrite(BLENDE4, Schrittfolge[a][3]);
        delay(delaytime);
      }
    }//for
  }//if
  else {
    for (int j = 0; j < int(abs(Schritte)); j++) {
      for (int a = 7; a >= 0; a--) {
        digitalWrite(BLENDE1, Schrittfolge[a][0]);
        digitalWrite(BLENDE2, Schrittfolge[a][1]);
        digitalWrite(BLENDE3, Schrittfolge[a][2]);
        digitalWrite(BLENDE4, Schrittfolge[a][3]);
        delay(delaytime);
      }
    }//for
  }//else
  digitalWrite(BLENDE1, 0);
  digitalWrite(BLENDE2, 0);
  digitalWrite(BLENDE3, 0);
  digitalWrite(BLENDE4, 0);
  Position = pos;
}//Blende

unsigned int BH1750_Read() {
  //Lichtsensor auslesen. Code aus Beispielsketch der Bibliothek entnommen.
  unsigned int i = 0;
  Wire.beginTransmission (BH1750_Device);
  Wire.requestFrom (BH1750_Device, 2);
  while (Wire.available())
  {
    i <<= 8;
    i |= Wire.read();
  }
  Wire.endTransmission();
  return i / 1.2; // Convert to Lux
}//BH1750_Read

void loop(void) { //Eigentliches Hauptprogramm
  int thetime; //Aktuelle Uhrzeit als Produkt von Stunde * Minute.
  unsigned int Lux; //Lichtstärke
  static long Ttast = 10; //[Sekunden]. Zeit, die das Programm wartet bis es von vorne beginnt, wird ebenso als Tastzeit für den PID-Regler benötigt.
  static float e = 0; //Regelabweichung
  static float esum = 0; //I-Glied
  static float ealt = 0; //D-Glied
  static int Ki = 6; //Integralanteil
  static int Kp = 180; //Proportionalanteil
  static int Kd = 2000; //Differentialanteil
  static float y; //Stellgröße
  float HeizungAnzeige = 0; //LCD-Anzeige der Heizleistung (0-99)
  static float LichtPWM; //PWM für LEDs
  static bool busy = false; //Variable dient dazu im LCD den gerade bewässerten Kanal anzuzeigen
  static unsigned long LCDtime = 0;
  static bool Leckage = false;
  static bool leer = false; //Hilfsvariable falls Füllstandsüberwachung des Kanisters implementiert wird. Wird derzeit nicht geändert.
  static bool aktualisieren = true; //Variable zum Reset des Verbrauchszählers.
  static unsigned long Timer1 = 0; //Startzeit des Hauptprogramms
  static unsigned long Timer2 = 0; //Endzeit des Hauptprogramms
  boolean Vent0 = false;
  boolean Vent1 = false;
  boolean Vent2 = false;
  boolean Vent3 = false;
  boolean Vent4 = false;
  boolean Vent5 = false;
  long Mittelwert = 0; //Mittelwert der Bodenfeuchtigkeiten pro Sensor
  int MAX = 5; //Anzahl der Messwerte der Bodenfeuchtigkeitssensoren
  float RHInnen_abs; //Absolute Luftfeuchtigkeit innen
  float RHAussen_abs; //Absolute Luftfeuchtigkeit außen
  static int DiffZeit = 60; //Zeitdifferenz die die Pumpe pro Kanal wartet, bis sie wieder aktiv wird.
  static int SoilMoistureTimer[6] = {0, 0, 0, 0, 0, 0}; //Speichert den Zeitpunkt an dem bewässert wurde.
  static float Fakt_Verbrauch[4] = {0.000519216, 0.00019343, 0.002933333, 0.0008}; //Faktoren zur Berechnung des Stromverbrauchs: Heizung, Licht, Konstant, Lüfter, siehe auch Datei Verbrauch.pdf
  
  Timer1 = millis(); //Starten des Timers um Laufzeit des Programms zu ermitteln (wichtig für PID).

  RTC.read(tm);
  tag = weekday() - 1;
  thetime = tm.Hour * 60 + tm.Minute;
  DEBUG_PRINTLN("Tag:\t" + String(tag));
  DEBUG_PRINTLN("thetime:\t" + String(thetime));
  //Leckagesensor auslesen
  if (digitalRead(LECKAGEPIN) == LOW) {
    Leckage = true;
    alarm("Leckage!");
    DEBUG_PRINTLN("Leckage!");
  }
  else {
    Leckage = false;
  }
  //Luftfeuchtigkeiten auslesen
  RHInnen = dhtInnen.readHumidity();
  RHInnen = int(RHInnen);
  if (RHInnen == 0 || isnan(RHInnen)) {
    alarm("Feucht. innen Sensor");
    RHInnen = 50;
  }
  RHAussen = dhtAussen.readHumidity();
  RHAussen = int(RHAussen);
  if (RHAussen == 0 || isnan(RHAussen)) {
    alarm("Feucht. aussen Sensor");
    RHAussen = 40;
  }
  RHElektronik = dhtElektronik.readHumidity();
  RHElektronik = int(RHElektronik);
  if (RHElektronik == 0 || isnan(RHElektronik)) {
    alarm("Feucht. Elektronik Sensor");
    RHElektronik = 40;
  }
  if (RHElektronik > 60) {
    Leckage = true;
  }

  DEBUG_PRINTLN("Feuchtigkeit innen:\t" + String(RHInnen));
  DEBUG_PRINTLN("Feuchtigkeit aussen:\t" + String(RHAussen));
  DEBUG_PRINTLN("Feuchtigkeit Elektronik:\t" + String(RHElektronik));
  DEBUG_PRINTLN("Feuchtigkeit Sollwert:\t" + String(RHSoll));
  
  //Temperaturen auslesen
  TempInnen = dhtInnen.readTemperature();
  if (isnan(TempInnen)) {
    alarm("Temp innen Sensor");
    TempInnen = 25.0;
  }
  TempAussen = dhtAussen.readTemperature();
  if (isnan(TempAussen)) {
    alarm("Temp aussen Sensor");
    TempAussen = 22.0;
  }
  TempElektronik = dhtElektronik.readTemperature();
  if (isnan(TempElektronik)) {
    alarm("Temp Elektronik Sensor");
    TempElektronik = 30.0;
  }

  //Relative (gemessene) Luftfeuchtigkeiten in absolute Luftfeuchtigkeiten umrechnen
  RHInnen_abs = (6.112 * exp((17.67 * TempInnen) / (TempInnen + 243.5)) * RHInnen * 2.1674) / (273.15 + TempInnen);
  RHAussen_abs = (6.112 * exp((17.67 * TempAussen) / (TempAussen + 243.5)) * RHAussen * 2.1674) / (273.15 + TempAussen);

  DEBUG_PRINTLN("Absolute Feuchtigkeit innen (g/m³):\t" + String(RHInnen_abs));
  DEBUG_PRINTLN("Absolute Feuchtigkeit aussen (g/m³):\t" + String(RHAussen_abs));
  DEBUG_PRINTLN("Temperatur innen:\t" + String(TempInnen));
  DEBUG_PRINTLN("Temperatur aussen:\t" + String(TempAussen));
  DEBUG_PRINTLN("Temperatur Elektronik:\t" + String(TempElektronik));
  DEBUG_PRINTLN("Temperatur Sollwert:\t" + String(TempSoll));

  //Temperaturregelung
  if (TempRegelung[tag] == true) {
    if ((thetime <= TempAus[tag] && thetime >= TempAn[tag])) {
      if (TempElektronik < 60) {//Falls die Temperatur der Elektronik größer als 60°C ist (willkürlich geschätzter Wert) dann stimmt irgendwas so gar nicht, nicht heizen und Warnung über LCD ausgeben.
        e = TempSoll - TempInnen;
        DEBUG_PRINTLN("Regelabweichung: " + String(e));
        if (e >= 1.0) { //Regelabweichung sehr groß (z.B. durch Temperaturänderung im Menü, also heizen was das Zeug hält.
          DEBUG_PRINTLN("Regelabweichung > 1, Reiner P-Regler");
          analogWrite(HEIZUNG, 255);
          HeizungAnzeige = 99;
        }
        else {
          y = Kp * e + Ki * Ttast * esum + Kd / Ttast * (e - ealt); //Eigentlicher PID-Regler
          ealt = e;
          DEBUG_PRINTLN("Stellgröße: " + String(y));
          DEBUG_PRINTLN("Esum: " + String(esum));
          if (y >= 0 && y < 255) { //Anti-Windup
            esum = esum + e;
            DEBUG_PRINTLN("esum update");
          }
          if (y < 0) {//Begrenzung PWM-Signal
            y = 0;
            DEBUG_PRINTLN("Y = 0");
          }
          if (y >= 255) {//Begrenzung PWM-Signal
            y = 255;
            DEBUG_PRINTLN("Y = 255");
          }
          analogWrite(HEIZUNG, int(y));
          HeizungAnzeige = y / 2.57; //Umrechnung von 0-255 (PWM-Signal) auf Prozent für LCD
        }//else (PID)
      }//if Temp Elektronik
      else {
        alarm("Temp. Elektronik pruefen");
      }//else
    }//if thetime <= TempAn usw.
    else {
      analogWrite(HEIZUNG, 0);
      DEBUG_PRINTLN("Temp aus");
    }
  }//if thetime <= TempAn usw.
  else {
    analogWrite(HEIZUNG, 0);
    DEBUG_PRINTLN("Temp aus");
  }

  //Luftfeuchtigkeitsregelung
  if (RHRegelung[tag] == true) {
    if (thetime <= BlendeAus[tag] && thetime >= BlendeAn[tag]) {
      if ((RHInnen > RHSoll) && (RHInnen_abs > RHAussen_abs)){
        if (RHInnen - RHSoll > 10) {
          Blende(10); //Maximal offen
          DEBUG_PRINTLN("Blende auf + Lüfter, zu feucht");
          digitalWrite(FAN, HIGH);
        }
        if (RHInnen - RHSoll  <= 10) {
          Blende(RHInnen - RHSoll); //Blende partiell öffnen
          digitalWrite(FAN, HIGH);
          DEBUG_PRINTLN("Blende teilweise + Lüfter, zu feucht");
        }
        if (RHInnen <= RHSoll) {
          Blende(0);
          digitalWrite(FAN, LOW);
          DEBUG_PRINTLN("Blende zu, zu trocken");
        }
      }
      else {
        digitalWrite(FAN, LOW);
        Blende(0);
      }
    }
    else {
      digitalWrite(FAN, LOW);
      Blende(0);
    }

  }//if RHRegelung
  else {
    digitalWrite(FAN, LOW);
    Blende(0);
  }

  //Lichtsteuerung
  if (LichtSteuerung[tag] == true) {
    if (thetime <= LichtAus[tag] && thetime >= LichtAn[tag]) {
      Lux = BH1750_Read();
      DEBUG_PRINTLN("Lux: " + String(Lux));
      if (Lux < 2000) {//LEDs werden erst angeschaltet, wenn es dunkel ist (2000 ist geschätzter Wert)
        LichtPWM = 255 * Licht * (1 - (Lux*0.0005)) / 99 ; //Formel für PWM-Signal in Abhängigkeit der aktuellen Helligkeit
        analogWrite(LED, int(LichtPWM));
        DEBUG_PRINTLN("LichtPWM" + String(LichtPWM));
      }
      else{
        LichtPWM = 0;
        analogWrite(LED, 0);
      }
    }
    else {
      DEBUG_PRINTLN("Licht aus");
      analogWrite(LED, 0);
    }
  }
  else {
    analogWrite(LED, 0);
  }

  //Bodenfeuchtigkeit auslesen
  pinMode(SM0, INPUT_PULLUP);
  pinMode(SM1, INPUT_PULLUP);
  pinMode(SM2, INPUT_PULLUP);
  pinMode(SM3, INPUT_PULLUP);
  pinMode(SM4, INPUT_PULLUP);
  pinMode(SM5, INPUT_PULLUP);

  //Zunächst alle Pullups aktivieren und Sensoren auslesen, falls kein Sensor angeschlossen ist, wird ein Wert größer 1000 ausgelesen..
  if (analogRead(SM0) > 1000) {
    SoilMoistureStatus[0] = false;
    SoilMoisture[0] = 0;
  }
  else {//Sensor ist anscheinend vorhanden
    pinMode(SM0, INPUT);
    Mittelwert = 0;
    for (int i = 0; i < MAX; i++) {
      Mittelwert += int(0.082 * analogRead(SM0)); //Mehrere Messwerte aufzeichnen....
    }
    SoilMoisture[0] = Mittelwert / MAX;//... und Mittelwert bilden.
    SoilMoistureStatus[0] = true;
    if (SoilMoisture[0] >= 100) {
      SoilMoisture[0] = 99;
    }
    if (SoilMoisture[0] < 0) {
      SoilMoisture[0] = 0;
      SoilMoistureStatus[0] = false;
    }
  }
  //Analoges Vorgehen für die anderen Sensoren.
  if (analogRead(SM1) > 1000) {
    SoilMoistureStatus[1] = false;
    SoilMoisture[1] = 0;
  }
  else {
    pinMode(SM1, INPUT);
    Mittelwert = 0;
    for (int i = 0; i < MAX; i++) {
      Mittelwert += int(0.082 * analogRead(SM1));
    }
    SoilMoisture[1] = Mittelwert / MAX;
    SoilMoistureStatus[1] = true;
    if (SoilMoisture[1] >= 100) {
      SoilMoisture[1] = 99;
    }
    if (SoilMoisture[1] < 0) {
      SoilMoisture[1] = 0;
      SoilMoistureStatus[1] = false;
    }
  }
  if (analogRead(SM2) > 1000) {
    SoilMoistureStatus[2] = false;
    SoilMoisture[2] = 0;
  }
  else {
    pinMode(SM2, INPUT);
    Mittelwert = 0;
    for (int i = 0; i < MAX; i++) {
      Mittelwert += int(0.082 * analogRead(SM2));
    }
    SoilMoisture[2] = Mittelwert / MAX;
    SoilMoistureStatus[2] = true;
    if (SoilMoisture[2] >= 100) {
      SoilMoisture[2] = 99;
    }
    if (SoilMoisture[2] < 0) {
      SoilMoisture[2] = 0;
      SoilMoistureStatus[2] = false;
    }
  }
  if (analogRead(SM3) > 1000) {
    SoilMoistureStatus[3] = false;
    SoilMoisture[3] = 0;
  }
  else {
    pinMode(SM3, INPUT);
    Mittelwert = 0;
    for (int i = 0; i < MAX; i++) {
      Mittelwert += int(0.082 * analogRead(SM3));
    }
    SoilMoisture[3] = Mittelwert / MAX;
    SoilMoistureStatus[3] = true;
    if (SoilMoisture[3] >= 100) {
      SoilMoisture[3] = 99;
    }
    if (SoilMoisture[3] < 0) {
      SoilMoisture[3] = 0;
      SoilMoistureStatus[3] = false;
    }
  }
  if (analogRead(SM4) > 1000) {
    SoilMoistureStatus[4] = false;
    SoilMoisture[4] = 0;
  }
  else {
    pinMode(SM4, INPUT);
    Mittelwert = 0;
    for (int i = 0; i < MAX; i++) {
      Mittelwert += int(0.082 * analogRead(SM4));
    }
    SoilMoisture[4] = Mittelwert / MAX;
    SoilMoistureStatus[4] = true;
    if (SoilMoisture[4] >= 100) {
      SoilMoisture[4] = 99;
    }
    if (SoilMoisture[4] < 0) {
      SoilMoisture[4] = 0;
      SoilMoistureStatus[4] = false;
    }
  }
  if (analogRead(SM5) > 1000) {
    SoilMoistureStatus[5] = false;
    SoilMoisture[5] = 0;
  }
  else {
    pinMode(SM5, INPUT);
    Mittelwert = 0;
    for (int i = 0; i < MAX; i++) {
      Mittelwert += int(0.082 * analogRead(SM5));
    }
    SoilMoisture[5] = Mittelwert / MAX;
    SoilMoistureStatus[5] = true;
    if (SoilMoisture[5] >= 100) {
      SoilMoisture[5] = 99;
    }
    if (SoilMoisture[5] < 0) {
      SoilMoisture[5] = 0;
      SoilMoistureStatus[5] = false;
    }
  }
  DEBUG_PRINTLN("Bodenfeuchtigkeit1: \t" + String(SoilMoisture[0]));
  DEBUG_PRINTLN("Bodenfeuchtigkeit2: \t" + String(SoilMoisture[1]));
  DEBUG_PRINTLN("Bodenfeuchtigkeit3: \t" + String(SoilMoisture[2]));
  DEBUG_PRINTLN("Bodenfeuchtigkeit4: \t" + String(SoilMoisture[3]));
  DEBUG_PRINTLN("Bodenfeuchtigkeit5: \t" + String(SoilMoisture[4]));
  DEBUG_PRINTLN("Bodenfeuchtigkeit6: \t" + String(SoilMoisture[5]));
  DEBUG_PRINTLN("BodenfeuchtigkeitSollWert1: \t" + String(SMUpperThreshold[0]));
  DEBUG_PRINTLN("BodenfeuchtigkeitSollWert2: \t" + String(SMUpperThreshold[1]));
  DEBUG_PRINTLN("BodenfeuchtigkeitSollWert3: \t" + String(SMUpperThreshold[2]));
  DEBUG_PRINTLN("BodenfeuchtigkeitSollWert4: \t" + String(SMUpperThreshold[3]));
  DEBUG_PRINTLN("BodenfeuchtigkeitSollWert5: \t" + String(SMUpperThreshold[4]));
  DEBUG_PRINTLN("BodenfeuchtigkeitSollWert6: \t" + String(SMUpperThreshold[5]));
  
  //Pumpe regeln
  if (Leckage == false && leer == false && Bewaesserung[tag] == true && thetime <= PumpeAus[tag] && thetime > PumpeAn[tag]) { //leer ist Variable auf die Zugegriffen werden kann, wenn Füllstandsüberwachung eingebaut ist, zur Zeit nutzlos.
    if (SoilMoistureStatus[0] == true && busy == false && SoilMoisture[0] <= SMUpperThreshold[0] && (thetime > SoilMoistureTimer[0] +  DiffZeit)) {
      SoilMoistureTimer[0] = thetime; //Zeit der Bewässerung für Kanal 1 speichern.
      digitalWrite(VENT0, HIGH); //Ventil öffnen und 500 ms warten.
      delay(500);
      digitalWrite(PUMPE, HIGH);
      busy = true; //Variable dient dazu im LCD den gerade bewässerten Kanal anzuzeigen
      DEBUG_PRINTLN("Ventil 1 aktiv");
      delay(5000); //Pumpe für 5 Sekunden einschalten.
      digitalWrite(PUMPE, LOW);
      digitalWrite(VENT0, LOW);
      Vent0 = true;
    }
    if (SoilMoistureStatus[1] == true && busy == false && SoilMoisture[1] <= SMUpperThreshold[1] && (thetime > SoilMoistureTimer[1] +  DiffZeit)) {
      SoilMoistureTimer[1] = thetime;
      digitalWrite(VENT1, HIGH);
      delay(500);
      digitalWrite(PUMPE, HIGH);
      busy = true;
      DEBUG_PRINTLN("Ventil 2 aktiv");
      delay(5000);
      digitalWrite(PUMPE, LOW);
      digitalWrite(VENT1, LOW);
      Vent1 = true;
    }
    if (SoilMoistureStatus[2] == true && busy == false && SoilMoisture[2] <= SMUpperThreshold[2] && (thetime > SoilMoistureTimer[2] +  DiffZeit)) {
      SoilMoistureTimer[2] = thetime;
      digitalWrite(VENT2, HIGH);
      delay(500);
      digitalWrite(PUMPE, HIGH);
      busy = true;
      DEBUG_PRINTLN("Ventil 3 aktiv");
      delay(5000);
      digitalWrite(PUMPE, LOW);
      digitalWrite(VENT2, LOW);
      Vent2 = true;
    }
    if (SoilMoistureStatus[3] == true && busy == false && SoilMoisture[3] <= SMUpperThreshold[3] && (thetime > SoilMoistureTimer[3] +  DiffZeit)) {
      SoilMoistureTimer[3] = thetime;
      digitalWrite(VENT3, HIGH);
      delay(500);
      digitalWrite(PUMPE, HIGH);
      busy = true;
      DEBUG_PRINTLN("Ventil 4 aktiv");
      delay(5000);
      digitalWrite(PUMPE, LOW);
      digitalWrite(VENT3, LOW);
      Vent3 = true;
    }
    if (SoilMoistureStatus[4] == true && busy == false && SoilMoisture[4] <= SMUpperThreshold[4] && (thetime > SoilMoistureTimer[4] +  DiffZeit)) {
      SoilMoistureTimer[4] = thetime;
      digitalWrite(VENT4, HIGH);
      delay(500);
      digitalWrite(PUMPE, HIGH);
      busy = true;
      DEBUG_PRINTLN("Ventil 5 aktiv");
      delay(5000);
      digitalWrite(PUMPE, LOW);
      digitalWrite(VENT4, LOW);
      Vent4 = true;
    }
    if (SoilMoistureStatus[5] == true && busy == false && SoilMoisture[5] <= SMUpperThreshold[5] && (thetime > SoilMoistureTimer[5] +  DiffZeit)) {
      SoilMoistureTimer[5] = thetime;
      digitalWrite(VENT5, HIGH);
      delay(500);
      digitalWrite(PUMPE, HIGH);
      busy = true;
      DEBUG_PRINTLN("Ventil 6 aktiv");
      delay(5000);
      digitalWrite(PUMPE, LOW);
      digitalWrite(VENT5, LOW);
      Vent5 = true;
    }
  }//Zeitprogramm
  else {
    DEBUG_PRINTLN("Pumpe aus");
    SoilMoistureTimer[0] = 0;
    SoilMoistureTimer[1] = 0;
    SoilMoistureTimer[2] = 0;
    SoilMoistureTimer[3] = 0;
    SoilMoistureTimer[4] = 0;
    SoilMoistureTimer[5] = 0;
    digitalWrite(PUMPE, LOW);
    digitalWrite(VENT0, LOW);
    digitalWrite(VENT1, LOW);
    digitalWrite(VENT2, LOW);
    digitalWrite(VENT3, LOW);
    digitalWrite(VENT4, LOW);
    digitalWrite(VENT5, LOW);
  }
  //Bewässerung = true, keine Leckage, Wasserstand voll

  //Display updaten
  //Zeile 0
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Tage[tag]); //Wochentag anzeigen
  lcd.print(" ");
  if (tm.Hour < 10) { //aktuelle Stunde anzeigen
    lcd.print(0);
    lcd.print(tm.Hour);
  }
  else {
    lcd.print(tm.Hour);
  }
  lcd.print(":");
  if (tm.Minute < 10) { //aktuelle Minute anzeigen
    lcd.print(0);
    lcd.print(tm.Minute);
  }
  else {
    lcd.print(tm.Minute);
  }
  lcd.print(" ");
  lcd.print(HeLCD); //Sondersymbol für Heizung und Heizleistung (0-99%) anzeigen.
  lcd.print(int(HeizungAnzeige));
  lcd.print("   ");
  if (busy == true) { //Bei aktiver Bewässerung Sondersymbol anzeigen und entsprechenden Kanal einblenden. Zusätzlich Hintergrundbeleuchtung aktivieren.
    lcd.print(BeLCD);
    HintergrundBeleuchtung = true;
    if (Vent0 == true) {
      lcd.print("1");
    }
    if (Vent1 == true) {
      lcd.print("2");
    }
    if (Vent2 == true) {
      lcd.print("3");
    }
    if (Vent3 == true) {
      lcd.print("4");
    }
    if (Vent4 == true) {
      lcd.print("5");
    }
    if (Vent5 == true) {
      lcd.print("6");
    }
  }
  //Zeile 1
  lcd.setCursor(0, 1);
  lcd.print(TiLCD); //Temperatur innen anzeigen + Sondersymbol
  lcd.print(round(TempInnen));
  lcd.print("C");
  lcd.print(" ");
  lcd.print(TaLCD); //Temperatur außen anzeigen + Sondersymbol
  lcd.print(round(TempAussen));
  lcd.print("C");
  lcd.print(" ");
  lcd.print(FiLCD); //Feuchtigkeit innen anzeigen + Sondersymbol
  if (RHInnen < 10) {
    lcd.print(0);
    lcd.print(int(RHInnen));
  }
  else if (RHInnen == 100) {
    lcd.print(99);
  }
  else {
    lcd.print(int(RHInnen));
  }
  lcd.print("%");
  lcd.print(" ");
  lcd.print(FaLCD); //Feuchtigkeit außen anzeigen + Sondersymbol
  if (RHAussen < 10) {
    lcd.print(0);
    lcd.print(int(RHAussen));
  }
  else if (RHAussen == 100) {
    lcd.print(99);
  }
  else {
    lcd.print(int(RHAussen));
  }
  lcd.print("%");
  //Zeile 2
  lcd.setCursor(0, 2); //Bodenfeuchtigkeit bzw. Status der Sensoren (NC) anzeigen.
  for (int i = 0; i < 3; i++) {
    lcd.print(i + 1);
    lcd.print(":");
    if (SoilMoistureStatus[i] == false) {
      lcd.print("NC");
      lcd.print("  ");
    }
    else {
      if (SoilMoisture[i] < 10) {
        lcd.print(0);
        lcd.print(SoilMoisture[i]);
        lcd.print("% ");
      }
      else if (SoilMoisture[i] == 100) {
        lcd.print(99);
        lcd.print("% ");
      }
      else {
        lcd.print(SoilMoisture[i]);
        lcd.print("% ");
      }
    }
  }
  //Zeile 3
  lcd.setCursor(0, 3); //Bodenfeuchtigkeit bzw. Status der Sensoren (NC) anzeigen.
  for (int i = 0; i < 3; i++) {
    lcd.print(i + 4);
    lcd.print(":");
    if (SoilMoistureStatus[i + 3] == false) {
      lcd.print("NC");
      lcd.print("  ");
    }
    else {
      if (SoilMoisture[i + 3] < 10) {
        lcd.print(0);
        lcd.print(SoilMoisture[i + 3]);
        lcd.print("% ");
      }
      else if (SoilMoisture[i + 3] == 100) {
        lcd.print(99);
        lcd.print("% ");
      }
      else {
        lcd.print(SoilMoisture[i + 3]);
        lcd.print("% ");
      }
    }
  }
  Timer2 = millis(); //Hält Uhrzeit fest, die das Programm für den Durchlauf benötigt.

  busy = false; 
  Verbrauch[0] = Verbrauch[0] + Fakt_Verbrauch[0] * y + Fakt_Verbrauch[1] * LichtPWM + Fakt_Verbrauch[2] + digitalRead(FAN) * Fakt_Verbrauch[3]; //[Wh]. Berechnet den Stromverbrauch, siehe Datei Stromverbrauch.pdf
  DEBUG_PRINTLN("Verbrauch :\t" + String(Verbrauch[0]));
  if (thetime == 0 && aktualisieren == true) {
    Verbrauch[3] = Verbrauch[2] + Verbrauch[0]; //72 h
    Verbrauch[2] = Verbrauch[1] + Verbrauch[0]; //48 h
    Verbrauch[1] = Verbrauch[0]; //24 h
    Verbrauch[0] = 0; //aktuell
    aktualisieren = false;
  }
  if (thetime != 0) { //Verbrauchszähler um 00:00 Uhr zurücksetzen
    aktualisieren = true;
  }
  while (millis() - Timer1 < Ttast * 1000) { //Umrechnung s --> ms, Programm fängt sich für die Zeit Ttast in Schleife, bevor Hauptroutine neu startet
    if (MenueStart == true) {
      Menue();
    }
    MenueStart = false;
    if (HintergrundBeleuchtung == true && LCDtime == 0) {
      lcd.backlight();
      LCDtime = millis();
      delay(100);
    }
    if (abs(millis() - LCDtime) > 60000) { //Falls der Butten einmal gedrückt wurde, leuchtet das Display für 60 sek.
      HintergrundBeleuchtung = false;
      LCDtime = 0;
      lcd.noBacklight();
    }//Insgesamt loop() 10 Sekunden laufen lassen
  }
}//loop

////////////////////////////////////////////////////////////////
//Auslesen des Drehgebers und Button + Entprellen
// Interrupt on A changing state
//Beispielcode von https://playground.arduino.cc/Main/RotaryEncoders/
void doEncoderA(void) {
  // debounce
  if (rotating == true && MenueStatus == true) {
    delayMicroseconds (100);  // wait a little until the bouncing is done
  }
  // Test transition, did things really change?
  if ( digitalRead(ENCODERA) != A_set ) {
    // debounce once more
    A_set = !A_set;
    if ( A_set && !B_set )
      Enc_A = true;
    rotating = false;  // Kein Entprellen mehr, bis while-Schleife in Menü den Debouncer zurücksetzt
  }
}

void doEncoderB(void) {
  if (rotating == true && MenueStatus == true) {
    delayMicroseconds (100);
  }
  if ( digitalRead(ENCODERB) != B_set ) {
    B_set = !B_set;
    if ( B_set && !A_set )
      Enc_B = true;
    rotating = false;
  }
}

void Button(void) {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  int debounce_time = 200; //ms
  int doubleclicktime = 600; //ms
  static unsigned int counter = 0;


  if (interrupt_time - last_interrupt_time > debounce_time) {
    if (interrupt_time - last_interrupt_time < doubleclicktime) {
      //tone(BUZZER, 2000, 200);
      DEBUG_PRINTLN("Doppelklick");
      MenueStart = true;
    }
    else {
      DEBUG_PRINTLN("Einfacher Klick");
      if (MenueStatus == true) {
        pressed = true;
      }
      else {
        HintergrundBeleuchtung = true;
      }
    }
  }
  last_interrupt_time = interrupt_time;
}
////////////////////////////////////////////////////////
