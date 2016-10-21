#!/bin/sh
HERE="`dirname "$0"`"
{
  DUMP_OD_LUMA_QM_Q6=1 $HERE/examples/encoder_example -v$1 -o /dev/null $HERE/zero.y4m 2>&1 |
  grep -F OD_LUMA_QM_Q6
  echo $1
} | sha1sum - | head -c8
