/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005  Adrian Wise
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

class Proc;
class STDTTY;

class IODEV
{
public:
  enum DEVICE
    {
      PTR_DEVICE=1,
      PTP_DEVICE=2,
      LPT_DEVICE=3,
      ASR_DEVICE=4
    };

  enum STATUS
    {
      STATUS_WAIT,  // Instruction compleye I/O not yet ready
      STATUS_READY, // Instruction complete I/O ready (for INA, OTA, SKS)
      STATUS_INCOMPLETE // Instruction incomplete - go to GUI
    };

  virtual STATUS ina(unsigned short instr, signed short &data);
  virtual STATUS ocp(unsigned short instr);
  virtual STATUS sks(unsigned short instr);
  virtual STATUS ota(unsigned short instr, signed short data);
  virtual STATUS smk(unsigned short mask);
  
  virtual void event(int reason);
  
  static IODEV **dispatch_table(Proc *p, STDTTY *stdtty);
  static void master_clear_devices(IODEV **dt);
  static void set_mask(IODEV **dt, unsigned short mask);

  static STATUS status(bool b){return (b)?STATUS_READY:STATUS_WAIT;};
};

#define REASON_MASTER_CLEAR -1
