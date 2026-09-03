#include <Arduino.h>
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "core/vars.h"
#include "comm/xbee_parser.h"
#include "variables/var_io.h"
void create_frame(unsigned char frame[], unsigned char data[], unsigned char data_size, unsigned char frame_id) {
    frame[0] = STARTDELIMITER; // Start delimiter
    frame[1] = highByte(data_size + 14); // Length MSB
    frame[2] = lowByte(data_size + 14); // Length LSB
    frame[3] = CMD_TRANSMIT_FRAME; // Transmit frame
    frame[4] = frame_id; // Frame Id
    // Destination address 64 bits (transferServer)
    frame[5] = xbee.replyAddr[0];
    frame[6] = xbee.replyAddr[1];
    frame[7] = xbee.replyAddr[2];
    frame[8] = xbee.replyAddr[3];
    frame[9] = xbee.replyAddr[4];
    frame[10] = xbee.replyAddr[5];
    frame[11] = xbee.replyAddr[6];
    frame[12] = xbee.replyAddr[7];
    // Destination address 16 bit
    frame[13] = 0xff;
    frame[14] = 0xfe;
    frame[15] = 0x00; // Broad cast radius
    frame[16] = 0x00; // Options
  // Data
  for (int i = 0; i < data_size; i++) {
    frame[17 + i] = data[i];
    }
    frame[17 + data_size] = 0; // Checksum
    set_checksum(frame, data_size + FRAME_HEADER);
}
void set_checksum(unsigned char frame[], int size) {
  frame[size - 1] = checksum(frame, size);
}
/**
  Descrip.
   Calcula el checksum y coloca su valor al final de la trama creada
   @param frame: array of bytes with the packet frame (this should include a last dummy byte to set calculate checksum)
   @param size: size of the frame
   -return el valor de checksum calculado
*/
unsigned char checksum(unsigned char frame[], int size) {
  unsigned char checksum = 0;
  for (int i = 3; i < size - 1; i++) {
    checksum += frame[i];
  }
   //Serial.print(F("checksum: "));
   //Serial.println(checksum, HEX);
  return (unsigned char) 0xff - checksum;
}
/**
  **Solo para debug**
  Descrip.
   Imprime la trama creada
   @param header: apuntador del mensaje antes de la trama
   @param frame: array of bytes with the packet frame (this should include a last dummy byte to set calculate checksum)
   @param size: tamano del contenido de la trama (sin cabecera)
   @param footer: apuntador del mensaje despues de la trama
   -return void
*/
void print_frame(const char *header,unsigned char frame[], unsigned char data_size, const char *footer) {
  unsigned char total = data_size + (frame[4] == 0 ? FRAME_HEADER : FRAME_HEADER_BROADCAST);
  Serial.print(header);
  for (int i = 0; i < total; i = i + 1) {
    if (frame[i] < 16) {
      Serial.print(F(" 0"));
    } else {
      Serial.print(F(" "));
    }
    Serial.print(frame[i], HEX);
  }
  Serial.println(footer);
}
/*
  Descrip.
  Envia la trama creada
   @param frame: array of bytes with the packet frame (this should include a last dummy byte to set calculate checksum)
   @param size: tamano del contenido de la trama (sin cabecera)
   -return void
*/
void send_frame(unsigned char frame[], int size) {
  unsigned char total = size + (frame[4] == 0 ? FRAME_HEADER : FRAME_HEADER_BROADCAST);
  //TODO: AGREGAR FUNCION Serial.availableForWrite() PARA CHEQUEAR CANTIDAD DE CARACTERES DISPONIBLES PARA ESCRIBIR EN EL BUFER
  /*uint8_t availableBytes = XBee.availableForWrite();
  if (availableBytes <= data_size)
  {
    //TODO: si la cantidad de caracteres disponibles en el buffer es menor a la cantidad por transmitir, llamar a la funciÃ³n .flush()
    Serial.print("PREV_FLUSH: ");
    Serial.print(micros());
    Serial.println();
    XBee.flush();
    Serial.print("POST_FLUSH: ");
    Serial.print(micros());
    Serial.println();
  }*/
  Serial3.flush();
  for (int i = 0; i < total; i = i + 1) {
    Serial3.write(frame[i]);
  }
  /*Serial.print("data_size: ");
  Serial.print(data_size);
  Serial.println();
  Serial.print("availableBytes: ");
  Serial.print(availableBytes,HEX);
  Serial.println();*/
}
/**
  **Solo para debug**
  Descrip.
    Se completa con 0 los valores que van de [0 - 9]
    -return void
*/
void printDigits(int digits) {
  // Serial.print(F(":"));
  // if (digits < 10)
  //   Serial.print(F("0"));
  // Serial.print(digits);
  Serial.print(0);
}
  /*
  Descrip.
    Crea trama broadcast
    -return void
  */
