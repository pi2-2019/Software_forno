/*  Exemplo de sensoriamento na matéria de PI2 na Faculdade Gama - UnB 
 *  Autore: Ítalo Rodrigo Moreira Borges
 *  Data: 23/06/2019
 *  
 *  sensoriamento_em_celsius.ino
 */

#include <atuador_PI2.h>
#define triac 13 // é igual a D7

//Declarando objeto atuador
Triac_PI2 AtuadorPI2(triac);


void setup() {
  
}

void loop() {
  int saida=1;
  AtuadorPI2.ControlePotencia(saida); //saida é uma porcentagem normalizada ou seja varia de 0-1
}
