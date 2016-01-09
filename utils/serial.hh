
struct termios;

class Serial
{
public:
  Serial();
  ~Serial();
  int get_fd(){return fd;};
  char receive(bool &ok);
  void transmit(char c, bool &ok);

private:
  int fd;
  struct termios *oldtio, *newtio;
  unsigned int character_time;
};
