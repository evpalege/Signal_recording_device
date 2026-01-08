#include "Display.h"

extern ACROBOTIC_SSD1306 oled; 

Display::Display(Buzzer* buzzerPtr, Leds* ledsPtr, Storage* m1Ptr, Storage* m2Ptr) : _currentLine(0), _isEditing(false)
{
  _bz = buzzerPtr; 
  _ld = ledsPtr; 
  _m1 = m1Ptr;
  _m2 = m2Ptr;
  for (int i = 0; i < REDACTOR_ITEMS; i++)
  {
    _stats[i] = 0;
  }
}

void Display::init()
{
  render(); 
}

void Display::update(ButtonHandler::Event event)
{
  if (event == ButtonHandler::NONE) return;

  if (!_isEditing)
  {
    if (event == ButtonHandler::SHORT_PRESS)
    {
      _currentLine = (_currentLine + 1) % MENU_ITEMS_COUNT;
    } 
    else if (event == ButtonHandler::LONG_PRESS) 
    {
      if (_currentLine < REDACTOR_ITEMS)
      {
        _isEditing = true;
      }
      else
      {
        executeAction(_currentLine);
      }
    }
  } 
  else
  {
    if (event == ButtonHandler::SHORT_PRESS)
    {
      incrementValue(_currentLine);
    } 
    else if (event == ButtonHandler::LONG_PRESS)
    {
      _isEditing = false;
    }
  }  
  render();
}

void Display::incrementValue(uint8_t line)
{
  uint8_t limits[] = {6, 20, 11, 2, 16}; 
  if (line < REDACTOR_ITEMS)
  {
    _stats[line] = (_stats[line] + 1) % limits[line];
  }
}

const char* Display::getValueText(uint8_t line)
{
    static const char* s_sensors[] = {"A/C", "A/G", "A/M", "G/M", "G/C", "M/C"};
    static const char* s_freq[]    = {"050", "100", "150", "200", "250", "300", "350", "400", "450", "500", "550", "600", "650", "700", "750", "800", "850", "900", "950", "01K"};
    static const char* s_gain[]    = {"001", "100", "200", "300", "400", "500", "600", "700", "800", "900", "01K"};
    static const char* s_init[]    = {"TIM", "HIT"};
    static const char* s_time[]    = {"05s", "10s", "15s", "20s", "25s", "30s", "35s", "40s", "45s", "50s", "55s", "60s", "02m", "03m", "04m", "05m"};

    switch (line)
    {
      case 0: return s_sensors[_stats[0]];
      case 1: return s_freq[_stats[1]];
      case 2: return s_gain[_stats[2]];
      case 3: return s_init[_stats[3]];
      case 4: return s_time[_stats[4]];
      default: return "";
    }
}

void Display::render()
{
  static const char* labels[] = {"SENSORS:", "FREQ:", "GAIN:", "INIT:", "TIME:", "GETDATA", "START", "CLEAR"};
    
  for (uint8_t i = 0; i < MENU_ITEMS_COUNT; i++)
  {
    oled.setTextXY(i, 0);
    oled.putString((_currentLine == i && !_isEditing) ? ">" : " ");
    oled.putString(labels[i]);

    if (i < REDACTOR_ITEMS)
    {
      oled.setTextXY(i, 11);
      oled.putString((_currentLine == i && _isEditing) ? ">" : " ");
      oled.putString(getValueText(i));
    }
  }
}

