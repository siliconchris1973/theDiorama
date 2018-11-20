//---------------------------------------
// DEFINES
//  enable / disable (comment out) certain features
//---------------------------------------
#define PUMP 1
//#define ANALOGPUMP 1
#define DIGITALPUMP 1
#define ACTIVATOR 1
#define LED 1
#define DEBUG 1

#ifdef PUMP
  const byte pumpPin = 9;                   // in case of the analog pump, this must be a PWM pin
  byte intensity = 0;                       // Actual tension: 12 - (255-intensity) * 5 / 255  
  const byte startIntensity = 128;          // intensity of the PWM pulse
  byte crease = 3;                          // Changes motor intensity
#endif

#ifdef LED
  const byte ledPin = 13;
#endif

#ifdef ACTIVATOR
  const byte activatorPin = 12;
  uint8_t activatorVal = LOW;
#endif


boolean activatePump = false;               // shall we activate the pump or not
unsigned long activateTime = 0;             // how long is the pump already active
const unsigned long maxActiveTime = 30000;  // how long shall the pump be active - 300 second
byte loopCounter = 0;                       // we only output a . to console every 10th loop

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
    Serial.print("initializing hardware"); 
  #endif
  
  #ifdef PUMP
    #ifdef DEBUG
      Serial.print(" PUMP"); 
    #endif
    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW);
  #endif
  #ifdef LED
    #ifdef DEBUG
      Serial.print(" LED"); 
    #endif
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
  #endif
  #ifdef ACTIVATOR
    #ifdef DEBUG
      Serial.print(" ACTIVATOR"); 
    #endif
    pinMode(activatorPin, INPUT);
  #endif
  #ifdef DEBUG
    Serial.print(" ready"); 
  #endif
}

void loop() {
  // on every tenth loop, we print out a . to the serial console
  #ifdef DEBUG
    if (loopCounter >= 10) { Serial.print("."); loopCounter = 0; }
  #endif

  #ifdef ACTIVATOR
    // check if the button was pressed. if yes activate the pump for 30 seconds
    activatorVal = digitalRead(activatorPin);  // read input value
    delay(50);
    if (activatorVal == HIGH) {
      // now check if the pump is already active
      if (!activatePump) {
        // if not, activate it
        activatePump = true;      // indicate to activate the pump
        activateTime = millis();  // record current timestamp
        
        #ifdef DEBUG
          Serial.print("+");
        #endif
      } else {
        // deactivate the pump if it was active when the button was pressed
        activatePump = false;
      }
    } 
  #endif
  
  // now check, if the pump was already active for 30 seconds and if so, shut it down
  if ((millis()-activateTime) > maxActiveTime && activatePump) {
    activatePump = false;
    
    #ifdef DEBUG
      Serial.print("-");
    #endif
  }

  if (!activatePump) {
    #ifdef LED
      digitalWrite(ledPin, LOW);          // turn led off
    #endif
    #ifdef PUMP
      digitalWrite(pumpPin, LOW);         // turn pump off
      intensity = startIntensity;         // reset pump motor intensity
    #endif
  } else {
    #ifdef LED
      digitalWrite(ledPin, HIGH);         // turn led on
    #endif
    #ifdef PUMP
      #ifdef ANALOGPUMP
        analogWrite(pumpPin, intensity);    // Writes PWM to the motor   
        intensity = intensity + crease;
        
        #ifdef DEBUG
          Serial.print(" ");Serial.print(intensity);
        #endif
        if (intensity == 0 || intensity >= 255) {
          crease = -crease;                // Increase to decrease due to line 101
        }
        
        delay(200);
      #endif
      #ifdef DIGITALPUMP
        digitalWrite(pumpPin, HIGH);
      #endif
    #endif 
  }
  
  delay(100);
  //loopCounter = loopCounter +1; 
  loopCounter++; 
}
