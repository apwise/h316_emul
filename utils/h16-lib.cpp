/* Manipulate library files
 *
 * Copyright (C) 2008, 2009, 2011, 2012, 2026  Adrian Wise
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

#include <cstdlib>

#include <iostream>
#include <ostream>
#include <istream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <ranges>
#include <cassert>

enum class ERROR {
  // Used in class Block
  NONE,     // All OK
  END,      // End Of File encountered while reading block leader
  E_O_F,    // End Of File encountered
  STREAM,   // Some other stream error
  SOH,      // Missing SOH at start of block
  CHANNEL,  // Channel punched that should not be
  DC3,      // Encountered DC3 in unexpected place
  DEL,      // DEL does not follow DC3
  CHECKSUM, // Checksum incorrect
  // Added by class ObjectBlock
  BLOCK_TYPE, // Unknown block type
  WORDS,      // No Words in block
  // Added by class ObjectFile
  OPEN,       // Could not open file
};

static const std::map<ERROR, std::string> errorName {
  {ERROR::NONE,       "NONE"},        
  {ERROR::END,        "END"},         
  {ERROR::E_O_F,      "E_O_F"},       
  {ERROR::STREAM,     "STREAM"},      
  {ERROR::SOH,        "SOH"},         
  {ERROR::CHANNEL,    "CHANNEL"},     
  {ERROR::DC3,        "DC3"},         
  {ERROR::DEL,        "DEL"},         
  {ERROR::CHECKSUM,   "CHECKSUM"},   
  {ERROR::BLOCK_TYPE, "BLOCK_TYPE"},  
  {ERROR::WORDS,      "WORDS"},       
  {ERROR::OPEN,       "OPEN"},
};

class Block
{
public:
  Block();
  virtual ~Block();
  void clear();

  void read(std::istream &st);
  void write(std::ostream &st);

  virtual bool operator==(const Block &rhs) const;
  
  void dump(std::ostream &st) const;

  ERROR getError() const { return error; };

  static Block *read_new_block(std::istream &st, ERROR *p_error = 0);
  static ERROR write_leader(std::ostream &st, unsigned int n);

  bool is_eot() { return eot; };
  void make_eot();
  void set_leader_frames(unsigned int n);

protected:
  bool ok() const { return (error == ERROR::NONE); };
  bool eot;
  std::streampos pos;
  std::list<unsigned> words;
  unsigned int leader_frames;

  virtual std::string annotation(std::list<unsigned>::const_iterator wi) const;
  
private:
  ERROR error;
  unsigned int checksum;


  unsigned char read_char(std::istream &st);
  bool read_block_start(std::istream &st);
  int read_invisible_char(std::istream &st, bool first);
  int read_invisible(std::istream &st);
  void read_block_finish(std::istream &st);

  void write_char(std::ostream &st, unsigned char uc);
  void write_block_start(std::ostream &st);
  void write_eot(std::ostream &st);
  static int to_invisible_char(int sixbit);
  //void write_invisible_char(std::ostream &st, int sixbit);
  static int to_invisible(int sixteenbit, int i);
  void write_invisible(std::ostream &st, int sixteenbit);
  void write_block_finish(std::ostream &st);
  void compute_checksum(bool add_not_replace = false);

  static const unsigned int SOH = 0201;
  static const unsigned int ETX = 0203;
  static const unsigned int DC3 = 0223;
  static const unsigned int DEL = 0377;
};

Block::Block()
  : eot(false),
    pos(-1),
    leader_frames(0),
    error(ERROR::NONE),
    checksum(0)
{
}

Block::~Block()
{
}

void Block::clear() {
  error = ERROR::NONE;
  eot = false;
  pos = -1;
  words.clear();
  leader_frames = 0;
  checksum = 0;
}

unsigned char Block::read_char(std::istream &st)
{
  char c = 0;
  if (ok()) {
    if (st.good()) {
      st.get(c);
      if (st.fail()) {
        if (st.eof())
          error = ERROR::E_O_F;
        else
          error = ERROR::STREAM;
        c = 0;
      }
    } else {
      error = ERROR::STREAM;
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
        error = ERROR::STREAM;
    }
  }
}

bool Block::read_block_start(std::istream &st)
{
  bool r = false;
  unsigned char c = 0;
  std::streampos stream_pos = st.tellg();
  if (ok()) c = read_char(st);
  while ((ok()) && (c == 0)) {
    leader_frames++;
    stream_pos = st.tellg();
    c = read_char(st);
  }
  if ((error == ERROR::E_O_F) && (c == 0))
    error = ERROR::END; // Clean end of file

  if (ok()) {
    switch(c) {
    case SOH: r = true; break; // Normal Start of block
    case ETX:
      c = read_char(st);
      if ((c == DC3) && (ok())) {
        c = read_char(st);
        if ((c == DEL) && (ok()))
          r = false; // It is an EOT mark
        else {
          if (ok()) st.putback(c);
          error = ERROR::SOH;
        }
      } else {
        if (ok()) st.putback(c);
        error = ERROR::SOH;
      }
      break;
    default:
      error = ERROR::SOH;
    }
  }
  if (ok()) {
    pos = stream_pos;
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

    if (r == ((int) DC3)) {
      if (first)
        r = -1;
      else
        error = ERROR::DC3;
    } else {
      l7 = r & 0177;
      
      switch (l7) {
      case 0174: l7 = 0005; break;
      case 0175: l7 = 0012; break;
      case 0176: l7 = 0021; break;
      case 0177: l7 = 0023; break;
      default:
        if ((r & (~mask)) != 0)
          error = ERROR::CHANNEL;
      }

      r = ((r>>2) & 0040) | (l7 & 0037);
    }
  }
  return r;
}

int Block::to_invisible_char(int sixbit)
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
  
  return c;
}

/*
void Block::write_invisible_char(std::ostream &st, int sixbit)
{
  write_char(st, to_invisible_char(sixbit));
}
*/

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

