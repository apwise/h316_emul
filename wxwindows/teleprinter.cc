#include <wx/tglbtn.h>
#include <wx/stattext.h>
#include <wx/dcclient.h>

#include <cmath>
#include <iostream>

#include <list>

#include "teleprinter.hh"

enum CtrlChars {
  NUL = 0x00, // null character
  SOH,        // start of heading
  STX,        // start of text
  ETX,        // end of text
  EOT,        // end of transmission
  ENQ,        // enquiry
  ACK,        // acknowledge
  BEL,        // bell
  BS,         // backspace
  HT,         // horizontal tab
  LF,         // new line
  VT,         // vertical tab
  FF,         // form feed
  CR,         // carriage ret
  SO,         // shift out
  SI,         // shift in
  DLE,        // data link escape
  DC1,        // device control 1
  DC2,        // device control 2
  DC3,        // device control 3
  DC4,        // device control 4
  NAK,        // negative ack.
  SYN,        // synchronous idle
  ETB,        // end of trans. blk
  CAN,        // cancel
  EM,         // end of medium
  SUB,        // substitute
  ESC,        // escape
  FS,         // file separator
  GS,         // group separator
  RS,         // record separator
  US,         // unit separator
  SPC,        // space
  DEL = 0x7f, // delet

  // ASR-specific aliases
  WRU  = ENQ, // Who are you (answer-back drum)
  XON  = DC1, // Start paper-tape reader
  TAPE = DC2, // ASR-35 Start paper-tape punch
  XOFF = DC3, // (When read from papertape) stop the reader
  //             ASR-35 Stop the paper-tape punch
  RUBOUT = DEL
};

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

Teleprinter::Teleprinter( wxWindow *parent,
                          wxWindowID id,
                          const wxPoint &pos,
                          const wxSize &size,
                          const wxString &name )
  : wxScrolledCanvas(parent, id, pos, size, 0, name)
  , parent(parent)
  , paper_width(85)
  , carriage_width(72)
  , font(wxFontInfo(12).FaceName("TeleType"))
  , font_width(GetCharWidth())
  , font_height(GetCharHeight())
  , FirstLineColumn(0)
  , paper(0)
  , paper_bitmap(0)
  , newline_pending(true)
  , return_pending(true)
  , current_colour(false)
{  
  DecideScrollbars(true);
}

Teleprinter::~Teleprinter()
{
}

void Teleprinter::DrawPaper(int width, int height)
{
  int x, y;
  unsigned int cwidth, left_offset;
  
  wxBitmap bitmap(width, height);

  if (paper) {
    delete paper;
  }

  if (paper_bitmap) {
    delete paper_bitmap;
  }
  
  paper_bitmap = new wxBitmap(width, height);
  paper = new wxMemoryDC(bitmap);

  cwidth  = (width + font_width - 1)  / font_width;

  left_offset = 0;
  if (cwidth > paper_width) {
    left_offset = ((cwidth-paper_width)/2);
  }

  wxColour EdgeColour(0xe6, 0xc7, 0x9b);
  wxColour CentreColour(0xf7, 0xe7, 0xcd);
  
  for (x=0; x<width; x++) {
    // Which text character is this pixel in?
    const unsigned int c = x / font_width;
    wxColour colour;
    
    if (c < left_offset) {
      colour.Set(0,0,0);
    } else if (c < (left_offset + paper_width)) {

      int p = x-(left_offset * font_width);
      double theta = static_cast<double>(p) * 2.0 * M_PI / static_cast<double>((paper_width * font_width)-1);
      double scale = 0.5 + 0.5*cos(theta);

      colour.Set(((EdgeColour.Red()   * scale) + (CentreColour.Red()   * (1.0-scale))),
                 ((EdgeColour.Green() * scale) + (CentreColour.Green() * (1.0-scale))),
                 ((EdgeColour.Blue()  * scale) + (CentreColour.Blue()  * (1.0-scale))));
    } else {
      colour.Set(0,0,0);
    }

    wxPen pen(colour);
    paper->SetPen(pen);
    
    for (y=0; y<height; y++) {
      paper->DrawPoint(x, y);
    }
  }
  
}
  
