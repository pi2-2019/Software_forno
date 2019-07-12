/*  Biblioteca do Sistema de atuação no controle do Forno 
 *  Autor: Ítalo Rodrigo Moreira Borges
 *  Data: 23/06/2019
 *  
 *  atuador_PI2.cpp
 */

#include "Arduino.h"
#include "atuador_PI2.h"
#define T 8333.33 // O período em us
//#define T 2000000 // O período em 2 segundos (Testando a biblioteca)

Triac_PI2::Triac_PI2(int pin)
{
   pinMode(pin, OUTPUT);
   _pin = pin;  
  
}

void Triac_PI2::ControlePotencia(float pot)
{
   digitalWrite(_pin, HIGH);
   delayMicroseconds(pot*T);  // Perído de Tempo do HIGH
   digitalWrite(_pin,  LOW);
   delayMicroseconds((1-pot)*T); // Período de Tempo do LOW que é complemento de HIGH
  
}
