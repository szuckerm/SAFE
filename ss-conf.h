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


#ifndef _SS_CONF_GUARD_
#define _SS_CONF_GUARD_
#include "ss-types.h"

//System parameters
#define MAX_TIME 5000000               // Max time to let the simulation run in ms
#define TASK_MULTIPLIER 490050      // number of times to reinsert tasks (for longer execution)
#define INST_PER_MEGA_INST 512      // combine instructions together.
#define DRAM_PORTS 2                // number of dram ports per unit
#define BARRIER_INTERVALS (100000)  // Barrier interval in cycles.
#define MAX_XE_CLOCK_SPEED_MHZ					4200		//Clock speed of XEs.
#define ROLLING_ENERGY_WINDOW (MAX_XE_CLOCK_SPEED_MHZ * 1e5 / INST_PER_MEGA_INST) //Rolling window in cycles to compute power from.
#define CYCLES_PER_ITERATION 1      // cycles passed per iteration (affects temperatures).
#define ENABLE_ADAPT_POLICY 0       // adaptation policy to use (0 = none, run everything at full speed)
#define TEMPERATURE_SWING 100.0     // When to send data to other agents in terms of degree changes
#define POWER_SWING 0.00005          // When to send data to other agents in terms of change in picowatts per second
#define TREE_BARRIERS 1             // use tree barriers instead of a single global barrier
#define BARRIER_COUNT 16            // count of barriers when using tree barriers -- ignored otherwise
#define FLOAT_TYPE double           // floating point number precision to use
#define ID_TYPE u32                 // used to identify tasks. Gives the max number of ids. Can shrink memory usage.
#define DEBUG 0                     // enable debugging (checking of bounds)
#define LOGGING_LEVEL						1
#define LOGGING_INTERVAL					100000
#define EXECUTION_TIMES 					3		//Allow measuring the execution times of different part of the code. 1=total time|2 = roles, barriers and sim, 3=Each role, each model (more specific)
#define MAX_STR_SZ						64UL
#define N_UNITS_IN_CHIP						16UL
#define N_BLOCKS_IN_UNIT					16UL
#define N_CORES_IN_BLOCK					8UL
#define S_CHIP_IN_MM						500		//Chip size in mm^2.
#define TEMPERATURE_JUNCTION					127		//maximum junction point temperature.
#define TEMPERATURE_OPERATION					100		//maximum operating temperature threshold.
#define TEMPERATURE_AMBIENT					50.0		//minimum temperature threshold.
#define QUEUE_FILE_SUFFIX					".queue"
#define TASK_FILE_SUFFIX					".task"
#define OUT_FILE_PREFIX						"temperatureRun"
#define TECHNOLOGY_SIZE_HEADER_FULL                             "22nm"
#define TECHNOLOGY_SIZE_HEADER_NTV                              "22nm_NTV"
#define TECHNOLOGY_SIZE_COMMENTS                                "//"
#define INSTRUCTION_TABLE_FILE_NAME                             "energyPerOp.txt"
#define MAX_POWER_PER_BLOCK					16.0
#define STATIC_ENERGY_FACTOR_FULL				0.2		//for static energy model.
#define STATIC_ENERGY_FACTOR_HALF				0.5		//for static energy model.
#define BLOCK_QUEUE_MAX_SIZE					100		//Limits the max number of tasks in the queue.
#define POWER_GOAL						80		//Power goal for the whole chip in watts.
#define BLOCK_POWER_GOAL_SCALE                                  1.2 // The power goal for the block is scaled for tunning according to the load
#define UNIT_POWER_GOAL_SCALE                                   1.2 // The power goal for the unit is scaled for tunning according to the load
#define DAMPENER                                                 50 // Controls the rate of goal changes during adaptation
#define BLOCK_CONTROL_CLOCK                                     500
#define UNIT_CONTROL_CLOCK                                      500
#define CHIP_CONTROL_CLOCK                                      500

typedef struct chip_layout_s
{
    FLOAT_TYPE chip_height_mm;     /* Chip height in millimeters */
    FLOAT_TYPE chip_width_mm;      /* Chip width in millimeters */
    u64 chip_height_num_units;      /* Number of units along chip height */
    u64 chip_width_num_units;       /* Number of units along chip width */
    FLOAT_TYPE unit_height_mm;     /* Unit height in millimeters */
    FLOAT_TYPE unit_width_mm;      /* Unit width in millimeters */
    u64 unit_height_num_blocks;     /* Number of blocks along unit height */
    u64 unit_width_num_blocks;      /* Number of blocks along unit width */
    FLOAT_TYPE block_height_mm;    /* Block height in millimeters */
    FLOAT_TYPE block_width_mm;     /* Block width in millimeters */
} Chip_layout;
extern Chip_layout chip_layout;
#endif
