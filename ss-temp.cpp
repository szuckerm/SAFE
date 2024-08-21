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


#include "ss-temp.h"
#include "ss-agent.h"
#include "ss-msr.h"

//specific heat of silicon is .71 joule/gram*kelvin
#define SPEC_HEAT_SI .71

//density of silicon is 2.33 grams/cubic centimeter
#define DENS_SI 2.33

//wafer thickness is .0775 cm and determines thermal mass and heat transfer across chip.... could be configurable but hardcoded for now
#define WAFER_THICKNESS  .0775

//watts per kelvin * meter used to derive conductance between blocks
///#define CONDUCTIVITY_SI 149

//Arbitrary value
#define CONDUCTIVITY_SI 3

//In this simulation we will use 50 micrometers between blocks... I have no idea if this is realistic and it will affect how quickly heat is spread and the temp gradients on chip
#define DIST_BETWEEN_BLOCKS .00005

//thermal resistance per square mm * second
//currently based off heatsink for a 500 mm^2 chip cooling 500 watts at 60*C over ambient (assumptions similar to what Shekar said)
//like wafer thickness, we could make this configurable but hardcoded for now
///#define THERM_R_PER_SQM (.016666667 * 1e-3)

// Calibrated resistance to make a 4.2Khz 500mm^2 chip operating at 100W heat up to 100C at which point the heat flux in/out is equal.
// Numbers used:
//  Core energy / cycle  = 11.6259765625 pJ
//  Block energy / cycle = 93.0078125 pJ
//  Chip energy / cycle  = 23810 pJ 
#define THERM_R_PER_SQM  (0.25 *1e-3)

struct
{
   FLOAT_TYPE thermal_mass;  //Information to translate between joules of heat and temperature in C
   FLOAT_TYPE thermal_r_heatsink;//constant that determines heat flow out of block in joules/degree C
   FLOAT_TYPE time_per_tick; //constant to determine real time for converting watts to joules

} temperature_sync_info;

struct
{
   FLOAT_TYPE top_bottom_r;
   FLOAT_TYPE left_right_r;

} my_therm_R = {00.0, 00.0};

void temperatureModelInit()
{
    /* Compute my thermal constants */
    {
        //.01 is multiplied to make both mm units into cm.
        temperature_sync_info.thermal_mass       = SPEC_HEAT_SI * DENS_SI * WAFER_THICKNESS * chip_layout.block_height_mm * chip_layout.block_width_mm * .01;
        temperature_sync_info.thermal_r_heatsink = (chip_layout.block_height_mm * .001) * (chip_layout.block_width_mm* .001) * (1.0/THERM_R_PER_SQM);
        temperature_sync_info.time_per_tick      = .000001/MAX_XE_CLOCK_SPEED_MHZ;
        my_therm_R.top_bottom_r                  = CONDUCTIVITY_SI * (chip_layout.block_width_mm * .001) * ( WAFER_THICKNESS * .01) / DIST_BETWEEN_BLOCKS;  //constants transform into meters.
        my_therm_R.left_right_r                  = CONDUCTIVITY_SI * (chip_layout.block_height_mm * .001) * ( WAFER_THICKNESS * .01) / DIST_BETWEEN_BLOCKS; //constants transform into meters.
    }
}

/* pass in energy in picojoules*/
FLOAT_TYPE estimateTemperature(FLOAT_TYPE energy, FLOAT_TYPE cycles)
{
    //compute temperature changes.
    FLOAT_TYPE curTemp = (energy * .000000000001) / temperature_sync_info.thermal_mass ; //temp increase from compute (constant is to get the thermal mass into picojoules)

    FLOAT_TYPE heat_buffer= (1.0) * temperature_sync_info.thermal_r_heatsink * cycles * INST_PER_MEGA_INST * CYCLES_PER_ITERATION * temperature_sync_info.time_per_tick; //heat lost through heatsink.
    curTemp -= heat_buffer / temperature_sync_info.thermal_mass; //temp drop from heatsink.

    return curTemp;
}

