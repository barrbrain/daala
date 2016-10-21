#!/bin/bash
LAST_OD_LUMA_QM_Q6=(80 129 130 66 89 98 52 52 62)

if [ -e last_run ]
then
  LAST_OD_LUMA_QM_Q6=( `cat last_run` )
fi

function run_file {
  local IFS=_
  echo "$*".out
}

last_run=`run_file ${LAST_OD_LUMA_QM_Q6[@]}`
if ! [ -e $last_run ]
then
  rm -f *-daala.out total.out
  OD_LUMA_QM_Q6="${LAST_OD_LUMA_QM_Q6[@]}" \
    ./rd_cache.sh subset3-mono/*.y4m > tmp.out
  mv tmp.out $last_run
fi

HIT=1

while [ $HIT = 1 ]
do
  HIT=0
  for step in 1 -1
  do
    for (( i=0; i < 9; ++i ))
    do
      OD_LUMA_QM_Q6=( ${LAST_OD_LUMA_QM_Q6[@]} )
      OD_LUMA_QM_Q6[i]=$(( OD_LUMA_QM_Q6[i] + $step ))
      last_run=`run_file ${LAST_OD_LUMA_QM_Q6[@]}`
      this_run=`run_file ${OD_LUMA_QM_Q6[@]}`
      if ! [ -e $this_run ]
      then
        rm -f *-daala.out total.out
        OD_LUMA_QM_Q6="${OD_LUMA_QM_Q6[@]}" \
          ./rd_cache.sh subset3-mono/*.y4m > tmp.out
        mv tmp.out $this_run
      fi
      ./tools/bd_rate.sh $last_run $this_run | tee bd_rate
      while grep -qF ' SSIM 0 0' bd_rate && grep -qF 'TSSIM 0 0' bd_rate &&
            grep -qF 'NRHVS 0 0' bd_rate && grep -qF ' PSNR 0 0' bd_rate
      do
        OD_LUMA_QM_Q6[i]=$(( OD_LUMA_QM_Q6[i] + $step ))
        this_run=`run_file ${OD_LUMA_QM_Q6[@]}`
        if ! [ -e $this_run ]
        then
          rm -f *-daala.out total.out
          OD_LUMA_QM_Q6="${OD_LUMA_QM_Q6[@]}" \
            ./rd_cache.sh subset3-mono/*.y4m > tmp.out
          mv tmp.out $this_run
        fi
        ./tools/bd_rate.sh $last_run $this_run | tee bd_rate
      done
      if grep -qF 'PSNRHVS -' bd_rate
      then
        LAST_OD_LUMA_QM_Q6=( ${OD_LUMA_QM_Q6[@]} )
        HIT=1
        echo ${LAST_OD_LUMA_QM_Q6[@]} > last_run
      fi
    done
  done
done
