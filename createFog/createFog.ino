// ------------------------------------------------------------------------------------
// 
//    CREATE FOG
// 
// Create cool iluminated fog to bring more magic into your diorama
// 
// 
// Functionality:
//    Control an e-cigarette vaporizer and a water pump to create fog for a diorama
// 
// CREATE FOG uses:
//    * a partially disassembled e-cigarette to vaporize liquid and ceate fog
//    * a water pump to puff out the fog from the vaporizer into the diorama
//    * an RGD led for cool effects
// 
// ------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------
// DEFINES
//  enable / disable (comment out) certain features on compile time
// ------------------------------------------------------------------------------------
//#define PUMP 1              // use a pump (either simple on/off or pwm driven below) to puff the fog into the diorama
//#define PWMDRIVENPUMP 1     // drive the pump through a PWM with slowly building up tension through PWM pulses
//#define VAPORIZER 1         // activate the vaporizer to actually ceate fog from a fluid
#define ACTIVATORBUTTON 1   // activate effects on the press of a button
#define PIRSENSOR 1         // activate the PIR sensor to detect motion
//#define SIMPLELED 1         // simple LED usually on pin 13 (build in led) for demo purposes
#define RGBLED 1            // create some really cool light effects in the fog with this RGB led
#define LIGHTEFFECTSWITCH 1 // switch between breathing style or stroboscope style light effects
#define TIMER 1             // this will activate the intermediate activation of the diorama every X hours
#define TIMERSWITCH 1       // this will activate the switch to activate the above timer function
#define DEBUG 1             // iutput debugging information to the serial console


// ------------------------------------------------------------------------------------
// CONSTANTS
// ------------------------------------------------------------------------------------
#ifdef PUMP
  const byte PUMP_PIN = 9;                   // in case of the PWM driven pump, this must be a PWM pin
  #ifdef PWMDRIVENPUMP
    const byte startIntensity = 128;        // intensity of the PWM pulse
  #endif
#endif

#ifdef VAPORIZER
  const byte VAPORIZER_PIN = A3;             // digital pin which activates the vaporizer
  const uint16_t vaporizerInterval = 3000;  // milliseconds the vaporizer is active and inactive
#endif

#ifdef SIMPLELED
  const byte SIMPLE_LED_PIN = 13;
#endif
#ifdef RGBLED
  const byte RED_LED_PIN = 8;
  const byte GREEN_LED_PIN = 7;
  const byte BLUE_LED_PIN = 6;
#endif

#ifdef LIGHTEFFECTSWITCH 
  const byte LIGHT_EFFECTS_SWITCH_PIN = 4;
#endif

#ifdef ACTIVATORBUTTON
  const byte EFFECTS_BUTTON_PIN = 12;
#endif

#ifdef PIRSENSOR
  const byte PIR_SENSOR_PIN = 2;
#endif

#ifdef TIMER
  // in case no one activates the diorama, this gives an interval after which the
  // fog and light effects are activated without manual intervention
  // 10800000 = 3 hours / 1800000 = 30 Minutes / 180000 = 3 Minutes
  const unsigned long maxInActiveTime = 180000;
  #ifdef TIMERSWITCH
    const byte TIMER_SWITCH_PIN = 3;
  #endif
#endif

// how long shall the effects (mainly pump and vaporizer) be active - 30 second
const unsigned long effectsMaxActiveTime = 30000;  



// ------------------------------------------------------------------------------------
// GLOBAL VARS TO DETERMINE CURRENT RUN STATE
// ------------------------------------------------------------------------------------
#ifdef ACTIVATORBUTTON
  uint8_t activatorVal = LOW;
#endif

#ifdef TIMER
  unsigned long lastEffectsTimeInMillis;
  boolean activateEffectsTimer = true;
  #ifdef TIMERSWITCH
    boolean lastEffectTimerState = false;
  #endif
#endif

#ifdef PWMDRIVENPUMP
  byte intensity = 0;                       // Actual tension: 12 - (255-intensity) * 5 / 255  
  byte crease = 3;                          // Changes motor intensity
#endif

