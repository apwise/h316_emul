/* Honeywell Series 16 emulator
 * Copyright (C) 2018  Adrian Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307 USA
 *
 */

#ifndef _SPI_HH_
#define _SPI_HH_

#include <cstdint>
#include <queue>

#include "iodev.hh"
#include "flash.hh"

class Proc;

class SPI : public IODEV
{
public:
  // Defaults for constructor
  static const unsigned int SMK_BIT   =          3; // Big-endian bit number
  static const unsigned int PWR_DLY   =      16800; // Power-up delay
  static const unsigned int DMC_CHN   =          0; // DMC channel minus one
  static const unsigned int BOOT_CMND =     0x960c; // 4FAST, 4-byte addr, 1-bit mode
  static const unsigned int BOOT_CTRL =      0x000; // Dual-SM, 8 dummy
  static const unsigned int BOOT_ADDR = 0x00000000; // Address in SPI Flash

  SPI(Proc *p,
      unsigned int smk_bit   = SMK_BIT,
      unsigned int pwr_dly   = PWR_DLY,
      unsigned int dmc_chn   = DMC_CHN,
      unsigned int boot_cmnd = BOOT_CMND,
      unsigned int boot_ctrl = BOOT_CTRL,
      unsigned int boot_addr = BOOT_ADDR
      );
  ~SPI();
  
  STATUS ina(unsigned short instr, signed short &data);
  STATUS ocp(unsigned short instr);
  STATUS sks(unsigned short instr);
  STATUS ota(unsigned short instr, signed short data);
  STATUS smk(unsigned short mask);

  void event(int reason);
  void dmc(signed short &data, bool erl);

private:
  class Inner
  {
  public:
    enum SM {
      SM_NONE,
      SM_ONE,
      SM_TWO,
      SM_FOUR
    };
    
    enum CMND {
      CMND_READ  = 0x0,
      CMND_IDLE  = 0x1,
      CMND_DUMMY = 0x2, // data[7:5] = -ve cycle count
      CMND_SMODE = 0x3, // data[7:6] = shift mode
      CMND_WPROT = 0x4, // data[7]   = write-protect
      CMND_HALF  = 0xf,

      CMND_SPEC = 0x100
    };
    
    Inner();
    ~Inner();
    unsigned int write(unsigned int data);
    bool rd_valid();
    bool read(std::uint8_t &data);
    bool accept();
    bool resp_idle();
    
  private:
    Flash flash;
    SM sm;
    bool wprot;
    bool idle;
    std::queue<std::uint8_t> read_data;
  };
  
  typedef const unsigned int cui_t; // Shorthand for the following declarations
  
  // Function codes
  static cui_t OCP_BOOT   = 000; // Start a boot operation
  // Ignore                 001 e.g. Stop paper tape reader
  static cui_t OCP_OUTP   = 002; // Enable output mode (to SPI Flash)
  static cui_t OCP_INP    = 003; // Enable input mode
  static cui_t OCP_MD16   = 004; // Enable 16-bit mode
  static cui_t OCP_MD8    = 005; // Enable 8-bit mode
  static cui_t OCP_DMC    = 006; // Enable DMC mode
  static cui_t OCP_PIO    = 007; // Enable Programmed I/O mode
  static cui_t OCP_CWPR   = 010; // Clear (hardware) write protect
  static cui_t OCP_SWPR   = 011; // Set (hardware) write protect
  static cui_t OCP_DLAB   = 012; // Disable load address at boot
  static cui_t OCP_ELAB   = 013; // Enable load address at boot
  static cui_t OCP_DBMT   = 014; // Disarm boot mode trigger
  static cui_t OCP_ABMT   = 015; // Enable boot mode trigger
  static cui_t OCP_LAST   = 016; // Mark next read/write data as last
  static cui_t OCP_FCCI   = 017; // Finish current command and go idle
  
  static cui_t SKS_RDY    = 000; // Skip if ready
  static cui_t SKS_RDYC   = 001; // Skip if ready for command
  static cui_t SKS_RLRD   = 002; // Skip if ready for low-level read data
  static cui_t SKS_NBUSY  = 003; // Skip if not busy 
  static cui_t SKS_NINTR  = 004; // Skip if not interrupting
  
  static cui_t INA_RDAT   = 000; // Input read data
  static cui_t INA_RLRD   = 002; // Input low-level read data
  static cui_t INA_CDAT   = 010; // Clear A and input read data
  static cui_t INA_CLRD   = 012; // Clear A and input low-level read data
  static cui_t INA_CTRL   = 015; // Clear A and input control register
  static cui_t INA_ARGH   = 016; // Clear A and input address register high
  static cui_t INA_ARGL   = 017; // Clear A and input address register low
  
  static cui_t OTA_WDAT   = 000; // Output data
  static cui_t OTA_CMND   = 001; // Output command
  static cui_t OTA_CTRL   = 005; // Output control register
  static cui_t OTA_ARGH   = 006; // Output address register high
  static cui_t OTA_ARGL   = 007; // Output address register low


  enum SPI_REASON {
    SPI_REASON_STATE = 0,
    SPI_REASON_NUM
  };

  enum SPI_STATE {
    STATE_POWR,
    STATE_IDLE,
    STATE_LOWL,
    STATE_WPRT,
    STATE_CMND,
    STATE_SHF1,
    STATE_ADR1,
    STATE_ADR2,
    STATE_ADR3,
    STATE_ADR4,
    STATE_SHF2,
    STATE_MODE,
    STATE_DUMM,
    STATE_DATA,
    STATE_WLST,
    STATE_DONE,
    STATE_DISC
  };

  // Constructor configuration constants
  cui_t smk_mask;
  cui_t pwr_dly;
  cui_t dmc_chn;
  cui_t boot_cmnd;
  cui_t boot_ctrl;
  cui_t boot_addr;

  Inner inner;

  bool intr_mask;
  bool intr_dmc;
  bool bt_trig;
  bool mode_out;
  bool mode_16;
  bool mode_dmc;
  bool wprot;
  bool laddrbt;
  bool last;
  unsigned long long reset_time;
  
  SPI_STATE state;
  bool wprot_ll;
  bool boot;
  uint16_t data_reg;
  uint16_t ctrl;
  uint32_t addr;
  bool data_l;
  bool data_h;
  uint16_t cmnd;
  bool data_last;
  bool read_paused;
  bool do_abort;
  
  void master_clear(void);
  bool spi_pilXX();
  bool ready();
  bool readyc();
  bool check_power();
  void start_command();
  void state_machine(bool start = false);

  friend std::ostream &operator<<(std::ostream &os, const SPI::SPI_STATE &state);
};

#endif // _SPI_HH_
