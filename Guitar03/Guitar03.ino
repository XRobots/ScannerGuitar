#include <usbhid.h>
#include <usbhub.h>
#include <hiduniversal.h>
#include <hidboot.h>
#include <SPI.h>
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

int barcode;
int prevBarcode;
int barcodeNew = 0;

int sw1;
int sw2;
int sw3;
int sw4;
int sw5;

int saveFlag1 = 0;
int saveFlag2 = 0;
int saveFlag3 = 0;

int joyUp;
int joyDown;
int joyLeft;
int joyRight;

int octave = 0;
int octaveFlag = 0;

int channel = 1;
int channelFlag = 0;
int channelFin;   // final channel selection

int save1 = 28;
int save2 = 33;
int save3 = 38;

int bend;
int bendPrev;

#define encoder0PinA 2      // encoder
#define encoder0PinB 3

volatile long encoder0Pos = 0;

class MyParser : public HIDReportParser {
  public:
    MyParser();
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
  protected:
    uint8_t KeyToAscii(bool upper, uint8_t mod, uint8_t key);
    virtual void OnKeyScanned(bool upper, uint8_t mod, uint8_t key);
    virtual void OnScanFinished();
};

MyParser::MyParser() {}

void MyParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  // If error or empty, return
  if (buf[2] == 1 || buf[2] == 0) return;

  for (uint8_t i = 7; i >= 2; i--) {
    // If empty, skip
    if (buf[i] == 0) continue;

    // If enter signal emitted, scan finished
    if (buf[i] == UHS_HID_BOOT_KEY_ENTER) {
      OnScanFinished();
    }

    // If not, continue normally
    else {
      // If bit position not in 2, it's uppercase words
      OnKeyScanned(i > 2, buf, buf[i]);
    }

    return;
  }
}

uint8_t MyParser::KeyToAscii(bool upper, uint8_t mod, uint8_t key) {
  // Letters
  if (VALUE_WITHIN(key, 0x04, 0x1d)) {
    if (upper) return (key - 4 + 'A');
    else return (key - 4 + 'a');
  }

  // Numbers
  else if (VALUE_WITHIN(key, 0x1e, 0x27)) {
    return ((key == UHS_HID_BOOT_KEY_ZERO) ? '0' : key - 0x1e + '1');
  }

  return 0;
}

void MyParser::OnKeyScanned(bool upper, uint8_t mod, uint8_t key) {
  uint8_t ascii = KeyToAscii(upper, mod, key);

  // ************** read barcode ****************

  barcode =  (int)ascii;            // get the integer for the ascii character in the bar code

  if (barcode >= 48 && barcode <= 57){      // fix the gaps tha do not have barcoe readable characters
    barcode = barcode - 8;
  }
  else if (barcode >=97){                   // fix the gaps tha do not have barcoe readable characters
    barcode = barcode - 47;
  }
  
  barcode = barcode + octave;       // add the octave shift on
  barcodeNew = 1;                   // set the flag so it plays the note on loop

  //************** deal with saving new notes to buttons
  //************** if the barcode is scanned while the switch is held down, it's value changes to that note

  if (sw3 == 0) {
    save1 = barcode;
  }
  else if (sw4 == 0) {
    save2 = barcode;
  }
  else if (sw5 == 0) {
    save3 = barcode;
  }
  
}

void MyParser::OnScanFinished() {
}

USB          Usb;
USBHub       Hub(&Usb);
HIDUniversal Hid(&Usb);
MyParser     Parser;

//******* setup ****************

void setup() {
  Serial.begin( 115200 );

  pinMode(2, INPUT_PULLUP);    // encoder pins
  pinMode(3, INPUT_PULLUP);
  
  pinMode(31, INPUT_PULLUP);    // joystick pins
  pinMode(33, INPUT_PULLUP);
  pinMode(35, INPUT_PULLUP);
  pinMode(37, INPUT_PULLUP);
  
  pinMode(30, INPUT_PULLUP);    // store notes
  pinMode(32, INPUT_PULLUP);    // store notes
  pinMode(34, INPUT_PULLUP);    // store notes
  pinMode(36, INPUT_PULLUP);    // button
  pinMode(38, INPUT_PULLUP);    // button
  

  attachInterrupt(0, doEncoderA, CHANGE);   // encoder intertupts
  attachInterrupt(1, doEncoderB, CHANGE); 
  
  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
  }

  delay( 200 );

  Hid.SetReportParser(0, &Parser);

      // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

    // Do the same for NoteOffs
    MIDI.setHandleNoteOff(handleNoteOff);

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();
}

// ************* turn all notes off on current channel

