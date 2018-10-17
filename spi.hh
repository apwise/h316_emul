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

#include "iodev.hh"

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
  void master_clear(void);
  
  typedef const unsigned int cui_t; // Shorthand for the following declarations
  
  // Function codes
  static cui_t OCP_BOOT   = 000; // Start a boot operation
  // Ignore               001 e.g. Stop paper tape reader
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
    SPI_REASON_TIMER = 0,
    
    SPI_REASON_NUM
  };

  // Constructor configuration constants
  cui_t smk_mask;
  cui_t pwr_dly;
  cui_t dmc_chn;
  cui_t boot_cmnd;
  cui_t boot_ctrl;
  cui_t boot_addr;

  bool intr_mask;
};

#endif // _SPI_HH_
