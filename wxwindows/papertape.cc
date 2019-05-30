/* Graphical representation of papertape
 *
 * Copyright (c) 2004, 2005, 2019  Adrian Wise
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
#include <cmath>

#include "papertape.hh"

#define TIMER_PERIOD 100 // update every tenth of a second

BEGIN_EVENT_TABLE(PaperTape, wxScrolledCanvas)
EVT_SIZE(     PaperTape::OnSize)
EVT_PAINT(    PaperTape::OnPaint)
EVT_TIMER(-1, PaperTape::OnTimer)
EVT_IDLE(     PaperTape::OnIdle)
END_EVENT_TABLE()

const int PaperTape::num_type[PT_num_types] =
{
  256,
  256,
  1,
  1,
  1,
  7,
  7
};

PaperTape::TapeChunk::TapeChunk(PT_type type, unsigned long size, const unsigned char *ptr)
  : type(type)
  , size(((type == PT_lead_triangle) ||
          (type == PT_tail_triangle)) ? num_type[type] : size)
{
  if (type == PT_holes) {
    unsigned int i;
    for (i=0; i<size; i++) {
      buffer.push_back(ptr[i]);
    }
  }
}

PaperTape::TapeChunk::~TapeChunk()
{
}

unsigned int PaperTape::TapeChunk::get_element(unsigned int index, PT_type &type)
{
  if (index < get_size()) {
    type = this->type;
    
    if ((type == PT_lead_triangle) || (type == PT_tail_triangle)) {
      return index;
    } else if (!buffer.empty()) {
      return (buffer[index]);
    } else {
      return 0;
    }
  }

  type = PT_num_types;
  return 0;
}

void PaperTape::TapeChunk::append(unsigned char ch)
{
  if (type == PT_holes) {
    buffer.push_back(ch);
  }
}

unsigned char PaperTape::TapeChunk::modify(unsigned int offset, unsigned char ch)
{
  unsigned int i = buffer.size();

  if (i >= offset) {
    i -= offset;

    ch = buffer[i] | ch;
    buffer[i] = ch;
  }

  return ch;
}

bool PaperTape::TapeChunk::un_append(unsigned char &ch)
{
  bool r = false;
  if ((type == PT_holes) && (!buffer.empty())) {
    ch = buffer.back();
    buffer.pop_back();
    r = true;
  }
  return r;
}

PaperTape::PaperTape( wxWindow *parent,
                      wxWindowID id,
                      const wxPoint &pos,
                      const wxSize &size,
                      bool reader,
                      int orient,
                      enum PT_direction direction,
                      bool mirror,
                      const wxString &name )
  : wxScrolledCanvas(parent, id, pos, size,
                     (orient == wxVERTICAL) ? wxVSCROLL : wxHSCROLL,
                     name)
  , reader(reader)
  , orient(orient)
  , direction(direction)
  , mirror(mirror)
  , current_tape_width(-1)
  , row_spacing(0)
  , paper_tape_visible_rows(0)
  , paper_tape_offset(0)
  , bitmaps(0)
  , black_start(0)
  , black_end(0)
  , AttachedFile(0)
  , AttachedLeaderCount(0)
  , AttachedZeroCount(0)
  , AttachedError(false)
  , timer(0)
  , pending_refresh(false)
  , position(0)
  , initial_position(0)
  , chunks()
{
  /*
   * No point allowing physical scrolling because we always
   * repaint the whole visible area of paper-tape anyway
   */
  EnableScrolling(false, false);

  timer = new wxTimer(this);

  int i, j;

  j = 0;
  for (i=0; i<PT_num_types; i++) {
    start_type[i] = j;
    j += num_type[i];
  }
  start_type[PT_num_types] = j;
}

PaperTape::~PaperTape( )
{
  CloseAttached();
  
  DestroyBitmaps();

  if (!chunks.empty()) {
    chunks.clear();
  }

  delete timer;
}

void PaperTape::Load(const unsigned char *buffer, unsigned long size,
                     unsigned int leader)
{
  /*
   * If there are any existing chunks then they need
   * to be deleted
   */

  if (!chunks.empty()) {
    chunks.clear();
  }

  chunks.push_back(TapeChunk(PT_lead_triangle));
  if (leader > 0) chunks.push_back(TapeChunk(PT_leader, leader));
  chunks.push_back(TapeChunk(PT_holes, size, buffer));
  if (leader > 0) chunks.push_back(TapeChunk(PT_leader, leader));
  chunks.push_back(TapeChunk(PT_tail_triangle));
  
  /*
   * set the current file pointer to half way through
   * the added leader
   */

  position = initial_position = num_type[PT_lead_triangle] + leader/2;

  /*
   * Need to set the scrolling position
   */
  SetScrollBars();

  /*
   * Force a repaint
   */
  Refresh();
}

