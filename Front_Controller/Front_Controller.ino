#include "Adafruit_NeoPixel.h"

#define OUT_LED 16
#define IN_LED 15
#define NUM_LEDS 31

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, 5, NEO_GRB + NEO_KHZ800);

#include "AxillaryFnc.h"
#include "AdvancedLEDcontroller.h"
#include "LogoController.h"
#include "StripsController.h"



LogoController *logoController;
StripsController *stripsController;



void setup() {

  logoController = new LogoController();
  stripsController = new StripsController();
}


void loop() {

  logoController->frame();

  if(digitalRead(6))
    stripsController->frame();  
    
  else if(!stripsController->resetted)
    stripsController->resetAll();
}