int Block::to_invisible(int sixteenbit, int i)
{
  int c;
  assert(i<3);
  switch(i) {
  case 0: c = to_invisible_char((sixteenbit >> 12) & 017); break;
  case 1: c = to_invisible_char((sixteenbit >> 6)  & 077); break;
  case 2: c = to_invisible_char( sixteenbit        & 077); break;
  default: c = DEL; 
  }
  return c;
}
  
void Block::write_invisible(std::ostream &st, int sixteenbit)
{
  write_char(st, to_invisible(sixteenbit, 0));
  write_char(st, to_invisible(sixteenbit, 1));
  write_char(st, to_invisible(sixteenbit, 2));
}

void Block::read_block_finish(std::istream &st)
{
  unsigned char c = 0;
  if (ok()) c = read_char(st);
  if (ok() && (c != DEL))
    error = ERROR::DEL;
}

void Block::write_block_finish(std::ostream &st)
{
  write_char(st, DC3);
  write_char(st, DEL);
}

void Block::read(std::istream &st)
{
  int w=0;

  error = ERROR::NONE;
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
        error = ERROR::CHECKSUM;
    }
  } else if (ok()) {
    // It's an EOT mark - set special flag
    eot = true;
  }

  //dump(std::cout);
}

bool Block::operator==(const Block &rhs) const {
  return (words == rhs.words);
}

// low-level diagnostic dump of the block
void Block::dump(std::ostream &st) const
{
  unsigned a = (pos >= 0) ? static_cast<unsigned>(pos) : 077777777;
  if (eot) {
    st << std::oct << std::setfill('0')
       << std::setw(8) << a
       << " " << std::setw(3) << ETX
       << " " << std::setw(3) << DC3
       << " " << std::setw(3) << DEL;
    if (pos >= 0) a+=3;
  } else {
    st << std::oct << std::setfill('0')
       << std::setw(8) << a
       << " " << std::setw(3) << SOH << "\n";
    if (pos >= 0) a++;
    
    for (std::list<unsigned>::const_iterator i = words.cbegin(); i != words.cend(); i++) {
      std::string annot = annotation(i);
      const int &word(*i);
      
      st << std::oct << std::setfill('0')
         << std::setw(8) << a
         << " " << std::setw(3) << to_invisible(word, 0)
         << " " << std::setw(3) << to_invisible(word, 1)
         << " " << std::setw(3) << to_invisible(word, 2);
      if (pos >= 0) a+=3;
    
      if (annot.size() > 0) {
        st << " " << annot;
      }
      st << "\n";
    }
    
    st << std::oct << std::setfill('0')
       << std::setw(8) << a
       << " " << std::setw(3) << DC3
       << " " << std::setw(3) << DEL;
    if (pos >= 0) a+=2;
  }

  st << std::endl;
}