void PaperTape::Load(const wxFileName &filename, unsigned int leader)
{
  bool ok = false;

  if (filename.IsOk()) {
    wxString path(filename.GetFullPath());
    wxFFile file(path, "rb");
    
    if (file.IsOpened()) {
      size_t length = file.Length();
      unsigned char *buffer = new unsigned char[length];
      size_t num_read = file.Read(buffer, length);
      if (num_read == length) {
        Load(buffer, length, leader);
        ok = true;
      }
    }
  }
  
  if (!ok) {
    wxString message = "Error reading file <";
    message += filename.GetFullName();
    message += ">";
    wxMessageDialog dialog(this, message, "Error", wxOK | wxICON_ERROR);
    (void) dialog.ShowModal();
  }
}

void PaperTape::PunchLeader(unsigned int leader)
{
  /*
   * If there are any existing chunks then they need
   * to be deleted
   */

  if (!chunks.empty()) {
    chunks.clear();
  }

  chunks.push_back(TapeChunk(PT_lead_triangle));
  if (leader > 0) chunks.push_back(TapeChunk(PT_leader, leader));
  
  position = initial_position = num_type[PT_lead_triangle] + leader;

  SetScrollBars();

  Refresh();
}

void PaperTape::Punch(unsigned char ch)
{
  if (chunks.empty() ||
      (chunks.back().get_type() != PT_holes)) {
    chunks.push_back(TapeChunk(PT_holes, 1, &ch));
  } else {
    unsigned long ts = total_size();
    if (position >= ts) {
      chunks.back().append(ch);
    } else {
      ch = chunks.back().modify(ts-position, ch);
    }
  }
  Write(ch);
  position++;
  
  SetScrollBars();
  Refresh();
}

bool PaperTape::CanBackspace()
{
  bool r = false;

  if ((!chunks.empty()) &&
      (chunks.back().get_type() == PT_holes)) {
    
    unsigned long min_pos = total_size() - chunks.back().get_size();
    if (position > min_pos) {
      r = true;
    }
  }

  return r;
}

void PaperTape::Backspace()
{
  if (CanBackspace()) {
    position--;
    UnWrite();
  }
  SetScrollBars();
  Refresh();
}

void PaperTape::Rewind()
{
  position = initial_position;
  SetScrollBarPosition();
  Refresh();
}

int PaperTape::Read()
{
  PT_type type;
  int data = get_element(position, type);

  if ((type == PT_holes) || (type == PT_leader)) {
    position ++;
    
    /*
     * Used to update the position for every character:
     *
     * SetScrollBarPosition();
     * Refresh();
     *
     * but this is not practical at high read rates because
     * the screen never gets updated. Instead start a timer
     * and update at a regular rate that can be managed.
     */
    
    set_pending_refresh();
    
    return data;
  } else {
    return -1;
  }
}

void PaperTape::DestroyBitmaps()
{
  int i;
  
  if (bitmaps) {
    for (i=0; i<start_type[PT_num_types]; i++) {
      if (bitmaps[i]) {
        delete (bitmaps[i]);
      }
    }
    delete [] bitmaps;
    bitmaps = 0;
  }
}

bool PaperTape::IsAttached()
{
  return (AttachedFile);
}

bool PaperTape::Attach(wxString filename)
{
  bool r = false;

  AttachedFile = new wxFFile(filename, "wb");
  r = AttachedFile->IsOpened();

  if (r) {
    if (AttachedFile->GetKind() != wxFILE_KIND_DISK) {
      /* Have to be able to seek (for backspace) */
      r = false;
    }
  }
  
  if (r) {
    AttachedLeaderCount = LeaderLength;
    AttachedError = false;

    WriteAll();
  } else {
    AttachedFile = 0;
    AttachedError = true;
  }
  AttachedZeroCount = 0;
  return r;
}

bool PaperTape::Save(wxString filename)
{
  bool r = Attach(filename);

  if (r) {
    r = CloseAttached();
  }
  return r;
}

void PaperTape::Write(unsigned char ch)
{
  if (AttachedFile) {
    if (AttachedLeaderCount > 0) {
      if (ch == 0) {
        /* Write nothing yet */
        AttachedLeaderCount--;
      } else {
        unsigned char buffer[LeaderLength + 1];
        unsigned int i;
        for (i = 0; i<LeaderLength; i++) {
          buffer[i] = 0;
        }
        buffer[i++] = ch;
        if (AttachedFile->Write(buffer, i) != i) {
          AttachedError = true;
        }
        AttachedLeaderCount = 0;
      }
    } else {
      if (AttachedFile->Write(&ch, 1) != 1) {
        AttachedError = true;
      }
      if (ch == 0) {
        AttachedZeroCount++;
      } else {
        AttachedZeroCount = 0;
      }
    }
  }
}

