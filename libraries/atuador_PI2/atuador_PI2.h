/*  Biblioteca do Sistema de atuação no controle do Forno 
 *  Autore: Ítalo Rodrigo Moreira Borges
 *  Data: 23/06/2019
 *  
 *  atuador_PI2.h
 */

  // guarda de inclusão
#ifndef AtuadorTemp
#define AtuadorTemp

#include "Arduino.h"

class Triac_PI2
{
   public:
       Triac_PI2(int pin);  
       void ControlePotencia(float pot);

   private:
       int _pin;
  
  
};

#endif
