#ifndef _SPI_DEV_HH_
#define _SPI_DEV_HH_

#include <cstdint>
#include <deque>

class SpiDev {

public:
  virtual void write(bool wprot, std::deque<uint8_t> &command) = 0;
  virtual uint8_t read() = 0;

private:
  
};

#endif // _SPI_DEV_HH_
