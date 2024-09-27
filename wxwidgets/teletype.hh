/* Teletype emulation application
 *
 * Copyright (c) 2019, 2020  Adrian Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307 USA
 *
 */
#ifndef _TELETYPE_HH_
#define _TELETYPE_HH_

#include <wx/wx.h>

#include "simpleportprefs.hh"
#include "serialport.h"

class AsrWidget;

class TeletypeFrame: public wxFrame, public SimplePortPrefs
{
public:
  
  TeletypeFrame(const wxString &title, const wxPoint &pos = wxDefaultPosition,
          const wxSize &size = wxDefaultSize);
  virtual ~TeletypeFrame();
  
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnClose(wxCloseEvent &event);
  void OnSerial(wxSerialPortEvent &event);
  
  void OnKeyDown(wxKeyEvent& event);
  void OnKeyUp(wxKeyEvent& event);
  void OnChar(wxKeyEvent& event);

  virtual void Process(unsigned char ch, int source=0);
  virtual wxPreferencesPage *Preferences();

private:
  static const int CHARACTER_TIMER_ID = 0;

  /*
  enum class Sources {
    Keyboard,
    SpecialFunctionKey,
    TapeReader,
    AnswerBack,
    Line
  };
  */
  
  //unsigned int ppi;
  wxBoxSizer *top_sizer;
  AsrWidget  *asr_widget;

  //unsigned int GetPPI();

  wxSerialPort *port;
  wxSerialPort::Config config;
  //wxSerialPort::SignalEventSet *ses;
  wxSerialPort::AsyncInput *asi;
  
  DECLARE_EVENT_TABLE()
};

#endif // _TELETYPE_HH_
