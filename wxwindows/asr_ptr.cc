#include "asr_ptr.hh"

AsrPtr::AsrPtr( wxWindow      *parent,
                wxWindowID     id,
                const wxPoint &pos,
                const wxSize  &size,
                long           style,
                const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , papertape(this)
  , top_sizer(wxVERTICAL)
{
  top_sizer.Add(&papertape,
                wxSizerFlags(1).Expand());

  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("START")),
                wxSizerFlags(0).Left());
  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("STOP")),
                wxSizerFlags(0).Left());
  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("AUTO")),
                wxSizerFlags(0).Left());
  top_sizer.Add(new wxRadioButton(this, wxID_ANY, wxT("REL.")),
                wxSizerFlags(0).Left());

  papertape.SetMinSize(wxSize(100,100));
  
  SetSizerAndFit(&top_sizer);
}

AsrPtr::~AsrPtr()
{
}

