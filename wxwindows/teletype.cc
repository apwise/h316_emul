/* Teletype emulation application
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
#include "printedpaper.hh"
#include "asr_ptp.hh"
#include "asr_ptr.hh"
#include "simpleport.hh"

class MyApp: public wxApp
{
    virtual bool OnInit();
};

class MyFrame: public wxFrame, public SimplePort
{
public:
  
  MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnClose(wxCloseEvent &event);
  
  void OnKeyDown(wxKeyEvent& event);
  void OnKeyUp(wxKeyEvent& event);
  void OnChar(wxKeyEvent& event);

  virtual void Process(unsigned char ch, int source);

private:
  static const int CHARACTER_TIMER_ID = 0;

  enum class Sources {
    Keyboard,
    SpecialFunctionKey,
    TapeReader,
    AnswerBack,
    Line
  };
  
  wxBoxSizer   *top_sizer;
  wxBoxSizer   *tape_sizer;
  PrintedPaper *printer;
  AsrPtp       *asr_ptp;
  AsrPtr       *asr_ptr;
  
  DECLARE_EVENT_TABLE()
};

enum
{
    ID_Quit = 1,
    ID_About,
    ID_Read
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(ID_Quit, MyFrame::OnQuit)
EVT_MENU(ID_About, MyFrame::OnAbout)
EVT_CLOSE(MyFrame::OnClose)
END_EVENT_TABLE()


IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
  MyFrame *frame = new MyFrame( wxT("Hello World"), wxPoint(50,50), wxSize(450,340) );
    frame->Show(TRUE);
    SetTopWindow(frame);
    return TRUE;
}

void MyFrame::OnClose(wxCloseEvent &event)
{
  if ( event.CanVeto() && asr_ptp->Unsaved() ) {
    if ( wxMessageBox("The papertape punch has unsaved data... continue closing?",
                      "Please confirm",
                      wxICON_QUESTION | wxYES_NO) != wxYES ) {
      event.Veto();
      return;
    }
  }
  event.Skip(); // Let the default handler do its thing
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
  : wxFrame((wxFrame *)NULL, -1, title, pos, size)
  , top_sizer(new wxBoxSizer(wxHORIZONTAL))
  , tape_sizer(new wxBoxSizer(wxVERTICAL))
  , printer(new PrintedPaper(this, static_cast<int>(Sources::Keyboard), static_cast<int>(Sources::SpecialFunctionKey)))
  , asr_ptp(new AsrPtp(this))
  , asr_ptr(new AsrPtr(this))

{
    wxMenu *menuFile = new wxMenu;

    menuFile->Append( ID_About, wxT("&About...") );
    menuFile->Append( ID_Read, wxT("&Read...") );
    menuFile->AppendSeparator();
    menuFile->Append( ID_Quit, wxT("E&xit") );

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, wxT("&File") );

    SetMenuBar( menuBar );

    CreateStatusBar();
    SetStatusText( wxT("Welcome to wxWindows!") );

    top_sizer->Add(tape_sizer,
                   wxSizerFlags(0).Border().Expand());
    top_sizer->Add(printer,
                   wxSizerFlags(1).Expand());
    printer->SetMinSize(wxSize(500,100));

    tape_sizer->Add(asr_ptp,
                    wxSizerFlags(1).Align(wxALIGN_TOP));
    tape_sizer->AddSpacer(10);
    tape_sizer->Add(asr_ptr,
                    wxSizerFlags(1).Align(wxALIGN_BOTTOM));
    
    SetSizerAndFit(top_sizer);
       
    /*
    printer.Print("HELLO \016CRUEL\017 WORLD\r\n");
    printer.Print("0123456789_\n");
    printer.Print("01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n");
    printer.Print("+/\rX\\\n");
    printer.Print("ABC\n\n");
    printer.Print("DEF");
    */
    
    Refresh();
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
  wxMessageBox(wxT("This is a wxWindows Hello world sample"),
               wxT("About Hello World"), wxOK | wxICON_INFORMATION, this);
}

void MyFrame::Process(unsigned char ch, int source)
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  asr_ptp->Punch(ch);
}
