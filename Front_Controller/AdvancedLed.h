#include "LedInstruction.h"

class AdvancedLed{

  byte id;
  LedInstruction *currentInstruction = NULL;



  public: 
  
  AdvancedLed(){}
  
  AdvancedLed(byte i){

    id = i;
  }



  void frame(){

    if(done() || currentInstruction->fillStartTime > millis())
      return;

    
    long curTime = millis();
    
    if(curTime >= currentInstruction->fillEndTime){

      moveQue();
      return;
    }

    float brightness = map(curTime - currentInstruction->fillStartTime, 0, currentInstruction->fillEndTime - currentInstruction->fillStartTime, 0, 255) / (float) 255;
            
    strip.setPixelColor(id, getMixCol(currentInstruction->startColor, currentInstruction->endColor, brightness)); 
  }

  boolean done(){

    return currentInstruction == NULL;
  }
  
  void addInstruction(LedInstruction *newInstruction){

    if(currentInstruction == NULL){

      currentInstruction = newInstruction;
      return;
    }
    
    LedInstruction *freeSpace = currentInstruction;

    while(freeSpace->next != NULL)
      freeSpace = freeSpace->next;

    freeSpace->next = newInstruction;
  }


  void moveQue(){

    LedInstruction *nextInstruction = currentInstruction->next;
    
    //Serial.print("deleting insruction "); Serial.print(currentInstruction->startColor); Serial.print("   " ); Serial.print(freeMemory());
    delete currentInstruction;
    //Serial.print("   to   "); Serial.println(freeMemory());

    currentInstruction = nextInstruction;
  }
};
