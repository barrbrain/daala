import random
import os
import subprocess
import re

def step(n):
  if n == 0: return random.choice([-1, 1])
  return n + (1, -1)[n < 0]

def perturb():
  with open('src/perturb.h', 'r') as f:
    lines = [l.split() for l in f.read().splitlines()]
  choice = random.randrange(len(lines)-1)
  lines[choice][-1] = str(step(int(lines[choice][-1])))
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
while True:
  perturb()
  os.system('make -j')
  NEXT = subprocess.check_output('git rev-parse HEAD'.split()).decode().strip()
  os.system('./tools/rd_collect.sh daala subset3-mono/*.y4m')
  os.system('OUTPUT=%s ./tools/rd_average.sh *-daala.out' % NEXT)
  if not test(HEAD+'.out', NEXT+'.out'):
    os.system('git checkout HEAD^')
    os.system('rm -v %s.out' % NEXT)
  else:
    HEAD = NEXT
