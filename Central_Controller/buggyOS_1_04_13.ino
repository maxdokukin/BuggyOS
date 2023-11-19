/*
Buggy OS version 1.04.13

включает:
обработка отключения питания
управление коробкой передач
управление командами через сериальный порт
режим отладки debug
подсчет скорости
подсчет RPM
подсчет пробега
подсчет моточасов
подсчет поездки
обработку кнопки сброса поездки
управление заслонкой по датчику температуры двигателя
**Вывод предупреждений на матрицу:
контроль датчика топлива на малый уровень
контроль датчика уровня тормозной жидкости
контроль датчика парковочного тормоза
контроль датчика уровня масла
контроль включенного дальнего света -----  ТРЕБУЕТ ДОРАБОТКИ ПО ПОРТАМ 

*/
bool debug=false;                             //режим отладки используется для вывода контрольных данных на сериальный порт

#include "EEPROM.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>
#include "LedControlMS.h"
#include "icons.h"
#include "serialConsole.h"

//******************************* распределение PIN ARDUINO *******************************

#define ONE_WIRE_BUS 20                       //pin датчиков температуры
#define CHOKE_SERVO_PIN 6                     // pin сервомашинка подсоса
#define TRIP_RESET_PIN A4                     // pin TRIP RESET BUTTON
#define HALL_SPEED_INT 0                      //pin 2 датчик скорости
#define HALL_ENGINE_INT 1                     //pin 3 датчик оборотов двигателя
#define ENGINE_RUN_PIN 31                     //pin двигатель работает
#define BRAKE_SENSOR 36                       //PIN датчик педали тормоза
#define PARKING_BRAKE_SENSOR 28               // датчик стояночного тормоза
#define MATRIX_DIN 29                         //PIN для матрицы
#define MATRIX_CLK 27                         //PIN для матрицы
#define MATRIX_CS 25                          //PIN для матрицы
#define POWER_ON A0                           //Pin питание сети включено
#define GEAR_BUTTON_PIN A1                    //PIN кнопок переключения скорости аналоговый
#define GEAR_SENSOR_PIN A2                    //PIN датчиков коробки передач аналоговый
#define GEAR_SWITCH_FORWARD 4                 //pin переключить в сторону передней скорости
#define GEAR_SWITCH_REAR 5                    //pin переключить в сторону задней скорости
#define OIL_SENSOR 30                         //PIN давление масла
#define FUEL_SENSOR A3                        //pin датчик топлива 0-1 вольт
#define LOW_BEAM 26                           //pin ближний свет             //  ПРОВЕРИТЬ ПИН!!!!!!!!!!!!!!!                         
#define HIGH_BEAM 24                          //pin дальний свет            //  ПРОВЕРИТЬ ПИН!!!!!!!!!!!!!!!
#define BRAKE_LIQUID 34                       //pin датчик торм жидк

//******************************** константы ******************************************

#define FORWARD_GEAR 0                          // перключаем коробку в сторону передней скорости
#define REAR_GEAR 2                             // перключаем коробку в сторону задней скорости
#define NEUTRAL_GEAR 1                          // перключаем коробку в сторону нейтральной скорости
#define NO_GEAR 3                               //скорость коробки передач не включена
#define FORWARD_VOL 5                           //напряжение пина при передней скорости
#define REAR_VOL 3.5                            //напряжение пина при задней скорости
#define NEUTRAL_VOL 4.2                         //напряжение пина при нейтральной скорости
#define GEAR_VOL 0.3                            //допуск значений напряжения переключателя скорости
#define GEAR_SHIFT_TIME 500                     //максимальное время на переключение скорости после которого будет ошибка
#define SWITCH_GEAR_ATTEMTPS 2                  //количество попыток включить скорость при ошибке переключения

LedControl lc=LedControl(MATRIX_DIN,MATRIX_CLK,MATRIX_CS,1);   // декларируем класс управления матрицей
byte matrixQue = B00000000;                      // очередь поочередно показываемых иконок
byte currentQue = 0;                              // номер показываемой иконки
unsigned long oldTimeIcon;                         //для счетчика времени
#define MAX_ICON_QUE 8                            //количество видов иконок (максимум можно 8)
#define ICON_DISPLAY_TIME 3000                   //ДЛИТЕЛЬНОСТЬ ПОКазывания иконки
const byte* matrixIcons[MAX_ICON_QUE] = {(byte*)iconOil, (byte*)iconChoke,(byte*)iconPark,(byte*)iconFuel,(byte*)iconGear,(byte*)iconLights,(byte*)iconArrow,(byte*)iconBrake};
byte iconType[MAX_ICON_QUE] = {0,0,0,0,0,0,0,0};    //выбор вторичного типа иконки для соответсвующей иконки в массиве иконок
int iconMultCount = 0;                                  //счетчик кадров анимации иконок
#define MULT_TICKS 4                                   //частота смены кадров анимации в течении ICON_DISPLAY_TIME

