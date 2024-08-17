/////////////////////////////////////////////////////////////////////////////
// Name:        serialport.h
// Purpose:     Wrapper for libserialport
// Author:      Adrian Wise <adrian@adrianwise.co.uk>
// Created:     2019-07-27
// Copyright:   (c) 2019, 2020, 2024 Adrian Wise
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#ifndef _SERIALPORT_H_
#define _SERIALPORT_H_

#include <wx/wx.h>
#include <wx/dynarray.h>

#include <deque>
#include <queue>

#include <cstdint>

// TODO: Replace Allocated() by IsOK() - more wxWidgets-like?

class wxSerialPort;
//WX_DECLARE_OBJARRAY(wxSerialPort, wxSerialPortArray);

class wxSerialPortEvent : public wxEvent
{
public:
  wxSerialPortEvent(wxEventType eventType, int id) : wxEvent(id, eventType) {}
  wxSerialPortEvent(const wxSerialPortEvent &e) : wxEvent(e) {}

  // implement the base class pure virtual
  virtual wxEvent *Clone() const { return new wxSerialPortEvent(*this); }

private:
  // No specific data
};

wxDECLARE_EVENT(wxSERIAL_PORT_SIGNAL, wxSerialPortEvent);

typedef void (wxEvtHandler::*wxSerialPortEventFunction)(wxSerialPortEvent &);
#define wxSerialPortEventHandler(func) wxEVENT_HANDLER_CAST(wxSerialPortEventFunction, func)

#define EVT_SERIAL_PORT_WAIT(id, func) \
    wx__DECLARE_EVT1(wxSERIAL_PORT_SIGNAL, id, wxSerialPortEventHandler(func))

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
    static Return New(Config **config);
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

    static Return New(EventSet **eventSet);
    EventSet(Return &r);
    EventSet();
    virtual ~EventSet();
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

  /*
  class SignalEventSet : public wxThreadHelper, public EventSet
  {
  public:
    static Return New(SignalEventSet **signalEventSet, wxEvtHandler *handler, wxWindowID id);
    SignalEventSet(Return &r, wxEvtHandler *handler, wxWindowID id);
    SignalEventSet(wxEvtHandler *handler, wxWindowID id);
    virtual ~SignalEventSet();

    wxWindowID GetId() const { return m_id; }

    static const int TIMEOUT_FOREVER = 0;
    Return Signal(unsigned int timeout_ms = TIMEOUT_FOREVER);

  private:
    // Not default constructible
    SignalEventSet();
    // Copy construction not allowed
    SignalEventSet(const SignalEventSet &signalEventSet);
    // Assignment not allowed
    SignalEventSet & operator=(const SignalEventSet &signalEventSet);

    wxEvtHandler *m_handler;
    wxWindowIDRef m_id;
    unsigned int m_timeout_ms;

    wxCriticalSection m_signalCS;

    virtual wxThread::ExitCode Entry();
  };
  */
  
  class AsyncIO
  {
  public:
    static const int DEFAULT_TIMEOUT = 337;
    
    enum StartAction
      {
       START_CLEAR_QUEUE,
       START_KEEP_QUEUED
      };
    
    enum StopAction
      {
       STOP_WAIT,
       STOP_REQUEST_ONLY
      };
    
    AsyncIO(EventSet     &eventSet,
            unsigned int  timeout_ms = DEFAULT_TIMEOUT);
    virtual ~AsyncIO();

    virtual wxThreadError Start(StartAction startAction = START_CLEAR_QUEUE);
    virtual wxThreadError Stop(StopAction stopAction = STOP_WAIT);

  protected:
    enum OnEventStatus
      {
       ON_EVENT_EXIT,
       ON_EVENT_CONTINUE
      };
     
  private:
    class AsyncIOThread : public wxThread
    {
    public:
      AsyncIOThread(AsyncIO &asyncIO, EventSet &eventSet, unsigned int timeout_ms);
      ~AsyncIOThread();
      
    private:
      AsyncIO      &m_AsyncIO;
      EventSet     &m_EventSet;
      unsigned int  m_timeout_ms;

      virtual ExitCode Entry();
    };
    
    EventSet            &m_EventSet;
    unsigned int const   m_timeout_ms;
    AsyncIOThread       *m_pAsyncIOThread;
    wxCriticalSection    m_pAsyncIOThreadCS;

    virtual OnEventStatus OnEvent() = 0;
    void OnExit();
  };

  class AsyncInput : public AsyncIO
  {
  public:
    AsyncInput(wxEvtHandler  *handler,
               wxWindowID    id,
               wxSerialPort &port,
               unsigned int  timeout_ms = DEFAULT_TIMEOUT);
    virtual ~AsyncInput();

    virtual wxThreadError Start(StartAction startAction = START_CLEAR_QUEUE);

    const uint8_t &front() const;
    void           pop();
    bool           empty() const;
    size_t         size()  const;
    uint8_t        read(bool &ok);
    uint8_t        read();
            
  private:
    wxEvtHandler * const       m_pHandler;
    wxWindowID const           m_id;

    wxSerialPort              &m_Port;
    EventSet                   m_EventSet;
    
    mutable wxCriticalSection  m_SendEventQueueCS;
    std::queue<uint8_t>        m_Queue;
    bool                       m_SendEvent;
    Return                     m_Error;
    
    virtual OnEventStatus OnEvent();
  };
  
  // Port enumeration
  //static Return ListPorts(wxSerialPortArray &portArray);
  typedef std::deque<wxSerialPort> portArray_t;
  static Return ListPorts(portArray_t &portArray);
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

  // Waiting - see EventSet or SignalEventSet, above

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
