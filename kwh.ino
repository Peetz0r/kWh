#include "EEPROM.h"
#include "AnythingEEPROM.h"
#include <inttypes.h>

#define READINGS       250
#define EEPROM_OFFSET  100
#define MS_PER_HOUR    3.6e6

struct SettingsStruct {
  unsigned short cycles_per_kwh;
  unsigned char  lower_threshold;
  unsigned char  upper_threshold;
  unsigned short max_watt;
} settings;

unsigned long debounce_time;

void calc_debounce() {
  debounce_time = (1000 * ((double) MS_PER_HOUR / ((long) settings.cycles_per_kwh * settings.max_watt)));
  Serial.print("Debounce time (ms): ");
  Serial.println(debounce_time);
}

void read_settings() {
  EEPROM_readAnything(EEPROM_OFFSET, settings);
  if (settings.lower_threshold == 0xff) settings.lower_threshold = 101;
  if (settings.upper_threshold == 0xff) settings.upper_threshold = 105;
  if (settings.cycles_per_kwh == 0xffff) settings.cycles_per_kwh = 375;
  if (settings.max_watt == 0xffff) settings.max_watt = 6000;
  Serial.println("Settings: ");
  Serial.println(settings.cycles_per_kwh, DEC);
  Serial.println(settings.lower_threshold, DEC);
  Serial.println(settings.upper_threshold, DEC);
  Serial.println(settings.max_watt, DEC);
  calc_debounce();
}

void save_settings() {
  EEPROM_writeAnything(EEPROM_OFFSET, settings);
  calc_debounce();
}

void setup () {
  Serial.begin(115200);
  pinMode(A1, INPUT);
  pinMode(13, OUTPUT);
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  read_settings();
}

boolean ledstate = LOW;
unsigned long cycle = 0;
unsigned long previous = 0; // timestamp

unsigned short readings[READINGS];
unsigned short cursor = 0;
boolean gotenough = false;

unsigned short hits = 0;

unsigned long restore_time = 0;
boolean settingschanged = false;

void loop () {

  unsigned short sum = 0;
  for (byte i = 0; i < 40; i++) {
    sum += analogRead(1);
  }

  unsigned long bigsum = 0;
  for (unsigned short i = 0; i < READINGS; i++) bigsum += readings[i];
  unsigned short average = bigsum / READINGS;

  unsigned short ratio = (double) sum / (average+1) * 100;

  if (restore_time && millis() >= restore_time) {
    restore_time = 0;
    if (settingschanged) {
      Serial.println("Saving settings");
      save_settings();
      settingschanged = false;
    }
  }

  unsigned short lo = settings.lower_threshold;
  unsigned short hi = settings.upper_threshold;

  if (hi == 254) {
      lo = 400;
      hi = 1000;
  }

  boolean newledstate = ledstate
    ? (ratio >  lo)
    : (ratio >= hi);

  if ((!gotenough) || (!newledstate)) {
    readings[cursor++] = sum;
    if (cursor >= READINGS) {
      cursor = 0;
      if (!gotenough) {
        gotenough = true;
        Serial.println("Done averaging");
      }
    }
  }

  if (newledstate) hits++;

  if (newledstate == ledstate) return;

  digitalWrite(13, ledstate = newledstate);

  if (!ledstate) {
    Serial.print("Marker: ");
    Serial.print(millis() - previous);
    Serial.print(" ms (");
    Serial.print(hits, DEC);
    Serial.println(" readings)");
    hits = 0;
    return;
  }

  unsigned long now = millis();
  unsigned long time = now - previous;

  if (time < debounce_time) return;

  previous = now;

  if (!cycle++) {
    Serial.println("Discarding incomplete cycle.");
    return;
  }

  double W = 1000 * ((double) MS_PER_HOUR / time) / settings.cycles_per_kwh;
  Serial.print("Cycle ");
  Serial.print(cycle, DEC);
  Serial.print(": ");
  Serial.print(time, DEC);
  Serial.print(" ms, ");
  Serial.print(W, 2);
  Serial.println(" W");
}

