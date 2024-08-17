/* Emulate paper-tape punch on a teleype
 *
 * Copyright (c) 2019, 2024  Adrian Wise
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
#include <wx/tglbtn.h>
#include "asr_ptp.hh"

AsrPtp::ButtonDescriptor AsrPtp::descriptions[ButtonIdEnd - ButtonIdRel] = {
  /* label  toggle state disable */
  {"REL.",  false, false,  true},
  {"OFF",    true,  true, false},
  {"B.SP.", false, false,  true},
  {"ON",     true, false, false}
};

AsrPtp::AsrPtp( wxWindow      *parent,
                wxWindowID     id,
                const wxPoint &pos,
                const wxSize  &size,
                long           style,
                const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , papertape(new PaperTape(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            /* reader = */ false,
                            wxVERTICAL, PaperTape::PT_TopToBottom))
  , top_sizer(new wxBoxSizer(wxVERTICAL))
  , button_sizer(new wxGridSizer(2, 10, 10))
  , punch_on(false)
{
  int i;

  for (i=ButtonIdRel; i<ButtonIdEnd; i++) {
    button_sizer->Add(ControlButton(i),
                      wxSizerFlags().
                      /*Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).*/
                      Expand());
  }

  top_sizer->Add(button_sizer,
                wxSizerFlags(0).Expand());
  top_sizer->Add(papertape,
                wxSizerFlags(1).Expand());

  unsigned int ppi = GetPPI();
  unsigned int sb_width = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, this);
  
  papertape->SetMinSize(wxSize(ppi + sb_width, 2 * ppi));

  papertape->PunchLeader(5);
  SetSizerAndFit(top_sizer);
}

unsigned int AsrPtp::GetPPI()
{
  wxSize size;
  int r;
  
  wxClientDC dc(this);
  size = dc.GetPPI();

  r = size.GetWidth();
  if (size.GetHeight() > r) {
    r = size.GetHeight();
  }

  return r;
}

wxBoxSizer *AsrPtp::ControlButton(int id)
{
  const int index = id - ButtonIdRel;
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  wxAnyButton *b;
  wxStaticText *t;
  
  if (descriptions[index].toggle) {
    wxToggleButton *tb = new wxToggleButton(this, id, wxT(""),
                                            wxDefaultPosition,
                                            wxSize(BUTTON_SIZE, BUTTON_SIZE));
    tb->SetValue(descriptions[index].state);
    b = tb;
  } else {
    b = new wxButton(this, id, wxT(""),
                     wxDefaultPosition, wxSize(BUTTON_SIZE, BUTTON_SIZE));
  }
  t = new wxStaticText(this, -1, descriptions[index].label);
  
  sizer->AddStretchSpacer();
  sizer->Add(b,
             wxSizerFlags(0).Align(wxALIGN_CENTER).FixedMinSize());
  sizer->Add(t,
             wxSizerFlags(0).Align(wxALIGN_CENTER).FixedMinSize());
  sizer->AddStretchSpacer();

  buttons.push_back(b);
  labels.push_back(t);
  
  if (descriptions[index].disable) {
    b->Disable();
    t->Disable();
  }
  
  return sizer;
}

AsrPtp::~AsrPtp()
{
}

void AsrPtp::CanBsp(bool value)
{
  buttons[ButtonIdBsp - ButtonIdRel]->Enable(value);
  labels[ButtonIdBsp - ButtonIdRel]->Enable(value);
}

void AsrPtp::Punch(unsigned char ch)
{
  if (punch_on) {
    if (! bsp_buffer.empty()) {
      unsigned char bch;
      bch = bsp_buffer.back();
      bsp_buffer.pop_back();
      
      ch |= bch;
    }
    
    papertape->Punch(ch);
    if (papertape->CanBackspace()) {
      CanBsp(true);
    }
  }
}

void AsrPtp::OnOff(ButtonId id, bool force)
{
  wxToggleButton * const tb_on  =
    dynamic_cast<wxToggleButton *>(buttons[ButtonIdOn  - ButtonIdRel]);
  wxToggleButton * const tb_off =
    dynamic_cast<wxToggleButton *>(buttons[ButtonIdOff - ButtonIdRel]);

  if ((tb_on) && (tb_off)) {
    bool v;

    if (id == ButtonIdOn) {
      if (force) {
        tb_on->SetValue(true);
      }
      v = tb_on->GetValue();
      punch_on = v;
      tb_off->SetValue(!v);
    } else if (id == ButtonIdOff) {
      if (force) {
        tb_off->SetValue(true);
      }
      v = tb_off->GetValue();
      punch_on = !v;
      tb_on->SetValue(!v);
    }
  }
}

