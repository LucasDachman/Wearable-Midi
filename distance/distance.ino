/**
 * Lucas Dachman
 * Date: November 30, 2018
 * 
 * Hardware Connections:
 * Pro Micro | HC-SR04 
 * ---------------------
 *     5V    |   VCC     
 *     14    |   Trig1     
 *     15    |   Echo1     
 *     10    |   Trig2     
 *     16    |   Echo2
 *     GND   |   GND
 *  
 * License:
 *  Public Domain
 */
#include "MIDIUSB.h"


// Pins
const int TRIG_PIN_2 = 10;
const int ECHO_PIN_2 = 16;
const int TRIG_PIN_1 = 14;
const int ECHO_PIN_1 = 15;

// Anything over 400 cm (23200 us pulse) is "out of range"
const unsigned int MAX_DIST = 23200;
const int BUF_SIZE = 10;
int [BUFF_SIZE] buf1;
int [BUFF_SIZE] buf2;
int index1 = 0;
int index2 = 0;

void setup() {

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN_1, OUTPUT);
  digitalWrite(TRIG_PIN_1, LOW);
  pinMode(TRIG_PIN_2, OUTPUT);
  digitalWrite(TRIG_PIN_2, LOW);

  // We'll use the serial monitor to view the sensor output
  Serial.begin(9600);
  //while (!Serial);
}

void loop() {
  float cm;
  // first distance
  cm = getDistance(TRIG_PIN_1, ECHO_PIN_1);
  int value1 = smoothValue(cm, buf1, &index1);
  sendMidi(16, value1);

  // second distance
  cm = getDistance(TRIG_PIN_2, ECHO_PIN_2);
  int value2 = smoothValue(cm, buf2, &index2);
  sendMidi(17, value2);
}

void sendMidi(byte control, int value) {
  if (value < 0) {
    return;
  }
  controlChange(1, control, value);
  MidiUSB.flush();
}

int smoothValue(float cm, int buffer[BUF_SIZE], int *index)
{
  // normalise values or return -1 if out of range
  if (cm < 0) {
    return -1;
  }
  if (cm > 40.0)
  {
    cm = 40.0;
  }
  int value = mapFloat(cm, 0.0, 40.0, 0.0, 127.0);
  buffer[*index] = value;
  //Serial.print(control); Serial.print(" "); Serial.println(value); buffer[*index] = val;
  (*index)++;
  if (*index >= BUF_SIZE)
  {
    *index = 0;
  }
  // sum and average values
  int average = 0;
  for (int i = 0; i < BUF_SIZE; i++)
  {
    average += buffer[i];
  }
  average = average / BUF_SIZE;
  return average;
}

  float getDistance(int TRIG_PIN, int ECHO_PIN)
  {
    unsigned long t1;
    unsigned long t2;
    unsigned long pulse_width;
    float cm;

    // Wait at least 60ms before next measurement
    delay(60);

    // Hold the trigger pin high for at least 10 us
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Wait for pulse on echo pin
    while (digitalRead(ECHO_PIN) == 0)
      ;

    // Measure how long the echo pin was held high (pulse width)
    // Note: the micros() counter will overflow after ~70 min
    t1 = micros();
    while (digitalRead(ECHO_PIN) == 1)
      ;
    t2 = micros();
    pulse_width = t2 - t1;

    // Calculate distance in centimeters and inches. The constants
    // are found in the datasheet, and calculated from the assumed speed
    //of sound in air at sea level (~340 m/s).
    cm = pulse_width / 58.0;

    // Print out results
    if (pulse_width > MAX_DIST)
    {
      return -1;
    } else {
    return cm;
  }
}

  
// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// Like map but with floats
int mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