void create_init_broadcast(unsigned char frame[], unsigned char data[], unsigned char data_size) {
  // TODO: use create_frame
  // Send vars
  // frame size = bf command (1) + bf msg id (2) + var amount (1) + sum of vars (var code (2) + var size (*))
  frame[0] = STARTDELIMITER; // Start delimiter
  frame[1] = highByte(data_size + 18); // Length MSB
  frame[2] = lowByte(data_size + 18); // Length LSB
  frame[3] = CMD_TRANSMIT_FRAME; // Transmit frame
  frame[4] = 0x01; // Frame Id
  // Destination address 64 bits (transferServer)
  frame[5] = 0;
  frame[6] = 0;
  frame[7] = 0;
  frame[8] = 0;
  frame[9] = 0;
  frame[10] = 0;
  frame[11] = 0xFF;
  frame[12] = 0xFF;
  // Destination address 16 bit
  frame[13] = 0xff;
  frame[14] = 0xfe;
  frame[15] = 0x00; // Broad cast radius
  frame[16] = 0x00; // Options
  // Biofeeder frame header
  frame[17] = CMD_INIT; // Init frame command
  frame[18] = 0xBB; // Msg Id MSB
  frame[19] = 0xBB; // Msg Id LSB
  frame[20] = 0x04; // Amount of var to send TODO: Calculate this dynamically
  // Data
  for (int i = 0; i < data_size; i++) {
    frame[21 + i] = data[i];
  }
  frame[21 + data_size] = 0; // Checksum
  set_checksum(frame, data_size + FRAME_HEADER_BROADCAST);
}
  /*
  Descrip.
    Crea data para la trama de broadcast
    -return void
  */
void create_data_broadcast(){
  int cont = 0;
  appendVarWire(VAR_SOFTWARE_VERSION, xbee.broadcast, cont);
  appendVarWire(VAR_HARDWARE_VERSION, xbee.broadcast, cont);
  appendVarWire(VAR_PROJECT_NAME, xbee.broadcast, cont);
  for (int j = 0; j < 3; ++j) {
    xbee.broadcast[cont] = VAR_WIRE_BYTE(VAR_PROFILE, j);
    cont++;
  }
  cont--;
}
  /*
  Descrip.
    Es un conjunto de funcion que se agrupa para
    - Crear data para la trama del broadcast
    - Crea la trama para el broadcast
    - Imprime la trama del broadcast
    - Envia la trama del broadcast
    -return void
  */
