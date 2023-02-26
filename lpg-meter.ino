#include <A4988.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define DIR 7          // dir pin of stepper driver a4988
#define STEP 6         // step pin o
#define RESET 4        // RESET pin
#define SLEEP 8        // SLEEP pin

#define RPM 200
#define MOTOR_STEPS 200
#define MOTOR_ACCEL 2000
#define MOTOR_DECEL 2000

A4988 stepper(MOTOR_STEPS, DIR, STEP);

int ill = 9;         // illumination led pin 
int fuel = 10;       // green fuel led pin
int res = 11;        // res fuel led pin

int interrupt = 2;
int buttonState = 0;
int mapval = 0;
int previous = 0;
int pos = 0;

int x = 330;       //max x of stepper motor rotation

bool reset_flag = true;
bool ign_flag = false;
bool no_sig = false;

byte keep_ADCSRA;  //keep state of ADC

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
  digitalWrite(SLEEP, HIGH);        //disable sleep mode of a4988

  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, LOW);
  delay(100);
  digitalWrite(RESET, HIGH);
  delay(100);                      //reset a4988

  stepper.begin(RPM, 1);
  stepper.setRPM(RPM);
  stepper.enable();

  stepper.setSpeedProfile(stepper.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);

  setled(1, ill);
  setled(0, fuel);

  Serial.println("Started");  
  reset_needle();               //homing
  delay(20);
}

void loop() {
  buttonState = digitalRead(interrupt);   //read analog value (0-5v input)
  
  int val = 0;
  for (int g=0; g<10; g++){
    val +=analogRead(level);
    //Serial.println(g);
  }
  val = val / 10;                        //get avg value from adc
  
  int maxlpg = 80;
  int lowlpg = 24;
  
  mapval = map(val, maxlpg, lowlpg, 0, x); // map val for stepper motor range
  
  if (buttonState != HIGH) {               // ignition on
    if(no_sig == true){                    // prevent fluctuating value 
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
    if(val>0 && val<x){                 //check if ADC is not 0
      setled(1, fuel);

      Serial.print("mapval: ");
      Serial.print(-mapval + previous);
      Serial.print(" val: ");
      Serial.println(val);
      
      if(-mapval + previous > 5 || -mapval + previous < 5){ // move stepper if more than 5 tick
              stepper.rotate(-mapval + previous);           // prevent stepper skiping small stepps
              previous = mapval;  
      }      
      reset_flag = false;
      delay(20);
    }else{                               //no signal from ADC
      setled(0, fuel);
      no_sig = true;
      Serial.println("no_sig true");   
    }
  } else {                                          // ign off  
    if(reset_flag == false){
       stepper.rotate(previous);                    //return to home pos
       Serial.println("go prev");     
    }      
      previous = 0;                                 //set prev pos to 0
      reset_flag = false;
      ign_flag = true;
      Serial.println("go sleep"); 
      delay(1000);
      enterSleep();                                 //go sleep
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
  digitalWrite(SLEEP, LOW);                   //sleep on A4988
  
  setled(0, ill);
  setled(0, fuel);
  setled(0, res);

  cli();                                                                     
  sleep_enable ();                                                     
  attachInterrupt(0, Wakeup_Routine, LOW);    //attach to interrupt pin 0(digital pin 2)
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  keep_ADCSRA = ADCSRA;                       //keep current ADC bit
  ADCSRA = 0;                                 //sleep on ADC                                   
  sleep_bod_disable();                                                                                           
  sei();                                                                    
  sleep_cpu ();      
}

void Wakeup_Routine()
{
  Serial.println("Wakeup!"); 
  ADCSRA = keep_ADCSRA;                       //restore ADC bit
  digitalWrite(SLEEP, HIGH);                  //wake up A4988
  digitalWrite(RESET, LOW);                   //reset A4988
  setled(1, ill);  
  sleep_disable();
  detachInterrupt(0);
}
