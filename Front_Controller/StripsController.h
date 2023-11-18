#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define FADE_DELAY 3000
#define HOLD_DELAY 3000
#define BLINK_DELAY 2000
int counters[4][2];
int booleans[4][2];
int data[4][2];
long delays[4][2];
float f_data[4][1];
int colors[4][2][3];

int localSteps[4];

class StripsController{
  
  Adafruit_PWMServoDriver pwm;

  public:bool resetted = true;

  public:
  StripsController(){

    pwm = Adafruit_PWMServoDriver();
    pwm.begin();
    pwm.setPWMFreq(300); 

    resetAll();
  }

  void frame(){

    if(resetted)
      resetted = false;
    
    channel_0(); //top left
    channel_1(); //lower left
    channel_2(); //top right
    channel_3(); //lower right
  }

  void channel_0(){


    if(!booleans[0][1])
      switch (localSteps[0]){

        case 0:
          blink_mode(0, 0x00ffff, BLINK_DELAY);
        break;

        case 1:
          hold_mode(0, 0x00ffff, HOLD_DELAY);
        break;

        case 2:
          fade_mode(0, 0x00ffff, 0xff00ff, FADE_DELAY);
        break;

        case 3:
          hold_mode(0, 0xff00ff, HOLD_DELAY);
        break;

        case 4:
          fade_mode(0, 0xff00ff, 0x00ffff, FADE_DELAY);

        break;

        case 5:
          hold_mode(0, 0x00ffff, HOLD_DELAY);
        break;
      }

    if(booleans[0][1]){
      
      resetValues(0);
  
      if(localSteps[0] < 4)
        localSteps[0]++;
      else
        localSteps[0] = 1;
    }
  }
  
  
  void channel_1(){
    
    if(!booleans[1][1])
      switch (localSteps[1]){

        case 0:
          blink_mode(1, 0xff00ff, BLINK_DELAY);
        break;

        case 1:
          hold_mode(1, 0xff00ff, HOLD_DELAY);
        break;

        case 2:
          fade_mode(1, 0xff00ff, 0x00ffff, FADE_DELAY);
        break;

        case 3:
          hold_mode(1, 0x00ffff, HOLD_DELAY);
        break;

        case 4:
          fade_mode(1, 0x00ffff, 0xff00ff, FADE_DELAY);

        break;
      }
  
  
    if(booleans[1][1])
    {
      resetValues(1);
  
      if(localSteps[1] < 4)
        localSteps[1]++;
      else
        localSteps[1] = 1;
    }
  }
  
  
  void channel_2(){
    
    if(!booleans[2][1])
      switch (localSteps[2]){

        case 0:
          blink_mode(2, 0x00ffff, BLINK_DELAY);
        break;

        case 1:
          hold_mode(2, 0x00ffff, HOLD_DELAY);
        break;

        case 2:
          fade_mode(2, 0x00ffff, 0xff00ff, FADE_DELAY);
        break;

        case 3:
          hold_mode(2, 0xff00ff, HOLD_DELAY);
        break;

        case 4:
          fade_mode(2, 0xff00ff, 0x00ffff, FADE_DELAY);

        break;
      }
  
  
    if(booleans[2][1])
    {
      resetValues(2);
  
      if(localSteps[2] < 4)
        localSteps[2]++;
      else
        localSteps[2] = 1;
    }
  }
  
  
  
  void channel_3(){
    
    if(!booleans[3][1])
      switch (localSteps[3]){

        case 0:
          blink_mode(3, 0xff00ff, BLINK_DELAY);
        break;

        case 1:
          hold_mode(3, 0xff00ff, HOLD_DELAY);
        break;

        case 2:
          fade_mode(3, 0xff00ff, 0x00ffff, FADE_DELAY);
        break;

        case 3:
          hold_mode(3, 0x00ffff, HOLD_DELAY);
        break;

        case 4:
          fade_mode(3, 0x00ffff, 0xff00ff, FADE_DELAY);
        break;
      }
  
  
    if(booleans[3][1])
    {
      resetValues(3);
  
      if(localSteps[3] < 4)
        localSteps[3]++;
      else
        localSteps[3] = 1;
    }
  }
  
  


  void PWM(int ch, int p){
    
    pwm.setPWM(ch, 0, map(p, 0, 255, 0, 4095));
  }
  
  
  
  void setColor(int ch, long col, float brightness){

    PWM(3 * ch, colCon(col, 'R') * brightness);
    PWM(3 * ch + 1, colCon(col, 'G') * brightness);
    PWM(3 * ch + 2, colCon(col, 'B') * brightness);
  }
  