void broadcast (){
  create_data_broadcast();
  xbee.payloadSize    = (sizeof(xbee.broadcast)/sizeof(xbee.broadcast[0]));
  //Serial.println((xbee.payloadSize));
  create_init_broadcast(xbee.txFrame, xbee.broadcast, xbee.payloadSize);
  print_frame("Send broadcast : <",xbee.txFrame, xbee.payloadSize," >");
  send_frame(xbee.txFrame, xbee.payloadSize);
}
static void xbeeLogRxFrame(void) {
  Serial.print(F("RX XBee ("));
  Serial.print(xbee.rxIndex);
  Serial.print(F("): <"));
  for (uint8_t i = 0; i < xbee.rxIndex; i++) {
    if (xbee.txFrame[i] < 0x10) Serial.print(F("0"));
    Serial.print(xbee.txFrame[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println(F(">"));
}
static void xbeeHandleBiofeederPayload(void) {
  if (xbee.rxIndex <= 15) {
    return;
  }
  for (uint8_t i = 0; i <= 7; i++) {
    xbee.replyAddr[i] = xbee.txFrame[4 + i];
  }
  if (xbee.txFrame[15] == CMD_WRITE_REQUEST) {
    unsigned char countByte = 0;
    unsigned char posLSB = 19;
    unsigned char posMSB = 20;
    unsigned char mult = 0;
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 4), 3) == 1) {
      xbee.comm.lastActivityMs = millis();
    }
    xbee.writeResponse[0] = CMD_WRITE_RESPONSE;
    xbee.writeResponse[1] = xbee.txFrame[16];
    xbee.writeResponse[2] = xbee.txFrame[17];
    xbee.writeResponse[3] = xbee.txFrame[18];
    for (int i = 0; i < xbee.txFrame[18]; ++i) {
      if (i != 0) {
        mult = 1;
      }
      posLSB = (posLSB + ((mult * 2) + countByte));
      posMSB = (posMSB + ((mult * 2) + countByte));
      int row = findRowById(xbee.txFrame[posLSB], xbee.txFrame[posMSB]);
      if (row >= 0) {
        uint8_t written = 0;
        if (applyVarWriteFromXbee(row, posLSB, xbee.txFrame, written)) {
          countByte = written;
        }
      }
    }
    xbee.payloadSize = sizeof(xbee.writeResponse) / sizeof(xbee.writeResponse[0]);
    create_frame(xbee.txFrame, xbee.writeResponse, xbee.payloadSize, 0);
    print_frame("Write var : <", xbee.txFrame, xbee.payloadSize, " >");
    send_frame(xbee.txFrame, xbee.payloadSize);
    app.comm.answerBroadcast = TRUE;
  }
  else if (xbee.txFrame[15] == CMD_READ_REQUEST) {
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 4), 3) == 1) {
      xbee.comm.lastActivityMs = millis();
    }
    int counter = 4;
    xbee.readRequest[0] = CMD_READ_RESPONSE;
    xbee.readRequest[1] = xbee.txFrame[16];
    xbee.readRequest[2] = xbee.txFrame[17];
    xbee.readRequest[3] = xbee.txFrame[18];
    for (int i = 0; i < xbee.txFrame[18]; ++i) {
      int row = findRowById(xbee.txFrame[19 + (i * 2)], xbee.txFrame[20 + (i * 2)]);
      if (row >= 0) {
        appendVarWire(row, xbee.readRequest, counter);
      }
    }
    xbee.payloadSize = counter;
    create_frame(xbee.txFrame, xbee.readRequest, xbee.payloadSize, 0);
    print_frame("Read var : <", xbee.txFrame, xbee.payloadSize, " >");
    send_frame(xbee.txFrame, xbee.payloadSize);
    app.comm.answerBroadcast = TRUE;
  }
  else if (xbee.txFrame[15] == CMD_ACK) {
    Serial.println(F("ACK recibido"));
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 4), 3) == 1) {
      xbee.comm.lastActivityMs = millis();
    }
    acs.inactividadFlag = false;
    xbee.writeResponse[0] = CMD_WRITE_RESPONSE;
    xbee.writeResponse[1] = xbee.txFrame[16];
    xbee.writeResponse[2] = xbee.txFrame[17];
    xbee.writeResponse[3] = xbee.txFrame[18];
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3), 3) == 1) {
      if (Events_ArduinoUNO) {
        varSetCycleGrams(0);
        VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 2) = 0;
        VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 3) = 0;
      }
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3), 3, 0);
    }
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4), 3) == 1) {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 4), 3, 0);
    }
    app.comm.deliveryAck = TRUE;
  }
  else if (xbee.txFrame[15] == CMD_ERROR) {
    xbee.stats.lastAppError = (xbee.rxIndex > 16) ? xbee.txFrame[16] : 0;
    xbee.stats.txDeliveryErrors++;
    Serial.print(F("XBee app error: 0x"));
    Serial.println(xbee.stats.lastAppError, HEX);
  }
  else if (xbee.txFrame[15] == CMD_MOTOR_CHANGE_CONFIRM) {
    if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 4), 3) == 1) {
      xbee.comm.lastActivityMs = millis();
    }
    bool ok = false;
    if (xbee.rxIndex >= 22 &&
        xbee.txFrame[20] == 0x00 &&
        xbee.txFrame[21] == 0x00) {
      ok = motorWearHandleChangeConfirm(xbee.txFrame[18], xbee.txFrame[19]);
    }
    xbee.writeResponse[0] = CMD_WRITE_RESPONSE;
    xbee.writeResponse[1] = xbee.txFrame[16];
    xbee.writeResponse[2] = xbee.txFrame[17];
    xbee.writeResponse[3] = ok ? 0x00 : 0x01;
    xbee.payloadSize = 4;
    create_frame(xbee.txFrame, xbee.writeResponse, xbee.payloadSize, 0);
    print_frame("Motor change confirm : <", xbee.txFrame, xbee.payloadSize, " >");
    send_frame(xbee.txFrame, xbee.payloadSize);
    app.comm.answerBroadcast = TRUE;
  }
  else if (xbee.txFrame[15] == CMD_CALIBRATION) {
    app.dosageRt.calibGrams = (xbee.txFrame[18] << 8) + xbee.txFrame[19];
  }
}
static void xbeeHandleTxStatus(void) {
  if (xbee.rxIndex < 9) {
    return;
  }
  xbee.stats.lastTxDeliveryStatus = xbee.txFrame[8];
  if (xbee.txFrame[8] != 0x00) {
    xbee.stats.txDeliveryErrors++;
    Serial.print(F("XBee TX status error: 0x"));
    Serial.println(xbee.txFrame[8], HEX);
  }
}
void xbeeProcessApiFrame(void) {
  xbeeLogRxFrame();
  if (xbee.txFrame[3] == CMD_RECEIVE_PACKET) {
    Serial.print(F("API 0x90 payload cmd: 0x"));
    if (xbee.rxIndex > 15) {
      Serial.println(xbee.txFrame[15], HEX);
    } else {
      Serial.println(F("(trama corta)"));
    }
    xbeeHandleBiofeederPayload();
  }
  else if (xbee.txFrame[3] == CMD_API_TX_STATUS) {
    xbeeHandleTxStatus();
  }
  else if (xbee.txFrame[3] == CMD_API_MODEM_STATUS) {
    Serial.print(F("XBee modem status: 0x"));
    if (xbee.rxIndex > 4) {
      Serial.println(xbee.txFrame[4], HEX);
    } else {
      Serial.println(F("?"));
    }
  }
  else {
    Serial.print(F("API frame type: 0x"));
    Serial.println(xbee.txFrame[3], HEX);
  }
}
void xbee_communication(void) {
  xbeeParserPoll();
}
void create_init_Alarms(unsigned char frame[], unsigned char data[], unsigned char data_size)
{
  // TODO: use create_frame
  // Send vars
  // frame size = bf command (1) + bf msg id (2) + var amount (1) + sum of vars (var code (2) + var size (*))
  frame[0] = STARTDELIMITER; // Start delimiter
  frame[1] = highByte(data_size + 18); // Length MSB
  frame[2] = lowByte(data_size + 18); // Length LSB
  frame[3] = CMD_TRANSMIT_FRAME; // Transmit frame
  frame[4] = 0x01; // Frame Id
  // Destination address 64 bits (transferServer)
  frame[5] = VAR_WIRE_BYTE(VAR_SERVER, 2);
  frame[6] = VAR_WIRE_BYTE(VAR_SERVER, 3);
  frame[7] = VAR_WIRE_BYTE(VAR_SERVER, 4);
  frame[8] = VAR_WIRE_BYTE(VAR_SERVER, 5);
  frame[9] = VAR_WIRE_BYTE(VAR_SERVER, 6);
  frame[10] = VAR_WIRE_BYTE(VAR_SERVER, 7);
  frame[11] = VAR_WIRE_BYTE(VAR_SERVER, 8);
  frame[12] = VAR_WIRE_BYTE(VAR_SERVER, 9);
  // Destination address 16 bit
  frame[13] = 0xff;
  frame[14] = 0xfe;
  frame[15] = 0x00; // Broad cast radius
  frame[16] = 0x00; // Options
  // Biofeeder frame header
  frame[17] = CMD_INIT_ALARM; // Init frame command
  frame[18] = 0xAA; // Msg Id MSB
  frame[19] = 0xAA; // Msg Id LSB
  frame[20] = xbee.alarmsVarCount; // Amount of var to send TODO: Calculate this dynamically
  // Data
  for (int i = 0; i < data_size; i++) {
    frame[21 + i] = data[i];
  }
  frame[21 + data_size] = 0; // Checksum
  set_checksum(frame, data_size + FRAME_HEADER_ALARMS);
}
void create_data_Alarms(void)
{
  int cont = 0;
  if (VAR_WIRE_BYTE(VAR_ALARMS, 2) != 0 || VAR_WIRE_BYTE(VAR_ALARMS, 3) != 0 ||
      VAR_WIRE_BYTE(VAR_ALARMS, 4) != 0 || VAR_WIRE_BYTE(VAR_ALARMS, 5) != 0)
  {
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_ALARMS)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_ALARMS, i);
      /*Serial.print("xbee.alarms[");
      Serial.print(cont);
      Serial.print("]: ");
      Serial.print(xbee.alarms[cont] , HEX);      
      Serial.println();*/
      cont++;
    }
    xbee.alarmsVarCount++;  
    /* Cantidad de gramos acumulados en el dia */
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_ACCUMULATED_GRAMS)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_ACCUMULATED_GRAMS, i);
      cont++;
    }
    xbee.alarmsVarCount++;   
  }
  /*    SI LA ALARMA FUE POR GRAMOS DOSIFICADOS   */
  if(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3) == 1)
  {
    if  (
          // ( bitRead( VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 3 ) == FALSE  )  && 
          // ( 
              // Desconectado X2
            ( bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 5) , 1 ) == 1 ) ||
            // Consumo excesivo en X2
            ( bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 5) , 4 ) == 1 ) ||
            // No hay balanceado en la tolva
            ( bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 5) , 6 ) == 1 ) ||
            // Tiempo de inactividad gr/vuelta
            (acs.inactividadFlag == true)
          // )
        ) {
          Serial.println(bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 4 ));
          if((bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 4 ) == TRUE) && (VAR_WIRE_BYTE(VAR_N_VUELTAS, 2) != 0)){
            int gr_dosificados = VAR_WIRE_BYTE(VAR_N_VUELTAS, 2) * (int)varFeedRateGramsPerTurn();
            VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 2) = (byte)(gr_dosificados >> 8);
            VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 3) = (byte)(gr_dosificados & 0xFF);
          }else{
            VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 2) = 0;
            VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 3) = 0;
          }
    }
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_DOSED_GRAMS)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_DOSED_GRAMS, i);
      /*Serial.print("xbee.alarms[");
      Serial.print(cont);
      Serial.print("]: ");
      Serial.print(xbee.alarms[cont] , HEX);      
      Serial.println();*/
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_ID_DOSAGE)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_ID_DOSAGE, i);
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_N_VUELTAS)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_N_VUELTAS, i);
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_T_VUELTAS)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_T_VUELTAS, i);
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_AMP_X2)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_AMP_X2, i);
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_AMP_X3)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_AMP_X3, i);
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < 6; ++i)
    {
      xbee.alarms[cont] = xbee.battBeforeFeed[i];
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < 6; ++i)
    {
      xbee.alarms[cont] = xbee.battAfterFeed[i];
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_AMP_MAX_X2)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_AMP_MAX_X2, i);
      cont++;
    }
    xbee.alarmsVarCount++;
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_AMP_MAX_X3)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_AMP_MAX_X3, i);
      cont++;
    }
    xbee.alarmsVarCount++;
    // Si esta habilitado el bit para pesar y si esta calibrado y esta conectado las celdas de carga
    // if( (bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 3) == TRUE) && (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 0 ) == 0) && 
    //     (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 1 ) == 0) ) {
    //     for (int i = 0; i < ((VAR_WIRE_LEN(VAR_WEIGHT)) ); ++i)
    //     {
    //       xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_WEIGHT, i);
    //       cont++;
    //     }
    //     xbee.alarmsVarCount++;
    //     for (int i = 0; i < ((VAR_WIRE_LEN(VAR_CYCLE_GRAMS)) ); ++i)
    //     {
    //       xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_CYCLE_GRAMS, i);
    //       cont++;
    //     }
    //     xbee.alarmsVarCount++;
    // }
  }
  /*    SI LA ALARMA FUE POR VOLTAJE DE BATERIA   */
  if ( (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 4) == 1) )
  {
    for (int i = 0; i < ((VAR_WIRE_LEN(VAR_BATTERY)) ); ++i)
    {
      xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_BATTERY, i);
      /*Serial.print("xbee.alarms[");
      Serial.print(cont);
      Serial.print("]: ");
      Serial.print(xbee.alarms[cont] , HEX);      
      Serial.println();*/
      cont++;
    }
    xbee.alarmsVarCount++;
  }
  /*    SI LA ALARMA FUE POR PESO   */
  // if ( ( bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 0 ) != bitRead(xbee.comm.alarmsSnapshot [2] , 0 ) ) || 
  //      ( bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 0 ) == 1 )
  //    )
  // {
  //   /*    Enviar Tara de Peso   */
  //   for (int i = 0; i < ((VAR_WIRE_LEN(VAR_WEIGHT_TARA)) ); ++i)
  //   {
  //     xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_WEIGHT_TARA, i);
  //     cont++;
  //   }
  //   xbee.alarmsVarCount++;
  //   /*    Enviar Referencia de Peso   */
  //   for (int i = 0; i < ((VAR_WIRE_LEN(VAR_WEIGHT_REF_RAW)) ); ++i)
  //   {
  //     xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_WEIGHT_REF_RAW, i);
  //     cont++;
  //   }
  //   xbee.alarmsVarCount++;
  //   /*    Enviar Referencia de peso por el Usuario   */
  //   for (int i = 0; i < ((VAR_WIRE_LEN(VAR_REFERENCE_KG)) ); ++i)
  //   {
  //     xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_REFERENCE_KG, i);
  //     cont++;
  //   }
  //   xbee.alarmsVarCount++;
  // }
  /*    SI LA ALARMA FUE POR TIEMPO CUMPLIDO   */
  // if ( bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 3) == 1 )
  // {
  //   //BYTES DE SETEO
  //   for (int i = 0; i < ((VAR_WIRE_LEN(VAR_SETTING_BYTES)) ); ++i)
  //   {
  //     xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_SETTING_BYTES, i);
  //     cont++;
  //   }
  //   xbee.alarmsVarCount++;
  //   //TIEMPO DE ENVIO DE LA ALARMA DE CHEQUEO DE LA COMUNICACION
  //   for (int i = 0; i < ((VAR_WIRE_LEN(VAR_TIME_ALARM)) ); ++i)
  //   {
  //     xbee.alarms[cont] = VAR_WIRE_BYTE(VAR_TIME_ALARM, i);
  //     cont++;
  //   }
  //   xbee.alarmsVarCount++;
  // }
  xbee.payloadSize = cont; // Asignacion de la cantidad de bytes de la "data"
  //CREAR AQUI LAS DEMAS POSIBLES ALARMAS//
  /*
  Serial.print("cont: ");
  Serial.print(cont);
  Serial.println();
  */
  return;
}
void Alarms (void)
{
  create_data_Alarms(); 
  create_init_Alarms(xbee.txFrame, xbee.alarms, xbee.payloadSize);
  print_frame("Send Alarms : <",xbee.txFrame, xbee.payloadSize," >");
  send_frame(xbee.txFrame, xbee.payloadSize);
  xbee.alarmsVarCount = 0;
  for (uint8_t i = 0; i < 4; i++) {
    xbee.comm.alarmsSnapshot[i] = VAR_WIRE_BYTE(VAR_ALARMS, 2 + i);
  }
}
void Send_Sequence_Alarms(void)
{
    // Si se activo el bit de dosificacion  
  if((bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3) == 1)  ||
    // Si se cambio los estados de las alarmas
    (xbee.comm.alarmsSnapshot[2] != VAR_WIRE_BYTE(VAR_ALARMS, 4))    ||
    // Si hay baja bateria
    (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 4) == true) ||
    // Si se cumple el tiempo de espera desde el Ãºltimo mensaje recibido
    (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 3) == 1)
    )
  {
    xbee.comm.ackTimeA = millis();
    // ******************** ALARMA DE DOSIFICACION ********************
    if( ( bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3) == 1) &&
      ( (xbee.comm.ackTimeA >= ( xbee.comm.ackTimeB + ( xbee.comm.ackMultiplier * 5000 ))) || (app.comm.initAlarms == FALSE) ) )
    {
      Alarms();
      if (app.comm.initAlarms == TRUE)
      {
        xbee.comm.ackMultiplier = 1;
      }
      xbee.comm.ackTimeB = millis();
      app.comm.initAlarms = TRUE; 
    }
    // ******************** ALARMA DE BATERIA ********************
    else if ( (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 4) == true) && 
              // (xbee.comm.ackTimeA >= ( xbee.comm.ackTimeB + 10000 )) || (app.comm.initAlarms == FALSE) )
              (xbee.comm.ackTimeA >= ( xbee.comm.ackTimeB + 900000 )) || (app.comm.initAlarms == FALSE) )
    {
      Alarms();
      if (app.comm.initAlarms == TRUE)
      {
        xbee.comm.ackMultiplier = xbee.comm.ackMultiplier * 2;
      }
      xbee.comm.ackTimeB = millis();
      if(xbee.comm.ackMultiplier > 32)
      {
        xbee.comm.ackMultiplier = 1;
      }
      app.comm.initAlarms = TRUE; 
    }
  }
  else if(app.comm.deliveryAck == TRUE)
  {
    xbee.comm.ackTimeB = 0;
    xbee.comm.ackMultiplier = 1;
    app.comm.deliveryAck = FALSE;
    app.comm.initAlarms= FALSE;
  }  
}
void Send_Sequence_Broadcast(void)
{
  if(app.comm.answerBroadcast == FALSE)
  {
    xbee.comm.broadcastTimeA = millis();
    if( (xbee.comm.broadcastTimeA >= (xbee.comm.broadcastTimeB + (xbee.comm.broadcastMultiplier * 60000))) || (app.comm.initBroadcast == FALSE) )
    {
      if(app.comm.sendUnicast == FALSE)
      {
        broadcast();
      }
      else
      {
        unicast();
      }      
      if (app.comm.initBroadcast == TRUE)
      {
        xbee.comm.broadcastMultiplier = xbee.comm.broadcastMultiplier * 2;
      }
      xbee.comm.broadcastTimeB = millis();
      /*Serial.print("multiplo broadcast: ");
      Serial.print(xbee.comm.broadcastMultiplier);
      Serial.println();*/
      if(xbee.comm.broadcastMultiplier > 32)
      {
        if(app.comm.sendUnicast == TRUE)
        {
          broadcast();
        }
        xbee.comm.broadcastMultiplier = 1;
      }
      app.comm.initBroadcast = TRUE; 
    }
  }    
}
void NewServer(void)
{
  //HACER SERVIDOR CUALQUIER DIRECCION QUE HAGA COMUNICACION
  if( (xbee.txFrame[4] != VAR_WIRE_BYTE(VAR_SERVER, 2)) || (xbee.txFrame[5] != VAR_WIRE_BYTE(VAR_SERVER, 3)) || (xbee.txFrame[6] != VAR_WIRE_BYTE(VAR_SERVER, 4)) || (xbee.txFrame[7] != VAR_WIRE_BYTE(VAR_SERVER, 5)) || (xbee.txFrame[8] != VAR_WIRE_BYTE(VAR_SERVER, 6)) || (xbee.txFrame[9] != VAR_WIRE_BYTE(VAR_SERVER, 7)) || (xbee.txFrame[10] != VAR_WIRE_BYTE(VAR_SERVER, 8)) || (xbee.txFrame[11] != VAR_WIRE_BYTE(VAR_SERVER, 9)) )
  {
    Serial.print("New Server: ");
    for(uint8_t i = 0; i<=7; i++)
    {
      VAR_WIRE_BYTE(VAR_SERVER, 2 + i) = xbee.txFrame[4 + i];
      Serial.print(xbee.txFrame[4 + i] , HEX);
      Serial.print(" ");
    }
    saveInEeprom(VAR_SERVER);
    Serial.println(" ");    
  }
}
/**
 * Required for serialEvent execution (Mega: llamar desde loop).
 * COMUNICACION POR EVENTO
 */
