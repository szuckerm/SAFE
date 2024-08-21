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


#ifndef _SS_AGENT_GUARD_
#define _SS_AGENT_GUARD_
#include <deque>
#include <map>
#include <mutex>
#include <thread>
extern "C" {
#include "ss-conf.h"
}
#include "ss-instructions.h"
#include "sa-api.h"

/* Possible DVFS states for XEs. */
///FIXME: DVFS should be at block level. Clock-gate should be at XE level.
enum DVFS_STATE
{
    XE_STATE_NONE,
    XE_STATE_HALF,
    XE_STATE_FULL
};

/* Used to specify the role of the CE. */
enum ROLE_STATES
{
    ROLE_STATE_BLOCK,
    ROLE_STATE_UNIT,
    ROLE_STATE_CHIP
};

/* Map for misc. agent related data. */
extern thread_local struct AgentMap
{
    u64 id;
    u64 bid;
    u64 uid;

    /* Current role of the given agent - (chip/unit/block role for the CE). */
    //Note: Changes overtime.
    u64 role;

    /* Current temperature of the agent in Celsius. */
    FLOAT_TYPE temperature;

    /* Current state of the agent (fanned into XEs). */
    struct xe
    {
        u16      state; //state of the agent's XEs.
        TaskType *task; //current executing task of XE
        
        //TODO If you want a pipeline you can add several currentInstructions
        u64      taskCounter; 
        u64      instCounter;  //current instruction of the task of each XE.

        InstType currentInstruction = {0};
        TaskQueueType taskQueue; // for storing task per XE.
    } xe[N_CORES_IN_BLOCK]; 

    /* Rolling window of energies for computing power. */
    //Note: the front contains the energy of the currently executing instruction.
    std::deque<FLOAT_TYPE> energyWindow;
    FLOAT_TYPE accumulatedEnergy = 0.0;

    /* local memory of agent -- used to communication mailboxes. */
    u64 memory[10000];

    /* Reference to neighbors -- if the node has them. */
    //Note: currently used for caching the neighbors temperatures.
    AgentMap* topNeighbor;
    AgentMap* btmNeighbor;
    AgentMap* lftNeighbor;
    AgentMap* rhtNeighbor;

    struct temp
    {
        FLOAT_TYPE top_push;
        FLOAT_TYPE btm_push;
        FLOAT_TYPE lft_push;
        FLOAT_TYPE rht_push;
        FLOAT_TYPE top_pull;
        FLOAT_TYPE btm_pull;
        FLOAT_TYPE lft_pull;
        FLOAT_TYPE rht_pull;
    } temp = {0.0};

    /* Software structures for this block. */
    //queues.
    TaskQueueType taskQueue;             // for storing tasks.
    
    std::deque<saMetadata> messageQueue; // for storing unsent messages.
    
    /*variable indicating block is done*/
    bool done = false;

    /* Used to gather statistics. */
    struct
    {
        u64 tasksExecuted;
        u64 instsExecuted;
    } statistics;

    #if LOGGING_LEVEL == 1
    //File to log information to.
    FILE* logfile;
    #endif
} agent;
extern AgentMap* agentMap[N_UNITS_IN_CHIP*N_BLOCKS_IN_UNIT];

extern std::chrono::high_resolution_clock::time_point simulationStartTime;

//Used in ss-main.c
auto engine (u64 tid) -> void;
#endif