#ifdef VAPORIZER
  unsigned long vaporizerActivateMillis=0;  // time in millis, the vaporizer was activated
  boolean vaporizerIsActive = false;
#endif

#ifdef RGBLED
  boolean redLedOn = false;
  boolean greenLedOn = false;
  boolean blueLedOn = false;
#endif
#ifdef LIGHTEFFECTSWITCH
  char lastLightEffectChosen = '-';          // to store the last light effect, that was chosen
#endif

boolean activateEffects = false;            // shall we activate the effects or not
unsigned long effectsActivationTime = 0;    // how long is the pump already active
byte loopCounter = 0;                       // we only output a . to console every 10th loop

char lightEffect = 's';                     // can either be s = stroboscope or b = breathing effect
byte lightsOn = 0;                          // used to cycle through colors in case of stroboscope


// ------------------------------------------------------------------------------------
// METHOD SKELETONS
// ------------------------------------------------------------------------------------
void decideOnActivation(void);              // set activateEffects true or false to start / stop effects
void controlEffects(void);                  // starts or stops the effects with below functions
void turnOffEffects(void);                  // turn off all effects
void turnOnPump(void);                      // turn on the water eh, fog pump
void turnOnVaporizer(void);                 // turn on the vaporizer
void turnOnLightEffects(char);              // turn on the lights, simple or rgb breathing or lightning effect

void timerSwitch(void);                     // check state of the timer switch
void lightEffectsSwitch(void);              // check state of led effects switch and set global var 


// ------------------------------------------------------------------------------------
// METHODS
// ------------------------------------------------------------------------------------
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
    Serial.print("initializing hardware"); 
  #endif
  
  #ifdef PUMP
    #ifdef DEBUG
      Serial.print(" PUMP"); 
    #endif
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
  #endif
  #ifdef VAPORIZER
    #ifdef DEBUG
      Serial.print(" VAPORIZER"); 
    #endif
    pinMode(VAPORIZER_PIN, OUTPUT);
    digitalWrite(VAPORIZER_PIN, LOW);
  #endif
  #ifdef SIMPLELED
    #ifdef DEBUG
      Serial.print(" SIMPLELED");
    #endif
    pinMode(SIMPLE_LED_PIN, OUTPUT);
    digitalWrite(SIMPLE_LED_PIN, LOW);
  #endif
  #ifdef RGBLED
    #ifdef DEBUG
      Serial.print(" RGBLED"); 
    #endif
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
  #endif
  #ifdef LIGHTEFFECTSWITCH
    #ifdef DEBUG
      Serial.print(" LIGHTEFFECTSWITCH"); 
    #endif
    pinMode(LIGHT_EFFECTS_SWITCH_PIN, INPUT);
  #endif
  #ifdef ACTIVATORBUTTON
    #ifdef DEBUG
      Serial.print(" ACTIVATORBUTTON"); 
    #endif
    pinMode(EFFECTS_BUTTON_PIN, INPUT);
  #endif
  #ifdef PIRSENSOR
    #ifdef DEBUG
      Serial.print(" PIRSENSOR"); 
    #endif
    pinMode(PIR_SENSOR_PIN, INPUT);
  #endif
  #ifdef TIMER
    #ifdef DEBUG
      Serial.print(" TIMER");
    #endif
    lastEffectsTimeInMillis = millis();
    #ifdef TIMERSWITCH
      #ifdef DEBUG
        Serial.print(" TIMERSWITCH");
      #endif
      pinMode(TIMER_SWITCH_PIN, INPUT);
      activateEffectsTimer = false;
    #endif
  #endif
  #ifdef DEBUG
    Serial.println(" ready"); 
  #endif
}


void loop() {
  // on every tenth loop, we print out a . to the serial console
  #ifdef DEBUG
  //  if (loopCounter >= 100) { Serial.print("."); loopCounter = 0; }
  #endif

  // in case the switch for the timer is included, check it's state on every loop
  checkTimerSwitch();
  
  // check if the effects shall be activated, that is there is motion near
  // or the maximum alloted time for inactivity of the diorama was reached
  decideOnActivation();
  
  // call the function that will activate or deactivate the effects
  // the pump, the vaporizer and the led
  controlEffects();
  
  //delay(300);
  loopCounter++; 
}


