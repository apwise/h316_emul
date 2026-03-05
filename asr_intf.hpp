/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2026  Adrian Wise
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

#ifndef _ASR_INTF_HPP_
#define _ASR_INTF_HPP_

#include "p_to_io_intf.hpp"
#include "iodev.hpp"

class Asr;

class AsrIntf : public PToIoIntf, public IoDev
{
public:
  enum class Event {
    MASTER_CLEAR = EVENT_MASTER_CLEAR,
    DUMMY_CYCLE,
    OUTPUT,
    INPUT,

    // These can be used in direct calls to event():
    PTR_ON,
    PTP_ON,
  };

  // Subdevices...
  static const int PTR {0};
  static const int PTP {1};

  AsrIntf(IoToPIntf &p);

  Status ina(uint16_t instr, int16_t &data);
  void ocp(uint16_t instr);
  Status sks(uint16_t instr);
  Status ota(uint16_t instr, int16_t data);
  void smk(uint16_t mask);

  void event(int reason);
  void set_filename(const std::string &filename, unsigned subdevice);

  void dmc(unsigned dmc_dev, int16_t &data, bool erl);

  bool special(char c);
        
private:
  enum class Activity
  {
    NONE,
    OUTPUT,
    INPUT,
    DUMMY
  };
  
  const char *name();
  void master_clear(void);
  Asr *asr;

  uint16_t mask; // just set the one bit for this device

  int data_buf;
  bool ready;
  bool input_pending;

  bool output_mode;
  bool output_pending;
  Activity activity;

};

#endif // _ASR_INTF_HPP_
