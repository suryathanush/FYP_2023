#include <avr/eeprom.h>
#include <LiquidCrystal.h>
#include <VirtualWire.h>
#include <AESLib.h>

const int SAFEWINDOW = 50;
long lastcount;
char *controller;
unsigned char msg[16];
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

#pragma pack 1
typedef struct {
    unsigned long serial;
    long counter;
    char command;
    long extra;
    int code;
    char termin;
} PackedData;

typedef struct {
    long counter;
    unsigned long serial;
    uint8_t skey[16];
} Settings;

typedef struct {
    long counter;
    unsigned long serial;
    int magic;
} PairingData;

Settings StoredSettings;
int errorcounter=0;

#define PAIR_BUTTON 8
#define RX_PIN A5

void setup()
{
    Serial.begin(9600);
    pinMode(PAIR_BUTTON,INPUT_PULLUP);
    //memcpy (&StoredSettings.skey, &key[0], sizeof(key));
    //eeprom_write_block((const void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
    delay(1000);               
    eeprom_read_block((void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
    //memcpy (&key[0],&StoredSettings.skey, sizeof(key));
    Serial.print("EEPROMKEY:");
    for(int k=0;k<16;k++)
    Serial.print(StoredSettings.skey[k],HEX);
    Serial.println("");
    Serial.println(StoredSettings.counter); // print a simple message
    Serial.print("SN: "); // print a simple message
    Serial.println(StoredSettings.serial);
    pinMode(2, OUTPUT);
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_set_rx_pin(RX_PIN);
    vw_setup(4000); // Bits per sec
    vw_rx_start();
}

void pairing(void)
{
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    Serial.println(" Pairing! "); 
    long start =millis();
    int ledState = LOW;
    long previousMillis = 0; // will store last time LED was updated
    do{
        //do pairing process and store serial and counter
        if (vw_get_message(buf, &buflen)) // Non-blocking
        {
      
        PairingData * pairData = (PairingData *)buf; 
           
            if(pairData->magic==555){
                Serial.println(pairData->serial);
                //Serial.println();
                Serial.println(pairData->counter);
                //Serial.println();
                StoredSettings.counter=pairData->counter;
                StoredSettings.serial=pairData->serial;
                eeprom_write_block((const void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
                delay(2000);
                Serial.println(" Done!");
                Serial.println(" Synced");
                delay(2000);
                break;
            }
            else
            {
                Serial.println(" Error!");
                Serial.println("Try again!");
            }
        }
        delay(100);
        //end of pairing
        unsigned long currentMillis = millis();
        if(currentMillis - previousMillis > 100) {
            previousMillis = currentMillis;
            if (ledState == LOW)
            ledState = HIGH;
            else
            ledState = LOW;
            digitalWrite(LED_BUILTIN, ledState);
        }
    } while ((millis()-start)<10000); //for 5 seconds
}
void loop()
{


   if (digitalRead(PAIR_BUTTON)==0)
    {
   pairing();
    }
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
        Serial.print("Encrypted:");
        memcpy(msg, buf, buflen);
        for(int k=0;k<16;k++)
        Serial.print(msg[k],BIN);
        Serial.println("");
        Serial.print("KEY:");
        for(int k=0;k<buflen;k++)
        Serial.print(key[k],HEX);
        Serial.println("");
        aes128_dec_single(key, msg);
        PackedData * sdData = (PackedData *)msg;
        Serial.print("Test Decrypt:");
        Serial.print(sdData->serial);
        Serial.println("");
        Serial.println(sdData->counter);
        
        digitalWrite(LED_BUILTIN,HIGH); //for red led
        long currentcounter;
        Serial.println(sdData->code);
        if(sdData->code==555){
            //do the job if the counter is with in safety window
            currentcounter= sdData->counter;
            if((currentcounter-StoredSettings.counter)<SAFEWINDOW && (currentcounter-StoredSettings.counter)>0)
            {
                Serial.println("Access Granted!");
                StoredSettings.counter=sdData->counter;
                eeprom_write_block((const void*)&StoredSettings, (void*)0, sizeof(StoredSettings));
                //do the stuff here for eg running the servo or door lock motor
                char command = sdData->command;
                switch (command) {
                        case 'a':
                              Serial.println("Code sent : A"); 
                              //do the servo here                              
                              break;
                        case 'b':
                              Serial.println("Code sent : B"); 
                              //do the servo here     
                              break;
                       }
                
            }
          else
            {
                errorcounter++;
                Serial.println("Needs Pairing!");
                Serial.println(errorcounter);
                Serial.println(StoredSettings.counter);
                Serial.println(currentcounter);               
            } 
        }
        else
        {
            Serial.println(" Error!");
            Serial.println(" Wrong Key??");
        }
        delay(100);
        digitalWrite(2,0);
    }
    delay(100); //just here to slow down a bit

}