bool Display::runTimer(int seconds, ButtonHandler* button)
{
  _bz->silence();
  oled.sendCommand(0xA7);                                                                                 // Screen inversion (white background)
  
  while (seconds >= 0)
  {
    if (button->getEvent() == ButtonHandler::LONG_PRESS)                                                 // Check interruption by button (LONG_PRESS)
    {
      oled.sendCommand(0xA6);                                                                            // Screen return
      return false; 
    }

    oled.clearDisplay();
    oled.setTextXY(CURSOR_X_CENTER, 5); 
    char buf[10];
    sprintf(buf, "%02d:%02d", seconds / 60, seconds % 60);
    oled.putString(buf);

    unsigned long startM = millis();
    while (millis() - startM < 1000)                                                                    // 1-second pause with button polling inside
    {
      if (button->getEvent() == ButtonHandler::LONG_PRESS)
      {
        oled.sendCommand(0xA6);
        return false;
      }
    }
    seconds--;
    _bz->chirp();
  }
  oled.sendCommand(0xA6);                                                                              // Normal mode
  return true;
}

void Display::executeAction(uint8_t line)
{
  if (line == 5)
  {                                                                                                   // GETDATA
    oled.clearDisplay();
    oled.setTextXY(0, 0);
    oled.putString("TRANSFER MODE");
    oled.setTextXY(1, 0);
    oled.putString("Sending to PC...");
  }
  else if (line == 7)
  {                                                                                                  // CLEAR
    showCleaningProgress();
    oled.clearDisplay();
    oled.setTextXY(CURSOR_X_CENTER, 5);
    oled.putString("DONE!");
    _bz->chirp(); 
    delay(1500);
  }
}

                                                                                                    // Getters for external use
int Display::getSelectedFreq()
{
  static int freqs[] = {50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000};
  return freqs[_stats[1]];
}
int Display::getSelectedGain()
{
  static int gains[] = {1, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
  return gains[_stats[2]];
}
int Display::getSelectedTime()
{
  static int times[] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 120, 180, 240, 300};
  return times[_stats[4]];
}
int Display::getSelectedInit()
{ 
  return _stats[3];                                                                                 // 0: TIM, 1: HIT
}
int Display::getSelectedSensors() { return _stats[0]; }

bool Display::renderStorageProgress(uint32_t page1, uint32_t page2, uint32_t totalPages)
{
    uint8_t perc1 = (uint8_t)(((uint64_t)page1 * 100) / totalPages);
    uint8_t perc2 = (uint8_t)(((uint64_t)page2 * 100) / totalPages);

    if (perc1 > 100)
    {
      perc1 = 100;
    }
    if (perc2 > 100)
    {
      perc2 = 100;
    }

                                                                                                    // Drawing M1
    oled.setTextXY(2, 0); oled.putString("M1:");
    oled.setTextXY(2, 7);
    char pBuf[8];
    sprintf(pBuf, "%3d%%", perc1);
    oled.putString(pBuf);

    oled.setTextXY(3, 0);
    uint8_t bar1 = (16 * perc1) / 100;
    for (uint8_t j = 0; j < 16; j++)
    {
      oled.putString(j < bar1 ? "|" : ".");
    }
                                                                                                    // Drawing M2
    oled.setTextXY(5, 0); oled.putString("M2:");
    oled.setTextXY(5, 7);
    sprintf(pBuf, "%3d%%", perc2);
    oled.putString(pBuf);

    oled.setTextXY(6, 0);
    uint8_t bar2 = (16 * perc2) / 100;
    for (uint8_t j = 0; j < 16; j++)
    {
      oled.putString(j < bar2 ? "|" : ".");
    }
    return (perc1 >= 100 && perc2 >= 100);
}


void Display::showCleaningProgress()
{
    oled.clearDisplay();
    oled.setTextXY(CURSOR_X_CENTER, 2);
    oled.putString("CLEANING");
    _m1->startBulkErase();
    _m2->startBulkErase();
    uint8_t d = 0;
    while (_m1->isBusy() || _m2->isBusy())
    {
      oled.setTextXY(CURSOR_X_CENTER, 10);
      if (d == 0) oled.putString(".  ");
      else if (d == 1) oled.putString(".. ");
      else oled.putString("...");
      d = (d + 1) % 3;
      _bz->chirp();
      delay(500); 
    }
}