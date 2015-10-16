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
  After 0, which indicates lossless, quantizers are computed by
   trunc(e(((cq-((cq-1)**2.25)/782.89249)-6.235)*.10989525)*(1<<4)),
   which features strictly equal-or-increasing interval spacing.
  This gives us a decreasingly log-spaced range of 9 to 1802
  (representing .5625 to 112.625 coded in Q4),
  with quantizer initially slightly less than doubling every
   six steps and slowing to approximately sqrt(2). */
  0x0000,
  /*1*/
  0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000F,
  /*7*/
  0x0011, 0x0013, 0x0015, 0x0017, 0x001A, 0x001D,
  /*13*/
  0x0020, 0x0023, 0x0027, 0x002B, 0x0030, 0x0035,
  /*19*/
  0x003B, 0x0041, 0x0047, 0x004F, 0x0057, 0x005F,
  /*25*/
  0x0069, 0x0073, 0x007E, 0x008A, 0x0097, 0x00A5,
  /*31*/
  0x00B4, 0x00C5, 0x00D7, 0x00EA, 0x00FF, 0x0115,
  /*37*/
  0x012D, 0x0146, 0x0162, 0x017F, 0x019E, 0x01C0,
  /*43*/
  0x01E4, 0x020A, 0x0232, 0x025D, 0x028B, 0x02BB,
  /*49*/
  0x02EE, 0x0324, 0x035D, 0x0399, 0x03D8, 0x041B,
  /*55*/
  0x0461, 0x04AA, 0x04F6, 0x0546, 0x0599, 0x05F0,
  /*61*/
  0x064A, 0x06A8, 0x070A
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

