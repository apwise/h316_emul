/* Emulate a teleype - communications preferences
 *
 * Copyright (c) 2020  Adrian Wise
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

#include "asr_comms_prefs.hh"
#include <wx/config.h>
#include <sstream>
//#include "serialport.h"
#include "teletype.hh"

#define CONFIG_COMMS "/teletype/comms"
#define CONFIG_COMMS_PORTNAME "portName"


class AsrCommsPrefsPanel : public wxPanel
{
public:
  AsrCommsPrefsPanel(wxWindow *parent,
                     wxWindowID winid = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxTAB_TRAVERSAL | wxNO_BORDER,
                     const wxString& name = wxPanelNameStr);
  ~AsrCommsPrefsPanel();
  
  void OnPort(wxCommandEvent &event);
  void OnBaud(wxCommandEvent &event);
  void OnBaudText(wxCommandEvent &event);

private:
  static const unsigned int baudRates[];

  enum
    {
     ID_Port,
     ID_Baud
    };

  TeletypeFrame *frame;
  wxSerialPort::portArray_t portArray;
  wxString baud;

  void GetSerialPorts(wxArrayString &ports, int &defaultPort);
  void SetSerialPort(int i);

  void GetBaudRates(wxArrayString &bauds, int &defaultBaud);
   
  DECLARE_EVENT_TABLE()

};

const unsigned int AsrCommsPrefsPanel::baudRates[] =
  {
   110, 300, 600, 1200, 2400, 4800, 9600,
   19200, 38400, 57600, 115200
  };

#define NUM_BAUDS (sizeof(baudRates) / sizeof(baudRates[0]))

/************************************************************************
 * Implement AsrCommsPrefs
 ***********************************************************************/

AsrCommsPrefs::AsrCommsPrefs()
  : panel(0)
{
}

AsrCommsPrefs::~AsrCommsPrefs()
{
}

wxString AsrCommsPrefs::GetName() const
{
  return "Comms";
}

#ifdef wxHAS_PREF_EDITOR_ICONS
wxBitmap AsrCommsPrefs::GetLargeIcon()
{
}
#endif

wxWindow *AsrCommsPrefs::CreateWindow (wxWindow *parent)
{
  if (panel) {
    panel->Destroy();
    panel = 0;
  }

  panel = new AsrCommsPrefsPanel(parent);
 
  return panel;
}

/************************************************************************
 * Implement the actual preferences panel
 ***********************************************************************/

BEGIN_EVENT_TABLE(AsrCommsPrefsPanel, wxPanel)
EVT_CHOICE(ID_Port, AsrCommsPrefsPanel::OnPort)
EVT_COMBOBOX(ID_Baud, AsrCommsPrefsPanel::OnBaud)
EVT_TEXT(ID_Baud, AsrCommsPrefsPanel::OnBaudText)
END_EVENT_TABLE()

