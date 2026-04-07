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
#include <cstdint>

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

#define DEBUG 0

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
  BLOCK_END,  // Ran off the end of the block while extracting
  UNEXPECTED, // Unexpected data values in block
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
  {ERROR::BLOCK_END,  "BLOCK_END"},
  {ERROR::UNEXPECTED, "UNEXPECTED"},
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
  std::vector<uint16_t> words;
  unsigned int leader_frames;
  static std::string octal(uint16_t v);

  virtual std::string annotation(unsigned wi) const;

  uint8_t extract8(unsigned byteOffset);
  uint16_t extract16(unsigned byteOffset);
  uint32_t extract24(unsigned byteOffset);
  std::string extractName(unsigned byteOffset);

private:
  ERROR error;
  uint16_t checksum;


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
    
    for (unsigned i = 0; i < words.size(); i++) {
      std::string annot = annotation(i);
      
      st << std::oct << std::setfill('0')
         << std::setw(8) << a
         << " " << std::setw(3) << to_invisible(words[i], 0)
         << " " << std::setw(3) << to_invisible(words[i], 1)
         << " " << std::setw(3) << to_invisible(words[i], 2);
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

std::string Block::annotation(unsigned wi) const {
  std::string s;

  if ((wi >= 0) && (wi < words.size())) {
    std::ostringstream st;
    st << std::oct << std::setfill('0')
       << std::setw(6) << words[wi];
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
  std::vector<uint16_t>::iterator limit(words.end());
  if (!add_not_replace) {
    limit--;
  }
  
  checksum = 0;
  for (std::vector<uint16_t>::iterator i = words.begin(); i != limit; i++) {
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

  void DecodeBlock();

  static ObjectBlock *read_new_block(std::istream &st, ERROR *p_error = 0,
                                     unsigned int *trailing_frames = 0);

  bool is_eoj() const { return ((type == 0) && (subtype == 3)); };

  bool legacy_block() const {
    return (type > 0);
  }
 
protected:
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

  enum class DT { // Data type
    GENERIC,
    FORWARD_9,
    FORWARD_DAC,
    KNOWN_9,
    KNOWN_DAC,
    L_GENERIC,
    L_ABS_9,
    L_ABS_DAC,
    L_REL_9,
    L_REL_DAC,
    L_COMP_9,
    L_COMP_DAC,
    L_LAST_REL,
    L_VALUE
  };

  enum class REL {
    ABSOLUTE = 0,
    POS_REL = 1,
    NEG_REL = 3
  };
  
  struct Data {
    uint32_t w24;
    DT type;
    uint16_t value;
    uint16_t instr; // For 9-bit relocations - includes F & T bits
    bool m; // Following symbol associated with this address
    REL rel;
    Data(ERROR &error, uint32_t w24, bool legacy = false);
    std::string annotation() const;
  };

  virtual std::string annotation(unsigned wi) const;
  std::string annotation_0_0(unsigned wi) const;
  std::string annotation_0_4(unsigned wi) const;
  std::string annotation_1and2(unsigned wi) const;
  std::string annotation_3and4(unsigned wi) const;
  std::string annotation_6(unsigned wi) const;
  std::string annotation_5and7(unsigned wi) const;

 
private:
  /*
   * Type of entries in the block_types table used to lookup the
   * type of a block being read.
   */
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

  uint16_t instr; // Should these be moved to a data item?
  uint16_t address;
  std::vector<std::string> names;
  std::vector<Data> items;

  void parse_block_type();
  void lookup_block_type();
  
  void DecodeBlockType0_0();
  void DecodeBlockType0_4();
  void DecodeBlockType0_10();
  void DecodeBlockType0_14();
  void DecodeBlockType0_44();
  void DecodeBlockType0_50();
  void DecodeBlockType1and2();
  void DecodeBlockType3up();

  void UpdateBlockType5and7(ObjectBlock &b);

  static std::string octal24(uint32_t v);
  static std::string sname(std::string name);
  static std::string sinstr(uint16_t instr, bool &tag);
};

ObjectBlock::ObjectBlock()
  : error(ERROR::NONE),
    type(-1),
    subtype(-1),
    block_type(nullptr),
    instr(0),
    address(0)
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
  instr = 0;
  address = 0;
  names.clear();
  items.clear();
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

std::string ObjectBlock::annotation(unsigned wi) const {
  std::string r = Block::annotation(wi);
  std::string s;
  
  if ((wi == 0) && (block_type)) {
    s = block_type->descr;
  } else if ((wi+1) == words.size()) {
    s = "Checksum";
  } else if ((type == 0) && (subtype==0)) {
    s = annotation_0_0(wi);
  } else if ((type == 0) && (subtype==4)) {
    s = annotation_0_4(wi);
  } else if ((type == 1) || (type == 2)) {
    s = annotation_1and2(wi);
  } else if ((type == 3) || (type == 4)) {
    s = annotation_3and4(wi);
  } else if ((type == 5) || (type == 7)) {
    s = annotation_5and7(wi);
  } else if (type == 6) {
    s = annotation_6(wi);
  }
  if (s.size() > 0) {
    r += " " + s;
  }
  return r;
}

std::string Block::octal(uint16_t v) {
  std::stringstream ss;
  ss << std::oct << std::setw(6) << std::setfill('0') << v;
  return ss.str();
}

std::string ObjectBlock::octal24(uint32_t v) {
  std::stringstream ss;
  ss << std::oct << std::setw(8) << std::setfill('0') << v;
  return ss.str();
}

std::string ObjectBlock::sname(std::string name) {
  int n = name.size();
  bool trimming = true;
  
  for (int i = (n-1); i >=0; i--) {
    name[i] &= 0x7f;
    if (trimming) {
      if (name[i] == ' ') {
        n--;
      } else {
        trimming = false;
      }
    }
  }
  return name.substr(0, n);  
}

std::string ObjectBlock::annotation_1and2(unsigned wi) const {
  assert((type == 1) || (type == 2));
  std::string r;
  if (wi == 1) {
    r = "@"+octal(address);
  } else if (((wi * 2) % 3) == 2) {
    unsigned i = ((wi - 2) * 2) / 3;
    r = items[i].annotation();
    if (DEBUG) {
      std::stringstream ss;
      ss << i;
      r += " i=" + ss.str();
    }
  } else if ((((wi * 2) + 1) % 3) == 2) {
    unsigned i = ((wi - 2) * 2) / 3;
    r = items[i].annotation();
    if (DEBUG) {
      std::stringstream ss;
      ss << i;
      r += " i=" + ss.str() + "*";
    }
  }
  /* else {
     r = "...";
     } */
  
  return r;
}

std::string ObjectBlock::annotation_3and4(unsigned wi) const {
  assert((type == 3) || (type == 4));
  std::string r;
  if (wi == 1) {
    r = "@" + octal(address);
  }
  return r;
}

std::string ObjectBlock::annotation_6(unsigned wi) const {
  assert(type == 6);
  std::string r;
  if (wi == 2) {
    r = "@"+octal(address);
  } else if ((wi == 4) && (names.size() >> 0)) {
    r = sname(names.front());
    switch (subtype) {
    case 0:
    case 2: r += " Subroutine definition"; break;
    case 1: r += " Common Block definition"; break;
    case 3: r += " Data storage into common follows"; break;
    default: break;
    }
  }
  return r;
}

std::string ObjectBlock::annotation_5and7(unsigned wi) const {
  assert((type == 5) || (type == 7));
  std::string r;
  if (wi == 1) {
    bool tag;
    r = sinstr(instr, tag) + octal(address);
    if (tag) {
      r += ",1";
    }
  } else if ((wi == 4) && (names.size() > 0)) {
    r = sname(names.front());
  }
  return r;
}

std::string ObjectBlock::annotation_0_0(unsigned wi) const {
  assert((type == 0) && (subtype == 0));
  std::string r;
  /*
   * 0 type
   * 1 number
   * 2 start name
   * 4 end first name
   */
  if ((wi >= 4) && (((wi - 4) % 3) == 0)) {
    unsigned i = ((wi - 4) / 3);
    r = sname(names[i]);
  }
  return r;
}

std::string ObjectBlock::annotation_0_4(unsigned wi) const {
  assert((type == 0) && (subtype == 4));
  std::string r;
  /*
   * 0 type
   * 1 number
   * 2 address
   * 3 start of 24-bit words...
   * 4 complete w24[0] 8->2
   * 5 complete w24[1] 10->11->2
   * 6 -               (12->0 13->1)
   * 7 complete w24[2]
   * 8 complete w24[3]
   */
  if (wi == 2) {
    r = "@" + octal(address);
  } else if (wi >= 3) {
    if (((wi * 2) % 3) == 2) {
      unsigned i = ((wi - 3) * 2) / 3;
      r = items[i].annotation();
      if (DEBUG) {
        std::stringstream ss;
        ss << i;
        r += " i=" + ss.str();
      }
    } else if ((((wi * 2) + 1) % 3) == 2) {
      unsigned i = ((wi - 3) * 2) / 3;
      r = items[i].annotation();
      if (DEBUG) {
        std::stringstream ss;
        ss << i;
        r += " i=" + ss.str() + "*";
      }
    }
    /* else {
       r = "...";
       } */
 }

  return r;
}

uint8_t Block::extract8(unsigned byteOffset) {
  unsigned wordOffset = byteOffset >> 1;
  bool lower = (byteOffset - (wordOffset << 1));
  if (wordOffset < words.size()) {
    if (lower) {
      return words[wordOffset] & 0xff;
    } else {
      return words[wordOffset] >> 8;
    }
  } else {
    error = ERROR::BLOCK_END;
    return 0;
  }
}
  
uint16_t Block::extract16(unsigned byteOffset) {
  uint16_t high = extract8(byteOffset++);
  uint16_t low  = extract8(byteOffset);
  return (high << 8) | low;
}

uint32_t Block::extract24(unsigned byteOffset) {
  uint32_t high = extract8(byteOffset++);
  uint32_t low  = extract16(byteOffset);
  return (high << 16) | low;
}

std::string Block::extractName(unsigned byteOffset) {
  std::string r;
  for (unsigned i = 0; i < 6; i++) {
    uint8_t c = extract8(byteOffset++);
    if (error != ERROR::NONE) {
      break;
    }
    r.push_back(c);
  }
  return r;
}

ObjectBlock::Data::Data(ERROR &error, uint32_t w24, bool legacy)
  : w24(w24) {
  m = false;
  rel = REL::ABSOLUTE; 
  
  if (legacy) {
    value = (w24 >> 4) & 0x3fff; // 14-bit address
    instr = (w24 >> 8) & 0xf300; // Instruction
    
    if ((w24 & 0xf) == 0) {
      type = DT::L_GENERIC;
      value = (w24 >> 8) & 0xffff; // Data or generic
      instr = 0;
    } else if ((w24 & 0xf) == 4) {
      type = DT::L_LAST_REL;
      instr = 0;
    } else if ((w24 & 0xf) == 8) {
      type = DT::L_VALUE;
      value = (w24 >> 4) & 0xffff; // 16-bit value
      instr = 0;
    } else if ((w24 & 0x7) == 1) {
      type = DT::L_ABS_9;
    } else if ((w24 & 0x7) == 2) {
      type = DT::L_REL_9;
    } else if ((w24 & 0x7) == 3) {
      type = DT::L_COMP_9;
    } else if ((w24 & 0x7) == 5) {
      type = DT::L_ABS_DAC;
    } else if ((w24 & 0x7) == 6) {
      type = DT::L_REL_DAC;
    } else if ((w24 & 0x7) == 7) {
      type = DT::L_COMP_DAC;
    } else {
      // No idea what this is
      type = DT::GENERIC;
      value = 0;
      instr = 0;
      error = ERROR::UNEXPECTED;
    }
  } else {
    instr = 0;
    if ((w24 & 0x1) == 1) {
      type = DT::KNOWN_9;
      instr = (w24 >> 8) & 0xf300; // Instruction
      value = (w24 >> 3) & 0x7fff; // 15-bit address
      rel = static_cast<REL>((w24 >> 1) & 0x3);
    } else if ((w24 & 0x7) == 0) {
      type = DT::GENERIC;
      value = (w24 >> 8) & 0xffff; // Data or generic
    } else if ((w24 & 0x7) == 2) {
      type = DT::FORWARD_9;
      instr = (w24 >> 8) & 0xf300;
      value = (w24 >> 3) & 0x1fff;
      m = (w24 >> 16) & 1;
    } else if ((w24 & 0x7) == 4) {
      type = DT::KNOWN_DAC;
      instr = (w24 >> 8) & 0x3000; // Just F & T
      value = (w24 >> 3) & 0xffff; // 16-bit address
      rel = static_cast<REL>((w24 >> 19) & 0x3);
    } else if ((w24 & 0x7) == 6) {
      type = DT::FORWARD_DAC;
      instr = (w24 >> 8) & 0x3000; // Just F & T
      value = (w24 >> 3) & 0x1fff;
      m = (w24 >> 16) & 1;
    }
  }
}

std::string ObjectBlock::sinstr(uint16_t instr, bool &tag) {
  std::string r;
  unsigned op = (instr >> 10) & 0xf;
  bool t = ((instr >> 14) & 1);
  switch (op) {
  case 0x0: r = "DAC"; break;
  case 0x1: r = "JMP"; break;
  case 0x2: r = "LDA"; break;
  case 0x3: r = "ANA"; break;
  case 0x4: r = "STA"; break;
  case 0x5: r = "ERA"; break;
  case 0x6: r = "ADD"; break;
  case 0x7: r = "SUB"; break;
  case 0x8: r = "JST"; break;
  case 0x9: r = "CAS"; break;
  case 0xa: r = "IRS"; break;
  case 0xb: r = "IMA"; break;
  case 0xc: r = "I/O"; break;
  case 0xd: r = (t) ? "LDX" : "STX"; break;
  case 0xe: r = "MPY"; break;
  case 0xf: r = "DIV"; break;
  }
  r += ((instr >> 15) & 1) ? "*" : " ";
  if (op == 0xd) {
    t = false;
  }
  tag = false;
  return r;
}

std::string ObjectBlock::Data::annotation() const {
  std::string r;
  bool tag;
  std::string si = sinstr(instr, tag);
  std::string more = ((m) ? "M" : "L");
  std::string srel = ((rel == REL::POS_REL) ? "R" :
                      (rel == REL::NEG_REL) ? "N" : "A");
  switch (type) {
  case DT::GENERIC:
    r = "      "+octal(value);
    tag = false;
    break;
  case DT::FORWARD_9:
    r = si + " " + more + octal(value);
    break;
  case DT::FORWARD_DAC:
    assert(((instr >> 10) & 0x0f) == 0);
    r = si + " " + more + octal(value);
    break;
  case DT::KNOWN_9:
    r = si + " " + srel + octal(value);
    break;
  case DT::KNOWN_DAC:
    assert(((instr >> 10) & 0x0f) == 0);
    r = si + " " + srel + octal(value);
    break;
  case DT::L_GENERIC:
    r = "G"+octal(value);
    tag = false;
    break;
  case DT::L_ABS_9:
    r = si + " " + "A"+octal(value);
    break;
  case DT::L_ABS_DAC:
    assert(((instr >> 10) & 0x0f) == 0);
    r = si + " " + "A"+octal(value);
    break;
  case DT::L_REL_9:
    r = si + " " + "R"+octal(value);
    break;
  case DT::L_REL_DAC:
    assert(((instr >> 10) & 0x0f) == 0);
    r = si + " " + "R"+octal(value);
    break;
  case DT::L_COMP_9:
    r = si + " " + "C"+octal(value);
    break;
  case DT::L_COMP_DAC:
    assert(((instr >> 10) & 0x0f) == 0);
    r = si + " " + "C"+octal(value);
    break;
  case DT::L_LAST_REL:
    r = "L"+octal(value);
    tag = false;
    break;
  case DT::L_VALUE:
    r = "V"+octal(value);
    tag = false;
    break;
  default:
    break;
  }
  if (tag) r += ",1";
  return octal24(w24) + " " + r;
}

void ObjectBlock::DecodeBlockType1and2() {
  assert((type == 1) || (type == 2));
  unsigned n = (extract16(0) >> 6) & 0x3f; // 6-bit number of words
  if (n >= 3) { // First word, address, and checksum
    n <<= 1; // Convert to bytes
    n -= 2; // Checksum
  } else {
    error = ERROR::BLOCK_END;
    return;
  }
  address = extract16(1) & 0x3fff; // 14-bit address;

  unsigned p = 3; // Offset of first 24-bit word
  while ((p+3) <= n) {
    uint32_t w24 = extract24(p);
    if (error == ERROR::NONE) {
      Data item(error, w24, /* legacy */ true);
      if (error == ERROR::NONE) {
        items.push_back(item);
      }
    } else {
      break;
    }
    p += 3; // Each word is 3 bytes
  }
}

void ObjectBlock::DecodeBlockType3up() {
  assert((type >= 3) && (type <= 7));

  if ((type == 5) || (type == 7)) {
    instr = (extract16(0) << 4) & 0xf300; // 6-bit instruction
  }
  address = extract16(1) & 0x3fff; // 14-bit address;

  if (type <= 4) {
    return;
  }
  
  std::string name = extractName(3);
  if (error==ERROR::NONE) {
    names.push_back(name);
  }

  if (type == 6) {
    uint8_t t = extract8(9);
    subtype = (t >> 6) & 0x3;
  }
}

void ObjectBlock::DecodeBlockType0_0() {
  unsigned n = extract8(2) & 0x3f;
  n <<= 2; // To bytes
  if (n < 6) { // Type, number, checksum
    error = ERROR::BLOCK_END;
  } else {
    n -= 2; // For the checksum
    unsigned p = 4; // Start of first symbol
    while ((p+6) <= n) {
      std::string s = extractName(p);
      if (error == ERROR::NONE) {
        names.push_back(s);
      } else {
        break;
      }
      p += 6;
    }
  }
}

void ObjectBlock::DecodeBlockType0_4() {
  unsigned n = extract8(2) & 0x3f;
  n <<= 2;
  if (n < 8) { // Type, number, location, checksum
    error = ERROR::BLOCK_END;
  } else {
    n -= 2; // For the checksum
    address = extract16(4);
    unsigned p = 6; // Start of first 24-bit word
    while ((p+3) <= n) {
      uint32_t w24 = extract24(p);
      if (error == ERROR::NONE) {
        Data item(error, w24);
        if (error == ERROR::NONE) {
          items.push_back(item);
        }
      } else {
        break;
      }
      p += 3; // Each word is 3 bytes
    }
  }
}

void ObjectBlock::DecodeBlockType0_10() {
  
}

void ObjectBlock::DecodeBlockType0_14() {
  unsigned n = extract8(2) & 0x3f;
  if (n < 4) {
    error = ERROR::BLOCK_END;
  } else {
    address = extract16(4) & 0x7fff;
  }
}

void ObjectBlock::DecodeBlockType0_44() {
  unsigned n = extract8(2) & 0x3f;
  if (n < 7) {
    error = ERROR::BLOCK_END;
  } else {
    std::string s = extractName(4);
    instr = extract16(10) & 1; // The P bit indicating DAC (else 9-bit)
  }
}

void ObjectBlock::DecodeBlockType0_50() {
  unsigned n = extract8(2) & 0x3f;
  n <<= 2;
  if (n < 6) { // Type, number, checksum
    error = ERROR::BLOCK_END;
  } else {
    n -= 2; // For the checksum
    unsigned p = 4; // Start of first symbol
    while ((p+6) <= n) {
      std::string s = extractName(p);
      if (error == ERROR::NONE) {
        names.push_back(s);
      } else {
        break;
      }
      p += 6;
    }
  }
}

void ObjectBlock::DecodeBlock() {
  parse_block_type();

  if ((error == ERROR::NONE) && (type >= 0)) { 
    if (type == 0) {
      if (subtype == 0) {
        DecodeBlockType0_0();
      } else if ((subtype < 4) || (subtype == 024) || (subtype == 030) ||
                 (subtype == 054) || (subtype == 060)) {
        // Nothing to do
      } else if (subtype == 4) {
        DecodeBlockType0_4();
      } else if (subtype == 010) {
        DecodeBlockType0_10();
      } else if (subtype == 014) {
        DecodeBlockType0_14();
      } else if (subtype == 044) {
        DecodeBlockType0_44();
      } else if (subtype == 050) {
        DecodeBlockType0_50();
      } else {
        std::cout << "subtype = " << std::oct << subtype << std::endl;
        assert(0);
      }
    } else if (type <= 2) {
      DecodeBlockType1and2();
    } else if (type <= 7) {
      DecodeBlockType3up();
    } else {
      assert(0);
    }
  }
}

void ObjectBlock::UpdateBlockType5and7(ObjectBlock &b) {
  assert((type == 5) || (type == 7));
  b.clear();

  
}

ObjectBlock *ObjectBlock::read_new_block(std::istream &st, ERROR *p_error,
                                         unsigned int *p_trailing_frames)
{
  ObjectBlock *block = new ObjectBlock();

  ERROR error = ERROR::NONE;

  block->Block::read(st);
  error = block->getError();

  if (error == ERROR::NONE) {
    block->DecodeBlock();
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

  bool any_legacy_blocks() const;

  void upgrade();

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

bool ObjectFile::any_legacy_blocks() const {
  for (auto block: blocks) {
    if (block.legacy_block()) {
      return true;
    }
  }
  return false;
}

void ObjectFile::upgrade() {
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
  } else if (action == ACTION::UPGRADE) {
    ObjectFile &file = *object_files.begin();
    if (file.any_legacy_blocks()) {
      file.upgrade();
    } else {
      std::cerr << "Warning: There are no legacy blocks to updgrade" << std::endl;
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
    
  } else if (action == ACTION::CONCATENATE) {
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
