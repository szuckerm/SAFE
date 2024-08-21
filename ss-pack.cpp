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


#include "ss-pack.h"
#include "ss-agent.h"
#include <assert.h>
#include <stdio.h>

/* C style declarations because these are used by the SA API. */
thread_local bool overTemperatureWarning;
/*Packs the temperature data into network form.*/
u8 packTemperatureData(FLOAT_TYPE delta)
{
    ///printf("Packed Temperature Argument %.2lf\n", delta);
    if(delta > TEMPERATURE_OPERATION-TEMPERATURE_AMBIENT)
    {
	if (!overTemperatureWarning)
	{
	  //printf("unt%ld.blk%ld: WARNING: temperature delta exceeded allowed value (%lfC)!\n", agent.uid, agent.bid, delta);
	  overTemperatureWarning=true;
	}
    }
    else
    {
      overTemperatureWarning=false;
    }
    assert(delta <= TEMPERATURE_OPERATION-TEMPERATURE_AMBIENT); //sanity check.

    u8 data = delta; //Warning: potential for clipping.
    u8 frac = (delta - data)*100;

    data <<= 2; //shift to pack the natural bits.

    //Pack fractional bits -- round down (leads to higher temperature estimates).
    if (frac > 75)
        data += (1<<2); ///increment natural number.
    else if (frac > 50)
        data |= 0b11;
    else if (frac > 25)
        data |= 0b10;
    else if (frac > 0)
        data |= 0b01;

    return data;
}

/*Unpacks the temperature data in network form.*/
FLOAT_TYPE unpackTemperatureData(u8 data)
{
    u8 frac = data & 0b11;
    FLOAT_TYPE temperature;

    //decode the fractional bits.
    if (frac == 0b00)
        temperature = 0.00;
    else if (frac == 0b01)
        temperature = 0.25;
    else if (frac == 0b10)
        temperature = 0.50;
    else if (frac == 0b11)
        temperature = 0.75;

    //decode the natural bits.
    temperature += (data >> 2);

    return temperature;
}

/*Packs the skew data into network form.*/
u8 packSkewData(FLOAT_TYPE skew)
{
    s64 data = skew;
    data /= 100; //Shift out LSBs.
//TODO FIX THIS SANITY CHECK FOR THE NEW SIZE
//    assert(data<128); //sanity check.
//    assert(data>-128);

    return data;
}

/*Unpacks the skew data in network form.*/
FLOAT_TYPE unpackSkewData(u8 data)
{
    s64 skew = *(s8*)&data; //interpret at signed 8 bit number.
    skew *= 100; //Shift in LSBs.

    return skew; //Warning: implicit conversion.
}

/*Pack the power data in network form.*/
/*
    Encodes power as multiples of the maxPower and bitwidth.
    For example If you have a maxPower=6.5, and bitWidth = 2 you have following
    possible encoded values: 1.625, 3.25, 4.875, 6.5. Note that this scheme
    rounds up because there is an implicit assumption that you can never have
    zero power, thus zero does not need to be encoded.
*/
u16 packPowerData(FLOAT_TYPE power, FLOAT_TYPE maxPower, u16 bitWidth)
{
    if (power>maxPower)
    {
      printf("unt%ld.blk%ld: WARNING: Maximum Power exceded. (%lf)!\n", agent.uid, agent.bid, power);
      power=maxPower;
    }

    //Notch based on the number of bits.
    FLOAT_TYPE notches = 1<<bitWidth;
//  FLOAT_TYPE notches = 2;
//  for (u16 n=1; n<bitWidth; n++)
//      notches *= 2;

    //Find the nearest notch (clips to the nearest-- rounding up).
    return (u16)(notches*power/maxPower);
}

/*Unpacks the power data in network form.*/
FLOAT_TYPE unpackPowerData(u16 notch, FLOAT_TYPE maxPower, u16 bitWidth)
{
    //Notch based on the number of bits.
    FLOAT_TYPE notches = 1<<bitWidth;
//  FLOAT_TYPE notches = 2;
//  for (u16 n=1; n<bitWidth; n++)
//      notches *= 2;

    // plus 1 because zero is not encoded.
    return (notch+1)*(maxPower/notches);
}
