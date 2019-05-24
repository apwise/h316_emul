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
#include <wx/tglbtn.h>
#include "asr_ptp.hh"

AsrPtp::ButtonDescriptor AsrPtp::descriptions[static_cast<int>(Buttons::NUM)] = {
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
  , papertape(new PaperTape(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, /* reader = */ false))
  , top_sizer(new wxBoxSizer(wxVERTICAL))
  , button_sizer(new wxGridSizer(2, 10, 10))
  , punch_on(false)
{
  unsigned int i;

  for (i=0; i<static_cast<int>(Buttons::NUM); i++) {
    button_sizer->Add(ControlButton(i),
                      wxSizerFlags().
                      Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).
                      Expand());
  }

  top_sizer->Add(button_sizer,
                wxSizerFlags(0).Expand());
  top_sizer->Add(papertape,
                wxSizerFlags(1).Expand());

  papertape->SetMinSize(wxSize(100,100));

  papertape->PunchLeader();
  SetSizerAndFit(top_sizer);
}

wxBoxSizer *AsrPtp::ControlButton(unsigned int index)
{
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  wxAnyButton *b;
  wxStaticText *t;
  
  if (descriptions[index].toggle) {
    wxToggleButton *tb = new wxToggleButton(this, ButtonIdOffset + index, wxT(""),
                                            wxDefaultPosition,
                                            wxSize(BUTTON_SIZE, BUTTON_SIZE));
    tb->SetValue(descriptions[index].state);
    b = tb;
  } else {
    b = new wxButton(this, ButtonIdOffset + index, wxT(""),
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
  const unsigned int index = static_cast<unsigned int>(Buttons::BSP);
  buttons[index]->Enable(value);
  labels[index]->Enable(value);
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
    if (papertape->DataSize() > 0) {
      CanBsp(true);
    }
  }
}

void AsrPtp::OnOff(Buttons b)
{
  const unsigned int i_on  = static_cast<unsigned int>(Buttons::ON );
  const unsigned int i_off = static_cast<unsigned int>(Buttons::OFF);
  wxToggleButton * const tb_on = dynamic_cast<wxToggleButton *>(buttons[i_on]);
  wxToggleButton * const tb_off = dynamic_cast<wxToggleButton *>(buttons[i_off]);

  if ((tb_on) && (tb_off)) {
    bool v;

    if (b == Buttons::ON) {
      v = tb_on->GetValue();
      punch_on = v;
      tb_off->SetValue(!v);
    } else if (b == Buttons::OFF) {
      v = tb_off->GetValue();
      punch_on = !v;
      tb_on->SetValue(!v);
    }
  }
}

void AsrPtp::OnButton(wxCommandEvent &event)
{
  //std::cout << __PRETTY_FUNCTION__ << " " << event.GetId() << std::endl;

  switch(static_cast<Buttons>(event.GetId() - ButtonIdOffset)) {
  case Buttons::BSP: {
    unsigned char ch;
    bool unpunched = papertape->UnPunch(ch);

    if (unpunched) {
      bsp_buffer.push_back(ch);
      if (papertape->DataSize() == 0) {
        CanBsp(false);
      }
    }
  } break;

  case Buttons::ON:  OnOff(Buttons::ON);  break;
  case Buttons::OFF: OnOff(Buttons::OFF); break;
    
  default:
    /* Ignore */
    break;
  }
}

#define ID(e) AsrPtp::ButtonIdOffset + static_cast<int>(AsrPtp::Buttons::e)

BEGIN_EVENT_TABLE(AsrPtp, wxPanel)
EVT_TOGGLEBUTTON(ID(OFF), AsrPtp::OnButton)
EVT_BUTTON      (ID(BSP), AsrPtp::OnButton)
EVT_TOGGLEBUTTON(ID(ON),  AsrPtp::OnButton)
END_EVENT_TABLE()
