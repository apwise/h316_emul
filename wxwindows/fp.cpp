/*
 * fp.cpp
 * Hx16 front-panel
 * Adrian Wise
 */

// {{{ Includes

#include <wx/wx.h> 
#include <wx/tglbtn.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <iostream>

#include "gui_types.hpp"
#include "gui.hpp"
#include "h16cmd.hpp"
#include "fp.hpp"

// }}}

// {{{ class BitButton : public wxPanel

class BitButton : public wxPanel
{
public:
  BitButton(wxWindow *parent, int num, char *label);
  bool GetValue();
  void SetValue(bool value);
  void EnableButton(bool enbl);

private:
  wxBoxSizer *sizer;
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
// {{{ class WordButtons : public wxPanel

class WordButtons : public wxPanel
{
public:
  WordButtons(wxWindow *parent);
  int WordButtons::GetValue();
  void WordButtons::SetValue(int n);
  void EnableButton(bool enbl);

private:
  wxBoxSizer *sizer;
  static char *labels[16];
  BitButton *buttons[16];
  ClearButton *clearbutton;
};

// }}}

// {{{ class OnOff : public wxBoxPanel

class OnOff : public wxPanel
{
public:
  OnOff(wxWindow *parent);
  bool GetValue();
  void SetValue(bool value);

private:
  wxBoxSizer *sizer;
  wxRadioButton *off;
  wxRadioButton *on;
};

// }}}
// {{{ class SenseButton : public wxPanel

class SenseButton : public wxPanel
{
public:
  SenseButton(wxWindow *parent, int num);
  bool GetValue();
  void SetValue(bool value);
  void OnOnOff(wxCommandEvent& event);

private:
  wxBoxSizer *sizer;
  wxRadioButton *on, *off;
  enum ID {
    ID_Off = 11000,
    ID_On
  };
  DECLARE_EVENT_TABLE()      
};

BEGIN_EVENT_TABLE(SenseButton, wxPanel)
  EVT_RADIOBUTTON(ID_On, SenseButton::OnOnOff)
  EVT_RADIOBUTTON(ID_Off, SenseButton::OnOnOff)
END_EVENT_TABLE()

// }}}
// {{{ class Sense : public wxPanel

class Sense : public wxPanel
{
public:
  Sense( wxWindow *parent );
  unsigned int GetValue();
  void SetValue(unsigned int bitfield);
  void Sense::Changed();
private:
  wxStaticBoxSizer *sizer;
  SenseButton *ss[4];
};

// }}}
// {{{ class RegButtons : public wxPanel

class RegButtons : public wxPanel
{
public:
  RegButtons( wxWindow *parent, CPU cpu_type );

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
  CPU cpu_type;
  wxStaticBoxSizer *sizer;
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
// {{{ class MasterClear : public wxPanel

class MasterClear : public wxPanel
{
public:
  MasterClear(wxWindow *parent);
  void EnableButton(bool enbl);

private:
  wxBoxSizer *sizer;
  wxStaticText *toptext;
  wxButton *button;
  wxStaticText *bottomtext;
};

// }}}
// {{{ class FetchStore : public wxPanel

class FetchStore : public wxPanel
{
public:
  FetchStore(wxWindow *parent);
  bool GetValue();
  void SetValue(bool value);

private:
  wxBoxSizer *sizer;
  enum ID {
    ID_FETCH = 10000,
    ID_STORE
  };
};

// }}}
// {{{ class P_PP1 : public wxPanel

class P_PP1 : public wxPanel
{
public:
  P_PP1(wxWindow *parent);
  bool GetValue();
  void SetValue(bool value);

private:
  wxBoxSizer *sizer;
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
// {{{ class MaSiRun : public wxPanel

class MaSiRun : public wxPanel
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
  wxBoxSizer *sizer;
  wxRadioButton *button[MSR_NUM];
  static char *label[MSR_NUM];
};

// }}}
// {{{ class Start : public wxPanel

class Start : public wxPanel
{
public:
  Start(wxWindow *parent);
  void EnableButton(bool enbl);

private:
  wxBoxSizer *sizer;
  wxStaticText *toptext;
  wxToggleButton *button;
};

// }}}
// {{{ class ControlButtons : public wxPanel

class ControlButtons : public wxPanel
{
public:
  ControlButtons(wxWindow *parent, CPU cpu_type);
  bool GetOnOff();
  void SetOnOff(bool b);
  enum RegButtons::RB GetRegisterButton();
  enum MaSiRun::MSR GetMaSiRun();