/*
 * Write all characters currently buffered to the file
 */
void PaperTape::WriteAll(bool fromCurrent)
{
  if (! fromCurrent) {
    position = initial_position;
  }
  
  while ( (position - initial_position) < DataSize() ) {
    unsigned char ch;
    PT_type type;
    
    ch = get_element(position, type);
    Write(ch);
    position++;
  }
}

void PaperTape::UnWrite()
{
  if (AttachedFile) {
    if (! AttachedFile->Seek(-1, wxFromCurrent)) {
      AttachedError = true;
    }
  }
}

bool PaperTape::CloseAttached()
{
  if (AttachedFile) {

    /*
     * If there's been backspacing near the end of the tape (file)
     * then advance through these characters once more and
     * write them back to the file. This also updates
     * AttachedZeroCount.
     */
    WriteAll(true);
   
    if (AttachedFile->Tell() != 0) {
      /* If anything has been written then add trailing "leader" */
      if (AttachedZeroCount < LeaderLength) {
        unsigned char buffer[LeaderLength];
        unsigned int i;
        for (i = 0; i<LeaderLength; i++) {
          buffer[i] = 0;
        }
        i -= AttachedZeroCount;
        if (AttachedFile->Write(buffer, i) != i) {
          AttachedError = true;
        }
      }
    }
    if (! AttachedFile->Close()) {
      AttachedError = true;
    }
    delete AttachedFile;
    AttachedFile = 0;
  }

  return !AttachedError;
}

void PaperTape::AllocateBitmaps()
{
  int i;
  
  if (bitmaps) {
    abort();
  }

  bitmaps = new wxBitmap *[start_type[PT_num_types]];
  
  for (i=0; i<start_type[PT_num_types]; i++) {
    bitmaps[i] = 0;
  }
}

void PaperTape::set_pending_refresh()
{
  pending_refresh = true;

  if (!timer->IsRunning()) {
    timer->Start(TIMER_PERIOD);
  }
}

void PaperTape::OnTimer(wxTimerEvent& WXUNUSED(event))
{
  if (pending_refresh) {
    pending_refresh = false;
    
    SetScrollBarPosition();
    Refresh();
  } else {
    timer->Stop();
  }
}

void PaperTape::OnIdle(wxIdleEvent& WXUNUSED(event))
{
  if (AttachedFile) {
    if (! AttachedFile->Flush()) {
      AttachedError = true;
    }
  }  
}

