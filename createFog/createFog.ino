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
#define PUMP 1            // use a pump (either simple on/off or pwm driven below) to puff the fog into the diorama
//#define PWMDRIVENPUMP 1   // drive the pump through a PWM with slowly building up tension through PWM pulses
//#define VAPORIZER 1       // activate the vaporizer to actually ceate fog from a fluid
#define ACTIVATORBUTTON 1 // activate effects on the press of a button
//#define USSENSOR 1        // activate the effects in case someone comes closer than 10 cm - non functinal at the moment
#define PIRSENSOR 1       // activate the PIR sensor to detect motion
#define SIMPLELED 1       // simple LED on Pin 13 - usually the build in led for demo purposes
//#define RGBLED 1          // create some really cool light effects in the fog with this RGB led
#define TIMER 1           // this will activate the intermediate activation of the diorama every X hours
#define TIMERSWITCH 1     // this will activate the switch to activate the above timer function
#define DEBUG 1           // iutput debugging information to the serial console


// ------------------------------------------------------------------------------------
// CONSTANTS
// ------------------------------------------------------------------------------------
#ifdef PUMP
  const byte pumpPin = 9;                   // in case of the PWM driven pump, this must be a PWM pin
  #ifdef PWMDRIVENPUMP
    const byte startIntensity = 128;          // intensity of the PWM pulse
  #endif
#endif

#ifdef VAPORIZER
  const byte vaporizerPin = A3;              // digital pin which activates the vaporizer
  const uint16_t vaporizerInterval = 3000;  // milliseconds the vaporizer is active and inactive
#endif

#ifdef SIMPLELED
  const byte ledPin = 13;
#endif
#ifdef RGBLED
  const byte redPin = 10;
  const byte greenPin = 11;
  const byte bluePin = 8;
#endif

#ifdef ACTIVATORBUTTON
  const byte activatorPin = 12;
#endif

#ifdef USSENSOR
  // define pin numbers for the ultra sonic sensor
  const byte usSensorTrigPin = A1;
  const byte usSensorEchoPin = A2;
  // if anything is closer that 10cm to the diorama, we want to 
  // activate the fog and light effects
  const uint8_t minDistance = 10; 
#endif

#ifdef PIRSENSOR
  const byte pirSensorPin = 2;
#endif

#ifdef TIMER
  // in case no one activates the diorama, this gives an interval after which the
  // fog and light effects are activated without manual intervention
  // 10800000 = 3 hours / 1800000 = 30 Minutes / 180000 = 3 Minutes
  const unsigned long maxInActiveTime = 180000;
  #ifdef TIMERSWITCH
    const byte timeSwitchPin = 3;
  #endif
#endif

// how long shall the effects (mainly pump and vaporizer) be active - 30 second
const unsigned long effectsMaxActiveTime = 30000;  



// ------------------------------------------------------------------------------------
// GLOBAL VARS TO DETERMINE CURRENT RUN STATE
// ------------------------------------------------------------------------------------
#ifdef PWMDRIVENPUMP
  byte intensity = 0;                       // Actual tension: 12 - (255-intensity) * 5 / 255  
  byte crease = 3;                          // Changes motor intensity
#endif

#ifdef VAPORIZER
  unsigned long vaporizerActivateMillis=0;  // time in millis, the vaporizer was activated
  boolean vaporizerIsActive = false;
#endif

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

boolean activateEffects = false;               // shall we activate the effects or not
unsigned long effectsActivationTime = 0;             // how long is the pump already active
byte loopCounter = 0;                       // we only output a . to console every 10th loop


// ------------------------------------------------------------------------------------
// METHOD SKELETONS
// ------------------------------------------------------------------------------------
void controlEffects(void);                  // starts or stops the water pump
void decideOnActivation(void);              // set activateEffects true or false
boolean readUsSensor(void);                 // return true in case motion is detected, false otherwise
void timerSwitch(void);                     // check state of the timer switch



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
    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW);
  #endif
  #ifdef VAPORIZER
    #ifdef DEBUG
      Serial.print(" VAPORIZER"); 
    #endif
    pinMode(vaporizerPin, OUTPUT);
    digitalWrite(vaporizerPin, LOW);
  #endif
  #ifdef SIMPLELED
    #ifdef DEBUG
      Serial.print(" SIMPLELED"); 
    #endif
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
  #endif
  #ifdef RGBLED
    #ifdef DEBUG
      Serial.print(" RGBLED"); 
    #endif
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);
  #endif
  #ifdef ACTIVATORBUTTON
    #ifdef DEBUG
      Serial.print(" ACTIVATORBUTTON"); 
    #endif
    pinMode(activatorPin, INPUT);
  #endif
  #ifdef USSENSOR
    #ifdef DEBUG
      Serial.print(" USSENSOR"); 
    #endif
    pinMode(usSensorTrigPin, OUTPUT); // Sets the usSensorTrigPin as an Output
    pinMode(usSensorEchoPin, INPUT); // Sets the usSensorEchoPin as an Input
  #endif
  #ifdef PIRSENSOR
    #ifdef DEBUG
      Serial.print(" PIRSENSOR"); 
    #endif
    pinMode(pirSensorPin, INPUT);
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
      pinMode(timeSwitchPin, INPUT);
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
  
  delay(100);
  loopCounter++; 
}


