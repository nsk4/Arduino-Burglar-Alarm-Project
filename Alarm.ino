#include <LiquidCrystal.h>
#include <IRremote.h>
#include <EEPROM.h>

/// LCD
LiquidCrystal lcd(12, 11, 7, 4, 3, 2);

/// Controller
#define IR_CONTROLLER_PIN 10
IRrecv irrecv(IR_CONTROLLER_PIN);
decode_results results;
unsigned int key;

/*
// uni controller
#define POW 0xFFA25D
#define MODE 0xFF629D
#define MUTE 0xFFE21D
#define PREV 0xFF22DD
#define NEXT 0xFF02FD
#define PLAY_PAUSE 0xFFC23D
#define MINUS 0xFFE01F
#define PLUS 0xFFA857
#define EQ 0xFF906F
#define ZERO 0xFF6897
#define HUNDRED 0xFF9867
#define RET 0xFFB04F
#define ONE 0xFF30CF
#define TWO 0xFF18E7
#define THREE 0xFF7A85
#define FOUR 0xFF10EF
#define FIVE 0xFF38C7
#define SIX 0xFF5AA5
#define SEVEN 0xFF42BD
#define EIGHT 0xFF4AB5
#define NINE 0xFF52AD
*/

// my controller
#define PREV 0x22DD
#define NEXT 0x2FD
#define MINUS 0xE01F
#define PLUS 0xA857
#define EQ 0x906F
#define ZERO 0x6897
#define HUNDRED 0x9867
#define ONE 0x30CF
#define TWO 0x18E7
#define THREE 0x7A85
#define FOUR 0x10EF
#define FIVE 0x38C7
#define SIX 0x5AA5
#define SEVEN 0x42BD
#define EIGHT 0x4AB5
#define NINE 0x52AD


/// Timer
volatile signed char timeLeft;
volatile signed char timeLeftBool;
volatile signed char alarmTimeLeft;
volatile signed char alarmTimeLeftBool;

volatile byte hrs;
volatile byte mins;
volatile byte secs;

#define MAX_TIME_TO_TRIGGER_SENSOR 30
#define MAX_TIME_TO_ENTER_PASSWORD 20
#define MAX_RETRY 3
int alarmPass[4];
int engPass[4];
int enteredPass[4];
int currPassNum;
int retryLeft;
#define MODE_ALARM 1
#define MODE_ENG 2
#define MODE_NORMAL 3
signed char mode;

/// Zones
#define ZONE_OFF 0
#define ZONE_ENTRY_EXIT 1
#define ZONE_DIGITAL 2
#define ZONE_ANALOGUE 3
#define ZONE_CONTINUOUS 4
signed char zones[4];
int zonesTreshold[4];
signed char zonesPin[4];
signed char zonesEntryExitStartHrs[4];
signed char zonesEntryExitEndHrs[4];
signed char zonesEntryExitStartMins[4];
signed char zonesEntryExitEndMins[4];
byte constantAlarm; // if continuous zone is triggered

// analogue pins
#define ZONE_ONE_PIN 0
#define ZONE_TWO_PIN 1
#define ZONE_THREE_PIN 2
#define ZONE_FOUR_PIN 3
#define ZONE_ONE 0
#define ZONE_TWO 1
#define ZONE_THREE 2
#define ZONE_FOUR 3


/// Sound
#define SOUND_OFF 0
#define SOUND_BUTTON_PRESSED 1
#define SOUND_TIMER_COUNTDOWN 2
#define SOUND_INCORRECT_PASSWORD 3
#define SOUND_CORRECT_PASSWORD 4
#define SOUND_ENG_MODE_ENTER 5
#define SOUND_ENG_MODE_EXIT 6
#define SOUND_ALARM_DISABLED 7
#define SOUND_ALARM 8
int soundMode;

// Menu Navigation
#define MENU_CHANGE_ALARM_PASS 0
#define MENU_CHANGE_ENG_PASS 1
#define MENU_SET_TIME 2
#define MENU_EDIT_ZONES 3
#define MENU_LOGOUT 4
#define MENU_EDIT_ZONES_SELECT 45
#define MENU_EDIT_ZONES_EDIT 46
#define MENU_EDIT_ZONES_EDIT_ASSIGN 123
#define MENU_EDIT_ZONES_EDIT_CHANGE_THRESHOLD 124
#define MENU_EDIT_ZONES_EDIT_ENTRY_EXIT_TIME 125
int selectedZone;
int selectedZoneEdit;
int currMenu;

/// EEPROM
#define EEPROM_CONTROL_BYTE 0
#define EEPROM_LAST_LOG_LOC 1
// 2 bytes per element
#define EEPROM_ENG_PASS_START 2
// 2 bytes per element
#define EEPROM_ALARM_PASS_START 10
// 1 byte per element
#define EEPROM_ZONES 18
// 2 bytes per element
#define EEPROM_ZONES_TRESHOLD 22
// 1 byte per element
#define EEPROM_ZONES_PIN 30
// 1 byte per element
#define EEPROM_ZONES_ENTRY_HRS 34
// 1 byte per element
#define EEPROM_ZONES_ENTRY_MINS 38
// 1 byte per element
#define EEPROM_ZONES_EXIT_HRS 42
// 1 byte per element
#define EEPROM_ZONES_EXIT_MINS 46
// start at 50 for log
#define EEPROM_LOG_START 50
#define EEPROM_LOG_END 1024
#define EEPROM_LOG_BLOCK_SIZE 5
byte lastLogLoc;

#define EVENT_CHANGED_CONFIG 0
#define EVENT_CHANGED_ALARM_PASS 1
#define EVENT_CHANGED_ENG_PASS 2
#define EVENT_ALARM 3
#define EVENT_ENTERED_ALARM_PASS 4
#define EVENT_ENTERED_ENG_PASS 5
#define EVENT_WRONG_PASS 6

