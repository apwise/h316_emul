/*
 * fp.cc
 * Hx16 front-panel
 * Adrian Wise
 */

// {{{ Includes

#include <wx/wx.h> 
#include <wx/tglbtn.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <iostream>

#include "fp.hh"

// }}}

// {{{ class BitButton : public wxBoxSizer

class BitButton : public wxBoxSizer
{
public:
  BitButton(wxWindow *parent, int num, char *label);
  bool GetValue();
  void SetValue(bool value);
  void EnableButton(bool enbl);

private:
  wxStaticText *top_label, *bottom_label;
  wxToggleButton *button;
};

#define BUTTON_SIZE 20

// }}}
// {{{ class ClearButton : public wxButton

class ClearButton : public wxBoxSizer
{
public:
  ClearButton(wxWindow *parent);
  void EnableButton(bool enbl);

private:
  wxStaticText *top_label;
  wxButton *button;
};

// }}}
// {{{ class WordButtons : public wxBoxSizer

class WordButtons : public wxBoxSizer
{
public:
  WordButtons(wxWindow *parent);
  int WordButtons::GetValue();
  void WordButtons::SetValue(int n);
  void EnableButton(bool enbl);

private:
  static char *labels[16];
  BitButton *buttons[16];
  ClearButton *clearbutton;
};

// }}}

// {{{ class OnOff : public wxBoxSizer

class OnOff : public wxBoxSizer
{
public:
  OnOff(wxWindow *parent);
  bool GetValue();
  void SetValue(bool value);

private:
  wxRadioButton *off;
  wxRadioButton *on;
};

// }}}
// {{{ class SenseButton : public wxBoxSizer

class SenseButton : public wxBoxSizer
{
public:
  SenseButton(wxWindow *parent, int num);
  bool GetValue();
  void SetValue(bool value);

private:
  enum ID {
    ID_OFF = 10000,
    ID_ON
  };
      
};

// }}}
// {{{ class Sense : public wxStaticBoxSizer

class Sense : public wxStaticBoxSizer
{
public:
  Sense( wxWindow *parent );
private:
  SenseButton *ss[4];
};

// }}}
// {{{ class RegButtons : public wxStaticBoxSizer

class RegButtons : public wxStaticBoxSizer
{
public:
  RegButtons( wxWindow *parent );

  // {{{ enum RB

  enum RB
    {
      RB_X,
      RB_A,
      RB_B,
      RB_OP,
      RB_PY,
      RB_M,
      RB_NUM
    };

  // }}}

  enum RB GetValue();

private:
  static char flags[RB_NUM];
  // {{{ enum FLAGS

  enum FLAGS
    {
      FLAG_116 = 1,
      FLAG_316 = 2,
      FLAG_416 = 4,
      FLAG_516 = 8,
      FLAG_716 = 16
    };

  // }}}
  static char *labels[RB_NUM];
  wxRadioButton *button[RB_NUM];
};

// }}}
// {{{ class MasterClear : public wxBoxSizer

class MasterClear : public wxBoxSizer
{
public:
  MasterClear(wxWindow *parent);
  void EnableButton(bool enbl);

private:
  wxStaticText *toptext;
  wxButton *button;
  wxStaticText *bottomtext;
};

// }}}
// {{{ class FetchStore : public wxBoxSizer

class FetchStore : public wxBoxSizer
{
public:
  FetchStore(wxWindow *parent);
  bool GetValue();
  void SetValue(bool value);

private:
  enum ID {
    ID_FETCH = 10000,
    ID_STORE
  };
};

// }}}
// {{{ class P_PP1 : public wxBoxSizer

class P_PP1 : public wxBoxSizer
{
public:
  P_PP1(wxWindow *parent);
  bool GetValue();
  void SetValue(bool value);

private:
  enum ID {
    ID_PP1 = 10000,
    ID_P
  };

  wxStaticText *label_PP1;
  wxRadioButton *button_PP1;
  wxRadioButton *button_P;
  wxStaticText *label_P;
  
};

// }}}
// {{{ class MaSiRun : public wxBoxSizer

class MaSiRun : public wxBoxSizer
{
public:
  MaSiRun(wxWindow *parent);
  
  enum MSR {
    MSR_MA,
    MSR_SI,
    MSR_RUN,

    MSR_NUM
  };

  enum MSR GetValue();

private:
  wxRadioButton *button[MSR_NUM];
  static char *label[MSR_NUM];
};

