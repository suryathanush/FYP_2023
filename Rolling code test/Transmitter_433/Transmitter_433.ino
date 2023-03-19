#define DEBUG_ENABLED
#include <avr/eeprom.h>
#include <VirtualWire.h>
#include <AESLib.h>
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
    int magic;
} PairingData;
typedef struct {
    long counter;
    unsigned long serial;
    uint8_t skey[16];
} Settings;

#define SERIAL_NUMBER 123456
#define PMAGIC 555
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};  //specific key for the fob

#define PAIR_BUTTON 8
#define TX_PIN 7
#define SEND_BUTTON 9

PackedData sData;
PairingData pData;

unsigned char msg[16];
long key_counter;

void pair(void) {
    long start =millis();
    int ledState = LOW;
    long previousMillis = 0; // will store last time LED was updated
    do{
        //sent the serial number and counter for syncing
        int len=sizeof(pData);
        vw_send((uint8_t *)&pData, len); // Send via rf
        vw_wait_tx();
        delay(1000);
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
void setup() {
    #ifdef DEBUG_ENABLED
    Serial.begin(9600);
    #endif
   pinMode(PAIR_BUTTON,INPUT_PULLUP); //switch for pairing
   pinMode(SEND_BUTTON,INPUT_PULLUP); //switch for sending
    pinMode(LED_BUILTIN,OUTPUT);
    vw_set_ptt_inverted(true); //
    vw_set_tx_pin(TX_PIN);
    vw_setup(4000);// speed of data transfer Kbps
   //read the stored counter & eepromdata
   eeprom_read_block((void*)&pData, (void*)0, sizeof(pData));
   //if serial number is not set
   if(pData.serial!=SERIAL_NUMBER)
    {
     //in case eeprom is empty/garbage as in first try
     pData.counter=1000;//random(10000,99999);
     pData.magic=PMAGIC;
     pData.serial=SERIAL_NUMBER;
    }
      sData.code=PMAGIC;
      sData.serial=SERIAL_NUMBER;
}
void loop(){
    if (digitalRead(PAIR_BUTTON)==0)
    {
        //pairing loop
        pair();
    }
 
   if (digitalRead(SEND_BUTTON)==0)
      {
          //send the command 
          sender();
      }
   
      delay(1000);
}

void sender(){
    //increment the counter
    pData.counter=pData.counter+1;
    sData.counter=pData.counter;
    //save the counter
    eeprom_write_block((const void*)&pData, (void*)0, sizeof(pData));     
    // Set the commands based on key
    // for testing i kept it 'a' but do a loop with analogue/digital read to get key state    
    sData.command='a';
    digitalWrite(LED_BUILTIN,1);
    int len = sizeof(sData);
    aes128_enc_single(key, &sData);
    #ifdef DEBUG_ENABLED
    Serial.print("Encrypted:");
    memcpy(msg, &sData, len);
    for(int k=0;k<16;k++)
    Serial.print(msg[k],HEX);
    Serial.println("");
    Serial.print("KEY:");
    for(int k=0;k<len;k++)
    Serial.print(key[k],HEX);
    Serial.println("");
    #endif
    vw_send((uint8_t *)&sData, len); // Send the 16 bytes
    vw_wait_tx();
    aes128_dec_single(key, &sData);
    PackedData * sdData = (PackedData *)&sData;
    #ifdef DEBUG_ENABLED
    Serial.print("Test Decrypt:");
    Serial.print(sdData->serial);
    Serial.println("");
    #endif
    digitalWrite(LED_BUILTIN,0); 
}
//END
