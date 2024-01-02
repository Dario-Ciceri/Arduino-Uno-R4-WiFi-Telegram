// Definizione del pin per il sensore PIR
#define PIR_PIN 8

// Inizializzazione del sensore PIR
void initPIR() {
  pinMode(PIR_PIN, INPUT);
}

// Lettura dello stato del sensore PIR
bool readPIR() {
  return digitalRead(PIR_PIN);
}