// }}}
// {{{ class Start : public wxBoxSizer

class Start : public wxBoxSizer
{
public:
  Start(wxWindow *parent);
  void EnableButton(bool enbl);

private:
  wxStaticText *toptext;
  wxButton *button;
};

// }}}
// {{{ class ControlButtons : public wxBoxSizer

class ControlButtons : public wxBoxSizer
{
public:
  ControlButtons(wxWindow *parent);
  bool GetOnOff();
  enum RegButtons::RB GetRegisterButton();
  enum MaSiRun::MSR GetMaSiRun();

  void DisableButtons(int flags);

private:
  OnOff *onoff;
  Sense *sense;
  RegButtons *regbuttons;
  MasterClear *masterclear;
  FetchStore *fetchstore;
  P_PP1 *p_pp1;
  MaSiRun *ma_si_run;
  Start *start;

};

// }}}


// {{{ BitButton

// {{{ BitButton::BitButton(wxWindow *parent, int num, char *label)

BitButton::BitButton(wxWindow *parent, int num, char *label)
  : wxBoxSizer( wxVERTICAL )
{
  char str[20]; // Use C++ strings - loose printf()

  sprintf(str, "%d", num);
  
  top_label = new wxStaticText(parent, -1, str);

  button = new wxToggleButton(parent, -1, "",
			      wxDefaultPosition,
			      wxSize(BUTTON_SIZE, BUTTON_SIZE));

  bottom_label = new wxStaticText(parent, -1, label);

  sprintf(str, "%d", num);
  
  Add( top_label, 0, wxALIGN_CENTRE);
  Add( button,
       0, wxALIGN_CENTRE);
  Add( bottom_label, 0, wxALIGN_CENTRE);

  SetValue(0);
}

// }}}

// {{{ bool BitButton::GetValue()

bool BitButton::GetValue()
{
  return button->GetValue();
}

// }}}
// {{{ void BitButton::SetValue(bool value)

void BitButton::SetValue(bool value)
{
  button->SetValue(value);
}  

// }}}
// {{{ void BitButton::EnableButton(bool enbl)

void BitButton::EnableButton(bool enbl)
{
  top_label->Enable(enbl);
  button->Enable(enbl);
  bottom_label->Enable(enbl);
}

// }}}

// }}}
// {{{ ClearButton

// {{{ ClearButton::ClearButton(wxWindow *parent)

ClearButton::ClearButton(wxWindow *parent)
  : wxBoxSizer( wxVERTICAL )
{
  top_label = new wxStaticText(parent, -1, "CLEAR");

  button = new wxButton(parent, FrontPanel::ID_Clear, "",
                        wxDefaultPosition,
                        wxSize(BUTTON_SIZE, BUTTON_SIZE));

  Add( top_label, 0, wxALIGN_CENTRE);
  Add( button, 0, wxALIGN_CENTRE);

}

// }}}

// {{{ ClearButton::EnableButton(bool enbl)

void ClearButton::EnableButton(bool enbl)
{
  top_label->Enable(enbl);
  button->Enable(enbl);
}

// }}}

// }}}
// {{{ WordButtons

// {{{ char *WordButtons::labels[16] =

char *WordButtons::labels[16] =
  {
    "P",  "MP", 0,      "DP", "EA", "ML",
    0,    "PI", "C",    "A",  "I",  "F",
    "T4", "T3", "T2",   "T1"
  };

// }}}
// {{{ WordButtons::WordButtons(wxWindow *parent)

WordButtons::WordButtons(wxWindow *parent)
  : wxBoxSizer( wxHORIZONTAL )
{
  int i;

  /*
   * Add a space at the left
   */
  Add(BUTTON_SIZE/2, 0, 0);

  for (i=15; i>=0; i--)
    {
      buttons[i] = new BitButton(parent, (16-i), labels[i]);

      Add(buttons[i], 0, wxALIGN_TOP);

      /*
       * If the button is the last of a group of three
       * then add a wide space that grows when the
       * panel is stretched. Otherwise a thin
       * space that doesn't grow.
       */
      if ((i % 3) == 0)
	Add(BUTTON_SIZE, 0, 1);
      else
	Add(BUTTON_SIZE/2, 0, 0);
    }

  clearbutton = new ClearButton(parent);
  Add(clearbutton, 0, wxALIGN_TOP);

  Add(BUTTON_SIZE/2, 0, 0);
}

// }}}
// {{{ int WordButtons::GetValue()