void Teleprinter::DecideScrollbars(bool init)
{
  wxClientDC dc(this);
  PrepareDC(dc);
  dc.SetFont(font);

  font_width  = dc.GetCharWidth();
  font_height = dc.GetCharHeight();

  //std::cout << "font_width = " << font_width << std::endl;
  //std::cout << "font_height = " << font_height << std::endl;
  
  int width, height;
  unsigned int cwidth, cheight; // Size in characters

  GetClientSize(&width, &height);
  //std::cout << "width = " << width << std::endl;
  //std::cout << "height = " << height << std::endl;

  cwidth  = (width + font_width - 1)  / font_width;
  cheight = (height + font_height - 1) / font_height;

  left_offset = 0;
  if (cwidth > paper_width) {
    left_offset = ((cwidth-paper_width)/2);
  }

  unsigned int left_margin = (paper_width - carriage_width) / 2;
  unsigned int lines = text.size();
  unsigned int line_width = CurrentColumn() + 1 + left_margin;
  
  unsigned int fcwidth, fcheight;
  fcwidth  = width  / font_width;
  fcheight = height / font_height;

  //std::cout << "line_width = " << line_width << " fcwidth = " << fcwidth << std::endl;
  
  SetScrollbars(font_width, font_height, paper_width-1, lines,
                ((line_width <fcwidth) ? 0 : (line_width-fcwidth)),
                ((text.size()<fcheight) ? 0 : (text.size()-fcheight)));
  ShowScrollbars(((cwidth  < paper_width) ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER),
                 ((cheight < lines      ) ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER));
 
  DrawPaper(((cwidth < (paper_width+1)) ? (paper_width+1) : cwidth) * font_width, font_height);
}

void Teleprinter::OnSize(wxSizeEvent &WXUNUSED(event))
{
  DecideScrollbars();
}

void Teleprinter::ConvertSingleLine(unsigned int line, unsigned int z, Printable &pr)
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

unsigned int Teleprinter::StartingColumn(unsigned int line)
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

unsigned int Teleprinter::CurrentColumn()
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
    if (Column > 0) {
      Column--;
    }
  }
  return Column;
} 
 
const Teleprinter::CacheLine_t *Teleprinter::Cached(unsigned int line)
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

