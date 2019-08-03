#ifndef _MESSAGE_HH_
#define _MESSAGE_HH_

/*
 * Commands produced by user interfaces
 */
class Message
{
public:

  enum DEST
    {
      NO_DEST
    };

  enum TYPE
    {
      NONE,
      GO
    };

  Message(DEST dest=NO_DEST, TYPE type = NONE);
  ~Message();

  DEST get_dest() const {return dest;};
  TYPE get_type() const {return type;};


  class Argument
  {
  public:
    enum ARG_TYPE
      {
	ARG_NONE,
	ARG_INT,
	ARG_LIST
      };

    union Value
    {
      int i;
      Argument **argv;
    };

    Argument();                  // Empty argument
    Argument(int i);             // Simple integer argument
    Argument(const Argument &a); // Copy constructor
    Argument &operator=(const Argument &a);
    ~Argument();
    
  private:
    ARG_TYPE type;
    Value val;
  };

private:
  enum DEST dest;
  enum TYPE type;
  int argc;
  Argument arg;
};

#endif

