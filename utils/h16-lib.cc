/* Manipulate library files
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
 *
 * At the moment all this can do is concatenate object files
 * into a library, removing spurious Fortran-IV End-Of-Job blocks
 */


#include <iostream>
#include <istream>
#include <fstream>
#include <vector>

class Block
{
public:
  Block();
  ~Block();

  void read(std::istream &st);
  void write(std::ostream &st);

  enum ERROR {
    ERR_NONE,     // All OK
    ERR_END,      // End Of File encountered while reading block leader
    ERR_EOF,      // End Of File encountered
    ERR_STREAM,   // Some other stream error
    ERR_SOH,      // Missing SOH at start of block
    ERR_CHANNEL,  // Channel punched that should not be
    ERR_DC3,      // Encountered DC3 in unexpected place
    ERR_DEL,      // DEL does not follow DC3
    ERR_CHECKSUM, // Checksum incorrect
    ERR_NUM
  };

  ERROR error() const { return Error; };

  static Block *read_new_block(std::istream &st, ERROR *p_error = 0);
  static ERROR write_leader(std::ostream &st, unsigned int n);

  bool is_eot() { return eot; };
  void make_eot();
  void set_leader_frames(unsigned int n);

protected:
  ERROR Error;
  bool ok() const { return Error == ERR_NONE; };
  bool eot;
  std::vector<unsigned int> words;
  unsigned int leader_frames;
  
private:
  unsigned int checksum;


  unsigned char read_char(std::istream &st);
  bool read_block_start(std::istream &st);
  int read_invisible_char(std::istream &st, bool first);
  int read_invisible(std::istream &st);
  void read_block_finish(std::istream &st);

  void write_char(std::ostream &st, unsigned char uc);
  void write_block_start(std::ostream &st);
  void write_eot(std::ostream &st);
  void write_invisible_char(std::ostream &st, int sixbit);
  void write_invisible(std::ostream &st, int sixteenbit);
  void write_block_finish(std::ostream &st);
  void compute_checksum(bool add_not_replace = false);

  static const unsigned int SOH = 0201;
  static const unsigned int ETX = 0203;
  static const unsigned int DC3 = 0223;
  static const unsigned int DEL = 0377;
};

Block::Block()
  : Error(ERR_NONE),
    eot(false),
    leader_frames(0),
    checksum(0)
{
}

Block::~Block()
{
}

unsigned char Block::read_char(std::istream &st)
{
  char c;
  if (ok()) {
    if (st.good()) {
      st.get(c);
      if (st.fail()) {
        if (st.eof())
          Error = ERR_EOF;
        else
          Error = ERR_STREAM;
        c = 0;
      }
    } else {
      Error = ERR_STREAM;
    }
  } else
    c = 0;
  return c;
}

void Block::write_char(std::ostream &st, unsigned char uc)
{
  char c = uc;

  if (ok()) {
    if (st.good()) {
      st.put(c);
      if (st.fail())
        Error = ERR_STREAM;
    }
  }
}

bool Block::read_block_start(std::istream &st)
{
  bool r = true;
  unsigned char c;
  if (ok()) c = read_char(st);
  while ((ok()) && (c == 0)) {
    leader_frames++;
    c = read_char(st);
  }
  if ((Error == ERR_EOF) && (c == 0))
    Error = ERR_END; // Clean end of file

  if (ok()) {
    switch(c) {
    case SOH: break; // Normal Start of block
    case ETX:
      c = read_char(st);
      if ((c == DC3) && (ok())) {
        c = read_char(st);
        if ((c == DEL) && (ok()))
          r = false; // It is an EOT mark
        else {
          if (ok()) st.putback(c);
          Error = ERR_SOH;
        }
      } else {
        if (ok()) st.putback(c);
        Error = ERR_SOH;
      }
      break;
    default:
      Error = ERR_SOH;
    }
  }
  return r;
}

void Block::write_block_start(std::ostream &st)
{
  write_char(st, SOH);
}

void Block::write_eot(std::ostream &st)
{
  write_char(st, ETX);
  write_char(st, DC3);
  write_char(st, DEL);
}

