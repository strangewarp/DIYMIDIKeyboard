
// DIY MIDI Keyboard
//
// Circuit:
// Arduino Mega + Salvaged Toy Keyboard or other button matrix. (yes, seriously)
// 5v -> 220ohm resistor -> MIDI jack's pin 4
// MIDI jack's pin 2 -> GND
// MIDI jack's pin 5 -> TX1 pin
// 5v -> 1k resistor -> (SPST switch -> GND) AND (ANALOG-IN "slswitchpin")
// 5v, GND -> 10k potentiometers -> ANALOG-IN 0, 1, 2, 3
// ledpins -> 1k resistors -> LEDs -> GND
// saveledpin/loadledpin -> 1k resistors -> two-color LED -> GND
// keys -> rowpins x colpins -> salvaged piano keyboard (NO GND CONNECTION)
// butkeys -> butrowpins x butcolpins -> simple button matrix (NO GND CONNECTION)
//
// Note: program will bug out if left plugged in for >49 days. Should work fine after a power-cycle though.

#include <Keypad.h> // https://github.com/Nullkraft/Keypad
#include <Potentiometer.h> // http://playground.arduino.cc/Code/Potentiometer
#include <EEPROM.h> // http://arduino.cc/en/Reference/EEPROM
#include "EEPROMAnything.h"

const byte presets = 9; // Number of preset buttons to store in savedata. Must match up with contents of "butkeys"

const byte rows = 4;
const byte cols = 8;

const byte butrows = 4;
const byte butcols = 4;

const unsigned int buttondebounce = 0;
const unsigned int pianodebounce = 0;

byte rowpins[rows] = {23, 25, 27, 29};
byte colpins[cols] = {39, 41, 43, 45, 47, 49, 51, 53};
byte butrowpins[3] = {38, 40, 42};
byte butcolpins[3] = {48, 50, 52};
const byte ledpins[8] = {22, 24, 26, 28, 30, 32, 34, 36};
const byte saveledpin = 2;
const byte loadledpin = 3;
const byte slswitchpin = 4;

byte octave = 0;
byte channel = 0;
byte command = 0;
byte velocity = 0;
byte octavecheck = 0;
byte channelcheck = 0;
byte commandcheck = 0;
byte velocitycheck = 0;
byte octavebus = 0;
byte channelbus = 0;
byte commandbus = 0;
byte velocitybus = 0;

int saveload = LOW; // SAVE-LOAD mode tracking variable. LOW for save mode, HIGH for load mode. Set by digitalRead of slswitchpin
int slnew = LOW; // Is used to compare the new saveload pin value to the previous iteration's saveload pin value
byte butactive = 0;
byte adjbutkey = 0;
byte adjnotekey = 0;
int pulseval = 0;
int pulsedelay = 20;
int pulseinc = 6;
unsigned long prevmillis = millis(); // Tracks the milliseconds that have elapsed.

char keys[rows][cols] = {
  { 1, 2, 3, 4, 5, 6, 7, 8},
  { 9,10,11,12,13,14,15,16},
  {17,18,19,20,21,22,23,24},
  {25,26,27,28,29,30,31,32}
};

char butkeys[butrows][butcols] = {
  { 1, 2, 3},
  { 4, 5, 6},
  { 7, 8, 9}
};

struct config_t {
  byte p[presets][4];
} pdata;

Keypad pianokeypad = Keypad(makeKeymap(keys), rowpins, colpins, rows, cols);

Keypad butkeypad = Keypad(makeKeymap(butkeys), butrowpins, butcolpins, butrows, butcols);

Potentiometer octavepot = Potentiometer(0);
Potentiometer channelpot = Potentiometer(1);
Potentiometer commandpot = Potentiometer(2);
Potentiometer velocitypot = Potentiometer(3);

boolean keydown[rows * cols];

byte binval = 0;

void setup() {
  
  for (int kd = 0; kd < (rows * cols); kd++) {
    keydown[kd] = false;
  }
  
  for (int lpin = 0; lpin < 8; lpin++) {
    pinMode(ledpins[lpin], OUTPUT);
  }
  
  pinMode(saveledpin, OUTPUT);
  pinMode(loadledpin, OUTPUT);
  
  pinMode(slswitchpin, INPUT);
  
  octavepot.setSectors(10);
  channelpot.setSectors(16);
  commandpot.setSectors(8);
  velocitypot.setSectors(128);
  
  butkeypad.setDebounceTime(buttondebounce);
  butkeypad.setHoldTime(1000000);
  
  pianokeypad.setDebounceTime(pianodebounce);
  pianokeypad.setHoldTime(1000000);
  
  EEPROM_readAnything(0, pdata);
  
  for (int pnum = 0; pnum < presets; pnum++) {
    if (pdata.p[pnum][0] > 9) {
      pdata.p[pnum][0] = 3;
    }
    if (pdata.p[pnum][1] > 15) {
      pdata.p[pnum][1] = 0;
    }
    if (pdata.p[pnum][2] > 7) {
      pdata.p[pnum][2] = 1;
    }
    if (pdata.p[pnum][3] > 127) {
      pdata.p[pnum][3] = 127;
    }
  }
  
  Serial.begin(31250);
  
}

