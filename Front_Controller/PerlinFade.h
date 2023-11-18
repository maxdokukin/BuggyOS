#define HUE_GAP 35000
#define FIRE_STEP 30
#define HUE_START 25000
#define MIN_BRIGHT 10
#define MAX_BRIGHT 255
#define MIN_SAT 180
#define MAX_SAT 255

#include <FastLED.h>

class PerlinFade{

  long lastFrame = 0;
  long counter = 0;

  public: 
  PerlinFade(){}
  
  
  void frame(){

    if(millis() - lastFrame < 10)
      return;

    for(int i = 0; i < NUM_LEDS; i++)
      strip.setPixelColor(i, getFireColor((inoise8(i * FIRE_STEP, counter))));

    counter += 5;
    
    lastFrame = millis();
  }

  long getFireColor(int val) {

    //Serial.print(HUE_START + map(val, 0, 255, 0, HUE_GAP)); Serial.print("  "); Serial.print(constrain(map(val, 0, 255, MAX_SAT, MIN_SAT), 0, 255)); Serial.print("  "); Serial.println(constrain(map(val, 0, 255, MIN_BRIGHT, MAX_BRIGHT), 0, 255));
    return strip.ColorHSV(
             HUE_START + map(val, 0, 255, 0, HUE_GAP),                    // H  
             constrain(map(val, 0, 255, MAX_SAT, MIN_SAT), 0, 255),       // S
             constrain(map(val, 0, 255, MIN_BRIGHT, MAX_BRIGHT), 0, 255)  // V   
           );
           
    //return strip.ColorHSV(HUE_START + map(val, 0, 255, 0, HUE_GAP), 255, 255);

     
  }

  long getPerlinColor(){

    return getFireColor((inoise8(0, 0)));
  }
  
};
