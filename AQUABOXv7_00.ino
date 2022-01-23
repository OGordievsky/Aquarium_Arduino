#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

#include <LiquidCrystal_I2C.h> // библиотека экрана
LiquidCrystal_I2C lcd(0x3F,16,2);

int PWM_LW_MIN = 0; //Если необходим ток покоя на LED - изменить эту константу 
int PWM_LW_MAX = 100; //Если необходимо ограничить максимальную яркость - уменьшить значение указано в процентах.
byte pwm_LW = 0; // вычисляемая величина ШИМ

#define PWM_LW_PIN 3 //Пин порта, где будет ШИМ LW 
#define dMinute 60UL //Константы для удобства перевода времени в функциях, соответсвующее количество секунд в 1 минуте
#define dHour 3600UL //Константы для удобства перевода времени в функциях, соответсвующее количество секунд в 1 часе
#define but1 9
#define but2 8
#define but3 7
#define but4 6
#define INTERVAL  500UL
#define solpin1 4
#define solpin2 5

RTC_DS1307 RTC; 

// начало блока переменных обработки состояния кнопок
boolean butSt1 = 0 ; 
unsigned char countSt1 = 0 ;
unsigned long prM1 = 0;

boolean butSt2 = 0 ;
unsigned char countSt2 = 0 ;
unsigned long prM2 = 0;

boolean butSt3 = 0 ;
unsigned char countSt3 = 0 ;
unsigned long prM3 = 0;

boolean butSt4 = 0 ;
unsigned char countSt4 = 0 ;
unsigned long prM4 = 0;
// конец блока переменных обработки состояния кнопок

int stopFilter =  1; // на какое время останавливаем фильтр, чтобы покормить рыб. По умолч 1 минута.

// обработка программы без delay, переменные для отсчёта интервалов события.
unsigned long prMI = 0;
unsigned long prScrMI = 0;

// переменные начала времени рассвета и заката
int  RH = 0;
int  RM = 0;
int  RD = 1;
int  ZH = 0;
int  ZM = 0;
int  ZD = 1;

// устанавливаем дату и время по умолчанию
int  YEARSET = 2018;
int  MONTHSET = 4;
int  DAYSET = 1;
int  HOURSET = 10;
int  MINUTESET = 20;


int pos = 1; // уровень меню 1
int pos1 = 1; // уровень меню 2
boolean StopMode = 0 ; //остановка всех устройств
boolean StopSol = 0 ; //остановка реле подачи углекислого газа

//******************************************************************************************** 


void setup(){ 


TCCR2B = TCCR2B & 0b11111000 | 0x02; //шим частотой 3000 Гц на 3 пине, если указаное ранее значение не менялось!!!


Serial.begin(9600);
Wire.begin(); //Инициируем I2C интерфейс 

//считываем данные из ранее сохранённых переменных
RH = EEPROM.read(0);
RM = EEPROM.read(1);
RD = EEPROM.read(2);
ZH = EEPROM.read(3);
ZM = EEPROM.read(4);
ZD = EEPROM.read(5); 
stopFilter = EEPROM.read(6);
PWM_LW_MAX = EEPROM.read(7);
PWM_LW_MIN = EEPROM.read(8);


RTC.begin(); //Инициирум RTC модуль 

lcd.init();
lcd.backlight();
 
pinMode(but1, INPUT);
pinMode(but2, INPUT);
pinMode(but3, INPUT); 
pinMode(but4, INPUT);  

pinMode (solpin1, OUTPUT);
pinMode (solpin2, OUTPUT);

pinMode (PWM_LW_PIN, OUTPUT);

if (! RTC.isrunning()) { 
RTC.adjust(DateTime(YEARSET, MONTHSET, DAYSET, HOURSET , MINUTESET , 01));   

delay (500);
} 
} // КОНЕЦ ИНИЦИАЛИЗАЦИИ 

//******************************************************************************************** 