int Block::read_invisible_char(std::istream &st, bool first)
{
  int r = 0;
  int l7;

  int mask = (first) ? 0017 : 0237;

  if (ok()) {
    r = read_char(st);

    if (r == DC3) {
      if (first)
        r = -1;
      else
        Error = ERR_DC3;
    } else {
      l7 = r & 0177;
      
      switch (l7) {
      case 0174: l7 = 0005; break;
      case 0175: l7 = 0012; break;
      case 0176: l7 = 0021; break;
      case 0177: l7 = 0023; break;
      default:
        if ((r & (~mask)) != 0)
          Error = ERR_CHANNEL;
      }

      r = ((r>>2) & 0040) | (l7 & 0037);
    }
  }
  return r;
}

void Block::write_invisible_char(std::ostream &st, int sixbit)
{
  int c = ((sixbit & 040) << 2) | (sixbit & 037);
  int l7 = c & 0177;

  switch (l7) {
  case 0005: l7 = 0174; break;
  case 0012: l7 = 0175; break;
  case 0021: l7 = 0176; break;
  case 0023: l7 = 0177; break;
  default:
    break; // Nothing to do...
  }

  c = (c & 0200) | l7;
  
  write_char(st, c);
}

int Block::read_invisible(std::istream &st)
{
  int r = 0;
  
  if (ok()) r = read_invisible_char(st, true);
  // Returns -1 when DC3 encountered
  if (r >= 0) {
    if (ok()) r = (r << 6) | read_invisible_char(st, false);
    if (ok()) r = (r << 6) | read_invisible_char(st, false);
  }
  //std::cout << "Block::read_invisible() = " << r << std::endl;
  return r;
}

void Block::write_invisible(std::ostream &st, int sixteenbit)
{
  write_invisible_char(st, (sixteenbit >> 12) & 017);
  write_invisible_char(st, (sixteenbit >> 6)  & 077);
  write_invisible_char(st,  sixteenbit        & 077);
}

void Block::read_block_finish(std::istream &st)
{
  unsigned char c = 0;
  if (ok()) c = read_char(st);
  if (ok() && (c != DEL))
    Error = ERR_DEL;
}

void Block::write_block_finish(std::ostream &st)
{
  write_char(st, DC3);
  write_char(st, DEL);
}

void Block::read(std::istream &st)
{
  int w=0;

  Error = ERR_NONE;
  leader_frames = 0;
  checksum = 0;
  words.resize(0);

  if (read_block_start(st)) {
    
    w = read_invisible(st);
    while ((w >= 0) && (ok())) {
      checksum = checksum ^ w;
      words.push_back(w);
      w = read_invisible(st);
    }
    
    if (ok())
      read_block_finish(st);
    
    if (ok()) {
      if (checksum != 0)
        Error = ERR_CHECKSUM;
    }
  } else {
    // It's an EOT mark - set special flag
    eot = true;
  }
}

//
// This is static - and hence doesn't use the
// per-Block-object Error field, so that it can
// be called from higher level objects
//
Block::ERROR Block::write_leader(std::ostream &st, unsigned int n)
{
  ERROR r = ERR_NONE;
  unsigned int i = 0;
  char z = 0;

  while ((r == ERR_NONE) && (i < n)) {
    if (st.good()) {
      st.put(z);
      if (st.fail())
        r = ERR_STREAM;
    }
    i++;
  }
  return r;
}

void Block::compute_checksum(bool add_not_replace)
{
  int n = words.size();
  int i;

  if (!add_not_replace)
    n -= 1;

  if (n < 1)
    abort();

  checksum = 0;
  for (i = 0; i<n; i++)
    checksum ^= words[i];

  if (add_not_replace)
    words.push_back(checksum);
  else
    words[n] = checksum;
}

void Block::write(std::ostream &st)
{
  Error = write_leader(st, leader_frames);

  if (eot) {
    write_eot(st);
  } else {
    int i;
    
    compute_checksum();
    
    write_block_start(st);
    
    for (i=0; i<words.size(); i++) {
      write_invisible(st, words[i]);
    }
    
    write_block_finish(st);
  }
}

Block *Block::read_new_block(std::istream &st, ERROR *p_error)
{
  Block *block = new Block();
  ERROR error;

  block->read(st);
  error = block->error();

  if (error != ERR_NONE) {
    delete block;
    block = 0;
  }

  if (p_error)
    *p_error = error;
  else {
    if (error != ERR_NONE) {
      std::cerr << "Error reading block" << std::endl;
      exit(1);
    }
  }
}

void Block::make_eot()
{
  leader_frames = 10;
  words.clear();
  eot = true;
}

void Block::set_leader_frames(unsigned int n)
{
  leader_frames = n;
}

class ObjectBlock : public Block
{
public:
  ObjectBlock();
  ~ObjectBlock();

  void parse_block_type();

  enum ERROR {
    ERR_NONE,     // All OK
    ERR_END,      // End Of File encountered while reading block leader
    ERR_EOF,      // End Of File encountered
    ERR_STREAM,   // Some other stream error
    ERR_SOH,      // Missing SOH at start of block
    ERR_CHANNEL,  // Channel punched that should not be
    ERR_DC3,      // Encountered DC3 in unexpected place
    ERR_DEL,      // DEL does not follow DC3
    ERR_CHECKSUM, // Checksum incorrect
    //
    ERR_BLOCK_TYPE, // Unknown block type
    ERR_WORDS,      // No Words in block
    ERR_NUM
  };

  static ObjectBlock *read_new_block(std::istream &st, ERROR *p_error = 0,
                                     unsigned int *trailing_frames = 0);

  bool is_eoj() { return ((type == 0) && (subtype == 3)); };

private:
  struct BlockType {
    int type;
    int subtype;
    const char *name;
  };

  ERROR Error;
  int type;
  int subtype;
  BlockType *block_type;

  static ERROR conv_block_error(Block::ERROR error);

  static BlockType block_types[];
  static ObjectBlock EOT_block_type;

  void lookup_block_type();
};

ObjectBlock::ObjectBlock()
  : Error(ERR_NONE),
    type(-1),
    subtype(-1),
    block_type(0)
{
}

ObjectBlock::~ObjectBlock()
{
}

ObjectBlock::BlockType ObjectBlock::block_types[] = {
  { 0, 000, "Subprogram Name"},
  { 0, 001, "Special Action - Force Load Next Subprogram"},
  { 0, 002, "Special Action - Turn On Chain Flag"},
  { 0, 003, "Special Action - End Of Job"},
  { 0, 004, "Data"},
  { 0, 010, "Symbol Number Definition"},
  { 0, 014, "End"},
  { 0, 024, "Relocatable Mode"},
  { 0, 030, "Absolute Mode"},
  { 0, 044, "Subprogram Call"},
  { 0, 050, "Subprogram Entry Point Definition"},
  { 0, 054, "Enter Extended-Memory-Mode Desectorizing"},
  { 0, 060, "Leave Extended-Memory-Mode Desectorizing"},
  { 0, 064, "Set Base Sector"},
  { 1,  -1, "Absolute Program Words"},
  { 2,  -1, "Relative Program Words"},
  { 3,  -1, "Absolute End Jump"},
  { 4,  -1, "Relative End Jump"},
  { 5,  -1, "Subroutine Call"},
  { 6,  -1, "Subroutine Or Common Block Definition"},
  { 7,  -1, "Reference To Item In Common"},

  {-1,  -1, "End Of Tape"}
};

void ObjectBlock::lookup_block_type()
{
  BlockType *bt = block_types;

  block_type = 0;

  while (bt->type >= 0) {

    if ((bt->type == this->type) &&
        ((bt->subtype < 0) ||
         ((bt->subtype == this->subtype)))) {
      //std::cout << bt->name << std::endl;
      block_type = bt;
    }
    bt++;
  }
  if (eot)
    block_type = bt;
  else
    Error = ERR_BLOCK_TYPE;
}

void ObjectBlock::parse_block_type()
{
  if (Block::ok()) {
    if (words.size()>0) {
      // Parse the block type
      type = (words[0] >> 12) & 017;
      subtype = (words[0] >> 6) & 077;
      lookup_block_type();
    } else {
      if (eot)
        lookup_block_type();
      else
        Error = ERR_WORDS;
    }
  }
}

ObjectBlock::ERROR ObjectBlock::conv_block_error(Block::ERROR error)
{
  ObjectBlock::ERROR r;

  switch (error) {
  case ERR_NONE:     r = ERR_NONE;     break;
  case ERR_END:      r = ERR_END;      break;
  case ERR_EOF:      r = ERR_EOF;      break;
  case ERR_STREAM:   r = ERR_STREAM;   break;
  case ERR_SOH:      r = ERR_SOH;      break;
  case ERR_CHANNEL:  r = ERR_CHANNEL;  break;
  case ERR_DC3:      r = ERR_DC3;      break;
  case ERR_DEL:      r = ERR_DEL;      break;
  case ERR_CHECKSUM: r = ERR_CHECKSUM; break;
  default:
    abort();
  }
  return r;
}


ObjectBlock *ObjectBlock::read_new_block(std::istream &st, ERROR *p_error,
                                         unsigned int *p_trailing_frames)
{
  ObjectBlock *block = new ObjectBlock();

  Block::ERROR block_error = Block::ERR_NONE;
  ERROR error = ERR_NONE;

  block->Block::read(st);
  block_error = block->error();

  if (block_error == Block::ERR_NONE) {
    block->parse_block_type();
  } else {
    error = conv_block_error(block_error);
    if ((error == ERR_END) && p_trailing_frames) {
      *p_trailing_frames = block->leader_frames;
    }
    delete block;
    block = 0;
  }

  if (p_error)
    *p_error = error;
  else {
    if (error != ERR_NONE) {
      std::cerr << "Error reading block" << std::endl;
      exit(1);
    }
  }
  return block;
}

class ObjectFile
{
public:
  ObjectFile();
  void read(const std::string &filename, ObjectBlock::ERROR *p_error=0);
  void write(std::ostream &st, Block::ERROR &r_error);
  void write(const std::string &filename, Block::ERROR *p_error=0);

  static ObjectFile *read_new_file(const std::string &filename,
                                   ObjectBlock::ERROR *p_error=0);
  void strip_end_markers();
  void append_standard_end_markers();
  void set_trailing_frames(unsigned int n);
  void set_leading_frames(unsigned int n);

private:
  std::vector<ObjectBlock *> blocks;
  unsigned int trailing_frames;
};

ObjectFile::ObjectFile()
  : trailing_frames(0)
{
}


void ObjectFile::read(const std::string &filename, ObjectBlock::ERROR *p_error)
{
  std::ifstream ifs;

  blocks.resize(0);
  trailing_frames=0;

  ifs.open(filename.c_str());
  if (!ifs.good()) {
    if (p_error) {
      *p_error = ObjectBlock::ERR_STREAM;
      return;
    } else {
      std::cerr << "Failed to open <" << filename
                << "> for reading" << std::endl;
      exit(1);
    }
  }
  
  ObjectBlock::ERROR error;
  ObjectBlock *block;

  block = ObjectBlock::read_new_block(ifs, &error, &trailing_frames);
  while (error == ObjectBlock::ERR_NONE) {
    blocks.push_back(block);
    block = ObjectBlock::read_new_block(ifs, &error, &trailing_frames);
  }

  ifs.close();

  if (error == ObjectBlock::ERR_END)
    error = ObjectBlock::ERR_NONE;

  if (error != ObjectBlock::ERR_NONE) {
    if (p_error)
      *p_error = error;
    else {
      std::cerr << "Error while reading <" << filename
                << ">" << std::endl;
      exit(1);
    }  
  }
}

void ObjectFile::write(std::ostream &st, Block::ERROR &r_error)
{
  int i=0;
  Block::ERROR error = Block::ERR_NONE;

  while ((error == Block::ERR_NONE) &&
         (i < blocks.size())) {
    blocks[i]->write(st);
    error = blocks[i]->error();
    i++;
  }

  if (error == Block::ERR_NONE)
    error = Block::write_leader(st, trailing_frames);

  r_error = error;
}

void ObjectFile::write(const std::string &filename, Block::ERROR *p_error)
{
  std::ofstream ofs;
  Block::ERROR error = Block::ERR_NONE;

  ofs.open(filename.c_str());

  if (!ofs.good()) {
    if (p_error) {
      *p_error = Block::ERR_STREAM;
      return;
    } else {
      std::cerr << "Failed to open <" << filename
                << "> for writing" << std::endl;
      exit(1);
    }
  }

  write(ofs, error);

  ofs.close();

  if (error != Block::ERR_NONE) {
    if (p_error)
      *p_error = error;
    else {
      std::cerr << "Error while writing <" << filename
                << ">" << std::endl;
      exit(1);
    }  
  }
}