void loop() {

  octavecheck = octavepot.getSector();
  channelcheck = channelpot.getSector();
  commandcheck = commandpot.getSector();
  velocitycheck = velocitypot.getSector();
  
  if (
    (octavecheck != octave)
    && (octavecheck != octavebus)
  ) {
    octave = octavecheck;
    octavebus = -1;
    setBinaryLEDs(octave);
  } else if (
    (channelcheck != channel)
    && (channelcheck != channelbus)
  ) {
    channel = channelcheck;
    channelbus = -1;
    setBinaryLEDs(128 + (command * 16) + channel);
  } else if (
    (commandcheck != command)
    && (commandcheck != commandbus)
  ) {
    command = commandcheck;
    commandbus = -1;
    setBinaryLEDs(128 + (command * 16) + channel);
  } else if (
    (velocitycheck != velocity)
    && (velocitycheck != velocitybus)
  ) {
    velocity = velocitycheck;
    velocitybus = -1;
    setBinaryLEDs(velocity);
  }
  
  slnew = digitalRead(slswitchpin);
  
  if (saveload != slnew) {
    saveload = slnew;
    if (saveload == LOW) { // Light up SAVE-LED
      analogWrite(saveledpin, pulseval);
      analogWrite(loadledpin, 0);
    } else if (saveload == HIGH) { // Light up LOAD-LED
      analogWrite(saveledpin, 0);
      analogWrite(loadledpin, pulseval);
    }
  }

  butactive = butkeypad.getKey();
  
  if (butkeypad.kstate == PRESSED) {
    if (butactive != NO_KEY) {
      
      for (int kdnum = 0; kdnum < (rows * cols); kdnum++) {
        if (keydown[kdnum] == true) {
          keydown[kdnum] = false;
          if (command == 1) {
            noteSend(
              channel + 128,
              ((pianokeypad.key[kdnum].kchar - 1) + (octave * 12)) % 128,
              velocity
            );
          }
        }
      }
      
      adjbutkey = butkeypad.kchar - 1;
      if (saveload == LOW) {
        pdata.p[adjbutkey][0] = octave;
        pdata.p[adjbutkey][1] = channel;
        pdata.p[adjbutkey][2] = command;
        pdata.p[adjbutkey][3] = velocity;
        EEPROM_writeAnything(0, pdata);
      } else if (saveload == HIGH) {
        octave = pdata.p[adjbutkey][0];
        channel = pdata.p[adjbutkey][1];
        command = pdata.p[adjbutkey][2];
        velocity = pdata.p[adjbutkey][3];
      }
      octavebus = octavecheck;
      channelbus = channelcheck;
      commandbus = commandcheck;
      velocitybus = velocitycheck;
      setBinaryLEDs(butactive);
      
    }
  }
  
  if ((millis() - pulsedelay) > prevmillis) { // Adjust the saveload LED's PWM pulsation, based on timing variables
  
    prevmillis = millis();
    
    pulseval += pulseinc;
    if (
      (pulseval < 0)
      || (pulseval > 255)
    ) {
      pulseinc *= -1;
      pulseval = max(0, min(255, pulseval));
    }
    
    if (saveload == LOW) { // Write the pulse value to SAVE-LED
      analogWrite(saveledpin, pulseval);
    } else if (saveload == HIGH) { // Write the pulse value to LOAD-LED
      analogWrite(loadledpin, pulseval);
    }
    
  }
  
  pianokeypad.getKeys();
  
  for (int i = 0; i < (rows * cols); i++) {
    
    if (pianokeypad.key[i].kchar) {
      
      adjnotekey = pianokeypad.key[i].kchar - 1;
      
      if (pianokeypad.key[i].kstate == PRESSED) {
        
        if (pianokeypad.key[i].stateChanged) {
          if (keydown[i] == false) {
            keydown[i] = true;
            noteSend(
              channel + (command * 16) + 128,
              (adjnotekey + (octave * 12)) % 128,
              velocity
            );
            setBinaryLEDs((adjnotekey + (octave * 12)) % 128);
          }
        }
        
      } else if (pianokeypad.key[i].kstate == RELEASED) {
        
        if (pianokeypad.key[i].stateChanged) {
          if (keydown[i] == true) {
            keydown[i] = false;
            if (command == 1) { // If command-type is NOTEON, send a corresponding NOTEOFF on key-release
              noteSend(
                channel + 128,
                (adjnotekey + (octave * 12)) % 128,
                velocity
              );
            }
          }
        }
        
      }
      
    }
    
  }
  
}

void noteSend(byte cmd, byte note, byte velo) {
  Serial.write(cmd);
  Serial.write(note);
  Serial.write(velo);
}

void setBinaryLEDs(int val) {
  binval = 128;
  for (int bnum = 0; bnum < 8; bnum++) {
    if (binval <= val) {
      digitalWrite(ledpins[bnum], HIGH);
      val -= binval;
    } else {
      digitalWrite(ledpins[bnum], LOW);
    }
    binval /= 2;
  }
}

