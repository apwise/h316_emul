
class WordButtons;
class ControlButtons;

class FrontPanel : public wxPanel
{
public:
  FrontPanel(wxWindow *parent);

  void OnClear(wxCommandEvent& event);
  void OnOnOff(wxCommandEvent& event);
  void OnMasterClear(wxCommandEvent& event);
  void OnMaSiRun(wxCommandEvent& event);
  void OnRegisterButton(wxCommandEvent& event);

  enum ID
    {
      ID_Off = 10000,
      ID_On,

      ID_Clear,
      ID_MasterClear,

      ID_X,
      ID_A,
      ID_B,
      ID_OP,
      ID_PY,
      ID_M,

      ID_MA,
      ID_SI,
      ID_RUN
    };

  enum DSBL
    {
      DSBL_OFF    = 1,
      DSBL_OP     = 2,
      DSBL_SI_RUN = 4
    };

private:
  WordButtons *wb;
  ControlButtons *cb;

  void EnableButtons();

  DECLARE_EVENT_TABLE()
};
