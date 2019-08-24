/////////////////////////////////////////////////////////////////////////////
// Name:        serialport.h
// Purpose:     Implementation of wrapper for libserialport
// Author:      Adrian Wise <adrian@adrianwise.co.uk>
// Created:     2019-07-27
// Copyright:   (c) 2019 Adrian Wise
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#endif //WX_PRECOMP

#include <libserialport.h>
#include "serialport.h"

class wxSerialPort::SerialPort
{
public:
  SerialPort() : m_port(NULL) {}
  
  SerialPort(const SerialPort &spd) : m_port(NULL)
  {
    (void) sp_copy_port(spd.m_port, &m_port);
  }
  
  ~SerialPort() {if (m_port) sp_free_port(m_port);}

  void SetPort(struct sp_port *p)
  {
    if (m_port) {
      sp_free_port(m_port);    
    }
    m_port = p;
  }
  
  struct sp_port *GetPort(){return m_port;}
  const struct sp_port *GetPort() const {return m_port;}
  
  static wxSerialPort::Return ReturnFromSP(sp_return e)
  {
    wxSerialPort::Return r = ERR_ARG;
    int s = static_cast<int>(e);

    if (s > 0) {
      // Must be a returned number of bytes transferred
      r = static_cast<wxSerialPort::Return>(s);
    } else {
      switch (e) {
      case SP_OK:       r = wxSerialPort::OK;       break;
      case SP_ERR_ARG:  r = wxSerialPort::ERR_ARG;  break;
      case SP_ERR_FAIL: r = wxSerialPort::ERR_FAIL; break;
      case SP_ERR_MEM:  r = wxSerialPort::ERR_MEM;  break;
      case SP_ERR_SUPP: r = wxSerialPort::ERR_SUPP; break;
      }
    }
    
    return r;
  }
    
  static enum sp_mode ModeToSP(wxSerialPort::Mode m)
  {
    enum sp_mode r;
    
    switch (m) {
    case MODE_READ:       r = SP_MODE_READ;       break;
    case MODE_WRITE:      r = SP_MODE_WRITE;      break;
    case MODE_READ_WRITE: r = SP_MODE_READ_WRITE; break;
    }
    
    return r;
  }
    
  static wxSerialPort::Transport TransportFromSP(enum sp_transport tr)
  {
    wxSerialPort::Transport r;
    
    switch (tr) {
    case SP_TRANSPORT_NATIVE:    r = TRANSPORT_NATIVE;    break;
    case SP_TRANSPORT_USB:       r = TRANSPORT_USB;       break;
    case SP_TRANSPORT_BLUETOOTH: r = TRANSPORT_BLUETOOTH; break;
    }
    
    return r;
  }
  
  static enum sp_parity ParityToSP(wxSerialPort::Parity p)
  {
    enum sp_parity r;
    
    switch (p) {
    case PARITY_INVALID: r = SP_PARITY_INVALID; break;
    case PARITY_NONE:    r = SP_PARITY_NONE;    break;
    case PARITY_ODD:     r = SP_PARITY_ODD;     break;
    case PARITY_EVEN:    r = SP_PARITY_EVEN;    break;
    case PARITY_MARK:    r = SP_PARITY_MARK;    break;
    case PARITY_SPACE:   r = SP_PARITY_SPACE;   break;
    }
    
    return r;
  }
  
  static wxSerialPort::Parity ParityFromSP(enum sp_parity p)
  {
    wxSerialPort::Parity r;
    
    switch (p) {
    case SP_PARITY_INVALID: r = PARITY_INVALID; break;
    case SP_PARITY_NONE:    r = PARITY_NONE;    break;
    case SP_PARITY_ODD:     r = PARITY_ODD;     break;
    case SP_PARITY_EVEN:    r = PARITY_EVEN;    break;
    case SP_PARITY_MARK:    r = PARITY_MARK;    break;
    case SP_PARITY_SPACE:   r = PARITY_SPACE;   break;
    }
    
    return r;
  }

  static enum sp_rts RtsToSP(wxSerialPort::Rts e)
  {
    enum sp_rts r;
    
    switch (e) {
    case RTS_INVALID:      r = SP_RTS_INVALID;      break;
    case RTS_OFF:          r = SP_RTS_OFF;          break;
    case RTS_ON:           r = SP_RTS_ON;           break;
    case RTS_FLOW_CONTROL: r = SP_RTS_FLOW_CONTROL; break;
    }
    
