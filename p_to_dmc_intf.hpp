/* Honeywell Series 16 emulator
 *
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
#ifndef _P_TO_DMC_INTF_HPP_
#define _P_TO_DMC_INTF_HPP_

class PToDmcIntf
{
public:
  virtual void dmc(unsigned dmc_dev, // 0 to 15
                   int16_t &data, bool erl) = 0;
};

#endif // _P_TO_DMC_INTF_HPP_