unsigned long oldTimeWarnings = 0;
#define WARNINGS_DELAY 300                            //как часто опрашивать датчики вызывающие сигналы предупреждения с выводом на матрицу

OneWire oneWire(ONE_WIRE_BUS);                       // декларируем класс управления датчиками температуры
DallasTemperature sensors(&oneWire);                  //инициализация сенсоров температуры
DeviceAddress engineTemp = { 
  0x28, 0xFF, 0xDA, 0x15, 0xA5, 0x16, 0x03, 0x75 };  // адрес датчика DS18B20 28FFDA15A5160375  - это датчик температуры двигателя
DeviceAddress oilTemp = { 
  0x28, 0xFF, 0x7E, 0xD9, 0xA4, 0x16, 0x03, 0xD9 }; // адрес датчика DS18B20 28FF7ED9A41603D9  - это датчик температуры масла
float tempC=0;                                      // текущая температура двигателя
#define TEMP_TIME_DELAY 5000                        //частота считывания температуры двигателя млсек
int oldTimeTemp = 0; 

#define CHOKE_TEMP_MIN 27                         //нижняя температура когда возд заслонка полностью закрыта
#define CHOKE_TEMP_MAX 50                         //верхняя температура когда заслонка полностью открыта

Servo myservo;                                    //декларируем класс управления сервопроводом
int servoPosition = 0;                              //положение серво подсоса
bool autoChoke = true;                          //автоуправление заслонкой либо ручное если false

#define FUEL_LOW 0.2                            //критичный уровень датчика топлива в вольтах

#define MILEAGE_EEPROM 0                        //long 4 байта пробег метры
#define MILEAGE_TRIP_EEPROM 4                  //long пробег TRIP
#define HOURS_ENGINE_EEPROM 8                   //long моточасы

#define TRIP_BUTTON_DELAY 2000                  //сколько надо держать нажатой кнопку сброса поездки
int timeTripResetOld=0;                         //для отсчета времени нажания кнопки сброса TRIP
bool tripResetFlag= false;                      //флаг нажатия кнопки сброса TRIP

#define WHEEL_LENGTH (0.6*2*3.14)               //окружность колеса длина в метрах

volatile float wheelSpeed=0;                    //скорость км час
volatile float engineSpeed=0;                   //RPM

volatile unsigned long previousTimeSpeed=0;    //предыдущее время срабатывания датчика скорости
volatile float mileageStart=0;                  //пробег с момента включения в метрах
volatile float mileageTripStart=0;               //пробег TRIP с момента включения метры

volatile unsigned long mileage;                          //пробег на момент включения питания метры
volatile unsigned long mileageTrip;                      //пробег TRIP на момент включения метры
volatile unsigned int engineHours;                       //моточасы на момент включения
volatile unsigned long previousTimeEngine=0;    //предыдущее время срабатывания датчика оборотов

unsigned long runTimeEngine=0;                //моточасы работы двигателя с момента включения
unsigned long oldTimeEngine=0;                //предыдущеее время работы двигателя нужно для вычисления моточасов

byte currentGear = NEUTRAL_GEAR;                 //последнее положение коробки передач НЕЙТРАЛЬНОЕ=1 ВПЕРЕД=0 НАЗАД=2
byte shiftResult = NEUTRAL_GEAR;               //положение коробки или код ошибки коробки

#define ERR_RN 254                               // коды ошибок коробки передач
#define ERR_NF 253                               // позволют определить между какими скоростями коробка зависла
#define ERR_FN 252
#define ERR_NR 251

unsigned long oldTimeDisplay=0;               //на время отладки для периодического вывода данных на серийный порт
#define DISPLAY_DELAY 2000

bool geardebug = false;                       //используется длля задержки вывода инфо коробки в режиме debug



//****************************************************************************************************************************
//****************************************************************************************************************************
//****************************************************************************************************************************
//**********************************ИНИЦИАЛИЗАЦИЯ ***************************************************************************

