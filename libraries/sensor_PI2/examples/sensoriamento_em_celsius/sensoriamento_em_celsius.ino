/*  Exemplo de sensoriamento na matéria de PI2 na Faculdade Gama - UnB 
 *  Autore: Ítalo Rodrigo Moreira Borges
 *  Data: 23/06/2019
 *  
 *  sensoriamento_em_celsius.ino
 */

 #include "sensor_PI2.h"
// Pinos
int maxSO = D0;
int maxCS = D1;
int maxCLK = D2;

// Instanciando objeto
MAX6675_PI2 moduloMAX(maxCLK, maxCS, maxSO);;



void setup() {
  Serial.begin(9600);
  
}

void loop() {
    
  float temp = moduloMAX.lerCelsius();
  Serial.print("\tTemperatura em Celsius = ");
  Serial.println(temp);
  
  
  Serial.println();
  delay(1000);
}