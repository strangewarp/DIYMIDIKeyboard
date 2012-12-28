
#include <EEPROM.h> // http://arduino.cc/en/Reference/EEPROM
#include <Keypad.h> // http://playground.arduino.cc//Code/Keypad
#include <Potentiometer.h> // http://playground.arduino.cc//Code/Potentiometer

#define CONFIG_VERSION "dk1"
#define CONFIG_START 32

struct settings {
  char data[16][4];
  char vers[4];
} saves = {
  {
    {2,0,1,127}, {2,1,1,127}, {2,2,1,127}, {2,3,1,127},
    {2,4,1,127}, {2,5,1,127}, {2,6,1,127}, {2,7,1,127},
    {2,8,1,127}, {2,9,1,127}, {2,10,1,127}, {2,11,1,127},
    {2,12,1,127}, {2,13,1,127}, {2,14,1,127}, {2,15,1,127}
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
byte slswitchpin = 4;

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
byte butactive = 0;
byte adjbutkey = 0;
byte pulseval = 0;
int pulsedelay = 20;
long prevmillis = 0;

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
  
  pinMode(slswitchpin, INPUT);
  
  loadConfig();
  
  octavepot.setSectors(10);
  channelpot.setSectors(16);
  commandpot.setSectors(8);
  velocitypot.setSectors(128);
  
  butkeypad.setDebounceTime(0);
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
  
  saveload = digitalRead(slswitchpin);
  if (saveload == LOW) { // Light up SAVE-LED
    pulseval = 150;
    analogWrite(saveledpin, 150);
    analogWrite(loadledpin, 0);
  } else if (saveload == HIGH) { // Light up LOAD-LED
    pulseval = 150;
    analogWrite(saveledpin, 0);
    analogWrite(loadledpin, 150);
  }

  butactive = butkeypad.getKey();
  
  if (butkeypad.kstate == PRESSED) {
    if (butkeypad.stateChanged) {
      
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
      
      if (saveload == LOW) {
        saves.data[butkeypad.kchar][0] = octave;
        saves.data[butkeypad.kchar][1] = channel;
        saves.data[butkeypad.kchar][2] = command;
        saves.data[butkeypad.kchar][3] = velocity;
        saveConfig();
      } else if (saveload == HIGH) {
        octave = saves.data[butkeypad.kchar][0];
        channel = saves.data[butkeypad.kchar][1];
        command = saves.data[butkeypad.kchar][2];
        velocity = saves.data[butkeypad.kchar][3];
      }
      octavebus = octavecheck;
      channelbus = channelcheck;
      commandbus = commandcheck;
      velocitybus = velocitycheck;
      setBinaryLEDs(adjbutkey);
      
    }
  }
  
  if ((millis() - pulsedelay) > prevmillis) { // Adjust the saveload LED's PWM pulsation, based on timing variables
    prevmillis = millis();
    pulseval = (pulseval + 2) % 255;
    if (saveload == LOW) { // Write the pulse value to SAVE-LED
      analogWrite(saveledpin, pulseval);
    } else if (saveload == HIGH) { // Write the pulse value to LOAD-LED
      analogWrite(loadledpin, pulseval);
    }
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

