 /*
This code was written to control a solenoid valve in order to manage the amount of time our show could be used.

More info can be found here: https://youtu.be/1Lek2_mfyv4 and here: https://adambyers.com/2017/05/shower-controller-manage-shower-length/
*/

//// STATES ////

#define S_ready 1 // Ready to accept button press to start a shower. Valve closed.
#define S_pause 2 // Delay after last shower to prevent immediate push of shower button. Valve closed.
#define S_shower 3 // Showering, valve open.
#define S_override 4 // Manual override. Valve open until override is turned off.

int state = S_pause; // Default startup state. We stat here to prevent abuse from removing power from the controller.

//// VARIABLES ////

// PIN Names
const int ValveFET = 2; // PIN connected to MOSFET that controls valve.
const int ShowerSW = 3; // PIN that the shower enable switch (button) is connected to.
const int ShowerLED = 4; // PIN that the LED to indicate shower is allowed (solid) or active (blinking. In my case the LED is integrated into the button used.
const int StopSW = 5; // PIN that shower stop switch (button) is connected to. This is so the user can cancel any remaining shower time. System will go to pause state if pushed.
const int StopLED = 6; // PIN that the LED for the stop button is connected to. This isn't really used to indicate anything specific, but is blinked along w/ShowerLED when in the pause or override state (in my case the LED integrated into the button i used).
const int OverrideSW = 7; // PIN that the override switch is connected to. This is housed inside the control box, which is remote from where the user controls are.
const int Buzzer = 8; // PIN that the buzzer is connected to. In this case the buzzer is connected through an NPN transistor.

// Time vars
const long AllowedShowerTime = 360000; // How long a shower is allowed (in milliseconds).
const long PauseTime = 90000 ; // How long after AllowedShowerTime has elapsed, or the stop button is pressed, before we allow another shower (in milliseconds).
long AllowedShowerTimeVar = AllowedShowerTime; // AllowedShowerTimeVar is manipulated in the sketch (subtracted from to keep time) but should be equal to AllowedShowerTime at start and when reset.
long PauseTimeVar = PauseTime; // Same as the relationship between AllowedShowerTimeVar and AllowedShowerTime.
unsigned long CurrentMillis;
unsigned long PreviousMillis;

int ButtonLEDstate = LOW; // Set the LEDs initial state to off.
int BlinkIntervalSlow = 1000;
int BlinkIntervalFast = 100;

int BuzzerInterval = 1000;
int BuzzerState = LOW; // Initialize as LOW.

//// SETUP ////

void setup() {
  pinMode(ValveFET, OUTPUT);
  pinMode(ShowerSW, INPUT);
  pinMode(ShowerLED, OUTPUT);
  pinMode(StopSW, INPUT);
  pinMode(StopLED, OUTPUT);
  pinMode(OverrideSW, INPUT);
  pinMode(Buzzer, OUTPUT);

  // Enable internal resistors - set inputs as HIGH on button PINs so we don't have to use external resistors.
  digitalWrite(ShowerSW, HIGH);
  digitalWrite(StopSW, HIGH);
  digitalWrite(OverrideSW, HIGH);

  CurrentMillis = millis(); // Initialize the clock at boot.

  digitalWrite(ValveFET, LOW); // Make sure the valve is closed at boot.

  //Serial.begin(9600); // For testing, comment out in production.
}

//// LOOP ////

void loop() {

  //Serial.println(state); // For testing, comment out in production.

  switch (state) {

    case S_ready:
      F_ready();
      break;

    case S_pause:
      F_pause();
      break;

    case S_shower:
      F_shower();
      break;

    case S_override:
      F_override();
      break;
  }
}

//// FUNCTIONS ////