std::string Block::annotation(std::list<unsigned>::const_iterator wi) const
{
  std::string s;

  if ((wi!=words.cbegin()) && (wi!=words.cend())) {
    std::ostringstream st;
    st << std::oct << std::setfill('0')
       << std::setw(6) << (*wi);
    s = st.str();
  }
  return s;
}

//
// This is static - and hence doesn't use the
// per-Block-object error field, so that it can
// be called from higher level objects
//
ERROR Block::write_leader(std::ostream &st, unsigned int n)
{
  ERROR r = ERROR::NONE;
  unsigned int i = 0;
  char z = 0;

  while ((r == ERROR::NONE) && (i < n)) {
    if (st.good()) {
      st.put(z);
      if (st.fail())
        r = ERROR::STREAM;
    }
    i++;
  }
  return r;
}

void Block::compute_checksum(bool add_not_replace)
{
  std::list<unsigned>::iterator limit(words.end());
  if (!add_not_replace) {
    limit--;
  }
  
  checksum = 0;
  for (std::list<unsigned>::iterator i = words.begin(); i != limit; i++) {
    checksum ^= (*i);
  }

  if (add_not_replace) {
    words.push_back(checksum);
  } else {
    (*limit) = checksum;
  }
}

void Block::write(std::ostream &st)
{
  error = write_leader(st, leader_frames);

  if (eot) {
    write_eot(st);
  } else {
    compute_checksum();
    
    write_block_start(st);
    
    for (auto word: words) {
      write_invisible(st, word);
    }
    
    write_block_finish(st);
  }
}

Block *Block::read_new_block(std::istream &st, ERROR *p_error)
{
  Block *block = new Block();
  ERROR error;

  block->read(st);
  error = block->getError();

  if (error != ERROR::NONE) {
    delete block;
    block = 0;
  }

  if (p_error)
    *p_error = error;
  else {
    if (error != ERROR::NONE) {
      std::cerr << "Error reading block" << std::endl;
      exit(1);
    }
  }
  return block;
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
  virtual ~ObjectBlock();
  void clear();

  void parse_block_type();

  enum class BT {
    NONE,
    SUB_PROG_NAME,
    SPECIAL_FORCE,
    SPECIAL_CHAIN,
    SPECIAL_END_OF_JOB,
    DATA,
    SYMBOL_NUM_DEFN,
    END,
    REL,
    ABS,
    CALL,
    SUBR,
    EXD,
    LXD,
    SETB,
    ABS_PROG_WORDS,
    REL_PROG_WORDS,
    ABS_END_JMP,
    REL_END_JMP,
    SUBR_CALL,
    SUBR_OR_COMN,
    REF_ITEM_COMN,
    EOT
  };
  
  static ObjectBlock *read_new_block(std::istream &st, ERROR *p_error = 0,
                                     unsigned int *trailing_frames = 0);

  bool is_eoj() { return ((type == 0) && (subtype == 3)); };

protected:
  virtual std::string annotation(std::list<unsigned>::const_iterator wi) const;

private:
  struct BlockType {
    enum BT bt;
    int type;
    int subtype;
    const char *descr;
  };

  ERROR error;
  int type;
  int subtype;
  const BlockType *block_type;

  static const std::vector<BlockType> block_types;
  static const BlockType eot_block_type;

  void lookup_block_type();
};

ObjectBlock::ObjectBlock()
  : error(ERROR::NONE),
    type(-1),
    subtype(-1),
    block_type(nullptr)
{
}

