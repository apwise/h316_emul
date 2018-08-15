#! /bin/bash

workspace="$( cd `dirname $0`/../..; pwd )"

. ../scripts/jshutest.inc

jshu_pkgname=h316_emul
BUILDDIR=${workspace}

run_vt()
{
  name=$1
  rm -f logfile.txt
  res=${jshuERROR}
  ${workspace}/install/bin/h16 -t ${name}.txt |& tee logfile.txt
  if diff logfile.txt ${name}_expected.txt; then
      res=${jshuPASS}
  else
      res=${jshuFAIL}
  fi
  rm -f logfile.txt

  return ${res}
}

ab16_cct4_Test()
{
  run_vt ab16_cct4
  return $?
}

o16_11t1_Test()
{
  run_vt o16_11t1
  return $?
}

ab16_cmt5_Test()
{
  run_vt ab16_cmt5
  return $?
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