/* pass in energy in picojoules*/
FLOAT_TYPE computeTemperature(FLOAT_TYPE energy, FLOAT_TYPE curTemp, FLOAT_TYPE topNeighborTemp, FLOAT_TYPE btmNeighborTemp, FLOAT_TYPE lftNeighborTemp, FLOAT_TYPE rhtNeighborTemp)
{
    FLOAT_TYPE heat_buffer[5] = {0};

    //compute temperature changes.
    curTemp += (energy * .000000000001) /  temperature_sync_info.thermal_mass ; //temp increase from compute (constant is to get the thermal mass into picojoules)

    //compute heat going to neighbors (always push heat to cooler neighbors and our hotter neighbors will add heat to us)

    //////heat in joules to be transfered.
    if (topNeighborTemp != 0.0 && (curTemp-topNeighborTemp) < 0.0) { // check if neighbor exists.
        heat_buffer[0]= (curTemp-topNeighborTemp) * my_therm_R.top_bottom_r  * INST_PER_MEGA_INST * CYCLES_PER_ITERATION * temperature_sync_info.time_per_tick;
    }

    if (btmNeighborTemp != 0.0 && (curTemp-btmNeighborTemp) < 0.0) { // check if neighbor exists.
        heat_buffer[1]= (curTemp-btmNeighborTemp) * my_therm_R.top_bottom_r  * INST_PER_MEGA_INST * CYCLES_PER_ITERATION * temperature_sync_info.time_per_tick;
    }

    if (lftNeighborTemp != 0.0 && (curTemp-lftNeighborTemp) < 0.0) { // check if neighbor exists.
        heat_buffer[2]= (curTemp-lftNeighborTemp) * my_therm_R.left_right_r * INST_PER_MEGA_INST * CYCLES_PER_ITERATION * temperature_sync_info.time_per_tick;
    }

    if (rhtNeighborTemp != 0.0 && (curTemp-rhtNeighborTemp) < 0.0) { // check if neighbor exists.
        heat_buffer[3]= (curTemp-rhtNeighborTemp) * my_therm_R.left_right_r * INST_PER_MEGA_INST * CYCLES_PER_ITERATION * temperature_sync_info.time_per_tick;
    }

    //temp drop to other blocks.
    curTemp -= ( heat_buffer[0] + heat_buffer[1] + heat_buffer[2] + heat_buffer[3]) /  temperature_sync_info.thermal_mass;

    heat_buffer[4]= (curTemp-TEMPERATURE_AMBIENT) * (temperature_sync_info.thermal_r_heatsink)  * INST_PER_MEGA_INST * CYCLES_PER_ITERATION * temperature_sync_info.time_per_tick; //heat lost through heatsink.

    curTemp -= heat_buffer[4] / temperature_sync_info.thermal_mass; //temp drop from heatsink.

    return curTemp; //new temperature.
}

/* pass in energy in picojoules*/
void computeTemperature(FLOAT_TYPE energy, FLOAT_TYPE cycles)
{
    FLOAT_TYPE curTemp = agent.temperature;
    FLOAT_TYPE heat_buffer[10] = {0};

    //compute heat going to neighbors (always push heat to cooler neighbors and our hotter neighbors will add heat to us)
    
    FLOAT_TYPE normalize = cycles * INST_PER_MEGA_INST * CYCLES_PER_ITERATION * temperature_sync_info.time_per_tick;

    //////heat in joules to be transfered to neighbors..
    if (agent.topNeighbor && (curTemp-agent.topNeighbor->temperature) > 0.0) { // check if neighbor exists.
        heat_buffer[0] = -(curTemp-agent.topNeighbor->temperature) * my_therm_R.top_bottom_r  * normalize;
        agent.topNeighbor->temp.btm_push -= heat_buffer[0];
    }

    if (agent.btmNeighbor && (curTemp-agent.btmNeighbor->temperature) > 0.0) { // check if neighbor exists.
        heat_buffer[1] = -(curTemp-agent.btmNeighbor->temperature) * my_therm_R.top_bottom_r  * normalize;
        agent.btmNeighbor->temp.top_push -= heat_buffer[1];
    }

    if (agent.lftNeighbor && (curTemp-agent.lftNeighbor->temperature) > 0.0) { // check if neighbor exists.
        heat_buffer[2] = -(curTemp-agent.lftNeighbor->temperature) * my_therm_R.left_right_r * normalize;
        agent.lftNeighbor->temp.rht_push -= heat_buffer[2];
    }

    if (agent.rhtNeighbor && (curTemp-agent.rhtNeighbor->temperature) > 0.0) { // check if neighbor exists.
        heat_buffer[3] = -(curTemp-agent.rhtNeighbor->temperature) * my_therm_R.left_right_r * normalize;
        agent.rhtNeighbor->temp.lft_push -= heat_buffer[3];
    }

    //Grab pushed heat from neighbors.
    heat_buffer[4] = agent.temp.top_push; heat_buffer[5] = agent.temp.btm_push;
    heat_buffer[6] = agent.temp.lft_push; heat_buffer[7] = agent.temp.rht_push;

    //Add cool down.
    heat_buffer[8] = (TEMPERATURE_AMBIENT-curTemp) * temperature_sync_info.thermal_r_heatsink * normalize;

    //Add heat.
    heat_buffer[9] = (energy * .000000000001);

    //Add buffer heat changes to temperature.
    curTemp += ((heat_buffer[4]-agent.temp.top_pull) + (heat_buffer[5]-agent.temp.btm_pull) 
                       +  (heat_buffer[6]-agent.temp.lft_pull) + (heat_buffer[7]-agent.temp.rht_pull)
                       +  heat_buffer[0] +  heat_buffer[1] +  heat_buffer[2] +  heat_buffer[3]
                       +  heat_buffer[8] + heat_buffer[9]) / temperature_sync_info.thermal_mass;

    {
        agent.temp.top_pull = heat_buffer[4];
        agent.temp.btm_pull = heat_buffer[5];
        agent.temp.lft_pull = heat_buffer[6];
        agent.temp.rht_pull = heat_buffer[7];
    }

    //compute temperature changes.
    //curTemp += (energy * .000000000001) /  temperature_sync_info.thermal_mass ; //temp increase from compute (constant is to get the thermal mass into picojoules)

    agent.temperature = curTemp; //new temperature.
}
