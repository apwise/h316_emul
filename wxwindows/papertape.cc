#include <wx/wx.h> 
#include <wx/tglbtn.h>
#include <wx/stattext.h>
#include <wx/dcclient.h>
#include <wx/timer.h>

#include <cmath>
#include <iostream>

#include <list>

#include "papertape.hh"

#define TIMER_PERIOD 100 // update every tenth of a second

BEGIN_EVENT_TABLE(wxPaperTape, wxScrolledWindow)
    EVT_PAINT  (wxPaperTape::OnPaint)
    EVT_TIMER(-1, wxPaperTape::OnTimer)
END_EVENT_TABLE()


int wxPaperTape::num_type[wxPT_num_types] =
{
  256,
  256,
  1,
  1,
  1,
  7,
  7
};

class wxPaperTape::TapeChunk
{
public:
  TapeChunk(wxPT_type type, long size = 0, const char *buffer = 0);
  ~TapeChunk();

  long get_size() {return size;};
  int get_element(int index, wxPT_type &type);

private:
  wxPT_type type;
  long size;
  char *buffer;

};

wxPaperTape::TapeChunk::TapeChunk(wxPT_type type, long size, const char *buffer)
  : type(type),
    size(size)
{
  if ((type == wxPT_lead_triangle) ||
      (type == wxPT_tail_triangle))
    this->size = num_type[type];

  if (buffer)
    {
      this->buffer = new char[size];
      int i;
      for (i=0; i<size; i++)
        this->buffer[i] = buffer[i];
    }
  else
    this->buffer = 0;
}

wxPaperTape::TapeChunk::~TapeChunk()
{
  if (buffer)
    delete [] buffer;
}

int wxPaperTape::TapeChunk::get_element(int index, wxPT_type &type)
{
  if (index < size)
    {
      type = this->type;

      if ((type == wxPT_lead_triangle) || (type == wxPT_tail_triangle))
        return index;
      else if (buffer)
        return (buffer[index] & 255);
      else
        return 0;
    }

  type = wxPT_num_types;
  return 0;
}

wxPaperTape::wxPaperTape( wxWindow *parent,
                          wxWindowID id,
                          const wxPoint &pos,
                          const wxSize &size,
                          bool reader,
                          int orient,
                          enum wxPT_direction direction,
                          bool mirror,
                          const wxString &name )
  : wxScrolledWindow(parent, id, pos, size,
                     (orient == wxVERTICAL) ? wxVSCROLL : wxHSCROLL,
                     name),
    parent(parent),
    reader(reader),
    orient(orient),
    direction(direction),
    mirror(mirror),

    current_tape_width(-1),
    bitmaps(0)
{
  /*
   * At this point we don't know how big (in pixels) the
   * window is - so delay anything real until the first call
   * to OnPaint(), however we know that we do want a scrollbar
   * and painting it will change the client area size so
   * ask for a default scrollbar now.
   */
  if (orient == wxVERTICAL)
    SetScrollbars(0, 10, 0, 100);
  else
    SetScrollbars(10, 0, 100, 0);

  /*
   * No point allowing physical scrolling because we always
   * repaint the whole visible area of paper-tape anyway
   */
  EnableScrolling(false, false);

  position = initial_position = 0;

  timer = new wxTimer(this);
  pending_refresh = false;

  /* for testing ... */
  
//   int i;
//   static char buffer[1024];
//   for (i=0; i<1024;i++)
//     {
//       buffer[i] = i & 255;
//       //std::cout << "buffer[" << i << "] = " << (static_cast<int>(buffer[i]) & 255) << std::endl;
//     }

//   chunks.push_back(*new TapeChunk(wxPT_lead_triangle));
//   chunks.push_back(*new TapeChunk(wxPT_leader, 20));
//   chunks.push_back(*new TapeChunk(wxPT_holes, 1024, buffer));
//   chunks.push_back(*new TapeChunk(wxPT_leader, 20));
//   chunks.push_back(*new TapeChunk(wxPT_tail_triangle));
  

}

wxPaperTape::~wxPaperTape( )
{
  DestroyBitmaps();
}

