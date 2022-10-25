#define READINGS       250
#define EEPROM_OFFSET  100
#define MS_PER_HOUR    3.6e6

unsigned long debounce_time;

void setup () {
  Serial.begin(115200);
  pinMode(A1, INPUT);
  pinMode(13, OUTPUT);
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);

  Serial.println("Settings: ");

  Serial.print("Cycles per kWh: ");
  Serial.println(CYCLES_PER_KWH, DEC);

  Serial.print("Thresholds: ");
  Serial.print(LOWER_THRESHOLD, DEC);
  Serial.print(" - ");
  Serial.println(UPPER_THRESHOLD, DEC);

  Serial.print("Max power (W): ");
  Serial.println(MAX_WATT, DEC);

  debounce_time = (1000 * ((double) MS_PER_HOUR / ((long) CYCLES_PER_KWH * MAX_WATT)));
  Serial.print("Debounce time (ms): ");
  Serial.println(debounce_time);
}

boolean ledstate = LOW;
unsigned long cycle = 0;
unsigned long previous = 0; // timestamp

unsigned short readings[READINGS];
unsigned short cursor = 0;
boolean gotenough = false;

unsigned short hits = 0;

void loop () {

  unsigned short sum = 0;
  for (byte i = 0; i < 40; i++) {
    sum += analogRead(1);
  }

  unsigned long bigsum = 0;
  for (unsigned short i = 0; i < READINGS; i++) bigsum += readings[i];
  unsigned short average = bigsum / READINGS;

  unsigned short ratio = (double) sum / (average+1) * 100;

  unsigned short lo = LOWER_THRESHOLD;
  unsigned short hi = UPPER_THRESHOLD;

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

  double W = 1000 * ((double) MS_PER_HOUR / time) / CYCLES_PER_KWH;
  Serial.print("Cycle ");
  Serial.print(cycle, DEC);
  Serial.print(": ");
  Serial.print(time, DEC);
  Serial.print(" ms, ");
  Serial.print(W, 2);
  Serial.println(" W");
}