void setup() {

  Serial.begin(9600);                                         //УБРАТЬ ПОСЛЕ ОТЛАДКИ!!!!!! запустить последовательный порт
  while (!Serial);                                             //УБРАТЬ ПОСЛЕ ОТЛАДКИ!!!!!! запустить последовательный порт

  lc.shutdown(0,false);                                         //включить матрицу
  lc.setIntensity(0,8);                                         //УСТАНОВИТЬ ЯРКОсть матрицы
  lc.clearDisplay(0);                                           //очистить матрицу
  
  attachInterrupt(HALL_SPEED_INT, hallSpeedInt, FALLING);        //установить прерывание датчика скорости
  attachInterrupt(HALL_ENGINE_INT, hallEngineInt, FALLING);        //установить прерывание датчика RPM
//  attachInterrupt(POWER_OFF_INT, powerOff, FALLING);          // УБРАТЬ КОММЕННТ ПОСЛЕ ОТЛАДКИ!!! закоментил чтобы не тратить EEPROM во время отладки e

  sensors.begin();
  sensors.setResolution(engineTemp, 10);                      //датчик темп мотора чувствительность
  sensors.setResolution(oilTemp, 10);                          //датчик темп масла чувствительность

  myservo.attach(CHOKE_SERVO_PIN);                            // attaches the servo on pin to the servo object
  myservo.write(0);                                           // sets the servo position 0


  mileage = readLongEeprom(MILEAGE_EEPROM);                        //при включении считать данные из EEPROM
  mileageTrip = readLongEeprom(MILEAGE_TRIP_EEPROM);                //при включении считать данные из EEPROM
  engineHours = readLongEeprom(HOURS_ENGINE_EEPROM);               //при включении считать данные из EEPROM

  Serial.print("Welcome to Buggy OS version 1.04.13!\n>");             

  pinMode(ENGINE_RUN_PIN,INPUT); 
  pinMode(TRIP_RESET_PIN,INPUT); 
  pinMode(BRAKE_SENSOR,INPUT_PULLUP);
  pinMode(PARKING_BRAKE_SENSOR,INPUT_PULLUP);
  pinMode(OIL_SENSOR, INPUT_PULLUP);
  pinMode(FUEL_SENSOR, INPUT);
  pinMode(BRAKE_LIQUID, INPUT_PULLUP);
  pinMode(HIGH_BEAM, INPUT);
  pinMode(LOW_BEAM, INPUT);
  pinMode(GEAR_SWITCH_FORWARD,OUTPUT);
  pinMode(GEAR_SWITCH_REAR,OUTPUT); 
  digitalWrite(GEAR_SWITCH_FORWARD,LOW);
  digitalWrite(GEAR_SWITCH_REAR,LOW);


  currentGear = detectGear();

  if ( currentGear != NO_GEAR ) currentGear=switchGear( NEUTRAL_GEAR );    //если нет ошибки коробки то включить нейтралку

//******** надо добавить обработку события если нейтралку не удалось включить. например установить запрет пуска двигателя через пин ********************!!!!!!!!!!!!!!!!!!

  if ( currentGear < NO_GEAR ) iconType[GEAR] = currentGear;    //установить иконку скорости
  else iconType[GEAR] = NO_GEAR;

digitalWrite(ENGINE_RUN_PIN,HIGH);                               // УБРАТЬ ПОСЛЕ ОТЛАДКИ!!!!   имитирует запущенный двигатель для отладки только!!!!1
}

//****************************************************************************************************************************
//****************************************************************************************************************************
// *********************************Основная программа ***********************************************************************
unsigned long cycle,timer;