ObjectBlock::~ObjectBlock()
{
}

void ObjectBlock::clear() {
  Block::clear();
  error = ERROR::NONE;
  type = -1;
  subtype = -1;
  block_type = nullptr;
}

const std::vector<ObjectBlock::BlockType> ObjectBlock::block_types {
  { BT::SUB_PROG_NAME,      0, 000, "Subprogram Name"},
  { BT::SPECIAL_FORCE,      0, 001, "Special Action - Force Load Next Subprogram"},
  { BT::SPECIAL_CHAIN,      0, 002, "Special Action - Turn On Chain Flag"},
  { BT::SPECIAL_END_OF_JOB, 0, 003, "Special Action - End Of Job"},
  { BT::DATA,               0, 004, "Data"},
  { BT::SYMBOL_NUM_DEFN,    0, 010, "Symbol Number Definition"},
  { BT::END,                0, 014, "End"},
  { BT::REL,                0, 024, "Relocatable Mode"},
  { BT::ABS,                0, 030, "Absolute Mode"},
  { BT::CALL,               0, 044, "Subprogram Call"},
  { BT::SUBR,               0, 050, "Subprogram Entry Point Definition"},
  { BT::EXD,                0, 054, "Enter Extended-Memory-Mode Desectorizing"},
  { BT::LXD,                0, 060, "Leave Extended-Memory-Mode Desectorizing"},
  { BT::SETB,               0, 064, "Set Base Sector"},
  { BT::ABS_PROG_WORDS,     1,  -1, "Absolute Program Words"},
  { BT::REL_PROG_WORDS,     2,  -1, "Relative Program Words"},
  { BT::ABS_END_JMP,        3,  -1, "Absolute End Jump"},
  { BT::REL_END_JMP,        4,  -1, "Relative End Jump"},
  { BT::SUBR_CALL,          5,  -1, "Subroutine Call"},
  { BT::SUBR_OR_COMN,       6,  -1, "Subroutine Or Common Block Definition"},
  { BT::REF_ITEM_COMN,      7,  -1, "Reference To Item In Common"},
};

const ObjectBlock::BlockType ObjectBlock::eot_block_type {
    BT::EOT,               -1,  -1, "End Of Tape"
};

void ObjectBlock::lookup_block_type()
{
  block_type = nullptr;
  
  for (const BlockType &bt: block_types) {
    if ((bt.type == type) &&
        ((bt.subtype < 0) || ((bt.subtype == subtype)))) {
      block_type = &bt;
      break;
    }
  }
  
  if (!block_type) {
    if (eot) {
      block_type = &eot_block_type;
    } else {
      error = ERROR::BLOCK_TYPE;
    }
  }
}

void ObjectBlock::parse_block_type()
{
  if (Block::ok()) {
    if (words.size()>0) {
      // Parse the block type
      unsigned word = words.front();
      type = (word >> 12) & 017;
      subtype = (word >> 6) & 077;
      lookup_block_type();
    } else {
      if (eot)
        lookup_block_type();
      else
        error = ERROR::WORDS;
    }
  }
}

std::string ObjectBlock::annotation(std::list<unsigned>::const_iterator wi) const
{
  std::string r = Block::annotation(wi);

  if ((wi == words.cbegin()) && (block_type)) {
    r = block_type->descr;
  }
  return r;
}

