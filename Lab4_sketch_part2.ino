/* EENG 348 Lab 4 Part 2          Yu Jun Shen and Austin Zhu
 * The lock-based concurrency code controls access to the 
 * shared pins to the OLED display, showing two balls on the 
 * screen. The left ball stays fixed with a constant radius, 
 * while the right ball shrinks in radius until it disappears. 
 * 
 * Like part 1, there is a "debugging LED" that blinks twice 
 * at the start and turns on at the end (in the while loop). 
 * YouTube link: https://youtu.be/2OVzwPYyXHo 
 */

#include "concurrency.h"
#include <stdlib.h>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>  // for LED screen

/******************** Define constants ********************/

// define Arduino pins
const int OLED_CS = 4;
const int OLED_DC = 5;
const int OLED_RST = 6;
const int OLED_CLK = 7;    // pin 7 to D0 on Yu Jun's OLED
const int OLED_SI = 8;     // pin 8 to D1 on Yu Jun's OLED
Adafruit_SSD1306 disp(128,64,OLED_SI,OLED_CLK,OLED_DC,OLED_RST,OLED_CS);
/* https://learn.adafruit.com/monochrome-oled-breakouts/wiring-128x32-spi-oled-display?view=all */

// debugging pins
const int debugLED = 2;     // for visual debugging

// Ball display constants
uint16_t x1 = 20;            // ball 1 center
uint16_t x2 = 100;            // ball 2 center

/******************** Structures ********************/

process_state *current_process;   // current_process prototype
process_state *ready_head;        // first structure ready to run --- naming issue? may be waiting now
lock_state *lockSPI;              // lock in this lab

struct process_state{          // process structure 
  unsigned int sp;             // stack pointer
  struct process_state *next;  // link tp next process
}; 

struct lock_state{
  boolean locked_value;    // True if locked, False if free
};


/******************** Display functions ********************
 * The rapid switching between p1 and p2 give the illusion * 
 * of two circles appearing. p1 (left circle) has a fixed
 * radius while p2 (right circle) decreases its radius until
 * it disappears. 
 ***********************************************************/
void p1(void)
{ // draw top circle 
  for(int i = 0; i < 50; i++){    
    lock_acquire(lockSPI);
    disp.clearDisplay();     // clear display to start fresh
    disp.drawCircle(x1, 30, 5, WHITE);
    disp.fillCircle(x1, 30, 5, WHITE);
    disp.display();
    lock_release(lockSPI);
    delay(35);     // give a chance for context switch 
  }
 
  disp.clearDisplay();
  return;
}

void p2(void)
{ // draw bottom circle 
  int r2 = 14;  
  for(int i = 0; i < 50; i++){
    lock_acquire(lockSPI);
    disp.clearDisplay();     // clear display to start fresh
    r2 -= i/4;
    disp.drawCircle(x2, 30, r2, WHITE);
    disp.fillCircle(x2, 30, r2, WHITE);
    disp.display();
    lock_release(lockSPI);
    delay(35);     // give a chance for context switch
  }
  disp.clearDisplay();
  return;
}

/************ Concurrency functions we implement ***********
 *  process_create, process_start and process_select
 ***********************************************************/

int process_create (void (*f) (void), int n){
  int stack_pointer;                     // local variable
  stack_pointer = process_init(*f, n);   //syntax for process_init
  
  if(stack_pointer == 0){
    return -1;   // error: stack could not be allocated
  }
  
  // at this point, stack_pointer is returned and non-zero. 
  // Traverse the ready list to append new process at the end.  
    
  process_state *p = ready_head;   
  //p is local structure variable
  
  // make new process
  process_state * new_process_state = (process_state *) malloc(sizeof(process_state));
  if(new_process_state == NULL){
    return -1;    // error: malloc failed
  } 
  new_process_state->sp = stack_pointer;   // int from process_init
  new_process_state->next = NULL;          // this is last link (for now)
  //new_process_state->amWaiting = false;  // default, not waiting
  
  if (p == NULL)                      // no ready process yet
  {
    ready_head = new_process_state;        
    // ready queue starts with new_process_state as head 
  }else 
  {        // ready_head != NULL, already has ready processes
    while(p->next != NULL)      // traverse the ready states 
    { 
      p = p->next;
    } 
    // at this point, p is the last link structure
    // so link current chain to new_process_state 
    p->next = new_process_state;      
  } 
  return 0;    // success
}