/// Alarm
#define LED_PIN 5

/// Sound
#define SPEAKER_PIN 6
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_E7  2637
#define NOTE_E7  2637
#define NOTE_C7  2093
#define NOTE_G7  3136
#define NOTE_G6  1568
#define NOTE_E6  1319
#define NOTE_A6  1760
#define NOTE_B6  1976
#define NOTE_AS6 1865
#define NOTE_D7  2349
#define NOTE_A7  3520
#define NOTE_F7  2794
byte currNote;
#define SPEAKER_MELODY_NUMBER 78

/// LCD functions
void clearWrite(const char* c, int x, int y)
{
  lcd.setCursor(0, y);
  lcd.print(F("                "));
  lcd.setCursor(x, y);
  lcd.print(c);
}
void printTime(int y = 0) // C++ FTW!!!!!
{
  printBase2(hrs, 0, y);
  lcd.setCursor(2, y);
  lcd.print(F(":"));
  printBase2(mins, 3, y);
  lcd.setCursor(5, y);
  lcd.print(F(":"));
  printBase2(secs, 6, y);
}
void printBase2(int var, int startX, int startY)
{
  if (var < 10)
  {
    lcd.setCursor(startX, startY);
    lcd.print(F("0"));
    lcd.setCursor(startX + 1, startY);
    lcd.print(var);
  }
  else
  {
    lcd.setCursor(startX, startY);
    lcd.print(var);
  }
}

/// EEPROM Functions
int readIntFromEEPROM(int address)
{
  int tmp = 0;
  char *p = (char *)&tmp;
  *p = EEPROM.read(address);
  *(p + 1) = EEPROM.read(address + 1);
  return tmp;
}
void readFromEEPROM()
{

  if (EEPROM.read(EEPROM_CONTROL_BYTE) != 0xAA)
  {
    // AA is our indicator that everything is ok
    writeToEEPROM(); // we save default values
    return;
  }
  lastLogLoc = EEPROM.read(EEPROM_LAST_LOG_LOC); // where to continue logging

  for (int i = 0; i < 4; i++)
  {
    // read all
    engPass[i] = readIntFromEEPROM(EEPROM_ENG_PASS_START + i * 2); // change to read int
    alarmPass[i] = readIntFromEEPROM(EEPROM_ALARM_PASS_START + i * 2);
    zones[i] = EEPROM.read(EEPROM_ZONES + i);
    zonesTreshold[i] = readIntFromEEPROM(EEPROM_ZONES_TRESHOLD + i * 2);
    zonesPin[i] = EEPROM.read(EEPROM_ZONES_PIN + i);
    zonesEntryExitStartHrs[i] = EEPROM.read(EEPROM_ZONES_ENTRY_HRS + i);
    zonesEntryExitStartMins[i] = EEPROM.read(EEPROM_ZONES_ENTRY_MINS + i);
    zonesEntryExitEndHrs[i] = EEPROM.read(EEPROM_ZONES_EXIT_HRS + i);
    zonesEntryExitEndMins[i] = EEPROM.read(EEPROM_ZONES_EXIT_MINS + i);
  }
}
void writeIntToEEPROM(int address, int val)
{
  char *p = (char *)&val;
  EEPROM.write(address, *p);
  EEPROM.write(address + 1, *(p + 1));
}
void writeToEEPROM()
{
  EEPROM.write(EEPROM_CONTROL_BYTE, 0xAA);
  EEPROM.write(EEPROM_LAST_LOG_LOC, lastLogLoc);

  for (int i = 0; i < 4; i++)
  {
    // write all
    writeIntToEEPROM(EEPROM_ENG_PASS_START + i * 2, engPass[i]);
    writeIntToEEPROM(EEPROM_ALARM_PASS_START + i * 2, alarmPass[i]);
    EEPROM.write(EEPROM_ZONES + i, zones[i]);
    writeIntToEEPROM(EEPROM_ZONES_TRESHOLD + i * 2, zonesTreshold[i]);
    EEPROM.write(EEPROM_ZONES_PIN + i, zonesPin[i]);
    EEPROM.write(EEPROM_ZONES_ENTRY_HRS + i, zonesEntryExitStartHrs[i]);
    EEPROM.write(EEPROM_ZONES_ENTRY_MINS + i, zonesEntryExitStartMins[i]);
    EEPROM.write(EEPROM_ZONES_EXIT_HRS + i, zonesEntryExitEndHrs[i]);
    EEPROM.write(EEPROM_ZONES_EXIT_MINS + i, zonesEntryExitEndMins[i]);
  }
}
void Log(signed char event, signed char zone=-1)
{
  // h m s z e
  if(!(lastLogLoc+EEPROM_LOG_BLOCK_SIZE<EEPROM_LOG_END && lastLogLoc+EEPROM_LOG_BLOCK_SIZE>=EEPROM_LOG_START))
  {
    lastLogLoc = EEPROM_LOG_START;
  }
  // write log
  EEPROM.write(lastLogLoc, hrs);
  EEPROM.write(lastLogLoc+1, mins);
  EEPROM.write(lastLogLoc+2, secs);
  EEPROM.write(lastLogLoc+3, event);
  EEPROM.write(lastLogLoc+4, zone);
  
  lastLogLoc += EEPROM_LOG_BLOCK_SIZE;
  EEPROM.write(EEPROM_LAST_LOG_LOC, lastLogLoc);
  Serial.print(hrs);Serial.print(F(":"));
  Serial.print(mins);Serial.print(F(":"));
  Serial.print(secs);Serial.print(F(":"));
  Serial.print(event);Serial.print(F(":"));
  Serial.println(zone);
}

