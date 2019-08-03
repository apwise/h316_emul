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
#include "asr_ptr.hh"

#define BINDEX(b) ((b) - ButtonIdStart)

BEGIN_EVENT_TABLE(AsrPtr, wxPanel)
EVT_RADIOBUTTON(ButtonIdStart, AsrPtr::OnButton)
EVT_RADIOBUTTON(ButtonIdAuto,  AsrPtr::OnButton)
EVT_RADIOBUTTON(ButtonIdStop,  AsrPtr::OnButton)
EVT_RADIOBUTTON(ButtonIdRel,   AsrPtr::OnButton)
EVT_TIMER(ButtonTimerId, AsrPtr::OnTimer)
//EVT_CONTEXT_MENU(AsrPtr::OnContextMenu)
END_EVENT_TABLE()

AsrPtr::AsrPtr( wxWindow      *parent,
                wxWindowID     id,
                const wxPoint &pos,
                const wxSize  &size,
                long           style,
                const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , papertape(new PaperTape(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, /* reader = */ true))
  , top_sizer(new wxBoxSizer(wxVERTICAL))
  , hasAuto(true)
  , buttonTimer(0)
{
  buttons.push_back(new wxRadioButton(this, ButtonIdStart, wxT("START")));
  buttons.push_back(new wxRadioButton(this, ButtonIdAuto,  wxT("AUTO" )));
  buttons.push_back(new wxRadioButton(this, ButtonIdStop,  wxT("STOP" )));
  buttons.push_back(new wxRadioButton(this, ButtonIdRel,   wxT("REL." )));
  
  top_sizer->Add(papertape, wxSizerFlags(1).Expand());

  top_sizer->Add(buttons[BINDEX(ButtonIdStart)], wxSizerFlags(0).Left());
  top_sizer->Add(buttons[BINDEX(ButtonIdAuto )], wxSizerFlags(0).Left());
  top_sizer->Add(buttons[BINDEX(ButtonIdStop )], wxSizerFlags(0).Left());
  top_sizer->Add(buttons[BINDEX(ButtonIdRel  )], wxSizerFlags(0).Left());

  if (hasAuto) {
    buttons[BINDEX(ButtonIdAuto)]->SetValue(true);
  } else {
    buttons[BINDEX(ButtonIdStop)]->SetValue(true);
    buttons[BINDEX(ButtonIdAuto)]->Hide();
  }
    
  unsigned int ppi = GetPPI();
  unsigned int sb_width = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, this);
  
  papertape->SetMinSize(wxSize(ppi + sb_width, 2 * ppi));
  
  SetSizerAndFit(top_sizer);
}

AsrPtr::~AsrPtr()
{
}

bool AsrPtr::Read(unsigned char &ch)
{
  return papertape->Read(ch);
}

unsigned int AsrPtr::GetPPI()
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

void AsrPtr::FileDialog()
{
  wxFileDialog dialog(this,
                      "Load",
                      wxEmptyString, wxEmptyString,
                      "Papertape files (*.ptp)|*.ptp|All files|*",
                      wxFD_OPEN);

  if (dialog.ShowModal() == wxID_CANCEL)
    return;

  std::cout << dialog.GetPath() << std::endl;

  wxFileName filename(dialog.GetPath());
  papertape->Load(filename, 5);
}

void AsrPtr::OnButton(wxCommandEvent &event)
{
  const ButtonId id = static_cast<ButtonId>(event.GetId());
  //std::cout << __PRETTY_FUNCTION__ << " " << event.GetId() << std::endl;

  switch(id) {
  case ButtonIdStart: ButtonBias(); FileDialog(); break;
  case ButtonIdAuto:  break;
  case ButtonIdStop:  ButtonBias(); break;
  case ButtonIdRel:   break;
    
  default:
    /* Ignore */
    break;
  }
}

void AsrPtr::ButtonBias()
{
  if (hasAuto) {
    if (! buttonTimer) {
      buttonTimer = new wxTimer(this, ButtonTimerId);
    }
    buttonTimer->Start(ButtonBiasTime, wxTIMER_ONE_SHOT);
  }
}

void AsrPtr::OnTimer(wxTimerEvent &event)
{
  buttons[BINDEX(ButtonIdAuto)]->SetValue(true);
}