  void DisableButtons(int flags);

private:
  CPU cpu_type;
  wxBoxSizer *sizer;
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
  : wxPanel( parent )
{
  sizer = new wxBoxSizer(wxVERTICAL);

  //char str[20]; // Use C++ strings - loose printf()
  wxString str;

  //sprintf(str, "%d", num);
  str << num;

  top_label = new wxStaticText(this, -1, str);

  button = new wxToggleButton(this, -1, "",
			      wxDefaultPosition,
			      wxSize(BUTTON_SIZE, BUTTON_SIZE));

  bottom_label = new wxStaticText(this, -1, label);

  sizer->Add( top_label, 1, wxALIGN_CENTRE);
  sizer->Add( button,
	      0, wxALIGN_CENTRE);
  sizer->Add( bottom_label, 1, wxALIGN_CENTRE);

  // Kludge around wxMSW bug...
  sizer->Add( BUTTON_SIZE, BUTTON_SIZE/2, 1, wxALIGN_CENTRE);

  SetSizer(sizer);
  sizer->SetSizeHints(this);

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
  : wxPanel( parent )
{
  int i;

  sizer = new wxBoxSizer( wxHORIZONTAL );

  /*
   * Add a space at the left
   */
  sizer->Add(BUTTON_SIZE/2, 0, 0);

  for (i=15; i>=0; i--)
    {
      buttons[i] = new BitButton(this, (16-i), labels[i]);

      sizer->Add(buttons[i], 0, wxALIGN_TOP);

      /*
       * If the button is the last of a group of three
       * then add a wide space that grows when the
       * panel is stretched. Otherwise a thin
       * space that doesn't grow.
       */
      if ((i % 3) == 0)
	sizer->Add(BUTTON_SIZE, 0, 1);
      else
	sizer->Add(BUTTON_SIZE/2, 0, 0);
    }

  clearbutton = new ClearButton(this);
  sizer->Add(clearbutton, 0, wxALIGN_TOP);

  sizer->Add(BUTTON_SIZE/2, 0, 0);

  SetSizer(sizer);
  sizer->SetSizeHints(this);

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
  : wxPanel( parent )
{
  sizer = new wxBoxSizer( wxVERTICAL );

  off = new wxRadioButton(this, FrontPanel::ID_Off, "OFF",
                          wxDefaultPosition, wxDefaultSize,
                          wxRB_GROUP);
  on = new wxRadioButton(this, FrontPanel::ID_On, "ON");

  sizer->Add( off, 0, wxALIGN_LEFT );
  sizer->Add( on,  0, wxALIGN_LEFT );

  SetSizer(sizer);
  sizer->SetSizeHints(this);  
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

// {{{ SenseButton

// {{{ SenseButton::SenseButton(wxWindow *parent, int num)

SenseButton::SenseButton(wxWindow *parent, int num)
  : wxPanel( parent )
{
  sizer = new wxBoxSizer( wxVERTICAL );

  wxString s;

  wxStaticText *t;

  s << num;
  t = new wxStaticText(this, -1, s);

  off = new wxRadioButton(this, ID_Off, "",
                          wxDefaultPosition, wxDefaultSize,
                          wxRB_GROUP);
  on = new wxRadioButton(this, ID_On, "");

  sizer->Add( t,   0, wxALIGN_CENTRE );
  sizer->Add( off, 0, wxALIGN_CENTRE );
  sizer->Add( on,  0, wxALIGN_CENTRE );

  SetSizer(sizer);
  sizer->SetSizeHints(this);

}

// }}}
// {{{ void SenseButton::OnOnOff(wxCommandEvent& event)

void SenseButton::OnOnOff(wxCommandEvent& event)
{
  std::cout << "Sense button On/Off" << std::endl;
  Sense *parent = dynamic_cast<Sense *>(GetParent());
  if (parent)
    parent->Changed();
}

// }}}
// {{{ bool SenseButton::GetValue()

bool SenseButton::GetValue()
{
  return on->GetValue();
}

// }}}
// {{{ void SenseButton::SetValue(bool value)

void SenseButton::SetValue(bool value)
{
  off->SetValue(!value);
  on->SetValue(value);
}

// }}}

// }}}
// {{{ Sense

// {{{ Sense::Sense( wxWindow *parent )

Sense::Sense( wxWindow *parent )
  : wxPanel( parent )
{
  int i;

  sizer = new wxStaticBoxSizer( new wxStaticBox( this, -1, "Sense" ),
				wxHORIZONTAL );

  for (i=1; i<=4; i++)
    {
      ss[i-1] = new SenseButton(this, i);
      if (i!=1)
	sizer->Add( BUTTON_SIZE/2, 0, 1, wxALIGN_CENTRE);
      sizer->Add(ss[i-1], 0, wxALIGN_CENTRE);
    }

  SetSizer(sizer);
  sizer->SetSizeHints(this);

}

// }}}
// {{{ unsigned int Sense::GetValue()

unsigned int Sense::GetValue()
{
  int i;
  unsigned int bitfield = 0;
  for (i=0; i<4 ; i++)
    bitfield |= ((ss[i]->GetValue() ? 1 : 0) << (3-i));
  return bitfield;
}

// }}}
// {{{ void Sense::SetValue(unsigned int bitfield)

void Sense::SetValue(unsigned int bitfield)
{
  int i;
  std::cout << "SetValue(" << bitfield << ")" << std::endl;
  
  for (i=0; i<4; i++)
    ss[i]->SetValue((bitfield >> (3-i)) & 1);
}

// }}}
// {{{ void Sense::Changed()

void Sense::Changed()
{
  unsigned int bitfield = GetValue();
  std::cout << "Sense changed to " << bitfield << std::endl;
}

// }}}

// }}}
// {{{ RegButtons

// {{{ RegButtons::RegButtons( wxWindow *parent, CPU cpu_type )

RegButtons::RegButtons( wxWindow *parent, CPU cpu_type )
  : wxPanel( parent ),
    cpu_type( cpu_type )
{
  int i;
  bool first = true;
  int processor = (1 << (static_cast<int>(cpu_type)-static_cast<int>(CPU_116)));

  sizer = new wxStaticBoxSizer( new wxStaticBox( this, -1, "Register" ),
		  wxHORIZONTAL );

  wxBoxSizer *vsizer;
			
  for (i=0; i<RB_NUM; i++)
    {
      if (processor & flags[i])
        {

	  vsizer = new wxBoxSizer(wxVERTICAL);

	  vsizer->Add(new wxStaticText(this, -1,
				       labels[i],
				       wxDefaultPosition, wxDefaultSize,
				       wxALIGN_CENTRE),
		      1, wxALIGN_CENTRE);
          
          button[i] = new wxRadioButton(this, FrontPanel::ID_X+i,
					"", wxDefaultPosition, wxDefaultSize,
					(first) ? wxRB_GROUP : 0),
          vsizer->Add(button[i], 1, wxALIGN_CENTRE);

	  if (!first)
	    sizer->Add( BUTTON_SIZE/2, 0, 1, wxALIGN_CENTRE);

	  sizer->Add(vsizer, 0, wxALIGN_CENTRE);

          first = false;
        }
      else
        button[i] = 0;
    }

  SetSizer(sizer);
  sizer->SetSizeHints(this);

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
  : wxPanel( parent )
{
  sizer = new wxBoxSizer( wxVERTICAL );
  
  toptext = new wxStaticText(this, -1,
                             "MSTR",
                             wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE);
  sizer->Add(toptext, 0, wxALIGN_CENTRE);
  
  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);

  button = new wxButton(this, FrontPanel::ID_MasterClear, "",
			wxDefaultPosition,
			wxSize(BUTTON_SIZE, BUTTON_SIZE));

  sizer->Add( button, 0, wxALIGN_CENTRE );
  
  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);
  
  bottomtext = new wxStaticText(this, -1,
                                "CLEAR",
                                wxDefaultPosition, wxDefaultSize,
                                wxALIGN_CENTRE);
  sizer->Add(bottomtext, 0, wxALIGN_CENTRE);

  SetSizer(sizer);
  sizer->SetSizeHints(this);

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
  : wxPanel( parent )
{
  wxRadioButton *fetch, *store;

  sizer = new wxBoxSizer( wxVERTICAL );

  fetch = new wxRadioButton(this, ID_FETCH, "",
			    wxDefaultPosition, wxDefaultSize,
			    wxRB_GROUP);
  store = new wxRadioButton(this, ID_STORE, "");

  sizer->Add( new wxStaticText(this, -1,
			       "FETCH",
			       wxDefaultPosition, wxDefaultSize,
			       wxALIGN_CENTRE),
	      0, wxALIGN_CENTRE);

  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);

  sizer->Add( fetch, 0, wxALIGN_CENTRE );
  sizer->Add( store,  0, wxALIGN_CENTRE );

  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);

  sizer->Add( new wxStaticText(this, -1,
			       "STORE",
			       wxDefaultPosition, wxDefaultSize,
			       wxALIGN_CENTRE),
	      0, wxALIGN_CENTRE);

  SetSizer(sizer);
  sizer->SetSizeHints(this);  
}

// }}}
// {{{ P_PP1::P_PP1(wxWindow *parent)

P_PP1::P_PP1(wxWindow *parent)
  : wxPanel( parent )
{
  sizer = new wxBoxSizer( wxVERTICAL );

  label_PP1 = new wxStaticText(this, -1, "P+1",
                               wxDefaultPosition, wxDefaultSize,
                               wxALIGN_CENTRE);

  button_PP1 = new wxRadioButton(this, ID_PP1, "",
                                 wxDefaultPosition, wxDefaultSize,
                                 wxRB_GROUP);

  button_P = new wxRadioButton(this, ID_P, "");
  
  label_P = new wxStaticText(this, -1, "P",
                             wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE);

  sizer->Add( label_PP1, 0, wxALIGN_CENTRE);

  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);

  sizer->Add( button_PP1, 0, wxALIGN_CENTRE );
  sizer->Add( button_P,  0, wxALIGN_CENTRE );

  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);

