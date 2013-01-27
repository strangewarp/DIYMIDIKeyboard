
#include <EEPROM.h> // http://arduino.cc/en/Reference/EEPROM
#include <Keypad.h> // https://github.com/Nullkraft/Keypad
#include <Potentiometer.h> // http://playground.arduino.cc//Code/Potentiometer

#define CONFIG_VERSION "dk5"
#define CONFIG_START 32

struct settings {
  byte data[9][4];
  char vers[4];
} saves = {
  {
    {4,1,1,127}, {2,1,1,127}, {2,2,1,127},
    {2,4,1,127}, {2,5,1,127}, {2,6,1,127},
    {2,8,1,127}, {2,9,1,127}, {2,10,1,127}
  },
  CONFIG_VERSION
};

const byte rows = 4;
const byte cols = 8;

const byte butrows = 4;
const byte butcols = 4;

byte rowpins[rows] = {23, 25, 27, 29};
byte colpins[cols] = {39, 41, 43, 45, 47, 49, 51, 53};
byte butrowpins[3] = {38, 40, 42};
byte butcolpins[3] = {48, 50, 52};
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
int slnew = LOW; // Is used to compare the new saveload pin value to the previous iteration's saveload pin value
byte butactive = 0;
byte adjbutkey = 0;
byte adjnotekey = 0;
int pulseval = 0;
int pulsedelay = 20;
int pulseinc = 5;
long prevmillis = millis();

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
  
  loadConfig();
  
  octavepot.setSectors(10);
  channelpot.setSectors(16);
  commandpot.setSectors(8);
  velocitypot.setSectors(128);
  
  butkeypad.setDebounceTime(0);
  butkeypad.setHoldTime(1000000);
  
  pianokeypad.setDebounceTime(0);
  pianokeypad.setHoldTime(1000000);
  
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
        saves.data[adjbutkey][0] = octave;
        saves.data[adjbutkey][1] = channel;
        saves.data[adjbutkey][2] = command;
        saves.data[adjbutkey][3] = velocity;
        saveConfig();
      } else if (saveload == HIGH) {
        octave = saves.data[adjbutkey][0];
        channel = saves.data[adjbutkey][1];
        command = saves.data[adjbutkey][2];
        velocity = saves.data[adjbutkey][3];
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
    if ((pulseval < 0)
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
            if (command == 1) {
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