/// Controller Functions
void interruptRemoteController()
{
  if (irrecv.decode(&results))
  {
    key = results.value;
    if (results.value == 0xFFFFFFFF) // holding down key - ignore
      key = 0;
    irrecv.resume(); // Receive the next value
  }
}
void checkButtonPressed()
{
  if (mode == MODE_ENG)
  {
    if (!(key == ONE ||
          key == TWO ||
          key == THREE ||
          key == FOUR ||
          key == FIVE ||
          key == SIX ||
          key == SEVEN ||
          key == EIGHT ||
          key == NINE ||
          key == NEXT ||
          key == PREV ||
          key == PLUS ||
          key == MINUS))
    {

      Serial.println(F("Incorrect key"));
      return; // not correct key for password
    }
    soundMode = SOUND_BUTTON_PRESSED;
    doEngStuff();
    
  }
  else
  {
    if(key==ZERO) 
    {
      resetPass(0); // 0 button resets entered pass, param 0 resets only pass, not time
      Serial.println(F("Password entry reset"));
      clearWrite("Password entry reset",0,1);
      return;
    }
    //Serial.println(key, HEX);
    if (!(key == ONE ||
          key == TWO ||
          key == THREE ||
          key == FOUR ||
          key == FIVE ||
          key == SIX ||
          key == SEVEN ||
          key == EIGHT ||
          key == NINE))
    {

      Serial.println(F("Incorrect key"));
      return; // not correct key for password
    }
    
    //Serial.print("ewq");


    soundMode = SOUND_BUTTON_PRESSED;

    if (currPassNum == 0) // first button
    {
      clearWrite("", 0, 1);
      startCountdownButton();
    }
    enteredPass[currPassNum] = key;
    currPassNum++;

    for (int i = 0; i < 4; i++)
    {
      if (enteredPass[i] != 0)
      {
        Serial.print(F("*"));
        lcd.setCursor(i, 1);
        lcd.print(F("*"));
      }
      else
      {
        Serial.print(F("_"));
        lcd.setCursor(i, 1);
        lcd.print(F("_"));
      }

    }
    Serial.println("");

    if (currPassNum >= 4)
    {
      int ok = 1;
      for (int i = 0; i < 4; i++)
      {
        if (enteredPass[i] != engPass[i])
        {
          ok = 0; // password does not match eng password
          break;
        }
      }
      if (ok) // password does match eng password
      {
        Serial.println(F("Logged in"));
        Log(EVENT_ENTERED_ENG_PASS);
        clearWrite("Logged in", 0, 1);

        stopTimer(); // disable timers
        resetPass(0);
        retryLeft = MAX_RETRY;
        mode = MODE_ENG;
        soundMode = SOUND_ENG_MODE_ENTER;
        currMenu = MENU_CHANGE_ALARM_PASS;
        Serial.println(F("MENU_CHANGE_ALARM_PASS"));
        clearWrite("Change Alarm Key", 0, 1);
        doEngStuff();
        selectedZone = ZONE_ONE;
        return;
      }
        else if (mode == MODE_ALARM)
        {
          resetPass(0);
          soundMode = SOUND_ALARM;
          return;
        }

      ok = 1;
      for (int i = 0; i < 4; i++)
      {
        if (enteredPass[i] != alarmPass[i])
        {
          ok = 0; // password does not match alarm password
          break;
        }
      }
      if (ok)// password match alarm password
      {
        // correct alarm pass
        Serial.println(F("Correct alarm pass"));
        Log(EVENT_ENTERED_ALARM_PASS);
        clearWrite("Right alarm pass", 0, 1);
        soundMode = SOUND_CORRECT_PASSWORD;
        retryLeft = MAX_RETRY;
        stopTimer();
        resetPass(0);
        startCountdownAlarm();
      }
      else
      {
        // wrong pass
        Serial.println(F("Incorrect alarm pass"));
        clearWrite("Wrong alarm pass", 0, 1);
        Log(EVENT_WRONG_PASS);
        soundMode = SOUND_INCORRECT_PASSWORD;
        retryLeft--;
        if (retryLeft <= 0)
        {
          // wrong pass to many times
          mode = MODE_ALARM;
          soundMode = SOUND_ALARM;
        }
        resetPass(0);
        
      }
    }
    else if (mode == MODE_ALARM)
    {
      soundMode = SOUND_ALARM;
      return;
    }
  }

  // is starting to enter password
  // is entering password
  // in in admin mode
}
void resetPass(int x)
{
  
  // resets entered password
  currPassNum = 0;
  enteredPass[0] = 0; enteredPass[1] = 0; enteredPass[2] = 0; enteredPass[3] = 0;
  if(!x)return; // just a password reset
  timeLeft = MAX_TIME_TO_ENTER_PASSWORD;
}


/// Timer Functions
void startCountdownAlarm()
{
  alarmTimeLeft = MAX_TIME_TO_TRIGGER_SENSOR;
  alarmTimeLeftBool = 1;
}
void startCountdownButton()
{
  timeLeft = MAX_TIME_TO_ENTER_PASSWORD;
  timeLeftBool = 1;
}
void stopTimer()
{
  alarmTimeLeftBool = 0;
  timeLeftBool = 0;
}
void timerInterrupt()
{
  //Serial.println("qwe");
  clearWrite("", 0, 0);
  
  if (alarmTimeLeftBool)
  {
    alarmTimeLeft--;
    if(mode!=MODE_ALARM)soundMode = SOUND_TIMER_COUNTDOWN;
    printBase2(alarmTimeLeft, 14, 0);
    Serial.println(alarmTimeLeft);
  }
  if (timeLeftBool)
  {
    timeLeft--;
    if(mode!=MODE_ALARM)soundMode = SOUND_TIMER_COUNTDOWN;
    printBase2(timeLeft, 14, 0);
    Serial.println(timeLeft);
  }
  if (alarmTimeLeftBool && alarmTimeLeft <= 0)
  {
    //printBase2(0,14,0); // clear
    Serial.println(F("Cannot enter anymore."));
    lcd.setCursor(14, 0);
    lcd.print(F("//"));
    alarmTimeLeftBool = 0; // we cannot enter anymore
  }
  else if (timeLeftBool && timeLeft <= 0)
  {
    lcd.setCursor(14, 0);
    lcd.print(F("**"));
    clearWrite("Alarm", 0, 1);
    mode = MODE_ALARM;
    soundMode = SOUND_ALARM;
  }

  secs++;
  if (secs >= 60)
  {
    secs = 0;
    mins++;
  }
  if (mins >= 60)
  {
    mins = 0;
    hrs++;
  }
  if (hrs >= 24)
  {
    hrs = 0;
  }
  printTime();
}