    return r;
  }
  
  static wxSerialPort::Rts RtsFromSP(enum sp_rts e)
  {
    wxSerialPort::Rts r;
    
    switch (e) {
    case SP_RTS_INVALID:      r = RTS_INVALID;      break;
    case SP_RTS_OFF:          r = RTS_OFF;          break;
    case SP_RTS_ON:           r = RTS_ON;           break;
    case SP_RTS_FLOW_CONTROL: r = RTS_FLOW_CONTROL; break;
    }
    
    return r;
  }
  
  static enum sp_cts CtsToSP(wxSerialPort::Cts e)
  {
    enum sp_cts r;
    
    switch (e) {
    case CTS_INVALID:      r = SP_CTS_INVALID;      break;
    case CTS_IGNORE:       r = SP_CTS_IGNORE;       break;
    case CTS_FLOW_CONTROL: r = SP_CTS_FLOW_CONTROL; break;
    }
    
    return r;
  }
  
  static wxSerialPort::Cts CtsFromSP(enum sp_cts e)
  {
    wxSerialPort::Cts r;
    
    switch (e) {
    case SP_CTS_INVALID:      r = CTS_INVALID;      break;
    case SP_CTS_IGNORE:       r = CTS_IGNORE;       break;
    case SP_CTS_FLOW_CONTROL: r = CTS_FLOW_CONTROL; break;
    }
    
    return r;
  }
  
  static enum sp_dtr DtrToSP(wxSerialPort::Dtr e)
  {
    enum sp_dtr r;
    
    switch (e) {
    case DTR_INVALID:      r = SP_DTR_INVALID;      break;
    case DTR_OFF:          r = SP_DTR_OFF;          break;
    case DTR_ON:           r = SP_DTR_ON;           break;
    case DTR_FLOW_CONTROL: r = SP_DTR_FLOW_CONTROL; break;
    }
    
    return r;
  }
  
  static wxSerialPort::Dtr DtrFromSP(enum sp_dtr e)
  {
    wxSerialPort::Dtr r;
    
    switch (e) {
    case SP_DTR_INVALID:      r = DTR_INVALID;      break;
    case SP_DTR_OFF:          r = DTR_OFF;          break;
    case SP_DTR_ON:           r = DTR_ON;           break;
    case SP_DTR_FLOW_CONTROL: r = DTR_FLOW_CONTROL; break;
    }
    
    return r;
  }
  
  static enum sp_dsr DsrToSP(wxSerialPort::Dsr e)
  {
    enum sp_dsr r;
    
    switch (e) {
    case DSR_INVALID:      r = SP_DSR_INVALID;      break;
    case DSR_IGNORE:       r = SP_DSR_IGNORE;       break;
    case DSR_FLOW_CONTROL: r = SP_DSR_FLOW_CONTROL; break;
    }
    
    return r;
  }
  
  static wxSerialPort::Dsr DsrFromSP(enum sp_dsr e)
  {
    wxSerialPort::Dsr r;
    
    switch (e) {
    case SP_DSR_INVALID:      r = DSR_INVALID;      break;
    case SP_DSR_IGNORE:       r = DSR_IGNORE;       break;
    case SP_DSR_FLOW_CONTROL: r = DSR_FLOW_CONTROL; break;
    }
    
    return r;
  }

  static enum sp_xonxoff XonXoffToSP(wxSerialPort::XonXoff e)
  {
    enum sp_xonxoff r;
    
    switch (e) {
    case XONXOFF_INVALID:  r = SP_XONXOFF_INVALID;  break;
    case XONXOFF_DISABLED: r = SP_XONXOFF_DISABLED; break;
    case XONXOFF_IN:       r = SP_XONXOFF_IN;       break;
    case XONXOFF_OUT:      r = SP_XONXOFF_OUT;      break;
    case XONXOFF_INOUT:    r = SP_XONXOFF_INOUT;    break;
    }
    
    return r;
  }

