/* sweep across a RGB LED Matrix
 Can work with either 16x32 or 32x64 Matrices, but make sure to wire up correctly
 For 32x64 Matrix wiring is (for Arduino Mega):
 G1=25      R1=24
 GND=GND    B1=26
 G2=28      R2=27
 GND=GND    B2=29
 B=A1       A=A0
 D=A3       C=A2
 LAT=10     CLK=11
 GND = GND  OE=9

 For 16x32 Matrix wiring is:
 G1=25      R1=24
 GND=GND    B1=26
 G2=28      R2=27
 GND=GND    B2=29
 B=A1       A=A0
 GND=GND    C=A2
 LAT=A3     CLK=11
 GND = GND  OE=9

 The program has three modes: pixel sweep, line sweep, and fill
 the speed can be changed by changing the potentiometer
 the mode can be changed by the switch



See: https://learn.adafruit.com/32x16-32x32-rgb-led-matrix/test-example-code

Download libraries from:

https://github.com/adafruit/RGB-matrix-Panel
and
https://github.com/adafruit/Adafruit-GFX-Library

*/

// Set up the RGB LED Matrix
// Taken from a number of Adafruit examples such as testshapes and testcolors
#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

char NAME[] = "PULSE-O-MATIC";
char VERSION[] = "0.1";
char CREATOR[] = "D. KAPLAN";

// define this for debugging output to serial port
// #define DEBUG

// define this to use sound output
#define SOUND

// set these to 0 or 1 to swap directions
#define X_DIR 1
#define Y_DIR 1

// use this for a 16x32 matrix
// if using the 32x64, leave it undefined
// #define Matrix16x32

// potentiometer for analog input
const int potPin1=A7;

// switches for digital input and to change the state of the sweep
const int switchAPin = 7;
const int switchBPin = 6;
// digital output for speaker
const int speakerPin = 12;


// pins for wiring the Matrix
#define CLK 11  // MUST be on PORTB! (Use pin 11 on Mega)
#define OE  9
#define A   A0
#define B   A1
#define C   A2

#ifdef Matrix16x32
  #define LAT A3
  RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);
#else
  #define LAT 10
  #define D   A3
  RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);
#endif


////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
////////////////////////////////////////////////////////////////////////////
// this is the built-in LED
const int ledPin=13;
// the types of sweeps possible
enum sweep { NONE, PIXEL, LINE };
// which type are we doing
sweep sweep_state;
// and which state were we in
sweep previous_state;
int sensorvalue;
// sweep delay in ms
int sweepdelay;
int previousdelay;
// how can we color things?
enum colormode { CONSTANT, PROGRESSIVE};

////////////////////////////////////////////////////////////////////////////
// Wheel()
// Input a value 0 to 24 to get a color value.
// The colours are a transition r - g - b - back to r.
// Taken from Adafruit testshapes_32x64
////////////////////////////////////////////////////////////////////////////
uint16_t Wheel(byte WheelPos) {
  if(WheelPos < 8) {
   return matrix.Color333(7 - WheelPos, WheelPos, 0);
  } else if(WheelPos < 16) {
   WheelPos -= 8;
   return matrix.Color333(0, 7-WheelPos, WheelPos);
  } else {
   WheelPos -= 16;
   return matrix.Color333(0, WheelPos, 7 - WheelPos);
  }
}

////////////////////////////////////////////////////////////////////////////
// checkIO()
// check the switch and potentiometer for input
////////////////////////////////////////////////////////////////////////////
void checkIO() {
  // read the value from the sensor:
  sweepdelay = analog2sweep(analogRead(potPin1));
  if (abs(sweepdelay - previousdelay) >= 5)
  {
    // if the change was large
    // use this to guard against small changes caused by ADC variability etc
    previousdelay=sweepdelay;
    #ifdef DEBUG
      Serial.print("Setting sweep delay to ");
      Serial.println(sweepdelay);
    #endif
  }
  digitalWrite(ledPin,digitalRead(switchAPin));
  if (digitalRead(switchAPin) == LOW)
  {
    sweep_state = LINE;
  }
  else if (digitalRead(switchBPin) == LOW)
  {
    sweep_state = PIXEL;
  }
  else {
    sweep_state = NONE;
  }
  
}


////////////////////////////////////////////////////////////////////////////
// analog2sweep()
// translate an analog sensor value [0,1024)
// to a sweep delay in ms
////////////////////////////////////////////////////////////////////////////
int analog2sweep(int analogvalue) {
  return analogvalue / 20;
}      

////////////////////////////////////////////////////////////////////////////
// TonePlayer class
////////////////////////////////////////////////////////////////////////////
class TonePlayer
{
  int period;

  ///////////////////
  public:
  TonePlayer()
  {
      period=0;
  }

  ///////////////////
  void Play(int periodtouse)
  {
    int frequency;
    period=periodtouse;
    // determine the frequency from the delaytime
    frequency=10000/period;
    tone(speakerPin, frequency, period);
  }
  
};
// initialize the tone player
TonePlayer toneplayer;



