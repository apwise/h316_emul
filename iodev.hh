/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2018  Adrian Wise
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
#ifndef _IODEV_HH_
#define _IODEV_HH_

class Proc;
class STDTTY;

class IODEV
{
public:
  enum DEVICE
    {
      PTR_DEVICE = 001, // Paper-tape reader
      PTP_DEVICE = 002, // Paper-tape punch
      LPT_DEVICE = 003, // Lineprinter
      ASR_DEVICE = 004, // ASR (teletype)
#if ENABLE_SPI
      SPI_DEVICE = 007, // SPI flash controller
#endif
      RTC_DEVICE = 020, // Real-time clock
      PLT_DEVICE = 027, // Incremental plotter

#if ENABLE_VERIF
      DUM_DEVICE = 034, // Any unused device (to pick up dummy)
      
      // Devices for verification purposes
      VSM_DEVICE = 035, // Simulation exit code
      VD1_DEVICE = 036, // DMC verification - channel
      VD2_DEVICE = 037, // DMC verification - central controller
#else
      DUM_DEVICE = 037, // Any unused device (to pick up dummy)
#endif
    };

  enum STATUS
    {
      STATUS_WAIT,  // Instruction complete I/O not yet ready
      STATUS_READY, // Instruction complete I/O ready (for INA, OTA, SKS)
      STATUS_INCOMPLETE // Instruction incomplete - go to GUI
    };

  IODEV(Proc *p);
  virtual ~IODEV();

  virtual STATUS ina(unsigned short instr, signed short &data);
  virtual STATUS ocp(unsigned short instr);
  virtual STATUS sks(unsigned short instr);
  virtual STATUS ota(unsigned short instr, signed short data);
  virtual STATUS smk(unsigned short mask);

  virtual void dmc(signed short &data, bool erl);
  
  virtual void event(int reason);
  
  static IODEV **dispatch_table(Proc *p, STDTTY *stdtty);
  static IODEV **dmc_dispatch_table(Proc *p, STDTTY *stdtty, IODEV **dt);
  static void master_clear_devices(IODEV **dt);
  static void set_mask(IODEV **dt, unsigned short mask);

  static STATUS status(bool b){return (b)?STATUS_READY:STATUS_WAIT;};

protected:
  Proc *p;
  static const int REASON_MASTER_CLEAR = -1;

  static unsigned short device_addr(unsigned short instr)
  {
    return instr & 077;
  }
  static unsigned short function_code(unsigned short instr)
  {
    return (instr >> 6) & 017;
  }
  
};

#endif //_IODEV_HH_