void F_ready() {
  PauseTimeVar = PauseTime; // Reset
  AllowedShowerTimeVar = AllowedShowerTime; // Reset

  digitalWrite(Buzzer, LOW); // Make sure the buzzer is turned off.
  digitalWrite(ValveFET, LOW); // Close the valve.

  digitalWrite(ShowerLED, HIGH); // Turn on the shower button LED to indicate it's press-able.
  digitalWrite(StopLED, LOW); // Turn off the stop button LED, to indicate that it's not functional in this state.

  if (digitalRead(ShowerSW) == LOW) {
    CurrentMillis = millis();
    state = S_shower;
  }

  if (digitalRead(OverrideSW) == LOW) {
    state = S_override;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void F_pause() {
  digitalWrite(ValveFET, LOW); // Close the valve.

  if (millis() - CurrentMillis > 1000) {
    PauseTimeVar = PauseTimeVar - 1000;
    CurrentMillis = millis();
  }

  F_BlinkLEDs(); // Blink both button LEDs.

  // If there is 2 sec of PauseTime left, start sounding an audible alarm.
  if (PauseTimeVar <= 2000) { // 2 sec
    F_buzzer();
  }

  PreviousMillis = CurrentMillis;

  if (PauseTimeVar == 0) {
    digitalWrite(Buzzer, LOW);
    state = S_ready;
  }

  if (digitalRead(OverrideSW) == LOW) {
    state = S_override;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void F_shower() {
  digitalWrite(ValveFET, HIGH); // Open the valve.

  if (millis() - CurrentMillis > 1000) {
    AllowedShowerTimeVar = AllowedShowerTimeVar - 1000;
    CurrentMillis = millis();
  }

  digitalWrite(StopLED, HIGH);
  F_BlinkShowerLED();

  // If there is 60 sec of ShowerTime left, start sounding an audible alarm.
  if (AllowedShowerTimeVar <= 60000) { // 60 sec
    F_buzzer();
  }

  PreviousMillis = CurrentMillis;

  if (AllowedShowerTimeVar == 0) {
    digitalWrite(Buzzer, LOW); // Make sure we turn the buzzer off.
    CurrentMillis = millis();
    state = S_pause;
  }

  if (digitalRead(OverrideSW) == LOW) {
    state = S_override;
  }

  if (digitalRead(StopSW) == LOW) {
    CurrentMillis = millis();
    state = S_pause;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void F_override() {
  digitalWrite(ValveFET, HIGH);
  digitalWrite(Buzzer, LOW);

  F_BlinkLEDsFast();

  PreviousMillis = CurrentMillis;

  if (digitalRead(OverrideSW) == HIGH) {
    state = S_ready;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void F_buzzer() {
  if (CurrentMillis - PreviousMillis >= BuzzerInterval) {
    if (BuzzerState == LOW) {
      BuzzerState = HIGH;
    } else {
      BuzzerState = LOW;
    }
    digitalWrite(Buzzer, BuzzerState);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void F_BlinkLEDs() {
  if (CurrentMillis - PreviousMillis >= BlinkIntervalSlow) {
    if (ButtonLEDstate == LOW) {
      ButtonLEDstate = HIGH;
    } else {
      ButtonLEDstate = LOW;
    }
    digitalWrite(ShowerLED, ButtonLEDstate);
    digitalWrite(StopLED, ButtonLEDstate);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void F_BlinkLEDsFast() {
  if (CurrentMillis - PreviousMillis >= BlinkIntervalFast) {
    if (ButtonLEDstate == LOW) {
      ButtonLEDstate = HIGH;
    } else {
      ButtonLEDstate = LOW;
    }
    digitalWrite(ShowerLED, ButtonLEDstate);
    digitalWrite(StopLED, ButtonLEDstate);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void F_BlinkShowerLED() {
  if (CurrentMillis - PreviousMillis >= BlinkIntervalSlow) {
    if (ButtonLEDstate == LOW) {
      ButtonLEDstate = HIGH;
    } else {
      ButtonLEDstate = LOW;
    }
    digitalWrite(ShowerLED, ButtonLEDstate);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

void F_BlinkStopLED() {
  if (CurrentMillis - PreviousMillis >= BlinkIntervalSlow) {
    if (ButtonLEDstate == LOW) {
      ButtonLEDstate = HIGH;
    } else {
      ButtonLEDstate = LOW;
    }
    digitalWrite(StopLED, ButtonLEDstate);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
