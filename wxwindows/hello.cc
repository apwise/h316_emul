/*
 * hworld.cpp
 * Hello world sample by Robert Roebling
 */

#include <wx/wx.h> 
#include <wx/tglbtn.h>
#include <wx/stattext.h>

#include "button.xpm"

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

    DECLARE_EVENT_TABLE()
};

enum
{
    ID_Quit = 1,
    ID_About,
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ID_Quit, MyFrame::OnQuit)
    EVT_MENU(ID_About, MyFrame::OnAbout)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame( "Hello World", wxPoint(50,50), wxSize(450,340) );
    frame->Show(TRUE);
    SetTopWindow(frame);
    return TRUE;
} 

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
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

    /* Adrian */

    wxBitmap *bitmap = new wxBitmap( button_xpm );
    wxBitmapButton *bb = new wxBitmapButton( this, -1, *bitmap );

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(bb, 0, wxALIGN_CENTRE);
    sizer->Add(new wxStaticText(this, -1, "1"), 0, wxALIGN_CENTRE);

    wxToggleButton *button = new wxToggleButton(this, 42, " ");
    sizer->Add(button, 0, wxALIGN_CENTRE);

    sizer->Add(new wxStaticText(this, -1, "T1"), 0, wxALIGN_CENTRE );

    SetSizer( sizer );      // use the sizer for layout
    sizer->SetSizeHints( this );   // set size hints to honour minimum size
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("This is a wxWindows Hello world sample",
        "About Hello World", wxOK | wxICON_INFORMATION, this);
}

