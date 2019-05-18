/*
 * hworld.cpp
 * Hello world sample by Robert Roebling
 */

#include <wx/wx.h> 
#include <wx/laywin.h>

#include <cmath>
#include <iostream>

#include <list>

#include "printedpaper.hh"
#include "asr_ptp.hh"
#include "asr_ptr.hh"



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
  
  void OnKeyDown(wxKeyEvent& event);
  void OnKeyUp(wxKeyEvent& event);
  void OnChar(wxKeyEvent& event);

private:
  static const int CHARACTER_TIMER_ID = 0;

  wxBoxSizer   top_sizer;
  wxBoxSizer   tape_sizer;
  PrintedPaper printer;
  AsrPtp       asr_ptp;
  AsrPtr       asr_ptr;
  
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
  : wxFrame((wxFrame *)NULL, -1, title, pos, size)
  , top_sizer(wxHORIZONTAL)
  , tape_sizer(wxVERTICAL)
  , printer(this)
  , asr_ptp(this)
  , asr_ptr(this)

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

    top_sizer.Add(&tape_sizer,
                  wxSizerFlags(0).Border().Expand());
    top_sizer.Add(&printer,
                  wxSizerFlags(1).Expand());
    printer.SetMinSize(wxSize(500,100));

    tape_sizer.Add(&asr_ptp,
                   wxSizerFlags(1).Align(wxALIGN_TOP));
    tape_sizer.AddSpacer(10);
    tape_sizer.Add(&asr_ptr,
                  wxSizerFlags(1).Align(wxALIGN_BOTTOM));

    SetSizerAndFit(&top_sizer);
       
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
