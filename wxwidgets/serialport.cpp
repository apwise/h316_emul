/////////////////////////////////////////////////////////////////////////////
// Name:        serialport.h
// Purpose:     Implementation of wrapper for libserialport
// Author:      Adrian Wise <adrian@adrianwise.co.uk>
// Created:     2019-07-27
// Copyright:   (c) 2019 wxWidgets development team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <libserialport.h>
#include "serialport.h"

wxIMPLEMENT_DYNAMIC_CLASS(wxSerialPort, wxObject);

class wxSerialPort::SerialPortData
{
public:
  SerialPortData();
  ~SerialPortData();
  const wxArrayString &PortNames();
  const wxArrayString &PortNickNames();
  
private:
  struct sp_port **m_ports;
  wxArrayString m_portNames;
  wxArrayString m_portNickNames;

  wxString NickName(const wxString &portName);
  
  void ClearPorts();
  void ListPorts();
};

wxSerialPort::SerialPortData::SerialPortData()
  : m_ports(NULL)
{
}

wxSerialPort::SerialPortData::~SerialPortData()
{
  ClearPorts();
}

void wxSerialPort::SerialPortData::ClearPorts()
{
  if (m_ports) {
    sp_free_port_list(m_ports);
    m_ports = NULL;
  }
}

void wxSerialPort::SerialPortData::ListPorts()
{
  ClearPorts();
  if ( sp_list_ports(&m_ports) != SP_OK ) {
    ClearPorts();
  }
}

const wxArrayString &wxSerialPort::SerialPortData::PortNames()
{
  struct sp_port **pl;
  struct sp_port *p;
  
  m_portNames.Clear();
  ListPorts();

  pl = m_ports;
  if (pl) {
    p = *pl++;
    while (p) {
      m_portNames.Add(sp_get_port_name(p));
      
      p = *pl++;
    }
  }
  
  return m_portNames;
}

wxString wxSerialPort::SerialPortData::NickName(const wxString &portName)
{
  const wxString devPrefix = "/dev/";
  wxString nickName;
  if (! portName.StartsWith(devPrefix, &nickName)) {
    nickName = portName;
  }
  return nickName;
}

const wxArrayString &wxSerialPort::SerialPortData::PortNickNames()
{
  m_portNickNames.Clear();
  const wxArrayString &pn = PortNames();
  unsigned int i;
  for (i=0; i<pn.GetCount(); i++) {
    m_portNickNames.Add(NickName(pn[i]));
  }
  
  return m_portNickNames;
}

wxSerialPort::wxSerialPort()
  : m_serialPortData(* new SerialPortData)
{
}

wxSerialPort::~wxSerialPort()
{
  m_serialPortData.~SerialPortData();
}

const wxArrayString &wxSerialPort::PortNames()
{
  return m_serialPortData.PortNames();
}

const wxArrayString &wxSerialPort::PortNickNames()
{
  return m_serialPortData.PortNickNames();
}