int WordButtons::GetValue()
{
  int n = 0;
  int i;
  for (i=15; i>=0; i--)
    n = (n << 1) | buttons[i]->GetValue();

  return n;
}

// }}}
// {{{ void WordButtons::SetValue(int n)

void WordButtons::SetValue(int n)
{
  int i;
  for (i=15; i>=0; i--)
    buttons[i]->SetValue((n >> i) & 1);
}

// }}}
// {{{ void WordButtons::EnableButton(bool enbl)

void WordButtons::EnableButton(bool enbl)
{
  int i;

  for (i=0; i<16; i++)
    buttons[i]->EnableButton(enbl);
  clearbutton->EnableButton(enbl);
}

// }}}

// }}}
// {{{ OnOff

// {{{ OnOff::OnOff(wxWindow *parent)

OnOff::OnOff(wxWindow *parent)
  : wxBoxSizer( wxVERTICAL )
{
  off = new wxRadioButton(parent, FrontPanel::ID_Off, "OFF",
                          wxDefaultPosition, wxDefaultSize,
                          wxRB_GROUP);
  on = new wxRadioButton(parent, FrontPanel::ID_On, "ON");

  Add( off, 0, wxALIGN_LEFT );
  Add( on,  0, wxALIGN_LEFT );
}

// }}}
// {{{ bool OnOff::GetValue()

bool OnOff::GetValue()
{
  return on->GetValue();
}

// }}}
// {{{ void OnOff::SetValue(bool value)

void OnOff::SetValue(bool value)
{
  on->SetValue(value);
}

// }}}

// }}}
// {{{ SenseButton::SenseButton(wxWindow *parent, int num)

SenseButton::SenseButton(wxWindow *parent, int num)
  : wxBoxSizer( wxVERTICAL )
{
  wxString s;

  wxRadioButton *on, *off;
  wxStaticText *t;

  s << num;
  t = new wxStaticText(parent, -1, s);

  off = new wxRadioButton(parent, ID_OFF, "",
                          wxDefaultPosition, wxDefaultSize,
                          wxRB_GROUP);
  on = new wxRadioButton(parent, ID_ON, "");

  Add( t,   0, wxALIGN_CENTRE );
  Add( off, 0, wxALIGN_CENTRE );
  Add( on,  0, wxALIGN_CENTRE );
}

// }}}
// {{{ Sense::Sense( wxWindow *parent )

Sense::Sense( wxWindow *parent )
  : wxStaticBoxSizer( new wxStaticBox( parent, -1, "Sense" ),
                      wxHORIZONTAL )
{
  int i;

  for (i=1; i<=4; i++)
    {
      ss[i-1] = new SenseButton(parent, i);
      Add(ss[i-1], 1, wxALIGN_CENTRE);
    }
}

// }}}
// {{{ RegButtons

// {{{ RegButtons::RegButtons( wxWindow *parent )

RegButtons::RegButtons( wxWindow *parent )
  : wxStaticBoxSizer( new wxStaticBox( parent, -1, "Register" ),
                      wxHORIZONTAL )
{
  int i;
  bool first = true;
  int processor = FLAG_316;

  wxBoxSizer *vsizer;
			
  for (i=0; i<RB_NUM; i++)
    {
      if (processor & flags[i])
        {

	  vsizer = new wxBoxSizer(wxVERTICAL);

	  vsizer->Add(new wxStaticText(parent, -1,
				       labels[i],
				       wxDefaultPosition, wxDefaultSize,
				       wxALIGN_CENTRE),
		      1, wxALIGN_CENTRE);
          
          button[i] = new wxRadioButton(parent, FrontPanel::ID_X+i,
					"", wxDefaultPosition, wxDefaultSize,
					(first) ? wxRB_GROUP : 0),
          vsizer->Add(button[i], 1, wxALIGN_CENTRE);

	  Add(vsizer, 1, wxALIGN_CENTRE);

          first = false;
        }
      else
        button[i] = 0;
    }
}

// }}}
// {{{ char RegButtons::flags[RB_NUM] =

char RegButtons::flags[RB_NUM] =
  {
    FLAG_116 |            FLAG_416 | FLAG_516 | FLAG_716, // X
    FLAG_116 | FLAG_316 | FLAG_416 | FLAG_516 | FLAG_716, // A
    FLAG_116 | FLAG_316 |            FLAG_516 | FLAG_716, // B
    FLAG_116 | FLAG_316 | FLAG_416 | FLAG_516 | FLAG_716, // OP
    FLAG_116 | FLAG_316 | FLAG_416 | FLAG_516 | FLAG_716, // PY
    FLAG_116 | FLAG_316 | FLAG_416 | FLAG_516 | FLAG_716  // M
  };