/// Zone Functions
void checkZones()
{
  if (mode == MODE_ENG) return; // eng mode disables the alarm
  for (int i = 0; i < 4; i++)
  {
    switch (zones[i])
    {
      case ZONE_ENTRY_EXIT:
        if ((analogRead(zonesPin[i]) < 512 ? LOW : HIGH) != zonesTreshold[i])
        {
          if (alarmTimeLeftBool || checkEntryExitTime(i) || alarmTimeLeftBool) // we can enter
          {
            //mode = MODE_NORMAL;
            //alarmTimeLeftBool = 0;
          }
          else
          {
            if(mode!=MODE_ALARM)Log(EVENT_ALARM, i);
            mode = MODE_ALARM;
            soundMode = SOUND_ALARM;
            return;
          }
        }

        break;
      case ZONE_DIGITAL:
        if ((analogRead(zonesPin[i]) < 512 ? LOW : HIGH) != zonesTreshold[i]) // treshold is HIGH or LOW
        {
          if (alarmTimeLeftBool)
          {
            // we can enter
            //alarmTimeLeftBool = 0;
          }
          else
          {
            // we have logical 0 when alarm is on
            if(mode!=MODE_ALARM)Log(EVENT_ALARM, i);
            mode = MODE_ALARM;
            soundMode = SOUND_ALARM;
            return;
          }
        }
        break;
      case ZONE_ANALOGUE:
        if (analogRead(zonesPin[i]) <= zonesTreshold[i])
        {
          if (alarmTimeLeftBool)
          {
            // we can enter
            //alarmTimeLeftBool = 0;
          }
          else
          {
            if(mode!=MODE_ALARM)Log(EVENT_ALARM, i);
            mode = MODE_ALARM;
            soundMode = SOUND_ALARM;
            return;
          }

        }
        break;
      case ZONE_CONTINUOUS:
        if ((analogRead(zonesPin[i]) < 512 ? LOW : HIGH) != zonesTreshold[i]) // if it changes to low it will keep alarm on
        {
          if (alarmTimeLeftBool)
          {
            // we can enter
            //alarmTimeLeftBool = 0;
          }
          else
          {
            if(mode!=MODE_ALARM)Log(EVENT_ALARM, i);
            constantAlarm=1;
            // we have logical 0 when alarm is on
            mode = MODE_ALARM;
            soundMode = SOUND_ALARM;
            return;
          }

        }
        break;
    }

    // only in case one that none of the sensors is under alarm this will happen
    
    if(mode==MODE_ALARM && !(alarmTimeLeftBool || timeLeftBool)) 
    {
      mode = MODE_NORMAL;
      soundMode = SOUND_ALARM_DISABLED;
      Serial.println(F("Alarm Off"));
      clearWrite("Alarm Off",0,1);
    }
    
  }
}
int checkEntryExitTime(int zone) // time based entry exit
{
  if (zonesEntryExitStartHrs[zone] <= hrs && zonesEntryExitEndHrs[zone] >= hrs && zonesEntryExitStartMins[zone] <= mins && zonesEntryExitEndMins[zone] >= mins)
  {
    return 1;
  }
  else return 0;
}
void changeZoneType(int selectedZone)
{
  Serial.print(F("Selected zone type: "));
  Serial.println(zones[selectedZone]);
  char tmp[50];
  sprintf(tmp, "Zone type:%i", zones[selectedZone]);
  clearWrite(tmp, 0, 1);
  while (1)
  {

    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (key == PLUS) zones[selectedZone]++;
      else if (key == MINUS) zones[selectedZone]--;
      else if (key == NEXT)
      {
        if ((zones[selectedZone] != ZONE_ANALOGUE) && zonesTreshold[selectedZone] > 1)zonesTreshold[selectedZone] = HIGH;
        writeToEEPROM();
        Log(EVENT_CHANGED_CONFIG);
        return;
      }

      if (zones[selectedZone] < ZONE_OFF)zones[selectedZone] = ZONE_CONTINUOUS;
      else if (zones[selectedZone] > ZONE_CONTINUOUS)zones[selectedZone] = ZONE_OFF;

      Serial.print(F("Selected zone type: "));
      Serial.println(zones[selectedZone]);
      char tmp[50];
      sprintf(tmp, "Zone type:%i", zones[selectedZone]);
      clearWrite(tmp, 0, 1);
    }
  }
}
void changeZoneThreshold(int selectedZone)
{
  Serial.print(F("Selected zone threshold: "));
  Serial.println(zonesTreshold[selectedZone]);
  char tmp[50];
  sprintf(tmp, "Threshold:%i", zonesTreshold[selectedZone]);
  clearWrite(tmp, 0, 1);
  while (1)
  {
    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (!(zones[selectedZone] == ZONE_ANALOGUE))
      {
        if (key == PLUS) zonesTreshold[selectedZone] = HIGH;
        if (key == MINUS) zonesTreshold[selectedZone] = LOW;
      }
      else
      {
        if (key == PLUS)
        {
          if (zonesTreshold[selectedZone] < 245)
            zonesTreshold[selectedZone] += 10;
          else if (zonesTreshold[selectedZone] < 255)
            zonesTreshold[selectedZone]++;
        }
        if (key == MINUS)
        {
          if (zonesTreshold[selectedZone] > 10)
            zonesTreshold[selectedZone] -= 10;
          else if (zonesTreshold[selectedZone] > 0)
            zonesTreshold[selectedZone]--;
        }
      }

      if (key == NEXT)
      {
        writeToEEPROM();
        Log(EVENT_CHANGED_CONFIG);
        return;
      }
      Serial.print(F("Selected zone threshold: "));
      Serial.println(zonesTreshold[selectedZone]);
      char tmp1[50];
      sprintf(tmp1, "Threshold:%i", zonesTreshold[selectedZone]);
      clearWrite(tmp1, 0, 1);
    }
  }
}
void printZoneEntryExit(int zone)
{
  Serial.print(F("Hs:"));
  Serial.print(zonesEntryExitStartHrs[zone]);
  Serial.print(F(" Ms:"));
  Serial.print(zonesEntryExitStartMins[zone]);
  Serial.print(F(" He:"));
  Serial.print(zonesEntryExitEndHrs[zone]);
  Serial.print(F(" Me:"));
  Serial.println(zonesEntryExitEndMins[zone]);

  printBase2(zonesEntryExitStartHrs[zone], 0, 1);
  lcd.setCursor(2, 1);
  lcd.print(F(":"));
  printBase2(zonesEntryExitStartMins[zone], 3, 3);
  lcd.setCursor(5, 1);
  lcd.print(F("-"));
  printBase2(zonesEntryExitEndHrs[zone], 6, 1);
  lcd.setCursor(8, 1);
  lcd.print(F(":"));
  printBase2(zonesEntryExitEndMins[zone], 9, 1);
}
void changeZoneEnterExitTyme(int selectedZone)
{
  clearWrite("", 0, 1);
  printZoneEntryExit(selectedZone);
  // set hrs start
  while (1)
  {
    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (key == PLUS && zonesEntryExitStartHrs[selectedZone] < 23) zonesEntryExitStartHrs[selectedZone]++;
      else if (key == MINUS && zonesEntryExitStartHrs[selectedZone] > 0) zonesEntryExitStartHrs[selectedZone]--;
      else if (key == NEXT)
      {
        break;
      }
      printZoneEntryExit(selectedZone);
    }
  }
  // set mins start
  while (1)
  {
    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (key == PLUS && zonesEntryExitStartMins[selectedZone] < 59) zonesEntryExitStartMins[selectedZone]++;
      else if (key == MINUS && zonesEntryExitStartMins[selectedZone] > 0) zonesEntryExitStartMins [selectedZone]--;
      else if (key == NEXT)
      {
        break;
      }
      printZoneEntryExit(selectedZone);
    }
  }

  // set hrs end
  while (1)
  {
    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (key == PLUS && zonesEntryExitEndHrs[selectedZone] < 23) zonesEntryExitEndHrs[selectedZone]++;
      else if (key == MINUS && zonesEntryExitEndHrs[selectedZone] > 0) zonesEntryExitEndHrs[selectedZone]--;
      else if (key == NEXT)
      {
        break;
      }
      printZoneEntryExit(selectedZone);
    }
  }
  // set mins end
  while (1)
  {
    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (key == PLUS && zonesEntryExitEndMins[selectedZone] < 59) zonesEntryExitEndMins[selectedZone]++;
      else if (key == MINUS && zonesEntryExitEndMins[selectedZone] > 0) zonesEntryExitEndMins[selectedZone]--;
      else if (key == NEXT)
      {
        break;
      }
      printZoneEntryExit(selectedZone);
    }
  }
  writeToEEPROM();
  Log(EVENT_CHANGED_CONFIG);
}


