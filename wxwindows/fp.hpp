/* Graphical representation of series-16 frontpanel
 *
 * Copyright (c) 2006, 2019  Adrian Wise
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

