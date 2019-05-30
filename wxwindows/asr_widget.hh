/* Emulate a teleype
 *
 * Copyright (c) 2019  Adrian Wise
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
#ifndef __ASR_WIDGET_HH__
#define __ASR_WIDGET_HH__

#include <wx/wx.h> 

#include "simpleport.hh"
#include "printedpaper.hh"
#include "asr_ptp.hh"
#include "asr_ptr.hh"

class AsrWidget: public wxPanel, public SimplePort
{
public:
  enum class Sources {
    Keyboard,
    SpecialFunctionKey,
    TapeReader,
    AnswerBack,
    Line
  };

  AsrWidget( wxWindow *parent,
             wxWindowID id = wxID_ANY,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxTAB_TRAVERSAL,
             const wxString& name = wxT("AsrWidget") );
  ~AsrWidget( );

  bool Unsaved(wxString &message);
  void Process(unsigned char ch, int source);

private:
  wxBoxSizer   *top_sizer;
  wxBoxSizer   *tape_sizer;
  PrintedPaper *printer;
  AsrPtp       *asr_ptp;
  AsrPtr       *asr_ptr;
};

#endif // __ASR_WIDGET_HH__
