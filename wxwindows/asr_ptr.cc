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

AsrPtr::AsrPtr( wxWindow      *parent,
                wxWindowID     id,
                const wxPoint &pos,
                const wxSize  &size,
                long           style,
                const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , papertape(this)
  , top_sizer(wxVERTICAL)
{
  top_sizer.Add(&papertape,
                wxSizerFlags(1).Expand());

  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("START")),
                wxSizerFlags(0).Left());
  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("STOP")),
                wxSizerFlags(0).Left());
  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("AUTO")),
                wxSizerFlags(0).Left());
  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("REL.")),
                wxSizerFlags(0).Left());

  papertape.SetMinSize(wxSize(100,100));
  
  SetSizerAndFit(&top_sizer);
}

AsrPtr::~AsrPtr()
{
}

