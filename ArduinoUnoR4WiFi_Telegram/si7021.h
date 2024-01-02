// Inclusione della libreria per il sensore Si7021
#include "Adafruit_Si7021.h"

// Creazione di un oggetto Si7021
Adafruit_Si7021 sensor = Adafruit_Si7021();

// Variabile per indicare la presenza del sensore di temperatura e umidità
bool hasTHSensor = false;

// Inizializzazione del sensore Si7021
void initSi7120() {
  Serial.println("Si7021 test!");

  // Verifica se il sensore Si7021 è presente
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    return;
  } else {
    hasTHSensor = true;
  }

  // Stampa del modello del sensore e della revisione
  Serial.print("Found model ");
  switch (sensor.getModel()) {
    case SI_Engineering_Samples:
      Serial.print("SI engineering samples"); break;
    case SI_7013:
      Serial.print("Si7013"); break;
    case SI_7020:
      Serial.print("Si7020"); break;
    case SI_7021:
      Serial.print("Si7021"); break;
    case SI_UNKNOWN:
    default:
      Serial.print("Unknown");
  }
  Serial.print(" Rev(");
  Serial.print(sensor.getRevision());
  Serial.print(")");
  Serial.print(" Serial #"); Serial.print(sensor.sernum_a, HEX); Serial.println(sensor.sernum_b, HEX);
}

// Lettura della temperatura dal sensore Si7021
double readTemperature() {
  return sensor.readTemperature();
}

// Lettura dell'umidità dal sensore Si7021
double readHumidity() {
  return sensor.readHumidity();
}
