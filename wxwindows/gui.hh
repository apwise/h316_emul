#ifndef _GUI_HH_
#define _GUI_HH_

#include <list>

#include "h16cmd.hh"

class FrontPanel;

class GUI : public wxFrame
{
public:

  GUI(const wxString& title, const wxPoint& pos, const wxSize& size);

  void QueueCommand(H16Cmd &cmd);

  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnCPU(wxCommandEvent& event);
  void OnIdle(wxIdleEvent& event);

  enum ID
    {
      ID_Quit = 10000,
      ID_About,
      ID_CPU_BASE,
      ID_CPU_MAX=ID_CPU_BASE+99
    };

private:
  FrontPanel *fp;

  struct CpuMenuItem
  {
    char *menu_string;
    enum CPU cpu_type;
  };
  static struct CpuMenuItem cpu_items[];
  wxMenu *menuCPU;
  CPU cpu_type;

  std::list<H16Cmd> command_list;

  DECLARE_EVENT_TABLE()
};

#endif

