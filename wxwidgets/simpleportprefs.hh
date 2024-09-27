/* Emulate a teleype - simple communication port interface with preferences
 *
 * Copyright (c) 2020  Adrian Wise
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
#ifndef _SIMPLE_PORT_PREFS_
#define _SIMPLE_PORT_PREFS_

#include "simpleport.hh"
#include <wx/wx.h> 
#include <wx/preferences.h> 

class SimplePortPrefs : public SimplePort
{
public:
  virtual wxPreferencesPage *Preferences()
  {
    return 0;
  }
};

#endif // _SIMPLE_PORT_PREFS_
