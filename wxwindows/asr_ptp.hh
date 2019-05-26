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

#include <list> 
#include <vector> 
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

  void Punch(unsigned char ch);
  bool Unsaved();
  
private:
  PaperTape   *papertape;
  wxBoxSizer  *top_sizer;
  wxGridSizer *button_sizer;
  bool punch_on;
  
  enum ButtonId {
    ButtonIdRel = 200,
    ButtonIdOff,
    ButtonIdBsp,
    ButtonIdOn,

    ButtonIdEnd
  };

  enum MenuId {
    MenuIdAttach = 210,
    MenuIdDetach,
    MenuIdSave,
    MenuIdDiscard,
    MenuIdOn,
    MenuIdOff,
    MenuIdBsp
  };
  
  struct ButtonDescriptor {
    const char *label;
    bool toggle;
    bool state;
    bool disable;
  };
  static ButtonDescriptor descriptions[ButtonIdEnd - ButtonIdRel];
  std::vector<wxAnyButton *>buttons;
  std::vector<wxStaticText *>labels;
  
  std::list<unsigned char> bsp_buffer;
  
  wxBoxSizer *ControlButton(int id);
  static const unsigned int BUTTON_SIZE = 20;

  void CanBsp(bool value);
  void OnOff(ButtonId id, bool force = false);
  void FileDialog(bool attach);
  void Detach();

  void ShowContextMenu(const wxPoint &pos);
  void Backspace();
  void OnButton(wxCommandEvent &event);
  void OnContextMenu(wxContextMenuEvent &event);

  DECLARE_EVENT_TABLE()
};

#endif // __ASR_PTP_HH__
