#include <wx/wx.h> 
#include <iostream>

#include "fp.hh"

class GUI : public wxFrame
{
public:

  GUI(const wxString& title, const wxPoint& pos, const wxSize& size);
  
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);

  enum ID
    {
      ID_Quit = 10000,
      ID_About
    };

private:
  FrontPanel *fp;

  DECLARE_EVENT_TABLE()
};

GUI::GUI(const wxString& title,
         const wxPoint& pos, const wxSize& size)
  : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
  wxMenu *menuFile = new wxMenu;
  
  menuFile->Append( ID_About, "&About..." );
  menuFile->AppendSeparator();
  menuFile->Append( ID_Quit, "E&xit" );
  
  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append( menuFile, "&File" );
  
  SetMenuBar( menuBar );
  
  CreateStatusBar();
  SetStatusText( "Welcome to wxWindows!" );
    
  wxBoxSizer *ts = new wxBoxSizer( wxHORIZONTAL );
  
  fp = new FrontPanel(this);

  ts->Add(fp, 1, wxGROW);

  SetSizer(ts);
  ts->SetSizeHints(this);
  
  Refresh();
}

BEGIN_EVENT_TABLE(GUI, wxFrame)
  EVT_MENU(ID_Quit, GUI::OnQuit)
  EVT_MENU(ID_About, GUI::OnAbout)
END_EVENT_TABLE()

void GUI::OnQuit(wxCommandEvent& WXUNUSED(event))
{
  Close(TRUE);
}

void GUI::OnAbout(wxCommandEvent& WXUNUSED(event))
{
  wxMessageBox("This is a wxWindows Hello world sample",
               "About Hello World", wxOK | wxICON_INFORMATION, this);
}

class MyApp: public wxApp
{
    virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
  GUI *gui = new GUI( "Hello World", wxDefaultPosition, wxSize(10,10) );
  gui->Show(TRUE);
  SetTopWindow(gui);
  return TRUE;
} 
