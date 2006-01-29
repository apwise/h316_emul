#include <wx/wx.h> 
#include <iostream>

#include "gui_types.hpp"
#include "fp.hpp"
#include "h16cmd.hpp"
#include "gui.hpp"

using namespace std;


struct GUI::CpuMenuItem GUI::cpu_items[] =
  {
    {"DDP-116", CPU_116},
    {"H316",    CPU_316},
    {"DDP-416", CPU_416},
    {"DDP-516", CPU_516},
    {"H716",    CPU_716},
    {0,CPU_NONE}
  };

GUI::GUI(const wxString& title,
         const wxPoint& pos, const wxSize& size)
  : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
  CPU default_cpu = CPU_516;

  wxMenu *menuFile = new wxMenu;
  menuCPU  = new wxMenu;
  int i;

  menuFile->Append( ID_About, "&About..." );
  menuFile->AppendSeparator();
  menuFile->Append( ID_Quit, "E&xit" );
  
  i=0;
  while (cpu_items[i].menu_string)
    {
      menuCPU->AppendCheckItem( ID_CPU_BASE+i, cpu_items[i].menu_string );

      if (cpu_items[i].cpu_type == default_cpu)
	menuCPU->Check(ID_CPU_BASE+i, true);
      else
	menuCPU->Check(ID_CPU_BASE+i, false);
      i++;
    };

  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append( menuFile, "&File" );
  menuBar->Append( menuCPU, "&CPU" );
  
  SetMenuBar( menuBar );
   
  CreateStatusBar();
  SetStatusText( "Welcome to wxWindows!" );
    
  wxBoxSizer *ts = new wxBoxSizer( wxHORIZONTAL );
  
  fp = new FrontPanel(this, default_cpu);

  ts->Add(fp, 1, wxGROW);

  SetSizer(ts);
  ts->SetSizeHints(this);
  
  Refresh();


}

BEGIN_EVENT_TABLE(GUI, wxFrame)
  EVT_MENU(ID_Quit, GUI::OnQuit)
  EVT_MENU(ID_About, GUI::OnAbout)
  EVT_MENU_RANGE(ID_CPU_BASE, ID_CPU_MAX, GUI::OnCPU)
  EVT_IDLE(GUI::OnIdle)
END_EVENT_TABLE()

void GUI::OnQuit(wxCommandEvent& WXUNUSED(event))
{
  Close(TRUE);
}

void GUI::OnAbout(wxCommandEvent& WXUNUSED(event))
{
  wxMessageBox("This is a wxWindows Hello world sample",
               "About Hello World", wxOK | wxICON_INFORMATION, this);
}

void GUI::OnCPU(wxCommandEvent& event)
{
  int id = event.GetId();
  
  //cout << "GUI::OnCPU() id = " << id << endl;

  int i;
  CPU new_cpu_type;

  i=0;
  while (cpu_items[i].menu_string)
    {
      if ((ID_CPU_BASE+i) == id)
	{
	  new_cpu_type = cpu_items[i].cpu_type;
	  menuCPU->Check(id, true);
	}
      else
	menuCPU->Check(ID_CPU_BASE+i, false);
      i++;
    };

  if (new_cpu_type != cpu_type)
    {
      cpu_type = new_cpu_type;

      fp->Destroy();
      fp = new FrontPanel(this, cpu_type);
    }
}

void GUI::OnIdle(wxIdleEvent& event)
{
  cout << "OnIdle()" << endl;
  
  if (!command_list.empty())
    {
      H16Cmd &cmd = command_list.front();

      cout << "OnIdle() got a command" << endl;
      
      switch (cmd.get_type())
	{
	case H16Cmd::GO:
	  cout << "OnIdle() got a GO" << endl;
	  break;

	default:
	  cerr << "OnIdle(): Unrecognized command" << endl;
	  
	}

      command_list.pop_front(); // Delete command just dealt with
	      
      event.RequestMore(); // Ask for OnIdle() to be called again
    }
}

void GUI::QueueCommand(H16Cmd &cmd)
{
  command_list.push_back(cmd);
}

class H16App: public wxApp
{
public:
  virtual bool OnInit();


};

IMPLEMENT_APP(H16App)

bool H16App::OnInit()
{
  GUI *gui = new GUI( "Hello World", wxDefaultPosition, wxSize(10,10) );
  gui->Show(TRUE);
  SetTopWindow(gui);
  return TRUE;
} 
