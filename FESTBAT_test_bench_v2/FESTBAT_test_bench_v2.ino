const int SHEET_NUM = 3;  // Anzhal der Sheets auf dem Aufnahmestapel

// Interrupt-Konfiguration
const unsigned long DEBOUNCE_DELAY = 200;     // Zeit in Millisekunden für die Entprellung
volatile bool userInterrupted = false;        // Globales Flag für Unterbrechung (interruptFlag)
volatile unsigned long lastDebounceTime = 0;  // Zeitstempel für die letzte Zustandsänderung für Entprellen

// Hauptsteuerungsflag
bool controller = false;

int SMT10_Capture, SMT10_Discard, Capture_Button, Discard_Button, Endpoint_Button;
int Left, Right, Up, Down;

int toggleState = LOW;

void moveDownToCaptureStack();
void moveDownToDiscardStack();
void moveToAboveCaptureStack();
void moveToAboveDiscardStack();
void readButtons();
void readControlButtons();
void stopMovement();
void userInterrupt();

void setup() {

  // Magnetventile-------------------------------------------------------
  // VUVS
  pinMode(5, OUTPUT);  // Vakuum
  // VUVG
  pinMode(6, OUTPUT);  // Schwenk
  pinMode(7, OUTPUT);  // Schwenk
  pinMode(8, OUTPUT);  // Hub
  pinMode(9, OUTPUT);  // Hub

  // Taster--------------------------------------------------------------
  Serial.begin(9600);
  // Näherungsschalter
  pinMode(22, INPUT);
  pinMode(24, INPUT);
  // Platform-Taster
  pinMode(32, INPUT);
  pinMode(33, INPUT);
  pinMode(34, INPUT);
  // Controller
  pinMode(46, INPUT);
  pinMode(47, INPUT);
  pinMode(48, INPUT);
  pinMode(49, INPUT);

  // Interrupt Pin
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), userInterrupt, CHANGE);
}

void loop() {

  readButtons();

  // Schwenk-Hub Modul mitels 4 Tastern (Up,Down,Left,Right) zu steuern.
  if (controller) {

    readControlButtons();

    // Vakuum muss noch hinzugefügt werden >> momentan zu wenig Taster
    if (Left == HIGH) {
      Serial.println(F("Left"));
      digitalWrite(6, HIGH);
    } else if (Right == HIGH) {
      Serial.println(F("Right"));
      digitalWrite(7, HIGH);
    } else if (Up == HIGH) {
      Serial.println(F("Up"));
      digitalWrite(8, HIGH);
    } else if (Down == HIGH) {
      Serial.println(F("Down"));
      digitalWrite(9, HIGH);
    } else {
      stopMovement();
    }
  }

  // Eingabe über den Seriellen Monitor
  // Mögliche Eingabe:  c -- Aktiviert den Controller (Steuerung des Linear-Schwenkmoduls über 4 Taster) >> Später Knopf auf Controller
  //                    a -- Aktiviert die Automatische Sequenz >> Später Knopf auf Controller
  //                    v -- Toggelt das Vakuum
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'v':
        Serial.println("vacuum");
        toggleState = !toggleState;
        digitalWrite(5, toggleState);
        break;
      case 'c':
        Serial.println(F("controller"));

        (userInterrupted) ? userInterrupted = false : 0;  // Wenn abgebrochen wurde durch interrupt, dann muss erst flag wieder deaktiviert werden

        controller = !controller;
        break;
      case 'a':
        Serial.println(F("automatic"));

        (userInterrupted) ? userInterrupted = false : 0;  // Wenn abgebrochen wurde durch interrupt, dann muss erst flag wieder deaktiviert werden

        for (int i = 0; i < SHEET_NUM; i++) {
          moveToAboveCaptureStack();
          moveDownToCaptureStack();
          digitalWrite(5, HIGH); // Delay & Vakuum aktivieren
          moveToAboveDiscardStack();
          moveDownToDiscardStack();
          digitalWrite(5, LOW); // Delay & Vakuum deaktivieren
        }
        moveToAboveCaptureStack();
        break;
    }
  }
}

void moveDownToCaptureStack() {
  bool goalPos = false;
  int state = 0;

  readButtons();
  if (SMT10_Capture != HIGH) {
    moveToAboveCaptureStack();
  }

  while (!goalPos && !userInterrupted) {

    readButtons();
    if (Capture_Button == HIGH && state == 0) {
      state = 1;
    }

    switch (state) {
      case 0:
        digitalWrite(7, LOW);
        digitalWrite(9, HIGH);
        break;
      case 1:
        digitalWrite(9, LOW);
        goalPos = true;
        break;
    }
  }

  if (userInterrupted) {
    stopMovement();
  }
}

void moveDownToDiscardStack() {
  bool goalPos = false;
  int state = 0;

  readButtons();
  if (SMT10_Discard != HIGH) {
    moveToAboveDiscardStack();
  }

  while (!goalPos && !userInterrupted) {

    readButtons();

    if (Discard_Button == HIGH && state == 0) {
      state = 1;
    }

    // Ausführen der Bewegungen des Linear-Schwenk-Moduls bei gewissem Zustand X
    switch (state) {
      case 0:
        digitalWrite(6, LOW);
        digitalWrite(9, HIGH);
        break;
      case 1:
        digitalWrite(9, LOW);
        goalPos = true;
        break;
    }
  }

  if (userInterrupted) {
    stopMovement();
  }
}