void loop() {

  byte gear, i = 0;
  float pinVoltage=0;
  char command[MAX_COMMAND_LENGTH+1]="";                    //строка для считывания комманд с сериального порта каждый цикл стирается

  cycle=millis()-timer;
  timer = millis();                           //сохранить текущее время с момента включения каждый цикл

/*
  if ( !analogRead(POWER_ON) ) {                            // если напряжение в сети пропало то сохранить все данные
    powerOff();
    delay(3000);                                            //задержка 3 сек чтоб питание ардуино отключилось, если успеет восстановиться то программа продолжиться
  }
*/

//***********************обработка данных(комманд) поступивших в сериальный порт ***********************
  if (Serial.available()) {
    i=0;
    while ( Serial.available() && i < (MAX_COMMAND_LENGTH+1)) {   //считываем данные из сериального порта если они есть но не длиннее лимита длины команды
      delay(5);
      command[i] = Serial.read();                 //считать один байт из буфера порта в строку command
     i++;
    }
    command[i] == 0;                              // ставим конец строки
    Serial.flush();                               // сброс лишних данных порта
    Serial.println(command);

    if ( strlen(command) !=0 ) executeCommand( i=processCommand( command ) );      // если полученная строка не пуста то проверяем ее на совпадение с командой и выполняем

  }

//******************управление коробкой передач*********************************************************

  pinVoltage = (float)analogRead( GEAR_BUTTON_PIN )/200;


  if (  pinVoltage > 1 && !digitalRead( BRAKE_SENSOR ) && !wheelSpeed ) {           //ЕСЛИ Нажат тормоз, скорость ноль и нажата любая кнопка коробки

if (debug) {                                  //только в режиме отладки
  Serial.print("Voltage is: ");              //УБРАТЬ после отладки!!!!!!!!!
  Serial.println(pinVoltage);
}

   if ( pinVoltage > (float)(REAR_VOL - GEAR_VOL) && pinVoltage < (float)(REAR_VOL + GEAR_VOL) ) gear = REAR_GEAR;   //определяем нажатую кнопку с учетом допуска напряжения
    else if ( pinVoltage < (float)(NEUTRAL_VOL + GEAR_VOL) ) gear = NEUTRAL_GEAR;
          else gear = FORWARD_GEAR;

   if ( currentGear == ERR_FN | currentGear == ERR_NF) {               // Если предыдущая попытка ошибка то даем ориентир для направления переключения
            if (gear == FORWARD_GEAR) currentGear = NEUTRAL_GEAR;
            else currentGear = FORWARD_GEAR;
   }
 
   if ( currentGear == ERR_RN | currentGear == ERR_NR) {               // Если предыдущая попытка ошибка то даем ориентир для направления переключения
            if (gear == REAR_GEAR) currentGear = NEUTRAL_GEAR;
            else currentGear = REAR_GEAR;
   }


    if ( gear != currentGear ){                                //переключаем скорость только если выбрана новая скорость

if (debug) { Serial.print("Shifting gear to: ");              //УБРАТЬ после отладки!!!!!!!!!
      Serial.println(gear);
}
      shiftResult = switchGear( gear );                       // shiftResult присвоится номер включенной скорости либо код ошибки

if (debug) { Serial.print("Shift result: ");
      Serial.println(shiftResult);                  //УБРАТЬ после отладки!!!!!!!!!
}
      if ( shiftResult != gear ) {                            //это выполнится если скорость не удалость переключить на выбранную gear

if (debug) Serial.println("Error shifting, fixing...");  //УБРАТЬ после отладки!!!!!!!!!

        switch ( shiftResult ) {                            // откатываем на последнюю успешно включенную скорость

          case ERR_FN:

            shiftResult = switchGear( FORWARD_GEAR );

          break;
 
          case ERR_NF:

            currentGear = FORWARD_GEAR;
            shiftResult = switchGear( NEUTRAL_GEAR );

          break;

          case ERR_RN:
            
            shiftResult = switchGear( REAR_GEAR );

          break;

          case ERR_NR:
            
            currentGear = REAR_GEAR;
            shiftResult = switchGear( NEUTRAL_GEAR );

          break;
          
        }
if (debug) {
Serial.print("Switched to: ");  //УБРАТЬ после отладки!!!!!!!!!
Serial.println(shiftResult);  //УБРАТЬ после отладки!!!!!!!!!
}
        currentGear = shiftResult;        // присваиваем новое значение скорости


      }

      currentGear = shiftResult;        // присваиваем новое значение скорости

    }
  }

  digitalWrite(GEAR_SWITCH_FORWARD,LOW);                   // на всякий случай убедиться что мотор коробки отключен
  digitalWrite(GEAR_SWITCH_REAR,LOW);                     // *****

//*********************конец обработки переключения коробки скоростей*************************************

//************c интервалом WARNING_DELAY опросить датчики вызывающие предупреждения и установить очередь предупреждений************

  if ((timer-oldTimeWarnings) >= WARNINGS_DELAY) {
    matrixQue = requestWarnings();        
    currentGear = detectGear();                                   //определить скорость
    if ( currentGear < NO_GEAR ) iconType[GEAR] = currentGear;    //установить тип иконки GEAR для отображения
    else iconType[GEAR] = NO_GEAR;
    oldTimeWarnings = timer;
  }

//******************* анимированный вывод предупреждений на матрицу **********************************************************
  if ( (timer - oldTimeIcon) > (unsigned long)(ICON_DISPLAY_TIME/MULT_TICKS) ) {    //таймер вывода матрицу

    if ( matrixQue ) displayIconQue();                  //отобразить иконку из очереди если очередь не пуста

    //здесь делаем поочередно меняющиеся иконки одного типа для мультиков
    iconType[CHOKE] = !iconType[CHOKE];                 //меняем активную иконку в кадре для мультика
    iconType[BRAKE] = !iconType[BRAKE];             //меняем активную иконку в кадре
    iconType[OIL] = !iconType[OIL];                     //меняем активную иконку
    iconType[FUEL] = !iconType[FUEL];                     //меняем активную иконку

    if ( ++iconMultCount >= MULT_TICKS ) {              //держим очередь mult_ticks раз
      currentQue++;                                     //двигаем очередь вперед
      iconMultCount = 0;                                //сброст счетчика кадров
    }
    
    if ( currentQue >= MAX_ICON_QUE ) currentQue = 0;   //сброс счетчика очереди по достижении максимума

    oldTimeIcon = timer;
  }

//*****************************************************************************************************

//******************** вывод данных на LCD *************************************************************
//*****************************************************************************************************

//******************счетчик моточасов*******************************************************************
  if (digitalRead(ENGINE_RUN_PIN)) {                        //если двигатель работает
    runTimeEngine += (timer - oldTimeEngine);                //добавить время работы
  }
  
  oldTimeEngine = timer;                                     //если от датчика нет сигнала то моточасы не добавляются
//*****************************************************************************************************

//*********** если сигналов от датчиков скорости нет то обнуление скоростей*****************************************
  if ( (timer - previousTimeSpeed) > 4500 ) wheelSpeed = 0;      //если от датчика нет сигнала более 4.5 сек то скорость ноль
  if ( (timer - previousTimeEngine) > 100 ) engineSpeed = 0;      //если от датчика нет сигнала более 0.1 сек то скорость ноль
//*****************************************************************************************************
  
//**********управление заслонкой в зависимости от температуры двигателя********************************
  
  if (autoChoke && ((timer-oldTimeTemp) >= TEMP_TIME_DELAY)) {      //раз в TEMP_TIME_DELAY мерить темпер двигателя и управлять заслонкой
    sensors.requestTemperatures();  
    tempC = sensors.getTempC(engineTemp); 

    servoPosition = map((int)tempC, CHOKE_TEMP_MIN, CHOKE_TEMP_MAX, 0, 180);     // scale it to use it with the servo (value between 0 and 180)
    if (servoPosition <= 0) servoPosition=0;
    if (servoPosition >= 180) servoPosition=180;

    myservo.write(servoPosition);                  // установить серво в положение пропорционально температуре
    
    oldTimeTemp = timer;                           // СБРОСИТЬ ТАЙМЕР задержки измерения температуры
  }
//*****************************************************************************************************

//**********************Управление кнопкой сброса TRIP *************************************************  
  if ((float)analogRead(TRIP_RESET_PIN)/200 > 4){                  //если сброс нажат ДОРАБОТАТЬ!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if ( tripResetFlag == false ){                  // и если это только что нажали
      timeTripResetOld = timer;                      // начать отсчет времени нажатия
      tripResetFlag = true;                        //установить флаг что нажата
    }
    else {                                         //если сброс нажат не только что
      if ( ( timer - timeTripResetOld ) > TRIP_BUTTON_DELAY ){  // и дольше секунды
        saveLongEeprom(MILEAGE_TRIP_EEPROM,0);       //обнулить счечик TRIP в памяти
        mileageTrip = mileageTripStart = 0;       //обнулить счечик TRIP
if (debug) Serial.println("Сброс TRIP");
      }     
      
    }
    
  }
  else tripResetFlag = false;                   //если кнопка сброса не нажата или отпущена то сброс флага