ObjectBlock *ObjectBlock::read_new_block(std::istream &st, ERROR *p_error,
                                         unsigned int *p_trailing_frames)
{
  ObjectBlock *block = new ObjectBlock();

  ERROR error = ERROR::NONE;

  block->Block::read(st);
  error = block->getError();

  if (error == ERROR::NONE) {
    block->parse_block_type();
  } else {
    if ((error == ERROR::END) && p_trailing_frames) {
      *p_trailing_frames = block->leader_frames;
    }
    delete block;
    block = nullptr;
  }

  if (p_error) {
    *p_error = error;
  } else {
    if (error != ERROR::NONE) {
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
  void read(const std::string &filename, bool list = false,
            ERROR *p_error=nullptr);
  void write(std::ostream &st, ERROR &r_error);
  void write(const std::string &filename, ERROR *p_error=nullptr);

  bool operator==(const ObjectFile &rhs) const;

  static ObjectFile *read_new_file(const std::string &filename,
                                   bool list=false,
                                   ERROR *p_error=nullptr);
  void strip_end_markers();
  void append_standard_end_markers();
  void set_trailing_frames(unsigned int n);
  void set_leading_frames(unsigned int n);

private:
  std::list<ObjectBlock> blocks;
  unsigned int trailing_frames;
};

ObjectFile::ObjectFile()
  : trailing_frames(0)
{
}


void ObjectFile::read(const std::string &filename,
                      bool list,
                      ERROR *p_error)
{
  std::ifstream ifs;

  blocks.resize(0);
  trailing_frames=0;

  ifs.open(filename.c_str());
  if (!ifs.good()) {
    if (p_error) {
      *p_error = ERROR::OPEN;
      return;
    } else {
      std::cerr << "Failed to open <" << filename
                << "> for reading" << std::endl;
      exit(1);
    }
  }
  
  ERROR error;
  ObjectBlock *block;

  block = ObjectBlock::read_new_block(ifs, &error, &trailing_frames);
  while (error == ERROR::NONE) {
    if (list) {
      block->dump(std::cout);
    }
    blocks.push_back(*block);
    bool eoj = block->is_eoj();
    delete block;
    if (eoj) {
      break;
    }
    block = ObjectBlock::read_new_block(ifs, &error, &trailing_frames);
  }

  ifs.close();

  if (error == ERROR::END) {
    error = ERROR::NONE;
  }
  
  if (error != ERROR::NONE) {
    if (p_error)
      *p_error = error;
    else {
      if (error == ERROR::OPEN) {
        std::cerr << "Failed to open <" << filename
                  << "> for reading" << std::endl;
      } else {
        std::cerr << "Error (" << errorName.at(error) << ") while reading <"
                  << filename << ">" << std::endl;
      }
      exit(1);
    }  
  }
}

void ObjectFile::write(std::ostream &st, ERROR &r_error)
{
  ERROR error = ERROR::NONE;

  for (auto block: blocks) {
    block.write(st);
    error = block.getError();
    if (error != ERROR::NONE) {
      break;
    }
  }

  if (error == ERROR::NONE) {
    error = Block::write_leader(st, trailing_frames);
  }

  r_error = error;
}

void ObjectFile::write(const std::string &filename, ERROR *p_error)
{
  std::ofstream ofs;
  ERROR error = ERROR::NONE;

  ofs.open(filename.c_str());

  if (!ofs.good()) {
    if (p_error) {
      *p_error = ERROR::OPEN;
      return;
    } else {
      std::cerr << "Failed to open <" << filename
                << "> for writing" << std::endl;
      exit(1);
    }
  }

  write(ofs, error);

  ofs.close();

  if (error != ERROR::NONE) {
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
                                      bool list,
                                      ERROR *p_error)
{
  ERROR error = ERROR::NONE;
  
  ObjectFile *objfile = new ObjectFile;

  objfile->read(filename, list, &error);

  if (error != ERROR::NONE) {
    delete objfile;
    objfile = nullptr;
  }

  if (p_error) {
    *p_error = error;
  } else if (error != ERROR::NONE) {
    if (error == ERROR::OPEN) {
      std::cerr << "Could not open <" << filename
                << "> for reading" << std::endl;
    } else {
      std::cerr << "Error (" << errorName.at(error) << ") while reading <"
                << filename << ">" << std::endl;
    }
    exit(1);
  }  
  return objfile;
}

void ObjectFile::strip_end_markers()
{
  std::ranges::reverse_view rev_blocks(blocks);
  for (auto block: rev_blocks) {
    if (block.is_eot() ||
        block.is_eoj()) {
      blocks.pop_back();
    } else {
      break;
    }
  }

  trailing_frames = 0;
}

void ObjectFile::append_standard_end_markers()
{
  auto &block = blocks.emplace_back();
  block.make_eot();
}

void ObjectFile::set_trailing_frames(unsigned int n)
{
  trailing_frames = n;
}

void ObjectFile::set_leading_frames(unsigned int n)
{
  if (blocks.size() > 0) {
    blocks.front().set_leader_frames(n);
  }
}

bool ObjectFile::operator==(const ObjectFile &rhs) const {
  return (blocks == rhs.blocks);
}

enum class ACTION {
  NONE,
  CONCATENATE,
  DIFF,
  UPGRADE,
  LIST
};

int main(int argc, char **argv)
{
  std::string output_filename;
  std::list<std::string> filenames;

  bool help = false;
  ACTION action = ACTION::NONE;

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

      case 'h':
        help = true;
        break;
          
      case 'c':
        if (action != ACTION::NONE) {
          usage = true;
        } else {
          action = ACTION::CONCATENATE;
        }
        break;
        
      case 'd':
        if (action != ACTION::NONE) {
          usage = true;
        } else {
          action = ACTION::DIFF;
        }
        break;
        
      case 'l':
        if (action != ACTION::NONE) {
          usage = true;
        } else {
          action = ACTION::LIST;
        }
        break;
        
      case 'u':
        if (action != ACTION::NONE) {
          usage = true;
        } else {
          action = ACTION::UPGRADE;
        }
        break;

      case 'o':
        if (argv[a][2])
          output_filename = &argv[a][2];
        else
          pending_filename = true;
        break;

      default:
        usage = true;
      }
    } else {
      filenames.push_back(argv[a]);
    }
    a++;
  }

  switch (action) {
  case ACTION::NONE:
    usage = true;
    break;
  case ACTION::CONCATENATE:
    if (filenames.size() < 2) {
      usage = true;
    }
    break;
  case ACTION::DIFF:
    if ((filenames.size() != 2) || (output_filename.size() > 0)) {
      usage = true;
    }
    break;
  case ACTION::LIST:
    if ((filenames.size() == 0) || (output_filename.size() > 0)) {
      usage = true;
    }
    break;
  case ACTION::UPGRADE:
    if (filenames.size() != 1) {
      usage = true;
    }
    break;
  default: // Unexpected action?
    usage = true;
  }

  if (usage) {
    std::cerr << "usage: " << argv[0]
              << " [-h] -c|-d|-l|-u [-o[ ]filename] <input-filename>..."
              << std::endl;
    exit(1);
  }

  if (help) {
    std::cout << "Print some help..." << std::endl;
    
    exit((usage) ? 1 : 0);
  }
  
  std::list<ObjectFile> object_files;
  bool list = (action == ACTION::LIST);
  
  for (auto filename: filenames) {
    ObjectFile *of = ObjectFile::read_new_file(filename, list);
    object_files.push_back(*of);
  }

  if (list) {
    exit(0);
  }
  
  if (action == ACTION::CONCATENATE) {

    auto last = object_files.end()--;
    for (auto iof = object_files.begin(); iof != object_files.end(); iof++) {
      auto &of = *iof;
      of.strip_end_markers();

      of.set_leading_frames((iof == object_files.begin()) ? 150 : 30);
      
      if (iof == last) {
        of.set_trailing_frames(150);
        of.append_standard_end_markers();
      } else {
        of.set_trailing_frames(0);
      }
    }
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

  int exit_code = 0;
  
  if (action == ACTION::DIFF) {
    assert(object_files.size() == 2);

    ObjectFile &first = *object_files.begin();
    ObjectFile &second = *(++object_files.begin());

    first.strip_end_markers();
    second.strip_end_markers();
  
    if (first == second) {
      std::cout << "Files are equivalent" << std::endl;
    } else {
      std::cout << "Files differ" << std::endl;
      exit_code = 2;
    }
    
  } else {
    std::ostream &os = (output_file) ? ofs : std::cout;
    
    ERROR error = ERROR::NONE;

    for (auto of: object_files) {
      of.write(os, error);
      if (error != ERROR::NONE) {
        break;
      }
    }
    
    if (error != ERROR::NONE) {
      std::cerr << "Error while writing object text" << std::endl;
      exit(1);
    }
  }
  exit(exit_code);  
}