void moveToAboveCaptureStack() {
  bool goalPos = false;
  int state = 0;

  while (!goalPos && !userInterrupted) {

    readButtons();
    if (Endpoint_Button == HIGH && state == 0) {
      state = 1;
    } else if (Endpoint_Button == LOW && state == 1) {
      state = 2;
    } else if (SMT10_Capture == HIGH && state == 2) {
      state = 3;
    }
    switch (state) {
      case 0:
        digitalWrite(8, HIGH);
        break;
      case 1:
        digitalWrite(8, LOW);
        digitalWrite(9, HIGH);
        delay(50);
        break;
      case 2:
        digitalWrite(9, LOW);
        digitalWrite(7, HIGH);
        break;
      case 3:
        digitalWrite(7, LOW);
        goalPos = true;
        break;
    }
  }

  if (userInterrupted) {
    stopMovement();
  }
}

void moveToAboveDiscardStack() {
  bool goalPos = false;
  int state = 0;

  while (!goalPos && !userInterrupted) {

    readButtons();
    if (Endpoint_Button == HIGH && state == 0) {
      state = 1;
    } else if (Endpoint_Button == LOW && state == 1) {
      state = 2;
    } else if (SMT10_Discard == HIGH && state == 2) {
      state = 3;
    }

    switch (state) {
      case 0:
        digitalWrite(8, HIGH);
        break;
      case 1:
        digitalWrite(8, LOW);
        digitalWrite(9, HIGH);
        delay(50);
        break;
      case 2:
        digitalWrite(9, LOW);
        digitalWrite(6, HIGH);
        break;
      case 3:
        digitalWrite(6, LOW);
        goalPos = true;
        break;
    }
  }

  if (userInterrupted) {
    stopMovement();
  }
}

void readButtons() {
  SMT10_Capture = digitalRead(22);  // Näherungstaster Aufnahmestapel
  SMT10_Discard = digitalRead(24);  // Näherungstaster Ablagestapel

  Capture_Button = digitalRead(32);   // Taster unter Aufnahmeplattform
  Discard_Button = digitalRead(33);   // Taster unter Ablageplattform
  Endpoint_Button = digitalRead(34);  // Taster an Ursprung des Greiferarms
}

void readControlButtons() {
  Left = digitalRead(46);
  Right = digitalRead(47);
  Up = digitalRead(48);
  Down = digitalRead(49);
}

void stopMovement() {
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
}

void userInterrupt() {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime > DEBOUNCE_DELAY && digitalRead(2) == HIGH) {
    userInterrupted = true;  // Ändere den Zustand des Interrupts, wenn die Entprellzeit vergangen ist
    Serial.println("Interrupt!");
  }
  lastDebounceTime = currentTime;  // Aktualisiere den Zeitstempel für die letzte Zustandsänderung
}




//  Automatische Fahrt
/*  if (automatic) {

  // Festlegen des Zustands während des Automatisierungsprozesses
   if (Endpoint_Button == HIGH && state == 0) {
      state = 1;
    } else if (Endpoint_Button == LOW && state == 1) {
      state = 2;
    } else if (SMT10_Capture == HIGH && state == 2) {
      state = 3;
    } else if (Capture_Button == HIGH && state == 3) {
      state = 4;
    } else if (Endpoint_Button == HIGH && state == 4) {
      state = 5;
    } else if (Endpoint_Button == LOW && state == 5) {
      state = 6;
    } else if (SMT10_Discard == HIGH && state == 6) {
      state = 7;
    } else if (Discard_Button == HIGH && state == 7) {
      state = 8;
    }

    // Ausführen der Bewegungen des Linear-Schwenk-Moduls bei gewissem Zustand X
    switch (state) {
      case 0:
        digitalWrite(8, HIGH);
        break;
      case 1:
        digitalWrite(8, LOW);
        digitalWrite(9, HIGH);
        delay(50);
        break;
      case 2:
        digitalWrite(9, LOW);
        digitalWrite(7, HIGH);
        break;
      case 3:
        digitalWrite(7, LOW);
        digitalWrite(9, HIGH);
        break;
      case 4:
        digitalWrite(9, LOW);
        // Vakuum aktivieren
        delay(200);
        digitalWrite(8, HIGH);
        break;
      case 5:
        digitalWrite(8, LOW);
        digitalWrite(9, HIGH);
        delay(50);
        break;
      case 6:
        digitalWrite(9, LOW);
        digitalWrite(6, HIGH);
        break;
      case 7:
        digitalWrite(6, LOW);
        digitalWrite(9, HIGH);
        break;
      case 8:
        digitalWrite(9, LOW);
        // Vakuum deaktivieren
        delay(200);
        state = 0;
        round_Counter++;
        break;
    }

    // Abschlusssequenz
    if (round_Counter == SHEET_NUM) {
      digitalWrite(8, HIGH);
      delay(1200);  // Später ändern durch Tasteranschlag oben, anstatt delay, ODER an Initialposition
      digitalWrite(8, LOW);
    }
  }
  */