wxBitmap *PaperTape::GetBitmap(int i, PT_type type)
{
  int bm = start_type[static_cast<int>(type)] + i;
  
  if ((i < 0) ||
      (i >= num_type[static_cast<int>(type)])) {
    abort();
  }
  
  if (bitmaps[bm]) {
    return bitmaps[bm];
  }
  
  /*
   * Allocate a bitmap
   */

  double d_paper_tape_width = static_cast<double>(current_tape_width);
  double spacing = d_paper_tape_width / 10.0;
        
  if (orient == wxVERTICAL) {
    bitmaps[bm] = new wxBitmap(current_tape_width, row_spacing, -1);
  } else {
    bitmaps[bm] = new wxBitmap(row_spacing, current_tape_width, -1);
  }
  
  wxMemoryDC dc;
  dc.SelectObject(*bitmaps[bm]);

  wxColour hole_colour(0,0,0);
  wxBrush hole_brush(hole_colour, wxSOLID);

  if (type == PT_no_tape) {
    dc.SetBackground(hole_brush);
    dc.Clear();
    dc.SetBrush(wxNullBrush);
    dc.SetBackground(wxNullBrush);
    return bitmaps[bm];
  }
  
  /*
   * Now paint the paper (if there is any)
   */
  
  wxColour paper_colour(245, 244, 229); /* Yellowing papertape colour */
  wxBrush paper_brush(paper_colour, wxSOLID);
  dc.SetBackground(paper_brush);
  dc.Clear();
   
  if (type == PT_no_feed) {
    return bitmaps[bm];  
  }
  
  static int shift[9] = {7, 6, 5, -1, 4, 3, 2, 1, 0};      

  bool drawcircles = false;

  int ii = i;
  if ((type == PT_lead_triangle) ||
      (type == PT_tail_triangle)) {
    i = 0;
  }
  
  switch(i) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x04:
  case 0x08:
  case 0x10:
  case 0x20:
  case 0x40:
  case 0x80:
    drawcircles = true;
  }
  
  if (drawcircles) {   
    /*
     * These basic cases are drawn using floating-point
     * anti-aliasing routines.
     */
    
    double yy = spacing/2.0;
    double xx = spacing;
    double data_hole = d_paper_tape_width / 32.0;
    double feed_hole = d_paper_tape_width / 64.0;
      
    if ((type == PT_light) && (reader)) {
      /*
       * Make the holes bright white and
       * make the holes a little bigger to further the
       * illusion of a bright light shining through
       */
      hole_colour.Set(250, 250, 255); /* Blue-ish bright white */
      data_hole *= 5.0/4.0;
      feed_hole *= 5.0/4.0;
    }
    
    int j, k;
    bool b;
    
    for (j=0; j<9; j++) {
      k = (mirror) ? (8-j) : j;
      
      if (shift[k] < 0) {
        b = true;
      } else {
        b = (i >> shift[k]) & 1;
      }
      
      if (b) {
        DrawCircle(dc,
                   (orient == wxVERTICAL) ? xx : yy,
                   (orient == wxVERTICAL) ? yy : xx,
                   (shift[k] < 0) ? feed_hole : data_hole,
                   paper_colour, hole_colour);
      }
      xx += spacing;
    }
    
    dc.SetPen(wxNullPen);
    dc.SetBrush(wxNullBrush);
  } else {
    /*
     * Form this one by bitbliting from the basic ones
     */
    wxMemoryDC src_dc;
    
    int j, k, m;
    bool b;
    
    double xx = spacing/2.0;
    double next_xx;
    
    for (j=0; j<9; j++) {
      next_xx = xx + spacing;
      
      k = (mirror) ? (8-j) : j;
          
      if (shift[k] < 0) {
        b = true;
      } else {
        b = (i >> shift[k]) & 1;
      }

      if (b) {
        if (shift[k] < 0) {
          m = 0;
        } else {
          m = 1 << shift[k];
        }
        
        src_dc.SelectObject(*GetBitmap(m, type));
        
        int x = static_cast<int>(floor(xx));
        int w = static_cast<int>(floor(next_xx)) - x;
        
        dc.Blit( (orient == wxVERTICAL) ? x : 0,
                 (orient == wxVERTICAL) ? 0 : x,
                 (orient == wxVERTICAL) ? w : row_spacing,
                 (orient == wxVERTICAL) ? row_spacing : w,
                 &src_dc,
                 (orient == wxVERTICAL) ? x : 0,
                 (orient == wxVERTICAL) ? 0 : x );
        
      }
      xx = next_xx;
    }
  }

  /*
   * For the "light" type used to represent the
   * reading point for a paper-tape reader draw a line
   * across the centre of the dots
   */
  if (type == PT_light) {
    hole_colour.Set(0, 0, 0);
    wxPen pen(hole_colour, 1, wxSOLID);
    dc.SetPen(pen);
    if (orient == wxVERTICAL) {
      dc.DrawLine(0, row_spacing/2, current_tape_width, row_spacing/2);
    } else {
      dc.DrawLine(row_spacing/2, 0, row_spacing/2, current_tape_width);
    }
    dc.SetPen(wxNullPen);
  }

  dc.SetBrush(wxNullBrush);
  dc.SetBackground(wxNullBrush);

  if ((type != PT_tail_triangle) && (type != PT_lead_triangle)) {
    return bitmaps[bm];
  }
  
  int it, ib;
  
  if (direction == PT_BottomToTop) {
    ii = (num_type[type]-1) - ii;
    ib = ii - (num_type[type]-1);
    it = ii;
  } else {
    it = ii - (num_type[type]-1);
    ib = ii;
  }

  if (type == PT_tail_triangle) {
    double xt[2], yt[2], tt;
    long xc[2], yc[2], tc;
    double m, c;
    bool lr;
    
    xt[0] = -2.5 * spacing;
    yt[0] = static_cast<double>(it) * static_cast<double>(row_spacing);
    xt[1] = 4.0 * spacing;
    yt[1] = (static_cast<double>(ib) + 0.5) * static_cast<double>(row_spacing);
    
    xc[0] = 0;
    yc[0] = 0;
    xc[1] = static_cast<long>(ceil(xt[1]));
    yc[1] = row_spacing;
    
    if (mirror) {
      xt[0] = d_paper_tape_width - xt[0];
      xt[1] = d_paper_tape_width - xt[1];
      tc = xc[0];
      xc[0] = current_tape_width - xc[1];
      xc[1] = current_tape_width - tc;
    }
    
    if (orient == wxHORIZONTAL) {
      tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
      tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
      tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
      tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;
      
      lr = (direction == PT_RightToLeft);
    } else {
      lr = !mirror;
    }
    
    m = (yt[1] - yt[0]) / (xt[1] - xt[0]);
    c = yt[0] - (m * xt[0]);
    
    FillLine(dc, m, c, lr, 
             xc, yc, hole_colour);
    
    xt[0] = d_paper_tape_width + (0.5 * spacing);
    yt[0] = static_cast<double>(it) * static_cast<double>(row_spacing);
    xt[1] = 4.0 * spacing;
    yt[1] = (static_cast<double>(ib) + 0.5) * static_cast<double>(row_spacing);
    
    xc[0] = static_cast<long>(ceil(xt[1]));
    yc[0] = 0;
    xc[1] = current_tape_width;
    yc[1] = row_spacing;
    
    if (mirror) {
      xt[0] = d_paper_tape_width - xt[0];
      xt[1] = d_paper_tape_width - xt[1];
      tc = xc[0];
      xc[0] = current_tape_width - xc[1];
      xc[1] = current_tape_width - tc;
    }
    
    if (orient == wxHORIZONTAL) {
      tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
      tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
      tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
      tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;
      
      lr = (direction == PT_RightToLeft);
    } else {
      lr = mirror;
    }
    m = (yt[1] - yt[0]) / (xt[1] - xt[0]);
    c = yt[0] - (m * xt[0]);
    
    FillLine(dc, m, c, lr, 
             xc, yc, hole_colour);
  } else if (type == PT_lead_triangle) {
    double xt[2], yt[2], tt;
    long xc[2], yc[2], tc;
    double m, c;
    bool lr;
    
    xt[0] = -2.5 * spacing;
    yt[0] = static_cast<double>(it) * static_cast<double>(row_spacing);
    xt[1] = 4.5 * spacing;
    yt[1] = (static_cast<double>(ib) + 1.0) * static_cast<double>(row_spacing);
    
    xc[0] = 0;
    yc[0] = 0;
    xc[1] = static_cast<long>(ceil(xt[1]));
    yc[1] = row_spacing;
    
    if (mirror) {
      xt[0] = d_paper_tape_width - xt[0];
      xt[1] = d_paper_tape_width - xt[1];
      tc = xc[0];
      xc[0] = current_tape_width - xc[1];
      xc[1] = current_tape_width - tc;
    }

    if (orient == wxHORIZONTAL) {
      tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
      tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
      tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
      tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;
      
      lr = (direction != PT_RightToLeft);
    } else {
      lr = mirror;
    }
    
    m = (yt[1] - yt[0]) / (xt[1] - xt[0]);
    c = yt[0] - (m * xt[0]);
    
    FillLine(dc, m, c, lr,
             xc, yc, hole_colour);
    
    xt[0] = d_paper_tape_width + (0.5 * spacing);
    yt[0] = static_cast<double>(it) * static_cast<double>(row_spacing);
    xt[1] = 3.5 * spacing;
    yt[1] = (static_cast<double>(ib) + 1.0) * static_cast<double>(row_spacing);
    
    xc[0] = static_cast<long>(ceil(xt[1]));
    yc[0] = 0;
    xc[1] = current_tape_width;
    yc[1] = row_spacing;
    
    if (mirror) {
      xt[0] = d_paper_tape_width - xt[0];
      xt[1] = d_paper_tape_width - xt[1];
      tc = xc[0];
      xc[0] = current_tape_width - xc[1];
      xc[1] = current_tape_width - tc;
    }
    
    if (orient == wxHORIZONTAL) {
      tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
      tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
      tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
      tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;
      
      lr = (direction != PT_RightToLeft);
    } else {
      lr = !mirror;
    }
    
    m = (yt[1] - yt[0]) / (xt[1] - xt[0]);
    c = yt[0] - (m * xt[0]);
    
    FillLine(dc, m, c, lr, 
             xc, yc, hole_colour);
    
  }
  
  return bitmaps[bm];
}

