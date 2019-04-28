#include <wx/tglbtn.h>
#include <wx/stattext.h>
#include <wx/dcclient.h>

#include <cmath>
#include <iostream>

#include <list>

#include "teleprinter.hh"

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

wxTeleprinter::wxTeleprinter( wxWindow *parent,
                              wxWindowID id,
                              const wxPoint &pos,
                              const wxSize &size,
                              const wxString &name )
  : wxScrolledCanvas(parent, id, pos, size, 0, name)
  , parent(parent)
  , paper_width(85)
  , carriage_width(72)
  , font(wxFontInfo(14).FaceName("TeleType"))
  , font_width(GetCharWidth())
  , font_height(GetCharHeight())
  , paper(0)
  , paper_bitmap(0)
{

  // For testing...
  text.resize(4);
  text[0].resize(1);
  text[1].resize(1);
  text[2].resize(1);
  text[3].resize(2);
  text[0][0] = "HELLO CRUEL WORLD";
  text[1][0] = "0123456789_";
  text[2][0] = "01234567890123456789012345678901234567890123456789012345678901234567890123456789";
  text[3][0] = "+/";
  text[3][1] = "X\\";

  DecideScrollbars(true);
}

wxTeleprinter::~wxTeleprinter()
{
}

void wxTeleprinter::DrawPaper(int width, int height)
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
  
void wxTeleprinter::DecideScrollbars(bool init)
{
  wxClientDC dc(this);
  PrepareDC(dc);
  dc.SetFont(font);

  font_width  = dc.GetCharWidth();
  font_height = dc.GetCharHeight();

  std::cout << "font_width = " << font_width << std::endl;
  std::cout << "font_height = " << font_height << std::endl;
  
  int width, height;
  unsigned int cwidth, cheight; // Size in characters
  unsigned int lines;

  GetClientSize(&width, &height);
  std::cout << "width = " << width << std::endl;
  std::cout << "height = " << height << std::endl;

  cwidth  = (width + font_width - 1)  / font_width;
  cheight = (height + font_height - 1) / font_height;

  left_offset = 0;
  if (cwidth > paper_width) {
    left_offset = ((cwidth-paper_width)/2);
  }

  lines = text.size();

  unsigned int left_margin = (paper_width - carriage_width) / 2;

  SetScrollbars(font_width, font_height, paper_width, lines,
                left_margin, 0);
  ShowScrollbars(((cwidth  < paper_width) ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER),
                 ((cheight < lines      ) ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER));
 
  DrawPaper(((cwidth < (paper_width+1)) ? (paper_width+1) : cwidth) * font_width, font_height);
}

void wxTeleprinter::OnSize(wxSizeEvent &WXUNUSED(event))
{
  DecideScrollbars();
}

const wxTeleprinter::CacheLine_t *wxTeleprinter::Cached(unsigned int line)
{
  Cache_t::iterator it;

  std::cout << "Cached("<<line<<")"<<std::endl;
  
  it = cache.find(line);

  if (it != cache.end()) {
    return &it->second;
  }

  if (line < text.size()) {
    // Line exists, but it's not in the cache
    // Iterate over all of the lines of text (that overprint one another)
    // on this spatial line.
    const unsigned int op = text[line].size();
    unsigned int i = 0;
    
    CacheLine_t CacheLine;
    CacheLine.resize(op);
    
    for (i = 0; i < op; i++) {
      // Iterate over the characters in the line
      unsigned int j = 0;
      for (j = 0; j < text[line][i].size(); j++) {
        unsigned int ch = text[line][i][j] & 0x7f;
        bool ac = false;
        bool p = false;
        
        if (ch < ' ') {
          // Handle control characters
          // (None on ASR-33)
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
          CacheLine[i].Characters.push_back(ch);
          CacheLine[i].AltColour.push_back(ac);
        }
      }
    }

    // Copy the line into the cache map, and return a pointer to
    // the copy now in the map
    cache[line] = CacheLine;
      it = cache.find(line);
      return &it->second;
  }

    return 0;
}

void wxTeleprinter::DrawText(wxPaintDC &dc, int l, int c, int w, int x, int y)
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
  
  std::cout << "DrawText(.., " << l << ", " << c << ", " << w
            << ", " << x << ", " << y << ")" << std::endl;
  
  dc.SetBackgroundMode(wxTRANSPARENT);
  //dc.SetBackgroundMode(wxSOLID);
  for (z=0; z<overstrikes; z++) {
    const Printable &p((*cl)[z]);
    unsigned int j;
    unsigned int len = p.Characters.size();
    
    /* Extract a substring.
     *
     * Note that a C string is deliberately used here so that
     * a wxString can then be built, guaranteeing that the current
     * locale is not used to interpret the string.
     */
    for (j=0; ((j<uw) && ((j+c) < len)); j++) {
      cs[j] = p.Characters[j+c];
    }
    cs[j] = '\0';
    
    wxString ws = wxString::FromAscii(cs);
    
    std::cout << "DrawText() ws=<" << ws << ">" << std::endl;
    
    dc.DrawText(ws,x,y);
    
    //if (z==0) {
    //  dc.SetBackgroundMode(wxTRANSPARENT);
    //}
    
    if ((c+uw) >= carriage_width) {
      /* The last character printable on the carriage has just been
       * printed. If more characters remain then these overprint
       * the last one.
       */
      int lx = x + ((w-1)*font_width);
      for (j = carriage_width; j < len; j++) {
        cs[0] = p.Characters[j];
        cs[1] = '\0';
        ws = wxString::FromAscii(cs);
        dc.DrawText(ws,lx,y);
      }
    }
  }
}

void wxTeleprinter::OnPaint(wxPaintEvent &WXUNUSED(event))
{
  int view_start_x, view_start_y;
  unsigned int left_margin = (paper_width - carriage_width) / 2;

  GetViewStart(&view_start_x, &view_start_y);
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

    std::cout << "GetX = " << upd.GetX()
              << " GetW = " << upd.GetW() << std::endl;
    std::cout << "vX = " << vX
              << " vY = " << vY
              << " vW = " << vW
              << " vH = " << vH << std::endl;
    
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
      //dc.Blit(0, l*font_height, upd.GetW(), font_height,
      //       paper, 0, 0);
      
      if (l < ((int) text.size())) {
        std::cout << "l = " << l << std::endl;
        
        int text_start = vX - left_margin - left_offset;
        int text_width = vW;
        
        if (text_start < 0) {
          text_width += text_start;
          text_start = 0;
        }
        
        std::cout << "text_start = " << text_start
                  << " text_width = " << text_width << std::endl;
        
        if (text_width > 0) {
          DrawText(dc, l, text_start, text_width,
                   ((text_start + left_offset + left_margin) * font_width),
                   (l*font_height));
        }
      }
    }
    upd ++ ;
  }
  
}

BEGIN_EVENT_TABLE(wxTeleprinter, wxScrolledCanvas)
EVT_PAINT (wxTeleprinter::OnPaint)
EVT_SIZE  (wxTeleprinter::OnSize)
END_EVENT_TABLE()