void serialEventRun(void) {
  if (Serial3.available()) xbee_communication();
}
/*
  Descrip.
    Crea trama unicast
    -return void
  */
void create_init_unicast(unsigned char frame[], unsigned char data[], unsigned char data_size) {
  // TODO: use create_frame
  // Send vars
  // frame size = bf command (1) + bf msg id (2) + var amount (1) + sum of vars (var code (2) + var size (*))
  frame[0] = STARTDELIMITER; // Start delimiter
  frame[1] = highByte(data_size + 18); // Length MSB
  frame[2] = lowByte(data_size + 18); // Length LSB
  frame[3] = CMD_TRANSMIT_FRAME; // Transmit frame
  frame[4] = 0x01; // Frame Id
  // Destination address 64 bits (transferServer)
  frame[5] = VAR_WIRE_BYTE(VAR_SERVER, 2);
  frame[6] = VAR_WIRE_BYTE(VAR_SERVER, 3);
  frame[7] = VAR_WIRE_BYTE(VAR_SERVER, 4);
  frame[8] = VAR_WIRE_BYTE(VAR_SERVER, 5);
  frame[9] = VAR_WIRE_BYTE(VAR_SERVER, 6);
  frame[10] = VAR_WIRE_BYTE(VAR_SERVER, 7);
  frame[11] = VAR_WIRE_BYTE(VAR_SERVER, 8);
  frame[12] = VAR_WIRE_BYTE(VAR_SERVER, 9);
  // Destination address 16 bit
  frame[13] = 0xff;
  frame[14] = 0xfe;
  frame[15] = 0x00; // Broad cast radius
  frame[16] = 0x00; // Options
  // Biofeeder frame header
  frame[17] = CMD_INIT; // Init frame command
  frame[18] = 0xFF; // Msg Id MSB
  frame[19] = 0xFF; // Msg Id LSB
  frame[20] = 0x04; // Amount of var to send TODO: Calculate this dynamically
  // Data
  for (int i = 0; i < data_size; i++) {
    frame[21 + i] = data[i];
  }
  frame[21 + data_size] = 0; // Checksum
  set_checksum(frame, data_size + FRAME_HEADER_UNICAST);
}
  /*
  Descrip.
    Crea data para la trama de unicast
    -return void
  */
