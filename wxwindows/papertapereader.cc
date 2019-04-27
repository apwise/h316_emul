#include <wx/wx.h> 
#include <wx/ffile.h>
#include <wx/statline.h>

#include <list> 
#include <iostream>


#include "papertape.hh"
#include "papertapereader.hh"

enum PTR_IDs
  {
    PTR_ID_LOAD = 10000,
    PTR_ID_REWIND
  };

BEGIN_EVENT_TABLE(PaperTapeReader, wxPanel)
  EVT_BUTTON(PTR_ID_LOAD, PaperTapeReader::OnLoad)
  EVT_BUTTON(PTR_ID_REWIND, PaperTapeReader::OnRewind)
END_EVENT_TABLE()

PaperTapeReader::PaperTapeReader(wxWindow* parent)
  : wxPanel(parent)  
{
  wxBoxSizer *v_sizer = new wxBoxSizer( wxVERTICAL );
  wxBoxSizer *button_sizer = new wxBoxSizer( wxHORIZONTAL );

  button_sizer->Add(
                    new wxButton( this, PTR_ID_LOAD, wxT("Load") ),
     0,           // make horizontally unstretchable
     wxALL,       // make border all around (implicit top alignment)
     10 );        // set border width to 10

  button_sizer->Add(
                    new wxButton( this, PTR_ID_REWIND, wxT("Rewind") ),
                    0,           // make horizontally unstretchable
                    wxALL,       // make border all around (implicit top alignment)
                    10 );        // set border width to 10

  v_sizer->Add(
     button_sizer,
     0,                // make vertically unstretchable
     wxALIGN_LEFT ); // no border and align to the left

//   v_sizer->Add(
//             new wxStaticLine(this, -1,
//                              wxDefaultPosition, wxSize(-1,100), wxLI_HORIZONTAL),
//             0,
//             wxALIGN_LEFT );

//     wxStaticLine( wxWindow *parent, wxWindowID id,
//             const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, 
//             long style = wxLI_HORIZONTAL, const wxString &name = wxStaticTextNameStr );

  papertape = new wxPaperTape(this, -1,
                              wxDefaultPosition, wxDefaultSize,
                              true, wxHORIZONTAL, wxPaperTape::wxPT_LeftToRight, false);

  v_sizer->Add(
               papertape,
               1,
               wxGROW );

  SetSizer( v_sizer );      // use the sizer for layout
  v_sizer->SetSizeHints( this );   // set size hints to honour minimum size
}

PaperTapeReader::~PaperTapeReader()
{
}

int PaperTapeReader::Read()
{
  return papertape->Read();
}

void PaperTapeReader::OnLoad(wxCommandEvent& WXUNUSED(event))
{
  wxFileDialog fd(this, wxT("Select papertape file"), wxT(""), wxT(""),
                  wxT("Papertape files (*.ptp)|*.ptp|Text Files (*.txt)|*.txt"),
                  wxFD_OPEN | wxFD_CHANGE_DIR);

  if (fd.ShowModal() == wxID_OK)
    {
      wxString filename = fd.GetFilename();

      //std::cout << "Selected " << filename << std::endl;

      wxFFile file(filename.c_str(), wxT("r"));

      if (file.IsOpened())
        {
          size_t len = file.Length();
          size_t len_read;
          char *buffer = new char[len];
          len_read = file.Read(buffer, len);
          file.Close();

          papertape->Load(buffer, len_read);
        }
    }
}

void PaperTapeReader::OnRewind(wxCommandEvent& WXUNUSED(event))
{
  papertape->Rewind();
}
