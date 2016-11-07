#!/bin/bash
HERE="`dirname "$0"`"
THE_RANGE="7 11 16 25 37"
SIGS=
mkdir -p "$HERE/rd_cache"
for V in $THE_RANGE
do
  SIG=`$HERE/qm_signature.sh $V`
  OUTPUT="$HERE/rd_cache/$SIG"
  if ! [ -e "$OUTPUT.out" ]
  then
    rm -f *-daala.out total.out
    RANGE=$V $HERE/tools/rd_collect.sh daala "$@" 2>&1 |
      awk '{print ""}' ORS=. >&2; echo >&2
    export OUTPUT
    $HERE/tools/rd_average.sh *-daala.out
  fi
  SIGS="$SIGS $OUTPUT.out"
done
cat $SIGS