/// Change Something Functions
void enterNewengPass()
{
  Serial.println(F("Enter new engineer pass."));
  clearWrite("Enter Eng Pass", 0, 1);
  for (int i = 0; i < 4; i++)
  {
    while (1)
    {
      if (irrecv.decode(&results))
      {
        key = results.value;
        irrecv.resume(); // Receive the next value
        if (key == ONE ||
            key == TWO ||
            key == THREE ||
            key == FOUR ||
            key == FIVE ||
            key == SIX ||
            key == SEVEN ||
            key == EIGHT ||
            key == NINE)
        {
          engPass[i] = key;
          Serial.print(F("*"));
          if (i == 0)clearWrite("", 0, 1);
          lcd.setCursor(i, 1);
          lcd.print(F("*"));
          break;
        }
      }
    }
  }
  writeToEEPROM();
  Log(EVENT_CHANGED_ENG_PASS);
}
void enterNewAlarmPass()
{
  Serial.println(F("Enter new alarm pass."));
  clearWrite("Enter Alarm Pass", 0, 1);
  for (int i = 0; i < 4; i++)
  {
    while (1)
    {
      if (irrecv.decode(&results))
      {
        key = results.value;
        irrecv.resume(); // Receive the next value
        if (key == ONE ||
            key == TWO ||
            key == THREE ||
            key == FOUR ||
            key == FIVE ||
            key == SIX ||
            key == SEVEN ||
            key == EIGHT ||
            key == NINE)
        {
          alarmPass[i] = key;
          Serial.print(F("*"));
          if (i == 0)clearWrite("", 0, 1);
          lcd.setCursor(i, 1);
          lcd.print(F("*"));
          break;
        }
      }
    }
  }
  writeToEEPROM();
  Log(EVENT_CHANGED_ALARM_PASS);
}
void setTime()
{

  Serial.print(F("H:"));
  Serial.print(hrs);
  Serial.print(F(" M:"));
  Serial.println(mins);
  clearWrite("", 0, 1);
  printTime(1);
  // set hrs
  while (1)
  {
    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (key == PLUS && hrs < 23) hrs++;
      else if (key == MINUS && hrs > 0) hrs--;
      else if (key == NEXT)
      {
        break;
      }
      Serial.print(F("H:"));
      Serial.print(hrs);
      Serial.print(F(" M:"));
      Serial.println(mins);
      printTime(1);
    }
  }
  // set mins
  while (1)
  {
    if (irrecv.decode(&results))
    {
      key = results.value;
      irrecv.resume(); // Receive the next value
      if (key == PLUS && mins < 59) mins++;
      else if (key == MINUS && mins > 0) mins--;
      else if (key == NEXT)
      {
        break;
      }
      Serial.print(F("H:"));
      Serial.print(hrs);
      Serial.print(F(" M:"));
      Serial.println(mins);
      printTime(1);
    }
  }
}

