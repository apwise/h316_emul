/* Emulate a teleype
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

#include "asr_widget.hh"

BEGIN_EVENT_TABLE(AsrWidget, wxPanel)
EVT_TIMER(wxID_ANY, AsrWidget::OnTimer)
END_EVENT_TABLE()

AsrWidget::AsrWidget( wxWindow      *parent,
                      wxWindowID     id,
                      const wxPoint &pos,
                      const wxSize  &size,
                      long           style,
                      const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , top_sizer(new wxBoxSizer(wxHORIZONTAL))
  , tape_sizer(new wxBoxSizer(wxVERTICAL))
  , printer(new PrintedPaper(this, static_cast<int>(Sources::Keyboard), static_cast<int>(Sources::SpecialFunctionKey)))
  , asr_ptp(new AsrPtp(this))
  , asr_ptr(new AsrPtr(this))
{

  top_sizer->Add(tape_sizer,
                 wxSizerFlags(0).Border().Expand());
  top_sizer->Add(printer,
                 wxSizerFlags(1).Expand());

  tape_sizer->Add(asr_ptp,
                  wxSizerFlags(1).Align(wxALIGN_TOP));
  tape_sizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
  tape_sizer->Add(asr_ptr,
                  wxSizerFlags(1).Align(wxALIGN_BOTTOM));

  character_timer = new wxTimer(this);
  character_timer->Start(timerPeriod, wxTIMER_CONTINUOUS);
  
  SetSizerAndFit(top_sizer);
}

AsrWidget::~AsrWidget()
{
}

bool AsrWidget::Unsaved(wxString &message)
{
  bool r = asr_ptp->Unsaved();

  if (r) {
    message = "The papertape punch has unsaved data... continue closing?";
  }
  return r;
}

void AsrWidget::Process(unsigned char ch, int source)
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  asr_ptp->Punch(ch);
}

void AsrWidget::OnTimer(wxTimerEvent& WXUNUSED(event))
{
  CharacterPoll();
}

/*
 * If we're simulating the extact timing of a teletype then
 * poll all the possible sources of characters and decide
 * where the characters should be sent.
 */
void AsrWidget::CharacterPoll()
{
  bool have_ptr, have_kbd, have_prt;
  unsigned char ptr_ch, kbd_ch, prt_ch;
  bool have_ch = false;
  unsigned char ch = 0;
  bool bell = false;
  
  have_ptr = asr_ptr->Read(ptr_ch);
  have_kbd = printer->PollKeyboard(kbd_ch);

  if (have_ptr) {
    ch |= ptr_ch;
    have_ch = true;
  }

  if (have_kbd) {
    ch |= kbd_ch;
    have_ch = true;
  }

  if (have_ch) {
    printer->Print(ch);
    asr_ptp->Punch(ch);

    if ((ch & 0x7f) == '\a') {
      bell = true;
    }
  }
  
  have_prt = printer->PollPrinter(prt_ch);

  if (have_prt) {
    if ((prt_ch & 0x7f) == '\a') {
      bell = true;
    }

    // Look for here-is...
  }

  if (bell) {
    wxBell();
  }
}