void PaperTape::SetScrollBars()
{
  long ts = total_size();

  /*
   * Since adding a scrollbar makes the width of the client less,
   * it makes the number of rows of tape that can be displayed greater,
   * which in turn may mean that a scrollbar is no longer required.
   * So with automatic control of the scrollbars it is possible to get
   * stuck in a loop alternately adding and removing the scrollbars.
   *
   * Use manual control - always have scrollbars, unless there is no
   * tape at all to display.
   *
  if (orient == wxVERTICAL) {
    ShowScrollbars(wxSHOW_SB_NEVER, ((ts == 0) ? wxSHOW_SB_NEVER : wxSHOW_SB_ALWAYS));
  } else {
    ShowScrollbars(((ts == 0) ? wxSHOW_SB_NEVER : wxSHOW_SB_ALWAYS), wxSHOW_SB_NEVER);
  }
  */
  if (orient == wxVERTICAL) {
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);
  } else {
    ShowScrollbars(wxSHOW_SB_ALWAYS, wxSHOW_SB_NEVER);
  }

  if (current_tape_width > 0) {
    // i.e. OnSize() has been run...
    long total = black_start + ts + black_end;
    
    if (orient == wxVERTICAL) {
      SetScrollbars(0, row_spacing, 0, total);
    } else {
      SetScrollbars(row_spacing, 0, total, 0);
    }
    SetScrollBarPosition();
  }
}