/// INI Functions
void setup()
{



  engPass[0] = FOUR; engPass[1] = THREE; engPass[2] = TWO; engPass[3] = ONE;
  alarmPass[0] = ONE; alarmPass[1] = TWO; alarmPass[2] = THREE; alarmPass[3] = FOUR;

  enteredPass[0] = 0; enteredPass[1] = 0; enteredPass[2] = 0; enteredPass[3] = 0;

  zones[0] = ZONE_OFF; zones[1] = ZONE_OFF; zones[2] = ZONE_OFF; zones[3] = ZONE_OFF;
  zonesTreshold[0] = LOW; zonesTreshold[1] = LOW; zonesTreshold[2] = LOW; zonesTreshold[3] = LOW;
  zonesPin[0] = ZONE_ONE_PIN; zonesPin[1] = ZONE_TWO_PIN; zonesPin[2] = ZONE_THREE_PIN; zonesPin[3] = ZONE_FOUR_PIN;

  zonesEntryExitStartHrs[0] = 0; zonesEntryExitStartHrs[1] = 0; zonesEntryExitStartHrs[2] = 0; zonesEntryExitStartHrs[3] = 0;
  zonesEntryExitEndHrs[0] = 0; zonesEntryExitEndHrs[1] = 0; zonesEntryExitEndHrs[2] = 0; zonesEntryExitEndHrs[3] = 0;
  zonesEntryExitStartMins[0] = 0; zonesEntryExitStartMins[1] = 0; zonesEntryExitStartMins[2] = 0; zonesEntryExitStartMins[3] = 0;
  zonesEntryExitEndMins[0] = 0; zonesEntryExitEndMins[1] = 0; zonesEntryExitEndMins[2] = 0; zonesEntryExitEndMins[3] = 0;

  key = 0;
  currPassNum = 0;
  retryLeft = MAX_RETRY;
  timeLeft = MAX_TIME_TO_ENTER_PASSWORD;
  alarmTimeLeft = MAX_TIME_TO_TRIGGER_SENSOR;
  mode = MODE_NORMAL;
  timeLeftBool = 0;
  alarmTimeLeftBool = 0;
  soundMode = SOUND_OFF;
  constantAlarm=0;

  interruptINI();

  currMenu = MENU_CHANGE_ALARM_PASS;
  selectedZone = ZONE_ONE;

  lastLogLoc = EEPROM_LOG_START;

  currNote=0;
  pinMode(6, OUTPUT);

  // LED
  pinMode(10, OUTPUT);

  Serial.begin(9600);
  irrecv.enableIRIn();
  lcd.begin(16, 2);

  secs = 0;
  mins = 0;
  hrs = 0;
  readFromEEPROM();
  
  Serial.println(F("Enter Pass"));
  clearWrite("Enter Pass", 0, 1);
}
void interruptINI()
{
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 15625;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);
  sei();

}
ISR(TIMER1_COMPA_vect) {
  timerInterrupt();
}

/// Sound Functions
int melody[] = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7,
  0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0,  0,
  NOTE_G6, 0, 0, 0,
 
  NOTE_C7, 0, 0, NOTE_G6,
  0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6,
  0, NOTE_AS6, NOTE_A6, 0,
 
  NOTE_G6, NOTE_E7, NOTE_G7,
  NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7,
  NOTE_D7, NOTE_B6, 0, 0,
 
  NOTE_C7, 0, 0, NOTE_G6,
  0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6,
  0, NOTE_AS6, NOTE_A6, 0,
 
  NOTE_G6, NOTE_E7, NOTE_G7,
  NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7,
  NOTE_D7, NOTE_B6, 0, 0
};
byte tempo[] = {
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
};
void playSound()
{
  // all sound must have as short interval as possible
  switch (soundMode)
  {
    case SOUND_BUTTON_PRESSED:
      //toneAC(NOTE_C4, 10, 125);
      analogWrite(SPEAKER_PIN,NOTE_C4);
      delay(100);
      analogWrite(SPEAKER_PIN,0);
      soundMode = SOUND_OFF;
      break;
    case SOUND_TIMER_COUNTDOWN:
      //toneAC(NOTE_D4, 10, 125);
      analogWrite(SPEAKER_PIN,NOTE_D4);
      delay(100);
      //Serial.println("qwe");
      analogWrite(SPEAKER_PIN,0);
      soundMode = SOUND_OFF;
      break;
    case SOUND_INCORRECT_PASSWORD:
      //toneAC(NOTE_E4, 10, 125);
      analogWrite(SPEAKER_PIN,NOTE_E4);
      delay(100);
      analogWrite(SPEAKER_PIN,0);
      soundMode = SOUND_OFF;
      break;
    case SOUND_CORRECT_PASSWORD:
      //toneAC(NOTE_F4, 10, 125);
      analogWrite(SPEAKER_PIN,NOTE_F4);
      delay(100);
      analogWrite(SPEAKER_PIN,0);
      soundMode = SOUND_OFF;
      break;
    case SOUND_ENG_MODE_ENTER:
      //toneAC(NOTE_G4, 10, 125);
      analogWrite(SPEAKER_PIN,NOTE_G4);
      delay(100);
      analogWrite(SPEAKER_PIN,0);
      soundMode = SOUND_OFF;
      break;
    case SOUND_ENG_MODE_EXIT:
      //toneAC(NOTE_A4, 10, 125);
      analogWrite(SPEAKER_PIN,NOTE_A4);
      delay(100);
      analogWrite(SPEAKER_PIN,0);
      soundMode = SOUND_OFF;
      break;
    case SOUND_ALARM:
      if(!(currNote<SPEAKER_MELODY_NUMBER))
      {
        currNote=0;
      }
      else
      {
        currNote++;
      }
      //toneAC(melody[currNote], 10, tempo[currNote]);
      analogWrite(SPEAKER_PIN,melody[currNote]);
      delay(tempo[currNote]*8);
      analogWrite(SPEAKER_PIN,0);
      return; // we keep alarm sound on
  }
  
}