void wxPaperTape::Load(const char *buffer, long size)
{
  /*
   * If there are any existing chunks then the need
   * to be deleted
   */

  if (!chunks.empty())
    chunks.clear();

  const int leader = 20;

  chunks.push_back(*new TapeChunk(wxPT_lead_triangle));
  chunks.push_back(*new TapeChunk(wxPT_leader, leader));
  chunks.push_back(*new TapeChunk(wxPT_holes, size, buffer));
  chunks.push_back(*new TapeChunk(wxPT_leader, leader));
  chunks.push_back(*new TapeChunk(wxPT_tail_triangle));
  
  /*
   * set the current file pointer to half way through
   * the added leader
   */

  position = initial_position = num_type[wxPT_lead_triangle] + leader/2;

  /*
   * Need to set the scrolling position
   */
  SetScrollBars();

  /*
   * Force a repaint
   */
  Refresh();
}

void wxPaperTape::Rewind()
{
  position = initial_position;
  SetScrollBarPosition();
  Refresh();
}

int wxPaperTape::Read()
{
  wxPT_type type;
  int data = get_element(position, type);

  if ((type == wxPT_holes) || (type == wxPT_leader))
    {
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
    }
  else
    return -1;
}

void wxPaperTape::DestroyBitmaps()
{
  int i;

  if (bitmaps)
    {
      for (i=0; i<start_type[wxPT_num_types]; i++)
        if (bitmaps[i])
          delete (bitmaps[i]);
      delete [] bitmaps;
    }

  bitmaps = 0;
}

void wxPaperTape::AllocateBitmaps()
{
  int i, j;

  if (bitmaps)
    abort();

  j = 0;
  for (i=0; i<wxPT_num_types; i++)
    {
      start_type[i] = j;
      j += num_type[i];
    }
  start_type[wxPT_num_types] = j;

  bitmaps = new wxBitmap *[start_type[wxPT_num_types]];
  
  for (i=0; i<start_type[wxPT_num_types]; i++)
    bitmaps[i] = 0;
  
}

void wxPaperTape::set_pending_refresh()
{
  pending_refresh = true;

  if (!timer->IsRunning())
    timer->Start(TIMER_PERIOD);
}

void wxPaperTape::OnTimer(wxTimerEvent& WXUNUSED(event))
{
  if (pending_refresh)
    {
      pending_refresh = false;
      
      SetScrollBarPosition();
      Refresh();
    }
  else
    timer->Stop();
}