void PaperTape::SetScrollBarPosition()
{
  long pos = position;
  
  //std::cout << __PRETTY_FUNCTION__ << " position = " << position << std::endl;
  //std::cout << __PRETTY_FUNCTION__ << " total_size = " << total_size() << std::endl;

  if (direction == PT_LeftToRight) {
    pos = total_size() - position;
  }

  if (paper_tape_offset != 0) {
    pos++;
  }

  //std::cout << __PRETTY_FUNCTION__ << " pos = " << pos << std::endl;
  
  /*
   * NB this works because the MIDDLE of the window
   * is offset by "black_start" which is also the amount
   * by which the position should be adjusted.
   */
  
  if (orient == wxVERTICAL) {
    Scroll(0, pos);
  } else {
    Scroll(pos, 0);
  }
}

void PaperTape::OnSize(wxSizeEvent &WXUNUSED(event))
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  int width, height;
  GetClientSize(&width, &height);

  //std::cout << "reader = " << reader << std::endl;
  //std::cout << "width = " << width << std::endl;
  //std::cout << "height = " << height << std::endl;
  
  int paper_tape_width = (orient == wxVERTICAL) ? width : height;

  if (paper_tape_width > 0) {
    double d_paper_tape_width = static_cast<double>(paper_tape_width);
    double spacing = d_paper_tape_width / 10.0;
    
    /*
     * spacing of the rows of tape is always set to
     * an integer to allow them to be bit-blitted into
     * place
     */
    row_spacing = static_cast<int>(ceil(spacing));
    if (row_spacing <= 0) {
      row_spacing = 1;
    }
    
    
    int paper_tape_visible_length = (orient == wxVERTICAL) ? height :  width;
    paper_tape_visible_rows  = (paper_tape_visible_length + row_spacing - 1) / row_spacing;
    int paper_tape_full_rows =  paper_tape_visible_length                    / row_spacing;
    paper_tape_offset = 0;
    if (direction == PT_BottomToTop) {
      paper_tape_offset = paper_tape_visible_length - (paper_tape_full_rows * row_spacing);
    }
    
    black_start = (reader) ? (paper_tape_visible_rows/2) : paper_tape_visible_rows;
    black_end = paper_tape_visible_rows - black_start;
    
    if (current_tape_width != paper_tape_width) {
      DestroyBitmaps();
      AllocateBitmaps();
      current_tape_width = paper_tape_width;
    }

    SetScrollBars();
  }

  Refresh();
}

void PaperTape::OnPaint(wxPaintEvent &WXUNUSED(event))
{
  wxPaintDC dc(this);
  PrepareDC(dc);
  
  int width, height;
  int view_start_x, view_start_y;
  
  GetClientSize(&width, &height);
  GetViewStart(&view_start_x, &view_start_y);
  
  //int paper_tape_visible_length = (orient == wxVERTICAL) ? height :  width;
  //int paper_tape_visible_rows = (paper_tape_visible_length + row_spacing - 1) / row_spacing;
  //int paper_tape_full_rows    =  paper_tape_visible_length                    / row_spacing;
  int paper_tape_first_visible = (orient == wxVERTICAL) ? view_start_y : view_start_x;
  //int offset = 0;
  if ((paper_tape_offset > 0) && (paper_tape_first_visible > 0)) {
    paper_tape_first_visible--;
  }
  wxMemoryDC src_dc;

  //if (direction == PT_BottomToTop) {
  //  std::cout << "paper_tape_visible_length = " << paper_tape_visible_length << std::endl;
  //  std::cout << "row_spacing = " << row_spacing << std::endl;
  //  offset = paper_tape_visible_length - (paper_tape_full_rows * row_spacing);
  //  std::cout << "offset = " << offset << std::endl;
  //}
  
  int i;
  for (i=0; i < paper_tape_visible_rows; i++) {
    unsigned long pos = i + paper_tape_first_visible;
    unsigned long total = black_start + total_size() + black_end;
    unsigned long index;
    unsigned int data;
    PT_type type;

    /*
    if (pos >= total) {
      pos = total - 1;
    }
    */
    
    if (direction == PT_TopToBottom) {
      if (pos >= total) {
        pos = 0;
      } else {
        pos = (total - 1) - pos;
      }
    }
    
    data = 0;
    type = PT_holes;
    
    if (pos < black_start) {
      type = PT_no_tape;
    } else if ((index = pos - black_start) < total_size()) {
      data = get_element(index, type);
      
      if (((type == PT_holes) || (type == PT_leader)) &&
          (index == position)) {
        type = PT_light;
      }
    } else {
      type = PT_no_tape;
    }
    src_dc.SelectObject(*GetBitmap(data, type));
    
    dc.Blit( (orient == wxVERTICAL) ? 0 : row_spacing * (i + paper_tape_first_visible) - paper_tape_offset,
             (orient == wxVERTICAL) ? row_spacing * (i + paper_tape_first_visible)  - paper_tape_offset : 0,
             (orient == wxVERTICAL) ? current_tape_width : row_spacing,
             (orient == wxVERTICAL) ? row_spacing : current_tape_width,
             &src_dc, 0, 0);
  } 
}

