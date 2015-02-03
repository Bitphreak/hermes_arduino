
#include <PubNub.h>

#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "pitches.h"

char *pubkey = NULL;
char subkey[] = "YOUR_PUBNUB_SUB_KEY";
char channel[] = "hermes-gradle";

// notes in the melody:
int failMelody[] = { NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3, NOTE_B3, NOTE_C4};
int successMelody[] = { NOTE_C4, NOTE_B3, NOTE_C3, NOTE_B2, NOTE_G3, NOTE_C4};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int failDurations[] = {4, 8, 8, 4, 4, 4, 4 };
int successDurations[] = {4, 8, 4, 8, 4, 4 };
  
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// Change to 0 and uncomment IPAddress to use a static ip address
#define USE_DHCP 1
//IPAddress ip(10,0,0,101);

// Change these to match your board
const byte EthernetPin = 10;
const byte SdPin = 4;
const byte SsPin = 53;
const byte SpeakerPin = 3;

int totalNotes;
int *tones;
int *timings;

void setup(){
  pinMode(SpeakerPin, OUTPUT);
  
  // SS must be output and disable any device using SS
  pinMode(SsPin,OUTPUT);
  digitalWrite(SsPin,HIGH); //any device on the SS pin will be disabled

  // disable w5100
  pinMode(EthernetPin,OUTPUT);
  digitalWrite(EthernetPin,HIGH);   // disable W5100

  // disable SD
  pinMode(SdPin,OUTPUT);
  digitalWrite(SdPin,HIGH);  // disable SD
  
  delay(1000);
  
  #ifdef USE_DHCP
  while(Ethernet.begin(mac) == 0){
    delay(1);
  }
  #else
  Ethernet.begin(mac, ip);
  #endif
   
  PubNub.begin(pubkey, subkey);
}

void handleEvent(const char* const project, const char* const date, const char* const id, const int event, const int state){
  if(event == 4){
    if (state == 0){
      //BUILD SUCCEEDED
      timings = successDurations;
      tones = successMelody;
      totalNotes = sizeof(successMelody)/sizeof(successMelody[0]);
    }else{
      // BUILD FAILED
      timings = failDurations;
      tones = failMelody;
      totalNotes = sizeof(failMelody)/sizeof(failMelody[0]);
    }
    for (int i=0; i<totalNotes; i++) {
      int noteDuration = 1000/timings[i];
      tone(SpeakerPin, tones[i]); //, noteDuration
      
       // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * .30;      
      delay(noteDuration);
      noTone(SpeakerPin);
      delay(pauseBetweenNotes);
    }
    Serial.println("Done playing tone!");
    noTone(SpeakerPin);
  }


}

void loop(){
  #ifdef USE_DHCP
  // Maintain DHCP Lease
  Ethernet.maintain();
  #endif
  
  PubSubClient *client = PubNub.subscribe(channel);
  if(!client)
    return;

  char buffer[64];
  size_t buflen = 0;

  while(client->wait_for_data()){
    buffer[buflen++] = client->read();
  }
  buffer[buflen] = 0;
  client->stop();
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonArray& messages = jsonBuffer.parseArray(buffer);
  if(!messages.success())
    return;
  
  for (JsonArray::iterator it=messages.begin(); it!=messages.end(); ++it){
    if(it->is<JsonObject&>()){
      JsonObject& jsonObject = *it;
      const char* project = NULL;
      const char* date = NULL;
      const char* id = NULL;
      int event = -1;
      int state = -1;
      
      if(jsonObject.containsKey("event"))
        event = jsonObject["event"];
      if(jsonObject.containsKey("state"))
        state = jsonObject["state"];

      handleEvent(project, date, id, event, state);
    }else{
      continue;
    }
  }
  
  delay(1000);
}