////////////////////////////////////////////////////////////////////////////
// generic Sweep class
// keeps track of the last pixel updated and waits until the appropriate time to do the next one
// should not be used on its own:
//  methods for TurnOn() and TurnOff() are virtual and need to be implemented in the derived classes
////////////////////////////////////////////////////////////////////////////
class Sweep
{
  protected:
    // current and last coordinates
    uint8_t x, y;
    uint8_t lastx, lasty;
    // this can be either a constant color or a progressive sweep that depends on the pixels
    colormode sweepcolormode;
    // for a constant color, these are the rgb values
    uint8_t pixelr, pixelg, pixelb;
    // last time a unit was set
    unsigned long previoustime;
    // is this instance on or off?
    boolean state;
    // should we update rows (y) as well as columns (x)?
    boolean updaterows;
    // factor to divide the delaytime for doing the sound timing
    int soundfactor;

  ///////////////////
  public:
  Sweep(colormode sweepcolormodetoset)
  {
      sweepcolormode=sweepcolormodetoset;
      // set default to WHITE
      pixelr=7;
      pixelg=7;
      pixelb=7;

      x=0;
      y=0;
      lastx=0;
      lasty=0;

      previoustime=0;

      // currently off
      state=false;

      // pixel by pixel
      updaterows=true;

      // length of the sound should be the length of a single unit
      soundfactor=1;
  }
  
  void setProgressive()
  {
    sweepcolormode=PROGRESSIVE;
  }

  void setColor(uint8_t r, uint8_t g, uint8_t b)
  {
    sweepcolormode=CONSTANT;
    
    pixelr=r;
    pixelg=g;
    pixelb=b;
  }

  ///////////////////
  void Update(int delaytime)
  {
    unsigned long currenttime = millis();
    if (currenttime - previoustime >= delaytime) {

      // turn on
      state=true;

      // call the derived turn-on method here.
      TurnOn();
      
      lastx=x;
      if (updaterows) 
      {
        lasty=y;
        y++;
        if (y==matrix.height()) {
          x++;
          y=0;
        }
      }
      else
      {
        x++;
      }
      if (x==matrix.width()) {
        x=0;
      }
      #ifdef SOUND
        if (y==matrix.height()/2 || !updaterows) {
          toneplayer.Play(delaytime / soundfactor);
        }
      #endif
      previoustime=currenttime;
    }
  }

  ///////////////////
  // Turn on the current 
  // this is virtual - needs to be sub-classed
  virtual void TurnOn(void) = 0;

  ///////////////////
  // Turn off the current and last unit
  // helps with state changes
  // this is virtual - needs to be sub-classed
  virtual void TurnOff(void) = 0;

};

////////////////////////////////////////////////////////////////////////////
// PixelSweep class
// sweeps across a single pixel 
// keeps track of the last pixel updated and waits until the appropriate time to do the next one
////////////////////////////////////////////////////////////////////////////
class PixelSweep : public Sweep
{

  ///////////////////
  public:
  PixelSweep(uint8_t r, uint8_t g, uint8_t b) : Sweep(CONSTANT)
  {
    setColor(r, g, b);
    soundfactor=1;
    updaterows=true;
  }

  ///////////////////
  void TurnOn()
  {
      matrix.drawPixel((matrix.width()-1)*(1-X_DIR) + x*(2*X_DIR-1), (matrix.height()-1)*(1-Y_DIR) + y*(2*Y_DIR-1), matrix.Color333(pixelr, pixelg, pixelb));
      // now previous pixel as black
      if (!((x == lastx) && (y == lasty))) {
        matrix.drawPixel((matrix.width()-1)*(1-X_DIR) + lastx*(2*X_DIR-1), (matrix.height()-1)*(1-Y_DIR) + lasty*(2*Y_DIR-1), matrix.Color333(0, 0, 0));
      }
  }

  ///////////////////
  // Just turn off the current and last pixel
  // helps with state changes
  void TurnOff()
  {
    if (state) {
      matrix.drawPixel((matrix.width()-1)*(1-X_DIR) + x*(2*X_DIR-1), (matrix.height()-1)*(1-Y_DIR) + y*(2*Y_DIR-1), matrix.Color333(0, 0, 0));
      matrix.drawPixel((matrix.width()-1)*(1-X_DIR) + lastx*(2*X_DIR-1), (matrix.height()-1)*(1-Y_DIR) + lasty*(2*Y_DIR-1), matrix.Color333(0, 0, 0));
      state=false;
    }
  }

};


////////////////////////////////////////////////////////////////////////////
// LineSweep class
// sweeps across a line 
// keeps track of the last line updated and waits until the appropriate time to do the next one
////////////////////////////////////////////////////////////////////////////
class LineSweep : public Sweep
{
  ///////////////////
  public:
  LineSweep() : Sweep(PROGRESSIVE)
  {
    soundfactor=4;
    updaterows=false;
  }

