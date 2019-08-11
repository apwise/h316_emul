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

class wxSerialPort::SerialPortData
{
public:
  SerialPortData();
  SerialPortData(const SerialPortData &spd);
  ~SerialPortData();

  void SetPort(struct sp_port *p);
  
  // Port handling
  wxSerialPort::Return Open(wxSerialPort::Mode mode);
  wxSerialPort::Return Close();
  wxString GetPortName();
  wxString GetPortDescription();
  wxSerialPort::Transport GetPortTransport();
  wxSerialPort::Return GetPortUsbBusAddress(int &usbBus, int &usbAddress);
  wxSerialPort::Return GetPortUsbVidPid(int &usbVid, int &usbPid);
  wxString GetPortUsbManufacturer();
  wxString GetPortUsbProduct();
  wxString GetPortUsbSerial();
  wxString GetPortBluetoothAddress();
  wxSerialPort::Return GetPortHandle(void *resultPtr);

  
  static wxSerialPort::Return ReturnEnum(sp_return r);
  
private:
  struct sp_port *m_port;

  static enum sp_mode ModeEnum(wxSerialPort::Mode mode);
  static wxSerialPort::Transport TransportEnum(enum sp_transport tr);
};

wxSerialPort::SerialPortData::SerialPortData()
  : m_port(NULL)
{
}

wxSerialPort::SerialPortData::SerialPortData(const SerialPortData &spd)
  : m_port(NULL)
{
  (void) sp_copy_port(spd.m_port, &m_port);
}

wxSerialPort::SerialPortData::~SerialPortData()
{
  sp_free_port(m_port);
}

void wxSerialPort::SerialPortData::SetPort(struct sp_port *p)
{
  if (m_port) {
    sp_free_port(m_port);    
  }
  m_port = p;
}

wxSerialPort::Return wxSerialPort::SerialPortData::ReturnEnum(sp_return e)
{
  wxSerialPort::Return r = ERR_ARG;
  
  switch (e) {
  case SP_OK:       r = wxSerialPort::OK;       break;
  case SP_ERR_ARG:  r = wxSerialPort::ERR_ARG;  break;
  case SP_ERR_FAIL: r = wxSerialPort::ERR_FAIL; break;
  case SP_ERR_MEM:  r = wxSerialPort::ERR_MEM;  break;
  case SP_ERR_SUPP: r = wxSerialPort::ERR_SUPP; break;
  }

  return r;
}

enum sp_mode wxSerialPort::SerialPortData::ModeEnum(wxSerialPort::Mode mode)
{
  enum sp_mode r;

  switch (mode) {
  case READ:       r = SP_MODE_READ;       break;
  case WRITE:      r = SP_MODE_WRITE;      break;
  case READ_WRITE: r = SP_MODE_READ_WRITE; break;
  }

  return r;
}

wxSerialPort::Transport wxSerialPort::SerialPortData::TransportEnum(enum sp_transport tr)
{
  wxSerialPort::Transport r;

  switch (tr) {
  case SP_TRANSPORT_NATIVE:    r = wxSerialPort::NATIVE;    break;
  case SP_TRANSPORT_USB:       r = wxSerialPort::USB;       break;
  case SP_TRANSPORT_BLUETOOTH: r = wxSerialPort::BLUETOOTH; break;
  }

  return r;
}

wxSerialPort::Return wxSerialPort::SerialPortData::Open(wxSerialPort::Mode mode)
{
  return ReturnEnum( sp_open(m_port, ModeEnum(mode)) );
}

wxSerialPort::Return wxSerialPort::SerialPortData::Close()
{
  return ReturnEnum( sp_close(m_port) );
}

wxString wxSerialPort::SerialPortData::GetPortName()
{
  const char *p = sp_get_port_name(m_port);
  wxString str(((p)?p:""), *wxConvFileName);
  return str;
}

wxString wxSerialPort::SerialPortData::GetPortDescription()
{
  const char *p = sp_get_port_description(m_port);
  wxString str(((p)?p:""));
  return str;
}

wxSerialPort::Transport wxSerialPort::SerialPortData::GetPortTransport()
{
  return TransportEnum(sp_get_port_transport(m_port));
}

wxSerialPort::Return wxSerialPort::SerialPortData::GetPortUsbBusAddress(int &usbBus, int &usbAddress)
{
  return ReturnEnum( sp_get_port_usb_bus_address(m_port, &usbBus, &usbAddress) );
}

