/*******************************************************
This program will sample a 50-150hz signal depending on ethanol
content, and output a 0-5V signal via PWM.
The LCD (for those using an Arduino Uno + LCD shield) will display ethanol content, hz input, mv output, fuel temp

Connect PWM output to TGV. 3.3kOhm resistor works fine.

Input pin 8 (PB0) ICP1 on Atmega328
Output pin 3 or 11, defined below


********************************************************/

// include the library code:
//Changes to use 0.96 oled screen //V-tsan
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, 4, 800000UL, 100000UL);

// FACE CODE HERE

static const unsigned char PROGMEM pointy[] ={
0x60, 0x06, 0xf0, 0x0f, 0xf8, 0x1f, 0x7c, 0x3e, 0x3e, 0x7c, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0,
0x03, 0xc0, 0x01, 0x80,
};

/* Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Pins:
 * GND = GND
 * VCC = 5V
 * SCL = A5
 * SDA = A4
  */


int inpPin = 8;     //define input pin to 8
int outPin = 11;    //define PWM output, possible pins with LCD and 32khz freq. are 3 and 11 (Nano and Uno)

//Define global variables
volatile uint16_t revTick;    //Ticks per revolution
uint16_t pwm_output  = 0;     //integer for storing PWM value (0-255 value)
int HZ;                   //unsigned 16bit integer for storing HZ input
int ethanol = 0;              //Store ethanol percentage here
float expectedv;              //store expected voltage here - range for typical GM sensors is usually 0.5-4.5v

int duty;                     //Duty cycle (0.0-100.0)
float period;                 //Store period time here (eg.0.0025 s)
float temperature = 0;        //Store fuel temperature here
int fahr = 0;
int cels = 0;
static long highTime = 0;
static long lowTime = 0;
static long tempPulse;

void setupTimer()   // setup timer1
{
  TCCR1A = 0;      // normal mode
  TCCR1B = 132;    // (10000100) Falling edge trigger, Timer = CPU Clock/256, noise cancellation on
  TCCR1C = 0;      // normal mode
  TIMSK1 = 33;     // (00100001) Input capture and overflow interupts enabled
  TCNT1 = 0;       // start from 0
}

ISR(TIMER1_CAPT_vect)    // PULSE DETECTED!  (interrupt automatically triggered, not called by main program)
{
  revTick = ICR1;      // save duration of last revolution
  TCNT1 = 0;       // restart timer for next revolution
}

ISR(TIMER1_OVF_vect)    // counter overflow/timeout
{ revTick = 0; }        // Ticks per second = 0

bool restDisplay = false; //oled resync

void setup()
{
  Serial.begin(9600);
  pinMode(inpPin,INPUT);
  setPwmFrequency(outPin,1); //Modify frequency on PWM output
  setupTimer();

  // face setup here
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setRotation(0);// Rotate the display at the start:  0, 1, 2 or 3 = (0, 90, 180 or 270 degrees)
  display.clearDisplay();
  // Initial screen formatting
}

void loop()
{
  getfueltemp(inpPin); //read fuel temp from input duty cycle

  if (revTick > 0) // Avoid dividing by zero, sample in the HZ
    {HZ = 62200 / revTick;}     // 3456000ticks per minute, 57600 per second
    else
    {HZ = 0;}                   //needs real sensor test to determine correct tickrate

  //calculate ethanol percentage
    if (HZ > 50) // Avoid dividing by zero
    {ethanol = (HZ-50);}
    else
    {ethanol = 0;}

if (ethanol > 99) // Avoid overflow in PWM
{ethanol = 99;}

  expectedv = ((((HZ-50.0)*0.01)*4)+0.5);
  //Screen calculations
  pwm_output = 1.1 * (255 * (expectedv/5.0)); //calculate output PWM for ECU


  //PWM output
  analogWrite(outPin, pwm_output); //write the PWM value to output pin
  delay(100);  //make screen more easily readable by not updating it too often



//FACE LOOP here
  ethanolScreen();
  delay(1000);
  boing();
  boing();

  ethanolScreen ();
  delay(1000);

  drawOpeneyes(22, 26);
  delay(1000);

  drawClosedyes(22);
  delay(50);

  drawOpeneyes(22, 26);
  delay(1000);

  drawNarrowMouth(16, 16);
  delay(1000);
  delay(1000);

  drawOpeneyes(22, 26);
  delay(1000);

  drawClosedyes(22);
  delay(50);

  drawOpeneyes(22, 26);
  delay(1000);

  ethanolScreen ();
  delay(1000);

}

