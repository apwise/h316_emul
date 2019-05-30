/* Emulate paper-tape reader on a teleype
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
#ifndef __ASR_PTR_HH__
#define __ASR_PTR_HH__

#include <vector> 
#include <wx/wx.h> 
#include "papertape.hh"

class AsrPtr: public wxPanel
{
public:
  AsrPtr( wxWindow *parent,
          wxWindowID id = wxID_ANY,
          const wxPoint& pos = wxDefaultPosition,
          const wxSize& size = wxDefaultSize,
          long style = wxTAB_TRAVERSAL,
          const wxString& name = wxT("AsrPtr") );
  ~AsrPtr( );
private:
  enum ButtonId {
    ButtonIdStart = 200,
    ButtonIdAuto,
    ButtonIdStop,
    ButtonIdRel,

    ButtonIdEnd
  };

  static const int ButtonTimerId = 209;
  static const int ButtonBiasTime = 300;

  PaperTape  *papertape;
  wxBoxSizer *top_sizer;
  std::vector<wxRadioButton *>buttons;
  
  bool hasAuto;

  wxTimer *buttonTimer;

  unsigned int GetPPI();
  
  void ButtonBias();
  
  void FileDialog();

  void OnButton(wxCommandEvent &event);
  void OnTimer(wxTimerEvent &event);
  //  void OnContextMenu(wxContextMenuEvent &event);

  DECLARE_EVENT_TABLE() 
};

#endif // __ASR_PTR_HH__
