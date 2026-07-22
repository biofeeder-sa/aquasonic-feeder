#include "acs/acs_sensor.h"
#include "core/vars.h"

AcsState acs = {};

void acsInit(void) {
  acs.umbral = 5.5f;
  acs.corriente_minima = 10.0f;
  acs.previousAmpMinX2 = 10000;
  acs.previousAmpMinX3 = 10000;
}

void configACS() {
  pinMode(ANALOGX3, INPUT);
}

unsigned int readACS(unsigned char pin) {
  return analogRead(pin);
}

unsigned int convertADCtoAMP(unsigned char pin) {
  float amp = 0;
  unsigned int res = 0;
  unsigned long adcLong = 0;
  unsigned long adcInt = 0;
  unsigned char counter = 100;
  for (int i = 0; i < counter; i++) {
    adcLong += readACS(pin);
  }
  adcInt = (adcLong / counter);
  float offset = 1.0;
  float cutInY = 0.0;
  float pending = 0.0;
  if (VAR_WIRE_BYTE(VAR_ACS_TYPE, 2) == 0) {
    offset = 0.0;
    cutInY = 511.5f;
    pending = 38.43f;
  } else if (VAR_WIRE_BYTE(VAR_ACS_TYPE, 2) == 1) {
    offset = 5.0;
    cutInY = 102.3f;
    pending = 38.33f;
  } else {
    offset = 0.0;
    cutInY = 509.5f;
    pending = 20.43f;
  }
  amp = (((adcInt - (cutInY - offset)) / pending) * 100);
  res = amp;
  if (res > 3000) {
    res = 1;
  }
  return res;
}

