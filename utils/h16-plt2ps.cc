/* Convert plot file from h316 emulator to postscript
 *
 * Copyright (C) 2008  Adrian Wise
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

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <list>

struct PlotterModel;
struct Media;

class PlotFile {

public:
  PlotFile();
  ~PlotFile();

  void readfile(std::istream &ins, bool ascii_file);
  void preprocess(bool scale_flag,
                  bool keep_flag,
                  bool force_portrait,
                  bool force_landscape,
                  const PlotterModel *plotter_model,
                  const Media *media);
  void headers(std::ostream &outs);
  void data(std::ostream &outs);

private:
  enum PLT_DIRN {
    PD_NULL = 000,
    PD_N  = 004,
    PD_NE = 005,
    PD_E  = 001,
    PD_SE = 011,
    PD_S  = 010,
    PD_SW = 012,
    PD_W  = 002,
    PD_NW = 006,
    PD_UP = 016,
    PD_DN = 014,
  
    PD_NUM
  };

  static const char *pd_names[16];
  
  int x_pos, y_pos;
  bool pen;

  int x_offset, y_offset;
  int x_range, y_range;

  class Segment {
  public:
    Segment(PLT_DIRN d, int c);

    void direction(PLT_DIRN d) {Direction = d;};
    PLT_DIRN direction() const {return Direction;};

    void count(int c) {Count = c;};
    int count() const {return Count;};

  private:
    PLT_DIRN Direction;
    int Count;
  };

  std::list<Segment> segments;
  void apply_segment(const Segment &seg);
  std::string translate(int x, int y);
};

const char *PlotFile::pd_names[16] = {
  "(null)", "E",       "W",  "(error)",
  "N",      "NE",      "NW", "(error)",
  "S",      "SE",      "SW", "(error)",
  "DN",     "(error)", "UP", "(error)" 
};

struct PlotterModel {
  const char **names;
  bool metric;
  int step; // In 0.1mm or mil units
  int paper_width; // In 0.1mm or mil units
  int limit_width; // In 0.1mm or mil units
};

struct Media {
  const char *name;
  int x;
  int y;
};

PlotFile::Segment::Segment(PLT_DIRN d, int c)
  : Direction(d),
    Count(c)
{
}

PlotFile::PlotFile()  
  : x_pos(0),
    y_pos(0),
    pen(false)
{
}

PlotFile::~PlotFile()
{
}

void PlotFile::readfile(std::istream &ins, bool ascii_file)
{
  PLT_DIRN current_direction;
  long current_count;

  while (ins) {
    current_direction = PD_NULL;
    current_count = 0;

    if (ascii_file) {
      char buf[100];
      int d;
      char *cp, *cp2;

      ins.getline(buf, 100);
      //std::cout << "Current line <" << buf << ">" << std::endl;

      if ((ins) && (strlen(buf) > 0)) {
        for (d = 15; d >= 0; d--) { // Note two-character cases first

          if ((pd_names[d][0] != '(') &&
              (strncmp(pd_names[d], buf, strlen(pd_names[d])) == 0)) {
            current_direction = (PLT_DIRN)(d);

            cp = &buf[strlen(pd_names[d])];
            while ((*cp) && isspace(*cp) && (*cp != '\n'))
              cp++;

            if ((*cp) && (*cp != '\n')) {
              // Is there a number
              current_count = strtol(cp, &cp2, 0);
              if (cp2 <= cp)
                current_direction = PD_NULL;
            }

            break;
          }
        }

        if (current_direction == PD_NULL) {
          std::cerr << "Could not parse line <" << buf << ">" << std::endl;
          exit(1);
        }
      }

    } else {
      // Binary file reader...
      
      int c = ins.get();
        
      if (ins) {
        while ((ins) && ((c & 0200) != 0)) {
          // This is a prefix
          printf("Prefix = %02x\n", c);
          current_count = (current_count << 7) | (c & 0177);
          c = ins.get();
        }

        if (ins) {
          // This is the actual command byte
          current_count     = (current_count << 3) | (c & 0007);
          current_direction = (PLT_DIRN) ((c >> 3) & 0017);
        } else {
          std::cerr << "Unexpected end of file between prefix and command" << std::endl;
          exit(2);
        }
      }
    }

    if (current_direction != PD_NULL) {
      Segment segment(current_direction, current_count);
      segments.push_back(segment);
    }

  }

}

void PlotFile::apply_segment(const Segment &seg)
{
  PLT_DIRN d = seg.direction();
  int c = seg.count() + 1;
  
  switch(d) {
  case PD_N:  y_pos+=c;           break;
  case PD_NE: y_pos+=c; x_pos+=c; break;
  case PD_E:            x_pos+=c; break;
  case PD_SE: y_pos-=c; x_pos+=c; break;
  case PD_S:  y_pos-=c;           break;
  case PD_SW: y_pos-=c; x_pos-=c; break;
  case PD_W:            x_pos-=c; break;
  case PD_NW: y_pos+=c; x_pos-=c; break;
  case PD_UP: pen = false; break;
  case PD_DN: pen = true;  break;
  default:
    abort();
  }
}

std::string PlotFile::translate(int x, int y)
{
  std::ostringstream ost;
  int tx, ty;
  tx = x + x_offset;
  ty = y + y_offset;
  ost << x << " " << y;
  return ost.str();
}

void PlotFile::preprocess(bool scale_flag,
                          bool keep_flag,
                          bool force_portrait,
                          bool force_landscape,
                          const PlotterModel *plotter_model,
                          const Media *media)
{
  std::list<Segment>::const_iterator i;

  x_pos = 0; y_pos = 0; pen = false;
  int x_max = 0;
  int x_min = 0;
  int y_max = 0;
  int y_min = 0;

  int ix_max = 0;
  int ix_min = 0;
  int iy_max = 0;
  int iy_min = 0;

  for (i = segments.begin(); i != segments.end(); i++) {
    apply_segment(*i);

    if (x_pos > x_max) x_max = x_pos;
    if (x_pos < x_min) x_min = x_pos;
    if (y_pos > y_max) y_max = y_pos;
    if (y_pos < y_min) y_min = y_pos;
    
    if (pen) {
      if (x_pos > ix_max) ix_max = x_pos;
      if (x_pos < ix_min) ix_min = x_pos;
      if (y_pos > iy_max) iy_max = y_pos;
      if (y_pos < iy_min) iy_min = y_pos;
    }
  }

  ix_range = 1 + ix_max - ix_min;
  iy_range = 1 + iy_max - iy_min;

  bool landscape = false;
  if (force_portrait)
    landscape = false;
  else if (force_landscape)
    landscape = true;
  else {
    //
    // Look at the width and height of the image
    // to see whether we'd choose portrait or
    // landscape.
    //
    landscape = (ix_range > iyrange);
  }

  x_offset = -x_min;
  x_range = x_offset + x_max + 1;
  y_offset = -y_min;
  y_range = y_offset + y_max + 1;

  std::cout << "offset = (" << x_offset << ", " << y_offset << ")" << std::endl;
  std::cout << "range = (" << x_range << ", " << y_range << ")" << std::endl;
}

void PlotFile::headers(std::ostream &outs)
{
  outs << "%!" << std::endl;

  //double scale = 842.0 / 3400.0;
  double scale = 595.0 / 3400.0;

  //outs << "270 rotate" << std::endl;
  outs << scale << " " << scale << " scale" << std::endl;
}

void PlotFile::data(std::ostream &outs)
{
  std::list<Segment>::const_iterator i;

  x_pos = 0; y_pos = 0; pen = false;
  int x_max = 0;
  int x_min = 0;
  int y_max = 0;
  int y_min = 0;

  bool drawing = false;
  bool prev_pen;

  for (i = segments.begin(); i != segments.end(); i++) {
    prev_pen = pen;
    apply_segment(*i);

    if (pen != prev_pen) {
      if (pen) {
        outs << "newpath" << std::endl;
        outs << translate(x_pos, y_pos) << " moveto" << std::endl;
        drawing = true;
      } else {
        outs << "stroke" << std::endl;
        drawing = false;
      }
    } else if (drawing) {
      outs << translate(x_pos, y_pos) << " lineto" << std::endl;
    }
  }
  outs << "showpage" << std::endl;
}

/*
 * Model Option   Step   Paper (steps) Limits (steps)
 *  3341   2113  0.1mm   360mm   3600   340mm   3400
 *  3342   2114  0.2mm   360mm   1800   340mm   1700
 *  3141   2111   5mil  14.125   2825  13.375   2675
 *  3142   2112  10mil  14.125   1412  13.375   1337
 *
 */