void loop() // ПРОГРАММЫй безусловный ЦИКЛ 
{
    
long sunrise_start = RH*dHour+RM*dMinute; //Начало восхода 
long sunrise_duration = RD*dMinute; //Длительность восхода 
long sunset_start = ZH*dHour+ZM*dMinute; //начало заката 
long sunset_duration = ZD*dMinute; //Длительность заката 

DateTime myTime = RTC.now(); //Читаем данные времени из RTC при каждом выполнении цикла 
long Day_time = myTime.unixtime() % 86400; //сохраняем в переменную - время в формате UNIX 
if ((Day_time<sunrise_start) || //Если с начала суток меньше чем начало восхода 
(Day_time>=sunset_start+sunset_duration)) { //Или больше чем начало заката + длительность 
pwm_LW = PWM_LW_MIN*2.5; //Величина для записи в порт равна минимуму 


//********************************************************************************************* 
// обработка интервала восхода 
//********************************************************************************************* 
}else if ((Day_time>=sunrise_start) && //Если с начала суток больше чем начало восхода 
(Day_time<sunrise_start+sunrise_duration)){ //И меньше чем начало восхода + длительность 

pwm_LW = ((Day_time - sunrise_start)*(PWM_LW_MAX-PWM_LW_MIN)*2.5) / sunrise_duration; //Вычисляем для рассвета величину для записи в порт ШИМ 


//********************************************************************************************* 
// обработка интервала заката 
//********************************************************************************************* 
}else if ((Day_time>=sunset_start) && //Если начала суток больше чем начало заката и меньше чем 
(Day_time<sunset_start+sunset_duration)){ //начало заката плюс длительность 

pwm_LW = ((sunset_start+sunset_duration - Day_time)*(PWM_LW_MAX-PWM_LW_MIN)*2.5) / sunset_duration; //Вычисляем для заката величину для записи в порт ШИМ 


//******************************************************************************************** 
// обработка интервала от конца рассвета и до начала заката, 
// когда свет должен быть включен на максимальную яркость 
//******************************************************************************************** 
}else { 
pwm_LW = PWM_LW_MAX*2.5; //Устанавливаем максимальную величину для записи в порт ШИМ 

} 

analogWrite(PWM_LW_PIN, pwm_LW); //Пишем в порт вычисленное значение 

if (Day_time < sunrise_start || Day_time >= (sunset_start + (sunset_duration/4))) StopSol = HIGH;
else StopSol = LOW;

if ((StopSol == HIGH)||(StopMode == HIGH )) digitalWrite (solpin1, LOW);
else digitalWrite (solpin1, HIGH);

if (StopMode == HIGH) digitalWrite (solpin2, LOW);
else digitalWrite (solpin2, HIGH);
  
if (digitalRead(but1) == HIGH){countSt1 ++ ;}
else if ((digitalRead(but1) == LOW)&&(countSt1>=2)){countSt1 --;}
else countSt1 = 0 ;
if ((countSt1 >=5)&&((millis() - prM1) >= INTERVAL)){
  prM1 = millis();  
  butSt1 = HIGH;                                 
  countSt1 = 8;
  }
else { butSt1 = LOW;}

if (digitalRead(but2) == HIGH){countSt2 ++ ;}
else if ((digitalRead(but2) == LOW)&&(countSt2>=2)){countSt2 --;}
else {countSt2 = 0 ;}
if ((countSt2 >=5)&&((millis() - prM2) >= INTERVAL)){
  prM2 = millis();  
  butSt2 = HIGH;
   countSt2 = 8;
  }
else { butSt2 = LOW;}

if (digitalRead(but3) == HIGH){countSt3 ++ ;}
else if ((digitalRead(but3) == LOW)&&(countSt3>=2)){countSt3 --;}
else {countSt3 = 0 ;}
if ((countSt3 >=5)&&((millis() - prM3) >= INTERVAL)){
  prM3 = millis();  
  butSt3 = HIGH;
  countSt3 = 8;
  }
else { butSt3 = LOW;}

if (digitalRead(but4) == HIGH){countSt4 ++ ;}
else if ((digitalRead(but4) == LOW)&&(countSt4>=2)){countSt4 --;}
else {countSt4 = 0 ;}
if ((countSt4 >=5)&&((millis() - prM4) >= INTERVAL)){
  prM4 = millis();  
  butSt4 = HIGH;
  countSt4 = 8;
  }
else { butSt4 = LOW;}




if (butSt1 == HIGH) { 
  pos++;
  lcd.clear();
  }
if (pos >= 10){
  lcd.clear();
  pos=1;
    }
    
//возрат на главный экран по умолчанию при 1 минуте бездействия
    
if (pos>1){
if (prScrMI > millis())prScrMI = 0;
else if ((butSt1 == HIGH)||(butSt2 == HIGH)||(butSt3 == HIGH)||(butSt4 == HIGH)) prScrMI = millis(); 
if (millis() - prScrMI > 60000 ){
lcd.clear();   
pos=1;
}
}
// очистка дисплея раз в 1 час 
/*
  if ( Day_time % 3600 == 0){
  delay (1500);
  lcd.clear();
     }
     
*/

switch (pos){
case 1:
    
      lcd.setCursor(0,0); 
      if (myTime.hour() < 10) lcd.print("0"); 
      lcd.print(myTime.hour(),DEC); 
      lcd.print(":"); 
      if (myTime.minute() < 10) lcd.print("0"); 
      lcd.print(myTime.minute(),DEC);
      lcd.print(":"); 
      lcd.print(myTime.second(),DEC);
      lcd.print(" ");
      lcd.setCursor(9,0); 
      lcd.print("|");
      if (myTime.day() < 10) lcd.print("0"); 
      lcd.print(myTime.day(),DEC); 
      lcd.print("|"); 
      if (myTime.month() < 10) lcd.print("0"); 
      lcd.print(myTime.month(),DEC); 
      lcd.print("|");
      
      lcd.setCursor(0,1);
      lcd.print ("PL");
      lcd.print(pwm_LW);
      lcd.print("  "); 
      lcd.setCursor(7,1);
      lcd.print ("F");
      if (StopMode == HIGH ) lcd.print("(O)");
      else lcd.print("(I)");
      lcd.setCursor(12,1);
      lcd.print ("S");
      if ((StopSol == HIGH)||(StopMode == HIGH )) lcd.print("(O)");
      else lcd.print("(I)");
      
//остановка компрессора и соленоида при нажатии на кнопку 4
if (prMI > millis())prMI = 0;
else if (butSt4 == HIGH) prMI = millis(); 
if (stopFilter * 1000 * dMinute + 1000 > millis() ) StopMode = LOW; 
else if (millis() - prMI < stopFilter * 1000 * dMinute ) StopMode = HIGH;
else StopMode = LOW;
      
      break;
      
      
case 2:
       
      lcd.setCursor(0,0); 
      lcd.print("SUNRISE START");
        if (butSt2 == HIGH) { 
        RH++;
        }
      if (RH >= 24)  {
        RH = 0; 
        }
        
         if (butSt3 == HIGH) { 
        RM = RM + 5;  
        }
      if (RM >= 60)  {
        RM = 0; 
        }
        
      lcd.setCursor(0,1);
      if ( RH < 10) lcd.print("0");      
      lcd.print(RH);
      lcd.print(":");
      if ( RM < 10) lcd.print("0");        
      lcd.print(RM);
      lcd.setCursor(6,1);
      lcd.print("h:m"); 
      
 lcd.setCursor(11,1);
      if (butSt4 == HIGH) {
      lcd.clear();  
      EEPROM.write(0, RH);
      EEPROM.write(1, RM);
      lcd.print("OK");
      butSt4 = LOW ;
      delay (INTERVAL/2) ;
      lcd.clear(); 
    
      }
      else {
        lcd.print ("Save?");
           }
      break;
      
case 3:
     
      lcd.setCursor(0,0); 
      lcd.print("SUNRISE LONG");
      if (butSt2 == HIGH) { 
        RD--;
        lcd.clear();
               }
      if (RD <= 0)  {
        RD = 180; 
        }
        
        if (butSt3 == HIGH) { 
        RD++;
        lcd.clear();
         }
      if (RD > 180)  {
        RD = 1;  
        }
      lcd.setCursor(0,1);
      if ( RD < 0) lcd.print("0");  
      lcd.print(RD);
      lcd.setCursor(6,1);
      lcd.print("min");  
      
 lcd.setCursor(11,1);
     if (butSt4 == HIGH) {
      lcd.clear();  
      EEPROM.write(2, RD);
      lcd.print("OK");
      butSt4 = LOW ;
      delay (INTERVAL/2) ;
      lcd.clear(); 
    
      }
      else {
        lcd.print ("Save?");
           }
      break;
case 4:       
 lcd.setCursor(0,0); 
      lcd.print("SUNSET START");
     if (butSt2 == HIGH) { 
        ZH++;
        }
      if (ZH >= 24)  {
        ZH = 0; 
        }

         if (butSt3 == HIGH) { 
        ZM = ZM + 5;  
         }
      if (ZM >= 60)  {
        ZM = 0; 
        }
        
      lcd.setCursor(0,1);
      if ( ZH < 10) lcd.print("0");      
      lcd.print(ZH);
      lcd.print(":");
      if ( ZM < 10) lcd.print("0");        
      lcd.print(ZM);           
      lcd.setCursor(6,1);
      lcd.print("h:m"); 

 lcd.setCursor(11,1);
      if (butSt4 == HIGH) {
      lcd.clear();  
      EEPROM.write(3, ZH);
      EEPROM.write(4, ZM);
      lcd.print("OK");
      butSt4 = LOW ;
      delay (INTERVAL/2) ;
      lcd.clear(); 
    
      }
      else {
        lcd.print ("Save?");
           }
            
      break;
     
case 5:
       
      lcd.setCursor(0,0); 
      lcd.print("SUNSET LONG");
      if (butSt2 == HIGH) { 
        ZD--;
        lcd.clear();
        }
      if (ZD <= 0)  {
        ZD = 180; 
        }
        if (butSt3 == HIGH) { 
        ZD++;
        lcd.clear();
        }
      if (ZD > 180)  {
        ZD = 1;  
        }
      lcd.setCursor(0,1);
      if ( ZD < 0) lcd.print("0");  
      lcd.print(ZD); 
      lcd.setCursor(6,1);
      lcd.print("min");
      
 lcd.setCursor(11,1);
      if (butSt4 == HIGH) {
      lcd.clear();  
      EEPROM.write(5, ZD);
      lcd.print("OK");
      butSt4 = LOW ;
      delay (INTERVAL/2) ;
      lcd.clear(); 
    
      }
      else {
        lcd.print ("Save?");
           }
      break;

case 6:
       
      lcd.setCursor(0,0); 
      lcd.print("FILTER DELAY");
      if (butSt2 == HIGH) { 
        stopFilter--;
        lcd.clear();
        }
      if (stopFilter <= 0)  {
        stopFilter = 30; 
        }
        if (butSt3 == HIGH) { 
        stopFilter++;
        lcd.clear();
        }
      if (stopFilter > 30)  {
        stopFilter = 0;  
        }
      lcd.setCursor(0,1);
      if ( stopFilter < 0) lcd.print("0");  
      lcd.print(stopFilter); 
      lcd.setCursor(6,1);
      lcd.print("min");
      
 lcd.setCursor(11,1);
      if (butSt4 == HIGH) {
      lcd.clear();  
      EEPROM.write(6, stopFilter);
      lcd.print("OK");
      butSt4 = LOW ;
      delay (INTERVAL/2) ;
      lcd.clear();     
      }
      else {
        lcd.print ("Save?");
           }
      break;
     
     
case 7:
     
      lcd.setCursor(0,0); //установка максимального освещения круто, но нет отображения текущего
      lcd.print("SUN POWER");
      lcd.print(" ");
      lcd.print(PWM_LW_MAX);
      lcd.print(" ");
      lcd.print("%");     
        if (butSt3 == HIGH)  { 
        PWM_LW_MAX = PWM_LW_MAX + 5;
         
         lcd.clear();
        }
      if (PWM_LW_MAX > 100)  {
        PWM_LW_MAX = 0;
         
        }
        if (butSt2 == HIGH) { 
       PWM_LW_MAX = PWM_LW_MAX - 5;
         
        lcd.clear();
        }
      if (PWM_LW_MAX < 0)  {
        PWM_LW_MAX = 100;  
        }
        lcd.setCursor(3,1);
        if (PWM_LW_MAX >= 10 && PWM_LW_MAX <20 ) { 
        lcd.print("*");
        }
        if (PWM_LW_MAX >=20 && PWM_LW_MAX <30 ) { 
        lcd.print("**");
        }
        if (PWM_LW_MAX >=30 && PWM_LW_MAX <40 ) { 
        lcd.print("***");
        } 
        if (PWM_LW_MAX >=40 && PWM_LW_MAX <50 ) { 
        lcd.print("****");
        } 
        if (PWM_LW_MAX >=50 && PWM_LW_MAX <60 ) { 
        lcd.print("*****");
        } 
        if (PWM_LW_MAX >=60 && PWM_LW_MAX <70 ) { 
        lcd.print("******");
        }
        if (PWM_LW_MAX >=70 && PWM_LW_MAX <80 ) { 
        lcd.print("*******");
        }
        if (PWM_LW_MAX >=80 && PWM_LW_MAX <90 ) { 
        lcd.print("********");
        }
        if (PWM_LW_MAX >=90 && PWM_LW_MAX <100 ) { 
        lcd.print("*********");
        }
        if (PWM_LW_MAX >=100) { 
        lcd.print("**********");
        }

      if (butSt4 == HIGH) {
      lcd.clear();  
      EEPROM.write(7, PWM_LW_MAX);
      lcd.print("OK");
      butSt4 = LOW ;
      delay (INTERVAL/2) ;
      lcd.clear();     
      }
      break;
      
case 8:
     
      lcd.setCursor(0,0); 
      lcd.print("NIGHT POWER");
      lcd.print(" ");
      lcd.print(PWM_LW_MIN);
      lcd.print("%");     
        if (butSt3 == HIGH)  { 
        PWM_LW_MIN = PWM_LW_MIN + 5;
         
         lcd.clear(); 
        }
      if (PWM_LW_MIN > 100)  {
        PWM_LW_MIN = 0; 
        }
        if (butSt2 == HIGH)  { 
       PWM_LW_MIN = PWM_LW_MIN - 5;
        
       lcd.clear(); 
        }
      if (PWM_LW_MIN < 0)  {
        PWM_LW_MIN = 100;  
        }
        lcd.setCursor(3,1);
        if (PWM_LW_MIN >= 10 && PWM_LW_MIN <20 ) { 
        lcd.print("*");
        }
        if (PWM_LW_MIN >=20 && PWM_LW_MIN <30 ) { 
        lcd.print("**");
        }
        if (PWM_LW_MIN >=30 && PWM_LW_MIN <40 ) { 
        lcd.print("***");
        } 
        if (PWM_LW_MIN >=40 && PWM_LW_MIN <50 ) { 
        lcd.print("****");
        } 
        if (PWM_LW_MIN >=50 && PWM_LW_MIN <60 ) { 
        lcd.print("*****");
        } 
        if (PWM_LW_MIN >=60 && PWM_LW_MIN <70 ) { 
        lcd.print("******");
        }
        if (PWM_LW_MIN >=70 && PWM_LW_MIN <80 ) { 
        lcd.print("*******");
        }
        if (PWM_LW_MIN >=80 && PWM_LW_MIN <90 ) { 
        lcd.print("********");
        }
        if (PWM_LW_MIN >=90 && PWM_LW_MIN <100 ) { 
        lcd.print("*********");
        }
        if (PWM_LW_MIN >=100) { 
        lcd.print("**********");
        }

      if (butSt4 == HIGH) {
      lcd.clear();  
      EEPROM.write(8, PWM_LW_MIN);
      lcd.print("OK");
      butSt4 = LOW ;
      delay (INTERVAL/2) ;
      lcd.clear(); 
      }
      break;
      
case 9:


if (butSt4 == HIGH) { 
  pos1 ++;
  lcd.clear();
  }
if (pos1 >= 7){
  lcd.clear();
  pos1=1;
    }
switch (pos1){
  case 1:
              lcd.setCursor(0,0); 
              lcd.print("Year Set"); 
              lcd.setCursor(0,1); 
              lcd.print(YEARSET);
                if (butSt2 == HIGH) { 
                YEARSET --;
                lcd.clear();
                }
              if (YEARSET <= 2017)  {
                YEARSET = 2028; 
                }
                if (butSt3 == HIGH) { 
                YEARSET ++;
                lcd.clear();
                }
              if (YEARSET > 2028)  {
                YEARSET = 2018;  
                }
              lcd.setCursor(0,1);
              if ( YEARSET < 2000) lcd.print("0000");  
              else lcd.print(YEARSET);  
break;

case 2:
  lcd.setCursor(0,0); 
              lcd.print("Month Set"); 
              lcd.setCursor(0,7); 
              lcd.print(MONTHSET); 
         
                if (butSt2 == HIGH) { 
                MONTHSET --;
                lcd.clear();
                }
              if (MONTHSET <= 0)  {
                MONTHSET = 12; 
                }
                if (butSt3 == HIGH) { 
                MONTHSET ++;
                lcd.clear();
                }
              if (MONTHSET > 12)  {
                MONTHSET = 1;  
                }
              lcd.setCursor(0,1);
              if ( MONTHSET < 1) lcd.print("00");  
              else lcd.print(MONTHSET); 

break;

case 3:
                 lcd.setCursor(0,0); 
              lcd.print("Day Set"); 
              lcd.setCursor(0,7); 
              lcd.print(DAYSET); 
         
                if (butSt2 == HIGH) { 
                DAYSET --;
                lcd.clear();
                }
              if (DAYSET <= 0)  {
                DAYSET = 31; 
                }
                if (butSt3 == HIGH) { 
                DAYSET ++;
                lcd.clear();
                }
              if (DAYSET > 31)  {
                DAYSET = 1;  
                }
              lcd.setCursor(0,1);
              if ( DAYSET < 1) lcd.print("00");  
              else lcd.print(DAYSET);

break;
             case 4:
               lcd.setCursor(0,0); 
              lcd.print("Hour Set"); 
              lcd.setCursor(0,7); 
              lcd.print(HOURSET); 
         
                if (butSt2 == HIGH) { 
                HOURSET --;
                lcd.clear();
                }
              if (HOURSET < 0)  {
                HOURSET = 23; 
                }
                if (butSt3 == HIGH) { 
                HOURSET ++;
                lcd.clear();
                }
              if (HOURSET > 23)  {
                HOURSET = 0;  
                }
              lcd.setCursor(0,1);
              if ( HOURSET < 0) lcd.print("00");  
              else lcd.print(HOURSET); 
break;

             case 5:
               lcd.setCursor(0,0); 
              lcd.print("Minute Set"); 
              lcd.setCursor(0,7); 
              lcd.print(MINUTESET); 
         
                if (butSt2 == HIGH) { 
                MINUTESET --;
                lcd.clear();
                }
              if (MINUTESET < 0)  {
                MINUTESET = 59; 
                }
                if (butSt3 == HIGH) { 
                MINUTESET ++;
                lcd.clear();
                }
              if (MINUTESET > 59)  {
                MINUTESET = 0;  
                }
              lcd.setCursor(0,1);
              if ( MINUTESET < 0) lcd.print("00");  
              else lcd.print(MINUTESET);    
break;
                case 6:
             
              lcd.setCursor(0,0); 
              lcd.print(DAYSET); 
              lcd.print("."); 
              lcd.print(MONTHSET);
              lcd.print(".");               
              lcd.print(YEARSET); 
                         
              lcd.setCursor(10,0); 
              lcd.print(HOURSET); 
              lcd.print(":"); 
              lcd.print(MINUTESET); 
              

              lcd.setCursor(5,1); 
              lcd.print("NO"); 
              lcd.setCursor(9,1); 
              lcd.print("YES"); 
         
                if (butSt2 == HIGH) { 
             lcd.clear();
             lcd.setCursor(0,0);     
             lcd.print("Not Save"); 
             delay (1000) ; 
                pos = 1  ;    
                }
         
                if (butSt3 == HIGH) { 
               lcd.clear();
               delay (1000) ;        
              RTC.adjust(DateTime(YEARSET, MONTHSET, DAYSET, HOURSET , MINUTESET , 01)); 
                lcd.print("OK Date is SET"); 
                delay (2000);
                pos = 1;
                }
break;

}        

      
}
}
