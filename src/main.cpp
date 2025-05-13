
#include <Arduino.h>
#include "lmic.h"
#include <hal/hal.h>
#include <SPI.h>
#include <SSD1306.h>
#include <Wire.h>

#define LEDPIN 2
#define OLED_I2C_ADDR 0x3C
#define Gache 4
#define Capteur_Porte 34

#define Led_O 12
#define Led_R 13
#define Alarme 25
#define Button 15
#define Led_V 15


int flag = 0;
const int slaveAddress = 0x08;
char receivedData[6] = {' ', ' ', ' ', ' ', ' ', ' '};
unsigned int counter = 0;
char TTN_response[30];
char old_receivedData[6] = {' ', ' ', ' ', ' ', ' ', ' '};
int timer = 0;
int i = 0;
int detect_touche=0;
#define OLED_SDA 21  // Replace with the correct SDA pin for your board
#define OLED_SCL 22  // Replace with the correct SCL pin for your board
SSD1306 display (OLED_I2C_ADDR, OLED_SDA, OLED_SCL);
int count = 0;
unsigned long temps; // Stocke le temps précédent
unsigned long temps_Alarme;
const unsigned long GACHE_INTERVAL = 5000; // Intervalle de 5 secondes


void requestData() {
    Wire.requestFrom(slaveAddress,9);
    int i = 0;
    while (Wire.available() && i < 9) {
      receivedData[i] = Wire.read();
      
       i++;
     }
   
 
}
bool VerifierCode(const char* code, const char* receivedData) {
    for (int i = 0; i < 6; i++) {
        if (code[i + 1] != receivedData[i + 2]) {
            return false; // Code incorrect
        }
    }
    return true; // Code correct
}




// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes.

// Copy the value from Device EUI from the TTN console in LSB mode.
static const u1_t PROGMEM DEVEUI[8]= { 0x64, 0xB7, 0x08, 0xAB, 0x84, 0x38, 0xFE, 0xFF};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// Copy the value from Application EUI from the TTN console in LSB mode
static const u1_t PROGMEM APPEUI[8]= {0x64, 0xB7, 0x08, 0xAB, 0x84, 0x38, 0xFE, 0xFF};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is. Anyway its in MSB mode.
static const u1_t PROGMEM APPKEY[16] = {0x64, 0xB7, 0x08, 0xAB, 0x84, 0x38, 0xFE, 0xFF,0x64, 0xB7, 0x08, 0xAB, 0x84, 0x38, 0xFE, 0xFF};
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 120;

// Pin mapping

const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 33, 32}  // Pins for the Heltec ESP32 Lora board/ TTGO Lora32 with 3D metal antenna
};
static uint8_t message[]="MDRCDCA  E  BFES";
static uint8_t code[] = "        ";
void do_send(osjob_t* j){
    // Payload to send (uplink)
    flag = 1;
    delay(50);
    

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, message, sizeof(message)-1, 0);
        Serial.println(F("Sending uplink packet..."));
        digitalWrite(LEDPIN, HIGH);
        display.clear();
        display.drawString (0, 0, "Sending uplink packet...");
        display.drawString (0, 50, String (++counter));
        display.display ();
        message[4]=' ';
        message[5]=' ';
        message[7]=' ';
        message[8]=' ';
        message[10]=' ';
        message[11]=' ';
    }
    // Next TX is scheduled after TX_COMPLETE event.
    flag = 0;
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
           case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            display.clear();
            display.drawString (0, 0, "EV_TXCOMPLETE event!");

            if (LMIC.txrxFlags & TXRX_ACK) {
              Serial.println(F("Received ack"));
              display.drawString (0, 20, "Received ACK.");
            }

            if (LMIC.dataLen) {
              int i = 0;
              // data received in rx slot after tx
              Serial.print(F("Data Received: "));
              Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
              Serial.println();
              Serial.println(LMIC.rssi);

              display.drawString (0, 9, "Received DATA.");
              for ( i = 0 ; i < LMIC.dataLen ; i++ )
              {
                TTN_response[i] = LMIC.frame[LMIC.dataBeg+i];
                code[i]=TTN_response[i];
              }
              TTN_response[i] = 0;
              display.drawString (0, 22, String(TTN_response));
              display.drawString (0, 32, String(LMIC.rssi));
              display.drawString (64,32, String(LMIC.snr));
            }

            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            digitalWrite(LEDPIN, LOW);
            display.drawString (0, 50, String (counter));
            display.display ();
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING: -> Joining..."));
            display.drawString(0,16 , "OTAA joining....");
            display.display();
            break;
        case EV_JOINED: {
              Serial.println(F("EV_JOINED"));
              display.clear();
              display.drawString(0 , 0 ,  "Joined!");
              display.display();
              // Disable link check validation (automatically enabled
              // during join, but not supported by TTN at this time).
              LMIC_setLinkCheckMode(0);
            }
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }

}

