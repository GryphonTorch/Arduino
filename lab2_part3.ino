// EENG 348 Lab 2 Part 3
// Sets a 5x7 LED matrix to display digits "0" to "9", using two 74HC595 shift registers
// DigitalWrite and PinMode are allowed in this part

// Video uploaded to YouTube at: https://www.youtube.com/watch?v=hbZ73638zbY 
// and https://www.youtube.com/watch?v=fJ7eu84kG8E 
/*  ELECTRICAL CONNECTIONS 
 *  Sixteen-bit output after Daisy chained shift registers, according to:
 *  Col1,Col2,Col3,Col4,Col5,X,X,X,X,Row1,Row2,Row3,Row4,Row5,Row6,Row7,Row8
 *  where Col, Row refers to LED matrix pins and X is do-not-care value.

 *  Shift Register 1,2 pins --> LED Matrix mapping (where S.R. 1 is directly 
 *  connected to Arduino):
 *    VA1,VB1,VC1,VD1,VE1,VF1,VG1,VH1,VA2,VB2,VC2,VD2,VE2,VF2,VG2,VH2
 * -->  1,  3, 10,  7,  8,  X,  X,  X,  X, 12, 11,  2,  9,  4,  5,  6
*/

// Define Arduino pins
const byte latchPin = 8;       // Latch pin of 74HC595 is connected to Digital pin 5
const byte clockPin = 12;      // Clock pin of 74HC595 is connected to Digital pin 6
const byte dataPin = 11;       // Data pin of 74HC595 is connected to Digital pin 4
const byte interruptPin = 2;   // Button connected to PD2
const int loopTime = 10E11;     // how long each digit loops

// Volatile variables (want interrupt to change, anytime)
volatile byte digitState = 0;        // For which digit to display. Global variable that ISR will change
volatile int counter = 0;            // global variable for interrupt to access
volatile bool buttonPushed = false;  // global flag

void setup(){
  //Serial.begin(9600);
  //pinMode(interruptPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), nextISR, FALLING);  // NEXT is our ISR

  
  Serial.begin(9600);
  pinMode(latchPin, OUTPUT);          // pinMode okay for part 3 of lab
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), nextISR, FALLING);  
  // nextISR is our interrupt service routine, defined at the end

  digitState = 8;   // press to start
}

void loop() 
{ 
  if(buttonPushed == true)
  { 
    buttonPushed = false;  // reset flag
  
    Serial.print("Digit pushed to: ");
    Serial.println(digitState);
    Serial.print("Entering switch... buttonPushed is ");
    Serial.println(buttonPushed);
  
    switch(digitState){
      case 0:         // Zero
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100000111110);
          updateShiftRegister(0b1011100001000001);
          updateShiftRegister(0b1101100001000001);
          updateShiftRegister(0b1110100001000001);
          updateShiftRegister(0b1111000000111110);
        }
        break;

      case 1:        // One
        for (counter = 0; counter < loopTime; counter++)
        {          
          updateShiftRegister(0b0111100000000000);
          updateShiftRegister(0b1011100000100001);   // note flipped order! "1"
          updateShiftRegister(0b1101100001111111);
          updateShiftRegister(0b1110100000000001);
          updateShiftRegister(0b1111000000000000);
        }      
        break;
 
      case 2:        // Two
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100000100111);  // pictorially, left part first
          updateShiftRegister(0b1011100001001001);   
          updateShiftRegister(0b1101100001001001);
          updateShiftRegister(0b1110100001001001);
          updateShiftRegister(0b1111000000110001);
        }
        break;

      case 3:        // Three
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100000100010);
          updateShiftRegister(0b1011100001000001);    
          updateShiftRegister(0b1101100001001001);
          updateShiftRegister(0b1110100001001001);
          updateShiftRegister(0b1111000000110110);
        }    
        break;

      case 4:        // Four
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100000001100);
          updateShiftRegister(0b1011100000010100);    
          updateShiftRegister(0b1101100000100100);
          updateShiftRegister(0b1110100001111111);
          updateShiftRegister(0b1111000000000100);
        }
        break;

      case 5:        // Five
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100001110010);
          updateShiftRegister(0b1011100001010001);    
          updateShiftRegister(0b1101100001010001);
          updateShiftRegister(0b1110100001010001);
          updateShiftRegister(0b1111000001001110);
        }
        break;

      case 6:        // Six
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100000011110);
          updateShiftRegister(0b1011100000101001);    
          updateShiftRegister(0b1101100001001001);
          updateShiftRegister(0b1110100001001001);
          updateShiftRegister(0b1111000000000110);
        }
        break;
    
      case 7:        // Seven
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100001000000);
          updateShiftRegister(0b1011100001001000);    
          updateShiftRegister(0b1101100001001111);
          updateShiftRegister(0b1110100001011000);
          updateShiftRegister(0b1111000001101000);
         }
        break;
      
      case 8:        // Eight
        for (counter = 0; counter < loopTime; counter++)
        {         
          updateShiftRegister(0b0111100000110110);
          updateShiftRegister(0b1011100001001001);    
          updateShiftRegister(0b1101100001001001);
          updateShiftRegister(0b1110100001001001);
          updateShiftRegister(0b1111000000110110);
        }
        break;

      case 9:        // Nine
        for (counter = 0; counter < loopTime; counter++)
        {             
          updateShiftRegister(0b0111100000110000);
          updateShiftRegister(0b1011100001001001);    
          updateShiftRegister(0b1101100001001001);
          updateShiftRegister(0b1110100001001001);
          updateShiftRegister(0b1111000000111110);
        }
        break;
      }  // close switch
      }  // close if
} // close void loop

void updateShiftRegister(int cur_data){
// Sets latchPin to low, then uses Arduino's shiftOut to shift cur_data 
// into shift registers. To finish, set latchPin back to high.

   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, cur_data);
   shiftOut(dataPin, clockPin, LSBFIRST, cur_data >> 8);
   digitalWrite(latchPin, HIGH);
}

void nextISR(){
  volatile static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200)   // debounce by waiting for signal
  {
    buttonPushed = true;
    counter = loopTime - 2;   // quickly stop loop
    
    if (digitState == 9)      // update digit
    {
      digitState = 0;
    }else{
      digitState += 1;   // new digit
    }
  }
  last_interrupt_time = interrupt_time;
}  
