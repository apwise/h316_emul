
class wxPaperTape: public wxScrolledWindow
{
public:

  enum wxPT_direction
    {
      wxPT_TopToBottom = 0,
      wxPT_LeftToRight = 0,
      wxPT_BottomToTop = 1,
      wxPT_RightToLeft = 1
    };

  wxPaperTape( wxWindow *parent,
               wxWindowID id = -1,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               bool reader = true,
               int orient = wxVERTICAL,
               enum wxPT_direction direction = wxPT_TopToBottom,
               bool mirror = false,
               const wxString& name = wxT("paperTape") );
  ~wxPaperTape( );

  void Load(const char *buffer, long size);
  void Rewind();
  int Read();

  void OnPaint(wxPaintEvent &event);
  void OnTimer(wxTimerEvent& event);

private:
  wxWindow *parent;
  bool reader;
  int orient;
  enum wxPT_direction direction;
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
  
  enum wxPT_type
    {
      wxPT_holes,
      wxPT_light,
      wxPT_leader,
      wxPT_no_feed,
      wxPT_no_tape,
      wxPT_lead_triangle,
      wxPT_tail_triangle,

      wxPT_num_types
    };
  
  int start_type[wxPT_num_types + 1];
  static int num_type[wxPT_num_types];

  wxTimer *timer;
  bool pending_refresh;

  void set_pending_refresh();

  void DestroyBitmaps();
  void AllocateBitmaps();
  wxBitmap *GetBitmap(int i, wxPT_type c);

  class TapeChunk;

  std::list<TapeChunk> chunks;
  long total_size();
  int get_element(int index, wxPT_type &type);

  long position, initial_position;
  void SetScrollBars();
  void SetScrollBarPosition();

  DECLARE_EVENT_TABLE()
};