  static wxSerialPort::XonXoff XonXoffFromSP(enum sp_xonxoff e)
  {
    wxSerialPort::XonXoff r;
    
    switch (e) {
    case SP_XONXOFF_INVALID:  r = XONXOFF_INVALID;  break;
    case SP_XONXOFF_DISABLED: r = XONXOFF_DISABLED; break;
    case SP_XONXOFF_IN:       r = XONXOFF_IN;       break;
    case SP_XONXOFF_OUT:      r = XONXOFF_OUT;      break;
    case SP_XONXOFF_INOUT:    r = XONXOFF_INOUT;    break;
    }
    
    return r;
  }

  static enum sp_flowcontrol FlowcontrolToSP(wxSerialPort::Flowcontrol e)
  {
    enum sp_flowcontrol r;
    
    switch (e) {
    case FLOWCONTROL_NONE:    r = SP_FLOWCONTROL_NONE;  break;
    case FLOWCONTROL_XONXOFF: r = SP_FLOWCONTROL_XONXOFF;  break;
    case FLOWCONTROL_RTSCTS:  r = SP_FLOWCONTROL_RTSCTS;  break;
    case FLOWCONTROL_DTRDSR:  r = SP_FLOWCONTROL_DTRDSR;  break;
    }
    
    return r;
  }
  
  static wxSerialPort::Signal SignalFromSP(enum sp_signal e)
  {
    wxSerialPort::Signal r = static_cast<wxSerialPort::Signal>(0);

    if (e & SP_SIG_CTS) r = static_cast<wxSerialPort::Signal>(r | SIG_CTS);
    if (e & SP_SIG_DSR) r = static_cast<wxSerialPort::Signal>(r | SIG_DSR);
    if (e & SP_SIG_DCD) r = static_cast<wxSerialPort::Signal>(r | SIG_DCD);
    if (e & SP_SIG_RI ) r = static_cast<wxSerialPort::Signal>(r | SIG_RI);
    
    return r;
  }
    
  static enum sp_buffer BufferToSP(wxSerialPort::Buffer e)
  {
    enum sp_buffer r;
    
    switch (e) {
    case BUF_INPUT:  r = SP_BUF_INPUT;  break;
    case BUF_OUTPUT: r = SP_BUF_OUTPUT; break;
    case BUF_BOTH:   r = SP_BUF_BOTH;   break;
    }
    
    return r;
  }
  
  static enum sp_event EventToSP(wxSerialPort::Event e)
  {
    enum sp_event r = static_cast<enum sp_event>(0);
    
    if (e & EVENT_RX_READY) r = static_cast<enum sp_event>(r | SP_EVENT_RX_READY);
    if (e & EVENT_TX_READY) r = static_cast<enum sp_event>(r | SP_EVENT_TX_READY);
    if (e & EVENT_ERROR)    r = static_cast<enum sp_event>(r | SP_EVENT_ERROR);
    
    return r;
  }

private:
  struct sp_port *m_port;
};

class wxSerialPort::Config::ConfigData
{
public:
  ConfigData(sp_port_config *cf = NULL) : m_config(cf) {}
  ~ConfigData() {if (m_config) sp_free_config(m_config);}

  void SetConfig(sp_port_config *cf)
  {
    if (m_config) sp_free_config(m_config);
    m_config = cf;
  }
  
  sp_port_config *GetConfig() {return m_config;}
  const sp_port_config *GetConfig() const {return m_config;}
  
private:
  struct sp_port_config *m_config;
};

class wxSerialPort::EventSet::EventSetData
{
public:
  EventSetData(sp_event_set *es = NULL) : m_event_set(es) {}
  ~EventSetData() {if (m_event_set) sp_free_event_set(m_event_set);}
  
  void SetEventSet(sp_event_set *es)
  {
    if (m_event_set) sp_free_event_set(m_event_set);
    m_event_set = es;
  }
  
  sp_event_set *GetEventSet() {return m_event_set;}
  
private:
  struct sp_event_set *m_event_set;
};

//------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxSerialPort, wxObject);

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(wxSerialPortArray);

wxSerialPort::wxSerialPort()
  : m_serialPort(* new SerialPort)
{
}

wxSerialPort::wxSerialPort(const wxSerialPort &port)
  : m_serialPort(port.m_serialPort)
{
}

wxSerialPort::wxSerialPort(const wxString &portName, Return &r)
  : m_serialPort(* new SerialPort)
{
  enum sp_return spr;
  struct sp_port *p;

  spr = sp_get_port_by_name(portName.c_str(), &p);

  if (spr == SP_OK) {
    m_serialPort.SetPort(p);
  }

  r = SerialPort::ReturnFromSP(spr);
}

