//////Editor Specs /////
let checkboxCount=8;

//////TE LOGO STUFF /////
let centerX, centerY; // the center point of the animation
let angle = 6; // starting angle of the white spots
let speedT = .2; // speed of rotation
let circleRadius = 100; // radius of the circle
let frequency = 2;
//////END TE LOGO STUFF /////

let noIO=0;
let foundDevice=0;
let firmware1=0;
let firmware2=0;
let deviceInNumber=-1;
let deviceOutNumber=-1;
let noDevices=0;
let magic;
let midiIN;//input device
let midiOUT;//output device


let button1;
let button2;

let byte = 0;
let checkboxes = [];

let dataArray = [];

let drop1;let drop2;let drop3;let drop4;let drop5;let drop6;
let drop7;let drop8;let drop9;let drop10;let drop11;let drop12;




function setup(){
  

  checkboxes.push(createCheckbox('').changed(updateBits));
  checkboxes.push(createCheckbox('').changed(updateBits));
 checkboxes.push(createCheckbox('').changed(updateBits));  checkboxes.push(createCheckbox('').changed(updateBits));
  checkboxes.push(createCheckbox('').changed(updateBits));
  checkboxes.push(createCheckbox('').changed(updateBits));
checkboxes.push(createCheckbox('').changed(updateBits));
checkboxes.push(createCheckbox('').changed(updateBits));
  
centerX = width / 2;
  centerY = height / 2;
  angleMode(DEGREES); // use degrees instead of radians for angle calculations
  
 
  
  drop1 = createSelect();
  for (let i = 1; i < 17; i++) {
    let option = i;
    drop1.option(option);
  }drop1.changed(getSelectedNote);
   drop1.position(220+75, -155+175+70); 
  
  drop2 = createSelect();
  for (let i = 1; i < 17; i++) {
    let option = i;
    drop2.option(option);
  }drop2.changed(getSelectedNote);
   drop2.position(220+75, -155+195+70); 
  
   drop3 = createSelect();
  for (let i = 1; i < 17; i++) {
    let option = i;
    drop3.option(option);
  }drop3.changed(getSelectedNote);
   drop3.position(220+75, -155+215+70); 
  
   drop4 = createSelect();
  for (let i = 1; i < 17; i++) {
    let option = i;
    drop4.option(option);
  }drop4.changed(getSelectedNote);
   drop4.position(220+75, -155+235+70); 
  
     drop5 = createSelect();
  for (let i = 1; i < 17; i++) {
    let option = i;
    drop5.option(option);
  }drop5.changed(getSelectedNote);
   drop5.position(220+75, -155+275+70); 
  
 
    drop6 = createSelect();
  for (let i = 2; i < 17; i++) {
    let option = int(i*6.25)+'%';
    drop6.option(option);
  }drop6.changed(getSelectedNote);
   drop6.position(220+75+150, -155+175+70); 
  
      
    drop7 = createSelect();
  for (let i = 1; i < 49; i++) {
    let option = i;
    drop7.option(option);
  }drop7.changed(getSelectedNote);
   drop7.position(250+75+150+20, -155+195+90); 
  
   drop8 = createSelect();
  for (let i = 1; i < 49; i++) {
    let option = i;
    drop8.option(option);
  }drop8.changed(getSelectedNote);
   drop8.position(250+75+150+20, -155+195+110); 
  
  // Request MIDI access
navigator.requestMIDIAccess({ sysex: true })
  .then(function(access) {
    // Get the MIDI input and output devices
    const inputDevices = Array.from(access.inputs.values());
    const outputDevices = Array.from(access.outputs.values());
    if (inputDevices.length === 0 || outputDevices.length === 0) {
      console.error('No MIDI input or output devices found');noIO=1;
      return;
    }

    
});
createCanvas(800, 350);
  
   
    button1 = createButton('Read');
  button1.position(230, 30);
  button1.mousePressed(readButton);
  
   button2 = createButton('Write');
  button2.position(530, 30);
  button2.mousePressed(writeButton);
 
  
   midiOut = WebMidi.outputs[0];
   //pulldown for MIDI I/O device selection 
  selI = createSelect();
  selI.position(280, 32);
    WebMidi.inputs.forEach((device, index) => {
               selI.option(device.name);
     });
         
   selO = createSelect();
  selO.position(579, 32);
  WebMidi.outputs.forEach((device, index) => {
               selO.option(device.name);
     });  
     
}

