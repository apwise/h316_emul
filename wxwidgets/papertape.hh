/* Graphical representation of papertape
 *
 * Copyright (c) 2004, 2007, 2019  Adrian Wise
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
#ifndef __PAPERTAPE_HH__
#define __PAPERTAPE_HH__

#include <list>
#include <vector>

#include <wx/wx.h> 
#include <wx/ffile.h> 
#include <wx/filename.h> 

class PaperTape: public wxScrolledCanvas
{
public:
  
  enum PT_direction {
    PT_TopToBottom = 0,
    PT_LeftToRight = 0,
    PT_BottomToTop = 1,
    PT_RightToLeft = 1
  };

  static const unsigned int LeaderLength = 20;
  
  PaperTape( wxWindow *parent,
             wxWindowID id = -1,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             bool reader = true,
             int orient = wxVERTICAL,
             enum PT_direction direction = PT_TopToBottom,
             bool mirror = false,
             const wxString& name = wxT("paperTape") );
  ~PaperTape( );
  
  void Load(const unsigned char *buffer, unsigned long size, unsigned int leader = LeaderLength);
  void Load(const wxFileName &filename, unsigned int leader = LeaderLength);
  void Rewind();
  bool Read(unsigned char &ch);

  void PunchLeader(unsigned int leader = LeaderLength);
  void Punch(unsigned char ch);
  bool CanBackspace();
  void Backspace();

  bool IsAttached();
  bool Attach(wxString filename);
  bool CloseAttached();
  bool Save(wxString filename);
  
  unsigned long DataSize(){return total_size(true);}
  
private:
  enum PT_type {
    PT_holes,
    PT_light,
    PT_leader,
    PT_no_feed,
    PT_no_tape,
    PT_lead_triangle,
    PT_tail_triangle,
    
    PT_num_types
  };

  class TapeChunk
  {
  public:
    TapeChunk(PT_type type, unsigned long size = 0, const unsigned char *ptr = 0);
    ~TapeChunk();
    
    PT_type get_type() {return type;}
    unsigned long get_size() {return (type == PT_holes) ? buffer.size() : size;}
    unsigned int get_element(unsigned int index, PT_type &type);
    void append(unsigned char ch);
    unsigned char modify(unsigned int offset, unsigned char ch);
    bool un_append(unsigned char &ch);
    
  private:
    const PT_type type;
    const unsigned long size;
    std::vector<unsigned char> buffer;
  };
  
  static const int num_type[PT_num_types];

  const bool reader;
  const int orient;
  const enum PT_direction direction;
  const bool mirror;
  
  int current_tape_width;
  int row_spacing;
  int paper_tape_visible_rows;
  int paper_tape_offset;
  wxBitmap **bitmaps;

  unsigned int black_start;
  unsigned int black_end;

  wxFFile *AttachedFile;
  unsigned int AttachedLeaderCount;
  unsigned int AttachedZeroCount;
  bool AttachedError;
  
  int start_type[PT_num_types + 1];
  wxTimer *timer;
  bool pending_refresh;

  unsigned long position, initial_position;

  std::list<TapeChunk> chunks;
  
  void DrawCircle(wxDC &dc, double xc, double yc,
                  double r, wxColour &foreground);
  int InsideCircle(double xc, double yc, double r,
                   double x, double y, double d);
  
  void FillLine(wxDC &dc,
                double m, double c, bool right,
                long xc[2], long yc[2],
                wxColour &foreground);
  int LeftOfLine(double m, double c,
                 double x, double y, double d);
  int PointLeftOfLine(double m, double c,
                      double x, double y);

  void set_pending_refresh();
  
  void DestroyBitmaps();
  void AllocateBitmaps();
  wxBitmap *GetBitmap(int i, PT_type c);
  
  unsigned long total_size(bool only_holes = false);
  unsigned int get_element(unsigned int index, PT_type &type);
  
  void SetScrollBars();
  void SetScrollBarPosition();

  void Write(unsigned char ch);
  void WriteAll(bool fromCurrent = false);
  void UnWrite();

  void OnSize(wxSizeEvent &event);
  void OnPaint(wxPaintEvent &event);
  void OnTimer(wxTimerEvent& event);
  void OnIdle(wxIdleEvent& event);

  DECLARE_EVENT_TABLE()
};

#endif // __PAPERTAPE_HH__