wxSerialPort::wxSerialPort(const wxString &portName)
  : m_serialPort(* new SerialPort)
{
  enum sp_return spr;
  struct sp_port *p;

  spr = sp_get_port_by_name(portName.c_str(), &p);

  if (spr == SP_OK) {
    m_serialPort.SetPort(p);
  }
}

wxSerialPort::~wxSerialPort()
{
  m_serialPort.~SerialPort();
}

wxSerialPort::Return wxSerialPort::Allocated()
{
  return (m_serialPort.GetPort()) ? OK : ERR_ARG;
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
          sp->m_serialPort.SetPort(q);
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

  return SerialPort::ReturnFromSP(r);
}

wxSerialPort::Return wxSerialPort::PortByName(const wxString &portName, wxSerialPort **port)
{
  Return r;

  if (!port) return ERR_ARG;

  *port = new wxSerialPort(portName, r);
  
  return r;
}

#define RFSP(name, sp_name)                                             \
  wxSerialPort::Return wxSerialPort::name()                             \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_serialPort.GetPort()));   \
  }

#define RFSPC(name, sp_name)                                            \
  wxSerialPort::Return wxSerialPort::name() const                       \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_serialPort.GetPort()));   \
  }

#define RFSP1(name, sp_name, arg1, sp_arg1)                             \
  wxSerialPort::Return wxSerialPort::name(arg1)                         \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_serialPort.GetPort(),     \
                                            sp_arg1));                  \
  }

#define RFSP1C(name, sp_name, arg1, sp_arg1)                            \
  wxSerialPort::Return wxSerialPort::name(arg1) const                   \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_serialPort.GetPort(),     \
                                            sp_arg1));                  \
  }

#define RFSP2(name, sp_name, arg1, sp_arg1, arg2, sp_arg2)              \
  wxSerialPort::Return wxSerialPort::name(arg1, arg2)                   \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_serialPort.GetPort(),     \
                                            sp_arg1, sp_arg2));         \
  }

#define RFSP2C(name, sp_name, arg1, sp_arg1, arg2, sp_arg2)             \
  wxSerialPort::Return wxSerialPort::name(arg1, arg2) const             \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_serialPort.GetPort(),     \
                                            sp_arg1, sp_arg2));         \
  }

#define WXSC(name, sp_name)                             \
  wxString wxSerialPort::name() const                   \
  {                                                     \
    const char *p = sp_name(m_serialPort.GetPort());    \
    wxString str(((p)?p:""));                           \
    return str;                                         \
  }

#define RFSP3(name, sp_name, t1, a1, t2, a2, t3, a3)                    \
  wxSerialPort::Return wxSerialPort::name(t1 a1, t2 a2, t3 a3)          \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_serialPort.GetPort(),     \
                                            a1, a2, a3));               \
  }

RFSP1(  Open,                    sp_open,
        Mode mode,               SerialPort::ModeToSP(mode))
RFSP(   Close,                   sp_close);
WXSC(   GetPortDescription,      sp_get_port_description)
RFSP2C( GetPortUsbBusAddress,    sp_get_port_usb_bus_address,
        int &usbBus,             &usbBus,
        int &usbAddress,         &usbAddress)
RFSP2C( GetPortUsbVidPid,        sp_get_port_usb_vid_pid,
        int &usbVid,             &usbVid,
        int &usbPid,             &usbPid)
WXSC(   GetPortUsbManufacturer,  sp_get_port_usb_manufacturer)
WXSC(   GetPortUsbProduct,       sp_get_port_usb_product)
WXSC(   GetPortUsbSerial,        sp_get_port_usb_serial)
WXSC(   GetPortBluetoothAddress, sp_get_port_bluetooth_address)
RFSP1C( GetPortHandle,           sp_get_port_handle,
        void *resultPtr,         resultPtr)

wxString wxSerialPort::GetPortName() const
{
  // Doesn't use WXS macro because of non-default string conversion
  
  const char *p = sp_get_port_name(m_serialPort.GetPort());
  wxString str(((p)?p:""), *wxConvFileName);
  return str;
}