//*****************************************************************************************************



// ******************** вывод данных в сериальный порт только в режиме отладки команда - debug ********
if (debug) {
  if ( (timer - oldTimeDisplay) > DISPLAY_DELAY ) {

    Serial.print("Gear:");
    Serial.println(currentGear);
    Serial.print("Cycle time: ");
    Serial.println(cycle);

  if ( wheelSpeed ){
    Serial.print("Speed:");
    Serial.println(int(wheelSpeed));
    Serial.print("Distance:");
    Serial.println(int(mileageStart));
    wheelSpeed = 0;
  
  }
  if ( engineSpeed ){
    Serial.print("Engine RPM:");
    Serial.println(int(engineSpeed));
    engineSpeed = 0;
  }
  oldTimeDisplay = timer; 
 
  }
}


}
//конец основной программы ****************************************************************************************

//********************************выполнение команды из списка команд управления********************
bool executeCommand( byte i){
  
  unsigned long paramL = (unsigned long)atol(commandParameter);
  unsigned int paramI = (unsigned int)atoi(commandParameter);

  
  switch ( i ) { 
      
    //*********************** здесь команды которые не входят в команду ALL  
    case 1:
      if ( paramL > 0 ) {
        Serial.print("EEPROM mileage set to: ");
        saveLongEeprom(MILEAGE_EEPROM,paramL);
        mileage = readLongEeprom(MILEAGE_EEPROM);
        Serial.println(mileage);
      }
      else Serial.println("Invalid parameter");
    break;

    case 4:
      if ( paramL > 0 ) {
        Serial.print("EEPROM engine hours set to: ");
        saveLongEeprom(HOURS_ENGINE_EEPROM,paramL);
        engineHours = readLongEeprom(HOURS_ENGINE_EEPROM);
        Serial.println(engineHours);
      }
      else Serial.println("Invalid parameter");
    break;

    case 16:
      saveToEeprom();
      asm volatile("jmp 0x00");
    break;
    
    case 22:

      Serial.println( helpCommand );

    break;
 
    case 23:

      debug = !debug;
      Serial.print("Debug mode: ");
      if ( debug ) Serial.println("ON");
      else Serial.println("OFF");
    break;
    
    case 24:
      if (!strcmp(commandParameter, "save")) {
        saveToEeprom();
        Serial.println("Saved to EEPROM");
      }
      else if (commandParameter[0] == 0 ) {
              Serial.print("EEPROM Mileage: ");
              Serial.println(readLongEeprom(MILEAGE_EEPROM));
              Serial.print("EEPROM Trip: ");
              Serial.println(readLongEeprom(MILEAGE_TRIP_EEPROM));
              Serial.print("EEPROM Engine hours: ");
              Serial.println(readLongEeprom(HOURS_ENGINE_EEPROM));
              }
            else Serial.println("Invalid parameter");
    break;
    
    case 255:

      Serial.println( "No such command" );

    break;

    // ************************************************    //если комманда ALL- то далее выполнятся все команды ниже которые выводят параметры
    case 15:
        if (digitalRead(ENGINE_RUN_PIN)) Serial.println("Engine is running");
        else Serial.println("Engine is idle");
        Serial.print("Cycle time: ");
        Serial.println(cycle);
    case 0:

      Serial.print("Mileage: ");
      Serial.println((float)(mileage+(unsigned long)mileageStart)/1000);
    if ( i != 15 ) break;                                                 //если отрабатывается команда ALL то выполняются все нижеслдующие команды

    case 2:
      if ( commandParameter[0] == 0 ) {
       Serial.print("Trip: ");
       Serial.println((float)(mileageTrip+(unsigned long)mileageTripStart)/1000);
      }
      else if (!strcmp(commandParameter, "reset")) {      
              saveLongEeprom(MILEAGE_TRIP_EEPROM,0);       //обнулить счечик TRIP в памяти
              mileageTrip = mileageTripStart = 0;       //обнулить счечик TRIP
              Serial.println("Trip is reset");
              }
           else Serial.println("Invalid parameter");
    if ( i != 15 ) break;

    case 3:
      Serial.print("Engine hours: ");
      Serial.println((float)(engineHours+(unsigned int)(runTimeEngine/60000))/60);
    if ( i != 15 ) break;

    case 5:
      Serial.print("Speed: ");
      Serial.println(int(wheelSpeed));
    if ( i != 15 ) break;

    case 6:
      Serial.print("RPM: ");
      Serial.println(int(engineSpeed));
    if ( i != 15 ) break;

    case 7:
      Serial.print("Brake pedal: ");
      if ( digitalRead( BRAKE_SENSOR ) ) Serial.println("OFF");
      else Serial.println("ON");
    if ( i != 15 ) break;
    
    case 8:
      Serial.print("Oil level: ");
      if ( digitalRead( OIL_SENSOR ) ) Serial.println("OK");
      else Serial.println("LOW!!!");
    if ( i != 15 ) break;

    case 9:
      if ( commandParameter[0] == 0 ) {
        Serial.print("Current gear: ");
        if ( currentGear == 0) Serial.println( "Drive");
        else if ( currentGear == 1 ) Serial.println( "Neutral");
              else if ( currentGear == 2 ) Serial.println( "Rear");
                    else {
                      Serial.print( "Error ");
                      Serial.println( shiftResult );
                    }
      }
      else if ( (strlen(commandParameter) == 1) && ((commandParameter[0] - 48) < 3) ) {         //если параметер это один символ и он цифра меньше 3х
              currentGear = shiftResult = switchGear( paramI );
              Serial.print("Shift result: ");
              Serial.println(shiftResult);
          }
           else Serial.println("Invalid parameter");
    if ( i != 15 ) break;
  
    case 10:    //gearsensor
       if (!strcmp(commandParameter, "debug")) {
        geardebug = !geardebug;
        Serial.print("Gear debug mode: ");
        if ( geardebug ) Serial.println("ON");
        else Serial.println("OFF");
       }
       Serial.print("Gear sensor: ");
        switch( detectGear()){
        case 0:
          Serial.println( "Drive");
        break;
        case 1:
         Serial.println( "Neutral");
        break;
        case 2:
         Serial.println( "Rear");
        break;
        default:
         Serial.println( "No position");
        }
    if ( i != 15 ) break;
  
    case 11:
      Serial.print("Fuel level (%): ");
      Serial.println((int)analogRead(FUEL_SENSOR)/10);
    if ( i != 15 ) break;
    
    case 12:
      Serial.print("Parking brake: ");
      if ( digitalRead( PARKING_BRAKE_SENSOR ) ) Serial.println("OFF");
      else Serial.println("ON");
    if ( i != 15 ) break;

    case 13:      //choke
      if (!strcmp(commandParameter, "auto")) {
        autoChoke = true;
        Serial.println("Choke is auto");
      }
      else if (!strcmp(commandParameter, "manual") ) {
              autoChoke = false;
              Serial.println("Choke is manual");
              }
           else if (commandParameter[0] == 0 ) {
                Serial.print("Choke servo angle is: ");
                Serial.println(myservo.read());
                if ( autoChoke ) Serial.println("Choke is auto");
                else Serial.println("Choke is manual");
                }
                else {
                byte s = atoi(commandParameter);
                  if ( s > 0 and s < 180 ) {
                        myservo.write(s);
                        Serial.print("Choke is in manual mode and servo ange set to: ");
                        Serial.println(s);
                        autoChoke = false; 
                        }
                     else Serial.println("Invalid parameter");
                }
    if ( i != 15 ) break;

    case 14:    //oiltemp      
      Serial.print("Oil temperature: ");
      Serial.println(sensors.getTempC(oilTemp));
    if ( i != 15 ) break;

    case 17:
      Serial.print("Engine temperature: ");
      Serial.println(tempC);
    if ( i != 15 ) break;

    case 18:
      Serial.print("Brake liquid level: ");
      if ( digitalRead( BRAKE_LIQUID ) ) Serial.println("OK");
      else Serial.println("LOW!!!");
    if ( i != 15 ) break;

    case 19:
      Serial.print("Matrix que: ");
      Serial.println(matrixQue, BIN);
    if ( i != 15 ) break;

    case 20:
      Serial.print("Voltage: ");
      Serial.println((float)analogRead(POWER_ON)/200);
    if ( i != 15 ) break;

    case 21:
      Serial.print("On-time (min): ");
      Serial.println( (float)millis()/60000 );
    if ( i != 15 ) break;

    }

  Serial.print(">");
  return true;
}

