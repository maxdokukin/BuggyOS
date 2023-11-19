/*
 * Распознание команд терминала
 * 
 * byte processCommand( char* )
 * 
 * возращает номер распознанной команды и записывает параметр в глобальную строку параметра char commandParameter[]
 * 
 */

#define NUMBER_OF_COMMANDS 25
#define MAX_COMMAND_LENGTH 21
#define MAX_PARAMETER_LENGTH 9

char commandParameter[MAX_PARAMETER_LENGTH+1];

const char* consoleCommands[NUMBER_OF_COMMANDS] = {
  "mileage",     //0 пробег",
  "setmileage",  //1 XXXX - установить пробег в EEPROM
  "trip",        //2 поездка
  "hours",       //3 моточасы
  "sethours",    //4 XXXX - установить моточасы в EEPROM
  "speed",       //5 скорость
  "rpm",         //6 обороты двиг
  "brake",       //7 педаль тормоза
  "oil",         //8 давление масла
  "gear",        //9 включенная скорость или ошибка коробки
  "gearsensor",  //10 показать активный датчик скорости
  "fuel",        //11 уровень топливо
  "park",        //12 стояночный тормоз
  "choke",       //13 положение сервопривода подсоса
  "oiltemp",     //14 температура масла
  "all",         //15 вывести все данные
  "reset",       //16 сброс чего нибудь?
  "temp",        //17 температура двигателя
  "brakeliquid", //18 уровень тормозной жидкости
  "matrixque",   //19 очередь матрицы в бинарном виде
  "voltage",     //20 напряжение сети A0
  "time",        //21 время с момента запуска системы
  "?",           //22 список команд
  "debug",       //23 включить/отключить показ отладочных данных
  "eeprom"       //24 чтение/запись всех переменных в EEPROM-  параметр save
};

const char helpCommand[] =
"all               - вывести все данные\n"
"brake             - педаль тормоза\n"
"brakeliquid       - уровень тормозной жидкости\n"
"debug             - переключать показ отладочных данных\n"
"eeprom [save]     - без параметра чтение/ с параметром- запись всех показателей в EEPROM\n"
"fuel              - уровень топливо\n"
"gear [0/1/2]      - включенная скорость или ошибка коробки, с параметром- переключить скорость\n"
"gearsensor [debug]- активный датчик скорости,  debug переключает режим отладки коробки передач\n"
"hours             - моточасы в часах\n"
"matrixque         - очередь матрицы в бинарном виде\n"
"mileage           - пробег в км \n"
"oil               - давление масла\n"
"oiltemp           - температура масла\n"
"park              - стояночный тормоз\n"
"reset             - перезагрузка системы с сохранением данных в EEPROM\n"
"rpm               - обороты двигателя\n"
"choke [auto/manual/(angle)] - без параметра показывает положение сервопривода заслонки, auto/manual режим заслонки, либо установить угол 0-180 град\n"
"sethours XXXX     - установить моточасы в минутах EEPROM\n"
"setmileage XXXX   - установить пробег в метрах EEPROM\n"
"speed             - скорость\n"
"temp              - температура двигателя\n"
"time              - время с момента запуска системы\n"
"trip [reset]      - поездка в км/ с параметром сброс на 0\n"
"voltage           - напряжение сети A0\n"
"?                 - список команд\n";


byte processCommand( char* command ){

  size_t strLen = strlen( command );
  byte commandNumber = 255;
  byte i;
  char comm[MAX_COMMAND_LENGTH+1];                  //сюда запишем выделенную из строки команду

  commandParameter[0]=0;                           //стереть строку параметра

  if ( strLen > MAX_COMMAND_LENGTH ) return 255;   //если переданная строка длиннее максимальной длинны команды то возвращаем ошибку

  for ( i=0; i<=strLen; i++){                       // выделяем из команды первое слово до пробела или до конца в отдельную строку comm

    if ( (command[i] == ' ') || (command[i] == 0)) { //если найден пробел или конец то:
      comm[i]=0;                                    //записать конец строки
      break;                                        //и закончить чтение строки команды
      }
      
    else comm[i] = command[i];
    
  }
if (debug) Serial.println(comm);


  // command[i] равно либо знак пробела либо конец строки
  byte ii = 0;  
  for ( ++i; i<=strLen; ++i){                         // если следующий знак еще не конец строки то далее выделяем из команды слово после пробела до следующего пробела или конца строки

    if ( (command[i] == ' ') || (command[i] == 0)) { //если найден пробел или конец то:
      commandParameter[ii]=0;                      //записать конец строки
      break;                                        //и закончить чтение строки команды
      }
      
    else commandParameter[ii] = command[i];

    ii++;
  }
if(debug) Serial.println(commandParameter);

  for ( byte i=0; i < NUMBER_OF_COMMANDS; i++) {      // сравниваем первое слово со всеми командами и присваиваем commandNumber номер команды либо останется значение 255 если нет такой команды

      byte result = strcmp( comm, consoleCommands[i] );

      if ( result == 0 ) {
        commandNumber = i;
        break;
      }
  }


  return commandNumber;
  
}

