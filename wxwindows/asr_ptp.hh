#ifndef __ASR_PTP_HH__
#define __ASR_PTP_HH__

#include <wx/wx.h> 
#include "papertape.hh"

class AsrPtp: public wxPanel
{
public:
  AsrPtp( wxWindow *parent,
          wxWindowID id = wxID_ANY,
          const wxPoint& pos = wxDefaultPosition,
          const wxSize& size = wxDefaultSize,
          long style = wxTAB_TRAVERSAL,
          const wxString& name = wxT("AsrPtp") );
  ~AsrPtp( );
private:
  PaperTape   papertape;
  wxBoxSizer  top_sizer;
  wxGridSizer button_sizer;

  enum AsrPtpButtons {
    AP_REL = wxID_HIGHEST,
    AP_OFF,
    AP_BSP,
    AP_ON
  };
  
  wxBoxSizer *ControlButton(std::string label);
  static const unsigned int BUTTON_SIZE = 20;
};

#endif // __ASR_PTP_HH__
