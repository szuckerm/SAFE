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


#include <cstdio>
#include "ss-msr.h"
#include "ss-agent.h"
extern "C" {
#include "ss-temp.h"
}

thread_local u64 cycle;
thread_local bool maxOperatingTempWarning;
thread_local bool maxChipTempWarning;

auto readClockMSR() -> u64
{
    return cycle;
}

/*Updates the clock -- should only be done in a single engine.*/
auto updateClockMSR() -> void
{
    cycle++; //increment the cycle count.

    //Sanity check.
    if (cycle == (u64)-1)
        printf("unt%ld.blk%ld: WARNING: Cycle count overflow!\n", agent.uid, agent.bid);
}

auto readPowerMSR() -> FLOAT_TYPE
{
    return agent.accumulatedEnergy*(MAX_XE_CLOCK_SPEED_MHZ*1E-6/ROLLING_ENERGY_WINDOW/INST_PER_MEGA_INST); //normalize for picojoules and megahertz.
}

/*Updates the current energy based on the currently running tasks and state of each XE.*/
///auto updatePowerMSR() -> void
///{
///    //Cycle each XE adding/subtracting energy deltas.
///    FLOAT_TYPE energyDelta = 0.0;
///    for(u64 id=0; id<N_CORES_IN_BLOCK; id++) 
///    {
///        u16 state = agent.xe[id].state;     //grab the state.
///        TaskType*& task = agent.xe[id].task; //grab the current task.
///        InstType * currentInstruction = & agent.xe[id].currentInstruction;
///        FLOAT_TYPE * accumEnergy4Half = &(agent.xe[id].accumEnergy4Half);
///
///        //it's necessary to make sure there is some task in execution
///        if (task!=NULL)
///        {
///            if (state != XE_STATE_NONE)
///            {
///                //Execute.
///                if (state == XE_STATE_FULL)
///                {
///                    energyDelta += currentInstruction->fullStateEnergy;
///                    energyDelta -= *accumEnergy4Half;
///                    *accumEnergy4Half = 0;
///                }
///                else if (state == XE_STATE_HALF)
///                {
///                    //When a block is in HALF state, each cycle a portion of the energy is stored. If it changes, 
///                    //the accumEnergy4Half will be subtracted from the full energy to avoid putting more energy 
///                    //than what it is (Although, DVFS change takes energy too, isn't it???)
///                    // energyDelta += (currentInstruction->halfStateEnergy*(1+STATIC_ENERGY_FACTOR_HALF))/currentInstruction->multiplier;
///                    *accumEnergy4Half += (currentInstruction->halfStateEnergy)/currentInstruction->multiplier;
///                }
///            }
///        }
///        else
///        {
///            if (state == XE_STATE_FULL)
///                energyDelta += noopInstruction.fullStateEnergy;
///            else if (state == XE_STATE_HALF)
///                energyDelta += (noopInstruction.halfStateEnergy)/currentInstruction->multiplier;
///        }
///    }
///    
///    //Push energy to rolling window for power computations.
///    agent.energyWindow.push_front(energyDelta);
///    if (agent.energyWindow.size() >= ROLLING_ENERGY_WINDOW)
///    {
///        if (agent.accumulatedEnergy == 0.0)
///        {
///            for(u64 i=0; i<ROLLING_ENERGY_WINDOW; i++)
///                agent.accumulatedEnergy += agent.energyWindow[i];
///        }
///        else
///        {
///            agent.accumulatedEnergy -= agent.energyWindow.back();
///            agent.accumulatedEnergy += agent.energyWindow.front();
///        }
///        agent.energyWindow.pop_back();
///    }
///}

u8 readTemperatureMSR()
{
    return TEMPERATURE_JUNCTION - (u64)agent.temperature; //report temperatures in terms of delta.
}

/*Updates the current temperature using the current front of the energy Window and neighbor temperatures.*/
auto updateTemperatureMSR() -> void
{
    //Grab the energy for this cycle.
    static thread_local FLOAT_TYPE energy = 0.0;
    if (agent.energyWindow.empty() == false)
            energy += agent.energyWindow.front();

    if(readClockMSR() % 2 == 0)
    {
        //Update the temperature.
        computeTemperature(energy, 2);
        energy = 0.0;
        if(agent.temperature > TEMPERATURE_JUNCTION) //maximum possible junction temperature.
        {
            agent.temperature = TEMPERATURE_JUNCTION;
            if (!maxOperatingTempWarning)
            {
              //printf("unt%ld.blk%ld: WARNING: exceeded junction temperatures, your chip is a mushroom cloud!\n", agent.uid, agent.bid);
              maxOperatingTempWarning=true;
            }
        }
        else if(agent.temperature > TEMPERATURE_OPERATION) //maximum operating temperature allowed.
        {
            if(!maxChipTempWarning)
            {
              //printf("unt%ld.blk%ld: WARNING: exceeded allowed operating temperature (%lfC)!\n", agent.uid, agent.bid, agent.temperature);
              maxChipTempWarning=true;
            }
        }
        else if(agent.temperature < 50.0)
        {
            agent.temperature = TEMPERATURE_AMBIENT;
            maxChipTempWarning=false;
            maxOperatingTempWarning=false;
        }
        else
        {
            maxChipTempWarning=false;
            maxOperatingTempWarning=false;
        }
    }
}
