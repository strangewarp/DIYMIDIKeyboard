
#include <Keypad.h>
#include <Potentiometer.h>

const byte rows = 4;
const byte cols = 8;

const byte butrows = 4;
const byte butcols = 4;

byte rowpins[rows] = {23, 25, 27, 29};
byte colpins[cols] = {39, 41, 43, 45, 47, 49, 51, 53};
byte butrowpins[4] = {22, 24, 26, 28};
byte butcolpins[4] = {30, 32, 34, 36};
byte ledpins[8] = {38, 40, 42, 44, 46, 48, 50, 52};

byte octave = 0;
byte channel = 0;
byte command = 0;
byte velocity = 0;
byte octavecheck = 0;
byte channelcheck = 0;
byte commandcheck = 0;
byte velocitycheck = 0;
int octaveold = 0;
int channelold = 0;
int commandold = 0;
int velocityold = 0;
int octavenew = 0;
int channelnew = 0;
int commandnew = 0;
int velocitynew = 0;

byte activebutton = 0;
byte saves[(butrows * butcols) - 2][4];
byte saveload = 1;
byte adjbutkey = 0;

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
  
  for (int sn = 0; sn < (butrows * butcols) - 2; sn++) {
    for (int sv = 0; sv < 4; sv++) {
      saves[sn][sv] = 0;
    }
  }
  
  for (int lpin = 0; lpin < 8; lpin++) {
    pinMode(ledpins[lpin], OUTPUT);
  }
  
  octavepot.setSectors(10);
  channelpot.setSectors(16);
  commandpot.setSectors(8);
  velocitypot.setSectors(128);
  
  butkeypad.setDebounceTime(700);
  butkeypad.setHoldTime(1000);
  
  keypad.setDebounceTime(0);
  keypad.setHoldTime(1000000);
  
  Serial.begin(31250);
  
}

void loop() {

  octavecheck = octavepot.getSector();
  channelcheck = channelpot.getSector();
  commandcheck = commandpot.getSector();
  velocitycheck = velocitypot.getSector();
  
  octavenew = octavepot.getValue();
  channelnew = channelpot.getValue();
  commandnew = commandpot.getValue();
  velocitynew = velocitypot.getValue();
  
  if (
  (octavecheck != octave)
  && (abs(octaveold - octavenew) > 32)
  ) {
    octave = octavecheck;
    octaveold = octavenew;
    setBinaryLEDs(octave);
  } else if (
  (channelcheck != channel)
  && (abs(channelold - channelnew) > 32)
  ) {
    channel = channelcheck;
    channelold = channelnew;
    setBinaryLEDs(channel);
  } else if (
  (commandcheck != command)
  && (abs(commandold - commandnew) > 32)
  ) {
    command = commandcheck;
    commandold = commandnew;
    setDecimalLEDs(command);
  } else if (
  (velocitycheck != velocity)
  && (abs(velocityold - velocitynew) > 6)
  ) {
    velocity = velocitycheck;
    velocityold = velocitynew;
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
          
          if (butkeypad.key[a].kchar > 1) {
            adjbutkey = butkeypad.key[a].kchar - 2;
            setBinaryLEDs(adjbutkey);
            if (saveload == 0) {
              saves[adjbutkey][0] = octave;
              saves[adjbutkey][1] = channel;
              saves[adjbutkey][2] = command;
              saves[adjbutkey][3] = velocity;
            } else if (saveload == 1) {
              octave = saves[adjbutkey][0];
              channel = saves[adjbutkey][1];
              command = saves[adjbutkey][2];
              velocity = saves[adjbutkey][3];
              octavecheck = octave;
              channelcheck = channel;
              commandcheck = command;
              velocitycheck = velocity;
            }
          } else if (butkeypad.key[a].kchar == 0) {
            setBinaryLEDs(0);
            saveload = 0;
          } else if (butkeypad.key[a].kchar == 1) {
            setBinaryLEDs(255);
            saveload = 1;
          }
          
        }
      }
      
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
    if (binval < val) {
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