  ///////////////////
  void TurnOn()
  {
      // translate from x to color c
      int c = (24*x)/matrix.width();
      // draw a line
      matrix.drawLine((matrix.width()-1)*(1-X_DIR) + x*(2*X_DIR-1), 0, (matrix.width()-1)*(1-X_DIR) + x*(2*X_DIR-1), matrix.height()-1, Wheel(uint8_t(c)));
      // now draw previous one as black
      if (x != lastx) {
        matrix.drawLine((matrix.width()-1)*(1-X_DIR) + lastx*(2*X_DIR-1), 0, (matrix.width()-1)*(1-X_DIR) + lastx*(2*X_DIR-1), matrix.height()-1, matrix.Color333(0, 0, 0));
      }

  }

  ///////////////////
  // Just turn off the current and last line
  // helps with state changes
  void TurnOff()
  {
    if (state) {
      matrix.drawLine((matrix.width()-1)*(1-X_DIR) + x*(2*X_DIR-1), 0, (matrix.width()-1)*(1-X_DIR) + x*(2*X_DIR-1), matrix.height()-1, matrix.Color333(0, 0, 0));
      matrix.drawLine((matrix.width()-1)*(1-X_DIR) + lastx*(2*X_DIR-1), 0, (matrix.width()-1)*(1-X_DIR) + lastx*(2*X_DIR-1), matrix.height()-1, matrix.Color333(0, 0, 0));
      state=0;
    }
  }
};

////////////////////////////////////////////////////////////////////////////
// AllOn class
// Illuminates the whole matrix with a changing color
////////////////////////////////////////////////////////////////////////////
class AllOn : public Sweep
{
  ///////////////////
  public:
  AllOn() : Sweep(PROGRESSIVE)
  {
    soundfactor=4;
    updaterows=false;
  }

  ///////////////////
  void TurnOn()
  {
    int c = (24*x)/matrix.width();
    matrix.fillScreen(Wheel(uint8_t(c)));      
  
  }

  ///////////////////
  // Just turn off the matrix
  // helps with state changes
  void TurnOff()
  {
    matrix.fillScreen(matrix.Color333(0, 0, 0));
  }
};

////////////////////////////////////////////////////////////////////////////
// Identify()
// Displays a splash screen
////////////////////////////////////////////////////////////////////////////
void Identify()
{
  // text handling from https://github.com/adafruit/RGB-matrix-Panel/blob/master/examples/testshapes_32x64/testshapes_32x64.ino
  // draw some text!
  matrix.setTextSize(1);     // size 1 == 8 pixels high
  matrix.setTextWrap(true); // Don't wrap at end of line - will do ourselves

  matrix.setCursor(0, 0);    // start at top left, with 8 pixel of spacing
  uint8_t w = 0;
  for (w=0; w<sizeof(NAME); w++) {
    matrix.setTextColor(Wheel(w));
    if (w==8) {
      matrix.println();    
    }
    matrix.print(NAME[w]);
  }
  matrix.println();
    
  matrix.setTextColor(matrix.Color333(7,7,7));
  matrix.print("v. ");
  matrix.println(VERSION);
  matrix.setTextColor(matrix.Color333(2,2,2));
  matrix.println(CREATOR);
}

////////////////////////////////////////////////////////////////////////////
// initialize the pixelsweep and linesweep, along with the fill
PixelSweep pixelsweep(7, 7, 7);
LineSweep linesweep;
AllOn allon;

////////////////////////////////////////////////////////////////////////////
void setup() {
  // have an external LED also there for showing when things happen
  pinMode(ledPin, OUTPUT);
  // switch for changing modes
  pinMode(switchAPin, INPUT_PULLUP);  
  pinMode(switchBPin, INPUT_PULLUP);  

  matrix.begin();

  #ifdef DEBUG
    Serial.begin(9600);
    while (! Serial); // Wait untilSerial is ready - Leonardo
    #ifdef Matrix16x32
      Serial.println("Using a 16x32 Matrix");
    #else
      Serial.println("Using a 32x64 Matrix");
    #endif  
  #endif

  sweep_state=PIXEL;
  previous_state=PIXEL;
  previousdelay=0;

  Identify();
  delay(500);
}

////////////////////////////////////////////////////////////////////////////
void loop() {
  // depending on the state of the sweep variable
  // run it in either mode
  switch (sweep_state) {
    case PIXEL:
      pixelsweep.Update(sweepdelay);    
      break;
    case LINE:
      linesweep.Update(sweepdelay); 
      break;
    case NONE:
      allon.Update(sweepdelay);
      break;
  }
  if (sweep_state != previous_state) 
  {
    #ifdef DEBUG
      Serial.print("Previous state was ");
      Serial.println(previous_state);
      Serial.print("New state is ");
      Serial.println(sweep_state);
    #endif
    // did we just switch
    switch (previous_state) {
      case PIXEL:
        pixelsweep.TurnOff();
        break;
      case LINE:
        linesweep.TurnOff();
        break;
      case NONE:
        allon.TurnOff();
        break;
    }
    previous_state=sweep_state;
  }
  checkIO();
}