// check for motion, or an activator in form of US sensor or button or simply time
// and then turn on or off the effects.
void decideOnActivation(void){
  // this get's set by whichever function is run first to control the effects
  // and prevents other methods from executing.
  boolean shallICheck = true;
  boolean readMotion = false;
  
  //
  // CHECK ON ACTIVATION
  //
  #ifdef TIMER
    if (shallICheck && activateEffectsTimer && (millis()-lastEffectsTimeInMillis >= maxInActiveTime) && !activateEffects) {
      // activate the effects
      activateEffects = true;
      effectsActivationTime = millis();  // record current timestamp
      
      // prevent the next method checking for activation to run
      // in case of the time function only if we reached the max time
      shallICheck = false;
      
      #ifdef DEBUG
        Serial.print("EVENT: ");Serial.print(effectsActivationTime);Serial.print(" TIMER");
      #endif
    }
  #endif
  
  // check if the button was pressed
  #ifdef ACTIVATORBUTTON
    if (shallICheck) {
      // check if the button was pressed. if yes activate the pump for 30 seconds
      activatorVal = digitalRead(EFFECTS_BUTTON_PIN);  // read input value
      delay(50);
      if (activatorVal == HIGH) {
        // now check if the pump is already active
        if (!activateEffects) {
          // if not, activate it
          activateEffects = true;      // indicate to activate the pump
          effectsActivationTime = millis();  // record current timestamp
          
          #ifdef DEBUG
            Serial.print("EVENT: ");Serial.print(effectsActivationTime);Serial.print(" BUTTON");
          #endif
        } else {
          // deactivate the pump if it was active when the button was pressed
          activateEffects = false;
          
          #ifdef DEBUG
            Serial.print(" STOP: ");Serial.println(millis());
          #endif
        }
        
        // prevent the next method checking for activation to run
        shallICheck = false;
      }
    }
  #endif
  
  // check the ultra sonic sensor for motion
  #ifdef PIRSENSOR
    if (shallICheck) {
      readMotion = false;
      if (digitalRead(PIR_SENSOR_PIN) == HIGH) readMotion = true;
      
      if (readMotion) {
        // now check if the pump is already active
        if (!activateEffects) {
          // if not, activate it
          activateEffects = true;             // indicate to activate the pump
          effectsActivationTime = millis();   // record current timestamp
          
          #ifdef DEBUG
            Serial.print("EVENT: ");Serial.print(effectsActivationTime);Serial.print(" PIR SENSOR");
          #endif
        } else {
          #ifdef DEBUG
            if (loopCounter >= 50) {
              Serial.print(" NEW EVENT: ");Serial.print(millis());
              loopCounter = 0;
            }
          #endif
        }
        
        // prevent the next method checking for activation to run
        shallICheck = false;
      }
    }
  #endif
  
  //
  // CHECK ON DEACTIVATION
  // 
  // check, if the effects where already active for 30 seconds and if so, shut them down
  if ((millis()-effectsActivationTime) > effectsMaxActiveTime && activateEffects) {
    #ifdef TIMER
      // set the lastEffectsTimeInMillis to current time so we can later check,
      // if we want to activate the effects because of inactivity (just for show)
      lastEffectsTimeInMillis = millis();
    #endif
    
    // finally make sure the effects are turnd off in the next main loop
    activateEffects = false;
    
    #ifdef DEBUG
      Serial.print(" STOP: ");Serial.println(millis());
    #endif
  }
}

// start the effects and keep them running for set time
void controlEffects(){  
  if (!activateEffects) {
    // the called function will, depending on which hardware is installed
    // turn off all effects
    turnOffEffects();
  } else {
    #ifdef PUMP
      // if we have a pump, turn it on
      turnOnPump();
    #endif

    #ifdef VAPORIZER
      // if we have a vaporizer, intermediately turn it on and off
      turnOnVaporizer();
    #endif
    
    #ifdef SIMPLELED
      // can be called with s = simple and r = rgb led  to start the light effects of these respectively
      turnOnLightEffects('s');
    #endif
    
    #ifdef RGBLED
      turnOnLightEffects('r');
    #endif
  }
}