function draw(){


  
  checkboxes[0].position(5,90);
  checkboxes[1].position(5,110);
  checkboxes[2].position(5,130);
  checkboxes[3].position(5,150);
  checkboxes[4].position(5,170);
  checkboxes[5].position(5,190);
  checkboxes[6].position(5,210);
checkboxes[7].position(5,230);
  
  background(1);
  

   //////  TE LOGO //////////
  fill(180); // white fill color
  noStroke(); // no stroke

   push();
  scale(0.4);translate(1800, 350);
   let amplitude = 150;
  speed = (amplitude * sin(TWO_PI * frequency * millis() / 1000) + amplitude)/600;
 
  // draw the spinning white spots
   for (let i = 0; i < 300; i += 20) {
    let x = centerX + cos(angle + i) * circleRadius*1.3;
    let y = centerY + sin(angle + i) * circleRadius*1.5;
    ellipse(x, y, 15, 15);
  }
    for (let i = 0; i < 300; i += 20) {
    let x = centerX + cos(180+angle + i) * -circleRadius;
    let y = centerY + sin(180+angle + i) * circleRadius;
    ellipse(x, y, 12, 12);
  }
    for (let i = 0; i < 300; i += 20) {
    let x = centerX + cos(angle + i) * circleRadius/1.3;
    let y = centerY + sin(angle + i) * circleRadius/1.5;
    ellipse(x, y, 4, 4);
  }
  angle += speedT; // update the angle for the next frame
    pop();
  ////// END TE LOGO //////////
  
    if(foundDevice){
      
      button2.removeAttribute('disabled');
    fill(150, 150, 200);
    text('TherapSID detected!', 8, 45);
      

    text('firmware: '+firmware1+'.'+firmware2, 8, 60);
  }else{
    
    button2.attribute('disabled', false);
    fill(150, 150, 200);
    text('This Tool requires', 8, 45);
    text('firmware 2.4 or above!', 8, 60);
    
   if(noIO==1){
    button1.attribute('disabled', false);
     fill(150, 0, 0);
     text('No MIDI devices detected! This tool requires MIDI in and out!', 8, 280);
   }else{
      button1.removeAttribute('disabled');
   }
         
  }
   fill(150, 150, 150);
  textSize(16);
  

  
text('MIDI OUTPUT (to device):', 530, 25);
text('TherapSID Tool V1.1                MIDI INPUT (from device):', 8, 25);
  

  text('MIDI CHANNELS:', -180+400+5, 170+70-155);
  text('MASTER', -180+400+5, 190+70-155);
  text('VOICE1                                    PITCH BEND RANGE', -180+400+5, 210+70-155);
  text('VOICE2                                    UP', -180+400+5, 230+70-155);
  text('VOICE3                                    DOWN', -180+400+5, 250+70-155);
  text('OUTPUT', -180+400+5, 290+70-155);
  
  text('OPTIONS', 10, 170+70-155);
  text('MODWHEEL > LFO1', -375+400+5, 190+70-155);
  text('AFTERTOUCH > LFO2', -375+400+5, 210+70-155);
  text('VELOCITY > LFO3', -375+400+5, 230+70-155);
  text('LFO SENDS CC', -375+400+5, 250+70-155);
  text('ARP SENDS NOTES', -375+400+5, 270+70-155);
  text('PW LIMITER', -375+400+5, 290+70-155);
  text('ARMSID MODE', -375+400+5, 310+70-155);
  text('NO ARP ON 1 KEY', -375+400+5, 310+90-155);
  
  text('MASTER VOL:', -180+400+5+220, 170+70-155);

  
}


function getSelectedNote() {
  let selectedNote;
   selectedNote = drop1.value();console.log("0");
   selectedNote = drop1.value();console.log("1");
  let noteNumber = parseInt(selectedNote.split(" ")[1]); // Extract the note number from the selected option
}

