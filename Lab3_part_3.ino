/* EENG 348 Lab 3 Part 3     by  Yu Jun Shen and Austin Zhu  
 * Clockwise: faster. Anticlockwise: slower. Ball bounces off four walls. 
 * Rotary encoder (COM-09117) middle pin connects to ground, and 
 * outside two pins connect to digital pins 2 and 3
 * Seven pin OLED display connected by SPI; see schematic for details
 * 
 * YouTube video of demonstration at: 
 * https://www.youtube.com/watch?v=ywuBsGQ8JHw
 */

# include <Adafruit_SSD1306.h>

// define pins
const int ROTARY_A = 2;              // connect to Arduino external interrupt pin
const int ROTARY_B = 3;
const int OLED_CS = 4;
const int OLED_DC = 5;
const int OLED_RST = 6;
const int OLED_CLK = 7;
const int OLED_SI = 8;
Adafruit_SSD1306 disp(128,64,OLED_SI,OLED_CLK,OLED_DC,OLED_RST,OLED_CS);
// pin connections explained at: https://learn.adafruit.com/monochrome-oled-breakouts/wiring-128x32-spi-oled-display?view=all 

volatile int encoder = 128;           // rotary input, to be modified in ISR                                      
int prev_encoder = 128;               // to store previous encoder
volatile int x_velocity = 0;          // rotary input, to be modified in ISR                                       
volatile int y_velocity = 0;          // rotary input, to be modified in ISR                                       
volatile int flipX = 1;               // flip flag X
volatile int flipY = 1;               // flip flag Y

uint16_t x0 = 30;                     // ball center horizontal displacement coordinate
uint16_t y0 = 20;                     // ball center vertical displacement coordinate
uint16_t r = 4;                       // ball radius (pixel)
  
volatile boolean half_step_clockwise = false;         // flag variables in interrupt
volatile boolean half_step_counterclockwise = false;
//  Increment (clockwise) iff ROTARY_A and ROTARY_B are both LOW, with 
//  half_step_clockwise = true previously 
//  Decrement (c-clockwise) iff ROTARY_A and ROTARY_B are both LOW, with 
//  half_step_counterclockwise = true previously 


void setup(){
  Serial.begin(9600);
  pinMode(ROTARY_A, INPUT_PULLUP);   // set interrupt pins as input with pullup
  pinMode(ROTARY_B, INPUT_PULLUP);    
  
  attachInterrupt(digitalPinToInterrupt(ROTARY_A), isr_rotaryA, FALLING);   
  // Call isr_rotaryA when ROTARY_A pin (digital 2) goes LOW
  attachInterrupt(digitalPinToInterrupt(ROTARY_B), isr_rotaryB, FALLING);   
  // Call isr_rotaryB when ROTARY_B pin (digital 3) goes LOW

  // display stuff
  disp.begin(SSD1306_SWITCHCAPVCC);   
  disp.clearDisplay();
  disp.display();
  disp.drawLine(0, 0, 63, 63, WHITE);
}

void loop(){
  if(encoder != prev_encoder){            // print changes to Serial monitor  
    Serial.print("Update: ");             // to show current velocity 
    Serial.print(encoder);     
    prev_encoder = encoder;
    Serial.print("      x_velocity: ");
    Serial.println(x_velocity);
  }

  // Display section
  disp.display();

  if(x0+x_velocity < 126 && 2 <= x0+x_velocity){
    x0 += x_velocity;            // basic motion
    Serial.println(x0);
  }else if (x0 >= 64){
    x_velocity = -x_velocity;    // flip once
    flipX = -flipX;              // change flag for interrupt (reflection)
    Serial.print("Flip: "); 
    Serial.println(x_velocity);
    x0 = 126;                    // at wall
  }else if (x0 < 64){
    x_velocity = -x_velocity;    // flip once
    flipX = -flipX;              // change flag for interrupt (reflection)
    Serial.print("Flip: ");
    Serial.println(x_velocity);
    x0 = 1;                      // at wall
  }
  
  if(y0+y_velocity < 62 && 2 <= y0+y_velocity){
    y0 += y_velocity;            // basic motion
    Serial.println(y0);
  }else if (y0 >= 32){
    y_velocity = -y_velocity;    // flip once
    flipY = -flipY;              // change flag for interrupt (reflection)
    Serial.print("Flip: ");
    Serial.println(y_velocity);
    y0 = 62;                     // bounce at wall
  }else if (y0 < 32){
    y_velocity = -y_velocity;    // flip once
    flipY = -flipY;              // change flag for interrupt (reflection)
    Serial.print("Flip: ");
    Serial.println(y_velocity);
    y0 = 1;                      // at wall
  }
  
  show_circle(x0, y0, r, WHITE);    // Draw circles (filled). 
  //This command "clears" everything previously on display
}

// Define interrupt service routines
void isr_rotaryA(){                            // ROTARY_A falls to LOW
  delay(1);                                    // Wait to debounce
  if(digitalRead(ROTARY_A) == LOW){            // if pin remains LOW (stable) 
    if(digitalRead(ROTARY_B) == HIGH && half_step_clockwise == false)
    {      
      // half step to clockwise done
      half_step_clockwise = true;                                     
    } 
    if(digitalRead(ROTARY_B) == LOW && half_step_counterclockwise == true)
    {         
      // full counterclockwise step done, reset flag and decrement 
      half_step_counterclockwise = false;                                    
      if(encoder > 0){   // saturate at 0
        encoder = encoder - 1;  
        x_velocity = (encoder - 128)*0.5*flipX;    // check flip or not
        y_velocity = (encoder - 128)*0.5*flipY;
      }                                          
    }
  }
}

void isr_rotaryB(){                            // ROTARY_B falls to LOW
  delay(1);                                    // Wait to debounce
  if(digitalRead(ROTARY_B) == LOW){            // if pin remains LOW (stable)
    if(digitalRead(ROTARY_A) == HIGH && half_step_counterclockwise == false)
    {      
      // half step to counterclockwise done
      half_step_counterclockwise = true;                                    
    }                                                     
    if(digitalRead(ROTARY_A) == LOW && half_step_clockwise == true)
    {      
      // full counterclockwise step done, reset flag and increment 
      half_step_clockwise = false;     
      if(encoder < 255){         // to saturate at 255
        encoder = encoder + 1;   
        x_velocity = (encoder - 128)*0.5*flipX;
        y_velocity = (encoder - 128)*0.5*flipY;      }
    }
  }
}

// draw function
void show_circle(int x0, int y0, int r, int color) {
  // syntax of drawCircle function: (uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

  disp.clearDisplay();     // clear display to start fresh

  disp.drawCircle(x0, y0, r, color);
  disp.fillCircle(x0, y0, r, color);
  disp.display();
  
}
