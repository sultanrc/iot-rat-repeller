uint8_t PIR_PIN = 13;        
uint8_t SIGNAL_PIN = 14;      
uint8_t BUZZER_PIN = 2;    

uint32_t buzzer_freq = 2000;    
uint32_t buzzer_duration = 1000; 
uint32_t buzzer_start_time = 0; 
bool buzzer_active = false;       

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(SIGNAL_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  ledcSetup(0, buzzer_freq, 8); // Saluran 0, frekuensi 500 Hz, resolusi 8-bit
  ledcAttachPin(BUZZER_PIN, 0); // Lampirkan pin buzzer ke saluran 0
}

void loop() {
  uint32_t current_time = millis();

  if (digitalRead(PIR_PIN) == HIGH) {
    if (!buzzer_active) {
      buzzer_active = true;
      buzzer_start_time = current_time; 
      digitalWrite(SIGNAL_PIN, HIGH);
      ledcWriteTone(0, buzzer_freq); 
    }
  }

  if (buzzer_active && (current_time - buzzer_start_time >= buzzer_duration)) {
    buzzer_active = false;
    digitalWrite(SIGNAL_PIN, LOW); 
    ledcWriteTone(0, 0); 
  }

  if (digitalRead(PIR_PIN) == LOW && !buzzer_active) {
    digitalWrite(SIGNAL_PIN, LOW);
    ledcWriteTone(0, 0);
  }
}