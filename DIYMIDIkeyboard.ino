
#include <EEPROM.h> // http://arduino.cc/en/Reference/EEPROM
#include <Keypad.h> // http://playground.arduino.cc//Code/Keypad
#include <Potentiometer.h> // http://playground.arduino.cc//Code/Potentiometer

#define CONFIG_VERSION "dk4"
#define CONFIG_START 32

struct settings {
  char data[15][4];
  char vers[4];
} saves = {
  {
    {2,0,1,127}, {2,1,1,127}, {2,2,1,127}, {2,3,1,127},
    {2,4,1,127}, {2,5,1,127}, {2,6,1,127}, {2,7,1,127},
    {2,8,1,127}, {2,9,1,127}, {2,10,1,127}, {2,11,1,127},
    {2,12,1,127}, {2,13,1,127}, {2,14,1,127}
  },
  CONFIG_VERSION
};

const byte rows = 4;
const byte cols = 8;

const byte butrows = 4;
const byte butcols = 4;

byte rowpins[rows] = {23, 25, 27, 29};
byte colpins[cols] = {39, 41, 43, 45, 47, 49, 51, 53};
byte butrowpins[4] = {38, 40, 42, 44};
byte butcolpins[4] = {46, 48, 50, 52};
byte ledpins[8] = {22, 24, 26, 28, 30, 32, 34, 36};
byte saveledpin = 2;
byte loadledpin = 3;

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

byte activebutton = 0;
byte saveload = 1; // SAVE-LOAD mode tracking variable. 0 for save mode, 1 for load mode.
byte adjbutkey = 0;
int pulseval = 0;
int pulseadj = 0;

char keys[rows][cols] = {
  { 0, 1, 2, 3, 4, 5, 6, 7},
  { 8, 9,10,11,12,13,14,15},
  {16,17,18,19,20,21,22,23},
  {24,25,26,27,28,29,30,31}
};

char butkeys[butrows][butcols] = {
  { 0, 1, 2, 3},
  { 4, 5, 6, 7},
  { 8, 9,10,11},
  {12,13,14,15}
};

Keypad keypad = Keypad(makeKeymap(keys), rowpins, colpins, rows, cols);

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
  
  loadConfig();
  
  octavepot.setSectors(10);
  channelpot.setSectors(16);
  commandpot.setSectors(8);
  velocitypot.setSectors(128);
  
  butkeypad.setDebounceTime(20);
  butkeypad.setHoldTime(1000000);
  
  keypad.setDebounceTime(0);
  keypad.setHoldTime(1000000);
  
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
  
  butkeypad.getKeys();
  
  for (int a = 0; a < (butrows * butcols); a++) {
    
    if (butkeypad.key[a].kchar) {
      
      if (butkeypad.key[a].kstate == PRESSED) {
        if (butkeypad.key[a].stateChanged) {
          
          for (int kdnum = 0; kdnum < (rows * cols); kdnum++) {
            if (keydown[kdnum] == true) {
              keydown[kdnum] = false;
              if (command == 1) {
                noteSend(
                  channel + 128,
                  (keypad.key[kdnum].kchar + (octave * 12)) % 128,
                  velocity
                );
              }
            }
          }
          
          if (butkeypad.key[a].kchar >= 1) {
            adjbutkey = butkeypad.key[a].kchar - 1;
            if (saveload == 0) {
              saves.data[adjbutkey][0] = octave;
              saves.data[adjbutkey][1] = channel;
              saves.data[adjbutkey][2] = command;
              saves.data[adjbutkey][3] = velocity;
              saveConfig();
            } else if (saveload == 1) {
              octave = saves.data[adjbutkey][0];
              channel = saves.data[adjbutkey][1];
              command = saves.data[adjbutkey][2];
              velocity = saves.data[adjbutkey][3];
            }
            octavebus = octavecheck;
            channelbus = channelcheck;
            commandbus = commandcheck;
            velocitybus = velocitycheck;
            setBinaryLEDs(adjbutkey);
          } else if (butkeypad.key[a].kchar == 0) {
            saveload ^= 1;
            if (saveload == 0) { // Light up SAVE-LED
              digitalWrite(saveledpin, HIGH);
              digitalWrite(loadledpin, LOW);
            } else if (saveload == 1) { // Light up LOAD-LED
              digitalWrite(saveledpin, LOW);
              digitalWrite(loadledpin, HIGH);
            }
          }
          
        }
      }
      
    }
    
  }
  
  pulseval = (pulseval + 1) % 31250;
  pulseadj = round(pulseval / 123);
  if (saveload == 0) { // Write the pulse value to SAVE-LED
    analogWrite(saveledpin, pulseadj);
  } else if (saveload == 1) { // Write the pulse value to LOAD-LED
    analogWrite(loadledpin, pulseadj);
  }
  
  keypad.getKeys();
  
  for (int i = 0; i < (rows * cols); i++) {
    
    if (keypad.key[i].kchar) {
      
      if (keypad.key[i].kstate == PRESSED) {
        
        if (keypad.key[i].stateChanged) {
          if (keydown[i] == false) {
            keydown[i] = true;
            noteSend(
              channel + (command * 16) + 128,
              (keypad.key[i].kchar + (octave * 12)) % 128,
              velocity
            );
            setBinaryLEDs((keypad.key[i].kchar + (octave * 12)) % 128);
          }
        }
        
      } else if (keypad.key[i].kstate == RELEASED) {
        
        if (keypad.key[i].stateChanged) {
          if (keydown[i] == true) {
            keydown[i] = false;
            if (command == 1) {
              noteSend(
                channel + 128,
                (keypad.key[i].kchar + (octave * 12)) % 128,
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

void setDecimalLEDs(int val) {
  for (int lnum = 0; lnum < 8; lnum++) {
    if (lnum == (val % 8)) {
      digitalWrite(ledpins[lnum], HIGH);
    } else {
      digitalWrite(ledpins[lnum], LOW);
    }
  }
}

void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (//EEPROM.read(CONFIG_START + sizeof(saves) - 1) == saves.vers[3] // this is '\0'
  EEPROM.read(CONFIG_START + sizeof(saves) - 2) == saves.vers[2]
  && EEPROM.read(CONFIG_START + sizeof(saves) - 3) == saves.vers[1]
  && EEPROM.read(CONFIG_START + sizeof(saves) - 4) == saves.vers[0])
  { // reads settings from EEPROM
    for (unsigned int t=0; t<sizeof(saves); t++) {
      *((char*)&saves + t) = EEPROM.read(CONFIG_START + t);
    }
  } else {
    // settings aren't valid! will overwrite with default settings
    saveConfig();
  }
}

void saveConfig() {
  for (unsigned int t = 0; t < sizeof(saves); t++) { // writes to EEPROM
    EEPROM.write(CONFIG_START + t, *((char*)&saves + t));
    // and verifies the data
    if (EEPROM.read(CONFIG_START + t) != *((char*)&saves + t)) {
      // error writing to EEPROM
    }
  }
}

