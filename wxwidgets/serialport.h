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

class wxSerialPort : public wxObject
{
public:
  class SerialPortData;

  wxSerialPort();
  ~wxSerialPort();

  const wxArrayString &PortNames();
  const wxArrayString &PortNickNames();

private:
  SerialPortData &m_serialPortData;
  
  wxDECLARE_DYNAMIC_CLASS(wxSerialPort);  
};

#endif // _SERIALPORT_H_

// Local Variables:
// mode: c++
// End:
