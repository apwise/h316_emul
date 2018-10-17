/* Honeywell Series 16 emulator
 * Copyright (C) 2018, 2024  Adrian Wise
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
#include <deque>
#include <ostream>

#include "iodev.hh"
#include "spi_dev.hh"

class Proc;

class SPI : public IODEV
{
public:
  // Constants for constructor
  static const unsigned int CHIP_SELECTS = 4;
  
  // Defaults for constructor
  static const unsigned int SMK_BIT   =          3; // Big-endian bit number
  static const unsigned int PWR_DLY   =      16800; // Power-up delay
  static const unsigned int DMC_CHN   =          0; // DMC channel minus one
  static const unsigned int BOOT_CMND =     0x960c; // 4FAST, 4-byte addr, 1-bit mode
  static const unsigned int BOOT_CTRL =      0x000; // Dual-SM, 8 dummy
  static const unsigned int BOOT_ADDR = 0x00000000; // Address in SPI Flash

  SPI(Proc *p,
      SpiDev *devices[CHIP_SELECTS],
      
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
    static const unsigned int CHIP_SELECTS = SPI::CHIP_SELECTS;
    
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
      CMND_DUMYW = 0x3, // data[7:5] = -ve cycle count
      CMND_SMODE = 0x4, // data[7:6] = shift mode
      CMND_WPROT = 0x5, // data[7]   = write-protect
      CMND_HALF  = 0xf,

      CMND_SPEC = 0x100
    };
    
    Inner(SpiDev *_devices[CHIP_SELECTS]);
    ~Inner();
    void chip_select(unsigned n);
    unsigned int write(unsigned int data);
    bool rd_valid();
    bool rd_empty();
    void rd_discard();
    bool read(std::uint8_t &data);
    bool accept();
    bool resp_idle();
    
  private:
    SpiDev *devices[CHIP_SELECTS];
    SM sm;
    bool wprot;
    bool idle;
    unsigned select;
    std::deque<std::uint8_t> write_data;
    std::deque<std::uint8_t> read_data;
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
    STATE_SHF3,
    STATE_HMOD,
    STATE_SHF4,
    STATE_DUM2,
    STATE_DUMM,
    STATE_DATA,
    STATE_DONE,
    STATE_DISC
  };

  static const uint8_t MODE_XIP_ENTER = 0xa5;
  static const uint8_t MODE_XIP_EXIT  = 0x5a;

  enum CMND_ADDR {
    CMND_ADDR_NONE,
    CMND_ADDR_CMND,
    CMND_ADDR_ADDR,
    CMND_ADDR_BOTH
  };

  enum MD_DUMMY {
    MD_DUMMY_NONE,
    MD_DUMMY_N_DUM,
    MD_DUMMY_ENTER_DUM,
    MD_DUMMY_EXIT_DUM
  };

  enum CH_SMODE {
    CH_SMODE_NONE,
    CH_SMODE_NEVER,
    CH_SMODE_ACMND,
    CH_SMODE_AADDR,
    CH_SMODE_NBIT
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
  bool last_w;
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
  bool do_abort;
  bool event_pending;
  bool dmc_pending;
  
  void master_clear(void);
  bool spi_pilXX();
  bool ready();
  bool readyc();
  bool check_power();
  void start_command();
  void service_read_data();
  void state_machine(bool strt_cmnd = false);
  void idle_state_machine();

  uint8_t ctrl_cs_addr()   const { return (ctrl >> 14) &    3; }
  bool    ctrl_addr_size() const { return (ctrl >> 13) &    1; }
  bool    ctrl_alt_smode() const { return (ctrl >> 12) &    1; }
  uint8_t ctrl_dummy_reg() const { return (ctrl >>  4) &  0xf; }
  uint8_t ctrl_dummy_mem() const { return  ctrl        &  0xf; }

  uint8_t cmnd_addr()      const { return (cmnd >> 14) &    3; }
  uint8_t cmnd_md_dummy()  const { return (cmnd >> 12) &    3; }
  uint8_t cmnd_ch_smode()  const { return (cmnd >>  9) &    7; }
  bool    cmnd_special()   const { return (cmnd >>  8) &    1; } // Both bit 8
  bool    cmnd_dummy_sel() const { return (cmnd >>  8) &    1; } // Both bit 8
  uint8_t cmnd_cmnd_byte() const { return  cmnd        & 0xff; }

  uint8_t ctrl_dummy_cyc() const { return ((cmnd_dummy_sel()) ?
                                           ctrl_dummy_reg() :
                                           ctrl_dummy_mem()); }
  bool any_dummy() const { return ((ctrl_dummy_cyc() != 0) && (! mode_out)); }
  bool second_dummy() const { return ((ctrl_dummy_cyc() == 0) ||
                                      (ctrl_dummy_cyc() >  8)); } // 9-15 or zero (meaning 16)

  // 000 -> 8 -> 000
  // 001 -> 1 -> 111
  // 010 -> 2 -> 110
  // ...
  // 111 -> 7 -> 001     
  uint8_t first_d_cyc()  const { return (second_dummy()) ? 0 : (~(ctrl_dummy_cyc() & 7) + 1); }
  uint8_t second_d_cyc() const { return (~(ctrl_dummy_cyc() & 7) + 1); }
  
  // Only send the half-byte mode (0xAx, etc.) if this is a read
  // (from SPI) operation - so that the bus must turn around - and
  // there are no dummy cycles (which would serve as bus turn-around
  // time) and the data will be shifted in 2-bit or 4-bit mode
  bool half_byte_mode() const { return ((!mode_out) && (!any_dummy()) &&
                                        ((cmnd_ch_smode() == CH_SMODE_ACMND) |
                                         (cmnd_ch_smode() == CH_SMODE_AADDR))); }

  // The number of cycles required for half a byte of dummy cycles
  // If shift mode is always 1-bit or it changes after address (and mode)
  // then 1-bit needs four cycles -> 100
  // else 2-bit needs two  cycles -> 110
  // else 4-bit needs one  cycle  -> 111
  uint8_t half_d_cyc() const { return (((cmnd_ch_smode() == CH_SMODE_NEVER) ||
                                        (cmnd_ch_smode() == CH_SMODE_AADDR)) ? 4 :
                                       ((ctrl_alt_smode()) ? 7 : 6)); }
    
  uint8_t xip() const { return (cmnd_md_dummy() & 1) ? MODE_XIP_EXIT  : MODE_XIP_ENTER; }
 
  friend std::ostream &operator<<(std::ostream &os, const SPI::SPI_STATE &state);
};

#endif // _SPI_HH_
