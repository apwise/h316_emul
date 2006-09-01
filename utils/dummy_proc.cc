#include <stdio.h>
#include "dummy_proc.hh"
#include "asr.hh"

Proc::Proc(ASR *asr)
{
	this->asr = asr;
}

bool Proc::special(char k)
{
	return asr->special(k);
}