// }}}
// {{{ char *RegButtons::labels[RB_NUM] =

char *RegButtons::labels[RB_NUM] =
  {
    "X", "A", "B", "OP", "PY", "M"
  };

// }}}
// {{{ enum RegButtons::RB RegButtons::GetValue()

enum RegButtons::RB RegButtons::GetValue()
{
  int r = static_cast<int>(RB_X);
  bool notfound = true;

  while ((notfound) && (r < RB_NUM))
    {
      if (button[r])
        {
          if (button[r]->GetValue())
            notfound = false;
          else
            r++;
        }
      else
        r++;
    }
  return static_cast<RB>(r);
}

// }}}

// }}}
// {{{ MasterClear

// {{{ MasterClear::MasterClear(wxWindow *parent)

MasterClear::MasterClear(wxWindow *parent)
  : wxBoxSizer( wxVERTICAL )
{
  toptext = new wxStaticText(parent, -1,
                             "MSTR",
                             wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE);
  Add(toptext, 0, wxALIGN_CENTRE);

  Add( 0, 0, 1, wxALIGN_CENTRE);

  button = new wxButton(parent, FrontPanel::ID_MasterClear, "",
			wxDefaultPosition,
			wxSize(BUTTON_SIZE, BUTTON_SIZE));

  Add( button, 0, wxALIGN_CENTRE );

  Add( 0, 0, 1, wxALIGN_CENTRE);

  bottomtext = new wxStaticText(parent, -1,
                                "CLEAR",
                                wxDefaultPosition, wxDefaultSize,
                                wxALIGN_CENTRE);
  Add(bottomtext, 0, wxALIGN_CENTRE);
}

// }}}
// {{{ void MasterClear::EnableButton(bool enbl)

void MasterClear::EnableButton(bool enbl)
{
  toptext->Enable(enbl);
  button->Enable(enbl);
  bottomtext->Enable(enbl);
}

// }}}

// }}}
// {{{ FetchStore::FetchStore(wxWindow *parent)

FetchStore::FetchStore(wxWindow *parent)
  : wxBoxSizer( wxVERTICAL )
{
  wxRadioButton *fetch, *store;

  fetch = new wxRadioButton(parent, ID_FETCH, "",
			    wxDefaultPosition, wxDefaultSize,
			    wxRB_GROUP);
  store = new wxRadioButton(parent, ID_STORE, "");

  Add( new wxStaticText(parent, -1,
			"FETCH",
			wxDefaultPosition, wxDefaultSize,
			wxALIGN_CENTRE),
       0, wxALIGN_CENTRE);

  Add( 0, 0, 1, wxALIGN_CENTRE);

  Add( fetch, 0, wxALIGN_CENTRE );
  Add( store,  0, wxALIGN_CENTRE );

  Add( 0, 0, 1, wxALIGN_CENTRE);

  Add( new wxStaticText(parent, -1,
			"STORE",
			wxDefaultPosition, wxDefaultSize,
			wxALIGN_CENTRE),
       0, wxALIGN_CENTRE);
}

// }}}
// {{{ P_PP1::P_PP1(wxWindow *parent)

P_PP1::P_PP1(wxWindow *parent)
  : wxBoxSizer( wxVERTICAL )
{
  label_PP1 = new wxStaticText(parent, -1, "P+1",
                               wxDefaultPosition, wxDefaultSize,
                               wxALIGN_CENTRE);

  button_PP1 = new wxRadioButton(parent, ID_PP1, "",
                                 wxDefaultPosition, wxDefaultSize,
                                 wxRB_GROUP);

  button_P = new wxRadioButton(parent, ID_P, "");
  
  label_P = new wxStaticText(parent, -1, "P",
                             wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE);

  Add( label_PP1, 0, wxALIGN_CENTRE);

  Add( 0, 0, 1, wxALIGN_CENTRE);

  Add( button_PP1, 0, wxALIGN_CENTRE );
  Add( button_P,  0, wxALIGN_CENTRE );

  Add( 0, 0, 1, wxALIGN_CENTRE);

  Add( label_P, 0, wxALIGN_CENTRE);
}

