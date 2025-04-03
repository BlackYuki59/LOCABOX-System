#include <SPI.h>
#include <Wire.h>
#define MOSI 11
#define SCLK 13
#define CS 10
#define RS 12
#define ECL 8
#define DATA_OUT_A 3
#define DATA_OUT_B 4
#define DATA_OUT_C 5
#define DATA_OUT_D 6
#define DA 2
#define SDA A4
#define SCL A5

char touche = ' ';
char tab[] = {'1', '7', '4', '*', '3', '9', '6', '#', '2', '8', '5', '0', 'B', 'b','V', 'R'};
char code[6] ; //tableau pour stocker un code de 6 caractères
int code_index = 0; // Index pour le tableau 'code'
unsigned int flag=0;
unsigned int tx_done=0;
unsigned int etat=0;
const unsigned int taille=6;
unsigned long temps;
const int slaveAddress = 0x08;
unsigned char dataToSend[9] = {'D', 'A',' ', ' ', ' ', ' ', ' ', ' ', 'F'};






void setup() {
  Serial.begin(115200);
  pinMode(ECL, OUTPUT);
  digitalWrite(ECL, HIGH);
  Wire.begin(slaveAddress);
 
  delay(100);
  pinMode(DATA_OUT_A, INPUT);
  pinMode(DATA_OUT_B, INPUT);
  pinMode(DATA_OUT_C, INPUT);
  pinMode(DATA_OUT_D, INPUT);
  pinMode(DA, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(DA), lect_touche, RISING);
   Wire.onRequest(requestEvent);
}

void lect_touche() {
  int dataA = digitalRead(DATA_OUT_A);
  int dataB = digitalRead(DATA_OUT_B);
  int dataC = digitalRead(DATA_OUT_C);
  int dataD = digitalRead(DATA_OUT_D);
  int nb = dataD + 2 * dataC + 4 * dataB + 8 * dataA;
  touche = tab[nb]; // Affectation directe de la valeur à la variable globale 'touche'
  flag=1;

}
void requestEvent() {
  
  
  for (int i=0;i<9;i++){
    Wire.write(dataToSend[i]);
    
   }
  if (etat==4) etat = 0;
  if (dataToSend[1]=='C') dataToSend[1]='A';
  
}

void loop() {
    if (etat==0)
    {
      Serial.println(" ");
      Serial.println("Etat 0 : Initialisation");
      for (int i=0;i<taille;i++) code[i]=' ';

      code_index=0;
      flag=0;
      etat=1;
    }
    if (etat==1 && flag==1 )etat=2;
      if (etat == 2) {
    Serial.println("Etat 2 : Lecture de la touche");
    flag = 0; // Réinitialiser le flag après la prise en compte
     if (touche == 'B' || touche == 'b' || touche == 'V' || touche == 'R') {
        Serial.println("Erreur : Touche invalide");
        etat = 3;
     }
     else if (touche == '#') {
      if (code_index > 0) {
        // Supprimer le premier caractère si '#' est pressé
        code_index--;            // Décrémenter l'index
        code[code_index] = ' ';  // Effacer la dernière position
      }
      Serial.println("Dernier chiffre supprimer");
      etat = 3;
    } else if (touche == '*') {
      // Réinitialiser tout si '*' est pressé
      memset(code, 0, sizeof(code));
      code_index = 0;
      Serial.println("Code Réinitialiser");
      etat = 3;
    } else {
      // Ajouter la touche au tableau si ce n'est pas '*' ou '#'
      code[code_index] = touche;
      code_index++;
      etat = 3; // Passer à l'état suivant
      temps = millis();
    }
  }
    if(etat==3 && flag==1 && code_index<6) etat=2;
    if(etat==3 && code_index>=6 && touche == '*') etat=0;
    if(etat==3 && code_index>=6 && touche == '#') 
    {
      etat=2;
    }
    if(etat==3 && code_index>=6 && touche == 'R') {
            
            Serial.println("Etat 4 : Attente");
            for (int i=0;i<taille;i++) {
              Serial.print(code[i]);
              dataToSend[i+2] = code[i];
              dataToSend[1]='C';
              etat=4;
              
              
              
             
            }
           
            
            temps=millis();
    }

    if(etat==3 && (millis()-temps)>5000) etat=0;
    if (etat==4 && (tx_done || (millis()-temps)>10000)) etat=0;
    
    
    

      
    

    // Attendre avant de recommencer une nouvelle saisie
    delay(10);
}
