
class Proc;
class STDTTY;

class LPT : public IODEV
{
 public:
  LPT(Proc *p, STDTTY *stdtty);
  bool ina(unsigned short instr, signed short &data);
  void ocp(unsigned short instr);
  bool sks(unsigned short instr);
  bool ota(unsigned short instr, signed short data);
  void smk(unsigned short mask);

  void event(int reason);
  void set_filename(char *filename);

 private:
  void master_clear(void);
  void open_file();
  void next_line();
  void deal_pending_nl();

  Proc *p;
  STDTTY *stdtty;

  FILE *fp;
  bool pending_filename;
  bool pending_nl;
  char *filename;
  
  unsigned short mask;

  int scan_counter;
  char line[121];
  int line_number;
};