wxSerialPort::Transport wxSerialPort::GetPortTransport() const
{
  // Not worth a macro - only function with this pattern
  return SerialPort::TransportFromSP(sp_get_port_transport(m_serialPort.GetPort()));
}

RFSP1C( GetConfig,               sp_get_config,
        Config &config,          config.GetConfigData().GetConfig() )
RFSP1(  SetConfig,               sp_set_config,
        const Config &config,    config.GetConfigData().GetConfig() )
RFSP1(  SetBaudrate,             sp_set_baudrate,
        int baudrate,            baudrate )
RFSP1(  SetBits,                 sp_set_bits,
        int bits,                bits )
RFSP1(  SetParity,               sp_set_bits,
        Parity parity,           SerialPort::ParityToSP(parity) )
RFSP1(  SetStopbits,             sp_set_stopbits,
        int stopbits,            stopbits )
RFSP1(  SetRts,                  sp_set_rts,
        Rts rts,                 SerialPort::RtsToSP(rts) )
RFSP1(  SetCts,                  sp_set_cts,
        Cts cts,                 SerialPort::CtsToSP(cts) )
RFSP1(  SetDtr,                  sp_set_dtr,
        Dtr dtr,                 SerialPort::DtrToSP(dtr) )
RFSP1(  SetDsr,                  sp_set_dsr,
        Dsr dsr,                 SerialPort::DsrToSP(dsr) )
RFSP1(  SetXonXoff,              sp_set_xon_xoff,
        XonXoff xonxoff,         SerialPort::XonXoffToSP(xonxoff) )
RFSP1(  SetFlowcontrol,          sp_set_flowcontrol,
        Flowcontrol flowcontrol, SerialPort::FlowcontrolToSP(flowcontrol) )

wxSerialPort::Return wxSerialPort::GetSignals(Signal &signal_mask) const
{
  enum sp_signal e;
  enum sp_return r = sp_get_signals(m_serialPort.GetPort(), &e);
  signal_mask = SerialPort::SignalFromSP(e);
  return SerialPort::ReturnFromSP(r);
}

RFSP(   StartBreak,              sp_start_break);
RFSP(   EndBreak,                sp_end_break);

RFSP3(  BlockingRead,            sp_blocking_read,
        void *, buf, size_t, count, unsigned int, timeout_ms)
RFSP3(  BlockingReadNext,        sp_blocking_read_next,
        void *, buf, size_t, count, unsigned int, timeout_ms)
RFSP2(  NonblockingRead,         sp_nonblocking_read,
        void *buf, buf, size_t count, count)
RFSP3(  BlockingWrite,           sp_blocking_write,
        void *, buf, size_t, count, unsigned int, timeout_ms)
RFSP2(  NonblockingWrite,        sp_nonblocking_write,
        void *buf, buf, size_t count, count)
RFSPC(  InputWaiting,            sp_input_waiting)
RFSPC(  OutputWaiting,           sp_output_waiting)
RFSP1(  Flush,                   sp_flush,
        Buffer buffers,          SerialPort::BufferToSP(buffers) )
RFSP(   Drain,                   sp_drain)

int wxSerialPort::LastErrorCode()
{
  return sp_last_error_code();
}

wxString wxSerialPort::LastErrorMessage()
{
  char *p = sp_last_error_message();
  wxString str(((p)?p:""));
  if (p) sp_free_error_message(p);
  return str;
}

void wxSerialPort::SetDebugHandler(void(*handler)(const char *format,...))
{
  sp_set_debug_handler(handler);
}

void wxSerialPort::SetDefaultDebugHandler()
{
  sp_set_debug_handler(sp_default_debug_handler);
}

int wxSerialPort::GetMajorPackageVersion()
{
  return sp_get_major_package_version();
}

int wxSerialPort::GetMinorPackageVersion()
{
  return sp_get_minor_package_version();
}

int wxSerialPort::GetMicroPackageVersion()
{
  return sp_get_micro_package_version();
}

wxString wxSerialPort::GetPackageVersionString()
{
  return wxString(sp_get_package_version_string());
}

int wxSerialPort::GetCurrentLibVersion()
{
  return sp_get_current_lib_version();
}

int wxSerialPort::GetRevisionLibVersion()
{
  return sp_get_revision_lib_version();
}

