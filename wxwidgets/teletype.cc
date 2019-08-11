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
#include "asr_widget.hh"

#include <wx/graphics.h>

#include "serialport.h"

#include "teletype_logo_128.xpm"

class MyApp: public wxApp
{
  virtual bool OnInit();
};

class MyFrame: public wxFrame, public SimplePort
{
public:
  
  MyFrame(const wxString &title, const wxPoint &pos = wxDefaultPosition,
          const wxSize &size = wxDefaultSize);

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

  //unsigned int ppi;
  wxBoxSizer *top_sizer;
  AsrWidget  *asr_widget;

  //unsigned int GetPPI();
  
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
  MyFrame *frame = new MyFrame( wxT("Teletype") );
  frame->Show(TRUE);
  SetTopWindow(frame);
  return TRUE;
}

void MyFrame::OnClose(wxCloseEvent &event)
{
  wxString message;
  
  if ( event.CanVeto() && asr_widget->Unsaved(message) ) {
    if ( wxMessageBox(message,
                      "Please confirm",
                      wxICON_QUESTION | wxYES_NO) != wxYES ) {
      event.Veto();
      return;
    }
  }
  event.Skip(); // Let the default handler do its thing
}

/*
unsigned int MyFrame::GetPPI()
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
*/

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
  : wxFrame((wxFrame *)NULL, -1, title, pos, size)
    //, ppi(GetPPI())
  , top_sizer(new wxBoxSizer(wxHORIZONTAL))
  , asr_widget(new AsrWidget(this))
{
  /*
  wxMenu *menuFile = new wxMenu;

  menuFile->Append( ID_About, wxT("&About...") );
  menuFile->Append( ID_Read, wxT("&Read...") );
  menuFile->AppendSeparator();
  menuFile->Append( ID_Quit, wxT("E&xit") );

  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append( menuFile, wxT("&File") );

  SetMenuBar( menuBar );
  */

  SetIcon(wxICON(teletype_logo_128));
  
  //CreateStatusBar();
  //SetStatusText( wxT("Welcome to wxWindows!") );

  top_sizer->Add(asr_widget,
                 wxSizerFlags(1).Expand());
    
  SetSizerAndFit(top_sizer);
  Refresh();

  #if 0
  wxSerialPort sp;
  const wxArrayString &n = sp.PortNickNames();
  unsigned int i;
  for (i=0; i<n.GetCount(); i++) {
    std::cout << n[i] << std::endl;
  }
  #endif
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
  //asr_ptp->Punch(ch);
}