void PaperTape::DrawCircle(wxDC &dc,
                           double xc, double yc, double r,
                           wxColour &background, wxColour &foreground)
{
  /*
   * Calculate the bounding coordinates of the pixels to be visited
   */

  long x_min = static_cast<long>(floor(xc-r));
  long y_min = static_cast<long>(floor(yc-r));
  long x_max = static_cast<long>(ceil(xc+r));
  long y_max = static_cast<long>(ceil(yc+r));
  
  long xx, yy;
  
  double dxx;
  double dyy;
  double d = 1.0 / 16.0;

  int area, n_area;
  int i, j;

  long b_red, b_green, b_blue;
  long f_red, f_green, f_blue;
  long red, green, blue;
  wxColour colour;
  wxPen pen(background, 1, wxSOLID);

  b_red = background.Red();
  b_green = background.Green();
  b_blue = background.Blue();
  f_red = foreground.Red();
  f_green = foreground.Green();
  f_blue = foreground.Blue();

  for (yy = y_min; yy < y_max; yy++) {
    for (xx = x_min; xx < x_max; xx++) {
      area = InsideCircle(xc, yc, r,
                          static_cast<double>(xx), static_cast<double>(yy), 1.0);
      
      switch (area) {
      case 2:
        area = 512;
        break;
        
      case 1:
        /*
         * Pixel straddles the circle.
         * divide into 256 mini-pixels and count how many are inside
         * and how many outside...
         */
        area = 0;
        dyy = static_cast<double>(yy);
        
        for (j=0; j<16; j++) {
          dxx = static_cast<double>(xx);
          
          for (i=0; i<16; i++) {
            area += InsideCircle(xc, yc, r,
                                 dxx, dyy, d);
            
            dxx += d;
          }
          dyy += d;
        }
        break;
        
      case 0:
        break;
        
      default:
        abort();
      }
      
      //std::cout << "(" << xx << "," << yy << ") area = " << static_cast<double>(area)/512.0 << std::endl;
      
      /*
       * Form appropriate colour
       */
      n_area = 512 - area;
      
      red = ((area * f_red) + (n_area * b_red)) / 512;
      green = ((area * f_green) + (n_area * b_green)) / 512;
      blue = ((area * f_blue) + (n_area * b_blue)) / 512;
      
      colour.Set(red, green, blue);
      pen.SetColour(colour);
      
      dc.SetPen(pen);
      dc.DrawPoint(xx, yy);
    }
  }
}

int PaperTape::InsideCircle(double xc, double yc, double r,
                            double x, double y, double d)
{
  /*
   * Given a circle defined by its centre (xc, yc) and radius, r
   * and a square with vertices at (x, y), (x+d, y), (x, y+d) and
   * (x+d, y+d)
   * Calculate whether the square lies wholly within the
   * circle (return 2), wholly outside the circle (return 0),
   * or straddles (return 1)
   */

  double r_squared = r * r;
  double dy, dx, d_squared;
  
  bool all_inside, all_outside;
  
  int i, j;
  double xx, yy;

  all_inside = all_outside = true;
  
  for (j = 0; j < 2; j++) {
    for (i = 0; i < 2; i++) {
      xx = x;
      yy = y;
      
      if (i > 0) {
        xx += d;
      }
      
      if (j > 0) {
        yy += d;
      }
      
      dx = xx - xc;
      dy = yy - yc;
      
      d_squared = (dx * dx) + (dy * dy);
      
      if (d_squared < r_squared) {
        all_outside = false;
      } else if (d_squared > r_squared) {
        all_inside = false;
      }
    }
  }
  
  if (all_inside) {
    return 2;
  } else if (all_outside) {
    return 0;
  }
  
  return 1;
}


