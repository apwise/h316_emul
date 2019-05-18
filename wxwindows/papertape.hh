#ifndef __PAPERTAPE_HH__
#define __PAPERTAPE_HH__

#include <list>
#include <wx/wx.h> 

class PaperTape: public wxScrolledCanvas
{
public:
  
  enum PT_direction {
    PT_TopToBottom = 0,
    PT_LeftToRight = 0,
    PT_BottomToTop = 1,
    PT_RightToLeft = 1
  };
  
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
  
  void Load(const char *buffer, long size);
  void Rewind();
  int Read();
  
private:
  wxWindow *parent;
  bool reader;
  int orient;
  enum PT_direction direction;
  bool mirror;
  
  int current_tape_width;
  int row_spacing;
  
  wxBitmap **bitmaps;

  int black_start;
  int black_end;
  
  void DrawCircle(wxDC &dc,
                  double xc, double yc, double r,
                  wxColour &background, wxColour &foreground);
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
  
  int start_type[PT_num_types + 1];
  static const int num_type[PT_num_types];

  wxTimer *timer;
  bool pending_refresh;
  
  void set_pending_refresh();
  
  void DestroyBitmaps();
  void AllocateBitmaps();
  wxBitmap *GetBitmap(int i, PT_type c);
  
  class TapeChunk;
  
  std::list<TapeChunk> chunks;
  long total_size();
  int get_element(int index, PT_type &type);
  
  long position, initial_position;
  void SetScrollBars();
  void SetScrollBarPosition();

  void OnPaint(wxPaintEvent &event);
  void OnTimer(wxTimerEvent& event);

  DECLARE_EVENT_TABLE()
};

#endif // __PAPERTAPE_HH__
