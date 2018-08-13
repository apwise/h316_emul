#! /bin/bash

workspace="$( cd `dirname $0`/../..; pwd )"

. ../scripts/jshutest.inc

jshu_pkgname=h316_emul
BUILDDIR=${workspace}

ab16cct4_Test()
{
  res=${jshuERROR}
  ../../install/bin/h16 -t ab16cct4.txt |& tee logfile.txt
  if diff logfile.txt ab16cct4_expected.txt; then
      res=${jshuPASS}
  else
      res=${jshuFAIL}
  fi
  rm -f logfile.txt
  return ${res}
}

o16_11t1_Test()
{
  res=${jshuERROR}
  ../../install/bin/h16 -t o16_11t1.txt |& tee logfile.txt
  if diff logfile.txt o16_11t1_expected.txt; then
      res=${jshuPASS}
  else
      res=${jshuFAIL}
  fi
  rm -f logfile.txt
  return ${res}
}

##############################################################
# main
##############################################################
# initialize testsuite
jshuInit

# run unit tests in this script
jshuRunTests

# result summary
jshuFinalize

echo Done.
echo
let tot=failed+errors
exit $tot
