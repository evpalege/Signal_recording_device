#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H

#include <Arduino_BMI270_BMM150.h>
#include <Wire.h>
#include <Arduino.h>

class IMUHandler {
public:
    IMUHandler();
    int getFrequency();                                                                                 // Returns the current frequency (informative)
    void set_AllMaxSpeed();                                                                             // Setting up BMI270/BMM150 for high speeds via Wire1
    void readAcc(int16_t* dest);                                                                        // Reading methods (take a pointer to an array of 3 elements)
    void readGyr(int16_t* dest);
    void readMag(int16_t* dest);
    void readCoi(int16_t* dest);                                                                        // Reading analog signal (emulation of 3 axes for packaging)
    uint8_t collectAndPack(uint8_t mode, uint8_t* buf1, uint16_t& off1, uint8_t* buf2, uint16_t& off2); // Main method for packing data into the buffer (16 bytes)
                                                                                                        // Returns a bitmask of buffer readiness (1 - buf1 ready, 2 - buf2 ready)
    bool checkHit();                                                                                    // Waiting for a sudden change in acceleration

private:

};

#endif