void PaperTape::FillLine(wxDC &dc,
                         double m, double c, bool right,
                         long xc[2], long yc[2],
                         wxColour &foreground)
{
  long xx, yy;
  
  double dxx;
  double dyy;
  double d = 1.0 / 16.0;
  
  int area, n_area;
  
  wxColour background;
  long b_red, b_green, b_blue;
  long f_red, f_green, f_blue;
  long red, green, blue;
  wxColour colour;
  wxPen pen(background, 1, wxSOLID);
  
  f_red = foreground.Red();
  f_green = foreground.Green();
  f_blue = foreground.Blue();
  
  int i, j;

  for (yy = yc[0]; yy < yc[1]; yy++) {
    for (xx = xc[0]; xx < xc[1]; xx++) {
      area = LeftOfLine(m, c, static_cast<double>(xx), static_cast<double>(yy), 1.0);
      
      switch (area) {
      case 2:
        area = 512;
        break;
        
      case 1:
        /*
         * Pixel straddles the triangle.
         * divide into 256 mini-pixels and count how many are inside
         * and how many outside...
         */
        area = 0;
        dyy = static_cast<double>(yy);
        
        for (j=0; j<16; j++) {
          dxx = static_cast<double>(xx);
            
          for (i=0; i<16; i++) {
            area += LeftOfLine(m, c, dxx, dyy, d);
            dxx += d;
          }
          dyy += d;
        }
        
        break;
        
      case 0:
        break;
        
      default:
        abort();
      }
      
      if (right) {
        area = 512 - area;
      }
      
      /*
       * Form appropriate colour
       */
      
      n_area = 512 - area;
      
      if (area != 0) {
        if (n_area != 0) {
          dc.GetPixel(xx, yy, &background);
          
          b_red = background.Red();
          b_green = background.Green();
          b_blue = background.Blue();
          
          
          red = ((area * f_red) + (n_area * b_red)) / 512;
          green = ((area * f_green) + (n_area * b_green)) / 512;
          blue = ((area * f_blue) + (n_area * b_blue)) / 512;
          colour.Set(red, green, blue);
        } else {
          colour.Set(f_red, f_green, f_blue);
        }
        pen.SetColour(colour);
        
        dc.SetPen(pen);
        dc.DrawPoint(xx, yy);
      }
    }
  }
}

int PaperTape::LeftOfLine(double m, double c,
                          double x, double y, double d)
{
  bool all_left, all_right;
  
  int i, j, a;
  double xx, yy;
  
  all_left = all_right = true;
  
  for (j = 0; j < 2; j++) {
    for (i = 0; i < 2; i++) {
      xx = x;
      yy = y;
      
      if (i > 0) {
        xx += d;
      }
      
      if (j > 0) {
        yy += d;
      }
      
      a = PointLeftOfLine(m, c, xx, yy);
      
      switch (a) {
      case 0:
        all_left = false;
        break;
        
      case 2:
        all_right = false;
        break;
        
      default:
        break;
      }
    }
  }
  
  if (all_left) {
    return 2;
  } else if (all_right) {
    return 0;
  }
  
  return 1;
}

/*
 * Is  a point (x, y) to the left of the line
 * defined by y=mx + c (m != 0)
 *
 * return 2 if it is to the left
 * return 0 if it is to the right
 * return 1 if it lies on the line
 */
int PaperTape::PointLeftOfLine(double m, double c,
                                 double x, double y)
{
  double xx;

  xx = (y - c) / m;

  if (xx > x) {
    return 2;
  } else if (xx < x) {
    return 0;
  } else {
    return 1;
  }
}

unsigned long PaperTape::total_size(bool only_holes)
{
  unsigned long s = 0;
  std::list<TapeChunk>::iterator i;

  for (i=chunks.begin(); i!=chunks.end(); i++) {
    if ((!only_holes) || (i->get_type() == PT_holes)) {
      s += i->get_size();
    }
  }
  
  return s;
}

unsigned int PaperTape::get_element(unsigned int index, PT_type &type)
{
  std::list<TapeChunk>::iterator i;
  unsigned int j;

  i=chunks.begin();

  j = index;
  while (i != chunks.end()) {
    if (j < i->get_size()) {
      return i->get_element(j, type);
    }
    j -= i->get_size();
    i++;
  }
  
  type = PT_num_types;
  return 0;
}
