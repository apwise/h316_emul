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
#include "get_filename_intf.hpp"
#include "iodev.hpp"

namespace h16 {
  class ASR;

  class AsrIntf : public PToIoIntf, public GetFilenameIntf, public IoDev
  {
  public:
    enum class Event {
      MASTER_CLEAR = EVENT_MASTER_CLEAR,
      DUMMY_CYCLE,
      OUTPUT,
      INPUT,
      KEYBRD,

      // These can be used in direct calls to event():
      PTR_ON,
      PTP_ON,
    };

    // Subdevices...
    static const int PTR {0};
    static const int PTP {1};

    AsrIntf(IoToPIntf &p);
    ~AsrIntf();

    IoStatus ina(uint16_t instr, int16_t &data);
    void ocp(uint16_t instr);
    IoStatus sks(uint16_t instr);
    IoStatus ota(uint16_t instr, int16_t data);
    void smk(uint16_t mask);

    void event(int reason);
    void set_filename(const std::string &filename, unsigned subdevice);

    void dmc(unsigned dmc_dev, int16_t &data, bool erl);

    bool special(char c);
        
    const char *name() const;

    // GetFilenameIntf - Just relay the request on the IoToPIntf
    std::string get_file_name(const std::string &device_name,
                              const std::string &extension,
                              const std::string &description) {
      return p.get_file_name(device_name, extension, description);
    }

  private:
    enum class Activity
      {
        NONE,
        OUTPUT,
        INPUT,
        DUMMY
      };
  
    void master_clear();
    ASR *asr;

    uint16_t mask; // just set the one bit for this device

    int data_buf;
    bool ready;
    bool input_pending;

    bool output_mode;
    bool output_pending;
    Activity activity;

  };
}
#endif // _ASR_INTF_HPP_
