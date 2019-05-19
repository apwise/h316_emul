/* Emulate paper-tape punch on a teleype
 *
 * Copyright (c) 2019  Adrian Wise
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
#ifndef __ASR_PTP_HH__
#define __ASR_PTP_HH__

#include <wx/wx.h> 
#include "papertape.hh"

class AsrPtp: public wxPanel
{
public:
  AsrPtp( wxWindow *parent,
          wxWindowID id = wxID_ANY,
          const wxPoint& pos = wxDefaultPosition,
          const wxSize& size = wxDefaultSize,
          long style = wxTAB_TRAVERSAL,
          const wxString& name = wxT("AsrPtp") );
  ~AsrPtp( );
private:
  PaperTape   papertape;
  wxBoxSizer  top_sizer;
  wxGridSizer button_sizer;

  enum AsrPtpButtons {
    AP_REL = wxID_HIGHEST,
    AP_OFF,
    AP_BSP,
    AP_ON
  };
  
  wxBoxSizer *ControlButton(std::string label);
  static const unsigned int BUTTON_SIZE = 20;
};

#endif // __ASR_PTP_HH__
