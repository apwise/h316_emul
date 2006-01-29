
#ifndef _FP_HPP_
#define _FP_HPP

#include "h16cmd.hpp"

class WordButtons;
class ControlButtons;

class FrontPanel : public wxPanel
{
public:
  FrontPanel(wxWindow *parent, enum CPU cpu_type);
  bool Destroy();

  void OnClear(wxCommandEvent& event);
  void OnOnOff(wxCommandEvent& event);
  void OnMasterClear(wxCommandEvent& event);
  void OnMaSiRun(wxCommandEvent& event);
  void OnRegisterButton(wxCommandEvent& event);
  void OnStart(wxCommandEvent& event);

  enum ID
    {
      ID_Off = 10000,
      ID_On,

      ID_Clear,
      ID_MasterClear,

      ID_SS1,
      ID_SS2,
      ID_SS3,
      ID_SS4,

      ID_X,
      ID_A,
      ID_B,
      ID_OP,
      ID_PY,
      ID_M,

      ID_MA,
      ID_SI,
      ID_RUN,

      ID_Start
    };

  enum DISABLE
    {
      DISABLE_OFF    = 1,
      DISABLE_OP     = 2,
      DISABLE_SI_RUN = 4
    };

private:
  WordButtons *wb;
  ControlButtons *cb;

  void EnableButtons();
  void Dispatch(H16Cmd &cmd);

  DECLARE_EVENT_TABLE()
};

#endif

