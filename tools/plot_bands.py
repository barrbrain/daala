#!/usr/bin/env python
import sys
import numpy as np
import matplotlib.pyplot as plt
from scipy import stats

def gen_plot(ax, fn):
  print(fn)
  data = np.loadtxt(fn,delimiter=',')
  values=np.vstack((np.log2(data.T[0]),data.T[1]))
  xmin = 0
  ymin = 0
  xmax = 17
  ymax = 1
  X, Y = np.mgrid[xmin:xmax:100j, ymin:ymax:100j]
  positions = np.vstack([X.ravel(), Y.ravel()])
  kernel = stats.gaussian_kde(values)
  Z = np.reshape(kernel(positions).T, X.shape)
  ax.set_xlim([2,17])
  ax.set_ylim([0,1])
  ax.imshow(np.rot90(Z), extent=[xmin, xmax, ymin, ymax], aspect='auto')

f, axarr = plt.subplots(4, 5, sharex=True, sharey=True)
gen_plot(axarr[0, 0], 'band_3_100k')
gen_plot(axarr[0, 1], 'band_4_100k')
gen_plot(axarr[0, 2], 'band_5_100k')
gen_plot(axarr[0, 3], 'band_7_100k')
gen_plot(axarr[0, 4], 'band_8_100k')
gen_plot(axarr[1, 0], 'band_9_100k')
gen_plot(axarr[1, 1], 'band_10_100k')
gen_plot(axarr[1, 2], 'band_11_100k')
gen_plot(axarr[1, 3], 'band_13_100k')
gen_plot(axarr[1, 4], 'band_14_100k')
gen_plot(axarr[2, 0], 'band_15_100k')
gen_plot(axarr[2, 1], 'band_16_100k')
gen_plot(axarr[2, 2], 'band_17_100k')
gen_plot(axarr[2, 3], 'band_18_100k')
gen_plot(axarr[2, 4], 'band_21_100k')
gen_plot(axarr[3, 0], 'band_22_100k')
gen_plot(axarr[3, 1], 'band_23_100k')
gen_plot(axarr[3, 2], 'band_24_100k')
gen_plot(axarr[3, 3], 'band_25_100k')
gen_plot(axarr[3, 4], 'band_26_100k')
plt.show()
