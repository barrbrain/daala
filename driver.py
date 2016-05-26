import random
import os
import subprocess
import re

def gensteps(lines):
  for i in range(len(lines)):
    v = int(lines[i][-1])
    if v == 0:
      yield (i, v - 1)
      yield (i, v + 1)
    else:
      yield (i, v + (1, -1)[v < 0])

def perturb(step):
  with open('src/perturb.h', 'r') as f:
    lines = [l.split() for l in f.read().splitlines()]
  choice, value = step
  lines[choice][-1] = str(value)
  with open('src/perturb.h', 'w') as f:
    f.write('\n'.join([' '.join(l) for l in lines])+'\n')
  os.system('git add src/perturb.h')
  os.system('git commit -m PERTURB')

better = re.compile('PSNRHVS *-')
def test(before, after):
  rate = subprocess.check_output(['./tools/bd_rate.sh', before, after])
  return better.search(rate.decode()) or 'PSNRHVS 0 0' in rate.decode()
  
HEAD = subprocess.check_output('git rev-parse HEAD'.split()).decode().strip()
os.system('make -j')
os.system('./tools/rd_collect.sh daala subset3-mono/*.y4m')
os.system('OUTPUT=%s ./tools/rd_average.sh *-daala.out' % HEAD)
moving = True
while moving:
  with open('src/perturb.h', 'r') as f:
    lines = [l.split() for l in f.read().splitlines()]
  steps = [s for s in gensteps(lines)]
  random.shuffle(steps)
  moving = False
  for step in steps:
    perturb(step)
    os.system('make -j')
    NEXT = subprocess.check_output('git rev-parse HEAD'.split()).decode().strip()
    os.system('./tools/rd_collect.sh daala subset3-mono/*.y4m')
    os.system('OUTPUT=%s ./tools/rd_average.sh *-daala.out' % NEXT)
    if not test(HEAD+'.out', NEXT+'.out'):
      os.system('git checkout HEAD^')
      os.system('rm -v %s.out' % NEXT)
    else:
      os.system('git push origin HEAD:refs/hidden/pvq-q6-subset3-low')
      HEAD = NEXT
      moving = True
