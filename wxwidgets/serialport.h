/////////////////////////////////////////////////////////////////////////////
// Name:        serialport.h
// Purpose:     Wrapper for libserialport
// Author:      Adrian Wise <adrian@adrianwise.co.uk>
// Created:     2019-07-27
// Copyright:   (c) 2019 wxWidgets development team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#ifndef _SERIALPORT_H__
#define _SERIALPORT_H_

#include <wx/wx.h>
#include <wx/dynarray.h>

class wxSerialPort;
WX_DECLARE_OBJARRAY(wxSerialPort, wxSerialPortArray);
                    
class wxSerialPort : public wxObject
{
public:
  enum Return {
    OK = 0,
    ERR_ARG,
    ERR_FAIL,
    ERR_MEM,
    ERR_SUPP
  };

  enum Mode {
    READ,
    WRITE,
    READ_WRITE
  };

  enum Transport {
    NATIVE,
    USB,
    BLUETOOTH
  };
  
  // Port enumeration
  static Return ListPorts(wxSerialPortArray &portArray);
  static Return PortByName(const wxString &portName, wxSerialPort *port);
  // Copy construction allows a port to be copied from a wxSerialPortArray
  wxSerialPort(const wxSerialPort &port);
  ~wxSerialPort();

  // Port handling
  Return Open(Mode mode);
  Return Close();
  wxString GetPortName();
  wxString GetPortDescription();
  Transport GetPortTransport();
  Return GetPortUsbBusAddress(int &usbBus, int &usbAddress);
  Return GetPortUsbVidPid(int &usbVid, int &usbPid);
  wxString GetPortUsbManufacturer();
  wxString GetPortUsbProduct();
  wxString GetPortUsbSerial();
  wxString GetPortBluetoothAddress();
  Return GetPortHandle(void *resultPtr);

private:
  class SerialPortData;

  // Default constructor only to be used in ListPorts() and PortByName()
  wxSerialPort();
  // Assignment not allowed
  wxSerialPort & operator=(const wxSerialPort &port);
  
  SerialPortData &m_serialPortData;
  
  wxDECLARE_DYNAMIC_CLASS(wxSerialPort);  
};

#endif // _SERIALPORT_H_

// Local Variables:
// mode: c++
// End:
