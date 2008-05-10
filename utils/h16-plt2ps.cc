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

/*
 * Model Option   Step   Paper (steps) Limits (steps)
 *  3341   2113  0.1mm   360mm   3600   340mm   3400
 *  3342   2114  0.2mm   360mm   1800   340mm   1700
 *  3141   2111   5mil  14.125   2825  13.375   2675
 *  3142   2112  10mil  14.125   1412  13.375   1337
 *
 */

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>

#include <string>
#include <list>

class PlotFile {

public:
  PlotFile();
  ~PlotFile();

  void readfile(const std::string filename, bool ascii_file);
  void preprocess();
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

};

const char *PlotFile::pd_names[16] = {
  "(null)", "E",       "W",  "(error)",
  "N",      "NE",      "NW", "(error)",
  "S",      "SE",      "SW", "(error)",
  "DN",     "(error)", "UP", "(error)" 
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

void PlotFile::readfile(const std::string filename, bool ascii_file)
{
  std::ifstream ins(filename.c_str());
  PLT_DIRN current_direction;
  long current_count;

  if (!ins) {
    std::cerr << "Cannot open <" << filename << "> for input" << std::endl;
    exit(1);
  }

  while (ins) {
    current_direction = PD_NULL;
    current_count = 0;

    if (ascii_file) {
      char buf[100];
      int d;
      char *cp, *cp2;

      ins.getline(buf, 100);
      //std::cout << "Current line <" << buf << ">" << std::endl;

      if (strlen(buf) > 0) {
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

void PlotFile::preprocess()
{
  std::list<Segment>::const_iterator i;

  x_pos = 0; y_pos = 0; pen = false;
  int x_max = 0;
  int x_min = 0;
  int y_max = 0;
  int y_min = 0;

  for (i = segments.begin(); i != segments.end(); i++) {
    apply_segment(*i);

    //if (pen) {
    if (x_pos > x_max) x_max = x_pos;
    if (x_pos < x_min) x_min = x_pos;
    if (y_pos > y_max) y_max = y_pos;
    if (y_pos < y_min) y_min = y_pos;
    //}
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
        outs << x_pos+x_offset << " "
             << y_pos+y_offset << " moveto" << std::endl;
        drawing = true;
      } else {
        outs << "stroke" << std::endl;
        drawing = false;
      }
    } else if (drawing) {
      outs << x_pos+x_offset << " "
           << y_pos+y_offset << " lineto" << std::endl;
    }
  }
  outs << "showpage" << std::endl;
}

static bool ascii_file = false;
static char *input_filename;
static char *output_filename;

static void print_usage(std::ostream &strm, char *arg0)
{
  strm << "Usage : " << arg0 << " [-h|--help] [-o <filename>] [-a] <filename>" << std::endl;
}

static void args(int argc, char **argv)
{
  int a;

  bool usage = false;
  bool help = false;

  ascii_file = false;
  input_filename = 0;
  output_filename = 0;

  for (a=1; a<argc; a++) {
    if (argv[a][0] == '-') {
      if ((strncmp(argv[a], "-h", 2) == 0) || (strncmp(argv[a], "--h", 3) == 0)) {
        help = true;
        break;
      } else if (strcmp(argv[a], "-a") == 0) {
        ascii_file = true;
      }
      else if (strcmp(argv[a], "-o") == 0) {
        a++;
        if (a<argc)
          output_filename = strdup(argv[a]);
        else {
          usage = true;
          break;
        }
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

  if (!input_filename)
    usage = true;

  if (usage) {
    print_usage(std::cerr, argv[0]);
    exit(1);
  }

  if (help) {
    print_usage(std::cout, argv[0]);

    // Any more help...
    exit(0);
  }
}

int main (int argc, char **argv)
{
  args(argc, argv);
  
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

  pf.readfile(input_filename, ascii_file);
  pf.preprocess();
  pf.headers(outs);
  pf.data(outs);

  exit(0);
}
