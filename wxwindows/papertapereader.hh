
class wxPaperTape;

class PaperTapeReader : public wxPanel
{
public:
  PaperTapeReader(wxWindow* parent);
  ~PaperTapeReader();
  int Read();

private:
  wxPaperTape *papertape;

  void OnLoad(wxCommandEvent& event);
  void OnRewind(wxCommandEvent& event);

  DECLARE_EVENT_TABLE()
};
