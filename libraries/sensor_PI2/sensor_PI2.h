/*  Biblioteca do Sistema de Sensoriamento do controle do Forno 
 *  Autore: Ítalo Rodrigo Moreira Borges
 *  Data: 23/06/2019
 *  
 *  sensor_PI2.h
 */

  // guarda de inclusão
#ifndef SensorTemp
#define SensorTemp
 
#include <Arduino.h>

// Criação da classe
class MAX6675_PI2 {
 public:
  MAX6675_PI2(int8_t SCLK, int8_t CS, int8_t MISO);
  double lerCelsius(void);
  
 private:
  int8_t sclk, miso, cs;
  uint8_t leituraSPI(void);
};

#endif