wxBitmap *wxPaperTape::GetBitmap(int i, wxPT_type type)
{
  int bm = start_type[static_cast<int>(type)] + i;

  if ((i < 0) ||
      (i >= num_type[static_cast<int>(type)]))
    abort();

  if (bitmaps[bm])
    return bitmaps[bm];

  /*
   * Allocate a bitmap
   */

  double d_paper_tape_width = static_cast<double>(current_tape_width);
  double spacing = d_paper_tape_width / 10.0;
        
  if (orient == wxVERTICAL)
    bitmaps[bm] = new wxBitmap(current_tape_width, row_spacing, -1);
  else
    bitmaps[bm] = new wxBitmap(row_spacing, current_tape_width, -1);

  wxMemoryDC dc;
  dc.SelectObject(*bitmaps[bm]);

  wxColour hole_colour(0,0,0);
  wxBrush hole_brush(hole_colour, wxSOLID);

  if (type == wxPT_no_tape)
    {
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

  /*
   * For the "light" type used to represent the
   * reading point for a paper-tape reader draw a line
   * across the centre of the dots
   */
  if (type == wxPT_light)
    {
      wxPen pen(hole_colour, 1, wxSOLID);
      dc.SetPen(pen);
      if (orient == wxVERTICAL)
        dc.DrawLine(0, row_spacing/2, current_tape_width, row_spacing/2);
      else
        dc.DrawLine(row_spacing/2, 0, row_spacing/2, current_tape_width);

      dc.SetPen(wxNullPen);
    }

  dc.SetBrush(wxNullBrush);
  dc.SetBackground(wxNullBrush);

  if (type == wxPT_no_feed)
    return bitmaps[bm];  

  static int shift[9] = {7, 6, 5, -1, 4, 3, 2, 1, 0};      

  bool drawcircles = false;

  int ii = i;
  if ((type == wxPT_lead_triangle) ||
      (type == wxPT_tail_triangle))
    i = 0;

  switch(i)
    {
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

  if (drawcircles)
    {   
      /*
       * These basic cases are drawn using floating-point
       * anti-aliasing routines.
       */
      
      double yy = spacing/2.0;
      double xx = spacing;
      double data_hole = d_paper_tape_width / 32.0;
      double feed_hole = d_paper_tape_width / 64.0;
      
      if (type == wxPT_light)
        {
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
      
      for (j=0; j<9; j++)
        {
          k = (mirror) ? (8-j) : j;
          
          if (shift[k] < 0)
            b = true;
          else
            b = (i >> shift[k]) & 1;
          
          if (b)
            DrawCircle(dc,
                       (orient == wxVERTICAL) ? xx : yy,
                       (orient == wxVERTICAL) ? yy : xx,
                       (shift[k] < 0) ? feed_hole : data_hole,
                       paper_colour, hole_colour);
          xx += spacing;
        }
      
      dc.SetPen(wxNullPen);
      dc.SetBrush(wxNullBrush);
    }
  else
    {
      /*
       * Form this one by bitbliting from the basic ones
       */
      wxMemoryDC src_dc;
      
      int j, k, m;
      bool b;
      
      double xx = spacing/2.0;
      double next_xx;
      
      for (j=0; j<9; j++)
        {
          next_xx = xx + spacing;
          
          k = (mirror) ? (8-j) : j;
          
          if (shift[k] < 0)
            b = true;
          else
            b = (i >> shift[k]) & 1;
          
          if (b)
            {
              if (shift[k] < 0)
                m = 0;
              else
                m = 1 << shift[k];
              
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

  if ((type != wxPT_tail_triangle) && (type != wxPT_lead_triangle))
    return bitmaps[bm];

  int it, ib;

  if (direction == wxPT_BottomToTop)
    {
      ii = (num_type[type]-1) - ii;
      ib = ii - (num_type[type]-1);
      it = ii;
    }
  else
    {
      it = ii - (num_type[type]-1);
      ib = ii;
    }

  if (type == wxPT_tail_triangle)
    {
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

      if (mirror)
        {
          xt[0] = d_paper_tape_width - xt[0];
          xt[1] = d_paper_tape_width - xt[1];
          tc = xc[0];
          xc[0] = current_tape_width - xc[1];
          xc[1] = current_tape_width - tc;
        }
 
      if (orient == wxHORIZONTAL)
        {
          tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
          tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
          tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
          tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;

          lr = (direction == wxPT_RightToLeft);
        }
      else
        lr = !mirror;

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

      if (mirror)
        {
          xt[0] = d_paper_tape_width - xt[0];
          xt[1] = d_paper_tape_width - xt[1];
          tc = xc[0];
          xc[0] = current_tape_width - xc[1];
          xc[1] = current_tape_width - tc;
        }
      
      if (orient == wxHORIZONTAL)
        {
          tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
          tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
          tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
          tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;
          
          lr = (direction == wxPT_RightToLeft);
        }
      else
        lr = mirror;

      m = (yt[1] - yt[0]) / (xt[1] - xt[0]);
      c = yt[0] - (m * xt[0]);
      
      FillLine(dc, m, c, lr, 
                xc, yc, hole_colour);
    }
  else if (type == wxPT_lead_triangle)
    {
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

      if (mirror)
        {
          xt[0] = d_paper_tape_width - xt[0];
          xt[1] = d_paper_tape_width - xt[1];
          tc = xc[0];
          xc[0] = current_tape_width - xc[1];
          xc[1] = current_tape_width - tc;
        }

      if (orient == wxHORIZONTAL)
        {
          tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
          tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
          tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
          tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;

          lr = (direction != wxPT_RightToLeft);
        }
      else
        lr = mirror;

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

      if (mirror)
        {
          xt[0] = d_paper_tape_width - xt[0];
          xt[1] = d_paper_tape_width - xt[1];
          tc = xc[0];
          xc[0] = current_tape_width - xc[1];
          xc[1] = current_tape_width - tc;
        }

      if (orient == wxHORIZONTAL)
        {
          tt = xt[0]; xt[0] = yt[0]; yt[0] = tt;
          tt = xt[1]; xt[1] = yt[1]; yt[1] = tt;
          tc = xc[0]; xc[0] = yc[0]; yc[0] = tc;
          tc = xc[1]; xc[1] = yc[1]; yc[1] = tc;

          lr = (direction != wxPT_RightToLeft);
        }
      else
        lr = !mirror;

      m = (yt[1] - yt[0]) / (xt[1] - xt[0]);
      c = yt[0] - (m * xt[0]);
      
      FillLine(dc, m, c, lr, 
                xc, yc, hole_colour);
      
    }

  return bitmaps[bm];
}

void wxPaperTape::SetScrollBars()
{
  long total = black_start + total_size() + black_end;
  
  if (orient == wxVERTICAL)
    SetScrollbars(0, row_spacing, 0, total);
  else
    SetScrollbars(row_spacing, 0, total, 0);
  SetScrollBarPosition();
}

void wxPaperTape::SetScrollBarPosition()
{
  long pos = position;

  if (direction == wxPT_LeftToRight)
    pos = total_size() - position;

  /*
   * NB this works because the MIDDLE of the window
   * is offset by "black_start" which is also the amount
   * by which the position should be adjusted.
   */

  if (orient == wxVERTICAL)
    Scroll(0, pos);
  else
    Scroll(pos, 0);
}

void wxPaperTape::OnPaint(wxPaintEvent &WXUNUSED(event))
{
  wxPaintDC dc(this);
  PrepareDC(dc);

  int width, height;
  int view_start_x, view_start_y;

  GetClientSize(&width, &height);
  GetViewStart(&view_start_x, &view_start_y);

  //std::cout << "wxPaperTape::OnPaint width = " << width << "height = " << height << std::endl;
  //std::cout << "wxPaperTape::OnPaint ViewStart (" << view_start_x << "," << view_start_y << ")" << std::endl;

  //int xcb, ycb, wcb, hcb;
  //dc.GetClippingBox(&xcb, &ycb, &wcb, &hcb);
  //std::cout << "wxPaperTape::OnPaint ClippingBox(" << xcb << "," << ycb << "," << wcb << "," << hcb << ")" << std::endl;

  int paper_tape_width = (orient == wxVERTICAL) ? width : height;
  double d_paper_tape_width = static_cast<double>(paper_tape_width);
  double spacing = d_paper_tape_width / 10.0;

  /*
   * spacing of the rows of tape is always set to
   * an integer to allow them to be bit-blitted into
   * place
   */
  row_spacing = static_cast<int>(ceil(spacing));
  if (row_spacing <= 0)
    row_spacing = 1;
  
  int paper_tape_visible_length = (orient == wxVERTICAL) ? height :  width;
  int paper_tape_visible_spaces = (paper_tape_visible_length + row_spacing - 1) / row_spacing;
  int paper_tape_first_visible = (orient == wxVERTICAL) ? view_start_y : view_start_x;
//   std::cout << "wxPaperTape::OnPaint Redraw position " << paper_tape_first_visible << " for " << paper_tape_visible_spaces << std::endl;

  if (current_tape_width != paper_tape_width)
    {
      current_tape_width = paper_tape_width;
     
      DestroyBitmaps();
      AllocateBitmaps();

      black_start = paper_tape_visible_spaces/2;
      black_end = paper_tape_visible_spaces - black_start;

      SetScrollBars();
    }
   
  wxMemoryDC src_dc;

  int i;
  for (i=0; i < paper_tape_visible_spaces; i++)
    {
      int pos = i + paper_tape_first_visible;
      int total = black_start + total_size() + black_end;
      int index, data;
      wxPT_type type;

      if (pos >= total)
        pos = total - 1;

      if (direction == wxPT_TopToBottom)
        pos = (total - 1) - pos;

      data = 0;
      type = wxPT_holes;

      if (pos < black_start)
        type = wxPT_no_tape;
      else if ((index = pos - black_start) < total_size())
        {
          data = get_element(index, type);

          if ((reader) &&
              ((type == wxPT_holes) || (type == wxPT_leader)) &&
              (index == position))
            type = wxPT_light;
        }
      else
        type = wxPT_no_tape;

      src_dc.SelectObject(*GetBitmap(data, type));
      
      dc.Blit( (orient == wxVERTICAL) ? 0 : row_spacing * (i + paper_tape_first_visible),
               (orient == wxVERTICAL) ? row_spacing * (i + paper_tape_first_visible) : 0,
               (orient == wxVERTICAL) ? current_tape_width : row_spacing,
               (orient == wxVERTICAL) ? row_spacing : current_tape_width,
               &src_dc, 0, 0);
    }

}

void wxPaperTape::DrawCircle(wxDC &dc,
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

  for (yy = y_min; yy < y_max; yy++)
    for (xx = x_min; xx < x_max; xx++)
      {
        area = InsideCircle(xc, yc, r,
                            static_cast<double>(xx), static_cast<double>(yy), 1.0);

        switch (area)
          {
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

            for (j=0; j<16; j++)
              {
                dxx = static_cast<double>(xx);

                for (i=0; i<16; i++)
                  {
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

int wxPaperTape::InsideCircle(double xc, double yc, double r,
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
  
  for (j = 0; j < 2; j++)
    for (i = 0; i < 2; i++)
      {
        xx = x;
        yy = y;

        if (i > 0)
          xx += d;

        if (j > 0)
          yy += d;

        dx = xx - xc;
        dy = yy - yc;

        d_squared = (dx * dx) + (dy * dy);

        if (d_squared < r_squared)
          all_outside = false;
        else if (d_squared > r_squared)
          all_inside = false;
      }

  if (all_inside)
    return 2;
  else if (all_outside)
    return 0;

  return 1;
}


void wxPaperTape::FillLine(wxDC &dc,
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

  for (yy = yc[0]; yy < yc[1]; yy++)
    for (xx = xc[0]; xx < xc[1]; xx++)
      {
        area = LeftOfLine(m, c, static_cast<double>(xx), static_cast<double>(yy), 1.0);
        
        switch (area)
          {
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

            for (j=0; j<16; j++)
              {
                dxx = static_cast<double>(xx);

                for (i=0; i<16; i++)
                  {
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

        if (right)
          area = 512 - area;

        /*
         * Form appropriate colour
         */

        n_area = 512 - area;

        if (area != 0)
          {
            if (n_area != 0)
              {
                dc.GetPixel(xx, yy, &background);
                
                b_red = background.Red();
                b_green = background.Green();
                b_blue = background.Blue();
                
                
                red = ((area * f_red) + (n_area * b_red)) / 512;
                green = ((area * f_green) + (n_area * b_green)) / 512;
                blue = ((area * f_blue) + (n_area * b_blue)) / 512;
                colour.Set(red, green, blue);
              }
            else
              colour.Set(f_red, f_green, f_blue);
            
            pen.SetColour(colour);

            dc.SetPen(pen);
            dc.DrawPoint(xx, yy);
          }
      }
}

int wxPaperTape::LeftOfLine(double m, double c,
                            double x, double y, double d)
{
  bool all_left, all_right;
  
  int i, j, a;
  double xx, yy;

  all_left = all_right = true;
  
  for (j = 0; j < 2; j++)
    for (i = 0; i < 2; i++)
      {
        xx = x;
        yy = y;

        if (i > 0)
          xx += d;

        if (j > 0)
          yy += d;

        a = PointLeftOfLine(m, c, xx, yy);

        switch (a)
          {
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
  
  if (all_left)
    return 2;
  else if (all_right)
    return 0;

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
int wxPaperTape::PointLeftOfLine(double m, double c,
                                 double x, double y)
{
  double xx;

  xx = (y - c) / m;

  if (xx > x)
    return 2;
  else if (xx < x)
    return 0;
  else
    return 1;
}

long wxPaperTape::total_size()
{
  long s = 0;
  std::list<TapeChunk>::iterator i;

  for (i=chunks.begin(); i!=chunks.end(); i++)
    s += i->get_size();

  return s;
}

int wxPaperTape::get_element(int index, wxPT_type &type)
{
  std::list<TapeChunk>::iterator i;
  int j;

  i=chunks.begin();

  j = index;
  while (i!=chunks.end())
    {
      if (j < i->get_size())
        return i->get_element(j, type);

      j -= i->get_size();
      i++;
    }

  type = wxPT_num_types;
  return 0;
}
