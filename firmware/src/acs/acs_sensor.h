#ifndef AQUASONIC_ACS_SENSOR_H_
#define AQUASONIC_ACS_SENSOR_H_

#include <Arduino.h>

#define ANALOGX3 A5
#define ANALOGX2 A4
#define ANALOGX1 A3
#define TIEMPO_ESPERA_INACTIVIDAD 10000

/* Motores invertidos: byte lógico 0 (índice 5), bit 7. Unidades: centiamperios. */
#define MOTOR_SWAP_ALARM_BYTE      5
#define MOTOR_SWAP_ALARM_BIT       7
#define MOTOR_SWAP_SPRAYER_MIN_CA  400u  /* aspersor ≥ 4.0 A */
#define MOTOR_SWAP_DOSER_MAX_CA    200u  /* dosificador ≤ 2.0 A */

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
  bool hadX2DisconnectedInCycle;
  bool hadX3DisconnectedInCycle;
  bool hadEmptyHopperInCycle;
} AcsState;

extern AcsState acs;

void acsInit(void);
void acsResetWearCycleFlags(void);
void configACS();
void sendAlarm(unsigned char pin);
void sensorAmp(unsigned char output);
unsigned int readACS(unsigned char pin);
void bitDisabledACS(unsigned char positionBit);
bool acsEvaluateMotorSwap(uint16_t imaxX2, uint16_t imaxX3);
unsigned int convertADCtoAMP(unsigned char pin);
void currentPeakCounter();
float captureCurrent();

#endif