ObjectFile *ObjectFile::read_new_file(const std::string &filename,
                                      ObjectBlock::ERROR *p_error)
{
  ObjectBlock::ERROR error = ObjectBlock::ERR_NONE;
  
  ObjectFile *objfile = new ObjectFile;

  objfile->read(filename, &error);

  if (error != ObjectBlock::ERR_NONE) {
    delete objfile;
    objfile = 0;
  }

  if (p_error)
    *p_error = error;
  else if (error != ObjectBlock::ERR_NONE) {
    std::cerr << "Error while reading <" << filename
              << ">" << std::endl;
    exit(1);
  }  
  return objfile;
}

void ObjectFile::strip_end_markers()
{
  int i = blocks.size()-1;
  bool trimming = true;

  while ((i >= 0) && (trimming)) {
    if (blocks[i]->is_eot() ||
        blocks[i]->is_eoj()) {
      delete blocks[i];
      blocks.resize(i);
    } else
      trimming = false;
    i--;
  }

  trailing_frames = 0;
}

void ObjectFile::append_standard_end_markers()
{
  ObjectBlock *block = new ObjectBlock;

  block->make_eot();
  blocks.push_back(block);
}

void ObjectFile::set_trailing_frames(unsigned int n)
{
  trailing_frames = n;
}

void ObjectFile::set_leading_frames(unsigned int n)
{
  if (blocks.size() > 0)
    blocks[0]->set_leader_frames(n);
}

int main(int argc, char **argv)
{
  std::string output_filename;
  std::vector<std::string> filenames;

  bool concatenate = false;

  bool usage = false;
  bool pending_filename = false;
  int a=1;
  bool reading_args = 1;
  while (a < argc) {
    if (pending_filename) {
      output_filename = argv[a];
      pending_filename = false;
    } else if ((argv[a][0] == '-') && (reading_args)) {
      switch(argv[a][1]) {
      case '\0':
        /*
         * Just '-' turn off arg-reading to
         * allow a '-' to start the input
         * filename.
         */
        reading_args = false;
        break;

      case 'c':
        concatenate = true;
        break;

      case 'o':
        if (argv[a][2])
          output_filename = &argv[a][2];
        else
          pending_filename = true;
        break;

      default:
        usage = 1;
      }
    } else {
      filenames.push_back(argv[a]);
    }
    a++;
  }

  if (filenames.size() == 0)
    usage = 1;

  if (usage) {
    std::cerr << "usage: " << argv[0]
              << " [-c] [-o[ ]filename] <input-filename>..."
              << std::endl;
    exit(1);
  }
  
  std::vector<ObjectFile *> object_files;
  int i;

  for (i=0; i<filenames.size(); i++)
    object_files.push_back(ObjectFile::read_new_file(filenames[i]));

  for (i=0; i<object_files.size(); i++) {
    object_files[i]->strip_end_markers();

    if (i == 0)
      object_files[i]->set_leading_frames(150);
    else
      object_files[i]->set_leading_frames(30);

    if (i == object_files.size()-1) {
      object_files[i]->set_trailing_frames(150);
      object_files[i]->append_standard_end_markers();
    } else
      object_files[i]->set_trailing_frames(0);
  }

  bool output_file = false;
  std::ofstream ofs;
  if (output_filename.size()>0) {
    ofs.open(output_filename.c_str());
    if (ofs.good())
      output_file = true;
    else {
      std::cerr << "Failed to open <" << output_filename
                << "> for writing" << std::endl;
      exit(1);
    }
  }

  std::ostream &os = (output_file) ? ofs : std::cout;

  Block::ERROR error = Block::ERR_NONE;
  i = 0;

  for(i=0; (i<object_files.size()) && (error == Block::ERR_NONE) ; i++) 
    object_files[i]->write(os, error);

  if (error != Block::ERR_NONE) {
    std::cerr << "Error while writing object text" << std::endl;
    exit(1);
  }
    
}
