//
// Search list for minimum value
// Arm NEON performance demo
//
// written by Larry Bank
//
#include <arm_neon.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define LIST_SIZE 10000000

//
//  Returns the relative time in microseconds
//
int Micros(void)
{
int iTime;
struct timespec res;

    clock_gettime(CLOCK_MONOTONIC, &res);
    iTime = 1000000*res.tv_sec + res.tv_nsec/1000;

    return iTime;
} /* Micros() */

//
// Find minimum value (scalar)
// returns the index of the minimum value
//
int scalar_find(int32_t *pList, int iCount)
{
int iMinPos=0, iMin = 0x7fffffff;
 
   for (int i=0; i<iCount; i++) {
      if (pList[i] < iMin) {
         iMin = pList[i];
         iMinPos = i;
      }
   } // for
   return iMinPos;
} /* scalar_find() */

//
// Find minimum value (vector)
// returns the index of the minimum value
//
int vector_find(int32_t *pList, int iCount)
{
int32x4_t vIn, vMin, vMinIndices, vIndices, vIncrement;
int32x2_t vMin_2, vMinIndex_2;
uint32x2_t vMask_2;
uint32x4_t vMask;
int iMinIndex;// , iMin;
const int32_t start_indices[] = {0,1,2,3};

   vIndices = vld1q_s32(start_indices);
   vIncrement = vdupq_n_s32(4);
   vMin = vdupq_n_s32(0x7fffffff); // set to max integer value to start

   for (int i=0; i<iCount; i+=4) {
      vIn = vld1q_s32(&pList[i]);
      vMask = vcltq_s32(vIn, vMin); // which lanes are less?
      vMin = vminq_s32(vIn, vMin); // keep the minimum values
      vMinIndices = vbslq_s32(vMask, vIndices, vMinIndices); // select min indices
      vIndices = vaddq_s32(vIndices, vIncrement); // update current indices
   } // for
   // Now we have 4 minimums and indices; find the min value + index
   vMask_2 = vclt_s32(vget_low_s32(vMin), vget_high_s32(vMin));
   vMin_2 = vmin_s32(vget_low_s32(vMin), vget_high_s32(vMin));
   vMinIndex_2 = vbsl_s32(vMask_2, vget_low_s32(vMinIndices), vget_high_s32(vMinIndices));
   vMask_2 = vclt_s32(vMin_2, vrev64_s32(vMin_2));
   vMin_2 = vmin_s32(vMin_2, vrev64_s32(vMin_2));
   vMinIndex_2 = vbsl_s32(vMask_2, vMinIndex_2, vrev64_s32(vMinIndex_2));
   // Now we have the final min and index
//   iMin = vget_lane_s32(vMin_2, 0);
   iMinIndex = vget_lane_s32(vMinIndex_2, 0);

   return iMinIndex;
} /* vector_find() */

int vector_find_unrolled(int32_t *pList, int iCount)
{
int32x4_t vIn0, vIn1, vMin0, vMin1, vMinIndices0, vMinIndices1, vIndices0, vIndices1, vIncrement;
int32x2_t vMin_2, vMinIndex_2;
uint32x2_t vMask_2;
uint32x4_t vMask0, vMask1;
int iMinIndex;// , iMin;
const int32_t start_indices[] = {0,1,2,3};

   vIndices0 = vld1q_s32(start_indices);
   vIndices1 = vaddq_s32(vIndices0, vdupq_n_s32(4));
   vIncrement = vdupq_n_s32(8);
   vMin0 = vMin1 = vdupq_n_s32(0x7fffffff); // set to max integer value to start

   for (int i=0; i<iCount; i+=8) {
      vIn0 = vld1q_s32(&pList[i]);
      vIn1 = vld1q_s32(&pList[i+4]);
      vMask0 = vcltq_s32(vIn0, vMin0); // which lanes are less?
      vMask1 = vcltq_s32(vIn1, vMin1);
      vMin0 = vminq_s32(vIn0, vMin0); // keep the minimum values
      vMin1 = vminq_s32(vIn1, vMin1);
      vMinIndices0 = vbslq_s32(vMask0, vIndices0, vMinIndices0); // select min indices
      vMinIndices1 = vbslq_s32(vMask1, vIndices1, vMinIndices1);
      vIndices0 = vaddq_s32(vIndices0, vIncrement); // update current indices
      vIndices1 = vaddq_s32(vIndices1, vIncrement);
   } // for
   // combine the 8 into 4
   vMask0 = vcltq_s32(vMin0, vMin1);
   vMin0 = vminq_s32(vMin0, vMin1);
   vMinIndices0 = vbslq_s32(vMask0, vMinIndices0, vMinIndices1);
   // Now we have 4 minimums and indices; find the min value + index
   vMask_2 = vclt_s32(vget_low_s32(vMin0), vget_high_s32(vMin0));
   vMin_2 = vmin_s32(vget_low_s32(vMin0), vget_high_s32(vMin0));
   vMinIndex_2 = vbsl_s32(vMask_2, vget_low_s32(vMinIndices0), vget_high_s32(vMinIndices0));
   vMask_2 = vclt_s32(vMin_2, vrev64_s32(vMin_2));
   vMin_2 = vmin_s32(vMin_2, vrev64_s32(vMin_2));
   vMinIndex_2 = vbsl_s32(vMask_2, vMinIndex_2, vrev64_s32(vMinIndex_2));
   // Now we have the final min and index
//   iMin = vget_lane_s32(vMin_2, 0);
   iMinIndex = vget_lane_s32(vMinIndex_2, 0);

   return iMinIndex;
} /* vector_find_unrolled() */
int main(int argc, char **argv)
{
int32_t *pList;
int iMinVal, iTime;

   pList = malloc(LIST_SIZE * sizeof(int32_t));
   if (pList == NULL) {
      printf("Memory allocation failed, exiting...");
      return -1;
   }

   printf("List search perf demo\n");
   for (int i=0; i<LIST_SIZE; i++) {
      pList[i] = (int32_t)rand();
   }
   iTime = Micros();
   iMinVal = scalar_find(pList, LIST_SIZE);
   iTime = Micros() - iTime;
   printf("Scalar time = %dus, value = %d\n", iTime, iMinVal);

   iTime = Micros();
   iMinVal = vector_find(pList, LIST_SIZE);
   iTime = Micros() - iTime;
   printf("Vector time = %dus, value = %d\n", iTime, iMinVal);

   iTime = Micros();
   iMinVal = vector_find_unrolled(pList, LIST_SIZE);
   iTime = Micros() - iTime;
   printf("Vector (unrolled) time = %dus, value = %d\n", iTime, iMinVal);

   free(pList);
   return 0;
} /* main() */