// turn on the pump
void turnOnPump(){
  #ifdef PUMP
    // if we have a pump wich is not PWM driven, just turn the power on
    #ifndef PWMDRIVENPUMP
      digitalWrite(PUMP_PIN, HIGH);
    #endif
    
    // if we want the pump to start slowly and build up tension,
    // we need to use the PWM driven pump option and do so accordingly here
    #ifdef PWMDRIVENPUMP
      analogWrite(PUMP_PIN, intensity);    // Writes PWM to the motor   
      intensity = intensity + crease;
      
      #ifdef DEBUG
        Serial.print(" ");Serial.print(intensity);
      #endif
      
      if (intensity == 0 || intensity >= 255) {
        crease = -crease;                // Increase to decrease due to line 255
      }
    #endif
  #endif
}


// turn on the vaporizer
void turnOnVaporizer(){
  #ifdef VAPORIZER
    // the vaporizer is turned on and off in circles for 3 seconds each
    if (millis()-vaporizerActivateMillis > vaporizerInterval)  { 
      vaporizerIsActive = !vaporizerIsActive; 
      vaporizerActivateMillis = millis();
    }
    if (vaporizerIsActive) {
      digitalWrite(VAPORIZER_PIN, HIGH);    // turn vaporizer on
    } else {
      digitalWrite(VAPORIZER_PIN, LOW);     // turn vaporizer off
    }
  #endif
}


