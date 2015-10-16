/*Daala video codec
Copyright (c) 2015 Daala project contributors.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include "internal.h"
#include "quantizer.h"

/*Daala codes 64 unique possible quantizers that are spaced out over a roughly
   logarithmic range.
  The table below maps coded quantizer values to actual quantizer values.
  After 0, which indicates lossless, quantizers are computed by
   trunc(e(((coded_quantizer)-6.235)*.10989525)*(1<<4)), which also happens to
   feature strictly equal-or-increasing interval spacing.
  That is, the interval between any two quantizers will never be smaller
   than a preceding interval.
  This gives us a log-spaced range of 9 to 8191 (representing .5625 to
   511.9375 coded in Q4), with quantizer slightly less than doubling every
   six steps.
  The starting point of 9 is not arbitrary; it represents the finest quantizer
   greater than .5 for a COEFF_SHIFT of 4, and results in a lossy encoding
   bitrate typically between between half and 3/4 the bitrate of lossless.*/
static const int OD_CODED_QUANTIZER_MAP_Q4[2][64]={{
  /*0*/
  0x0000,
  /*1*/
  0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000F,
  /*7*/
  0x0011, 0x0013, 0x0015, 0x0018, 0x001B, 0x001E,
  /*13*/
  0x0021, 0x0024, 0x0029, 0x002E, 0x0034, 0x003A,
  /*19*/
  0x0041, 0x0048, 0x0051, 0x005A, 0x0064, 0x0070,
  /*25*/
  0x007D, 0x008C, 0x009C, 0x00AE, 0x00C3, 0x00D9,
  /*31*/
  0x00F3, 0x010F, 0x012F, 0x0152, 0x0179, 0x01A5,
  /*37*/
  0x01D6, 0x020D, 0x0249, 0x028E, 0x02DA, 0x032E,
  /*43*/
  0x038D, 0x03F7, 0x046D, 0x04F0, 0x0583, 0x0627,
  /*49*/
  0x06De, 0x07AA, 0x088E, 0x098D, 0x0AA9, 0x0BE6,
  /*55*/
  0x0D48, 0x0ED3, 0x108C, 0x1278, 0x149D, 0x1702,
  /*61*/
  0x19AE, 0x1CAA, 0x1FFF
}, {
/*Daala codes 64 unique possible quantizers for chroma planes.
  The table below maps coded quantizer values to actual quantizer values.
  After 0, which indicates lossless, quantizers have been fitted
   to subset1 against a log-mean-CIEDE2000 metric and maximize
   the distance in BD-rate space from the following bounding curves:
   trunc(e(((cq-((cq-1)**2.5)/976.377)-6.235)*.10989525)*(1<<4))
   trunc(e(((cq-((cq-1)**2)/124)-6.235)*.10989525)*(1<<4))
   while maintaining strictly equal-or-increasing interval spacing.
  This gives us an asymptotically log-spaced range of 9 to 2419
  (representing .5625 to 151.1875 coded in Q4),
  with quantizer slightly less than doubling every
   six steps at each end and slowing in the middle. */
  0x0000,
  /*1*/
  0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E,
  /*7*/
  0x0010, 0x0012, 0x0014, 0x0016, 0x0019, 0x001C,
  /*13*/
  0x001F, 0x0022, 0x0026, 0x002A, 0x002E, 0x0032,
  /*19*/
  0x0037, 0x003D, 0x0043, 0x0049, 0x0051, 0x0059,
  /*25*/
  0x0061, 0x0069, 0x0073, 0x007E, 0x008A, 0x0097,
  /*31*/
  0x00A5, 0x00B3, 0x00C3, 0x00D4, 0x00E5, 0x00F9,
  /*37*/
  0x0110, 0x0127, 0x013E, 0x0156, 0x0171, 0x018F,
  /*43*/
  0x01AE, 0x01CF, 0x01F7, 0x021F, 0x0247, 0x026F,
  /*49*/
  0x0297, 0x02CA, 0x02FD, 0x0333, 0x037B, 0x03C3,
  /*55*/
  0x0417, 0x0483, 0x04FC, 0x0591, 0x0626, 0x06BC,
  /*61*/
  0x078A, 0x0871, 0x0973
}};

const int OD_N_CODED_QUANTIZERS =
 sizeof(*OD_CODED_QUANTIZER_MAP_Q4)/sizeof(**OD_CODED_QUANTIZER_MAP_Q4);

/*Maps coded quantizer to actual quantizer value.*/
int od_codedquantizer_to_quantizer(int cq, int is_chroma) {
  /*The quantizers above are sensible for a COEFF_SHIFT of 4 or
     greater.
    ASSERT just in case we ever try to use them for COEFF_SHIFT < 4,
     scale to COEFF_SHIFT for COEFF_SHIFT > 4.*/
  OD_ASSERT(OD_COEFF_SHIFT >= 4);
  if (cq == 0) return 0;
  return cq < OD_N_CODED_QUANTIZERS
   ? (OD_CODED_QUANTIZER_MAP_Q4[is_chroma][cq]
   << OD_COEFF_SHIFT >> 4)
   : (OD_CODED_QUANTIZER_MAP_Q4[is_chroma][OD_N_CODED_QUANTIZERS-1]
   << OD_COEFF_SHIFT >> 4);
}

/*Maps a quantizer to the largest coded quantizer with a mapped value
   less than or equal to the one passed in, except for values between 0
   (lossless) and the minimum lossy quantizer, in which case the
   minimum lossy quantizer is returned.*/
int od_quantizer_to_codedquantizer(int q, int is_chroma){
  if (q == 0) {
    return 0;
  }
  else {
    int hi;
    int lo;
    hi = OD_N_CODED_QUANTIZERS;
    lo = 1;
    /*In the event OD_COEFF_SHIFT > 4, scale the passed in quantizer
      down to Q4 from matching the shift.*/
    q = q << 4 >> OD_COEFF_SHIFT;
    while (hi > lo + 1) {
      unsigned mid;
      mid = (hi + lo) >> 1;
      if (q < OD_CODED_QUANTIZER_MAP_Q4[is_chroma][mid]) {
        hi = mid;
      }
      else {
        lo = mid;
      }
    }
    return lo;
  }
}