void Teleprinter::DrawTextLine(wxPaintDC &dc, int l, int c, int w, int x, int y)
{
  const CacheLine_t *cl = Cached(l);
  char cs[MAX_CARRIAGE_WIDTH+1];
  
  unsigned int overstrikes = cl->size();
  unsigned int z;
  
  if ((static_cast<unsigned int>(c) >= carriage_width) || (w < 1)) {
    return; // Nothing to do...
  }
  
  if (static_cast<unsigned int>(c+w) > carriage_width) {
    w = carriage_width - c;
  }
  
  const unsigned int uw(w);
  
  //std::cout << "DrawTextLine(.., " << l << ", " << c << ", " << w
  //          << ", " << x << ", " << y << ")" << std::endl;
  
  dc.SetBackgroundMode(wxTRANSPARENT);
  for (z=0; z<overstrikes; z++) {
    const Printable &p((*cl)[z]);
    int cx = x;
    unsigned int j = 0;
    unsigned int len = p.Characters.size();
    bool AltColour = p.AltColour[c];
    
    while ((j<uw) && ((j+c) < len)) {
      unsigned int k = 0;
      /* Extract a substring.
       *
       * Note that a C string is deliberately used here guaranteeing
       * that, when the wxString is built from it, the current
       * locale is not used to interpret the string.
       */
      while ((j<uw) && ((j+c) < len)) {
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
    
    if ((c+uw) >= carriage_width) {
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

void Teleprinter::OnPaint(wxPaintEvent &WXUNUSED(event))
{
  int view_start_x, view_start_y;
  unsigned int left_margin = (paper_width - carriage_width) / 2;

  GetViewStart(&view_start_x, &view_start_y);

  int width, height;
  int cheight; // Size in characters
  int lines = text.size();
  int top_offset;
  
  GetClientSize(&width, &height);
  // Here want the number of full lines that can be drawn
  cheight = height / font_height;

  top_offset = 0;
  if (lines < cheight) {
    top_offset = cheight - lines;
  }
  //std::cout << "top_offset = " << top_offset << std::endl;
  
  /*
    std::cout << "view_start_x = " << view_start_x
    << " view_start_y = " << view_start_y << std::endl
    << "left_offset = " << left_offset
    << " left_margin = " << left_margin << std::endl;
  */     
  int vX,vY,vW,vH; // Dimensions of client area in characters
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

    int l;
    for (l = vY; l < (vY + vH); l++) {
      
      /*
       * Draw columns from vX to (vX+vW-1)
       */
      
      dc.Blit(vX * font_width, l*font_height, vW * font_width, font_height,
              paper, vX * font_width, 0);
      
      if ((l >= top_offset) && ((l-top_offset) < lines)) {
        //std::cout << "l = " << l << std::endl;
        
        int text_start = vX - left_margin - left_offset;
        int text_width = vW;
        
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

void Teleprinter::InvalidateCacheLine(unsigned int line)
{
  Cache_t::iterator it = cache.find(line);

  if (it != cache.end()) {
    cache.erase(it);
  }
}

void Teleprinter::DisplayLast()
{
  unsigned int current_line = (text.empty()) ? 0 : text.size() - 1;
  /*
   * An incremetal update of the last cache line is probably
   * possible, but for now just brute-force this by discarding
   * the last line.
   */
  InvalidateCacheLine(current_line);
  const CacheLine_t *cl = Cached(current_line);
  if (cl) {
    const CacheLine_t &c(*cl);
    unsigned int z = (c.empty()) ? 0 : (c.size() - 1);
    unsigned int current_column = (c[z].Characters.empty()) ? 0 : c[z].Characters.size() - 1;

    int view_start_x, view_start_y;
    int width, height, cwidth, cheight;
    bool changed = false;
    unsigned int left_margin = (paper_width - carriage_width) / 2;
    
    GetViewStart(&view_start_x, &view_start_y);
    GetClientSize(&width, &height);
    cwidth  = width  / font_width;
    cheight = height / font_height;

    if (view_start_y > static_cast<int>(current_line)) {
      view_start_y = current_line;
      changed = true;
    } else if ((view_start_y + cheight) <= static_cast<int>(current_line)) {
      view_start_y = current_line - cheight + 1;
      changed = true;
    }

    current_column += left_margin;
    if (view_start_x > static_cast<int>(current_column)) {
      view_start_x = current_column;
      changed = true;
    } else if ((view_start_x + cwidth) <= static_cast<int>(current_column)) {
      view_start_x = current_column - cwidth + 1;
      changed = true;
    }

    if (changed) {
      Scroll(view_start_x, view_start_y);
    }
  }
  Refresh();
}

void Teleprinter::InternalPrint(unsigned char ch, bool update)
{
  unsigned int StartLines = text.size();
  bool store = false;
  ch &= 0x7f; // Lose any parity bits

  if ((ch >= ' ') && (ch < 0x7f)) {
    store = true;
  } else {
    switch (ch) {
    case '\t': store = true; break; // Assume some can do tabs
    case '\n': store = newline_pending; newline_pending = true; break;
    case '\r': return_pending = true;  break;
    case 0x0e: current_colour = true;  store = !newline_pending; break;
    case 0x0f: current_colour = false; store = !newline_pending; break;
    default: /* do nothing */ break;
    }
  }
  
  if (store) {
    unsigned int current_line = (text.empty()) ? 0 : text.size() - 1;
    if (newline_pending) {
      TextLine tl;
      tl.Text.resize(1);
      if (ch != '\n') {
        tl.Text[0].push_back(ch);
        newline_pending = false;
      }
      tl.AltColour = current_colour;
      tl.FollowsReturn = return_pending;
      text.push_back(tl);
      return_pending = false;
    } else if (return_pending) {
      std::string str(1,ch);
      text[current_line].Text.push_back(str);
      return_pending = false;
    } else {
      unsigned int current_z = ((text.empty()) ? 0 :
                                ((text[current_line].Text.empty()) ? 0 :
                                 text[current_line].Text.size() - 1));
      text[current_line].Text[current_z].push_back(ch);
    }

    if (update) {
      if (StartLines == text.size()) {
        DisplayLast();
      } else {
        // May need to turn on the vertical scrollbar...
        DecideScrollbars();
      }
    }
  }
}

void Teleprinter::Print(std::string str)
{
  unsigned int i;
  for (i=0; i<str.size(); i++) {
    InternalPrint(str[i], false);
  }

  cache.clear();
  DecideScrollbars();
}

BEGIN_EVENT_TABLE(Teleprinter, wxScrolledCanvas)
EVT_PAINT (Teleprinter::OnPaint)
EVT_SIZE  (Teleprinter::OnSize)
END_EVENT_TABLE()
