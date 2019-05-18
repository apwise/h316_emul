#ifndef __ASR_PTR_HH__
#define __ASR_PTR_HH__

#include <wx/wx.h> 
#include "papertape.hh"

class AsrPtr: public wxPanel
{
public:
  AsrPtr( wxWindow *parent,
          wxWindowID id = wxID_ANY,
          const wxPoint& pos = wxDefaultPosition,
          const wxSize& size = wxDefaultSize,
          long style = wxTAB_TRAVERSAL,
          const wxString& name = wxT("AsrPtr") );
  ~AsrPtr( );
private:
  PaperTape   papertape;
  wxBoxSizer  top_sizer;
};

#endif // __ASR_PTR_HH__
