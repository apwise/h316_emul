/* Honeywell Series 16 emulator
 *
 * Copyright (C) 2018, 2026  Adrian Wise
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
 */

#ifndef _VDMC_HPP_
#define _VDMC_HPP_

#include "p_to_io_intf.hpp"
#include "p_to_dmc_intf.hpp"
#include "iodev.hpp"

#include <bitset>

class Proc;

class VDMC : public PToIoIntf, public PToDmcIntf, public IoDev
{
public:
  VDMC(IoToPIntf &p, unsigned cc_addr);

  Status ina(uint16_t instr, int16_t &data);
  Status sks(uint16_t instr);
  Status ota(uint16_t instr, int16_t data);
  void ocp(uint16_t instr);
  void smk(uint16_t mask);

  void event(int reason);
  void set_filename(const std::string &filename, unsigned subdevice);

  void dmc(unsigned dmc_dev, int16_t &data, bool erl);

private:
  const char *name();
  void master_clear(void);

  bool is_central(unsigned short instr)
  {
    return (device_addr(instr) == cc_addr);
  }
  void reload_time_cntr(unsigned int chan);
  bool chan_error(unsigned int chan);
  bool interrupts(signed short *chan = 0);
  void evaluate_interrupts();

  // Function codes for per-channel device
  static const unsigned int OTA_TRFR_SIZE = 000;
  static const unsigned int SKS_NOT_BUSY  = 001;
  static const unsigned int OTA_DATA_PRNG = 002;
  static const unsigned int SKS_NO_ERROR  = 003;
  static const unsigned int SKS_NOT_INTR  = 004;
  static const unsigned int OTA_TIME_PRNG = 005;
  static const unsigned int OTA_TIME_CTRL = 006;
  //                                        007
  static const unsigned int INA_TRFR_SIZE = 010;
  //                                        011
  static const unsigned int INA_DATA_PRNG = 012;
  //                                        013
  //                                        014
  static const unsigned int INA_TIME_PRNG = 015;
  static const unsigned int INA_TIME_CTRL = 016;
  static const unsigned int INA_CHAN_ERRS = 017;
  // OCPs  
  static const unsigned int OCP_CLEAR_ERR = 003;
  static const unsigned int OCP_CLEAR_INT = 004;
  static const unsigned int OCP_STRT_INPT = 005;
  static const unsigned int OCP_STRT_OUTP = 006;

  // Function codes for central controller device
  //                                        000
  //                        SKS_NOT_BUSY  = 001
  //                                        002
  static const unsigned int SKS_NTRFR_ERR = 003;
  //                        SKS_NOT_INTR  = 004
  //                                   005, 006
  static const unsigned int OTA_CHAN_SLCT = 007;
  //                                  010 - 014
  static const unsigned int INA_ERR_CHAN  = 015;
  static const unsigned int INA_INTR_CHAN = 016;
  static const unsigned int INA_CHAN_SLCT = 017;
  // OCPs
  //                        OCP_CLEAR_ERR = 003
  
  static const unsigned int PRNG_TAPS = 0xA55A;
  
  static const unsigned int SMK_BIT = 3; /* Big-endian bit number */
  static const unsigned int SMK_MASK = (1 << (16-SMK_BIT));

  static const unsigned int NUM_CHANNELS = 16;
  
  enum class Event {
    MASTER_CLEAR = EVENT_MASTER_CLEAR,
    TIMER = 0,
    
    NUM = TIMER + NUM_CHANNELS
  };

  enum class ErrorBits {
    MISSING_EOR,
    UNEXPECTED_EOR,
    DATA_MISMATCH,

    NUM
  };
  
  const unsigned int cc_addr; // Device address of central controller
  static const unsigned int numErrorBits = static_cast<unsigned>(ErrorBits::NUM);

  bool intr_mask;
  unsigned int channel;
  std::bitset<NUM_CHANNELS> chn_interrupt;
  std::bitset<NUM_CHANNELS> busy;
  unsigned short trfr_size[NUM_CHANNELS];
  unsigned short data_prng[NUM_CHANNELS];
  unsigned short time_prng[NUM_CHANNELS];
  unsigned short time_ctrl[NUM_CHANNELS];
  std::bitset<NUM_CHANNELS> unexpected_trfr;
  std::bitset<numErrorBits> error_bits[NUM_CHANNELS];
  std::bitset<NUM_CHANNELS> input_mode;
  unsigned short checksum[NUM_CHANNELS];
  unsigned short trfr_cntr[NUM_CHANNELS];
  unsigned short pending_transfers[NUM_CHANNELS];
  bool last_pending[NUM_CHANNELS];
};

#endif // _VDMC_HPP_
