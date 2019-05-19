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
#include "asr_ptp.hh"

AsrPtp::AsrPtp( wxWindow      *parent,
                wxWindowID     id,
                const wxPoint &pos,
                const wxSize  &size,
                long           style,
                const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , papertape(this)
  , top_sizer(wxVERTICAL)
  , button_sizer(2, 10, 10)
{
  button_sizer.Add(ControlButton("REL."),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());
  button_sizer.Add(ControlButton("OFF"),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());
  button_sizer.Add(ControlButton("BSP."),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());
  button_sizer.Add(ControlButton("ON"),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());

  top_sizer.Add(&button_sizer,
                wxSizerFlags(0).Expand());
  top_sizer.Add(&papertape,
                wxSizerFlags(1).Expand());

  papertape.SetMinSize(wxSize(100,100));

  SetSizerAndFit(&top_sizer);
}

wxBoxSizer *AsrPtp::ControlButton(std::string label)
{
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

  sizer->AddStretchSpacer();
  sizer->Add(new wxButton(this, wxID_ANY, wxT(""),
                          wxDefaultPosition, wxSize(BUTTON_SIZE, BUTTON_SIZE)),
             wxSizerFlags(0).Align(wxALIGN_CENTER).FixedMinSize());
  sizer->Add(new wxStaticText(this, -1, label),
             wxSizerFlags(0).Align(wxALIGN_CENTER).FixedMinSize());
  sizer->AddStretchSpacer();
  
  return sizer;
}

AsrPtp::~AsrPtp()
{
}

