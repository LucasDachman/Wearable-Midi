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
const int BUF_SIZE = 5;
int buf1 [BUF_SIZE];
int buf2[BUF_SIZE];
int index1 = 0;
int index2 = 0;
int lastMidiVal1 = 0;
int lastMidiVal2 = 0;

void setup() {

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN_1, OUTPUT);
  digitalWrite(TRIG_PIN_1, LOW);
  pinMode(TRIG_PIN_2, OUTPUT);
  digitalWrite(TRIG_PIN_2, LOW);

  // init buffers
  for (int i = 0; i < BUF_SIZE; i++) {
    buf1[i] = 0;
    buf2[i] = 0;
  }

  Serial.begin(9600);
  //while (!Serial);
  Serial.println("AAAAHHHHH");
}

void loop()
{
  float cm;
  int value1;
  int value2;

  // first distance
  cm = getDistance(TRIG_PIN_1, ECHO_PIN_1);
  if (cm >= 0)
  {
    value1 = smoothValue(cm, buf1, &index1);
    if (lastMidiVal1 != value1)
    {
      lastMidiVal1 = value1;
      controlChange(1, 16, value1);
      MidiUSB.flush();
    }
  }

  // second distance
  cm = getDistance(TRIG_PIN_2, ECHO_PIN_2);
  if (cm >= 0)
  {
    value2 = smoothValue(cm, buf2, &index2);
    if (lastMidiVal2 != value2)
    {
      lastMidiVal2 = value2;
      controlChange(1, 17, value2);
      MidiUSB.flush();
    }
  }
  // Serial.println(cm);
  // Serial.print("1: "); Serial.print(value1); Serial.print("\t");
  // Serial.print("2: "); Serial.println(value2);
  // Wait at least 60ms before next measurement
  delay(60);
}

int smoothValue(float cm, int buffer[BUF_SIZE], int *index)
{
  // normalise values or return -1 if out of range
  if (cm < 12) {
    cm = 12;
  }
  if (cm > 40.0)
  {
    cm = 40.0;
  }
  int value = mapFloat(cm, 12.0, 40.0, 0.0, 127.0);
  buffer[*index] = value;
  (*index)++;
  if (*index >= BUF_SIZE)
  {
    *index = 0;
  }
  int median = findMedian(buffer, BUF_SIZE);
  return median;
}

/* https://www.geeksforgeeks.org/program-for-mean-and-median-of-an-unsorted-array/ */
// Function for calculating median 
float findMedian(int a[], int n) 
{ 
    float median;
    // First we sort the array 
    sort(a, n); 
  
    // check for even case 
    if (n % 2 != 0) {
       median = (float)a[n/2]; 
    } else {
      median = (float)(a[(n-1)/2] + a[n/2])/2.0; 
    }
      
    return median;
}

/* https://www.geeksforgeeks.org/insertion-sort/ */
/* Function to sort an array using insertion sort*/
void sort(int arr[], int n)
{
  int i, key, j;
  for (i = 1; i < n; i++)
  {
    key = arr[i];
    j = i - 1;

    /* Move elements of arr[0..i-1], that are 
          greater than key, to one position ahead 
          of their current position */
    while (j >= 0 && arr[j] > key)
    {
      arr[j + 1] = arr[j];
      j = j - 1;
    }
    arr[j + 1] = key;
  }
}

float getDistance(int TRIG_PIN, int ECHO_PIN)
{
  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float cm;

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
  }
  else
  {
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