void process_start (void){  
  process_begin();         // this runs (once) in the void loop
}

unsigned int process_select (unsigned int cursp){
  /* argument cursp is stack pointer of currently running process
   * If no currently running procss, or it terminated, cursp = 0.
   * If no process ready to execute: return 0. Else, for ready 
   * process, return value of stack pointer for next ready process. */
  
  if(cursp == 0)    
  { /* No currently running process, which means lock has unlocked. */
    if (ready_head == NULL)      // no process ready to execute
    {
      free(current_process);     // free memory
      free(ready_head);
      disp.clearDisplay();
      return 0;                  // Done. 
    }else              
    { /* No currently running process, but another process is ready. */
      current_process = ready_head;    
      ready_head = ready_head->next;  // update ready_head
      return current_process->sp;  
    }
  }
  else 
  { /*  if(cursp != 0), non-terminated case */
    current_process->sp = cursp;             // int input
    process_state *temp = current_process;   // temp is local variable

    if(lockSPI->locked_value == true)
    { // lock is locked; do not context switch.
      // resume current process, and make the other process wait. 
      return cursp;
    }else{
      // lock is unlocked; do context switch.
      if (ready_head != NULL)
      { /*cursp != 0, unlocked lock, ready_head != NULL */                
        // suppose at first: [p1_sp | pointer] -> [p2_sp | pointer] -> NULL
        // with p1 as current_process, p2 as ready_state. Interrupt occurs
      
        ready_head -> next = current_process;
        // [p1_sp | pointer] -> [p2_sp | pointer] -> [p1_sp | pointer] -> ...
         
        temp = ready_head;
        // temp = [p2_sp | pointer] -> ... not NULL
      
        ready_head = ready_head -> next;
        // ready_head = [p1_sp | pointer] -> ... not NULL
      
        current_process = temp;
        // current_process = [p2_sp | pointer] -> ... not NULL
      
        ready_head -> next = NULL;
        // ready_head = [p1_sp | pointer] -> NULL
        // So here, we have [p2_sp | pointer] -> [p1_sp | pointer] -> NULL
        // with p2 as current and p1 as ready. (Local )temp is not saved. 
        return current_process->sp; 
      }else 
      { /*cursp != 0, unlocked lock, ready_head = NULL */  
        return cursp; 
      }     
     }  // close unlocked lock condition
    }   // close if cursp != 0 condition
  }     // close process_select condition
  

/************* Lock functions we implement *************/
void lock_init(lock_t *l){
  lock_state * new_lock = (lock_state *) malloc(sizeof(lock_state));
  new_lock -> locked_value = false;   // initially unlocked
  l = new_lock;
}

void lock_acquire(lock_t *l){
  // acquires the lock l if it is free; otherwise blocks by 
  // calling yield, which calls process_select. 
  if (l->locked_value == false){  // acquires lock
    l->locked_value = true;
    return; 
  }else{   // fails to acquire lock
    yield();
  }
}

void lock_release (lock_t *l){
  // releases the lock l
  l->locked_value = false;  
}


/*************** Execution of main program ***************/

void setup() {
  pinMode(debugLED, OUTPUT);
  
  // set-up display
  disp.begin(SSD1306_SWITCHCAPVCC);   
  disp.clearDisplay();
  disp.display();
  disp.drawLine(0,0,63,63,WHITE);   

  current_process = NULL;   // initiate current_process pointer  
  ready_head = NULL;        // at first, ready_head is NULL
  
  lock_init(lockSPI);     // initialize lock
  
  if (process_create (p1, 64) < 0)
  {                         // ready_head points to p1's structure
    return;
  }

  if (process_create (p2, 64) < 0)
  {                         // ready_head points to p2's structure
    return;
  }

  digitalWrite(debugLED, HIGH); // visual check before leaving setup
  delay(300);                 
  digitalWrite(debugLED, LOW);
  delay(300);
  digitalWrite(debugLED, HIGH);  
  delay(300);                 
  digitalWrite(debugLED, LOW);    
  delay(300);

}

void loop() {   
  process_start();

  disp.clearDisplay();
  disp.display();

  while(1){  
    digitalWrite(debugLED, HIGH); // visual check at end          
  }
}
