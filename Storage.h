#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SPI.h>

class Storage {
public:
  enum ChipSelect
  { 
    MEM_1, MEM_2 
  };
  enum Commands
  {
    WREN = 0x06,
    RDSR = 0x05,
    READ = 0x03,
    PP   = 0x02,                                                          // Page Program
    SE   = 0x20,                                                          // Sector Erase
    BE   = 0x60                                                           // Bulk Erase
  };
  Storage(int csPin);
  void init();
    
                                                                          // Working with pages (0...65535)
  void writePage(uint32_t pageAddr, uint8_t* data);
  void readPage(uint32_t pageAddr, uint8_t* data);
  void eraseSector(uint32_t addr);                                        // Sector 4KB
  void eraseChip();                                                       // Full cleanup
  bool isBusy();                                                          // Check status
  void startBulkErase();
  static void writeTo(ChipSelect chip, uint32_t pageAddr, uint8_t* data, Storage& m1, Storage& m2);

private:
  int _cs;
  void writeEnable();
  void waitForReady();
};

#endif