// }}}
// {{{ MaSiRun

char *MaSiRun::label[MSR_NUM] =
  { "MA", "SI", "RUN" };

// {{{ MaSiRun::MaSiRun(wxWindow *parent)

MaSiRun::MaSiRun(wxWindow *parent)
  : wxBoxSizer( wxVERTICAL )
{
  int i;

  for (i=0; i<MSR_NUM; i++)
    {
      button[i] = new wxRadioButton(parent, FrontPanel::ID_MA+i, label[i],
                                    wxDefaultPosition, wxDefaultSize,
                                    (i==0) ? wxRB_GROUP : 0);
      Add( button[i], 0, wxALIGN_LEFT );
    }
}

// }}}

enum MaSiRun::MSR MaSiRun::GetValue()
{
  int i=0;
  bool not_found = true;

  while ((i<MSR_NUM) && not_found)
    {
      if (button[i]->GetValue())
        not_found = false;
      else
        i++;
    }

  return static_cast<enum MSR>(i);
}

// }}}
// {{{ Start

// {{{ Start::Start(wxWindow *parent)

Start::Start(wxWindow *parent)
  : wxBoxSizer( wxVERTICAL )
{

  toptext = new wxStaticText(parent, -1,
			"START",
			wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE);

  Add(toptext, 0, wxALIGN_CENTRE);

  Add( 0, 0, 1, wxALIGN_CENTRE);

  button = new wxButton(parent, -1, "",
			wxDefaultPosition,
			wxSize(BUTTON_SIZE*2, BUTTON_SIZE*2));

  Add( button, 0, wxALIGN_CENTRE );

  Add( 0, 0, 1, wxALIGN_CENTRE);
}

// }}}
// {{{ Start::EnableButton(bool enbl)

void Start::EnableButton(bool enbl)
{
  toptext->Enable(enbl);
  button->Enable(enbl);
}

// }}}

// }}}

// {{{ ControlButtons

// {{{ ControlButtons::ControlButtons(wxWindow *parent)

ControlButtons::ControlButtons(wxWindow *parent)
  : wxBoxSizer( wxHORIZONTAL )
{
  /*
   * Add a space at the left
   */
  Add(BUTTON_SIZE/2, 0, 0);

  onoff = new OnOff(parent);
  Add( onoff, 0, wxALIGN_CENTRE );
  Add(BUTTON_SIZE/2, 0, 1);
  
  sense = new Sense(parent);
  Add ( sense, 0, wxGROW );
  Add(BUTTON_SIZE/2, 0, 1);
  
  regbuttons = new RegButtons(parent);
  Add( regbuttons, 0, wxGROW );
  Add(BUTTON_SIZE/2, 0, 1);
  
  masterclear = new MasterClear(parent);
  Add (masterclear, 0, wxGROW );
  Add(BUTTON_SIZE/2, 0, 1);
  
  fetchstore = new FetchStore(parent);
  Add (fetchstore, 0, wxGROW );
  Add(BUTTON_SIZE/2, 0, 1);

  p_pp1 = new P_PP1(parent);
  Add (p_pp1, 0, wxGROW );
  Add(BUTTON_SIZE/2, 0, 1);

  ma_si_run = new MaSiRun(parent);
  Add (ma_si_run, 0, wxALIGN_CENTRE );
  Add(BUTTON_SIZE/2, 0, 1);

  start = new Start(parent);
  Add (start, 0, wxGROW );

  Add(BUTTON_SIZE/2, 0, 0);
}

// }}}
// {{{ bool ControlButtons::GetOnOff()

bool ControlButtons::GetOnOff()
{
  return onoff->GetValue();
}

// }}}
// {{{ void ControlButtons::DisableButtons(int flags)

void ControlButtons::DisableButtons(int flags)
{
  bool b;

  /*
   * In ControlButtons only the
   * MasterClear and the Start buttons
   * are disabled
   */

  b = ( (flags & FrontPanel::DSBL_OFF) ||
        (flags & FrontPanel::DSBL_SI_RUN) );
  masterclear->EnableButton(!b);
  start->EnableButton(!b);
}

// }}}
// {{{ enum MaSiRun::MSR ControlButtons::GetMaSiRun()

enum MaSiRun::MSR ControlButtons::GetMaSiRun()
{
  return ma_si_run->GetValue();
}

// }}}
// {{{ enum RegButtons::RB ControlButtons::GetRegisterButton()

