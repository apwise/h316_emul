/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2010, 2011, 2026  Adrian Wise
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

#ifndef _CPU_RTL_HPP_
#define _CPU_RTL_HPP_

#include "cpu.hpp"

namespace h16 {
  
  class CpuRtl : public CPU {
  public:
    CpuRtl(bool HasEa);
    virtual ~CpuRtl();

    virtual void master_clear();

    void sampled_io(bool x_drlin, uint16_t x_inb) {
      drlin=x_drlin;
      inb=x_inb;
    }

    void do_instr(bool &run_flag) {
      run_flag = CPU::do_instr();
    }

  private:
    virtual IoStatus ina(uint16_t instr, int16_t &data);
    virtual IoStatus sks(uint16_t instr);
    virtual IoStatus ota(uint16_t instr, int16_t data);
    virtual void ocp(uint16_t instr);
    virtual void smk(uint16_t mask);
    virtual void event(IoDevice dev, int reason);
    virtual bool jump_time_to_event(uint64_t &half_cycles);
    virtual void io_polling(uint16_t instr);
    virtual void dmc(unsigned dmc_dev, // 0 to 15
                     int16_t &data, bool erl);
  
    /*
     * Sampled I/O values
     *
     * Used only in RTL simulations
     */
    bool drlin;
    uint16_t inb;

  };
}
#endif // _CPU_RTL_HPP_
