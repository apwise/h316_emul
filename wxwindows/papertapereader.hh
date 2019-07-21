
class PaperTape;

class PaperTapeReader : public wxPanel
{
public:
  PaperTapeReader(wxWindow* parent);
  ~PaperTapeReader();
  bool Read(unsigned char &ch);

private:
  PaperTape *papertape;

  void OnLoad(wxCommandEvent& event);
  void OnRewind(wxCommandEvent& event);

  DECLARE_EVENT_TABLE()
};