//****************************// опрос датчиков и установка индикации предупреждений *****************************
byte requestWarnings(void) {

  byte que = B00000000;

  if (wheelSpeed < 5 ) que = que | (1<<GEAR);
  if (!digitalRead(OIL_SENSOR)) que = que | (1<<OIL);
  if (!digitalRead(PARKING_BRAKE_SENSOR)) que = que | (1<<PARK);
  if (myservo.read() > 10)  que = que | (1<<CHOKE);
  if ((float)analogRead(FUEL_SENSOR)/1000 < FUEL_LOW ) que = que | (1<<FUEL);
  if (!digitalRead(BRAKE_LIQUID)) que = que | (1<<BRAKE);
  if (digitalRead(HIGH_BEAM)) {
    que = que | (1<<LIGHTS);
    iconType[LIGHTS] = 1;
  }
  else if (digitalRead(LOW_BEAM)) {
    que = que | (1<<LIGHTS);
    iconType[LIGHTS] = 0;     
    }

  if ( currentGear < NO_GEAR ) iconType[GEAR] = currentGear;    //установить иконку скорости
  else iconType[GEAR] = NO_GEAR;

return que;
}

//************отобразить иконку текущей очереди если она активна(1) если неактивна(0) то пропускать очередь пока не найдем активную иконку и отображаем ее*******

