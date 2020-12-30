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
#include <wx/config.h>

#include "serialport.h"
#include "asr_comms_prefs.hh"

#include "teletype_logo_128.xpm"

#define ROOT_CONFIG "teletypeApp"

class MyApp: public wxApp
{
  virtual bool OnInit();
};

class MyFrame: public wxFrame, public SimplePortPrefs
{
public:
  
  MyFrame(const wxString &title, const wxPoint &pos = wxDefaultPosition,
          const wxSize &size = wxDefaultSize);
  virtual ~MyFrame();
  
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnClose(wxCloseEvent &event);
  void OnSerial(wxSerialPortEvent &event);
  
  void OnKeyDown(wxKeyEvent& event);
  void OnKeyUp(wxKeyEvent& event);
  void OnChar(wxKeyEvent& event);

  virtual void Process(unsigned char ch, int source=0);
  virtual wxPreferencesPage *Preferences();

private:
  static const int CHARACTER_TIMER_ID = 0;

  /*
  enum class Sources {
    Keyboard,
    SpecialFunctionKey,
    TapeReader,
    AnswerBack,
    Line
  };
  */
  
  //unsigned int ppi;
  wxBoxSizer *top_sizer;
  AsrWidget  *asr_widget;

  //unsigned int GetPPI();

  wxSerialPort *port;
  wxSerialPort::Config config;
  //wxSerialPort::SignalEventSet *ses;
  wxSerialPort::AsyncInput *asi;
  
  DECLARE_EVENT_TABLE()
};

enum
  {
    ID_Quit = 1,
    ID_About,
    ID_Read,
    ID_Serial
  };

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(ID_Quit, MyFrame::OnQuit)
EVT_MENU(ID_About, MyFrame::OnAbout)
EVT_CLOSE(MyFrame::OnClose)
EVT_SERIAL_PORT_WAIT(ID_Serial, MyFrame::OnSerial)
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

  std::cout << __PRETTY_FUNCTION__ << std::endl;
  
  if ( event.CanVeto() && asr_widget->Unsaved(message) ) {
    if ( wxMessageBox(message,
                      "Please confirm",
                      wxICON_QUESTION | wxYES_NO) != wxYES ) {
      event.Veto();
      return;
    }
  }
  
  /*
  if (ses) {
    delete ses;
    ses = 0;
  }
  */
  if (asi) {
    asi->Stop();
    delete asi;
    asi = 0;
  }
  if (port) {
    delete port;
    port = 0;
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
  , asr_widget(new AsrWidget(this, this))
  , port(0)
    //, ses(0)
  , asi(0)
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

#if 1
  //wxSerialPortArray portArray;
  wxSerialPort::portArray_t portArray;
  wxSerialPort::Return r = wxSerialPort::ListPorts(portArray);

  std::cout << "ListPorts() returned " << static_cast<int>(r) << std::endl;

  unsigned int i;
  //for (i=0; i<portArray.GetCount(); i++) {
  for (i=0; i<portArray.size(); i++) {
    std::cout << portArray[i].GetPortName() << std::endl;
  }

  if (config.Allocated() != wxSerialPort::OK)
    std::cerr << "config not allocated" << std::endl;

  port = new wxSerialPort(portArray[2]);
  if (port->Allocated() != wxSerialPort::OK)
    std::cerr << "port not allocated" << std::endl;

  r = port->Open(wxSerialPort::MODE_READ_WRITE);
  std::cout << "Open() returned " << static_cast<int>(r) << std::endl;

  r = port->GetConfig(config);
  std::cout << "GetConfig() returned " << static_cast<int>(r) << std::endl;

  if (r == wxSerialPort::OK) r = config.SetBaudrate(9600);
  if (r == wxSerialPort::OK) r = config.SetBits(8);
  if (r == wxSerialPort::OK) r = config.SetParity(wxSerialPort::PARITY_NONE);
  if (r == wxSerialPort::OK) r = config.SetStopbits(1);
  if (r == wxSerialPort::OK) r = config.SetFlowcontrol(wxSerialPort::FLOWCONTROL_NONE);

  std::cout << "config setting calls returned " << static_cast<int>(r) << std::endl;

  r = port->SetConfig(config);
  std::cout << "SetConfig() returned " << static_cast<int>(r) << std::endl;

  asi = new wxSerialPort::AsyncInput(this, ID_Serial, *port);
  asi->Start();
  
  /*
  r = wxSerialPort::SignalEventSet::New(&ses, this, ID_Serial);
  std::cout << "SignalEventSet::New() returned " << static_cast<int>(r) << std::endl;

  r = ses->AddPortEvent(*port, wxSerialPort::EVENT_RX_READY);
  std::cout << "AddPortEvent() returned " << static_cast<int>(r) << std::endl;

  r = ses->Signal();
  std::cout << "Signal() returned " << static_cast<int>(r) << std::endl;
  */
  
#endif

  (void) wxConfigBase::Set(new wxConfig(ROOT_CONFIG));

  wxConfigBase *conf = wxConfigBase::Get();

  if (conf->HasEntry("fred")) {
    wxString str;
    (void) conf->Read("fred", &str, "harry");
    std::cout << "fred = " << str << std::endl;
  } else {
    (void) conf->Write("fred", "jim");
    std::cout << "Initialized fred" << std::endl;
  }
}

MyFrame::~MyFrame()
{
  /*
  if (ses) {
    delete ses;
    ses = 0;
  }
  */
  if (asi) {
    asi->Stop();
    delete asi;
    asi = 0;
  }

  if (port) {
    delete port;
    port = 0;
  }
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
  uint8_t c = ch & 0x7f;
  port->NonblockingWrite(&c, 1);
}

void MyFrame::OnSerial(wxSerialPortEvent &event)
{
  /*
  uint8_t c;
  wxSerialPort::Return r;
  */
  
  while (! asi->empty()) {
    uint8_t c = asi->read();
    asr_widget->Process(c);
  }

  /*
  r = port->NonblockingRead(&c, 1);

  if (r == 1) {
    asr_widget->Process(c);
  }
  
  if (r >= 0) {
    // Wait for the next character
    //r = ses->Signal();
  }
  */

  // Is there anyting to do with an error return?
}

wxPreferencesPage *MyFrame::Preferences()
{
  return new AsrCommsPrefs;
}

