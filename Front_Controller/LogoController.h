#include "PerlinFade.h"

class LogoController{

  AdvancedLEDcontroller *ledController;
  PerlinFade *pf;
  byte stage = 0;

  public:
  LogoController(){

    strip.begin();
    strip.setBrightness(255);
    strip.clear();
    strip.show();
  
    pf = new PerlinFade();
    ledController = new AdvancedLEDcontroller(NUM_LEDS);

    updateStage();
  }

  void frame(){

    if(stage == 4){

      pf->frame();

      strip.show();
      return;
    }
    else if(ledController->done())
      updateStage();

    ledController->frame();
    
    strip.show();
  }



  void updateStage(){

    long st = millis();
    //Serial.println(stage);
    switch(stage){

      case 0:

        for(int i = 0; i < OUT_LED; i++){
    
        ledController->addInstruction(i, new LedInstruction(st + 50 * i, st + 50 * (i + 10), 0x00ffff, 0xff00ff));
      
        if(i < 15)
          ledController->addInstruction(30 - i, new LedInstruction(st + 50 * i, st + 50 * (i + 10), 0x00ffff, 0xff00ff)); 
        }  
        stage = 1;
      break;

      case 1:
    
        for(int i = 0; i < NUM_LEDS; i++){
    
          if(i <= 15)
            ledController->addInstruction(15 - i, new LedInstruction(st + 50 * i, st+ 50 * (i + 10), 0xff00ff, 0x00ffff));
        
          if(i >= 16)
            ledController->addInstruction(i, new LedInstruction(st + 50 * i, st + 50 * (i + 10), 0xff00ff, 0x00ffff)); 
        }

        stage = 2;
      break;

      case 2:

        for(int i = 0; i < NUM_LEDS; i++)
            ledController->addInstruction(i, new LedInstruction(st, st + 1000, 0x00ffff, pf->getPerlinColor()));
          
        stage = 3;

      break;

      case 3:

        stage = 4;

      break;
    }
  }




  
};
