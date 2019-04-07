/*
    Classical conditioning task with odor stimuli indicating the probability
    Author: Dohoung Kim
*/

//// Pin definition
// portD (pin 0-7)
#define odorPin0 2 // Normally-open valve that transmit fresh air
#define odorPin1 3 // cue 1
#define odorPin2 4 // cue 2
#define odorPin3 5 // cue 3
#define odorPin4 6 // cue 4
#define odorPin5 7 // final valve

// portB (pin 8-13)
#define punishmentPin 0 //
#define rewardPin 1
#define noRewardPin 2 //
#define eventPin 3 // pin 11 output event signal 
#define sensorPin 4 // pin 12
#define laserPin 5 // pin 13

//// Task related variables
int nTrial = 200; // total trial number
int nCue = 2; // cue number (1, 2, 4)
unsigned long valveDuration = 130;
unsigned long duration[6] = {500000,1000000,1000000,130000,2370000,2000000}; // epoch duration
unsigned long rewardProbability[4] = {0, 0, 0, 0}; // unit: percent
unsigned long outcomeIdentity[4] = {1, 1, 1, 1}; // 1: reward, 2: punishment
// {0, 30000, 42000, 53000, 64000, 77000, 86000, 92000, 100000, 107000, 116000};
// {0, 20000, 30000, 40000, 47000, 52000, 57000, 60000, 65000, 70000, 75000};// 0ul...10ul
unsigned long rewardAmount[11] = {0, 10000, 20000, 30000, 35000, 40000, 47000, 52000, 57000, 60000, 65000}; // 0ul...10ul
unsigned long rewardDuration = 42000;
unsigned long punishDuration = 100000;
unsigned long lDuration = 100;

int state = 9;
    // state 0: base
    // state 1: cue
    // state 2: delay
    // state 3: reward
    // state 4: intertrial interval
    // state 9: stand-by
int select = 0; // serial input (w for water, t for trial number, s for start, e for end...)
int iTrial = 0; // current trial
unsigned long inivalveDuration = 100000; // water valve open duration
int itiDuration = 2;
int delayDuration = 1;
int prob1 = 0;
int prob2 = 0;
int prob3 = 0;
int prob4 = 0;
int cueChoice = 0;

int cue = 0; // current cue
int prevCue = 0; // previous cue
int nRepeat = 1; // current repeat number
const int maxRepeat = 3; // maximal repeat number
int addCue = 0; // the rest
int cueIndex = 0; // randomized cue
int cue_plus[4] = {0, 0, 0, 0}; //determine cue kinds
int identity = 0; // outcome identity

int reversal = 0;
int reversalTimes = 0;
boolean lightOn = false;
boolean reward = 0; // reward in current trial
boolean outcome = 0; // did mouse lick during delay period?
boolean waterClear = true; // did mouse lick after water reward? if not, do not give additional water
unsigned long times = 0; // current time
unsigned long timeStart = 0; // current epoch start time
boolean prevsensor = false; // previous sensor state
boolean cursensor = false; // current sensor state
String inString = "";

// for printf function (do not edit)
int serial_putchar(char c, FILE* f) {
    if (c == '\n') serial_putchar('\r', f);
    return Serial.write(c) == 1? 0 : 1;
}
FILE serial_stdout;
// end of printf function (do not edit)

void setup() {
    Serial.begin(115200); // serial setup
    fdev_setup_stream(&serial_stdout, serial_putchar, NULL, _FDEV_SETUP_WRITE); // printf setup (do not edit)
    stdout = &serial_stdout; // printf setup (do not edit)
    randomSeed(analogRead(0)); // determines seed number of random function
  
    // Pin setup
    DDRD = B11111110; // Pin 7, 6, 5, 4, 3, 2, 1, 0. Pin 0 and 1 is reserved for serial communication.
    DDRB = B00101111; // Pin xx, xx, 13, 12, 11, 10, 9, 8. 
    PORTD &= B00000011; // reset pin 2-7
    PORTB &= B11010000; // reset pin 8-11 and 13 (pin 12 is sensor)
}

//// Serial printf output form
    // b000 : state 0 and trial number
    // c0 : state 1 and current cue
    // d1 : state 2
    // r0 : state 3 and current reward (Y/N)
    // i : state 4 and outcome
    // l1 : licking
    // e9 : trial end
    // v0 : reversal mode
    // f1 : reversal frequency(times)
    // o1 : outcome contingehcy(identity)
    // n2 : nCue
    // t000 : nTrial
    