void sendAlarm(unsigned char pin) {
  bool attemp = 0;
  unsigned int amp = 0;
  unsigned char out = 0;
  unsigned int ampMax = 1000;
  uint16_t ampWarning = 0;
  amp = convertADCtoAMP(pin);
  if (pin == ANALOGX3) {
    out = X3;
    ampMax = (VAR_WIRE_BYTE(VAR_PROT_X3, 2) * 100);
    ampWarning = (VAR_WIRE_BYTE(VAR_AMP_WARN_X3, 2) << 8) | VAR_WIRE_BYTE(VAR_AMP_WARN_X3, 3);
    attemp = acs.attempX3;
    VAR_WIRE_BYTE(VAR_AMP_X3, 2) = highByte(amp);
    VAR_WIRE_BYTE(VAR_AMP_X3, 3) = lowByte(amp);
    acs.ampX3 = amp;
    if (amp > acs.previousAmpMaxX3) {
      VAR_WIRE_BYTE(VAR_AMP_MAX_X3, 2) = highByte(amp);
      VAR_WIRE_BYTE(VAR_AMP_MAX_X3, 3) = lowByte(amp);
      acs.previousAmpMaxX3 = amp;
    } else if (amp < acs.previousAmpMinX3) {
      VAR_WIRE_BYTE(VAR_AMP_MIN_X3, 2) = highByte(amp);
      VAR_WIRE_BYTE(VAR_AMP_MIN_X3, 3) = lowByte(amp);
      acs.previousAmpMinX3 = amp;
    }
    if (amp >= ampWarning) {
      Serial.println(F("Alarma de advertencia en motor X3"));
      Serial.print(amp);
      Serial.print(F(" Amp - "));
      Serial.println(ampWarning);
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 2), 1, 1);
    } else {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 2), 1, 0);
    }
  } else if (pin == ANALOGX2) {
    acs.corriente_actual = amp / 100.0;
    out = X2;
    ampMax = (VAR_WIRE_BYTE(VAR_PROT_X2, 2) * 100);
    ampWarning = (VAR_WIRE_BYTE(VAR_AMP_WARN_X2, 2) << 8) | VAR_WIRE_BYTE(VAR_AMP_WARN_X2, 3);
    attemp = acs.attempX2;
    VAR_WIRE_BYTE(VAR_AMP_X2, 2) = highByte(amp);
    VAR_WIRE_BYTE(VAR_AMP_X2, 3) = lowByte(amp);
    if (amp > acs.previousAmpMaxX2) {
      VAR_WIRE_BYTE(VAR_AMP_MAX_X2, 2) = highByte(amp);
      VAR_WIRE_BYTE(VAR_AMP_MAX_X2, 3) = lowByte(amp);
      acs.previousAmpMaxX2 = amp;
    } else if (amp < acs.previousAmpMinX2) {
      VAR_WIRE_BYTE(VAR_AMP_MIN_X2, 2) = highByte(amp);
      VAR_WIRE_BYTE(VAR_AMP_MIN_X2, 3) = lowByte(amp);
      acs.previousAmpMinX2 = amp;
    }
    if (amp >= ampWarning) {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 2), 0, 1);
    } else {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 2), 0, 0);
    }
    if ((digitalRead(X2) == ON && (millis() - app.dosageRt.sprayerStartMs >= 500)) && (digitalRead(X3) == OFF)) {
      if (acs.emptyHopperint < amp && amp < 500) {
        acs.emptyHopperint = amp + 15;
        VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 2) = highByte(acs.emptyHopperint);
        VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 3) = lowByte(acs.emptyHopperint);
        Serial.print(F("emptyHopperint: "));
        Serial.println(acs.emptyHopperint);
      }
    }
  } else {
    out = X1;
    ampMax = (VAR_WIRE_BYTE(VAR_PROT_X1, 2) * 100);
    attemp = acs.attempX1;
    VAR_WIRE_BYTE(VAR_AMP_X1, 2) = highByte(amp);
    VAR_WIRE_BYTE(VAR_AMP_X1, 3) = lowByte(amp);
  }
  if ((amp >= ampMax) && (attemp == 0)) {
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 5), 3) == 1) {
      if (pin == ANALOGX1) {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 3, 1);
      }
    }
    if (pin == ANALOGX2) {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 4, 1);
      acs.highCurrentX2 = true;
      Serial.print(F("Proteccion de Consumo en X2: "));
      Serial.println(amp);
      digitalWrite(out, OFF);
    }
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 5), 5) == 1) {
      if (pin == ANALOGX3) {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 5, 1);
        acs.highCurrentX3 = true;
        Serial.println(F("Proteccion de Consumo en X3"));
        digitalWrite(out, OFF);
      }
    }
  } else {
    if (pin == ANALOGX1) {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 3, 0);
    }
    if (pin == ANALOGX2) {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 4, 0);
    }
    if (pin == ANALOGX3) {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 5, 0);
    }
  }
  if (pin == ANALOGX1) {
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 5), 0) == 1) {
      if (amp <= VAR_WIRE_BYTE(VAR_DISCONNECTED, 2)) {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 0, 1);
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 3, 0);
      } else {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 0, 0);
      }
    }
  }
  if (pin == ANALOGX2) {
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 5), 1) == 1) {
      if (amp <= VAR_WIRE_BYTE(VAR_DISCONNECTED, 2)) {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 1, 1);
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 4, 0);
      } else {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 1, 0);
      }
    }
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 5), 6) == 1) {
      if (digitalRead(X3) == ON) {
        if (amp > acs.highestValueAmpX2) {
          acs.highestValueAmpX2 = amp;
        }
      }
      acs.emptyHopperint = ((VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 2) << 8) + VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 3));
      if ((acs.highestValueAmpX2 > (VAR_WIRE_BYTE(VAR_DISCONNECTED, 2) + 1)) && (acs.highestValueAmpX2 <= acs.emptyHopperint)) {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 6, 1);
      } else {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 6, 0);
      }
    }
  }
  if (pin == ANALOGX3) {
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 5), 2) == 1) {
      if (amp <= VAR_WIRE_BYTE(VAR_DISCONNECTED, 2)) {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 2, 1);
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 5, 0);
      } else {
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), 2, 0);
      }
    }
  }
  bitDisabledACS(0);
  bitDisabledACS(1);
  bitDisabledACS(2);
  bitDisabledACS(3);
  bitDisabledACS(4);
  bitDisabledACS(5);
  bitDisabledACS(6);
}