void displayIconQue(void){

  if ( !matrixQue ) return;                                                  // защита от пустой очереди

  while (((0b00000001 << currentQue) & matrixQue) == 0 ) {                  //ищем следующюю не пустую позицию в очереди
    if ( ++currentQue >= MAX_ICON_QUE ) currentQue = 0;
  }

  displayMatrixIcon(matrixIcons[currentQue] + 8*iconType[currentQue]);       //отобразить иконку соответствующую очереди
  
}

void displayMatrixIcon(const byte* icon){           //отобразить иконку на матрице

  byte i = 0;

  for (i=0; i < 8; i++){

    lc.setRow(0, i, icon[i]);
  }
  
}

//********************** переключаем скорость, возвращаем номер включенной скорости либо 255 если не удалось определить результат *********
byte switchGear(byte gear){

  byte sensor;
  unsigned long timer = millis();

  if ( gear != currentGear) {
  
     switch ( gear ){

      case FORWARD_GEAR:    //передняя **************
      
      if ( currentGear == REAR_GEAR ) shiftResult = ERR_RN; //если переключаем с задней то присваиваем код потенциальной ошибки при зависании недоходя до нейтралки
      else shiftResult = ERR_NF;

      digitalWrite(GEAR_SWITCH_FORWARD, HIGH);           //даем питание на мотор коробки

if (debug) Serial.println("GEAR MOVE FORWARD!");  

      while ( millis() < (timer + GEAR_SHIFT_TIME) ) {   // ждем сигнал датчика передней скорости в течении времени GEAR_SHIFT_TIME

        sensor = detectGear();

        if ( currentGear == REAR_GEAR && sensor == NEUTRAL_GEAR ){
          shiftResult = ERR_NF;                                // если переключаем с заднейдней то следим за прохождением нейтральной и присваиваем другой код ошибки
          currentGear = NEUTRAL_GEAR;                          // и присваиваем последнюю успешно включенную скорость нейтралка
        }
        if ( sensor == FORWARD_GEAR ) {                  //если появился сигнал с датчика передней скорости то 
          digitalWrite(GEAR_SWITCH_FORWARD, LOW);        //отключить питание на мотор коробки
          return FORWARD_GEAR;                           //вернуть успешное включение передней скорости      
        }
      }
      
      digitalWrite(GEAR_SWITCH_FORWARD, LOW);                    //отключить питание на мотор коробки

      return shiftResult;                                        // вернуть код ошибки- не удалось включить переднюю скорость
      break;

      case REAR_GEAR:       //задняя*****************

      if ( currentGear == FORWARD_GEAR ) shiftResult = ERR_FN; //если переключаем с передней то присваиваем код потенциальной ошибки при зависании недоходя до нейтралки
      else shiftResult = ERR_NR;

      digitalWrite(GEAR_SWITCH_REAR, HIGH);                   //даем питание на мотор коробки
 
 if (debug) Serial.println("GEAR MOVE REAR!");  

      while ( millis() < (timer + GEAR_SHIFT_TIME) ) {        // ждем сигнал датчика задней скорости в течении времени GEAR_SHIFT_TIME

        sensor = detectGear();

        if ( currentGear == FORWARD_GEAR && sensor == NEUTRAL_GEAR ){  // если переключаем с передней то следим за прохождением нейтральной и присваиваем другой код ошибки
          shiftResult = ERR_NR; 
          currentGear = NEUTRAL_GEAR;                          // и присваиваем последнюю успешно включенную скорость нейтралка
        }
        
        if ( sensor == REAR_GEAR ) {                    //если появился сигнал с датчика задней скорости то 
          digitalWrite(GEAR_SWITCH_REAR, LOW);          //отключить питание на мотор коробки
          return REAR_GEAR;                             //вернуть успешное включение задней скорости      
        }
      }
      
      digitalWrite(GEAR_SWITCH_REAR, LOW);                    //отключить питание на мотор коробки

      return shiftResult;                                       // вернуть код ошибки- не удалось включить заднюю скорость
      break;

      case NEUTRAL_GEAR:     //нейтралка ************
      
      if ( currentGear == FORWARD_GEAR ) sensor = GEAR_SWITCH_REAR;  //определяем в какую сторону переключать и присваиваем sensor номер порта включения мотора коробки
      else sensor = GEAR_SWITCH_FORWARD;

      digitalWrite(sensor, HIGH);                       //даем питание на мотор коробки

if (debug) Serial.println("GEAR MOVE NEUTRAL!");  

      while ( millis() < (timer + GEAR_SHIFT_TIME) ) {   // ждем сигнал датчика нейтральной скорости в течении времени GEAR_SHIFT_TIME

        if ( detectGear() == NEUTRAL_GEAR ) {            //если появился сигнал с датчика нейтральной скорости то 
          digitalWrite(sensor, LOW);                    //отключить питание на мотор коробки
          return NEUTRAL_GEAR;                           //вернуть успешное включение нейтральной скорости      
        }
      }
      digitalWrite(sensor, LOW);                    //отключить питание на мотор коробки

      if ( currentGear == FORWARD_GEAR ) return ERR_FN;     // вернуть код ошибки- не удалось преключить с передней на нейтралку
      else return ERR_RN;                                   // вернуть код ошибки- не удалось преключить с задней на нейтралку
      
      break;

      return NO_GEAR;                                        // включить не удалось положение коробки не понятно возвращаем код неизвестной ошибки
    }
  }
  return currentGear;
}


