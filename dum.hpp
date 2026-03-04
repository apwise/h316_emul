/* Honeywell Series 16 emulator
 * Copyright (C) 2026  Adrian Wise
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

#ifndef _DUM_HPP_
#define _DUM_HPP_

#include "p_to_io_intf.hpp"
#include "p_to_dmc_intf.hpp"
#include "iodev.hpp"

class Dum : public PToIoIntf, public PToDmcIntf, public IoDev
{
public:
  Dum(IoToPIntf &p);
  ~Dum(){};

  Status ina(uint16_t instr, int16_t &data);
  void ocp(uint16_t instr);
  Status sks(uint16_t instr);
  Status ota(uint16_t instr, int16_t data);
  void smk(uint16_t mask);

  void event(int reason);

  void set_filename(const std::string &filename, unsigned subdevice); 

  void dmc(unsigned dmc_dev, int16_t &data, bool erl);  
      
private:
  const char *name();
};

#endif // _DUM_HPP_