// can be called with s = simple and r = rgb led  to start the light effects of these respectively
void turnOnLightEffects(char light){
  if (light == 's') {
    #ifdef SIMPLELED
      if (lightEffect == 'b') {
        analogWrite(SIMPLE_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
      } else {
        digitalWrite(SIMPLE_LED_PIN, HIGH);
      }
    #endif
  } else if (light == 'r') {
    #ifdef RGBLED
      if (lightEffect == 'b') {
        // in case the light effects switch is LOW, we want a breathing like pulsating of light effects
        
        analogWrite(RED_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
        //delay(driftValue);
        //analogWrite(GREEN_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
        analogWrite(GREEN_LED_PIN, 128);
        //delay(driftValue);
        analogWrite(BLUE_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
      } else if (lightEffect == 'c') {
        // this is another breathin gstyle effect that cycles through all colors of the rgb led

        
      } else if (lightEffect == 's') {
        // in case the light effects switch is HIGH, we want a thunderstorm style lightning effect
        
        lightsOn++;
        if (lightsOn == 1) {
          analogWrite(RED_LED_PIN, 64);
          analogWrite(BLUE_LED_PIN, 0);
          analogWrite(GREEN_LED_PIN, 0);
        } else if (lightsOn == 2) {
          analogWrite(RED_LED_PIN, 127);
          analogWrite(BLUE_LED_PIN, 64);
          analogWrite(GREEN_LED_PIN, 0);
        } else if (lightsOn == 3) {
          analogWrite(RED_LED_PIN, 255);
          analogWrite(BLUE_LED_PIN, 127);
          analogWrite(GREEN_LED_PIN, 64);
        } else if (lightsOn == 4) {
          analogWrite(RED_LED_PIN, 127);
          analogWrite(BLUE_LED_PIN, 255);
          analogWrite(GREEN_LED_PIN, 127);
        } else if (lightsOn == 5) {
          analogWrite(RED_LED_PIN, 64);
          analogWrite(BLUE_LED_PIN, 127);
          analogWrite(GREEN_LED_PIN, 255);
        } else if (lightsOn == 6) {
          analogWrite(RED_LED_PIN, 0);
          analogWrite(BLUE_LED_PIN, 64);
          analogWrite(GREEN_LED_PIN, 127);
        } else if (lightsOn == 7) {
          analogWrite(RED_LED_PIN, 64);
          analogWrite(BLUE_LED_PIN, 0);
          analogWrite(GREEN_LED_PIN, 64);
        } else if (lightsOn == 8) {
          analogWrite(RED_LED_PIN, 127);
          analogWrite(BLUE_LED_PIN, 64);
          analogWrite(GREEN_LED_PIN, 0);
        } else if (lightsOn == 9) {
          analogWrite(RED_LED_PIN, 255);
          analogWrite(BLUE_LED_PIN, 127);
          analogWrite(GREEN_LED_PIN, 64);
        } else if (lightsOn == 10) {
          analogWrite(RED_LED_PIN, 127);
          analogWrite(BLUE_LED_PIN, 255);
          analogWrite(GREEN_LED_PIN, 127);
        } else if (lightsOn == 11) {
          analogWrite(RED_LED_PIN, 64);
          analogWrite(BLUE_LED_PIN, 127);
          analogWrite(GREEN_LED_PIN, 255);
        } else if (lightsOn == 12) {
          analogWrite(RED_LED_PIN, 64);
          analogWrite(BLUE_LED_PIN, 64);
          analogWrite(GREEN_LED_PIN, 127);
        } else if (lightsOn == 13) {
          analogWrite(RED_LED_PIN, 64);
          analogWrite(BLUE_LED_PIN, 64);
          analogWrite(GREEN_LED_PIN, 64);
        }  else {
          lightsOn = 0;
          analogWrite(RED_LED_PIN, 255);
          analogWrite(BLUE_LED_PIN, 255);
          analogWrite(GREEN_LED_PIN, 255);
        }
        delay(100);
      }
    #endif
  }
  
}


// shuts down all effects - you might have guessed that
void turnOffEffects(){
  #ifdef PUMP
    digitalWrite(PUMP_PIN, LOW);         // turn pump off
    #ifdef PWMDRIVENPUMP
      intensity = startIntensity;       // reset pump motor intensity
    #endif
  #endif
  #ifdef VAPORIZER
    digitalWrite(VAPORIZER_PIN, LOW);    // turn vaporizer off
  #endif
  #ifdef SIMPLELED
    digitalWrite(SIMPLE_LED_PIN, LOW);    // turn led off
  #endif
  #ifdef RGBLED
    digitalWrite(RED_LED_PIN, LOW);       // turn led off
    digitalWrite(GREEN_LED_PIN, LOW);     // turn led off
    digitalWrite(BLUE_LED_PIN, LOW);      // turn led off
  #endif
}


// check wether or not the time functionality shall be used to intermediately start the effects
void checkTimerSwitch(){
  #ifdef TIMERSWITCH
    byte switchTimerValue = digitalRead(TIMER_SWITCH_PIN);
    if (switchTimerValue == HIGH) {
      activateEffectsTimer = true;
      if (activateEffectsTimer != lastEffectTimerState) {
        lastEffectTimerState = activateEffectsTimer;
        #ifdef DEBUG
          Serial.print("EVENT: ");Serial.print(millis());Serial.println(" TIMER ON");
        #endif
      }
      
    } else {
      activateEffectsTimer = false;
      if (activateEffectsTimer != lastEffectTimerState) {
        lastEffectTimerState = activateEffectsTimer;
        #ifdef DEBUG
          Serial.print("EVENT: ");Serial.print(millis());Serial.println(" TIMER OFF");
        #endif
      }
      
    }
  #endif
}

// switch between strobsocope style lightning or slowly pulsating light effect
void lightEffectsSwitch(){
  #ifdef LIGHTEFFECTSWITCH
    byte switchLightEffectValue = digitalRead(LIGHT_EFFECTS_SWITCH_PIN);
    
    if (switchLightEffectValue  == HIGH) {
      lightEffect = 's';
      if (lightEffect != lastLightEffectChosen) {
        lastLightEffectChosen = lightEffect;
        #ifdef DEBUG
          Serial.print("EVENT: ");Serial.print(millis());Serial.println(" LIGHT EFFECT STROBOSCOPE");
        #endif
      }
    } else {
      lightEffect = 'b';
      if (lightEffect != lastLightEffectChosen) {
        lastLightEffectChosen = lightEffect;
        #ifdef DEBUG
          Serial.print("EVENT: ");Serial.print(millis());Serial.println(" LIGHT EFFECT BREATHING");
        #endif
      }
    }
  #endif
}