// check for motion, or an activator in form of US sensor or button or simply time
// and then turn on or off the effects.
void decideOnActivation(void){
  // this get's set by whichever function is run first to control the effects
  // and prevents other methods from executing.
  boolean shallICheck = true;
  boolean readMotion = false;
  
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
      activatorVal = digitalRead(activatorPin);  // read input value
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
      if (digitalRead(pirSensorPin) == HIGH) readMotion = true;
      
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
  
  // check the ultra sonic sensor for motion
  #ifdef USSENSOR
    if (shallICheck) {
      readMotion = false;
      readMotion = readUsSensor(); 
      
      if (readMotion) {
        // now check if the pump is already active
        if (!activateEffects) {
          // if not, activate it
          activateEffects = true;      // indicate to activate the pump
          effectsActivationTime = millis();  // record current timestamp
          
          #ifdef DEBUG
            Serial.print("EVENT: ");Serial.print(effectsActivationTime);Serial.print(" US SENSOR");
          #endif
        }
        
        // prevent the next method checking for activation to run
        shallICheck = false;
      } 
    }
  #endif
  
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

// start the water pump and keep it running for set time
void controlEffects(){  
  if (!activateEffects) {
    #ifdef PUMP
      digitalWrite(pumpPin, LOW);         // turn pump off
      #ifdef PWMDRIVENPUMP
        intensity = startIntensity;         // reset pump motor intensity
      #endif
    #endif
    #ifdef VAPORIZER
      digitalWrite(vaporizerPin, LOW);    // turn vaporizer off
    #endif
    #ifdef SIMPLELED
      digitalWrite(ledPin, LOW);          // turn led off
    #endif
  } else {
    #ifdef PUMP
      // if we want the pump to start slowly and build up tension,
      // we need to use the PWM driven pump option and do so accordingly here
      #ifdef PWMDRIVENPUMP
        analogWrite(pumpPin, intensity);    // Writes PWM to the motor   
        intensity = intensity + crease;
        
        #ifdef DEBUG
          Serial.print(" ");Serial.print(intensity);
        #endif
        
        if (intensity == 0 || intensity >= 255) {
          crease = -crease;                // Increase to decrease due to line 255
        }
      #endif
      // if we have a pump wich is not PWM driven as in starts up slowly, just turn the power on
      #ifndef PWMDRIVENPUMP
        digitalWrite(pumpPin, HIGH);
      #endif
    #endif

    #ifdef VAPORIZER
      // the vaporizer is turned on and off in circles for 3 seconds each
      if (millis()-vaporizerActivateMillis > vaporizerInterval)  { 
        vaporizerIsActive = !vaporizerIsActive; 
        vaporizerActivateMillis = millis();
      }
      if (vaporizerIsActive) {
        digitalWrite(vaporizerPin, HIGH);    // turn vaporizer on
      } else {
        digitalWrite(vaporizerPin, LOW);     // turn vaporizer off
      }
    #endif

    #ifdef SIMPLELED
      digitalWrite(ledPin, HIGH);         // turn led on
    #endif

    // finally we put a small delay here, so pump and vaporizer and everything else is not overburdened
    delay(200);
  }
}

// drive an ultra sonic distance sensor to read check whether or not someone is near
boolean readUsSensor(){
  #ifdef USSENSOR
    // defines variables
    long duration = 0;
    int distance = 10000;
    
    // Clears the usSensorTrigPin
    digitalWrite(usSensorTrigPin, LOW);
    delayMicroseconds(2);
    
    // Sets the usSensorTrigPin on HIGH state for 10 micro seconds
    digitalWrite(usSensorTrigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(usSensorTrigPin, LOW);
    
    // Reads the usSensorEchoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(usSensorEchoPin, HIGH);
    
    // Calculating the distance
    distance= duration*0.034/2;
    
    // Prints the distance on the Serial Monitor
    #ifdef DEBUG
      Serial.print(" Distance: ");
      Serial.print(distance);
    #endif
  
    // only in case something is near enoufgh, we want to activate the effects
    if (distance < minDistance) {
      return(true);
    } else {
      return(false);
    }
  #endif

  // in case USSENSOR is not defined, this always returns false
  #ifndef USSENSOR
    return(false);
  #endif
}

void checkTimerSwitch(){
  #ifdef TIMERSWITCH
    byte switchTimerValue = digitalRead(timeSwitchPin);
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
