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


// ONLY ONE OF THE TWO OPTIONS BELOW CAN BE CHOSEN
//
// Activate this option to use a sine wave style breathing with more less fixed colors on the rgb led
//#define RGBSINEWAVE 1
//
// Activate the next option to use the properties of the RGB Colour Cube to cycle through colors
// The RGB colour space can be viewed as a cube of colour. If we assume a cube of dimension 1, then the 
// coordinates of the vertices for the cubve will range from (0,0,0) to (1,1,1) (all black to all white).
// The transitions between each vertex will be a smooth colour flow and we can exploit this by using the 
// path coordinates as the LED transition effect.
// Total traversal time is ((MAX_RGB_VALUE - MIN_RGB_VALUE) * TRANSITION_DELAY) + WAIT_DELAY
// eg, ((255-0)*70)+500 = 18350ms = 18.35s 
// -->  we should make sure that we have enough effect time for at least one cycle
// 
// All credits for the code of this cool color cycle go to Marco Colli (April 2012)
//    https://forum.arduino.cc/index.php?topic=102040.0
#define RGBCOLORCYCLE 1


// ------------------------------------------------------------------------------------
// CONSTANTS
// ------------------------------------------------------------------------------------
#ifdef PUMP
  const byte PUMP_PIN = 9;                   // in case of the PWM driven pump, this must be a PWM pin
  #ifdef PWMDRIVENPUMP
    const byte startIntensity = 128;         // intensity of the PWM pulse
  #endif
#endif

#ifdef VAPORIZER
  const byte VAPORIZER_PIN = A3;             // digital pin which activates the vaporizer
  const uint16_t vaporizerInterval = 3000;   // milliseconds the vaporizer is active and inactive
#endif

#ifdef SIMPLELED
  const byte SIMPLE_LED_PIN = 13;
#endif
#ifdef RGBLED
  const byte RED_LED_PIN = 8;
  const byte GREEN_LED_PIN = 7;
  const byte BLUE_LED_PIN = 6;
#endif

// in case of the RGB Color Cycler we need a lot of stuff ...
#ifdef RGBCOLORCYCLE
  // Constants for readability are better than magic numbers
  // Used to adjust the limits for the LED, especially if it has a lower ON threshold
  #define  MIN_RGB_VALUE  10   // no smaller than 0. 
  #define  MAX_RGB_VALUE  255  // no bigger than 255.
  
  // Slowing things down we need ...
  #define  TRANSITION_DELAY  70   // in milliseconds, between individual light changes
  #define  WAIT_DELAY        500  // in milliseconds, at the end of each traverse
  //
  // Total traversal time is ((MAX_RGB_VALUE - MIN_RGB_VALUE) * TRANSITION_DELAY) + WAIT_DELAY
  // eg, ((255-0)*70)+500 = 18350ms = 18.35s
  
  // Structure to contain a 3D coordinate
  typedef struct {
    byte  x, y, z;
  } coord;
  
  static coord  v; // the current rgb coordinates (colour) being displayed
  
  /*
  Vertices of a cube
        
      C+----------+G
      /|        / |
    B+---------+F |
     | |       |  |    y   
     |D+-------|--+H   ^  7 z
     |/        | /     | /
    A+---------+E      +--->x
  
  */
  const coord vertex[] = {
   //x  y  z      name
    {0, 0, 0}, // A or 0
    {0, 1, 0}, // B or 1
    {0, 1, 1}, // C or 2
    {0, 0, 1}, // D or 3
    {1, 0, 0}, // E or 4
    {1, 1, 0}, // F or 5
    {1, 1, 1}, // G or 6
    {1, 0, 1}  // H or 7
  };
  
  /*
  A list of vertex numbers encoded 2 per byte.
  Hex digits are used as vertices 0-7 fit nicely (3 bits 000-111) and have the same visual
  representation as decimal, so bytes 0x12, 0x34 ... should be interpreted as vertex 1 to 
  v2 to v3 to v4 (ie, one continuous path B to C to D to E).
  */
  const byte path[] = {
    0x01, 0x23, 0x76, 0x54, 0x03, 0x21, 0x56, 0x74,             // trace the edges
    0x13, 0x64, 0x16, 0x02, 0x75, 0x24, 0x35, 0x17, 0x25, 0x70, // do the diagonals
  };
  
  #define  MAX_PATH_SIZE  (sizeof(path)/sizeof(path[0]))  // size of the array
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

// how long shall the effects (pump, vaporizer and lights) be active?
// usually 30 second
#ifndef RGBCOLORCYCLE
  const unsigned long effectsMaxActiveTime = 30000;
#endif
// in case we use the rgb color cycle, we need approx 18,35 second for one cycle
#ifdef RGBCOLORCYCLE
  const unsigned long effectsMaxActiveTime = 36700;  // this gives us enough time for 2 cycles
#endif



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

void traverseColorCube(int, int, int);      // traverse the vertices of a 3d color cube to determine next color value

