/*
 * hworld.cpp
 * Hello world sample by Robert Roebling
 */

#include <wx/wx.h> 
#include <wx/tglbtn.h>
#include <wx/stattext.h>
#include <wx/dcclient.h>
#include <wx/timer.h>


#include <cmath>
#include <iostream>

#include <list>

#include "papertape.hh"
#include "papertapereader.hh"
#include "teleprinter.hh"



class MyApp: public wxApp
{
    virtual bool OnInit();
};

class MyFrame: public wxFrame
{
public:

    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnRead(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);

private:
  wxTimer *timer;
  PaperTapeReader *reader;
  Teleprinter *printer;

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
    EVT_MENU(ID_Read, MyFrame::OnRead)
  EVT_TIMER(-1, MyFrame::OnTimer)
END_EVENT_TABLE()


IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
  MyFrame *frame = new MyFrame( wxT("Hello World"), wxPoint(50,50), wxSize(450,340) );
    frame->Show(TRUE);
    SetTopWindow(frame);
    return TRUE;
} 

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
  : wxFrame((wxFrame *)NULL, -1, title, pos, size),
    timer(0)
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

    /* Adrian */

    //reader = new PaperTapeReader(this);
    printer = new Teleprinter(this);

    printer->Print("HELLO \016CRUEL\017 WORLD\r\n");
    printer->Print("0123456789_\n");
    printer->Print("01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n");
    printer->Print("+/\rX\\\n");
    printer->Print("ABC\n\n");
    printer->Print("DEF");
    
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

void MyFrame::OnRead(wxCommandEvent& WXUNUSED(event))
{
  //std::cout << "OnRead" << std::endl;
  timer = new wxTimer(this);
  timer->Start(2);
}

void MyFrame::OnTimer(wxTimerEvent& WXUNUSED(event))
{
  int data = reader->Read();
  
  if (data < 0)
    timer->Stop();
//   else
//     std::cout << "OnTimer " << data << std::endl;
}
