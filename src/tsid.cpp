#include "globals.h"
#include "display.h"
#include "arp.h"
#include "midi.h"
#include "mux.h"
#include "boot.h"
#include "isr.h"
#include "preset.h"
#include "sid.h"
#include "leds.h"
#include "paraphonic.h"
#include "lfo.h"
#include "sid.h"


 /*

no filter mode


/*/


#include <EEPROM.h>
#include <TimerOne.h>

#include <MIDI.h>













void setup(){
 
  for(int i=0;i<3;i++){
  filterEnabled[i]=1;
  }
  
 arpSpeedBase=100;
 delay(100);


 
   
  
  
 mydisplay.shutdown(0, false);  // turns on display 
 mydisplay.setIntensity(0, 1); // 15 = brightest


 pinMode(A0,INPUT);//mux inputs
 pinMode(A1,INPUT);//mux inputs
 pinMode(A2,INPUT);//mux inputs
 pinMode(A3,INPUT);//butt inputs
 pinMode(A4,INPUT);//butt inputs
 digitalWrite(A3,HIGH);
 digitalWrite(A4,HIGH);
 

 
  DDRB=255;//data port
  DDRC=255;//address port
  
   
   mux(9);
  
   mydisplay.shutdown(0, false);  // turns on display 
 mydisplay.setIntensity(0, 1); // 15 = brightest
 
if((PINA & _BV (4)) == 0){sendDump();}else{
  
  mux(3);if((PINA & _BV (4)) == 0){recieveDump();}
}

  pinMode(16,INPUT);//CV switch1
  pinMode(17,INPUT);//CV switch2
  pinMode(18,INPUT);//CV switch3

 digitalWrite(16,HIGH);
 digitalWrite(17,HIGH);
 digitalWrite(18,HIGH);
    
 
 pinMode(A7,INPUT);//gate
 digitalWrite(A7,HIGH);
 
  
 
   
 
  destiPitch1=destiPitch2=destiPitch3=1;

  
  
 




     
 mydisplay.shutdown(0, false);  // turns on display 
 mydisplay.setIntensity(0, 1); // 15 = brightest
 
 boot();
 
Timer1.initialize(100); // 
Timer1.attachInterrupt(isr); // attach the service routine here

 
  
   DDRD |= _BV (2); // SID1
   DDRD |= _BV (6); // SID2
   DDRD |= _BV (3); // LATCH

DDRC=B11111000;

  
    
  
sid[24]=B00010001;//Filter off full vol
 

sid[23]=B11111111;






  presetLast=EEPROM.read(3999);
  
  masterChannel=EEPROM.read(3998);if(masterChannel>16){masterChannel=1;}
  masterChannelOut=EEPROM.read(3997);if(masterChannelOut>16){masterChannelOut=1;}
  
  if(EEPROM.read(3996)>0){sendLfo=false;}else{sendLfo=true;}
  if(EEPROM.read(3995)>0){sendArp=false;}else{sendArp=true;}
  
  
  if(fatMode>3){fatMode=0;}
  if(fatMode==2){fat=15;}
else if(fatMode==3){fat=30;}

  if(presetLast>99)presetLast=99;
  preset=presetLast;

 pinMode(A6,OUTPUT);//reset
 digitalWrite(A6,LOW);
 delay(100);
 digitalWrite(A6,HIGH);

sidPitch(0,0);
sidPitch(1,0);
sidPitch(2,0);

 load(presetLast);presetLast=preset+1;
HandleNoteOn(masterChannel,1,0);
      
      
    
  
init1MhzClock();

MIDI.begin(MIDI_CHANNEL_OMNI);
}

byte x;


void loop(){
  
  if(fatShow){digit(0,12);digit(1,fatMode+1);fatShow=false;}
  
  readMidi();
  if(arpCounter>=arpSpeed+100){arpCounter=0;arpTick();}
  
  if(shape1PressedTimer>10000){shape1Pressed=false;shape1PressedTimer=0;pa=!pa;paraChange();}
  
  
  if(first){   
    
    if(millis()>800){
  //clone 1 everywhere
  digit(1,versionDecimal);digit(0,version);mydisplay.setLed(0,7,6,1);delay(1000);first=false;
  //for(int i=12;i<100;i++){preset=i;save();}
  }}
  

  if((dotTimer)&&(!first)){dotTimer--;if(!dotTimer){mydisplay.setLed(0,7,6,0);mydisplay.setLed(0,7,7,0);}}
  //pa mode
  
  //if(pa!=paLast){paLast=pa;paraChange();}

if((gate)&&(!pa)){mux(15);key=map(analogRead(A2),0,1023,12,72);}

  //CV
  //mux(2) and mux(3) both point to cv2?!!
  
  if((PINC & _BV (0)) == 0){cvActive[0]=true;mux(12);lfo[0]=analogRead(A2)>>2;}else{cvActive[0]=false;}
  if((PINC & _BV (1)) == 0){cvActive[1]=true;mux(2);lfo[1]=analogRead(A2)>>2;}else{cvActive[1]=false;}
  if((PINC & _BV (2)) == 0){cvActive[2]=true;mux(10);lfo[2]=analogRead(A2)>>2;}else{cvActive[2]=false;}

  
//cvActive[0]=false;
//cvActive[1]=false;
//cvActive[2]=false;

  if(jumble){load(1);jumble=0;}
  
  if((!saveMode)&&(presetLast!=preset)){load(preset);presetLast=preset;EEPROM.write(3999,presetLast);}
  if(saveBounce)saveBounce--;
if(frozen){frozen--;}


readMux();  sidUpdate();

lfoTick();  sidUpdate();
calculatePitch(); sidUpdate();
}
