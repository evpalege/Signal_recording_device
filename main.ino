#include <Wire.h>
#include <Arduino_BMI270_BMM150.h> 
#include <ACROBOTIC_SSD1306.h>
#include <mbed.h> 
#include "Button.h"
#include "Leds.h"
#include "Buzzer.h"
#include "Display.h"
#include "Storage.h"
#include "IMUHandler.h"

#define BUZZER_PIN  2
#define BUTTON_PIN  5
#define LED1_PIN    A0
#define LED2_PIN    A1
#define MEM1_CS     9
#define MEM2_CS     10

#define WIRE_SPEED  1000000UL
#define UART_SPEED  921600UL
#define SPI_SPEED   8000000UL
#define BUFF_SIZE   256
#define BUZ_VALUE   3100

enum SystemState
{ 
    MENU, COUNTDOWN, WAIT_HIT, RECORDING, DATA_TRANSFER
};

SystemState currentState = MENU;

mbed::Ticker sampleTicker;      
Buzzer Buzzer(BUZZER_PIN);
Leds Leds(LED1_PIN, LED2_PIN);
ButtonHandler Button(BUTTON_PIN, &Buzzer);
Storage Memory1(MEM1_CS);
Storage Memory2(MEM2_CS);
Display Gui(&Buzzer, &Leds, &Memory1, &Memory2);
IMUHandler Sensors;

uint8_t pageBuffer1[BUFF_SIZE], pageBuffer2[BUFF_SIZE]; 
uint16_t offset1 = 0, offset2 = 0;
volatile bool readyM1 = false, readyM2 = false;
volatile bool timeToReadSensors = false;
uint32_t page1 = 0, page2 = 0;
int selectedMode, selectedFreq;
uint32_t samplesInSecond = 0;
uint32_t lastStatMillis = 0;

void TimerHandler()
{
    timeToReadSensors = true;
}

void startRecording()
{
    page1 = 0; page2 = 0;
    offset1 = 0;      
    offset2 = 0xFFFF; 
    readyM1 = false; readyM2 = false;
    samplesInSecond = 0;
    
    uint32_t intervalUs = 1000000UL / selectedFreq;
    sampleTicker.attach_us(&TimerHandler, intervalUs); 
    oled.clearDisplay();
}

void stopRecording()
{
    sampleTicker.detach(); 
    currentState = MENU;
    oled.clearDisplay();
    Gui.render(); 
}

void setup()
{
    Serial.begin(UART_SPEED); 
    Wire.begin();
    Wire.setClock(WIRE_SPEED); 
    Wire1.setClock(WIRE_SPEED); 

    if (!IMU.begin())
    {
        oled.init();
        oled.clearDisplay();
        oled.putString("IMU ERROR");
        while(true);
    }

    SPI.begin();
    SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE0));

    Buzzer.setVolume(BUZ_VALUE);
    oled.init();
    Gui.init();
    Memory1.init();
    Memory2.init();
    Sensors.set_AllMaxSpeed(); 
    oled.clearDisplay();
    Gui.render();
    Buzzer.chirp();
    Leds.blink(250);
}

void loop()
{
    ButtonHandler::Event ev = Button.getEvent();

    switch (currentState) {
        case MENU:
            Gui.update(ev);
            if (ev == ButtonHandler::LONG_PRESS)
            {
                int line = Gui.getCurrentLine();
                if (line == 6)  // START
                {
                    selectedMode = Gui.getSelectedSensors();
                    selectedFreq = Gui.getSelectedFreq();
                    currentState = COUNTDOWN;
                }
                else if (line == 5) // GETDATA
                {
                    page1 = 0; page2 = 0;
                    selectedMode = Gui.getSelectedSensors();
                    currentState = DATA_TRANSFER;
                    oled.clearDisplay();
                }
            }
            break;

        case COUNTDOWN:
            if (Gui.runTimer(Gui.getSelectedTime(), &Button))
            {
                if (Gui.getSelectedInit() == 0)
                { 
                    startRecording(); 
                    currentState = RECORDING;
                } else
                { 
                    oled.clearDisplay();
                    oled.setTextXY(3, 1);
                    oled.putString("READY! WAIT HIT");
                    Buzzer.chirp();
                    currentState = WAIT_HIT;
                }
            } else
            {
                stopRecording();
            }
            break;

        case WAIT_HIT:
            if (Sensors.checkHit())
            {
                Buzzer.chirp();
                startRecording();
                currentState = RECORDING;
            }
            if (ev == ButtonHandler::LONG_PRESS) stopRecording();
            break;

        case RECORDING:
            if (ev == ButtonHandler::LONG_PRESS)
            {
                stopRecording();
                Buzzer.chirp();
                return;
            }

            if (timeToReadSensors)
            {
                timeToReadSensors = false;
                uint8_t status = Sensors.collectAndPack(selectedMode, pageBuffer1, offset1, pageBuffer2, offset2);
                samplesInSecond++;
                if (status & 1)
                {
                    readyM1 = true;
                }
                if (status & 2)
                {
                    readyM2 = true;
                }
            }

            if (readyM1)
            { 
                Memory1.writePage(page1++, pageBuffer1); 
                readyM1 = false; 
            }
            
            if (readyM2)
            { 
                Memory2.writePage(page2++, pageBuffer2); 
                readyM2 = false; 
            }
            /* DEBUG: DATA WRITE SPEED
            if (millis() - lastStatMillis > 1000)
            {
                Serial.print(F("Freq: ")); Serial.print(samplesInSecond); 
                Serial.print(F(" Hz | M1 Pg: ")); Serial.print(page1);
                Serial.print(F(" | M2 Pg: ")); Serial.println(page2);
                samplesInSecond = 0;
                lastStatMillis = millis();
            }
            */
            static uint32_t guiT = 0;
            if (millis() - guiT > 200)
            {
                guiT = millis();
                if (Gui.renderStorageProgress(page1, page2, 65536))
                {
                    stopRecording();
                }
            }
            break;

        case DATA_TRANSFER:
            {
                const uint32_t totalPages = 65536;
                static uint8_t transferBuf[256];
                static bool modeSent = false;

                if (!modeSent) // Send once when transmitting data (sensors)
                {
                    uint8_t m = (uint8_t)selectedMode;
                    Serial.write(m); 
                    modeSent = true;
                    return; 
                }
                // Sequential unloading data
                if (page1 < totalPages)
                {
                    Memory1.readPage(page1, transferBuf);
                    Serial.write(transferBuf, 256);
                    page1++;
                } 
                else if (page2 < totalPages)
                {
                    Memory2.readPage(page2, transferBuf);
                    Serial.write(transferBuf, 256);
                    page2++;
                } 
                else    // end of transmitting
                {
                    modeSent = false; // Сброс флага для следующего раза
                    Buzzer.chirp();
                    stopRecording();
                    return;
                }
                // Loading indicator
                static uint32_t lastUpdate = 0;
                if (millis() - lastUpdate > 300)
                {
                    lastUpdate = millis();
                    Gui.renderStorageProgress(page1, page2, totalPages);
                }

                // Cancel job
                if (ev == ButtonHandler::LONG_PRESS)
                {
                    modeSent = false;
                    Buzzer.chirp();
                    stopRecording();
                }
            }
            break;
    }
}