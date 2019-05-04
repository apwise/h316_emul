#ifndef __PRINTEDPAPER_HH__
#define __PRINTEDPAPER_HH__

#include <wx/wx.h> 
#include <vector> 
#include <map> 
#include <string> 

class PrintedPaper: public wxScrolledCanvas
{
public:

  PrintedPaper( wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                const wxString& name = wxT("teleprinter") );
  ~PrintedPaper( );
  void Print(unsigned char ch){InternalPrint(ch);}
  void Print(std::string str);
private:
  wxWindow *parent;

  static const unsigned int MAX_CARRIAGE_WIDTH = 150;
  
  unsigned int paper_width; // Paper width in characters
  unsigned int carriage_width; // Carriage width in characters
  unsigned int left_offset; // Window edge to start of paper
  
  wxFont font;
  wxCoord font_width, font_height;

  /*
   * A vector of strings - lines that overprint one another,
   * note that the first of these does not necessarily start
   * at the LHS of the paper (it only does if FollowsReturn
   * is true).
   */
  typedef std::vector< std::string > TextLine_t;
  struct TextLine {
    bool AltColour;     // Whether starts in alternate colour 
    bool FollowsReturn; // Whether starts at LHS of paper
    TextLine_t Text;    // Storage of text
  };
  typedef std::vector< TextLine > Text_t;

  Text_t text;   // Storage of text
  unsigned int FirstLineColumn; // Start column of very first line

  typedef std::vector< bool > AltColour_t;
  struct Printable {
    std::string Characters;
    AltColour_t AltColour;
  };
  typedef std::vector< Printable > CacheLine_t;
  typedef std::map< unsigned int, CacheLine_t > Cache_t;

  Cache_t cache; // Cache of processed text ready for drawing

  wxMemoryDC *paper;
  wxBitmap *paper_bitmap;

  bool newline_pending;
  bool return_pending;
  bool current_colour;
  
  void OnPaint(wxPaintEvent &event);
  void OnSize(wxSizeEvent &event);
  void DecideScrollbars(bool init = false);
  void DrawTextLine(wxPaintDC &dc, int l, int c, int w, int x, int y);
  const CacheLine_t *Cached(unsigned int line);
  void InvalidateCacheLine(unsigned int line);
  void DrawPaper(int width, int height);
  void DisplayLast();
  void InternalPrint(unsigned char ch, bool update = true);
  void ConvertSingleLine(unsigned int line, unsigned int z, Printable &pr);
  unsigned int StartingColumn(unsigned int line);
  unsigned int CurrentColumn();

  DECLARE_EVENT_TABLE()
};

#endif // __PRINTEDPAPER_HH__