  void resetAll(){
    
    for(int i = 0; i < 4; i++){
      
      resetValues(i);
      localSteps[i] = 0;
    }

    resetted = true;
  }
  
  void resetValues(byte ch){
    
    for(int i = 0; i < 2; i++)
    {
      counters[ch][i] = 0;
      
      booleans[ch][i] = false;
      
      data[ch][i] = 0;
  
      delays[ch][i] = 0;
    }   
     
    f_data[ch][0] = 0;
  
    for(int j = 0; j < 3; j++)
      for(int k = 0; k < 2; k++)
        colors[ch][k][j] = 0;
  }

  void blink_mode(byte ch, long col, int del){
    
    if(booleans[ch][1])                                    //EXIT
      return;
  
  
    if(counters[ch][0] == 3 && f_data[ch][0] <= 1)        //FINAL BRIGHTNESS GAIN
    {
      setColor(ch, col, f_data[ch][0]);
          
      f_data[ch][0] += 0.05;
  
      return;
    }
  
  
    if(counters[ch][0] == 3 && f_data >= 1)               //SET QUIT FLAG
    {
      booleans[ch][1] = true;
  
      return;
    }
  
  
    if(millis() - delays[ch][0] >= del && booleans[ch][0]) //TERMINATION
    {
      counters[ch][0] = 3;
  
      return;
    }
  
    
    if(!booleans[ch][0])                                   //SETUP
    {
      delays[ch][0] = millis();
      
      booleans[ch][0] = true;
      booleans[ch][1] = false;
  
      data[ch][0] = random(150, 200);
    }
  
  
    if(counters[ch][0] == 0 && f_data[ch][0] <= 1)        //BRIGHTNESS INCREASING
    {
      setColor(ch, col, f_data[ch][0]);
  
      f_data[ch][0] += 0.05;
  
      return;
    }
  
  
    if(counters[ch][0] == 0 && f_data[ch][0] >= 1)        //SWITCHING FORM 0 PHASE TO 1
    {
      counters[ch][0] = 1;
      delays[ch][1] = millis();
    }
  
  
    if(counters[ch][0] == 1)                              //DELAY WHILE LED ON
    {
      if(millis() - delays[ch][1] >= data[ch][0])
      {
        delays[ch][1] = millis();
  
        setColor(ch, 0, 1);
        f_data[ch][0] = 0;
  
  
        if(data[ch][0] < 30)
          data[ch][0] = random(10, 30);
        else
          data[ch][0] *= 0.7;
  
        counters[ch][0] = 2;
      }
      else
        return;
    }
  
  
    if(counters[ch][0] == 2)                              //DELAY WHILE LED OFF
      if(millis() - delays[ch][1] >= data[ch][0] >> 1)
        counters[ch][0] = 0;    
  }


  void getColors(long col1, long col2, int col[2][3])
  {
    col[0][0] = colCon(col1, 'R');
    col[0][1] = colCon(col1, 'G');
    col[0][2] = colCon(col1, 'B');
  
    col[1][0] = colCon(col2, 'R') - col[0][0];
    col[1][1] = colCon(col2, 'G') - col[0][1];
    col[1][2] = colCon(col2, 'B') - col[0][2];
  }

  
  void fade_mode(byte ch, long col1, long col2, int del)
  {
    if(booleans[ch][1])    //EXIT
      return;
  
  
    if(!booleans[ch][0])   //SETUP
    {
      delays[ch][0] = millis();
  
      getColors(col1, col2, colors[ch]);
      
      booleans[ch][0] = true;
      booleans[ch][1] = false;
    }
  
    if(millis() - delays[ch][0] >= del) //TERMINATION
    { 
      booleans[ch][1] = true;
      return;
    }
  
    PWM(ch * 3, colors[ch][0][0] + colors[ch][1][0] * (map(millis() - delays[ch][0], 0, del, 0, 1000) / (float) 1000));
    PWM(ch * 3 + 1, colors[ch][0][1] + colors[ch][1][1] * (map(millis() - delays[ch][0], 0, del, 0, 1000) / (float) 1000));
    PWM(ch * 3 + 2, colors[ch][0][2] + colors[ch][1][2] * (map(millis() - delays[ch][0], 0, del, 0, 1000) / (float) 1000));
    
  }
  

  void hold_mode(int ch, long col, int del)
  {
    if(booleans[ch][1])  //EXIT
      return;
  
  
    if(!booleans[ch][0])   //SETUP
    {    
      delays[ch][0] = millis();
  
      setColor(ch, col, 1);
      
      booleans[ch][0] = true;
      booleans[ch][1] = false;
    }
  
  
    if(millis() - delays[ch][0] >= del) //TERMINATION
      booleans[ch][1] = true;
  }
};