function writeButton(){
  
midiOUT = WebMidi.getOutputByName(deviceOutNumber);
  //Send all the settings back to device 
  let channel = midiOUT.channels[16];

  channel.sendControlChange(90, drop1.elt.selectedIndex);
  channel.sendControlChange(94, drop2.elt.selectedIndex);//voice1
  channel.sendControlChange(95, drop3.elt.selectedIndex);//voice2
  channel.sendControlChange(96, drop4.elt.selectedIndex);//voice3
  channel.sendControlChange(91, drop5.elt.selectedIndex);//output
  channel.sendControlChange(89, 1+drop6.elt.selectedIndex);//volume
  
  channel.sendControlChange(99, drop7.elt.selectedIndex);//volume
  channel.sendControlChange(100, drop8.elt.selectedIndex);//volume
  
  channel.sendControlChange(88, int(checkboxes[5].checked()));
  
  channel.sendControlChange(85, int(checkboxes[0].checked()));
  channel.sendControlChange(86, int(checkboxes[1].checked()));
  channel.sendControlChange(87, int(checkboxes[2].checked()));
  channel.sendControlChange(92, int(checkboxes[3].checked()));
  channel.sendControlChange(93, int(checkboxes[4].checked()));
  channel.sendControlChange(97, int(checkboxes[6].checked()));
  channel.sendControlChange(101, int(checkboxes[7].checked()));
  //channel.sendControlChange(98, int(checkboxes[7].checked()));//quantized

   
  
}

function readButton(){
  // ENABLE LISTENER on input device 
  //send magic NoteOFF 19.   82 on ch16 to make device spit out its settings
  //                    |note |vel
 
   if(!firstTime){midiIN.removeListener();}
  deviceInNumber=selI.value();
  deviceOutNumber=selO.value();
  console.log("IN:"+deviceInNumber);
  console.log("OUT:"+deviceOutNumber);
  if((deviceInNumber==0)&&(deviceOutNumber==0)){noDevices=1;}
  
   midiIN = WebMidi.getInputByName(deviceInNumber);
midiIN.addListener('controlchange', "16", HandleControlChange);
  firstTime=0;
  
    midiOUT = WebMidi.getOutputByName(deviceOutNumber);

let channel = midiOUT.channels[16];
  //magic numbers
  channel.sendControlChange(19, 82);
  channel.sendControlChange(19, 82);
  magic=true;
}

let firstTime=1;

function HandleControlChange(e){
 if(magic){
   let note=e.controller.number;
   let vel=e.rawValue;
   
  switch(note){
      
  case 1:if(vel==2){foundDevice=1;}break;//product ID TherapSID is 2
  case 2:firmware1=vel;break;//Firmware digit 1
  case 3:firmware2=vel;break;//Firmware digit 2
  
  case 90:if(vel<16){drop1.elt.selectedIndex = vel-1;}break;//master channel
  case 94:if(vel<16){drop2.elt.selectedIndex = vel-1;}break;//voice1
  case 95:if(vel<16){drop3.elt.selectedIndex = vel-1;}break;//voice2
  case 96:if(vel<16){drop4.elt.selectedIndex = vel-1;}break;//voice3
  case 91:if(vel<16){drop5.elt.selectedIndex = vel-1;}break;//out channel
  case 89:if(vel<16){drop6.elt.selectedIndex = vel-1;}break;//volume
  
  case 99:if(vel<49){drop7.elt.selectedIndex = vel-1;}break;//pitchUP
  case 100:if(vel<49){drop8.elt.selectedIndex = vel-1;}break;//pitchDown
  
  case 85:if(vel>0){checkboxes[0].checked(true);}else{checkboxes[0].checked(false);}break;
  case 86:if(vel>0){checkboxes[1].checked(true);}else{checkboxes[1].checked(false);}break;
  case 87:if(vel>0){checkboxes[2].checked(true);}else{checkboxes[2].checked(false);}break;
  case 92:if(vel>0){checkboxes[3].checked(true);}else{checkboxes[3].checked(false);}break;
  case 93:if(vel>0){checkboxes[4].checked(true);}else{checkboxes[4].checked(false);}break;
  case 88:if(vel>0){checkboxes[5].checked(true);}else{checkboxes[5].checked(false);}break;
  case 97:if(vel>0){checkboxes[6].checked(true);}else{checkboxes[6].checked(false);}break; 
  case 101:if(vel>0){checkboxes[7].checked(true);}else{checkboxes[7].checked(false);}break; 
 
 //case 17:if(vel<16){drop7.elt.selectedIndex = vel-1;}break;//tune range
 
  }
 
  
}}

function updateBits() {
  
}