static const char *m3341_names[] = {
  "3341", "341", "2113", "13", "3", 0 };
static const char *m3342_names[] = {
  "3342", "342", "2114", "14", "4", 0 };
static const char *m3141_names[] = {
  "3141", "141", "2111", "11", "1", 0 };
static const char *m3142_names[] = {
  "3142", "142", "2112", "12", "2", 0 };
static const PlotterModel plotter_models[] = {
  {m3341_names, true,   1,  3600,  3400},
  {m3342_names, true,   2,  3600,  3400},
  {m3141_names, false,  5, 14125, 13375},
  {m3142_names, false, 10, 14125, 13375},
  {0,           false,  0,     0,     0}
};

static const Media medias[] = {
  {"Folio",      595,  935},
  {"Executive",  522,  756},
  {"Letter",     612,  792},
  {"Legal",      612, 1008},
  {"Ledger",    1224,  792},
  {"Tabloid",    792, 1224},
  {"A0",        2384, 3370},
  {"A1",        1684, 2384},
  {"A2",        1191, 1684},
  {"A3",         842, 1191},
  {"A4",         595,  842},
  {"A5",         420,  595},
  {"A6",         297,  420},
  {"A7",         210,  297},
  {"A8",         148,  210},
  {"A9",         105,  148},
  {"B0",        2920, 4127},
  {"B1",        2064, 2920},
  {"B2",        1460, 2064},
  {"B3",        1032, 1460},
  {"B4",         729, 1032},
  {"B5",         516,  729},
  {"B6",         363,  516},
  {"B7",         258,  363},
  {"B8",         181,  258},
  {"B9",         127,  181}, 
  {"B10",         91,  127},
  {0,              0,    0}
};