//*******************************определить включенную скорость ****************************
byte detectGear(void){               

  float gearPinVoltage = (float)analogRead( GEAR_SENSOR_PIN )/200;

if (geardebug) {
  Serial.print("Pin Voltage: ");
  Serial.println(gearPinVoltage);
}   
    if ( gearPinVoltage < (float)(REAR_VOL - GEAR_VOL) ) {
if (geardebug) Serial.println("NO SENSOR!");
      return NO_GEAR;
    }  

      
    if ( gearPinVoltage < (float)(REAR_VOL + GEAR_VOL) ) {     //определяем активный датчик скорости с учетом допуска напряжения
if (geardebug) Serial.println("REAR SENSOR!");  
      return REAR_GEAR; 
    }
    else {
      if ( gearPinVoltage < (float)(NEUTRAL_VOL + GEAR_VOL) ) {
if (geardebug) Serial.println("NEUTRAL SENSOR!");  
        
        return NEUTRAL_GEAR;
      }
    
          else {
if (geardebug) Serial.println("FORWARD SENSOR!");  

            return FORWARD_GEAR;
          }
    }
}

//******************* Обработка прерываний для счета скорости и оборотов двигателя ************************
void hallSpeedInt(void){

  unsigned long currentTime = millis();

  if ((currentTime - previousTimeSpeed) > 50){           //защита от искрения датчика
    wheelSpeed = ((WHEEL_LENGTH/((float)(currentTime - previousTimeSpeed)/1000))*3.6)/4;

    previousTimeSpeed = currentTime;
  
    mileageStart += WHEEL_LENGTH;
    mileageTripStart += WHEEL_LENGTH;    
  
  }  
}

void hallEngineInt(void){

  unsigned long currentTime = millis();

  if ((currentTime - previousTimeEngine) > 10){           //защита от искрения датчика
    engineSpeed = (1/((float)(currentTime - previousTimeEngine)/1000))*60;

    previousTimeEngine = currentTime;  
  }  
}
//****************************************************************************************************************

bool powerOff(){                                           // обработка события пропадания питания сети
  
  saveToEeprom();

}

void saveLongEeprom (byte address, unsigned long data){
  byte *dataw = (byte*)&data;

  for (byte i=0; i < 4; i++) EEPROM.update(address+i, *(dataw+i));   
}

unsigned long readLongEeprom (byte address ){
  unsigned long data;
  byte *dataw = (byte*)&data;

  for (byte i=0; i < 4; i++) *(dataw+i) = EEPROM.read(address+i);
   
return data;
}

void saveToEeprom(){                                       // сохранение данных в EEPROM

 mileage +=(unsigned long)mileageStart;
 mileageTrip += (unsigned long)mileageTripStart;
 engineHours += (unsigned int)(runTimeEngine/60000);
 
 saveLongEeprom(MILEAGE_EEPROM,mileage);
 saveLongEeprom(MILEAGE_TRIP_EEPROM,mileageTrip);
 saveLongEeprom(HOURS_ENGINE_EEPROM,engineHours);

}
