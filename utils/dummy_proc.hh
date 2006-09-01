/* Dummy proc.hh */

#define STANDALONE 1

class ASR;

class Proc
{
public:
	Proc(ASR *asr);
	bool special(char k);

private:
	ASR *asr;
};