void getfueltemp(int inpPin){ //read fuel temp from input duty cycle
highTime = 0;
lowTime = 0;

tempPulse = pulseIn(inpPin,HIGH);
  if(tempPulse>highTime){
  highTime = tempPulse;
  }

tempPulse = pulseIn(inpPin,LOW);
  if(tempPulse>lowTime){
  lowTime = tempPulse;
  }

duty = ((100*(highTime/(double (lowTime+highTime))))); //Calculate duty cycle (integer extra decimal)
float T = (float(1.0/float(HZ)));             //Calculate total period time
float period = float(100-duty)*T;             //Calculate the active period time (100-duty)*T
float temp2 = float(10) * float(period);      //Convert ms to whole number
temperature = ((40.25 * temp2)-81.25);        //Calculate temperature for display (1ms = -40, 5ms = 80)
int cels = int(temperature);
cels = cels*0.1;
float fahrtemp = ((temperature*1.8)+32);
fahr = fahrtemp*0.1;

}

void setPwmFrequency(int pin, int divisor) { //This code snippet raises the timers linked to the PWM outputs
  byte mode;                                 //This way the PWM frequency can be raised or lowered. Prescaler of 1 sets PWM output to 32KHz (pin 3, 11)
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}


void drawOpeneyes(int eyeWidth, int eyeHeight) {
  display.clearDisplay();
  display.fillRoundRect(0, 20, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
  display.fillRoundRect(display.width()-eyeWidth, 20, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
  display.fillRoundRect(display.width()/2-20, display.height()-10, 40, 10, 5, WHITE);
  display.display();
}

void drawClosedyes(int eyeWidth) {
  display.clearDisplay();
  display.fillRoundRect(0, 28, eyeWidth, 6, 3, WHITE);
  display.fillRoundRect(display.width()-eyeWidth, 28, eyeWidth, 6, 3, WHITE);
  display.fillRoundRect(display.width()/2-20, display.height()-10, 40, 10, 5, WHITE);
  display.display();
}

void drawNarrowMouth(int eyeWidth, int eyeHeight) {
  display.clearDisplay();
  display.fillRoundRect(0, 20, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
  display.fillRoundRect(display.width()-eyeWidth, 20, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
  display.fillRoundRect(display.width()/2-35, display.height()-10, 70, 10, 5, WHITE);
  display.display();
}

void boing() {
  int eyeWidth = 22, eyeHeight = 26, animSpeed = 4, eyeStretch = 10;

  int eyePosY;


  for(int i = -eyeStretch ; i <= 50-26; i+=animSpeed){
    eyePosY = max(i, 0);

    display.clearDisplay();
    display.fillRoundRect(0, eyePosY, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
    display.fillRoundRect(display.width()-eyeWidth, eyePosY, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
    display.drawBitmap(display.width()/2-8, display.height()-20, pointy, 16, 10, WHITE);
    display.display();


    if(i>=64-eyeWidth-eyeStretch) eyeHeight -= animSpeed;
    else if(i<0) eyeHeight += animSpeed;
  }


  for(int i = 50-26; i > -eyeStretch; i-=animSpeed){
    eyePosY = max(i, 0);

    display.clearDisplay();
    display.fillRoundRect(0, eyePosY, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
    display.fillRoundRect(display.width()-eyeWidth, eyePosY, eyeWidth, eyeHeight, eyeWidth/2, WHITE);
    display.drawBitmap(display.width()/2-8, display.height()-20, pointy, 16, 10, WHITE);
    display.display();


    if(i>64-eyeWidth-eyeStretch) eyeHeight += animSpeed;
    else if(i<=0) eyeHeight -= animSpeed;
  }

}
void ethanolScreen() {
  display.setTextWrap(false);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Ethanol:    %");
  display.setCursor(50, 0);
  display.print(ethanol);
  display.display();
  display.setCursor(0,50);
  display.print("     Hz       C");
  display.setCursor(20, 50);
  display.print(HZ);
  display.setCursor(50, 50);
  display.print(temperature); //Use this for celsius
  display.display();
  Serial.println(ethanol);
  Serial.println(pwm_output);
  Serial.println(expectedv);
  Serial.println(HZ);
  delay(1000);
}
