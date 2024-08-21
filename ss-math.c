/*
 * Copyright (c) 2014, University of Delaware
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "ss-math.h"
#include "ss-conf.h"

FLOAT_TYPE computeAverage(FLOAT_TYPE* data, u64 count)
{
    FLOAT_TYPE avg = 0;
    for(u64 i=0; i<count; i++)
        avg += data[i];
    return avg/count;
}

FLOAT_TYPE computeVariance(FLOAT_TYPE* data, FLOAT_TYPE avg, u64 count)
{
    FLOAT_TYPE var = 0;
    for(u64 i=0; i<count; i++)
        var += (data[i]-avg)*(data[i]-avg); //let the compiler hoist.
    return var/count;
}

FLOAT_TYPE computeCovariance(FLOAT_TYPE* data, FLOAT_TYPE avg, FLOAT_TYPE* data2, FLOAT_TYPE avg2, u64 count)
{
    FLOAT_TYPE var = 0;
    for(u64 i=0; i<count; i++)
        var += (data[i]-avg)*(data2[i]-avg2);
    return var/count;
}

FLOAT_TYPE computeSkew(FLOAT_TYPE* data, FLOAT_TYPE avg, u64 count)
{
    FLOAT_TYPE skew = 0;
    for(u64 i=0; i<count; i++)
        skew += (data[i]-avg)*(data[i]-avg)*(data[i]-avg); //let the compiler hoist.
    return skew/count;
}
