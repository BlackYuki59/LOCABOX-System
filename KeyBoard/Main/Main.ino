#include <SPI.h>
#include <Wire.h>
#define MOSI 11
#define SCLK 13
#define CS 10
#define RS 9
#define ECL 8
#define DATA_OUT_A 3
#define DATA_OUT_B 4
#define DATA_OUT_C 5
#define DATA_OUT_D 6
#define DA 2
#define SDA A4
#define SCL A5
#define MAX_CHARACTERS_PER_LINE 16
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


void initializeScreen() {
  // Initialiser l'écran avec les commandes appropriées pour RW1063
  sendCommand(0x38);  // code d'affichage : 8 bits, 2 lignes, mode de caractères
  delay(100);          // Attendre un court instant
  sendCommand(0x38);  // code d'affichage : 8 bits, 2 lignes, mode de caractères
  delay(100);          // Attendre un court instant
  sendCommand(0x38);  // code d'affichage : 8 bits, 2 lignes, mode de caractères
  delay(100);          // Attendre un court instant
  sendCommand(0x38);  // code d'affichage : 8 bits, 2 lignes, mode de caractères
  delay(100); 
  sendCommand(0x08);
  delay(100);
  sendCommand(0x01);
  delay(100); 
  sendCommand(0x0C);  // Affichage activé, curseur masqué
  delay(100);
  sendCommand(0x06);  // Incrémenter automatiquement la position du curseur
   delay(100); 
}

void sendCommand(byte cmd) {
  // Envoyer une commande à l'écran
  digitalWrite(RS_PIN, LOW);  // Commande
  digitalWrite(CS_PIN, LOW);  // Activer le CS
  SPI.transfer(cmd);          // Envoyer la commande
  digitalWrite(CS_PIN, HIGH); // Désactiver le CS
}

void sendData(byte data) {
  // Envoyer des données à l'écran
  digitalWrite(RS_PIN, HIGH);  // Données
  digitalWrite(CS_PIN, LOW);   // Activer le CS
  SPI.transfer(data);          // Envoyer les données
  digitalWrite(CS_PIN, HIGH);  // Désactiver le CS
}

void drawText(const char* text) {
  int i = 0;
  
  // Parcours chaque caractère du texte à afficher
  while (*text && i < MAX_CHARACTERS_PER_LINE) {
    sendData(*text++);  // Envoyer chaque caractère
    i++;
  }
}

void deleteLastCharacterOnScreen(int currentIndex) {
  if (currentIndex > 0) {
    // Déplacer le curseur à la position de la dernière lettre
    setCursorPosition(0, currentIndex - 1); // Ligne 0, colonne de la dernière lettre

    // Envoyer un espace pour effacer la dernière lettre
    sendData(' ');
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ECL, OUTPUT);
  digitalWrite(ECL, HIGH);
  Wire.begin(slaveAddress);
  pinMode(RS, OUTPUT);
  pinMode(CS, OUTPUT);
 
  delay(100);
  pinMode(DATA_OUT_A, INPUT);
  pinMode(DATA_OUT_B, INPUT);
  pinMode(DATA_OUT_C, INPUT);
  pinMode(DATA_OUT_D, INPUT);
  pinMode(DA, INPUT_PULLUP);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV64);  // Ajuste la vitesse SPI si nécessaire
  
  // Initialisation de l'écran
  initializeScreen(); 
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
      sendCommand(0x01);  // Effacer tout l'écran
      delay(2);           // Attendre que l'effacement soit effectif
      sendCommand(0x80);  // Définir le curseur au début (ligne 1, colonne 1)
      delay(100);
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
        deleteLastCharacterOnScreen(code_index);// Supprimer la lettre à l'écran
      }
      Serial.println("Dernier chiffre supprimer");
      etat = 3;
    } else if (touche == '*') {
      // Réinitialiser tout si '*' est pressé
      sendCommand(0x01);
      memset(code, 0, sizeof(code));
      code_index = 0;
      Serial.println("Code Réinitialiser");
      etat = 3;
    } else {
      // Ajouter la touche au tableau si ce n'est pas '*' ou '#'
      drawText(&touche);
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
