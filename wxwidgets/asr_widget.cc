/* Emulate a teletype
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

#include "asr_widget.hh"
#include "asr_comms_prefs.hh"

BEGIN_EVENT_TABLE(AsrWidget, wxPanel)
EVT_TIMER(wxID_ANY, AsrWidget::OnTimer)
EVT_CONTEXT_MENU(   AsrWidget::OnContextMenu)
END_EVENT_TABLE()

class AsrPreferencesPage: public wxStockPreferencesPage
{
public:
  AsrPreferencesPage(Kind kind);
  ~AsrPreferencesPage();

  virtual wxWindow *CreateWindow (wxWindow *parent);
  
private:
  wxPanel *panel;
};

AsrPreferencesPage::AsrPreferencesPage(Kind kind)
  : wxStockPreferencesPage(kind)
  , panel(0)
{
}

AsrPreferencesPage::~AsrPreferencesPage()
{
  if (panel) delete panel;
}

wxWindow *AsrPreferencesPage::CreateWindow (wxWindow *parent)
{
  if (!panel) {
    panel = new wxPanel(parent);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxSizerFlags flagsLabel(1), flagsItem(1), flagsLine(1);
    flagsLabel.Align(/*wxALIGN_LEFT|*/wxALIGN_CENTRE_VERTICAL).Border(wxRIGHT, 5);
    flagsItem.Align(/*wxALIGN_RIGHT|*/wxALIGN_CENTRE_VERTICAL);
    flagsLine.Border(wxALL, 10);
 
    wxBoxSizer *h_sizer = new wxBoxSizer(wxHORIZONTAL);

    h_sizer->Add(new wxStaticText(panel, wxID_ANY, "Keyboard parity"), flagsLabel);
    const wxString parityChoices[4] = {"Clear", "Even", "Odd", "Force"};
    wxChoice *parityChoice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          4, parityChoices);
    parityChoice->SetSelection(3);
    h_sizer->Add(parityChoice, flagsItem);

    sizer->Add(h_sizer, flagsLine);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Tom"), flagsLine);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Dick"), flagsLine);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Harry"), flagsLine);

    panel->SetSizerAndFit(sizer);
  }

  return panel;
}

AsrWidget::AsrWidget( wxWindow      *parent,
                      SimplePortPrefs *Line,
                      wxWindowID     id,
                      const wxPoint &pos,
                      const wxSize  &size,
                      long           style,
                      const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , Line(Line)
  , top_sizer(new wxBoxSizer(wxHORIZONTAL))
  , tape_sizer(new wxBoxSizer(wxVERTICAL))
  , printer(new PrintedPaper(this, static_cast<int>(Sources::Keyboard), static_cast<int>(Sources::SpecialFunctionKey)))
  , asr_ptp(new AsrPtp(this))
  , asr_ptr(new AsrPtr(this))
  , preferences(0)
{

  top_sizer->Add(tape_sizer,
                 wxSizerFlags(0).Border().Expand());
  top_sizer->Add(printer,
                 wxSizerFlags(1).Expand());

  tape_sizer->Add(asr_ptp,
                  wxSizerFlags(1).Align(wxALIGN_TOP));
  tape_sizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
  tape_sizer->Add(asr_ptr,
                  wxSizerFlags(1)/*.Align(wxALIGN_BOTTOM)*/);

  character_timer = new wxTimer(this);
  character_timer->Start(timerPeriod, wxTIMER_CONTINUOUS);
  
  SetSizerAndFit(top_sizer);

  preferences = new wxPreferencesEditor("Preferences");
  wxPreferencesPage *commsPrefs = (Line) ? Line->Preferences() : 0;
  if (commsPrefs) {
    preferences->AddPage(commsPrefs);
  }
  preferences->AddPage(new AsrPreferencesPage(wxStockPreferencesPage::Kind_General));
}

AsrWidget::~AsrWidget()
{
  //if (preferences) {
  //  delete preferences;
  //}
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
  bool bell = false;

  printer->Print(ch);
  asr_ptp->Punch(ch);

  if ((ch & 0x7f) == '\a') {
    bell = true;
  }

  if (bell) {
    wxBell();
  }
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
    // Test on local echo needed...
    printer->Print(ch);
    asr_ptp->Punch(ch);

    if (Line) {
      Line->Process(ch);
    }
    
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

void AsrWidget::OnContextMenu(wxContextMenuEvent &event)
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

void AsrWidget::ShowContextMenu(const wxPoint &pos)
{
  wxMenu menu;

  menu.Append(MenuIdPreferences, wxT("&Preferences"));
  
  int id = GetPopupMenuSelectionFromUser(menu, pos);

  switch(id) {
  case MenuIdPreferences: PreferencesDialog(); break;
  default:
    /* Do nothing */
    ;
  }

}

void AsrWidget::PreferencesDialog()
{
  preferences->Show(this);
}
