#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <ACROBOTIC_SSD1306.h>
#include "Button.h"
#include "Leds.h"
#include "Buzzer.h" 
#include "Storage.h"

#define BLINK_INTERVAL 500
#define CURSOR_X_CENTER 4

class Display
{
public:
    static const uint8_t MENU_ITEMS_COUNT = 8;
    static const uint8_t REDACTOR_ITEMS = 5;

    Display(Buzzer* buzzerPtr, Leds* ledsPtr, Storage* m1Ptr, Storage* m2Ptr);
    
    void init();
    void update(ButtonHandler::Event event);
    
    int getSelectedFreq();
    int getSelectedGain();
    int getSelectedTime();
    int getSelectedInit();
    int getSelectedSensors();
    
    uint8_t getCurrentLine() const                                                          // Method for getting the current menu item (needed for the loop)
    { 
        return _currentLine; 
    }

    bool renderStorageProgress(uint32_t page1, uint32_t page2, uint32_t totalPages);        // Drawing recording progress (returns true if memory is full)
    void render();                                                                          // Rendering the main menu
    bool runTimer(int seconds, ButtonHandler* button);                                      // Move runTimer to public so it can be called from loop before starting recording

private:
    uint8_t _currentLine;
    bool _isEditing;
    int _stats[REDACTOR_ITEMS]; 
    
    Buzzer* _bz; 
    Leds* _ld;  
    Storage* _m1; 
    Storage* _m2;

    void incrementValue(uint8_t line);
    void executeAction(uint8_t line);
    const char* getValueText(uint8_t line);
    void showCleaningProgress();
};

#endif
