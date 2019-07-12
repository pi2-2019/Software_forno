//Importando as Bibliotecas
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <sensor_PI2.h>
#include <Ticker.h>
#include "index.h"

//Definindo os pinos
#define maxSO  D0 //  D0
#define maxCS  D1 //  D1
#define maxCLK D2 //  D2
#define triac  12 //  D6
#define zero   D7
#define LED 2

//Instanciando os Objetos
MAX6675_PI2 moduloMAX(maxCLK, maxCS, maxSO);
Ticker timerSensor;
Ticker timerPerfil;
Ticker timerSerial;
ESP8266WebServer server(80); //Server on port 80

//Variáveis Globais 
bool inits = false; 

//const char* ssid = "S9 gustavo";
//const char* password = "fernandinha";
const char* ssid = "leothi_note";
const char* password = "VmcVl7CJ";

float kc=5.207;
float ki=0.006;
float kd=55.44;
int Ts=100;    
float Ti= (kc*Ts)/ki; 
float Td= (kd*Ts)/kc;  
float ek_1=0;
float ek_2=0;
float uk_1=0;
float yk=0; 
float rk=0;  
float ek= yk-rk;
float uk= kc*(ek-ek_1+ Ts/Ti*ek+Td/Ts*(ek-2*ek_1+ek_2))+uk_1;

int contCiclos=0;
int controle_potencia=0;

int t_perfil=0;
int array_perfil=0;
float set_point=0;
float perfil_temp [19]={28.9988,52.1978,75.9768,99.1758,128.7545,141.5140,155.4334,174.5726,197.1916,224.4505,241.8498,209.9511,181.5323,156.5934,128.1746,101.4957,75.3968,55.6776,28.9988};
int perfil_tempo[19]={4,18,31,48,74,89,111,129,143,161,180,201,210,220,232,246,262,276,298};
 

void setup()
{
  pinMode(triac, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600); //apenas para debugar

  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/readADC", handleADC); //This page is called by java Script AJAX
  server.on("/init", HTTP_POST, handleInit);
  
  server.begin();  
  Serial.println("HTTP server started");
  Serial.println("");  
  
  timerSensor.attach(3,sensor_ler);
  timerPerfil.attach(1,perfil_reflow);
  timerSerial.attach(4,exibirSerial);

  attachInterrupt(zero, angle, RISING);
}

void loop()
{
  server.handleClient();     
  if(inits == true){
    yk=set_point;   // set point
    ek= yk-rk;
    uk= (kc*((ek-ek_1)+ ((Ts/Ti)*(ek))+((Td/Ts)*(ek-2*ek_1+ek_2))))+uk_1; 
    
    controle_potencia=uk;
    if((yk<rk)) { 
    controle_potencia=0;
    uk=0;
    }
      
    if((controle_potencia > 100) && (yk>=rk)){
       controle_potencia=100;
       uk=100;  
    }
    if(controle_potencia<0){
      controle_potencia=0;
      uk=0;
    }  
       
    ek_2=ek_1;
    ek_1=ek;
    uk_1=uk; 
  }
}

void angle(){
  if(inits == true) {
      contCiclos++;
         
      if(contCiclos>100){
          contCiclos=0;
        }      
      if(contCiclos<=controle_potencia){
       digitalWrite(triac, HIGH);         
       }
      else if((contCiclos>=controle_potencia) && (contCiclos<=10)){
       digitalWrite(triac,LOW);       
      }
  }
}

// Timer
void sensor_ler(){
   rk = moduloMAX.lerCelsius();
}

void exibirSerial(){     
    Serial.println("");
    Serial.println("");
    Serial.print("sensor C = "); Serial.println(rk); //apenas verificando a saida
    Serial.print("temperatura desejada = "); Serial.println(set_point); //quando é float acrescenta um long
    Serial.print("controle_potencia = "); Serial.println(controle_potencia); //apenas verificando a saida
    Serial.print("uk = "); Serial.println(uk); //apenas verificando a saida
    Serial.print("tempo = "); Serial.println(t_perfil); //apenas verificando a saida
    Serial.print("posicao = "); Serial.println(array_perfil); //apenas verificando a saida
    Serial.print("inits = "); Serial.println(inits);
}

 
void perfil_reflow() 
 {
   if(inits == true) {
     t_perfil++;  
  
     if(t_perfil==perfil_tempo[array_perfil])
     {
       array_perfil++;
       set_point=perfil_temp[array_perfil];
     }
  
     if(array_perfil>0){
         if((rk<perfil_temp[array_perfil-1]) && (array_perfil<=10) )
         {
             t_perfil--;
         }
     }
   }   
}

void handleRoot() {                                    
 String s = MAIN_page; 
 digitalWrite(LED, LOW);
 server.send(200, "text/html", s); 
}

void handleADC() {
 String sensorRead = String(rk);
 server.send(200, "text/plane", sensorRead); 
} 

void handleInit() {
  inits = true; 
  server.sendHeader("Location","/");
  server.send(303);
}