void checkAlarm()
{
  if (mode == MODE_ALARM)
  {
    // change LED
    analogWrite(LED_PIN, 255);
    Serial.println(F("Alarm"));
    clearWrite("Alarm", 0, 1);
    delay(50);
  }
  else
  {
    analogWrite(LED_PIN, LOW);
    currNote=0;
  }
}

void doEngStuff()
{
  switch (currMenu)
  {
    case MENU_CHANGE_ALARM_PASS:
      switch (key)
      {
        case MINUS:
          currMenu = MENU_LOGOUT;
          Serial.println(F("MENU_LOGOUT"));
          clearWrite("Logout", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_CHANGE_ENG_PASS;
          Serial.println(F("MENU_CHANGE_ENG_PASS"));
          clearWrite("Change Eng Pass", 0, 1);
          break;
        case PREV:
          break;
        case NEXT:
          enterNewAlarmPass();
          Serial.println(F("MENU_CHANGE_ALARM_PASS"));
          clearWrite("Change Alarm Key", 0, 1);
          break;
      }

      break;
    case MENU_CHANGE_ENG_PASS:
      switch (key)
      {
        case MINUS:
          currMenu = MENU_CHANGE_ALARM_PASS;
          Serial.println(F("MENU_CHANGE_ALARM_PASS"));
          clearWrite("Change Alarm Key", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_SET_TIME;
          Serial.println(F("MENU_SET_TIME"));
          clearWrite("Set Time", 0, 1);
          break;
        case PREV:
          break;
        case NEXT:
          enterNewengPass();
          Serial.println(F("MENU_CHANGE_ENG_PASS"));
          clearWrite("Change Eng Pass", 0, 1);
          break;
      }
      break;
    case MENU_SET_TIME:
      switch (key)
      {
        case MINUS:
          currMenu = MENU_CHANGE_ENG_PASS;
          Serial.println(F("MENU_CHANGE_ENG_PASS"));
          clearWrite("Change Eng Pass", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_EDIT_ZONES;
          Serial.println(F("MENU_EDIT_ZONES"));
          clearWrite("Edit Zones", 0, 1);
          break;
        case PREV:
          break;
        case NEXT:
          setTime();
          Serial.println(F("MENU_SET_TIME"));
          clearWrite("Set Time", 0, 1);
          break;
      }
      break;

    case MENU_EDIT_ZONES:
      switch (key)
      {
        case MINUS:
          currMenu = MENU_SET_TIME;
          Serial.println(F("MENU_SET_TIME"));
          clearWrite("Set Time", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_LOGOUT;
          Serial.println(F("MENU_LOGOUT"));
          clearWrite("Logout", 0, 1);
          break;
        case PREV:
          break;
        case NEXT:
          currMenu = MENU_EDIT_ZONES_SELECT;
          Serial.println(F("MENU_EDIT_ZONES_SELECT"));
          Serial.print(F("Selected Zone: "));
          selectedZone = ZONE_ONE;
          Serial.println(selectedZone);
          char tmp1[50];
          sprintf(tmp1, "Selected Zone:%i", selectedZone);
          clearWrite(tmp1, 0, 1);
          break;
      }
      break;
    case MENU_LOGOUT:
      switch (key)
      {
        case MINUS:

          currMenu = MENU_EDIT_ZONES;
          Serial.println(F("MENU_EDIT_ZONES"));
          clearWrite("Edit Zones", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_CHANGE_ALARM_PASS;
          Serial.println(F("MENU_CHANGE_ALARM_PASS"));
          clearWrite("Change Alarm Key", 0, 1);
          break;
        case PREV:
          break;
        case NEXT:
          // logout
          mode = MODE_NORMAL;
          soundMode = SOUND_ENG_MODE_EXIT;
          stopTimer();
          resetPass(0);
          Serial.println(F("Logout"));
          //clearWrite("Logout",0,1);
          clearWrite("Enter Pass", 0, 1);
          break;
      }
      break;

    case MENU_EDIT_ZONES_SELECT:
      switch (key)
      {
        case MINUS:
          selectedZone--;
          if (selectedZone < ZONE_ONE) selectedZone = ZONE_FOUR;
          Serial.print(F("Selected zone: "));
          Serial.println(selectedZone);
          char tmp2[50];
          sprintf(tmp2, "Selected zone:%i", selectedZone);
          clearWrite(tmp2, 0, 1);
          break;
        case PLUS:
          selectedZone++;
          if (selectedZone > ZONE_FOUR) selectedZone = ZONE_ONE;
          Serial.print(F("Selected zone: "));
          Serial.println(selectedZone);
          char tmp3[50];
          sprintf(tmp3, "Selected zone:%i", selectedZone);
          clearWrite(tmp3, 0, 1);
          break;
        case PREV:
          currMenu = MENU_EDIT_ZONES;
          Serial.println(F("MENU_EDIT_ZONES"));
          clearWrite("Edit Zones", 0, 1);
          break;
        case NEXT:
          currMenu = MENU_EDIT_ZONES_EDIT_ASSIGN;
          Serial.println(F("MENU_EDIT_ZONES_EDIT_ASSIGN"));
          clearWrite("Assign Zone Type", 0, 1);
          break;
      }
      break;


    case MENU_EDIT_ZONES_EDIT_ASSIGN:
      switch (key)
      {
        case MINUS:
          currMenu = MENU_EDIT_ZONES_EDIT_ENTRY_EXIT_TIME;
          Serial.println(F("MENU_EDIT_ZONES_EDIT_ENTRY_EXIT_TIME"));
          clearWrite("Entry/Exit Time", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_EDIT_ZONES_EDIT_CHANGE_THRESHOLD;
          Serial.println(F("MENU_EDIT_ZONES_EDIT_CHANGE_THRESHOLD"));
          clearWrite("Change Treshold", 0, 1);
          break;
        case PREV:
          currMenu = MENU_EDIT_ZONES_SELECT;
          Serial.println(F("MENU_EDIT_ZONES_SELECT"));
          Serial.print(F("Selected Zone: "));
          selectedZone = ZONE_ONE;
          Serial.println(selectedZone);
          char tmp4[50];
          sprintf(tmp4, "Selected Zone:%i", selectedZone);
          clearWrite(tmp4, 0, 1);
          break;
        case NEXT:
          changeZoneType(selectedZone);
          Serial.println(F("MENU_EDIT_ZONES_EDIT_ASSIGN"));
          clearWrite("Assign Zone Type", 0, 1);
          break;
      }
      break;
    case MENU_EDIT_ZONES_EDIT_CHANGE_THRESHOLD:
      switch (key)
      {
        case MINUS:
          currMenu = MENU_EDIT_ZONES_EDIT_ASSIGN;
          Serial.println(F("MENU_EDIT_ZONES_EDIT_ASSIGN"));
          clearWrite("Assign Zone Type", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_EDIT_ZONES_EDIT_ENTRY_EXIT_TIME;
          Serial.println(F("MENU_EDIT_ZONES_EDIT_ENTRY_EXIT_TIME"));
          clearWrite("Entry/Exit Time", 0, 1);
          break;
        case PREV:
          currMenu = MENU_EDIT_ZONES_SELECT;
          Serial.println(F("MENU_EDIT_ZONES_SELECT"));
          Serial.print("Selected Zone: ");
          selectedZone = ZONE_ONE;
          Serial.println(selectedZone);
          char tmp5[50];
          sprintf(tmp5, "Selected Zone:%i", selectedZone);
          clearWrite(tmp5, 0, 1);
          break;
        case NEXT:
          changeZoneThreshold(selectedZone);
          Serial.println(F("MENU_EDIT_ZONES_EDIT_CHANGE_THRESHOLD"));
          clearWrite("Change Treshold", 0, 1);
          break;
      }
      break;
    case MENU_EDIT_ZONES_EDIT_ENTRY_EXIT_TIME:
      switch (key)
      {
        case MINUS:
          currMenu = MENU_EDIT_ZONES_EDIT_CHANGE_THRESHOLD;
          Serial.println(F("MENU_EDIT_ZONES_EDIT_CHANGE_THRESHOLD"));
          clearWrite("Change Treshold", 0, 1);
          break;
        case PLUS:
          currMenu = MENU_EDIT_ZONES_EDIT_ASSIGN;
          Serial.println(F("MENU_EDIT_ZONES_EDIT_ASSIGN"));
          clearWrite("Assign Zone Type", 0, 1);
          break;
        case PREV:
          currMenu = MENU_EDIT_ZONES_SELECT;
          Serial.println(F("MENU_EDIT_ZONES_SELECT"));
          Serial.print(F("Selected Zone: "));
          selectedZone = ZONE_ONE;
          Serial.println(selectedZone);
          char tmp6[50];
          sprintf(tmp6, "Selected Zone:%i", selectedZone);
          clearWrite(tmp6, 0, 1);
          break;
        case NEXT:
          changeZoneEnterExitTyme(selectedZone);
          Serial.println(F("MENU_EDIT_ZONES_EDIT_ENTRY_EXIT_TIME"));
          clearWrite("Entry/Exit Time", 0, 1);
          break;
      }
      break;
  }


  // change pass - eng and alarm
  // enter new password
  // set time
  //edit zones
  // select zone
  // assign mode
  // change treshold - if digital then only high/low
  // assign entry exit time
  // logout

  // up-minus, down-plus, back-prev, ok-next

}

void loop()
{
  
  if(constantAlarm)
  {
      mode=MODE_ALARM;
      soundMode=SOUND_ALARM;
      checkAlarm();
      playSound();
      delay(50);
      return;
  }
  interruptRemoteController();
  if (key)
  {
    //Serial.println(key, HEX);
    checkButtonPressed(); // key was not 0
    //checkAlarm();
  }
  checkZones(); // TODO: if alarm is password set then do not reset and also disable timers
  checkAlarm();
  //Serial.println(soundMode);
  playSound();

  key = 0;
  //Serial.print(" ");
  //Serial.println(millis());
}

/*
sound
use strings from flash
clear unused or unimportant defines


// ir controller - 1 pin
// analogue in - 4 pins
// lcd display - 12 pins (incl +/-)
// sound - 1 pin
// alarm led analogue out - 1 pin


http://arduinoliquidcrystal.readthedocs.org/en/latest/_images/LCD_bb.png
http://www.danielmorlock.de/wp-content/uploads/2012/12/arduino_ir.png
lcd(12, 11, 7, 4, 3, 2); // !!! 7 !!! lcd(12, 11, 5, 4, 3, 2);
IR_CONTROLLER_PIN 10
LED_PIN 5 // 10
SPEAKER_PIN 6
analog pins for sensors 0-3


*/

