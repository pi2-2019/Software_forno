/*  Biblioteca do Sistema de Sensoriamento do controle do Forno 
 *  Autore: Ítalo Rodrigo Moreira Borges
 *  Data: 23/06/2019
 *  
 *  sensor_PI2.cpp
 */

#include <stdlib.h>
#include <Arduino.h>
#include "sensor_PI2.h"

// Criação do objeto MAX
MAX6675_PI2::MAX6675_PI2(int8_t SCLK, int8_t CS, int8_t MISO) {
  sclk = SCLK;
  cs = CS;
  miso = MISO;

  //define pin modes
  pinMode(cs, OUTPUT);
  pinMode(sclk, OUTPUT); 
  pinMode(miso, INPUT);

  digitalWrite(cs, HIGH);
}

// Função Celsius
double MAX6675_PI2::lerCelsius(void) {

  uint16_t pre_temp;

  digitalWrite(cs, LOW);
  //delay(1);

  pre_temp = leituraSPI();
  pre_temp <<= 8;
  pre_temp |= leituraSPI();

  digitalWrite(cs, HIGH);

  if (pre_temp & 0x4) {
    return NAN; 
  }

  pre_temp >>= 3;

  float temp_celsius = pre_temp * 0.25;
  
  return temp_celsius;
}

// Função SPI
byte MAX6675_PI2::leituraSPI(void) { 
  int i;
  byte registrador = 0;

  for (i=7; i>=0; i--)
  {
    digitalWrite(sclk, LOW);
    //delay(1);
    if (digitalRead(miso)) {
      registrador |= (1 << i);
    }

    digitalWrite(sclk, HIGH);
    //delay(1);
  }

  return registrador;
}
