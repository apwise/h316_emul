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
  class SerialPort;

  enum Return {
    OK       =  0,
    ERR_ARG  = -1,
    ERR_FAIL = -2,
    ERR_MEM  = -3,
    ERR_SUPP = -4
  };

  enum Mode {
    MODE_READ = 1,
    MODE_WRITE,
    MODE_READ_WRITE
  };

  enum Transport {
    TRANSPORT_NATIVE,
    TRANSPORT_USB,
    TRANSPORT_BLUETOOTH
  };
  
  enum Parity {
    PARITY_INVALID = -1,
    PARITY_NONE,
    PARITY_ODD,
    PARITY_EVEN,
    PARITY_MARK,
    PARITY_SPACE
  };
  
  enum Rts {
    RTS_INVALID = -1,
    RTS_OFF,
    RTS_ON,
    RTS_FLOW_CONTROL
  };
  
  enum Cts {
    CTS_INVALID = -1,
    CTS_IGNORE,
    CTS_FLOW_CONTROL
  };
  
  enum Dtr {
    DTR_INVALID = -1,
    DTR_OFF,
    DTR_ON,
    DTR_FLOW_CONTROL
  };
  
  enum Dsr {
    DSR_INVALID = -1,
    DSR_IGNORE,
    DSR_FLOW_CONTROL
  };
  
  enum XonXoff {
    XONXOFF_INVALID = -1,
    XONXOFF_DISABLED,
    XONXOFF_IN,
    XONXOFF_OUT,
    XONXOFF_INOUT
  };

  enum Flowcontrol {
    FLOWCONTROL_NONE,
    FLOWCONTROL_XONXOFF,
    FLOWCONTROL_RTSCTS,
    FLOWCONTROL_DTRDSR
  };

  enum Signal {
    SIG_CTS = 1,
    SIG_DSR = 2,
    SIG_DCD = 4,
    SIG_RI  = 8
  };

  enum Buffer {
    BUF_INPUT = 1,
    BUF_OUTPUT,
    BUF_BOTH
  };

  enum Event {
    EVENT_RX_READY = 1,
    EVENT_TX_READY = 2,
    EVENT_ERROR    = 4
  };
  
  class Config
  {
  public:
    class ConfigData;
    static Return NewConfig(Config **config);
    Config(Return &r);
    Config();
    ~Config();
    Return Allocated();
    
    ConfigData &GetConfigData() {return m_configData;}
    const ConfigData &GetConfigData() const {return m_configData;}

    Return GetBaudrate(int &baudrate) const;
    Return SetBaudrate(int baudrate);
    Return GetBits(int &bits) const;
    Return SetBits(int bits);
    Return GetParity(Parity &parity) const;
    Return SetParity(Parity parity);
    Return GetStopbits(int &stopbits) const;
    Return SetStopbits(int stopbits);
    Return GetRts(Rts &rts) const;
    Return SetRts(Rts rts);
    Return GetCts(Cts &cts) const;
    Return SetCts(Cts cts);
    Return GetDtr(Dtr &dtr) const;
    Return SetDtr(Dtr dtr);
    Return GetDsr(Dsr &dsr) const;
    Return SetDsr(Dsr dsr);
    Return GetXonXoff(XonXoff &xonxoff) const;
    Return SetXonXoff(XonXoff xonxoff);
    Return SetFlowcontrol(Flowcontrol flowcontrol);
    
  private:
    // Copy construction not allowed
    Config(const Config &config);
    // Assignment not allowed
    Config & operator=(const Config &config);

    ConfigData &m_configData;
  };
  
  class EventSet
  {
  public:
    class EventSetData;
    static Return NewEventSet(EventSet **eventSet);
    EventSet(Return &r);
    EventSet();
    ~EventSet();
    Return Allocated();

    EventSetData &GetEventSetData() {return m_eventSetData;}

    Return AddPortEvent(const wxSerialPort &p, Event mask);
    Return Wait(unsigned int timeout_ms);
    
  private:
    // Copy construction not allowed
    EventSet(const EventSet &eventSet);
    // Assignment not allowed
    EventSet & operator=(const EventSet &eventSet);

    EventSetData &m_eventSetData;
  };
  
  // Port enumeration
  static Return ListPorts(wxSerialPortArray &portArray);
  static Return PortByName(const wxString &portName, wxSerialPort **port);
  
  // Copy construction allows a port to be copied from a wxSerialPortArray
  wxSerialPort(const wxSerialPort &port);
  
  // Constructor equivalent to PortByName()
  wxSerialPort(const wxString &portName, Return &r);
  wxSerialPort(const wxString &portName);
  ~wxSerialPort();

  // Use to check that wxSerialPort(portName) succeeded
  Return Allocated(); 

  const SerialPort &GetSerialPort() const {return m_serialPort;}
  
  // Port handling
  Return Open(Mode mode);
  Return Close();
  wxString GetPortName() const;
  wxString GetPortDescription() const;
  Transport GetPortTransport() const;
  Return GetPortUsbBusAddress(int &usbBus, int &usbAddress) const;
  Return GetPortUsbVidPid(int &usbVid, int &usbPid) const;
  wxString GetPortUsbManufacturer() const;
  wxString GetPortUsbProduct() const;
  wxString GetPortUsbSerial() const;
  wxString GetPortBluetoothAddress() const;
  Return GetPortHandle(void *resultPtr) const;

  // Configuration
  Return GetConfig(Config &config) const;
  Return SetConfig(const Config &config);
  Return SetBaudrate(int baudrate);
  Return SetBits(int bits);
  Return SetParity(Parity parity);
  Return SetStopbits(int stopbits);
  Return SetRts(Rts rts);
  Return SetCts(Cts cts);
  Return SetDtr(Dtr dtr);
  Return SetDsr(Dsr dsr);
  Return SetXonXoff(XonXoff xonxoff);
  Return SetFlowcontrol(Flowcontrol flowcontrol);

  // Signals
  Return GetSignals(Signal &signal_mask) const;
  Return StartBreak();
  Return EndBreak();

  // Data handling
  Return BlockingRead(void *buf, size_t count, unsigned int timeout_ms);
  Return BlockingReadNext(void *buf, size_t count, unsigned int timeout_ms);
  Return NonblockingRead(void *buf, size_t count);
  Return BlockingWrite(void *buf, size_t count, unsigned int timeout_ms);
  Return NonblockingWrite(void *buf, size_t count);
  Return InputWaiting() const;
  Return OutputWaiting() const;
  Return Flush(Buffer buffers);
  Return Drain();

  // Waiting - see EventSet, above

  // Errors
  static int LastErrorCode();
  static wxString LastErrorMessage();
  static void SetDebugHandler(void(*handler)(const char *format,...));
  static void SetDefaultDebugHandler();

  // Versions
  static int GetMajorPackageVersion();
  static int GetMinorPackageVersion();
  static int GetMicroPackageVersion();
  static wxString GetPackageVersionString();

  static int GetCurrentLibVersion();
  static int GetRevisionLibVersion();
  static int GetAgeLibVersion();
  static wxString GetLibVersionString();
  
private:

  // Default constructor only to be used in ListPorts() and PortByName()
  wxSerialPort();
  // Assignment not allowed
  wxSerialPort & operator=(const wxSerialPort &port);
  
  SerialPort &m_serialPort;
  
  wxDECLARE_DYNAMIC_CLASS(wxSerialPort);  
};

#endif // _SERIALPORT_H_

// Local Variables:
// mode: c++
// End:
