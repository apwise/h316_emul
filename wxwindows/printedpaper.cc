/* Graphical representation of impact page-printer
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

#include <cmath>
#include <iostream>
#include <chrono>

#include "printedpaper.hh"

/*
 * ASR-33
 *
 * Characters are 1/10 inch wide
 * Paper 8.5 inches = 85 characters
 * Printing width is 72 characters
 *
 * Work with 6 characters to left, 7 to right of printable.
 * (6+72+7 = 85)
 *
 * If window is wider than 85 characters, then draw paper in
 * the middle of the window with a border each side.
 */

PrintedPaper::PrintedPaper( wxWindow *parent,
                            wxWindowID id,
                            const wxPoint &pos,
                            const wxSize &size,
                            const wxString &name )
  : wxScrolledCanvas(parent, id, pos, size, 0, name)
  , timer(this, CHARACTER_TIMER_ID)
  , paper_width(85)
  , carriage_width(72)
  , bell_position(62)
  , left_margin((paper_width - carriage_width) / 2)
  , font(wxFontInfo(12).FaceName("TeleType"))
  , font_width(GetCharWidth())
  , font_height(GetCharHeight())
  , fc_width(0)
  , fc_height(0)
  , pc_width(0)
  , pc_height(0)
  , top_offset(0)
  , left_offset(0)
  , FirstLineColumn(0)
  , CurrentAltColour(false)
  , paper(0)
  , paper_bitmap(0)
  , TickCounter(CHARACTERS_PER_SECOND-1)
  , inverted_cursor(false)
  , have_focus(false)
  , chime(false)
  , break_phase(BREAK_NONE)
  , AnswerBackCounter(-1)
{  
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  FontMetrics();
  DecideScrollbars();

  timer.Start(100, wxTIMER_CONTINUOUS);

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  generator.seed(seed);

  int i;
  for (i=1; i<=20; i++) {
    unsigned char ch = i;
    AnswerBackDrum.push_back(ch);
  }
}

PrintedPaper::~PrintedPaper()
{
}

void PrintedPaper::DrawPaper(int width, int height)
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  int x, y;
  
  wxBitmap bitmap(width, height);

  if (paper) {
    delete paper;
  }

  if (paper_bitmap) {
    delete paper_bitmap;
  }
  
  paper_bitmap = new wxBitmap(width, height);
  paper = new wxMemoryDC(bitmap);

  wxColour EdgeColour(0xe6, 0xc7, 0x9b);
  wxColour CentreColour(0xf7, 0xe7, 0xcd);
  
  for (x=0; x<width; x++) {
    // Which text character is this pixel in?
    const unsigned int c = x / font_width;
    wxColour colour;
    
    if ((c < left_offset) || (c >= (left_offset + paper_width))) {
      colour.Set(0,0,0);
    } else {

      int p = x-(left_offset * font_width);
      double theta = static_cast<double>(p) * 2.0 * M_PI / static_cast<double>((paper_width * font_width)-1);
      double scale = 0.5 + 0.5*cos(theta);
      double iscale = 1.0-scale;

      colour.Set(((EdgeColour.Red()   * scale) + (CentreColour.Red()   * iscale)),
                 ((EdgeColour.Green() * scale) + (CentreColour.Green() * iscale)),
                 ((EdgeColour.Blue()  * scale) + (CentreColour.Blue()  * iscale)));
    }
    
    wxPen pen(colour);
    paper->SetPen(pen);
    
    for (y=0; y<height; y++) {
      paper->DrawPoint(x, y);
    }
  }
  
}
  
void PrintedPaper::FontMetrics()
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  wxClientDC dc(this);
  PrepareDC(dc);
  dc.SetFont(font);

  font_width  = dc.GetCharWidth();
  font_height = dc.GetCharHeight();
}

void PrintedPaper::DecideScrollbars()
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  
  wxClientDC dc(this);
  PrepareDC(dc);
  dc.SetFont(font);

  int width, height;

  GetClientSize(&width, &height);

  fc_width  =  width                     / font_width;
  fc_height =  height                    / font_height;
  pc_width  = (width  + font_width  - 1) / font_width;
  pc_height = (height + font_height - 1) / font_height;

  left_offset = (fc_width >= paper_width) ? ((fc_width-paper_width)/2) : 0;

  unsigned int lines = text.size();
  //unsigned int cols  = CursorColumn();

  // If less text than window height, put it in the lower lines
  top_offset  = (lines < fc_height) ? fc_height - lines : 0;

  SetScrollRate(font_width, font_height);
  SetVirtualSize(paper_width * font_width,
                 lines       * font_height);
  /*
  set parameters except the actual position...
    
  SetScrollbars(font_width, font_height, paper_width, lines,
                ((cols  < fc_width)  ? 0 : (cols  - fc_width)),
                ((lines < fc_height) ? 0 : (lines - fc_height)));
  
  ShowScrollbars(((fc_width  < paper_width) ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER),
                 ((fc_height < lines      ) ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER));
  */
  
  DrawPaper(((pc_width < paper_width) ? paper_width : pc_width) * font_width, font_height);
  Refresh();
}

void PrintedPaper::OnSize(wxSizeEvent &WXUNUSED(event))
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  DecideScrollbars();
}

void PrintedPaper::ConvertSingleLine(unsigned int line, unsigned int z, Printable &pr)
{
  unsigned int j;
  bool AltColour = text[line].AltColour;
  
  for (j = 0; j < text[line].Text[z].size(); j++) {
    unsigned int ch = text[line].Text[z][j] & 0x7f;
    bool p = false;
    
    if (ch < ' ') {
      // Handle control characters
      // (None on ASR-33)
      if (/* Some config that says do two-colour ribbon */ 1) {
        if (ch == 016) {
          AltColour = true;
        } else if (ch == 017) {
          AltColour = false;
        }
      }
    } else {
      if (ch <= '_') {
        // Digits, punctuation, and upper case
        p = true;
      } else if ((ch > '_') && (ch < '|')) {
        // ASR-33 map lower-case to upper
        ch &= (~0x20);
        p = true;
      }
    }
    
    if (p) {
      pr.Characters.push_back(ch);
      pr.AltColour.push_back(AltColour);
    }
  }
}

unsigned int PrintedPaper::StartingColumn(unsigned int line)
{
  unsigned int Column;
  
  if (text[line].FollowsReturn) {
    Column = 0;
  } else if (line == 0) {
    Column = FirstLineColumn;
  } else {
    const unsigned int pl = line - 1;
    Cache_t::const_iterator it = cache.find(pl);

    /* If the offset be determined without bringing
     * the previous line into the cache, and the line
     * isn't already in the cache... */
    if (it == cache.cend() &&
        (text[pl].FollowsReturn ||
         (text[pl].Text.size() > 1))) {
      // Either follows, or contains a CR, so offset is the
      // number of characters in the last overprinted line
      Printable pr;
      unsigned int z = (text[pl].Text.empty()) ? 0 : (text[pl].Text.size() - 1);
      ConvertSingleLine(pl, z, pr);
      Column = pr.Characters.size();
    } else {
      // Get the previous line in the cache, which may
      // well recursively call this function
      const CacheLine_t *cl = Cached(pl);
      if (cl->empty()) {
        Column = 0;
      } else {
        Column = (*cl)[cl->size() - 1].Characters.size();
      }
    }
  }
  return Column;
}

unsigned int PrintedPaper::CursorColumn()
{
  unsigned int Column;

  if (text.empty()) {
    Column = FirstLineColumn;
  } else {
    unsigned int line = text.size()-1;
    const CacheLine_t *cl = Cached(line);
    if (cl->empty()) {
      Column = 0;
    } else {
      Column = (*cl)[cl->size() - 1].Characters.size();
    }
    if (Column >= carriage_width) {
      Column = carriage_width - 1;
    }
  }
  return Column;
}

const PrintedPaper::CacheLine_t *PrintedPaper::Cached(unsigned int line)
{
  Cache_t::iterator it;
  
  //std::cout << "Cached("<<line<<")"<<std::endl;
  
  it = cache.find(line);

  if (it != cache.end()) {
    return &it->second;
  }

  if (line < text.size()) {
    // Line exists, but it's not in the cache

    const unsigned int op = text[line].Text.size();
    CacheLine_t CacheLine;
    unsigned int z;

    CacheLine.resize(op);

    // Add spaces to represent the starting column (if this horizontal line
    // was reached by a LF without a CR).
    unsigned int Column = StartingColumn(line);       
    if (Column > 0) {
      if (op == 0) {
        CacheLine.resize(1);
      }
      CacheLine[0].Characters = std::string(Column, ' ');
      // Since they're spaces, don't worry about their colour
      CacheLine[0].AltColour  = std::vector<bool>(Column, false);
    }
    
    // Iterate over all of the lines of text (that overprint one another)
    // on this spatial line.
    for (z = 0; z < op; z++) {
      ConvertSingleLine(line, z, CacheLine[z]);
    }

    // Copy the line into the cache map, and return a pointer to
    // the copy now in the map
    cache[line] = CacheLine;
    it = cache.find(line);
    return &it->second;
  }

  return 0;
}

void PrintedPaper::DrawTextLine(wxPaintDC &dc, unsigned int l, unsigned int c,
                                unsigned int w, unsigned int x, unsigned int y)
{
  const CacheLine_t *cl = Cached(l);
  char cs[MAX_CARRIAGE_WIDTH+1];
  
  unsigned int overstrikes = cl->size();
  unsigned int z;
  
  if ((c >= carriage_width) || (w == 0)) {
    return; // Nothing to do...
  }
  
  if ((c+w) > carriage_width) {
    w = carriage_width - c;
  }
  
  
  //std::cout << "DrawTextLine(.., " << l << ", " << c << ", " << w
  //          << ", " << x << ", " << y << ")" << std::endl;
  
  dc.SetBackgroundMode(wxTRANSPARENT);
  for (z=0; z<overstrikes; z++) {
    const Printable &p((*cl)[z]);
    int cx = x;
    unsigned int j = 0;
    unsigned int len = p.Characters.size();
    bool AltColour = (p.AltColour.empty()) ? false : p.AltColour[c];
    
    while ((j<w) && ((j+c) < len)) {
      unsigned int k = 0;
      /* Extract a substring.
       *
       * Note that a C string is deliberately used here guaranteeing
       * that, when the wxString is built from it, the current
       * locale is not used to interpret the string.
       */
      while ((j<w) && ((j+c) < len)) {
        if (p.AltColour[j+c] != AltColour) {
          break; // Colour changed
        }
        cs[k++] = p.Characters[c + (j++)];
      }
      cs[k] = '\0';
    
      wxString ws(wxString::FromAscii(cs));
    
      //std::cout << "DrawTextLine() ws=<" << ws << ">" << std::endl;

      dc.SetTextForeground((AltColour) ? *wxRED : *wxBLACK);
      dc.DrawText(ws, cx, y);
      cx += (k * font_width);
      AltColour = p.AltColour[c+j];
    }
    
    if ((c+w) >= carriage_width) {
      /* The last character printable on the carriage has just been
       * printed. If more characters remain then these overprint
       * the last one.
       */
      int lx = x + ((w - 1) * font_width);
      for (j = carriage_width; j < len; j++) {
        cs[0] = p.Characters[j];
        cs[1] = '\0';
        AltColour = p.AltColour[j];
        wxString ws(wxString::FromAscii(cs));
        dc.SetTextForeground((AltColour) ? *wxRED : *wxBLACK);
        dc.DrawText(ws, lx, y);
      }
    }
  }
}

void PrintedPaper::OnPaint(wxPaintEvent &WXUNUSED(event))
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;

  int view_start_x, view_start_y;
  GetViewStart(&view_start_x, &view_start_y);

  unsigned int lines = text.size();
  
  /*
    std::cout << "view_start_x = " << view_start_x
    << " view_start_y = " << view_start_y << std::endl
    << "left_offset = " << left_offset
    << " left_margin = " << left_margin << std::endl;
  */
  //std::cout << "font_width = " << font_width << std::endl;

  unsigned int vX,vY,vW,vH; // Dimensions of client area in characters
  wxRegionIterator upd(GetUpdateRegion()); // get the update rect list
  while (upd) {
    vX = (view_start_x * font_width) + upd.GetX();
    vY = (view_start_y * font_height) + upd.GetY();

    vW = (vX + upd.GetW() + font_width - 1) / font_width;
    vH = (vY + upd.GetH() + font_height - 1) / font_height;

    vX /= font_width;
    vY /= font_height;
    vW -= vX;
    vH -= vY;

    /*
    std::cout << "GetX = " << upd.GetX()
              << " GetW = " << upd.GetW() << std::endl;
    std::cout << "vX = " << vX
              << " vY = " << vY
              << " vW = " << vW
              << " vH = " << vH << std::endl;
    */
    
    wxPaintDC dc(this);
    PrepareDC(dc);
    dc.SetFont(font);

    unsigned int l;
    for (l = vY; l < (vY + vH); l++) {
      
      /*
       * Draw columns from vX to (vX+vW-1)
       */
      
      dc.Blit(vX * font_width, l*font_height, vW * font_width, font_height,
              paper, vX * font_width, 0);
      
      if ((l >= top_offset) && ((l-top_offset) < lines)) {
        /*
        std::cout << "l = " << l
                  << "vX = " << vX
                  << " vY = " << vY
                  << " vW = " << vW
                  << " vH = " << vH << std::endl;
        
        std::cout << "left_margin = " << left_margin
                  << " left_offset = " << left_offset << std::endl;
        */
        int text_start = vX - left_margin - left_offset;
        int text_width = vW;
        
        //std::cout << "text_start = " << text_start
        //          << " text_width = " << text_width << std::endl;
        if (text_start < 0) {
          text_width += text_start;
          text_start = 0;
        }
        
        //std::cout << "text_start = " << text_start
        //          << " text_width = " << text_width << std::endl;
        
        if (text_width > 0) {
          DrawTextLine(dc, l-top_offset, text_start, text_width,
                       ((text_start + left_offset + left_margin) * font_width),
                       (l*font_height));
        }
      }
    }
    upd ++ ;
  }
  
}

void PrintedPaper::InvalidateCacheLine(unsigned int line)
{
  Cache_t::iterator it = cache.find(line);

  if (it != cache.end()) {
    cache.erase(it);
  }
}

void PrintedPaper::UnsignedViewStart(unsigned int &vsx, unsigned int &vsy)
{
  int view_start_x, view_start_y;
    
  GetViewStart(&view_start_x, &view_start_y);

  vsx = (view_start_x < 0) ? 0 : view_start_x;
  vsy = (view_start_y < 0) ? 0 : view_start_y;
}

void PrintedPaper::DisplayLast()
{
  /*
   * An incremental update of the last cache line is probably
   * possible, but for now just brute-force this by discarding
   * the last line.
   */
  unsigned int current_line = (text.empty()) ? 0 : text.size() - 1;
  InvalidateCacheLine(current_line);

  unsigned int cursor_column = CursorColumn();
  unsigned int current_column = cursor_column;

  //std::cout << "cursor_column = " << cursor_column
  //          << " current_column = " << current_column << std::endl;
  
  if (current_column > 0) {
    current_column--;
  }
  cursor_column  += left_offset + left_margin + 1; // RH edge of cursor
  current_column += left_offset + left_margin;

  unsigned int view_start_x, view_start_y;
  bool changed = false;
  
  UnsignedViewStart(view_start_x, view_start_y);
  
  if (view_start_y > current_line) {
    view_start_y = current_line;
    changed = true;
  } else if ((view_start_y + fc_height) <= current_line) {
    view_start_y = current_line - fc_height + 1;
    changed = true;
  }
  
  if (view_start_x > current_column) {
    view_start_x = current_column;
    changed = true;
  } else if ((view_start_x + fc_width) <= cursor_column) {
      view_start_x = cursor_column - fc_width;
      changed = true;
    }
  
  if (changed) {
    Scroll(view_start_x, view_start_y);
  }

  Refresh();
}

void PrintedPaper::InternalPrint(unsigned char ch, bool update)
{
  unsigned int StartLines = text.size();
  unsigned int StartColumn = CursorColumn();
  bool store = false;
  ch &= 0x7f; // Lose any parity bits

  if ((ch >= ' ') && (ch < 0x7f)) {
    store = true;
  } else {
    switch (ch) {
    case '\t': store = true; break; // Assume some can do tabs
    case '\n': store = true; break;
    case '\r': store = true; break;
    case 0x0e: CurrentAltColour = true;  store = true; break;
    case 0x0f: CurrentAltColour = false; store = true; break;
    default: /* do nothing */ break;
    }
  }

  if (store) {
    //unsigned int current_line = (text.empty()) ? 0 : text.size() - 1;

    if (text.empty()) {
      if (ch == 'r') {      
        FirstLineColumn = 0;
      }
      TextLine tl;
      tl.AltColour = CurrentAltColour;
      tl.FollowsReturn = (FirstLineColumn == 0);
      text.push_back(tl);
    }

    if (ch == '\r') {
      /*
       * Move to a new line of overprinting text within the current
       * physical line. If the previous overprinting line is empty
       * (after a LF, for example) then just reuse it.
       */
      TextLine &tl(text.back());
      if ((tl.Text.empty()) ||
          ((tl.Text.size() == 1) && (tl.Text[0].empty()))) {
        tl.FollowsReturn = true;
      }
      if ((tl.Text.empty()) || (! tl.Text.back().empty())) {
        std::string str;
        tl.Text.push_back(str);
      }
    } else if (ch == '\n') {
      /*
       * Move to a new physical line of text.
       * If the last overprinting line of the previous physical line
       * is empty (after a CR) then delete it.
       */
      bool follows_return = false;
      TextLine &tl(text.back());
      if (! tl.Text.empty()) {
        std::string &str(tl.Text.back());
        if (str.empty()) {
          follows_return = true;
          tl.Text.pop_back();
        }
      }
      TextLine ntl;
      std::string str;
      ntl.AltColour = CurrentAltColour;
      ntl.FollowsReturn = follows_return;
      ntl.Text.push_back(str);
      text.push_back(ntl);
    } else {
      TextLine &tl(text.back());
      if (tl.Text.empty()) {
        std::string str(1,ch);
        tl.Text.push_back(str);
      } else {
        tl.Text.back().push_back(ch);
      }
    }

    if (update) {
      if (StartLines != text.size()) {
        // May need to turn on the vertical scrollbar...
        DecideScrollbars();
      }
      DisplayLast();
      inverted_cursor = false;
      
      unsigned int cursor_column = CursorColumn();
      chime = ((StartColumn   <  bell_position) &&
               (cursor_column >= bell_position));
      if (chime) {
        wxBell();
      }
    }
  }
}

void PrintedPaper::Print(std::string str)
{
  unsigned int i;
  for (i=0; i<str.size(); i++) {
    InternalPrint(str[i], false);
  }

  cache.clear();
  DecideScrollbars();
}

void PrintedPaper::OnKeyDown(wxKeyEvent& event)
{
  bool skip = true;
  wxChar ch = event.GetUnicodeKey();

  if (ch == WXK_NONE) {
    ch = event.GetKeyCode();
  }

  if (ch == WXK_PAUSE) {
    break_phase = BREAK_ACTIVE;
    skip = false;
  }

  event.Skip(skip);
}

void PrintedPaper::OnChar(wxKeyEvent& event)
{
  bool skip = true;
  
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  wxChar ch = event.GetUnicodeKey();

  if (ch == WXK_NONE) {
    ch = event.GetKeyCode();

    if (ch == WXK_F12) {
      TriggerAnswerBack();
    }
  }

  // Map an un-modified Backspace to line-feed
  if (ch == '\010') {
    if (event.GetModifiers() == wxMOD_NONE) {
      ch = '\n';
    }
  }
  
  std::cout << "ch = " << ch << std::endl;
  
  if (ch < 127) {
    skip = false;
    Print(ch);
  }

  event.Skip(skip);
}

void PrintedPaper::OnKeyUp(wxKeyEvent& event)
{
  bool skip = true;
  wxChar ch = event.GetUnicodeKey();

  if (ch == WXK_NONE) {
    ch = event.GetKeyCode();
  }

  if (ch == WXK_PAUSE) {
    break_phase = BREAK_RANDOM;
    skip = false;
  }

  event.Skip(skip);
}

void PrintedPaper::InvertCursor()
{
  wxClientDC dc(this);
  unsigned int view_start_x, view_start_y;
  
  UnsignedViewStart(view_start_x, view_start_y);

  unsigned int current_line = (text.empty()) ? 0 : text.size() - 1;
  unsigned int cursor_column = CursorColumn();
  
  current_line  += top_offset                - view_start_y;
  cursor_column += left_offset + left_margin - view_start_x;

  dc.Blit(cursor_column * font_width,
          current_line  * font_height,
          font_width,
          font_height,
          &dc, // No Source, but have to pass something...
          0,
          0,
          wxINVERT);
  
  inverted_cursor = !inverted_cursor;
}

void PrintedPaper::TriggerAnswerBack()
{
  AnswerBackCounter = 0;
}

void PrintedPaper::OnTimer(wxTimerEvent &event)
{
  if (have_focus &&
      ((TickCounter == 0) ||
       (TickCounter == (CHARACTERS_PER_SECOND/2)))) {
    InvertCursor();
  }

  if (TickCounter == 0) {
    TickCounter = CHARACTERS_PER_SECOND - 1;
  } else {
    TickCounter--;
  }

  if (break_phase == BREAK_ACTIVE) {
    std::cout << "ch = 0" << std::endl;
  } else if (break_phase == BREAK_RANDOM) {
    std::uniform_int_distribution<int> distribution(0,10);
    int bit = distribution(generator);
    int ch = 0xff;
    if (bit < 8) {
      ch = (ch << bit) & 0xff;
    }
    std::cout << "ch = " << ch << std::endl;
    break_phase = BREAK_NONE;
  } else if (AnswerBackCounter >= 0) {
    int ch = AnswerBackDrum[AnswerBackCounter];
    std::cout << "ch = " << ch << std::endl;    
  }

  // Out here in case both break and AnswerBack
  // running at the same time...
  if (AnswerBackCounter >= 0) {
    AnswerBackCounter++;
    if (AnswerBackCounter >= static_cast<int>(AnswerBackDrum.size())) {
      AnswerBackCounter = -1;
    }
  }
  
}

void PrintedPaper::OnSetFocus(wxFocusEvent &event)
{
  //std::cout<<__PRETTY_FUNCTION__<<std::endl;
  have_focus = true;
}

void PrintedPaper::OnKillFocus(wxFocusEvent &event)
{
  //std::cout<<__PRETTY_FUNCTION__<<std::endl;
  if (have_focus) {
    if (inverted_cursor) {
      InvertCursor();
    }
    have_focus = false;
  }
}

BEGIN_EVENT_TABLE(PrintedPaper, wxScrolledCanvas)
EVT_PAINT (PrintedPaper::OnPaint)
EVT_SIZE  (PrintedPaper::OnSize)
EVT_KEY_DOWN(PrintedPaper::OnKeyDown)
EVT_CHAR(PrintedPaper::OnChar)
EVT_KEY_UP(PrintedPaper::OnKeyUp)
EVT_TIMER(PrintedPaper::CHARACTER_TIMER_ID, PrintedPaper::OnTimer)
EVT_SET_FOCUS(PrintedPaper::OnSetFocus)
EVT_KILL_FOCUS(PrintedPaper::OnKillFocus)
END_EVENT_TABLE()