wxSerialPort::Return wxSerialPort::SerialPortData::GetPortUsbVidPid(int &usbVid, int &usbPid)
{
  return ReturnEnum( sp_get_port_usb_vid_pid(m_port, &usbVid, &usbPid) );
}

wxString wxSerialPort::SerialPortData::GetPortUsbManufacturer()
{
  const char *p = sp_get_port_usb_manufacturer(m_port);
  wxString str(((p)?p:""));
  return str;
}

wxString wxSerialPort::SerialPortData::GetPortUsbProduct()
{
  const char *p = sp_get_port_usb_product(m_port);
  wxString str(((p)?p:""));
  return str;
}

wxString wxSerialPort::SerialPortData::GetPortUsbSerial()
{
  const char *p = sp_get_port_usb_serial(m_port);
  wxString str(((p)?p:""));
  return str;
}

wxString wxSerialPort::SerialPortData::GetPortBluetoothAddress()
{
  const char *p = sp_get_port_bluetooth_address(m_port);
  wxString str(((p)?p:""));
  return str;
}

wxSerialPort::Return wxSerialPort::SerialPortData::GetPortHandle(void *resultPtr)
{
  return ReturnEnum( sp_get_port_handle(m_port, resultPtr) );  
}

//------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxSerialPort, wxObject);

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(wxSerialPortArray);

wxSerialPort::wxSerialPort()
  : m_serialPortData(* new SerialPortData)
{
}

wxSerialPort::wxSerialPort(const wxSerialPort &port)
  : m_serialPortData(port.m_serialPortData)
{
}

wxSerialPort::~wxSerialPort()
{
  m_serialPortData.~SerialPortData();
}

wxSerialPort::Return wxSerialPort::ListPorts(wxSerialPortArray &portArray)
{
  enum sp_return r;
  struct sp_port **ports;
  
  portArray.Clear();

  r = sp_list_ports(&ports);
  
  if ( r == SP_OK ) {
    struct sp_port **pl;
    struct sp_port *p;

    pl = ports;
    if (pl) {
      p = *pl++;
      while ((p) && ( r == SP_OK )) {
        struct sp_port *q;

        r = sp_copy_port(p, &q);
        if ( r == SP_OK ) {
          wxSerialPort *sp = new wxSerialPort;
          sp->m_serialPortData.SetPort(q);
          portArray.Add(sp);
        }
        
        p = *pl++;
      }
    }
  }

  sp_free_port_list(ports);
  
  if (r != SP_OK) {
    portArray.Clear();
  }

  return SerialPortData::ReturnEnum(r);
}

wxSerialPort::Return wxSerialPort::PortByName(const wxString &portName, wxSerialPort *port)
{
  enum sp_return r;
  struct sp_port *p;

  r = sp_get_port_by_name(portName.c_str(), &p);

  if (r == SP_OK) {
    port = new wxSerialPort;
    port->m_serialPortData.SetPort(p);
  } else {
    port = NULL;
  }
  
  return SerialPortData::ReturnEnum(r);
}

wxSerialPort::Return wxSerialPort::Open(Mode mode)
{
  return m_serialPortData.Open(mode);
}

wxSerialPort::Return wxSerialPort::Close()
{
  return m_serialPortData.Close();
}

wxString wxSerialPort::GetPortName()
{
  return m_serialPortData.GetPortName();
}

wxString wxSerialPort::GetPortDescription()
{
  return m_serialPortData.GetPortDescription();
}

wxSerialPort::Transport wxSerialPort::GetPortTransport()
{
  return m_serialPortData.GetPortTransport();
}

wxSerialPort::Return wxSerialPort::GetPortUsbBusAddress(int &usbBus, int &usbAddress)
{
  return m_serialPortData.GetPortUsbBusAddress(usbBus, usbAddress);
}

wxSerialPort::Return wxSerialPort::GetPortUsbVidPid(int &usbVid, int &usbPid)
{
  return m_serialPortData.GetPortUsbVidPid(usbVid, usbPid);
}

wxString wxSerialPort::GetPortUsbManufacturer()
{
  return m_serialPortData.GetPortUsbManufacturer();
}

wxString wxSerialPort::GetPortUsbProduct()
{
  return m_serialPortData.GetPortUsbProduct();
}

wxString wxSerialPort::GetPortUsbSerial()
{
  return m_serialPortData.GetPortUsbSerial();
}

wxString wxSerialPort::GetPortBluetoothAddress()
{
  return m_serialPortData.GetPortBluetoothAddress();
}

wxSerialPort::Return wxSerialPort::GetPortHandle(void *resultPtr)
{
  return m_serialPortData.GetPortHandle(resultPtr);
}
