/* Emulate a teleype - communications preferences
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

#ifndef __ASR_COMMS_PREFS_HH__
#define __ASR_COMMS_PREFS_HH__

#include <wx/wx.h> 
#include <wx/preferences.h> 

class AsrCommsPrefs: public wxPreferencesPage
{
public:
  AsrCommsPrefs();
  ~AsrCommsPrefs();

  virtual wxString GetName() const;
#ifdef wxHAS_PREF_EDITOR_ICONS
  virtual wxBitmap GetLargeIcon();
#endif
  virtual wxWindow *CreateWindow (wxWindow *parent);
  
private:
  wxPanel *panel;
  
};

#endif // __ASR_COMMS_PREFS_HH__