void checkTimerSwitch(void);                // check state of the timer switch
void checkLightEffectsSwitch(void);         // check state of led effects switch and set global var 


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
  // in case the switch for the timer is included, check it's state on every loop
  #ifdef TIMERSWITCH
    checkTimerSwitch();
  #endif
  
  // in case the switch to chose the light effects style is included, check it's state on every loop
  #ifdef LIGHTEFFECTSWITCH
    checkLightEffectsSwitch();
  #endif
  
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
        // in case the light effects switch is LOW, we want a smooth light effect
        analogWrite(SIMPLE_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
      } else {
        // in case the light effects switch is HIGH, we want the light just to be on
        digitalWrite(SIMPLE_LED_PIN, HIGH);
      }
    #endif
  } else if (light == 'r') {
    #ifdef RGBLED
      if (lightEffect == 'b') {
        // in case the light effects switch is LOW, we want a smooth light effect
        // this can either be a breathing like pulsating or a cycle through all given colors
        // which one is determined at compile time with the directive RGBSINEWAVE or RGBCOLORCYCLE
        #ifdef RGBSINEWAVE
          // this effect gives a natural pulsating or breathing like effect with more less fixed colors
          
          analogWrite(RED_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
          //delay(driftValue);
          //analogWrite(GREEN_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
          analogWrite(GREEN_LED_PIN, 128);
          //delay(driftValue);
          analogWrite(BLUE_LED_PIN, 128 + 127 * cos(2 * PI / 20000 * millis()));
        #endif
        
        #ifdef RGBCOLORCYCLE
          // this effect cycles through all colors of the rgb led
          // code is by Marco Coll from April 2012
          
          int    v1, v2=0;    // the new vertex and the previous one
          
          // initialise the place we start from as the first vertex in the array
          v.x = (vertex[v2].x ? MAX_RGB_VALUE : MIN_RGB_VALUE);
          v.y = (vertex[v2].y ? MAX_RGB_VALUE : MIN_RGB_VALUE);
          v.z = (vertex[v2].z ? MAX_RGB_VALUE : MIN_RGB_VALUE);
          
          // Now just loop through the path, traversing from one point to the next
          for (int i = 0; i < 2*MAX_PATH_SIZE; i++) {
            // !! loop index is double what the path index is as it is a nybble index !!
            v1 = v2;
            if (i&1)  // odd number is the second element and ...
              v2 = path[i>>1] & 0xf;  // ... the bottom nybble (index /2) or ...
            else      // ... even number is the first element and ...
              v2 = path[i>>1] >> 4;  // ... the top nybble
              
            traverse(vertex[v2].x-vertex[v1].x, 
                     vertex[v2].y-vertex[v1].y, 
                     vertex[v2].z-vertex[v1].z);
          }
        #endif
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


// Move along the colour line from where we are to the next vertex of the cube.
// The transition is achieved by applying the 'delta' value to the coordinate.
// By definition all the coordinates will complete the transition at the same 
// time as we only have one loop index.
// this code is by Marco Colli April 2012
void traverse(int dx, int dy, int dz) {
  #ifdef RGBCOLORCYCLE
    if ((dx == 0) && (dy == 0) && (dz == 0))   // no point looping if we are staying in the same spot!
      return;
      
    for (int i = 0; i < MAX_RGB_VALUE-MIN_RGB_VALUE; i++, v.x += dx, v.y += dy, v.z += dz) {
      // set the colour in the LED
      analogWrite(RED_LED_PIN, v.x);
      analogWrite(GREEN_LED_PIN, v.y);
      analogWrite(BLUE_LED_PIN, v.z);
      
      delay(TRANSITION_DELAY);  // wait fot the transition delay
    }
    
    delay(WAIT_DELAY);          // give it an extra rest at the end of the traverse
  #endif
}


// check wether or not the time functionality shall be used to intermediately start the effects
void checkTimerSwitch(){
  #ifdef TIMERSWITCH
    byte switchTimerValue = digitalRead(TIMER_SWITCH_PIN);
    if (switchTimerValue == HIGH) {
      // in case the timer switch is HIGH we want to activate the effects every now and then automatically
      activateEffectsTimer = true;
      if (activateEffectsTimer != lastEffectTimerState) {
        lastEffectTimerState = activateEffectsTimer;
        #ifdef DEBUG
          Serial.print("EVENT: ");Serial.print(millis());Serial.println(" TIMER ON");
        #endif
      }
      
    } else {
      // in case the timer switch is LOW we only want to activate the effects by manually pressing a button or with the PIR sensor
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
void checkLightEffectsSwitch(){
  #ifdef LIGHTEFFECTSWITCH
    byte switchLightEffectValue = digitalRead(LIGHT_EFFECTS_SWITCH_PIN);
    
    if (switchLightEffectValue  == HIGH) {
      // in case the light effects switch is HIGH we want a stroboscope or lightning effect
      
      lightEffect = 's';
      if (lightEffect != lastLightEffectChosen) {
        lastLightEffectChosen = lightEffect;
        #ifdef DEBUG
          Serial.print("EVENT: ");Serial.print(millis());Serial.println(" LIGHT EFFECT STROBOSCOPE");
        #endif
      }
    } else {
      // in case the light effects switch is LOW we want a smooth light effect, breathing or color cycling
      
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
