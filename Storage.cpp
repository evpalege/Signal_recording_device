#include "Storage.h"

Storage::Storage(int csPin) : _cs(csPin)
{

}

void Storage::init()
{
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  SPI.begin();
}

bool Storage::isBusy()
{
  digitalWrite(_cs, LOW);
  SPI.transfer(RDSR);
  uint8_t status = SPI.transfer(0);
  digitalWrite(_cs, HIGH);
  return (status & 0x01);                                                         // Check the WIP (Write In Progress) bit
}

void Storage::waitForReady()
{
  while(isBusy())
  { 
    delayMicroseconds(100);
  }
}

void Storage::writeEnable()
{
  digitalWrite(_cs, LOW);
  SPI.transfer(WREN);
  digitalWrite(_cs, HIGH);
}

void Storage::eraseSector(uint32_t addr)
{
  writeEnable();
  digitalWrite(_cs, LOW);
  SPI.transfer(SE);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8) & 0xFF);
  SPI.transfer(addr & 0xFF);
  digitalWrite(_cs, HIGH);
  waitForReady();
}

void Storage::writePage(uint32_t pageAddr, uint8_t* data)
{
    uint32_t addr = pageAddr * 256; 
    writeEnable();
    digitalWrite(_cs, LOW);
    SPI.transfer(PP);
    SPI.transfer((addr >> 16) & 0xFF);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(data, 256);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
}

void Storage::readPage(uint32_t pageAddr, uint8_t* data)
{
  uint32_t addr = pageAddr * 256;
  digitalWrite(_cs, LOW);
  SPI.transfer(READ);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8) & 0xFF);
  SPI.transfer(addr & 0xFF);
  for (int i = 0; i < 256; i++) 
  {
    data[i] = SPI.transfer(0);
  }
  digitalWrite(_cs, HIGH);
}

void Storage::writeTo(Storage::ChipSelect chip, uint32_t pageAddr, uint8_t* data, Storage& m1, Storage& m2)
{
  if (chip == Storage::MEM_1) {
    m1.writePage(pageAddr, data);
  } 
  else {
    m2.writePage(pageAddr, data);
  }
}

void Storage::startBulkErase()
{
  writeEnable();
  digitalWrite(_cs, LOW);
  SPI.transfer(BE);
  digitalWrite(_cs, HIGH);
}