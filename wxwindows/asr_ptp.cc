#include "asr_ptp.hh"

AsrPtp::AsrPtp( wxWindow      *parent,
                wxWindowID     id,
                const wxPoint &pos,
                const wxSize  &size,
                long           style,
                const wxString &name)
  : wxPanel(parent, id, pos, size, style, name)
  , papertape(this)
  , top_sizer(wxVERTICAL)
  , button_sizer(2, 10, 10)
{
  button_sizer.Add(ControlButton("REL."),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());
  button_sizer.Add(ControlButton("OFF"),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());
  button_sizer.Add(ControlButton("BSP."),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());
  button_sizer.Add(ControlButton("ON"),
                   wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL).Expand());

  top_sizer.Add(&button_sizer,
                wxSizerFlags(0).Expand());
  top_sizer.Add(&papertape,
                wxSizerFlags(1).Expand());

  papertape.SetMinSize(wxSize(100,100));

  SetSizerAndFit(&top_sizer);
}

wxBoxSizer *AsrPtp::ControlButton(std::string label)
{
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

  sizer->AddStretchSpacer();
  sizer->Add(new wxButton(this, wxID_ANY, wxT(""),
                          wxDefaultPosition, wxSize(BUTTON_SIZE, BUTTON_SIZE)),
             wxSizerFlags(0).Align(wxALIGN_CENTER).FixedMinSize());
  sizer->Add(new wxStaticText(this, -1, label),
             wxSizerFlags(0).Align(wxALIGN_CENTER).FixedMinSize());
  sizer->AddStretchSpacer();
  
  return sizer;
}

AsrPtp::~AsrPtp()
{
}

