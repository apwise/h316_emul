
class Proc;
class STDTTY;

class PTP : public IODEV
{
 public:
  PTP(Proc *p, STDTTY *stdtty);
  bool ina(unsigned short instr, signed short &data);
  void ocp(unsigned short instr);
  bool sks(unsigned short instr);
  bool ota(unsigned short instr, signed short data);
  void smk(unsigned short mask);

  void event(int reason);
  void set_filename(char *filename);

 private:
  void master_clear(void);
  void turn_power_on(void);

  Proc *p;
  STDTTY *stdtty;

  FILE *fp;
  bool pending_filename;
  char *filename;
  
  bool ascii_file;
  bool ready;
  bool power_on;
  
  unsigned short mask;
};
