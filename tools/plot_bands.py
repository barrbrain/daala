#!/usr/bin/env python
import numpy as np
import matplotlib.pyplot as plt
from scipy import stats

qmap = range(9, 600)[::-1]


def k_from_g_log2(n, log2_g, q0, beta):
    shift = 4 if beta > 1 else 0
    return (log2_g * (1. / beta) - np.log2(q0) + 0.5 * np.log2(n + 3) - 0.5 - np.log2(beta) + shift).clip(0)


def rate_est(n, log2_k, f):
    k = np.exp2(log2_k)
    return (0.881 * n / np.log(2)) * (1 + 0.66 * f) * np.log1p((np.log(n * 2 * (0.52 * f + 0.017)) * k * (1. / n)).clip(0)) + 10.4


xmin = 0
ymin = 0
xmax = 17
ymax = 1
X, Y = np.mgrid[xmin:xmax:100j, ymin:ymax:100j]
positions = np.vstack([X.ravel(), Y.ravel()])


def gen_plot(ax, fn, n):
    print(fn)
    data = np.loadtxt(fn, delimiter=' ')
    values = np.vstack((np.log2(data.T[1]), data.T[0]))
    kernel = stats.gaussian_kde(values)
    Z = np.reshape(kernel(positions).T, X.shape)
    Z /= Z.sum()
    log2_qmap = np.log2(qmap)
    non_am = np.array([np.multiply(rate_est(n, k_from_g_log2(n, X, q0, 1), Y), Z).sum() for q0 in qmap])
    am = np.array([np.multiply(rate_est(n, k_from_g_log2(n, X, q0, 1.5), Y), Z).sum() for q0 in qmap])
    am_inv = np.interp(np.log2(non_am), np.log2(am), log2_qmap) - log2_qmap
    print(np.round(np.exp2(am_inv[-290:-10].mean()) * 16))
    ax.set_xlim([2, 17])
    ax.set_ylim([0, 1])
    ax.imshow(np.rot90(Z), extent=[xmin, xmax, ymin, ymax], aspect='auto')


f, axarr = plt.subplots(4, 5, sharex=True, sharey=True)
gen_plot(axarr[0, 0], 'band_3_100k', 15)
gen_plot(axarr[0, 1], 'band_4_100k', 8)
gen_plot(axarr[0, 2], 'band_5_100k', 32)
gen_plot(axarr[0, 3], 'band_7_100k', 15)
gen_plot(axarr[0, 4], 'band_8_100k', 8)
gen_plot(axarr[1, 0], 'band_9_100k', 32)
gen_plot(axarr[1, 1], 'band_10_100k', 32)
gen_plot(axarr[1, 2], 'band_11_100k', 128)
gen_plot(axarr[1, 3], 'band_13_100k', 15)
gen_plot(axarr[1, 4], 'band_14_100k', 8)
gen_plot(axarr[2, 0], 'band_15_100k', 32)
gen_plot(axarr[2, 1], 'band_16_100k', 32)
gen_plot(axarr[2, 2], 'band_17_100k', 128)
gen_plot(axarr[2, 3], 'band_18_100k', 128)
gen_plot(axarr[2, 4], 'band_21_100k', 15)
gen_plot(axarr[3, 0], 'band_22_100k', 8)
gen_plot(axarr[3, 1], 'band_23_100k', 32)
gen_plot(axarr[3, 2], 'band_24_100k', 32)
gen_plot(axarr[3, 3], 'band_25_100k', 128)
gen_plot(axarr[3, 4], 'band_26_100k', 128)
plt.show()
