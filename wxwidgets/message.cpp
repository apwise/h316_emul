#include "message.hh"

Message::Argument::Argument()
  : type(ARG_NONE)
{
  val.i = 0;
}

Message::Argument::Argument(int i)
  : type(ARG_INT)
{
  val.i = i;
}

Message::Argument::Argument(const Argument &a)
  : type(a.type)
{
  int i, n;

  switch (type)
    {
    case ARG_NONE:
      val.i = 0;
      break;
    case ARG_INT:
      val.i = a.val.i;
      break;
    case ARG_LIST:
      n = 0;
      while (a.val.argv[n])
	n++;

      val.argv = new Argument *[n+1];
      for (i=0; i<n; i++)
	val.argv[i] = a.val.argv[i];
      val.argv[n] = 0;
    }
}

Message::Argument &Message::Argument::operator=(const Argument &a)
{
  int i, n;
  if (this != &a)
    {
      if (type == ARG_LIST)
	delete [] val.argv;
      
      type = a.type;

      switch (type)
	{
	case ARG_NONE:
	  val.i = 0;
	  break;
	case ARG_INT:
	  val.i = a.val.i;
	  break;
	case ARG_LIST:
	  n = 0;
	  while (a.val.argv[n])
	    n++;
	  
	  val.argv = new Argument *[n+1];
	  for (i=0; i<n; i++)
	    val.argv[i] = a.val.argv[i];
	  val.argv[n] = 0;
	}
      
    }
  return *this;
}

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