void loop() {
    // Standby state
    if (state==9) {
        if (Serial.available() > 0) {
            select = Serial.read();
            
            // water valve on
            if (select == 'w') {
                inivalveDuration = Serial.parseInt();
                if (inivalveDuration == 0) {
                    inivalveDuration = valveDuration;
                }
                valveOn(inivalveDuration);
            }

            // light on
            else if (select == 'l') {                              
               int deltime = random(100000, 200000);
               LOn (lDuration);
               delay(deltime);
            }

           // punish valve on
            else if (select == 'a') {
                inivalveDuration = Serial.parseInt();
                if (inivalveDuration == 0) {
                    inivalveDuration = 1000000;
                }
                punishOn(inivalveDuration);            
            }
           
            // set trial number
            else if (select == 't') {
                nTrial = Serial.parseInt();
                if (nTrial == 0) {
                    nTrial = 200;
                }
            }
            
            // set reward amount
            else if (select == 'r') {
                rewardDuration = rewardAmount[Serial.parseInt()];
                duration[3] = rewardDuration;
                duration[4] = 2500000 - rewardDuration;
            }
            else if (select =='u'){
              punishDuration = Serial.parseInt();
              punishDuration = punishDuration*1000;
            }
            
             // intertrial interval
            else if (select == 'i') {
                itiDuration = Serial.parseInt();
                if (itiDuration < 1 || itiDuration > 20) {
                    itiDuration = 2;
                }
                duration[5] = itiDuration * 1000000;
            }
            
            // cue selection
                // c10: cue A only
                // c11: cue B only
                // c12: cue C only
                // c13: cue D only
                // c20: cue AB [addCue/3,addCue%3]
                // c21: cue AC [0,1]
                // c22: cue AD [0,2]
                // c24: cue BC 1,1
                // c25: cue BD 1,2
                // c28: cue CD 2 2
                // c30: cue ABC[addCue/5, addCue/3,addCue%2]
                // c31: cue ABD
                // c33: cue ACD
                // c35: cue BCD
                // c40: cue ABCD
            else if (select == 'c') {
                cueChoice = Serial.parseInt();
                nCue = cueChoice/10;
                addCue = cueChoice % 10;
                if (nCue == 1) {
                  cue_plus[0] = addCue; // cue A,B,C,D
                }
                else if(nCue ==2){
                  cue_plus[0] = addCue/3;
                  cue_plus[1] = addCue%3;
                }
                else if (nCue ==3) {
                  cue_plus[0] = addCue/5;
                  cue_plus[1] = addCue/3;
                  cue_plus[2] = addCue%2;
                }
            }

            // probability
            else if (select == 'p') {
                prob1 = Serial.parseInt();
                rewardProbability[0+cue_plus[0]] = prob1; // cue A
                if (nCue==2){
                  prob2 = Serial.parseInt();
                  rewardProbability[1+cue_plus[1]] = prob2; // cue B
                }
                else if (nCue==3) {
                  prob2 = Serial.parseInt();
                  prob3 = Serial.parseInt();
                  rewardProbability[1+cue_plus[1]] = prob2; // cue B
                  rewardProbability[2+cue_plus[2]] = prob3; // cue C
                }
                else if (nCue==4){
                  prob2 = Serial.parseInt();
                  prob3 = Serial.parseInt();
                  prob4 = Serial.parseInt();
                  rewardProbability[1] = prob2; // cue B
                  rewardProbability[2] = prob3; // cue C
                  rewardProbability[3] = prob4; // cue D
                }          
            }

            // Delay
            else if (select == 'd') {
                 delayDuration = Serial.parseInt();
                 duration[2] = delayDuration * 1000000;
            }
            
            // Reversal mode
              // v0 : no reversal
              // v1 : once reversal
              // v2 : multiple reversal
           else if (select == 'v'){
             reversal = Serial.parseInt();
             reversalTimes = Serial.parseInt();
           }

           //Outcome identity
           // 1:reward 2: punish
           else if (select == 'o'){
              identity = Serial.parseInt();
              if (identity == 0){
                outcomeIdentity[0] = 1;
                outcomeIdentity[1] = 1;
                outcomeIdentity[2] = 2;
                outcomeIdentity[3] = 2;
              }
              else if (identity == 1){
                outcomeIdentity[0] = 1;
                outcomeIdentity[1] = 2;
                outcomeIdentity[2] = 2;
                outcomeIdentity[3] = 1;
              }
              else if (identity == 2){
                outcomeIdentity[0] = 1;
                outcomeIdentity[1] = 2;
                outcomeIdentity[2] = 1;
                outcomeIdentity[3] = 2;
              }          
              else if (identity == 3){         
                outcomeIdentity[0] = 2;
                outcomeIdentity[1] = 1;
                outcomeIdentity[2] = 1;
                outcomeIdentity[3] = 2;
              }
              else if (identity == 4){
                outcomeIdentity[0] = 2;
                outcomeIdentity[1] = 1;
                outcomeIdentity[2] = 2;
                outcomeIdentity[3] = 1;
              }
              else if (identity == 5){
                outcomeIdentity[0] = 2;
                outcomeIdentity[1] = 2;
                outcomeIdentity[2] = 1;
                outcomeIdentity[3] = 1;
              }
              else if (identity ==6){
                outcomeIdentity[0] = 2;
                outcomeIdentity[1] = 2;
                outcomeIdentity[2] = 2;
                outcomeIdentity[3] = 2;
              }
            }
            
            // start trial
                // s0: no modulation
                // s1: modulation
            else if (select == 's') {
                int iType = Serial.parseInt();
                if (iType == 0) {
                    lightOn = false;
                }
                else if (iType == 1) {
                    lightOn = true;
                } 
                else {
                    nCue = 2;
                    addCue = 0;
                    lightOn = false;
                    rewardProbability[0] = 75;
                    rewardProbability[1] = 25;
                    rewardProbability[2] = 50;
                    rewardProbability[3] = 75;
                }
                printf("%luo%d\n",times,identity);
                printf("%lun%d\n",times,cueChoice);
                printf("%lut%d\n",times,nTrial);
                if (reversal!=0){
                   printf("%luf%d\n",times,reversalTimes);
                 }
                state = 5;
            }
        }
    }
    
    // Task state
    else if (state < 9) {
        times = micros();
    
        // Sensor read
        cursensor = bitRead(PINB,sensorPin);
        if (cursensor & ~prevsensor) {
            printf("%lul1\n",times); // %l:decimal %lu:long decimal \n:enter
            prevsensor = cursensor;
            if (state==2) { 
                outcome = true;
            } // if sensor is touched during delay state, make outcome true. else, it is omitted trial.
            if (waterClear == false) {
                waterClear = true;
            }
        } // sensor is turned on
        else if (~cursensor & prevsensor) {
            prevsensor = cursensor;
        } // sensor is turned off
       
        // State conversion if time reaches each epoch duration
        if (times >= timeStart+duration[state]) { // is current duration(=time-timeStart) longer than epoch duration?
            timeStart = times; // reset epoch start time
            
            // state 0: baseline -> 1: cue (0.5 s)
            if (state==0) {
                PORTD &= B01111111; // turn off pin 7 (final odor output valve)
                
                state = 1; // state conversion 0 to 1
                printf("%luc%d\n",times,cue);
            }
          
            // state 1: cue -> state 2: delay
            else if (state==1) {
              if (duration[2] >0) {
                PORTD &= B00000011; // turn off all odor valves
              }

                state = 2;
                printf("%lud1\n",times);
            }
          
            // state 2: delay -> 3: reward
            else if (state==2) { 
                reward = (random(1,101) <= rewardProbability[cue]); // determine whether reward is given or not 
               if (reward && waterClear && outcomeIdentity[cue]==1) {
                    bitSet(PORTB,rewardPin); // reward valve open
                    waterClear = false; // water is not cleared
                    duration[3] = rewardDuration;
                    duration[4] = 2500000 - rewardDuration;
                    printf("%lur1\n",times);
                }
                else if (reward && outcomeIdentity[cue]==2) {
                    bitSet(PORTB,punishmentPin);
                    duration[3] = punishDuration;
                    duration[4] = 2500000 - punishDuration;
                    printf("%lur1\n",times);     
                }
                else if (reward==false) {                     
                    bitSet(PORTB,noRewardPin); // notify if no reward is given
                    printf("%lur0\n",times);
                }
                if (duration[2] <=0) {
                PORTD &= B00000011; // turn off all odor valves
              }
                
                state = 3;
            }
          
            // state 3: reward -> 4: reward lick
            else if (state==3) { 
                PORTB &= B00110000; // reset pin 8-11 (turn off water valves)
                
                state = 4;
            }

            // state 4: reward lick -> 5: iti
            else if (state==4) {
                bitClear(PORTB,laserPin);

                state = 5;
                duration[5] = random((itiDuration-1)*1000000,itiDuration*1000000+1000001); // determine iti duration
                printf("%lui%d\n",times,outcome);
                printf("%luv%d\n",times,reversal);
                outcome = false;
            }
          
            // state 5: iti -> 0: base
            else if (state==5) {
                if (iTrial>=nTrial) { // if current trial number is reached goal trial number, return to standby state
                    PORTD = B10000000;
                    delay(10);
                    PORTB &= B00010000; // reset water valve
                    PORTD &= B00000011; // reset odor valve
                    
                    state = 9;
                    iTrial = 0;
                    nTrial = 200;
                    itiDuration = 2;
                    duration[3] = 130000;
                    waterClear = true;

                    printf("%lue%d\n",times,state);
                }
                else {
                    cueIndex = random(nCue);
                    cue = cue_plus[cueIndex] + cueIndex; // choice cue for next trial

                    if (cue==prevCue) {
                        ++nRepeat;
                        if (nCue>1 && nRepeat>maxRepeat) {
                            cueIndex = random(nCue-1);
                            cue = cue_plus[cueIndex] + cueIndex;
                            if (cue >= prevCue) {
                              cue = cue_plus[cueIndex+1] + cueIndex+1;;
                            }
                        }
                    } // if same cue repeats over 3 times, choose difference cue.
                    else {
                        nRepeat = 1;
                    }
                    PORTD = B10000100 | (B00001000 << cue); // turn on NO valve, final valve, and cue stimulus valve
                    printf("%lup1%d\n",times,rewardProbability[0]);
                    printf("%lup2%d\n",times,rewardProbability[1]);
                    printf("%lup3%d\n",times,rewardProbability[2]);
                    printf("%lup4%d\n",times,rewardProbability[3]);
                    
                    state = 0;
                    ++iTrial;
                    prevCue = cue;
                    printf("%lub%d\n",times,iTrial);
                    }
                }
        } // State conversion end
        
        // Check serial
        if (Serial.available() > 0) {
            select = Serial.read();
            if (select == 'e') {
                nTrial = 0;
            }
             //Outcome identity reversal
             // 1:reward 2: punish
             else if (select == 'o'){
                identity = Serial.parseInt();
                if (identity == 0){
                  outcomeIdentity[0] = 1;
                  outcomeIdentity[1] = 1;
                  outcomeIdentity[2] = 2;
                  outcomeIdentity[3] = 2;
                }
                else if (identity == 1){
                  outcomeIdentity[0] = 1;
                  outcomeIdentity[1] = 2;
                  outcomeIdentity[2] = 2;
                  outcomeIdentity[3] = 1;
                }
                else if (identity == 2){
                  outcomeIdentity[0] = 1;
                  outcomeIdentity[1] = 2;
                  outcomeIdentity[2] = 1;
                  outcomeIdentity[3] = 2;
                }          
                else if (identity == 3){         
                  outcomeIdentity[0] = 2;
                  outcomeIdentity[1] = 1;
                  outcomeIdentity[2] = 1;
                  outcomeIdentity[3] = 2;
                }
                else if (identity == 4){
                  outcomeIdentity[0] = 2;
                  outcomeIdentity[1] = 1;
                  outcomeIdentity[2] = 2;
                  outcomeIdentity[3] = 1;
                }
                else if (identity == 5){
                  outcomeIdentity[0] = 2;
                  outcomeIdentity[1] = 2;
                  outcomeIdentity[2] = 1;
                  outcomeIdentity[3] = 1;
                }
              }
              // probability
              else if (select == 'p') {
                  prob1 = Serial.parseInt();
                  rewardProbability[0+cue_plus[0]] = prob1; // cue A
                  if (nCue==2){
                    prob2 = Serial.parseInt();
                    rewardProbability[1+cue_plus[1]] = prob2; // cue B
                  }
                  else if (nCue==3) {
                    prob2 = Serial.parseInt();
                    prob3 = Serial.parseInt();
                    rewardProbability[1+cue_plus[1]] = prob2; // cue B
                    rewardProbability[2+cue_plus[2]] = prob3; // cue C
                  }
                  else if (nCue==4){
                    prob2 = Serial.parseInt();
                    prob3 = Serial.parseInt();
                    prob4 = Serial.parseInt();
                    rewardProbability[1+cue_plus[1]] = prob2; // cue B
                    rewardProbability[2+cue_plus[2]] = prob3; // cue C
                    rewardProbability[3+cue_plus[3]] = prob4; // cue D
                  }          
              }
        }
    }
}

void valveOn(unsigned long valveDuration)
{
  bitSet(PORTB,rewardPin);
  delay(valveDuration);
  bitClear(PORTB,rewardPin);
}

void punishOn(unsigned long valveDuration)
{
  bitSet(PORTB,punishmentPin);
  delay(valveDuration);
  bitClear(PORTB,punishmentPin);
}

void LOn(unsigned long lDuration)
{
  bitSet(PORTB,laserPin);   
  delay(lDuration);
  bitClear(PORTB,laserPin);
}