void AsrPtp::Backspace()
{
  papertape->Backspace();
  if (! papertape->CanBackspace()) {
    CanBsp(false);
  }
}

void AsrPtp::FileDialog(bool attach)
{
  wxFileDialog dialog(this,
                      ((attach) ? "Attach" : "Save"),
                      wxEmptyString, wxEmptyString,
                      "Papertape files (*.ptp)|*.ptp|All files|*",
                      wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

  if (dialog.ShowModal() == wxID_CANCEL)
    return;

  std::cout << dialog.GetPath() << std::endl;

  if (attach) {
    papertape->Attach(dialog.GetPath());
    OnOff(ButtonIdOn,  true);
  } else {
    papertape->Save(dialog.GetPath());
    papertape->PunchLeader();
  }
}

void AsrPtp::Detach()
{
  papertape->CloseAttached();
  papertape->PunchLeader();
}

bool AsrPtp::Unsaved()
{
  bool r = true;

  if (papertape->IsAttached() ||
      (papertape->DataSize() == 0)) {
    r = false;
  }

  return r;
}

void AsrPtp::OnButton(wxCommandEvent &event)
{
  const ButtonId id = static_cast<ButtonId>(event.GetId());
  //std::cout << __PRETTY_FUNCTION__ << " " << event.GetId() << std::endl;

  switch(id) {
  case ButtonIdBsp: Backspace(); break;
  case ButtonIdOn:
  case ButtonIdOff: OnOff(id); break;
    
  default:
    /* Ignore */
    break;
  }
}

void AsrPtp::OnContextMenu(wxContextMenuEvent &event)
{
  wxPoint point = event.GetPosition();

  if (point == wxDefaultPosition) {
    // From keyboard
    wxSize size = GetSize();
    point.x = size.x / 2;
    point.y = size.y / 2;
  } else {
    point = wxDefaultPosition;
  }
  ShowContextMenu(point);
}

void AsrPtp::ShowContextMenu(const wxPoint &pos)
{
  wxMenu menu;

  if (papertape->IsAttached()) {
    menu.Append(MenuIdDetach, wxT("&Detach"));
  } else {
    menu.Append(MenuIdAttach, wxT("&Attach"));
    if (papertape->DataSize() != 0) {
      menu.Append(MenuIdSave, wxT("&Save"));
      menu.Append(MenuIdDiscard, wxT("&Discard"));
    }
  }
  
  menu.AppendSeparator();
  
  if (punch_on) {
    menu.Append(MenuIdOff, wxT("O&ff"));
  } else {
    menu.Append(MenuIdOn, wxT("O&n"));
  }
  
  if (papertape->CanBackspace()) {
    menu.Append(MenuIdBsp, wxT("&Backspace"));
  }
  
  int id = GetPopupMenuSelectionFromUser(menu, pos);
  switch(id) {
  case MenuIdAttach: FileDialog(true); break;
  case MenuIdDetach: Detach(); break;
  case MenuIdSave:   FileDialog(false); break;
  case MenuIdOn:  OnOff(ButtonIdOn,  true); break;
  case MenuIdOff: OnOff(ButtonIdOff, true); break;
  case MenuIdDiscard: {
    wxMessageDialog dialog(this, "Confirm discard", "Confirm", wxOK|wxCANCEL);
    if (dialog.ShowModal() == wxID_OK) {
      papertape->PunchLeader();
    }
  } break;
  case MenuIdBsp: Backspace(); break;
  default:
    /* Do nothing */
    ;
  }
}

BEGIN_EVENT_TABLE(AsrPtp, wxPanel)
EVT_TOGGLEBUTTON(ButtonIdOff, AsrPtp::OnButton)
EVT_BUTTON      (ButtonIdBsp, AsrPtp::OnButton)
EVT_TOGGLEBUTTON(ButtonIdOn,  AsrPtp::OnButton)
EVT_CONTEXT_MENU(             AsrPtp::OnContextMenu)
END_EVENT_TABLE()