enum RegButtons::RB ControlButtons::GetRegisterButton()
{
  return regbuttons->GetValue();
}

// }}}

// }}}

// {{{ FrontPanel::FrontPanel(wxWindow *parent)

FrontPanel::FrontPanel(wxWindow *parent)
  : wxPanel(parent)
{
  wxBoxSizer *ts = new wxBoxSizer( wxVERTICAL );
  
  wb = new WordButtons(this);
  cb = new ControlButtons(this);
  
  ts->Add( wb, 2, wxGROW );
  ts->Add( new wxStaticLine( this, -1), 0, wxGROW);
  ts->Add( cb, 3, wxGROW );
  
  SetSizer(ts);
  ts->SetSizeHints(this);
  
  Refresh();
}

// }}}
// {{{ BEGIN_EVENT_TABLE(FrontPanel, wxPanel)

BEGIN_EVENT_TABLE(FrontPanel, wxPanel)
  EVT_RADIOBUTTON(ID_On, FrontPanel::OnOnOff)
  EVT_RADIOBUTTON(ID_Off, FrontPanel::OnOnOff)

  EVT_BUTTON(ID_Clear, FrontPanel::OnClear)
  EVT_BUTTON(ID_MasterClear, FrontPanel::OnMasterClear)

  EVT_RADIOBUTTON(ID_X, FrontPanel::OnRegisterButton)
  EVT_RADIOBUTTON(ID_A, FrontPanel::OnRegisterButton)
  EVT_RADIOBUTTON(ID_B, FrontPanel::OnRegisterButton)
  EVT_RADIOBUTTON(ID_OP, FrontPanel::OnRegisterButton)
  EVT_RADIOBUTTON(ID_PY, FrontPanel::OnRegisterButton)
  EVT_RADIOBUTTON(ID_M, FrontPanel::OnRegisterButton)

  EVT_RADIOBUTTON(ID_MA, FrontPanel::OnMaSiRun)
  EVT_RADIOBUTTON(ID_SI, FrontPanel::OnMaSiRun)
  EVT_RADIOBUTTON(ID_RUN, FrontPanel::OnMaSiRun)
END_EVENT_TABLE()

  // }}}

// {{{ void FrontPanel::EnableButtons()

void FrontPanel::EnableButtons()
{
  /*
   * Gather the information to allow the
   * appropriate buttons to be set
   * enabled or disabled
   */

  int flags = 0;

  if (!cb->GetOnOff())
    flags |= DSBL_OFF;
  
  if (cb->GetRegisterButton() == RegButtons::RB_OP)
    flags |= DSBL_OP;
  
  if (cb->GetMaSiRun() != MaSiRun::MSR_MA)
    flags |= DSBL_SI_RUN;

  cb->DisableButtons(flags);

  wb->EnableButton(flags == 0);
}

// }}}

// {{{ void FrontPanel::OnClear(wxCommandEvent& WXUNUSED(event))

void FrontPanel::OnClear(wxCommandEvent& WXUNUSED(event))
{
  std::cout << "Clear" << std::endl;
  wb->SetValue(0);
}

// }}}
// {{{ void FrontPanel::OnMasterClear(wxCommandEvent& WXUNUSED(event))

void FrontPanel::OnMasterClear(wxCommandEvent& WXUNUSED(event))
{
  std::cout << "MasterClear" << std::endl;
  wb->SetValue(0);
}

// }}}
// {{{ void FrontPanel::OnMaSiRun(wxCommandEvent& event)

void FrontPanel::OnMaSiRun(wxCommandEvent& event)
{
  int id = event.GetId();

  switch (id)
    {
    case ID_MA:
      std::cout << "MA" << std::endl;
      break;
      
    case ID_SI:
      std::cout << "SI" << std::endl;
      break;
      
    case ID_RUN:
      std::cout << "RUN" << std::endl;
      break;
      
    default:
      break;
    }

  EnableButtons();
}

// }}}
// {{{ void FrontPanel::OnOnOff(wxCommandEvent& event)

void FrontPanel::OnOnOff(wxCommandEvent& event)
{
  //int id = event.GetId();
  EnableButtons();
}

// }}}
// {{{ void FrontPanel::OnRegisterButton(wxCommandEvent& event)

void FrontPanel::OnRegisterButton(wxCommandEvent& event)
{
  //int id = event.GetId();

  std::cout << "OnRegisterButton" << std::endl;

  EnableButtons();
}

// }}}