int wxSerialPort::GetAgeLibVersion()
{
  return sp_get_age_lib_version();
}

wxString wxSerialPort::GetLibVersionString()
{
  return wxString(sp_get_lib_version_string());
}

//------------------------------------------------------------------

wxSerialPort::Return wxSerialPort::Config::New(Config **config)
{
  Return r;

  if (!config) return ERR_ARG;
  
  *config = new Config(r);

  return r;
}

wxSerialPort::Config::Config(Return &r)
  : m_configData(* new ConfigData)
{
  enum sp_return spr;
  struct sp_port_config *pc;
  
  spr = sp_new_config(&pc);

  if (spr == SP_OK) {
    m_configData.SetConfig(pc);
  }
  r = SerialPort::ReturnFromSP(spr);
}

wxSerialPort::Config::Config()
  : m_configData(* new ConfigData)
{
}

wxSerialPort::Config::~Config()
{
  m_configData.~ConfigData();
}

wxSerialPort::Return wxSerialPort::Config::Allocated()
{
  return (m_configData.GetConfig()) ? OK : ERR_ARG;
}
 
#define RFSPCF1(name, sp_name, arg1, sp_arg1)                           \
  wxSerialPort::Return wxSerialPort::Config::name(arg1)                 \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_configData.GetConfig(),   \
                                            sp_arg1));                  \
  }

#define RFSPCF1C(name, sp_name, arg1, sp_arg1)                          \
  wxSerialPort::Return wxSerialPort::Config::name(arg1) const           \
  {                                                                     \
    return SerialPort::ReturnFromSP(sp_name(m_configData.GetConfig(),   \
                                            sp_arg1));                  \
  }

#define RFSPCF1E(name, sp_name, en, sp_en, arg1, sp_arg1)               \
  wxSerialPort::Return wxSerialPort::Config::name(arg1) const           \
  {                                                                     \
    enum sp_en e;                                                       \
    enum sp_return r = sp_name(m_configData.GetConfig(), &e);           \
    sp_arg1 = SerialPort::en ## FromSP(e);                              \
    return SerialPort::ReturnFromSP(r);                                 \
  }
 
RFSPCF1C( GetBaudrate,           sp_get_config_baudrate,
          int &baudrate,         &baudrate )
RFSPCF1(  SetBaudrate,           sp_set_config_baudrate,
          int baudrate,          baudrate )
RFSPCF1C( GetBits,               sp_get_config_bits,
          int &bits,             &bits )
RFSPCF1(  SetBits,               sp_set_config_bits,
          int bits,              bits )
RFSPCF1E( GetParity,             sp_get_config_parity,
          Parity,                sp_parity,
          Parity &parity,        parity )
RFSPCF1(  SetParity,             sp_set_config_parity,
          Parity parity,         SerialPort::ParityToSP(parity) )
RFSPCF1C( GetStopbits,           sp_get_config_stopbits,
          int &stopbits,         &stopbits )
RFSPCF1(  SetStopbits,           sp_set_config_stopbits,
          int stopbits,          stopbits )
RFSPCF1E( GetRts,                sp_get_config_rts,
          Rts,                   sp_rts,
          Rts &rts,              rts )
RFSPCF1(  SetRts,                sp_set_config_rts,
          Rts rts,               SerialPort::RtsToSP(rts) )
RFSPCF1E( GetCts,                sp_get_config_cts,
          Cts,                   sp_cts,
          Cts &cts,              cts )
RFSPCF1(  SetCts,                sp_set_config_cts,
          Cts cts,               SerialPort::CtsToSP(cts) )
RFSPCF1E( GetDtr,                sp_get_config_dtr,
          Dtr,                   sp_dtr,
          Dtr &dtr,              dtr )
RFSPCF1(  SetDtr,                sp_set_config_dtr,
          Dtr dtr,               SerialPort::DtrToSP(dtr) )
RFSPCF1E( GetDsr,                sp_get_config_dsr,
          Dsr,                   sp_dsr,
          Dsr &dsr,              dsr )
RFSPCF1(  SetDsr,                sp_set_config_dsr,
          Dsr dsr,               SerialPort::DsrToSP(dsr) )
RFSPCF1E( GetXonXoff,            sp_get_config_xon_xoff,
          XonXoff,               sp_xonxoff,
          XonXoff &xonxoff,      xonxoff )