  sizer->Add( label_P, 0, wxALIGN_CENTRE);

  SetSizer(sizer);
  sizer->SetSizeHints(this);  
}

// }}}
// {{{ MaSiRun

char *MaSiRun::label[MSR_NUM] =
  { "MA", "SI", "RUN" };

// {{{ MaSiRun::MaSiRun(wxWindow *parent)

MaSiRun::MaSiRun(wxWindow *parent)
  : wxPanel( parent )
{
  int i;

  sizer = new wxBoxSizer( wxVERTICAL );

  for (i=0; i<MSR_NUM; i++)
    {
      button[i] = new wxRadioButton(this, FrontPanel::ID_MA+i, label[i],
                                    wxDefaultPosition, wxDefaultSize,
                                    (i==0) ? wxRB_GROUP : 0);
      sizer->Add( button[i], 0, wxALIGN_LEFT );
    }

  SetSizer(sizer);
  sizer->SetSizeHints(this);  
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
  : wxPanel( parent )
{
  sizer = new wxBoxSizer( wxVERTICAL );

  toptext = new wxStaticText(this, -1,
			     "START",
			     wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE);
  
  sizer->Add(toptext, 0, wxALIGN_CENTRE);
  
  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);
  
  button = new wxToggleButton(this, FrontPanel::ID_Start, "",
			      wxDefaultPosition,
			      wxSize(BUTTON_SIZE*2, BUTTON_SIZE*2));
  
  sizer->Add( button, 0, wxALIGN_CENTRE );
  
  sizer->Add( 0, 0, 1, wxALIGN_CENTRE);

  SetSizer(sizer);
  sizer->SetSizeHints(this);  
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

// {{{ ControlButtons::ControlButtons(wxWindow *parent, CPU cpu_type)

ControlButtons::ControlButtons(wxWindow *parent, CPU cpu_type)
  : wxPanel( parent ),
    cpu_type( cpu_type )
{

  sizer = new wxBoxSizer( wxHORIZONTAL );
  /*
   * Add a space at the left
   */
  sizer->Add(BUTTON_SIZE/2, 0, 0);

  onoff = new OnOff(this);
  sizer->Add( onoff, 0, wxALIGN_CENTRE );
  sizer->Add(BUTTON_SIZE/2, 0, 1);
  
  if (cpu_type == CPU_416)
    sense = 0;
  else
    {
      sense = new Sense(this);
      sizer->Add ( sense, 0, wxGROW );
      sizer->Add(BUTTON_SIZE/2, 0, 1);
    };

  regbuttons = new RegButtons(this, cpu_type);
  sizer->Add( regbuttons, 0, wxGROW );
  sizer->Add(BUTTON_SIZE/2, 0, 1);
  
  masterclear = new MasterClear(this);
  sizer->Add (masterclear, 0, wxGROW );
  sizer->Add(BUTTON_SIZE/2, 0, 1);
  
  fetchstore = new FetchStore(this);
  sizer->Add (fetchstore, 0, wxGROW );
  sizer->Add(BUTTON_SIZE/2, 0, 1);

  p_pp1 = new P_PP1(this);
  sizer->Add (p_pp1, 0, wxGROW );
  sizer->Add(BUTTON_SIZE/2, 0, 1);

  ma_si_run = new MaSiRun(this);
  sizer->Add (ma_si_run, 0, wxALIGN_CENTRE );
  sizer->Add(BUTTON_SIZE/2, 0, 1);

  start = new Start(this);
  sizer->Add (start, 0, wxGROW );

  sizer->Add(BUTTON_SIZE/2, 0, 0);

  SetSizer(sizer);
  sizer->SetSizeHints(this);  
}

// }}}
// {{{ bool ControlButtons::GetOnOff()

bool ControlButtons::GetOnOff()
{
  return onoff->GetValue();
}

// }}}
// {{{ void ControlButtons::SetOnOff(bool b)

void ControlButtons::SetOnOff(bool b)
{
  onoff->SetValue(b);
}

// }}}
// {{{ void ControlButtons::DisableButtons(int flags)

void ControlButtons::DisableButtons(int flags)
{
  /*
   * In ControlButtons only the
   * MasterClear and the Start buttons
   * are disabled
   */

  masterclear->EnableButton(!( (flags & FrontPanel::DISABLE_OFF) ||
			       (flags & FrontPanel::DISABLE_SI_RUN)) );
  start->EnableButton(!(flags & FrontPanel::DISABLE_OFF));
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

// {{{ FrontPanel::FrontPanel(wxWindow *parent, enum CPU cpu_type)

FrontPanel::FrontPanel(wxWindow *parent, enum CPU cpu_type)
  : wxPanel(parent)
{
  wxBoxSizer *ts = new wxBoxSizer( wxVERTICAL );
  
  wb = new WordButtons(this);
  cb = new ControlButtons(this, cpu_type);
  
  ts->Add( wb, 2, wxGROW );
  ts->Add( new wxStaticLine( this, -1), 0, wxGROW);
  ts->Add( cb, 3, wxGROW );
  
  SetSizer(ts);
  ts->SetSizeHints(this);
  
  /*
   * Start with the machine turned on
   */
  cb->SetOnOff(true);

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

  EVT_TOGGLEBUTTON(ID_Start, FrontPanel::OnStart)

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
    flags |= DISABLE_OFF;
  
  if (cb->GetRegisterButton() == RegButtons::RB_OP)
    flags |= DISABLE_OP;
  
  if (cb->GetMaSiRun() != MaSiRun::MSR_MA)
    flags |= DISABLE_SI_RUN;

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
// {{{ void FrontPanel::OnStart(wxCommandEvent& WXUNUSED(event))

void FrontPanel::OnStart(wxCommandEvent& WXUNUSED(event))
{
  std::cout << "Start" << std::endl;

  H16Cmd *cmd = new H16Cmd(H16Cmd::GO);

  Dispatch(*cmd);
}

// }}}
// {{{ bool FrontPanel::Destroy()

bool FrontPanel::Destroy()
{
 return wxWindow::Destroy();
}

// }}}
// {{{ void FrontPanel::Dispatch(H16Cmd &cmd)

void FrontPanel::Dispatch(H16Cmd &cmd)
{
  GUI *gui = static_cast<GUI *>(GetParent());
  
  gui->QueueCommand(cmd);
}

// }}}