void create_data_unicast(){
  int cont = 0;
  appendVarWire(VAR_SOFTWARE_VERSION, xbee.unicast, cont);
  appendVarWire(VAR_HARDWARE_VERSION, xbee.unicast, cont);
  appendVarWire(VAR_PROJECT_NAME, xbee.unicast, cont);
  for (int j = 0; j < 3; ++j) {
    xbee.unicast[cont] = VAR_WIRE_BYTE(VAR_PROFILE, j);
    cont++;
  }
  cont--;
}
  /*
  Descrip.
    Es un conjunto de funcion que se agrupa para
    - Crear data para la trama del unicast
    - Crea la trama para el unicast
    - Imprime la trama del unicast
    - Envia la trama del unicast
    -return void
  */
void unicast (void){
  create_data_unicast();
  xbee.payloadSize    = (sizeof(xbee.unicast)/sizeof(xbee.unicast[0]));
  //Serial.println((xbee.payloadSize));
  create_init_unicast(xbee.txFrame, xbee.unicast, xbee.payloadSize);
  print_frame("Send unicast : <",xbee.txFrame, xbee.payloadSize," >");
  send_frame(xbee.txFrame, xbee.payloadSize);
}
void cleanAlarms(){
  VAR_WIRE_BYTE(VAR_ALARMS, 2) = 0;
  VAR_WIRE_BYTE(VAR_ALARMS, 3) = 0;
  VAR_WIRE_BYTE(VAR_ALARMS, 4) = 0;
  VAR_WIRE_BYTE(VAR_ALARMS, 5) = 0;
  return;
}