AsrCommsPrefsPanel::AsrCommsPrefsPanel(wxWindow *parent,
                                       wxWindowID winid, const wxPoint& pos, const wxSize& size,
                                       long style, const wxString& name)
  : wxPanel(parent, winid, pos, size, style, name)
  , frame(0)
{
  wxWindow *p = parent;

  // Walk up the tree of windows to find the top-level frame
  while (p) {
    frame = dynamic_cast<TeletypeFrame *>(p);
    if (frame) {
      break;
    }
    p = p->GetParent();
  }

  if (!p) {
    std::cerr << "Can't find frame" << std::endl;
    exit(1);
  }

  wxBoxSizer *sizer, *hSizer;

  sizer = new wxBoxSizer(wxVERTICAL);
  
  wxSizerFlags flagsLabel(1), flagsItem(1), flagsLine(1);
  flagsLabel.Align(wxALIGN_LEFT|wxALIGN_CENTRE_VERTICAL).Border(wxRIGHT, 5);
  flagsItem.Align(wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL);
  flagsLine.Border(wxALL, 10);

  /**********************************************************************
   * Port selection
   *********************************************************************/

  wxArrayString ports;
  int defaultPort;

  hSizer = new wxBoxSizer(wxHORIZONTAL);

  GetSerialPorts(ports, defaultPort);

  hSizer->Add(new wxStaticText(this, wxID_ANY, "Port"), flagsLabel);
  wxChoice *portsChoice = new wxChoice(this, ID_Port, wxDefaultPosition, wxDefaultSize,
                                       ports, 0, wxDefaultValidator, "Port");
  if (defaultPort >= 0) {
    portsChoice->SetSelection(defaultPort);
  }
  
  hSizer->Add(portsChoice, flagsItem);
  
  sizer->Add(hSizer, flagsLine);
  
  /**********************************************************************
   * Baud rate
   *********************************************************************/

  wxArrayString bauds;
  int defaultBaud;

  hSizer = new wxBoxSizer(wxHORIZONTAL);

  GetBaudRates(bauds, defaultBaud);

  hSizer->Add(new wxStaticText(this, wxID_ANY, "Baud rate"), flagsLabel);
  wxChoice *baudsCombo = new wxComboBox(this, ID_Baud, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        bauds, 0,
                                        wxTextValidator(wxFILTER_DIGITS, &baud), "Baud rate");
  if (defaultBaud >= 0) {
    baudsCombo->SetSelection(defaultBaud);
  }
  
  hSizer->Add(baudsCombo, flagsItem);
  
  sizer->Add(hSizer, flagsLine);

  /*********************************************************************/

  SetSizerAndFit(sizer); 
}

AsrCommsPrefsPanel::~AsrCommsPrefsPanel()
{
}

void AsrCommsPrefsPanel::GetSerialPorts(wxArrayString &ports, int &defaultPort)
{
  unsigned int i;

  wxConfigBase *config = wxConfigBase::Get();
  wxString configPath = config->GetPath();
  config->SetPath(CONFIG_COMMS);

  wxString portName;
  bool haveDefault = config->Read(CONFIG_COMMS_PORTNAME, &portName);
  defaultPort = -1; // Until it's found
  
  ports.Clear();
  
  wxSerialPort::Return r = wxSerialPort::ListPorts(portArray);

  if (r == wxSerialPort::OK) {
    for (i=0; i<portArray.size(); i++) {
      wxString name = portArray[i].GetPortName();
      wxString shortName;
      
      if (! name.StartsWith("/dev/", &shortName)) {
        shortName = name;
      }

      ports.Add(shortName);

      if (haveDefault && (portName == name)) {
        std::cout << "GetSerialPorts: " << name << std::endl; 
        defaultPort = (int) i;
        std::cout << "GetSerialPorts: " << defaultPort << std::endl; 
      }
    }
  }

  config->SetPath(configPath);
}

void AsrCommsPrefsPanel::SetSerialPort(int i)
{
  wxConfigBase *config = wxConfigBase::Get();
  wxString configPath = config->GetPath();
  config->SetPath(CONFIG_COMMS);

  std::cout << "Write" << std::endl;
  
  if (config->Write(CONFIG_COMMS_PORTNAME, portArray[i].GetPortName())) {
    config->Flush();
  }
  
  config->SetPath(configPath);
}

void AsrCommsPrefsPanel::GetBaudRates(wxArrayString &bauds, int &defaultBaud)
{
  unsigned int i;
  
  bauds.Clear();

  for (i=0; i<NUM_BAUDS; i++) {
    std::ostringstream ss;
    ss << baudRates[i];

    bauds.Add(ss.str());
  }
  
  
}

void AsrCommsPrefsPanel::OnPort(wxCommandEvent &event)
{
  SetSerialPort(event.GetInt());
}

void AsrCommsPrefsPanel::OnBaud(wxCommandEvent &event)
{
  std::cout << __PRETTY_FUNCTION__ << " " << event.GetInt() << std::endl;
  std::cout << "Baud = " << baud << std::endl;
}

void AsrCommsPrefsPanel::OnBaudText(wxCommandEvent &event)
{
  std::cout << __PRETTY_FUNCTION__ << " " << event.GetString() << std::endl;
  std::cout << "Baud = " << baud << std::endl;
}

