#ifndef _H16CMD_HPP_
#define _H16CMD_HPP_

/*
 * Commands produced by user interfaces
 */
class H16Cmd
{
public:
  enum TYPE
    {
      NONE,
      GO
    };

  H16Cmd(TYPE type = NONE);
  ~H16Cmd();

  TYPE get_type() const {return type;};

private:
  enum TYPE type;
  int argc;
  union {
    int i;
    void *p;
  } args;
};

#endif

