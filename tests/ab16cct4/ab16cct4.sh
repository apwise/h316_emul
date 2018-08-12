#! /bin/bash

workspace="$( cd `dirname $0`/../..; pwd )"

. ../scripts/jshutest.inc

jshu_pkgname=h316_emul
BUILDDIR=${workspace}

ab16cct4_Test()
{
  ../../install/bin/h16 -t ab16cct4.txt > logfile.txt
  if diff logfile.txt expected.txt; then
      return ${jshuPASS}
  else
      return ${jshuFAIL}
  fi
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
