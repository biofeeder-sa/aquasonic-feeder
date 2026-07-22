#ifndef AQUASONIC_ACS_SENSOR_H_
#define AQUASONIC_ACS_SENSOR_H_

#include <Arduino.h>

#define ANALOGX3 A5
#define ANALOGX2 A4
#define ANALOGX1 A3
#define TIEMPO_ESPERA_INACTIVIDAD 10000

typedef struct {
  bool attempX3;
  bool attempX2;
  bool attempX1;
  bool highCurrentX2;
  bool highCurrentX3;
  float corriente_actual;
  bool enPico;
  float umbral;
  int medidas_consecutivas_menores;
  int medidas_consecutivas_mayores;
  float corriente_maxima;
  float corriente_minima;
  float corriente_anterior;
  bool flanco;
  unsigned int medidas_consecutivas;
  unsigned int nPicos;
  unsigned int nPicosAnterior;
  unsigned long ultimoPicoTiempo;
  unsigned long tiempoAnteriorVueltas;
  unsigned long tiempoActualVueltas;
  unsigned long tiempoEntreVueltas;
  unsigned int ultimoPicosValor;
  bool inactividadFlag;
  unsigned long tiempoInicioInactividad;
  unsigned int previousAmpMaxX2;
  unsigned int previousAmpMinX2;
  unsigned int previousAmpMaxX3;
  unsigned int previousAmpMinX3;
  unsigned int ampX3;
  unsigned int highestValueAmpX2;
  unsigned int emptyHopperint;
} AcsState;

extern AcsState acs;

void acsInit(void);
void configACS();
void sendAlarm(unsigned char pin);
void sensorAmp(unsigned char output);
unsigned int readACS(unsigned char pin);
void bitDisabledACS(unsigned char positionBit);
unsigned int convertADCtoAMP(unsigned char pin);
void currentPeakCounter();
float captureCurrent();

#endif
