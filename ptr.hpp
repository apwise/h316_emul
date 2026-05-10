/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 1999, 2004, 2005, 2026  Adrian Wise
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
#ifndef _PTR_HPP_
#define _PTR_HPP_

#include "p_to_io_intf.hpp"
#include "iodev.hpp"
#include "tty_file.hpp"

namespace h16 {
  
  class PTR : public PToIoIntf, public IoDev
  {
  public:
    PTR(IoToPIntf &p);

    IoStatus ina(uint16_t instr, int16_t &data);
    IoStatus sks(uint16_t instr);
    IoStatus ota(uint16_t instr, int16_t data);
    void ocp(uint16_t instr);
    void smk(uint16_t mask);

    void event(int reason);
    void set_filename(const std::string &filename, unsigned subdevice); 

    void dmc(unsigned dmc_dev, int16_t &data, bool erl);

    const char *name() const;

  private:
    enum Event {
      MASTER_CLEAR = EVENT_MASTER_CLEAR,
      CHARACTER,
    };
  
    void master_clear();
    void open_file();
    void start_reader();

    TTY_file tty_file;
    std::string filename;

    bool eot; // End Of Tape
    unsigned int eot_counter;

    uint16_t mask;
  
    bool tape_running;
    bool ready;
    unsigned char data_buf;

    bool ignore_event;
    int events_queued;

    int data_count;
  };
}
#endif // _PTR_HPP_