static const PlotterModel *lookup_plotter_model(const std::string &str)
{
  const PlotterModel *pm = plotter_models;

  while (pm->names) {
    const char **mn = pm->names;
    while (*mn) {
      std::string smn(*mn);
      if (smn == str) {
        return pm;
      }
      mn++;
    }
    pm++;
  }
  return 0;
}

static const Media *lookup_media(const std::string &str)
{
  const Media *m = medias;

  // Should ignore case?

  while (m->name) {
    std::string sm(m->name);
    if (sm == str) {
      return m;
    }
    m++;
  }
  return 0;
}

static bool parse_double(const std::string &str, double &v)
{
  const char *nptr = str.c_str();
  char *endptr;
  v = strtod(nptr, &endptr);
  return ((endptr-nptr) == str.size());
}

static bool ascii_file;
static char *input_filename;
static char *output_filename;
static const PlotterModel *plotter_model;
static const Media *media;
static double pen_width;
static bool scale_flag;
static bool keep_flag;
static bool force_portrait;
static bool force_landscape;

#define ENV_MODEL     "H16PLT2PS_MODEL"
#define ENV_MEDIA     "H16PLT2PS_MEDIA"
#define ENV_PEN_WIDTH "H16PLT2PS_PEN_WIDTH"

#define DEFAULT_MODEL     "2113"
#define DEFAULT_MEDIA     "A4"
#define DEFAULT_PEN_WIDTH 0.5

static void defs()
{
  ascii_file = false;
  input_filename = 0;
  output_filename = 0;
  plotter_model = lookup_plotter_model(DEFAULT_MODEL);
  media = lookup_media(DEFAULT_MEDIA);
  pen_width = DEFAULT_PEN_WIDTH;
  scale_flag = false;
  keep_flag = false;
  force_portrait = false;
  force_landscape = false;
}

static void envs()
{
  const char *value;

  if ( (value = getenv(ENV_MODEL)) ) {
    const PlotterModel *pm = lookup_plotter_model(value);
    if (pm)
      plotter_model = pm;
    else {
      std::cerr << "Warning: Couldn't identify model from "
                << ENV_MODEL << " set to <"
                << value << ">" << std::endl;
    }
  }

  if ( (value = getenv(ENV_MEDIA)) ) {
    const Media *m = lookup_media(value);
    if (m)
      media = m;
    else {
      std::cerr << "Warning: Couldn't identify media from "
                << ENV_MEDIA << " set to <"
                << value << ">" << std::endl;
    }
  }

  if ( (value = getenv(ENV_PEN_WIDTH)) ) {
    double w;
    if (parse_double(value, w))
      pen_width = w;
    else {
      std::cerr << "Warning: Couldn't parse pen width from "
                << ENV_PEN_WIDTH << " set to <"
                << value << ">" << std::endl;
    }
  }

}

/*
 * -a : ascii plotfile format
 * -e : EPSF ???
 * -fp : force portrait
 * -fl : force landscape
 * -h : help
 * -k : Keep actual paper size (else scale for paper)
 * -m <media> : specify media (env. variable, A4)
 * -o : output file
 * -p <model> : plotter model
 * -s : Scale image to fill paper (else full ploter width)
 * -w : Pen width in mm (floating point)
 */

static void print_usage(std::ostream &strm, char *arg0)
{
  strm << "Usage : " << arg0
       << " [-h] [-a] [-f<l|p>] [-i] [-k] [-m <media>] [-o <filename>] [-p <model>] [-w <width>] <filename>" << std::endl;
}