void allOff() {
  for (int i = 0; i <= 127; i++) {
    MIDI.sendNoteOff(i, 0, channelFin);
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{   
        MIDI.sendNoteOn(pitch, velocity, channel);    
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{    
        MIDI.sendNoteOff(pitch, velocity, channel); 
}

//*********** main loop *********************

void loop() {
  Usb.Task();

  sw1 = digitalRead(38);
  sw2 = digitalRead(36);

  joyRight = digitalRead(31);
  joyLeft = digitalRead(33);
  joyDown = digitalRead(35);
  joyUp = digitalRead(37);

  sw3 = digitalRead(32);
  sw4 = digitalRead(30);
  sw5 = digitalRead(34);

  // handle stored note buttons and debounce

  if (sw3 == 0 && saveFlag1 == 0) {
    barcode = save1;        // play the saved note
    barcodeNew = 1;         // say we have a new note to play
    saveFlag1 = 1;          // toggle that we have pressed the button
  }
  else if (sw3 == 1){       // check it botton is unpressed properly before allowing it's press function to hapen again.
    saveFlag1 = 0;
  }
  
  if (sw4 == 0 && saveFlag2 == 0) {
    barcode = save2;
    barcodeNew = 1;
    saveFlag2 = 1;
  }
  else if (sw4 == 1){
    saveFlag2 = 0;
  }
  
  if (sw5 == 0 && saveFlag3 == 0) {
    barcode = save3;
    barcodeNew = 1;
    saveFlag3 = 1;
  }
  else if (sw5 == 1){
    saveFlag3 = 0;
  }

  // ***** play the notes if there's a new scan or new button press

  if (barcodeNew == 1) {

      if (sw2 == 1) {
          MIDI.sendNoteOff(prevBarcode, 0, channelFin);     // Stop the previous note
      }  
      MIDI.sendNoteOn(barcode, 127, channelFin);          // play all the notes you scan
    
      prevBarcode = barcode;    // bookmark the previous barcode so we can tur the note off

      Serial.println(barcode);

      barcodeNew = 0;
  }

// deal with octave shift debounce

  if (joyRight == 0 && octaveFlag == 0) {
    octave = octave + 12;
    octaveFlag = 1;
  }
  else if (joyLeft == 0 && octaveFlag == 0) {
    octave = octave - 12;
    octaveFlag = 1;
  }
  else if (joyLeft == 1 && joyRight == 1) {
    delay(5);
    octaveFlag = 0;
  }
  octave = constrain(octave,-24,+24);         // add pr subtract 12 semitones to the octave variable

  // deal with channel shift debounce

  if (joyUp == 0 && channelFlag == 0) {
    channel = channel + 1;
    channelFlag = 1;
  }
  else if (joyDown == 0 && channelFlag == 0) {
    channel = channel - 1;
    channelFlag = 1;
  }
  else if (joyUp == 1 && joyDown == 1) {
    delay(5);
    channelFlag = 0;
  }
  
  channel = constrain(channel,1,3);

  if (channel >= 3) {
    channelFin = 10;
  }

  else {
    channelFin = channel;
  }

   // pitch bend

  encoder0Pos = constrain(encoder0Pos, 0,500);
  if (encoder0Pos < 100){
    bend = 8192;
  }
  else if (encoder0Pos >= 100) {
    bend = map(encoder0Pos, 100, 500, 8192,16383);
  }
  
  if (bend != bendPrev) {
    MIDI.sendPitchBend(bend,channelFin);
  }

  bendPrev = bend;

// ************** turn all notes off on that channel************
  if (sw1 == 0) {
    allOff();
  }

}   // end of main loop


//********************* interrupt service routines for encoder phases *********************

void doEncoderA(){  

  // look for a low-to-high on channel A
  if (digitalRead(encoder0PinA) == HIGH) { 
    // check channel B to see which way encoder is turning
    if (digitalRead(encoder0PinB) == LOW) {  
      encoder0Pos = encoder0Pos + 1;         // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;         // CCW
    }
  }
  else   // must be a high-to-low edge on channel A                                       
  { 
    // check channel B to see which way encoder is turning  
    if (digitalRead(encoder0PinB) == HIGH) {   
      encoder0Pos = encoder0Pos + 1;          // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;          // CCW
    }
  }
 
}

void doEncoderB(){  

  // look for a low-to-high on channel B
  if (digitalRead(encoder0PinB) == HIGH) {   
   // check channel A to see which way encoder is turning
    if (digitalRead(encoder0PinA) == HIGH) {  
      encoder0Pos = encoder0Pos + 1;         // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;         // CCW
    }
  }
  // Look for a high-to-low on channel B
  else { 
    // check channel B to see which way encoder is turning  
    if (digitalRead(encoder0PinA) == LOW) {   
      encoder0Pos = encoder0Pos + 1;          // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;          // CCW
    }
  }
  

}



