/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2018, 2026  Adrian Wise
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

#include "config.h"

#include "io_dispatch.hpp"
#include "iodev.hpp"

#include <cassert>
#include <string>
#include <set>

#include "dum.hpp"
#include "asr_intf.hpp"
#include "ptr.hpp"
#include "ptp.hpp"
#include "lpt.hpp"
#include "rtc.hpp"
#include "plt.hpp"
#if ENABLE_VERIF
#include "vdmc.hpp"
#include "vsim.hpp"
#endif
#if ENABLE_SPI
#include "spi.hpp"
#include "fram.hpp"
#endif

#include "proc.hpp"

#define D(X) static_cast<unsigned>(DEVICE::X)

IoDispatch::IoDispatch(IoToPIntf &p)
  : IoDev(p) {
  io_table.clear();

  Dum *dum = new Dum(p);

  io_table.resize(64, dum);

  io_table[D(ASR)] = new AsrIntf(p);

  io_table[D(PTR)] = new PTR(p);
  io_table[D(PTP)] = new PTP(p);
  io_table[D(LPT)] = new LPT(p);

#if ENABLE_SPI
  FRAM *fram = new FRAM;
  SpiDev *devices[SPI::CHIP_SELECTS] = {fram, 0, 0, 0};

  SPI *spi = new SPI(p, devices);
  io_table[D(SPI)] = spi;
#endif
  
  io_table[D(RTC)] = new RTC(p);
  io_table[D(PLT)] = new PLT(p);

#if ENABLE_VERIF
  io_table[D(VSM)] = new VSIM(p);
  VDMC *vdmc = new VDMC(p, D(VD2));
  io_table[D(VD1)] = vdmc;
  io_table[D(VD2)] = vdmc;
#endif
  
  dmc_table.clear();

  unsigned i=0;
#if ENABLE_SPI
  dmc_table.push_back(static_cast<PToDmcIntf *>(spi));
  i++;
#endif

#if ENABLE_VERIF
  for( ; i<16; i++) {
    dmc_table.push_back(static_cast<PToDmcIntf *>(vdmc));
  }
#endif
  
  for( ; i<16; i++) {
    dmc_table.push_back(static_cast<PToDmcIntf *>(dum));
  }
  
}

IoDispatch::~IoDispatch() {
  std::set<PToIoIntf *> deleted_devices;
  for (auto d: io_table) {
    if (deleted_devices.count(d)==0) {
      deleted_devices.insert(d);
      delete d;
    }
  }
}

void IoDispatch::master_clear_devices()
{
  for (auto d: io_table) {
    d->event(PToIoIntf::EVENT_MASTER_CLEAR);
  }
}

PToIoIntf::Status IoDispatch::ina(uint16_t instr, int16_t &data) {
  return io_table[device_addr(instr)] -> ina(instr, data);
}

PToIoIntf::Status IoDispatch::sks(uint16_t instr) {
  return io_table[device_addr(instr)] -> sks(instr);
}

PToIoIntf::Status IoDispatch::ota(uint16_t instr, int16_t data) {
  return io_table[device_addr(instr)] -> ota(instr, data);
}

void IoDispatch::ocp(uint16_t instr) {
  io_table[device_addr(instr)] -> ocp(instr);
}

void IoDispatch::smk(uint16_t mask) {
  for (auto d: io_table) {
    d->smk(mask);
  }
}

void IoDispatch::dmc(unsigned dmc_dev, int16_t &data, bool erl) {
  assert(dmc_dev < 16);
  PToDmcIntf *d = dynamic_cast<PToDmcIntf *>(dmc_table[dmc_dev]);
  if (d) {
    d -> dmc(dmc_dev, data, erl);
  } else {
    p.anomaly(IoToPIntf::Level::ERROR, "DMC cycle to non-DMC enabled device");
  }
}

void IoDispatch::event(DEVICE dev, unsigned reason) {
  io_table[static_cast<unsigned>(dev)] -> event(reason);
}

void IoDispatch::set_filename(DEVICE dev, const std::string &filename, int subdevice) {
  io_table[static_cast<unsigned>(dev)] -> set_filename(filename, subdevice);
}

const char *IoDispatch::name() {
  return "Dispatch";
}
