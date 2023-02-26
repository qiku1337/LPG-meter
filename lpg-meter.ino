#include <A4988.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define DIR 7
#define STEP 6
#define RESET 4
#define SLEEP 8

#define RPM 200
#define MOTOR_STEPS 200
#define MOTOR_ACCEL 2000
#define MOTOR_DECEL 2000

A4988 stepper(MOTOR_STEPS, DIR, STEP);

int ill = 9;
int fuel = 10;
int res = 11;
int interrupt = 2;
int buttonState = 0;
int mapval = 0;
int previous = 0;
int pos = 0;

int x = 330;

bool reset_flag = true;
bool ign_flag = false;
bool no_sig = false;

byte keep_ADCSRA; 

float level = A1;

void setup() {
  Serial.begin(115200);
  
  //pinMode(fuel, OUTPUT);
  //pinMode(res, OUTPUT);
  
  pinMode(ill, OUTPUT);
  pinMode(fuel, OUTPUT);
  pinMode(res, OUTPUT);

  pinMode(interrupt, INPUT_PULLUP);

  pinMode(SLEEP, OUTPUT);
  digitalWrite(SLEEP, HIGH);

  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, LOW);
  delay(100);
  digitalWrite(RESET, HIGH);
  delay(100);

  stepper.begin(RPM, 1);
  stepper.setRPM(RPM);
  stepper.enable();

  stepper.setSpeedProfile(stepper.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);

  setled(1, ill);
  setled(0, fuel);

  Serial.println("Started");  
  reset_needle();
  delay(20);
}

void loop() {
  buttonState = digitalRead(interrupt);
  int val = 0;
  for (int g=0; g<10; g++){
    val +=analogRead(level);
    //Serial.println(g);
  }
  val = val / 10;
  
  int maxlpg = 80;
  int lowlpg = 24;
  mapval = map(val, maxlpg, lowlpg, 0, x);
  
  if (buttonState != HIGH) {                  //jak jest zaplon
    if(no_sig == true){
      delay(200);
      //val = 0;
      no_sig = false;
    }
    if(mapval<65){
      setled(1, res);
    }else{
      setled(0, res);
    }
    if(ign_flag == true){
      delay(500);
      digitalWrite(RESET, HIGH);
      delay(500);
      ign_flag = false;
    }
    if(val>0 && val<x){
      setled(1, fuel);

      Serial.print("mapval: ");
      Serial.print(-mapval + previous);
      Serial.print(" val: ");
      Serial.println(val);
      
      if(-mapval + previous > 5 || -mapval + previous < 5){
              stepper.rotate(-mapval + previous); 
              previous = mapval;  
      }      
      reset_flag = false;
      delay(20);
    }else{
      setled(0, fuel);
      no_sig = true;
      Serial.println("no_sig true");   
    }
  } else { // wyl zaplonu  
    if(reset_flag == false){
       stepper.rotate(previous);  
       Serial.println("go prev");     
    }      
      previous = 0;
      reset_flag = false;
      ign_flag = true;
      Serial.println("go sleep"); 
      delay(1000);
      enterSleep();
  }
}

void setled(int st, int led) {
  if (st == 1) {
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
  }
}

void reset_needle() {
  Serial.println("Reset needle");
  stepper.setRPM(RPM);
  stepper.rotate(1400);
  digitalWrite(RESET, LOW);
  delay(100);
  digitalWrite(RESET, HIGH);
  delay(400);
  //stepper.setRPM(RPM);
  //stepper.rotate(-28);
  //delay(200);  
}

void enterSleep(void)
{
  digitalWrite(SLEEP, LOW);
  
  setled(0, ill);
  setled(0, fuel);
  setled(0, res);

  cli();                                                                     
  sleep_enable ();                                                     
  attachInterrupt(0, Wakeup_Routine, LOW);
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  keep_ADCSRA = ADCSRA;
  ADCSRA = 0;                                                          
  sleep_bod_disable();                                                                                           
  sei();                                                                    
  sleep_cpu ();      
}

void Wakeup_Routine()
{
  Serial.println("Wakeup!"); 
  ADCSRA = keep_ADCSRA;
  digitalWrite(SLEEP, HIGH);
  digitalWrite(RESET, LOW);
  setled(1, ill);  
  sleep_disable();
  detachInterrupt(0);
}
