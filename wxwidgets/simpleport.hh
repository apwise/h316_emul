#ifndef _SIMPLE_PORT_
#define _SIMPLE_PORT_

class SimplePort
{
public:
  SimplePort(){}
  virtual ~SimplePort(){}
  virtual void Process(unsigned char ch, int source=0) = 0;
};

#endif // _SIMPLE_PORT_