void sensorAmp(unsigned char output) {
  unsigned char input = 0;
  if (output == X3) {
    input = ANALOGX3;
  } else if (output == X2) {
    input = ANALOGX2;
  } else {
    input = ANALOGX1;
  }
  if (digitalRead(output) == 0) {
    if (output == X3) {
      acs.attempX3 = 0;
    } else if (output == X2) {
      acs.attempX2 = 0;
    } else {
      acs.attempX1 = 0;
    }
  } else {
    sendAlarm(input);
  }
}

void bitDisabledACS(unsigned char positionBit) {
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 5), positionBit) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5), positionBit, 0);
  }
}

float captureCurrent() {
  int numSamples = 50;
  long total = 0;
  const float sensitivity = 0.1;
  const int voltageReference = 5;
  const int adcResolution = 1023;
  const int sensorPin = A4;
  for (int i = 0; i < numSamples; i++) {
    int sensorValue = analogRead(sensorPin);
    total += sensorValue;
    delay(1);
  }
  float avgSensorValue = total / numSamples;
  float voltageOffset = avgSensorValue * (voltageReference / (float)adcResolution);
  total = 0;
  for (int i = 0; i < numSamples; i++) {
    int sensorValue = analogRead(sensorPin);
    float voltageReading = sensorValue * (voltageReference / (float)adcResolution);
    float current = (voltageReading - voltageOffset) / sensitivity;
    total += current * current;
    delay(1);
  }
  return sqrt(total / numSamples);
}

void currentPeakCounter() {
  acs.corriente_actual = convertADCtoAMP(ANALOGX2) / 100.0;
  if (acs.corriente_actual > acs.corriente_anterior) {
    acs.medidas_consecutivas_mayores++;
    acs.medidas_consecutivas_menores = 0;
    if (acs.medidas_consecutivas_mayores >= 3 && acs.flanco == false && acs.corriente_actual >= acs.umbral) {
      acs.flanco = true;
    }
  } else {
    acs.medidas_consecutivas_menores++;
    acs.medidas_consecutivas_mayores = 0;
    if (acs.medidas_consecutivas_menores >= 3 && acs.flanco == true && acs.corriente_actual <= acs.umbral) {
      acs.nPicos++;
      acs.flanco = false;
    }
  }
  if (acs.corriente_actual < acs.corriente_minima) {
    acs.corriente_minima = acs.corriente_actual;
  }
  if (acs.corriente_actual > acs.corriente_maxima) {
    acs.corriente_maxima = acs.corriente_actual;
  }
  if ((acs.corriente_maxima - acs.corriente_minima) <= 1.5) {
    acs.umbral = 5.5;
  } else {
    acs.umbral = (acs.corriente_maxima + acs.corriente_minima) / 2;
  }
  acs.corriente_anterior = acs.corriente_actual;
  if (bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3), 4) == TRUE) {
    if (acs.nPicos != acs.ultimoPicosValor) {
      acs.tiempoInicioInactividad = millis();
      acs.ultimoPicosValor = acs.nPicos;
    }
    if (millis() - acs.tiempoInicioInactividad >= TIEMPO_ESPERA_INACTIVIDAD) {
      acs.inactividadFlag = true;
    } else {
      acs.inactividadFlag = false;
    }
  }
  if (acs.nPicos != acs.nPicosAnterior) {
    acs.tiempoActualVueltas = millis();
    acs.tiempoEntreVueltas = acs.tiempoActualVueltas - acs.tiempoAnteriorVueltas;
    acs.tiempoAnteriorVueltas = acs.tiempoActualVueltas;
    acs.nPicosAnterior = acs.nPicos;
  }
}
