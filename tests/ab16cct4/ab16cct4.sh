#! /bin/sh
../../install/bin/h16 -t ab16cct4.txt > logfile.txt
diff logfile.txt expected.txt
