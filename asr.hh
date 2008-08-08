/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005  Adrian Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307 USA
 *
 */
class STDTTY;

#define ASR_PTR 0
#define ASR_PTP 1

class ASR
{
public:
  ASR(STDTTY *stdtty);
  bool get_asrch(char &c);
  void put_asrch(char c);
  void set_filename(char *filename, bool asr_ptp);
  void asr_ptp_on(char *filenamep);
  void asr_ptr_on(char *filenamep);
  bool file_input(){return (running[ASR_PTR]);};

  bool special(char c);
private:
  STDTTY *stdtty;

  FILE *fp[2];
  char *filename[2];
  bool pending_filename[2];
  bool running[2];
  bool ascii_file[2];
  bool silent_file[2];
  int char_count[2];

  bool tape_char_received;
  bool xoff_received;
  void clear_ptp_flags();
  void open_punch_file();

  bool insert_lf;
  bool stop_after_next;
  bool xoff_read;
  void clear_ptr_flags();
  void open_reader_file();

  void get_filename(bool asr_ptp);
  void close_file(bool asr_ptp);
  void echo_asrch(char c, bool from_serial);

};

