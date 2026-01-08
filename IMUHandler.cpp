#include "IMUHandler.h"
#include <Wire.h>
#include <Arduino.h> 

#define BMI270_ADDR 0x68
#define BMM150_ADDR 0x10 
#define REG_ACC_DATA 0x0C 
#define REG_GYR_DATA 0x12 

IMUHandler::IMUHandler() {}

int IMUHandler::getFrequency() { return 1000; }

void IMUHandler::set_AllMaxSpeed()
{
    Wire1.beginTransmission(BMI270_ADDR);                       // Setting the maximum speed/range for the Accelerometer // 0x41 = ACC_RANGE, 0x8C = ODR 1600Hz, Range +/- 16G, Normal mode
    Wire1.write(0x41); 
    Wire1.write(0x8C); 
    Wire1.endTransmission();
    Wire1.beginTransmission(BMI270_ADDR);                       // Setting the maximum speed/range for the Gyroscope // 0x43 = GYR_RANGE, 0xE9 = ODR 3200Hz, Range +/- 2000dps, Normal mode
    Wire1.write(0x43); 
    Wire1.write(0xE9); 
    Wire1.endTransmission();
}

void IMUHandler::readAcc(int16_t* dest)
{
    Wire1.beginTransmission(BMI270_ADDR);
    Wire1.write(REG_ACC_DATA);
    Wire1.endTransmission(false);                               // Using Restart (false)
    Wire1.requestFrom(BMI270_ADDR, 6);
    if (Wire1.available() == 6)
    {
        dest[0] = (int16_t)(Wire1.read() | (Wire1.read() << 8));
        dest[1] = (int16_t)(Wire1.read() | (Wire1.read() << 8));
        dest[2] = (int16_t)(Wire1.read() | (Wire1.read() << 8));
    }
}

void IMUHandler::readGyr(int16_t* dest)
{
    Wire1.beginTransmission(BMI270_ADDR);
    Wire1.write(REG_GYR_DATA);
    Wire1.endTransmission(false);
    Wire1.requestFrom(BMI270_ADDR, 6);
    if (Wire1.available() == 6)
    {
        dest[0] = (int16_t)(Wire1.read() | (Wire1.read() << 8));
        dest[1] = (int16_t)(Wire1.read() | (Wire1.read() << 8));
        dest[2] = (int16_t)(Wire1.read() | (Wire1.read() << 8));
    }
}

void IMUHandler::readMag(int16_t* dest)
{
    Wire1.beginTransmission(BMM150_ADDR);
    Wire1.write(0x42);
    Wire1.endTransmission(false);
    Wire1.requestFrom(BMM150_ADDR, 6);
    if (Wire1.available() == 6)
    {
        dest[0] = (int16_t)(Wire1.read() | (Wire1.read() << 8)) >> 3;
        dest[1] = (int16_t)(Wire1.read() | (Wire1.read() << 8)) >> 3;
        dest[2] = (int16_t)(Wire1.read() | (Wire1.read() << 8)) >> 5;
    }
}

void IMUHandler::readCoi(int16_t* dest)
{
    int16_t val = (int16_t)analogRead(A0);
    dest[0] = val; dest[1] = val; dest[2] = val;
}

bool IMUHandler::checkHit() {
    const float threshold_multiplier = 2.5; 
    const int16_t noise_floor = 500;
    int16_t acc[3];
    int16_t last_val = 0;
    readAcc(acc);
    last_val = abs(acc[0]);
    while(true)
    {
        delay(10);
        readAcc(acc);
        int16_t current_val = abs(acc[0]);
        if(current_val > (last_val * threshold_multiplier) && current_val > noise_floor)
        {
            break;
        }
        last_val = current_val;
    }
    return true;
}

uint8_t IMUHandler::collectAndPack(uint8_t mode, uint8_t* buf1, uint16_t& off1, uint8_t* buf2, uint16_t& off2)  // FUNCTION FOR PING-PONG RECORDING
{
    uint32_t currentMicros = micros();
    int16_t s1[3], s2[3];
    
    switch(mode)                                                                                                // Read both sensors at the same time
    {
        case 0: readAcc(s1); readCoi(s2); break;
        case 1: readAcc(s1); readGyr(s2); break;
        case 2: readAcc(s1); readMag(s2); break;
        case 3: readGyr(s1); readMag(s2); break;
        case 4: readGyr(s1); readCoi(s2); break;
        case 5: readMag(s1); readCoi(s2); break;
    }

    uint8_t readyStatus = 0;

    uint8_t* activeBuf = (off2 == 0xFFFF) ? buf1 : buf2;                                                        // DETERMINE THE ACTIVE BUFFER (M1 or M2) // If off2 == 0xFFFF, it means the queue is currently M1
    uint16_t& activeOff = (off2 == 0xFFFF) ? off1 : off2;

    memcpy(activeBuf + activeOff, s1, 6);                                                                       // PACK BOTH SENSORS INTO ONE PACKAGE (16 bytes) // [0-5] Sensor 1 | [6-11] Sensor 2 | [12-15] Time
    memcpy(activeBuf + activeOff + 6, s2, 6);         
    memcpy(activeBuf + activeOff + 12, &currentMicros, 4); 
    
    activeOff += 16;

    if (activeOff >= 256)                                                                                       // SWITCHING MEMORY WHEN LOADING THE PAGE
    {
        if (off2 == 0xFFFF)
        {
            readyStatus |= 1;
            activeOff = 0;
            off2 = 0;
        } else
        {
            readyStatus |= 2;
            activeOff = 0;
            off2 = 0xFFFF;
        }
    }
    return readyStatus;
}