RFSPCF1(  SetXonXoff,            sp_set_config_xon_xoff,
          XonXoff xonxoff,       SerialPort::XonXoffToSP(xonxoff) )
RFSPCF1(  SetFlowcontrol,        sp_set_config_flowcontrol,
          Flowcontrol fc,        SerialPort::FlowcontrolToSP(fc) )

//------------------------------------------------------------------

wxSerialPort::Return wxSerialPort::EventSet::New(EventSet **eventSet)
{
  Return r;

  if (!eventSet) return ERR_ARG;
  
  *eventSet = new EventSet(r);

  return r;
}

wxSerialPort::EventSet::EventSet(Return &r)
  : m_eventSetData(* new EventSetData)
{
  enum sp_return spr;
  struct sp_event_set *es;
  
  spr = sp_new_event_set(&es);

  if (spr == SP_OK) {
    m_eventSetData.SetEventSet(es);
  }
  r = SerialPort::ReturnFromSP(spr);
}

wxSerialPort::EventSet::EventSet()
  : m_eventSetData(* new EventSetData)
{
}

wxSerialPort::EventSet::~EventSet()
{
  m_eventSetData.~EventSetData();
}

wxSerialPort::Return wxSerialPort::EventSet::Allocated()
{
  return (m_eventSetData.GetEventSet()) ? OK : ERR_ARG;
}

wxSerialPort::Return wxSerialPort::EventSet::AddPortEvent(const wxSerialPort &p,
                                                          wxSerialPort::Event mask)
{
  return SerialPort::ReturnFromSP(sp_add_port_events(GetEventSetData().GetEventSet(),
                                                     p.GetSerialPort().GetPort(),
                                                     SerialPort::EventToSP(mask)));
}

wxSerialPort::Return wxSerialPort::EventSet::Wait(unsigned int timeout_ms)
{
  return SerialPort::ReturnFromSP(sp_wait(GetEventSetData().GetEventSet(),
                                          timeout_ms));
}

//------------------------------------------------------------------
wxDEFINE_EVENT(wxSERIAL_PORT_SIGNAL, wxSerialPortEvent);

wxSerialPort::Return wxSerialPort::SignalEventSet::New(SignalEventSet **signalEventSet,
                                                       wxEvtHandler *handler,
                                                       wxWindowID id)
{
  Return r;

  if (!signalEventSet) return ERR_ARG;

  *signalEventSet = new SignalEventSet(r, handler, id);

  return r;
}

wxSerialPort::SignalEventSet::SignalEventSet(Return &r, wxEvtHandler *handler, int id)
  : EventSet(r)
  , m_handler(handler)
  , m_id((id == wxID_ANY) ? wxIdManager::ReserveId() : id)
  , m_timeout_ms(0)
{
}

wxSerialPort::SignalEventSet::SignalEventSet(wxEvtHandler *handler, wxWindowID id)
  : m_handler(handler)
  , m_id((id == wxID_ANY) ? wxIdManager::ReserveId() : id)
  , m_timeout_ms(0)
{
}

wxSerialPort::SignalEventSet::~SignalEventSet()
{
  wxCriticalSectionLocker locker(m_signalCS);
  wxThread *thread = GetThread();

  if (thread) {
    thread->Kill();
  }

}

wxSerialPort::Return wxSerialPort::SignalEventSet::Signal(unsigned int timeout_ms)
{
  m_timeout_ms = timeout_ms;

  if (GetThread()) {
    return ERR_FAIL;
  } else {
    if (CreateThread(wxTHREAD_DETACHED) != wxTHREAD_NO_ERROR) {
      return ERR_FAIL;
    }

    if (GetThread()->Run() != wxTHREAD_NO_ERROR) {
      return ERR_FAIL;
    }
  }
  return OK;
}

wxThread::ExitCode wxSerialPort::SignalEventSet::Entry()
{
  enum sp_return spr = sp_wait(GetEventSetData().GetEventSet(),
                               m_timeout_ms);
  {
    wxCriticalSectionLocker locker(m_signalCS);

    if (spr == SP_OK) {
      wxQueueEvent(m_handler, new wxSerialPortEvent(wxSERIAL_PORT_SIGNAL, m_id));
    }

    return static_cast<wxThread::ExitCode>( 0 );
  }
}
