#ifndef __TELEPRINTER_HH__
#define __TELEPRINTER_HH__

#include <wx/wx.h> 
#include <vector> 
#include <map> 
#include <string> 

class wxTeleprinter: public wxScrolledCanvas
{
public:

  wxTeleprinter( wxWindow *parent,
                 wxWindowID id = -1,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 const wxString& name = wxT("teleprinter") );
  ~wxTeleprinter( );

private:
  wxWindow *parent;

  static const unsigned int MAX_CARRIAGE_WIDTH = 150;
  
  unsigned int paper_width; // Paper width in characters
  unsigned int carriage_width; // Carriage width in characters
  unsigned int left_offset; // Window edge to start of paper
  
  wxFont font;
  wxCoord font_width, font_height;
  
  typedef std::vector< std::string > TextLine_t;
  typedef std::vector< TextLine_t > Text_t;
  typedef std::vector< bool > AltColour_t;

  struct Printable {
    std::string Characters;
    AltColour_t AltColour;
  };
  typedef std::vector< Printable > CacheLine_t;
  typedef std::map< unsigned int, CacheLine_t > Cache_t;
  
  Text_t text;
  Cache_t cache;

  wxMemoryDC *paper;
  wxBitmap *paper_bitmap;
  
  void OnPaint(wxPaintEvent &event);
  void OnSize(wxSizeEvent &event);
  void DecideScrollbars(bool init = false);
  void DrawText(wxPaintDC &dc, int l, int c, int w, int x, int y);
  const CacheLine_t *Cached(unsigned int line);
  void DrawPaper(int width, int height);

  DECLARE_EVENT_TABLE()
};

#endif // __TELEPRINTER_HH__