int etatPorte; // Declare the variable to store the state of the door sensor
int etatButton;
void setup() {
    Serial.begin(115200);
    delay(2500);                      // Give time to the serial monitor to pick up
    Serial.println(F("Starting..."));

    pinMode(Capteur_Porte, INPUT); // Capteur de porte
    pinMode(Gache, OUTPUT);        // Gâche
    
    pinMode(Led_R, OUTPUT);        // LED rouge
    pinMode(Led_O, OUTPUT);        // LED orange
    pinMode(Alarme, OUTPUT); 
    pinMode(Button, INPUT); 

    etatPorte = digitalRead(Capteur_Porte); // Capteur de porte
    etatButton = digitalRead(Button);
    // Initialiser les LEDs et la gâche
    digitalWrite(Led_R, HIGH); // LED rouge allumée par défaut
   
    digitalWrite(Led_O, LOW);  // LED orange éteinte
    digitalWrite(Gache, HIGH); 
    digitalWrite(Alarme, LOW);



    // Use the Blue pin to signal transmission.
    pinMode(LEDPIN,OUTPUT);

    display.init ();
    display.flipScreenVertically ();
    display.setFont (ArialMT_Plain_10);
    display.setTextAlignment (TEXT_ALIGN_LEFT);
    display.drawString (0, 0, "Starting....");
    display.display ();

   
    os_init();

    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
   
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    //LMIC_setDrTxpow(DR_SF11,14);
    LMIC_setDrTxpow(DR_SF9,14);

    // Start job
    do_send(&sendjob);     // Will fire up also the join
    //LMIC_startJoining();
}


char etat=0;
void loop() {
    os_runloop_once();
    if (flag == 0) {
        if (timer >= 375000) { // 1 second
            requestData();
            message[1] = 'O';
            message[2] = 'K';
            etatPorte = digitalRead(Capteur_Porte); // Capteur de porte
            etatButton = digitalRead(Button);
            Serial.print((int)etat);
            Serial.print(" ");
            Serial.print(etatPorte);
            Serial.print(" ");
            Serial.print(etatButton);
            Serial.println(" ");            

            
            if (receivedData[1] == 'C') //reception code arduino affichage monitor et etat b1=1
            {
                for (int i = 0; i < 9; i++) {
                    Serial.print(receivedData[i]);
                    etat = etat | 2;
                }
            }
            if (code[0] == 'C')//reception code lora etat b0=1
             {
                code[0] = 'R';
                message[4] = 'C';
                message[5] = 'R';
                etat = etat | 1;
                Serial.println("Code Lora reçu");
                
            }

            if ((etat & 0x03) == 0x03)  // si etat=3 (code recu arduino et code lora recu)
            {
                if(VerifierCode((const char*)code, receivedData) == true) //si code correct deverouille et met etat b3=1
                {
                    Serial.println("Code correct!");
                    digitalWrite(Gache, LOW);  // Gâche activée
                    digitalWrite(Alarme, LOW); // Alarme désactivée
                    message[10] = 'D';
                    message[11] = 'V';
                    message[4] = 'C';
                    message[5] = 'U';
                    temps = millis();
                    etat=etat|0x18;
                    count = 0;
                    
                } else {
                    Serial.println("Code incorrect!");
                    count++;
                    if (count <= 2) {
                        message[10] = 'C';
                        message[11] = 'F';
                    } else if (count == 3) {
                        message[7] = 'E';
                        message[8] = 'C';
                        digitalWrite(Alarme, HIGH);
                        count = 0;
                    }
                }
                etat=etat&0xFD; //remet etat b1=0
            }
                        
             
            if((etatPorte == HIGH ) && ((etat & 0x08)==0x00) && ((etat & 0x20) == 0x00)&& ((etat & 0x04) == 0x00)){//Porte ouverte et gache vérouillée met etat b5=1, met etat b6=1 
              
                    digitalWrite(Alarme, HIGH);
                    message[13] = 'O'; // Porte ouverte
                    message[14] = 'U';
                    message[7] = 'I';
                    message[8] = 'T'; 
                    etat=etat|0x20;
                    etat=etat|0x40;
                    temps_Alarme = millis();
                    Serial.println("Porte ouverte alarme déclenchée");
                    
                
            }
            if((etatPorte == LOW) && (etatButton == LOW) && ((etat & 0x04)==0x00)){ //Box fermée et bouton appuyé = dévérouillage gache met etat b2=1
                    digitalWrite(Gache, LOW);
                    digitalWrite(Led_O, HIGH);
                    digitalWrite(Led_R, LOW);
                    message[13] = 'F';
                    message[14] = 'E';
                    message[10] = 'D';
                    message[11] = 'V'; 
                    temps = millis();
                    etat=etat|0x04;
                    Serial.println("Déverrouillage de l'intérieur du box");
                    
                }
                
                    
                
            
                
             
           
            if (((etat & 0x08 )==0x08)  && (etatPorte == HIGH))  {//code correcte et portre ouverte 
                
                    digitalWrite(Led_R, LOW);
                    digitalWrite(Led_O, LOW);
                    message[13] = 'O'; // Porte ouverte
                    message[14] = 'U';
                    etat=etat&0xF7;
                    Serial.println("Porte ouverte");
                 }
                
            if (etatPorte == LOW){ // Porte fermée
                    digitalWrite(Led_R, HIGH);
                    message[13] = 'F';
                    message[14] = 'E';
                    temps = millis();
                    
                    etat = etat & 0xDB;
            }
            if (((etat & 0x10)== 0x10) &&(millis()-temps)>5000){//Vérouillage Gache parès 5 secondes lorsque la porte est fermée
                        digitalWrite(Gache, HIGH);
                        digitalWrite(Led_R, HIGH);
                        digitalWrite(Led_O, LOW);
                        message[10] = 'V';
                        message[11] = 'R';
                        etat=etat&0xE7;
                        Serial.println("Gâche verrouillée");
            }
            if(((etat & 0x40) == 0x40) && (millis()-temps_Alarme)>30000){//Désactivation de l'alarme après 30 secondes
                        digitalWrite(Alarme, LOW);
                        etat=etat&0xBF;
                        Serial.println("Alarme désactivée");
            }
                    
                
                
                
                

                
            
          timer = 0;  
        }
    
            
    
    else {
        timer++;
        }
    }
        }
        