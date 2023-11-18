#include "AdvancedLed.h"

class AdvancedLEDcontroller{

  long frameEndTime = 0;
  AdvancedLed *leds;

  public:
  AdvancedLEDcontroller(byte led_num){

    leds = new AdvancedLed[led_num];
    
    for(int i = 0; i < led_num; i++)
      leds[i] = AdvancedLed(i);
  }

  void frame(){

    for(int i = 0; i < NUM_LEDS; i++)
      leds[i].frame();
  }
  
  void addInstruction(byte id, LedInstruction *newInstruction){

    leds[id].addInstruction(newInstruction);

    if(newInstruction->fillEndTime > frameEndTime){

      //Serial.println(newInstruction->fillEndTime);
      frameEndTime = newInstruction->fillEndTime;
    }
  }

  boolean done(){

    return frameEndTime <= millis();
  }
};
