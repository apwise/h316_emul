#include "h16cmd.hh"

H16Cmd::H16Cmd(H16Cmd::TYPE type)
  : type(type),
    argc(0)
{
  args.i = 0;
}

H16Cmd::~H16Cmd()
{
}