static void args(int argc, char **argv)
{
  int a;

  bool usage = false;
  bool help = false;

  bool media_set_by_args = false;

  for (a=1; a<argc; a++) {
    if (argv[a][0] == '-') {
      if (strcmp(argv[a], "-a") == 0) {
        ascii_file = true;
      } else if (strncmp(argv[a], "-f", 2) == 0) {
        char c = '\0';
        if (strlen(argv[a]) == 2) {
          a++;
          if ((a<argc) && 
              (strlen(argv[a]) == 1))
            c = argv[a][0];
          else {usage = true; break; }
        } else if (strlen(argv[a]) == 3) {
          c = argv[a][2];
        } else { usage = true; break; }
        c = tolower(c);
        //std::cout << "c = <" << c << ">" << std::endl;
        if (c == 'p')
          force_portrait = true;
        else if (c == 'l')
          force_landscape = true;
        else { usage = true; break; }
      } else if ((strncmp(argv[a], "-h", 2) == 0) || (strncmp(argv[a], "--h", 3) == 0)) {
        help = true;
        break;
      } else if (strcmp(argv[a], "-k") == 0) {
        keep_flag = true;
      } else if (strcmp(argv[a], "-m") == 0) {
        a++;
        if ((a<argc) &&
            (media = lookup_media(argv[a])))
          media_set_by_args = true;
        else {
          usage = true;
          break;
        }
      } else if (strcmp(argv[a], "-o") == 0) {
        a++;
        if (a<argc)
          output_filename = strdup(argv[a]);
        else {
          usage = true;
          break;
        }
      } else if (strcmp(argv[a], "-p") == 0) {
        a++;
        if ((a>=argc) ||
            (!(plotter_model = lookup_plotter_model(argv[a])))) {
          usage = true;
          break;
        }
      } else if (strcmp(argv[a], "-s") == 0) {
        scale_flag = true;
      } else if (strcmp(argv[a], "-w") == 0) {
        a++;
        if ((a>=argc) ||
            (!parse_double(argv[a], pen_width))) {
          usage = true;
          break;
        }
      } else {
        usage = true;
        break;
      }
    } else {
      if (a == (argc-1))
        input_filename = strdup(argv[a]);
      else {
        usage = true;
        break;
      }
    }
  }

  if (help) {
    print_usage(std::cout, argv[0]);

    std::cout << " -a            : ASCII input file format " << std::endl;
    std::cout << " -fl           : Force landscape" << std::endl;
    std::cout << " -fp           : Force portrait" << std::endl;
    std::cout << " -h            : This help" << std::endl;
    std::cout << " -s            : Scale image to fit paper" << std::endl;
    std::cout << " -k            : Keep actual plotter paper size" << std::endl;
    std::cout << " -m <media>    : Select media size" << std::endl;
    std::cout << "     media     = \"A4\", \"Letter\", etc." << std::endl;
    std::cout << " -o <filename> : Set output file (else stdout)" << std::endl;
    std::cout << " -p <plotter>  : Select plotter" << std::endl;
    std::cout << "     plotter   - either plotter model," << std::endl;
    std::cout << "                 i.e.:3341, 3342, 3141, 3142" << std::endl;
    std::cout << "               - or Honeywell option number," << std::endl;
    std::cout << "                 i.e.:2111, 2112, 2113, 2114" << std::endl;
    std::cout << " -w <float>    : Width of pen in mm" << std::endl;
    std::cout << " <filename>    : Input plot file" << std::endl;

    exit(0);
  }

  if (!input_filename)
    usage = true;

  if (force_portrait && force_landscape) {
    std::cerr << "Can't force both portrait and landscape" << std::endl;
    exit(1);
  }

  if (keep_flag && scale_flag) {
    std::cerr << "Doesn't make sense to ask for reproduction at full size" << std::endl
              << "and ask for the image to be scaled to fit the paper" << std::endl;
    exit(1);
  }
 
  // If media set by environment variable then silently
  // ignore, but if set by args, can't have a -k
  if (keep_flag && media_set_by_args) {
    std::cerr << "Doesn't make sense to ask for reproduction at full size" << std::endl
              << "and specify a particular media size" << std::endl;
    exit(1);
  }

  if (usage) {
    print_usage(std::cerr, argv[0]);
    exit(1);
  }
}

int main (int argc, char **argv)
{
  defs(); // Set default values
  envs(); // Values from environment variables
  args(argc, argv); // Command line arguments
  
  std::ifstream ins(input_filename);

  if (!ins) {
    std::cerr << "Cannot open <" << input_filename << "> for input" << std::endl;
    exit(1);
  }

  std::ofstream outfs;

  if (output_filename) {
    outfs.open(output_filename);

    if (!outfs) {
      std::cerr << "Cannot open <" << output_filename << "> for output" << std::endl;
      exit(1);
    }
  }

  std::ostream &outs = (output_filename) ? outfs : std::cout;

  PlotFile pf;

  pf.readfile(ins, ascii_file);
  pf.preprocess(scale_flag, keep_flag, force_portrait, force_landscape,
                plotter_model, media);
  pf.headers(outs);
  pf.data(outs);

  exit(0);
}
