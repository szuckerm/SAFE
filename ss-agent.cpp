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

#include <atomic>
#include <chrono>
#include <random>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <ctime>
#include <ratio>
#include <iostream>
#include <climits>
#include "ss-agent.h"
extern "C" {
#include "ss-math.h"
#include "ss-temp.h"
}
#include "ss-msr.h"
#include "sa-api.h"

thread_local struct AgentMap agent;
AgentMap* agentMap[N_UNITS_IN_CHIP*N_BLOCKS_IN_UNIT];
std::atomic<u64> done; // incremented to agent count signals the simulation as finished.
std::chrono::high_resolution_clock::time_point simulationStartTime; //simulation start time.
std::atomic<s64> dram_ports[N_UNITS_IN_CHIP];

#if EXECUTION_TIMES == 1 || EXECUTION_TIMES == 3 || EXECUTION_TIMES == 2
  thread_local struct Times
  {
    double roles;
    double rolesChip;
    double rolesUnit;
    double rolesBlock;
    double simulation;
    double barriers;
  }times;


  std::chrono::high_resolution_clock::time_point start;
  std::chrono::high_resolution_clock::time_point start2;
  std::chrono::high_resolution_clock::time_point stop;
  std::chrono::high_resolution_clock::time_point stop2;
#endif


thread_local struct LeafNodeState
{
    u8 stub;
    FLOAT_TYPE powerGoal;
    std::deque <saMetadata> parentBuffer;

    bool underControl;

    void push(saMetadata& msg)
    {

        if(!parentBuffer.empty())
        {
            saMetadata* msg2send = &parentBuffer.front();
            u64 attr1, attr2;
            saGetMetadata(&msg, SA_ATR_METADATA_TYPE, &attr1);
            saGetMetadata(msg2send, SA_ATR_METADATA_TYPE, &attr2);
            if (attr1 == attr2)
            {
                parentBuffer.pop_front();
            }
        }
        parentBuffer.push_back(msg);
    }

    void flush()
    {
      /*Flushing the parent*/
      //check if there is any message to send
      if(!parentBuffer.empty())
      {
          saLocation parentSlot = saGetParentLocation();
          saMetadata * msg2send;
          msg2send = &parentBuffer.front();
          if (saSendMetadata(parentSlot,(void*)msg2send)==0)
          {
              parentBuffer.pop_front();
          }
      }
    }
} leafNodeState = {0};

thread_local struct BranchNodeState
{
    FLOAT_TYPE temperatureAvg;
    FLOAT_TYPE temperatureVar;
    FLOAT_TYPE temperatureSD;
    FLOAT_TYPE temperatureSkew;
    FLOAT_TYPE temperatureMap[N_BLOCKS_IN_UNIT];

    FLOAT_TYPE powerTotal;
    FLOAT_TYPE powerAvg;
    FLOAT_TYPE powerSD;
    FLOAT_TYPE powerVar;
    FLOAT_TYPE powerSkew;
    FLOAT_TYPE powerMap[N_BLOCKS_IN_UNIT];

    FLOAT_TYPE powerGoal;
    FLOAT_TYPE currentMultipliers[N_CORES_IN_BLOCK];

    std::deque <saMetadata> parentBuffer;
    std::deque <saMetadata> childBuffers[N_BLOCKS_IN_UNIT];

    bool underControl;

    void push(saMetadata& msg)
    {

        if(!parentBuffer.empty())
        {
            saMetadata* msg2send = &parentBuffer.front();
            u64 attr1, attr2;
            saGetMetadata(&msg, SA_ATR_METADATA_TYPE, &attr1);
            saGetMetadata(msg2send, SA_ATR_METADATA_TYPE, &attr2);
            if (attr1 == attr2)
            {
                parentBuffer.pop_front();
            }
        }
        parentBuffer.push_back(msg);
    }

    //Flush method try to send the information in the buffers
    void flush()
    {
       /*Flushing the children buffers*/
       for(u64 child=0; child<N_BLOCKS_IN_UNIT; child++)
       {
          //if there is any message to send.
          if (!childBuffers[child].empty())
          {
             saLocation childSlot = saGetChildLocation(child);
             saMetadata * msg2send;
             msg2send = &childBuffers[child].front();
             if (saSendMetadata(childSlot,(void*)msg2send)==0)
             {
               childBuffers[child].pop_front();
             }
          }
       }

       /*Flushing the parent*/
      //check if there is any message to send
      if(!parentBuffer.empty())
      {
          saLocation parentSlot = saGetParentLocation();
          saMetadata * msg2send;
          msg2send = &parentBuffer.front();
          if (saSendMetadata(parentSlot,(void*)msg2send)==0)
          {
               parentBuffer.pop_front();
          }
      }
    }

} branchNodeState = {0}; //space for unit roles + Chip role.

thread_local struct RootNodeState
{
    FLOAT_TYPE temperatureAvg;
    FLOAT_TYPE temperatureVar;
    FLOAT_TYPE temperatureSD;
    FLOAT_TYPE temperatureSkew;
    FLOAT_TYPE temperatureAvgMap[N_UNITS_IN_CHIP];
    FLOAT_TYPE temperatureVarMap[N_UNITS_IN_CHIP];
    FLOAT_TYPE temperatureSDMap[N_UNITS_IN_CHIP];
    FLOAT_TYPE temperatureSkewMap[N_UNITS_IN_CHIP];

    FLOAT_TYPE powerTotal; // our total.
    FLOAT_TYPE powerAvg;
    FLOAT_TYPE powerVar;
    FLOAT_TYPE powerSD;
    FLOAT_TYPE powerSkew;
    FLOAT_TYPE powerTotalMap[N_UNITS_IN_CHIP]; // for previous level.
    FLOAT_TYPE powerAvgMap[N_UNITS_IN_CHIP];
    FLOAT_TYPE powerVarMap[N_UNITS_IN_CHIP];
    FLOAT_TYPE powerSDMap[N_UNITS_IN_CHIP];
    FLOAT_TYPE powerSkewMap[N_UNITS_IN_CHIP];

    FLOAT_TYPE powerGoal;
    FLOAT_TYPE currentMultipliers[N_UNITS_IN_CHIP];
    std::deque <saMetadata> childBuffers[N_UNITS_IN_CHIP];
    void flush()
    {
      /*Flushing the children buffers*/
       for(u64 child=0; child<N_BLOCKS_IN_UNIT; child++)
       {
           //if there is any message to send.
           if (!childBuffers[child].empty())
           {
              saLocation childSlot = saGetChildLocation(child);
              saMetadata * msg2send;
              msg2send = &childBuffers[child].front();
              if (saSendMetadata(childSlot,(void*)msg2send)==0)
              {
                   childBuffers[child].pop_front();
              }
           }
       }
    }
} rootNodeState = {0}; //space for unit roles + Chip role.

auto inline initializeAgent() -> void
{
//XXX FIXME WARNING The casting may represent problems in different architectures.
//XXX The operation is false if id-width is unsigned long and is compared to -1.
//XXX casting necessary but may represent problems in the future.
assert(agent.bid <= USHRT_MAX && "agent.bid too big: cast to long will potentially result in negative value");
//Helpers to calculate neighbours assuming periodicity (blk calculations).
    #define TOP_NEIGHBOR_PERIODIC(id, width, height) \
        ((long)(id-width) > -1 ? (long)id-width : (long)(id-width)+width*height)
    #define BTM_NEIGHBOR_PERIODIC(id, width, height) \
        (id+width < width*height ? id+width : id+width-width*height)
    #define LFT_NEIGHBOR_PERIODIC(id, width, height) \
        (id%width != 0 ? id-1 : id-1+width)
    #define RHT_NEIGHBOR_PERIODIC(id, width, height) \
        ((id+1)%width != 0 ? id+1 : id+1-width)

    //Helpers to calculate neighbours assuming non-periodicity (unit calculations).

    #define EAGENT_NOT_FOUND (u64)-1
    #define TOP_NEIGHBOR(id, width, height) \
        (((long)(id-width) > -1) ? id-width : EAGENT_NOT_FOUND)
    #define BTM_NEIGHBOR(id, width, height, count) \
        ((id+width < width*height) && (id+width < count) ? id+width : EAGENT_NOT_FOUND)
    #define LFT_NEIGHBOR(id, width, height) \
        ((id%width != 0) ? id-1 : EAGENT_NOT_FOUND)
    #define RHT_NEIGHBOR(id, width, height, count) \
        (((id+1)%width != 0) && (id+1 < count) ? id+1 : EAGENT_NOT_FOUND)

    //assign temperature.
    agent.temperature = 50;

    //initialize the underControl flag
    leafNodeState.underControl = false;
    branchNodeState.underControl = false;

    //Initialize some SA related data. and the multipliers of the roots for the controller.
    for (u64 i=0; i<N_UNITS_IN_CHIP; i++)
    {
        branchNodeState.temperatureMap[i] = 50.0;
    }
    for (u64 i=0; i<N_BLOCKS_IN_UNIT; i++)
    {
        rootNodeState.temperatureAvgMap[i] = 50.0;
        rootNodeState.currentMultipliers[i]=1;
    }
    for (u64 i=0; i<N_CORES_IN_BLOCK; i++)
    {
        branchNodeState.currentMultipliers[i]=1;
    }

    //Neighbor ids.
    u64 n_uid = agent.uid; u64 n_bid;

    //Find top neighbor.
    n_bid = TOP_NEIGHBOR_PERIODIC(agent.bid, chip_layout.unit_width_num_blocks, chip_layout.unit_height_num_blocks);
    n_uid = (n_bid >= agent.bid) ? TOP_NEIGHBOR(agent.uid, chip_layout.chip_width_num_units, chip_layout.chip_height_num_units) : agent.uid; //check if in another unit.
    if (n_uid != EAGENT_NOT_FOUND) {
        agent.topNeighbor = agentMap[n_uid*N_BLOCKS_IN_UNIT+n_bid];
        ///printf("\t%d  \n", n_uid*N_BLOCKS_IN_UNIT+n_bid);
    }

    //Find left neighbor.
    n_bid = LFT_NEIGHBOR_PERIODIC(agent.bid, chip_layout.unit_width_num_blocks, chip_layout.unit_height_num_blocks);
    n_uid = (n_bid >= agent.bid) ? LFT_NEIGHBOR(agent.uid, chip_layout.chip_width_num_units, chip_layout.chip_height_num_units) : agent.uid; //check if in another unit.
    if (n_uid != EAGENT_NOT_FOUND) {
        agent.lftNeighbor = agentMap[n_uid*N_BLOCKS_IN_UNIT+n_bid];
        ///printf("%d", n_uid*N_BLOCKS_IN_UNIT+n_bid);
    }

    ///printf("\t%d ", tid);

    //Find right neighbor.
    n_bid = RHT_NEIGHBOR_PERIODIC(agent.bid, chip_layout.unit_width_num_blocks, chip_layout.unit_height_num_blocks);
    n_uid = (n_bid <= agent.bid) ? RHT_NEIGHBOR(agent.uid, chip_layout.chip_width_num_units, chip_layout.chip_height_num_units, N_UNITS_IN_CHIP) : agent.uid; //check if in another unit.
    if (n_uid != EAGENT_NOT_FOUND) {
        agent.rhtNeighbor = agentMap[n_uid*N_BLOCKS_IN_UNIT+n_bid];
        ///printf("\t%d ", n_uid*N_BLOCKS_IN_UNIT+n_bid);
    }

    ///printf("\n");

    //Find bottom neighbor.
    n_bid = BTM_NEIGHBOR_PERIODIC(agent.bid, chip_layout.unit_width_num_blocks, (u64)chip_layout.unit_height_num_blocks);
    n_uid = (n_bid <= agent.bid) ? BTM_NEIGHBOR(agent.uid, chip_layout.chip_width_num_units, (u64)chip_layout.chip_height_num_units, N_UNITS_IN_CHIP) : agent.uid; //check if in another unit.
    if (n_uid != EAGENT_NOT_FOUND) {
        agent.btmNeighbor = agentMap[n_uid*N_BLOCKS_IN_UNIT+n_bid];
        ///printf("\t%d\n", n_uid*N_BLOCKS_IN_UNIT+n_bid);
    }
}

auto inline pushWorkRoundRobin() -> void
{
    //push task into queue more than once if MULTIPLIER specified
    for(u64 taskMultiplier = 0; taskMultiplier<TASK_MULTIPLIER; taskMultiplier++)
    {
        u64 taskNumber = 0;
        //If I still have instructions to schedule.
        while (taskNumber < taskPool.size())
        {
            for(u64 i = 0 ; i < N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP; i++)
            {
                //Push 8 tasks to the same block for it to work on.
                for(u64 j = 0 ; j < 8 && taskNumber<taskPool.size() ; j++)
                {
                    agentMap[i]->taskQueue.push_back(taskPool[taskNumber]); // add front to another queue (copies).
                    taskNumber++;
                }
            }
        }
    }
    taskPool.clear();
    taskPool.shrink_to_fit();
}

auto inline scheduleWork() -> void
{
    for(u64 i = 0 ; i < N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP; i++)
    {
        u64 taskNumber = 0;
        //Cycle each XE scheduling tasks.
        while (taskNumber < agentMap[i]->taskQueue.size())
        {
            for(u64 j=0; j<N_CORES_IN_BLOCK; j++)
            {
                //add new task to be executed in the next cycle if any available.
                if(taskNumber < agentMap[i]->taskQueue.size())
                {
                    //The new task is the task that is in front of the in the taskQueue of the agent.
                    agentMap[i]->xe[j].taskQueue.push_back(agentMap[i]->taskQueue[taskNumber++]);
                }
                else
                {
                    agentMap[i]->taskQueue.clear();
                    agentMap[i]->taskQueue.shrink_to_fit();
                    break;
                }
            }
        }
        agentMap[i]->taskQueue.clear();
        agentMap[i]->taskQueue.shrink_to_fit();
    }
}

auto inline ExecuteWork() -> void
{
    if(agent.done == true)
        return;

    if(((FLOAT_TYPE)readClockMSR()*INST_PER_MEGA_INST/((FLOAT_TYPE)MAX_XE_CLOCK_SPEED_MHZ*1000)) >= MAX_TIME && agent.done == false)
    {
        // tell everyone we finished.
        done++;
        agent.done = true; // mark us as done so we don't increment again.
        return;
    }

    bool executedWork = false;
    FLOAT_TYPE energyDelta[N_CORES_IN_BLOCK] = {0.0};
    static thread_local s64 port[N_CORES_IN_BLOCK] = {-1, -1, -1, -1, -1, -1, -1, -1};
    //Cycle each XE scheduling tasks.
    for(u64 i=0; i<N_CORES_IN_BLOCK; i++)
    {
        u16& state = agent.xe[i].state;                                    //grab the state.
        TaskType*& task = agent.xe[i].task;                                //grab the current task.
        u64& instCounter = agent.xe[i].instCounter;                        //Grab the current Instruction Counter within the task
        u64& taskCounter = agent.xe[i].taskCounter;                        //Grab the current Task Counter within the task
        InstType& currentInstruction = agent.xe[i].currentInstruction;     //Grab the current instruction

        if(taskCounter <= agent.xe[i].taskQueue.size())
        {
            if(currentInstruction.latency <= 0)
            {
                if(task == NULL || instCounter >= task->instructions.size())
                {
                    if (taskCounter == agent.xe[i].taskQueue.size())
                        continue;
                    #if DEBUG == 0
                    task = &taskSet.lookup(agent.xe[i].taskQueue[(taskCounter++)]);
                    #else
                    task = &taskSet.lookup(agent.xe[i].taskQueue.at((taskCounter++)));
                    #endif
                    instCounter = 0;
                    //Statistics.
                    agent.statistics.tasksExecuted++;
                }
                currentInstruction=instructionSet.lookup(task->instructions[(instCounter)++]);

                assert(currentInstruction.latency > 0);
                //Statistics.
                agent.statistics.instsExecuted++;
            }

            executedWork = true;

            if(currentInstruction.type > 0 && port[i] <= 0)
            {
                bool failed_acquire = false;
                if(dram_ports[agent.uid].load(std::memory_order_relaxed) > 0)
                {
                    failed_acquire = true;
                    port[i] = dram_ports[agent.uid].fetch_sub(1);
                }
                if(port[i] <= 0)
                {
                    if(failed_acquire)
                        dram_ports[agent.uid]++;

                    if (state == XE_STATE_FULL)
                        energyDelta[i] = noopInstruction.fullStateEnergy;
                    else if (state == XE_STATE_HALF)
                        energyDelta[i] = noopInstruction.halfStateEnergy;

                    continue;
                }
            }

            //
            if (state == XE_STATE_FULL)
            {
                //Execution of instructions FULL DVFS STATE

                //Decrement latency of the current instruction by the full state multiplier.
                currentInstruction.latency -= currentInstruction.multiplier;
                if(currentInstruction.type > 0)
                {
                    currentInstruction.type -= currentInstruction.multiplier;
                    if(currentInstruction.type <= 0)
                    {
                        port[i] = -1;
                        dram_ports[agent.uid]++;
                    }
                }
                energyDelta[i] = currentInstruction.fullStateEnergy;

                continue;
            }
            else if (state == XE_STATE_HALF)
            {
                //Execution of instructions HALF DVFS STATE

                //Decrement latency of the current instruction by 1.
                currentInstruction.latency--;
                if(currentInstruction.type > 0)
                {
                    currentInstruction.type--;
                    if(currentInstruction.type <= 0)
                    {
                        port[i] = -1;
                        dram_ports[agent.uid]++;
                    }
                }
                energyDelta[i] = currentInstruction.halfStateEnergy;

                continue;
            }
        }
    }

    //done working.
    //if((executedWork==false && agent.done == false))
    //{
    //    // tell everyone we finished.
    //    done++;
    //    agent.done = true; // mark us as done so we don't increment again.
    //}

    FLOAT_TYPE energy = 0.0;
    for(u64 i=0; i<N_CORES_IN_BLOCK; i++)
        energy+=energyDelta[i];

    //Push energy to rolling window for power computations.
    agent.energyWindow.push_front(energy);
    if (agent.energyWindow.size() >= ROLLING_ENERGY_WINDOW)
    {
        if (agent.accumulatedEnergy == 0.0)
        {
            for(u64 i=0; i<ROLLING_ENERGY_WINDOW; i++)
                agent.accumulatedEnergy += agent.energyWindow[i];
        }
        else
        {
            agent.accumulatedEnergy -= agent.energyWindow.back();
            agent.accumulatedEnergy += agent.energyWindow.front();
        }
        agent.energyWindow.pop_back();
    }

    updateTemperatureMSR();
}

auto inline adjustMultiplier(FLOAT_TYPE& powerTotal, FLOAT_TYPE& powerGoal, FLOAT_TYPE& multiplier, s64& dampener) -> bool
{
    if(dampener < 0)
        dampener = 0;
    if ((powerTotal>powerGoal*1.10 || powerTotal<powerGoal*0.90) && dampener == 0)
    {
        multiplier -= ((powerTotal/powerGoal - 1.0) * powerTotal) * 0.008;
        dampener = DAMPENER*2;
    }
    else if ((powerTotal>powerGoal*1.05 || powerTotal<powerGoal*0.95) && dampener == 0)
    {
        multiplier -= ((powerTotal/powerGoal - 1.0) * powerTotal) * 0.000025;
        dampener = DAMPENER;
    }
    else if ((powerTotal>powerGoal*1.03 || powerTotal<powerGoal*0.97) && dampener == 0)
    {
        multiplier -= ((powerTotal/powerGoal - 1.0) * powerTotal) * 0.000025;
        dampener = DAMPENER;
    }
    else if ((powerTotal>powerGoal*1.02 || powerTotal<powerGoal*0.98) && dampener == 0)
    {
        multiplier -= ((powerTotal/powerGoal - 1.0) * powerTotal) * 0.000025;
        dampener = DAMPENER;
    }
    else
    {
        dampener--;
        return false;
    }

    return true;
}

unsigned int seed = 0;
auto inline chipControl() -> void
{
    if(readClockMSR()*INST_PER_MEGA_INST < MAX_XE_CLOCK_SPEED_MHZ * 1e5)
        return;
    #if ENABLE_ADAPT_POLICY == 0
    //do nothing
    #elif ENABLE_ADAPT_POLICY == 1
    //We would like to check if our current total power is above the power budget, if so, modify the multiplier and update the power budget of an aleatory unit
    //If the current total power is above the powerGoal plus 10%

    static thread_local u64 waitCycles = 1;
    waitCycles--;
    static thread_local s64 dampener = 0;
    if (!waitCycles)
    {
        waitCycles = CHIP_CONTROL_CLOCK;
        //Pick up a unit randomly
        int randomUnit=rand_r(&seed)%N_UNITS_IN_CHIP;
        if(adjustMultiplier(rootNodeState.powerTotal, rootNodeState.powerGoal, rootNodeState.currentMultipliers[randomUnit], dampener))
        {
            //Message to be sent
            saBlkCtrlMetadata msgPowerGoal = SA_BLK_CTRL_METADATA_INITIALIZER;
            u64 typeIns = SA_ATR_FUB_CTRL_POWER_GOAL_VALID;
            //Type of control Power Goal. Value node.powerGoal/N_BLOCKS_IN_UNIT
            FLOAT_TYPE powerPerBlock=rootNodeState.powerGoal*rootNodeState.currentMultipliers[randomUnit]/N_UNITS_IN_CHIP;
            saSetMetadata(&msgPowerGoal, SA_ATR_FUB_CTRL, &typeIns);
            saSetMetadata(&msgPowerGoal, SA_ATR_FUB_POWER_GOAL, &powerPerBlock);
            rootNodeState.childBuffers[randomUnit].push_back(*reinterpret_cast<saMetadata*>(&msgPowerGoal));
        }
    }
    #endif
}

auto inline chipRole() -> void
{
    if(agent.done == true)
        return;
    agent.role = ROLE_STATE_CHIP;

    auto& node = rootNodeState; //get node state.

    if (agent.bid == 0 && agent.uid == 0) // Congratulations you were picked as the controller in your chip.
    {
        //Temperature related.
        node.temperatureAvg  = computeAverage(node.temperatureAvgMap, N_UNITS_IN_CHIP);
        FLOAT_TYPE avgOfVar = computeAverage(node.temperatureVarMap, N_UNITS_IN_CHIP);
        node.temperatureVar  = avgOfVar + computeVariance(node.temperatureAvgMap,
                                           node.temperatureAvg, N_UNITS_IN_CHIP);
        node.temperatureSD   = sqrt(node.temperatureVar);
        node.temperatureSkew = computeAverage(node.temperatureSkewMap, N_UNITS_IN_CHIP)
                                         + computeSkew(node.temperatureAvgMap, node.temperatureAvg, N_UNITS_IN_CHIP)
                                         + 3*computeCovariance(node.temperatureAvgMap, node.temperatureAvg,
                                           node.temperatureVarMap, avgOfVar, N_UNITS_IN_CHIP);

        //Power related -- note that that these statistics ignore lower level data points.
        node.powerTotal = 0.0;
        for (u64 i=0; i<N_UNITS_IN_CHIP; i++)
            node.powerTotal += node.powerTotalMap[i];
        node.powerAvg  = node.powerTotal/N_UNITS_IN_CHIP;
        node.powerVar  = computeVariance(node.powerTotalMap,
                         node.powerAvg, N_UNITS_IN_CHIP);
        node.powerSD   = sqrt(node.powerVar);
        node.powerSkew = computeSkew(node.powerTotalMap,
                         node.powerAvg, N_UNITS_IN_CHIP);

        //CONTROL POLICY
        chipControl();
        /*We check the mail */

        //Iterate over child mailboxes.
        for(u64 child=0; child<N_UNITS_IN_CHIP; child++)
        {
            //Grab child slot location.
            saLocation childSlot = saGetChildLocation(child);

            //Fetch message from slot.
            saMetadata msg;
            if(saRecvMetadata(childSlot,(void*)&msg) == 0)
            {
                //Success.
                if (*(u64*)&msg != 0)
                {
                    //Found message.
                    u64 attr;

                    ///FIXME: add metadata APIs for checking this type of thing?
                    //Retrieve metadata type.
                    saGetMetadata(&msg, SA_ATR_METADATA_TYPE, &attr);
                    switch(attr)
                    {
                        case SA_METADATA_TYPE_AGG_INFO:
                        {
                            //Record temperature data (implicit unpacking in the API).
                            {
                                //record aggregated data.
                                saGetMetadata(&msg, SA_ATR_AGG_TEMP_MEAN, &node.temperatureAvgMap[child]);
                                saGetMetadata(&msg, SA_ATR_AGG_TEMP_SD, &node.temperatureSDMap[child]);
                                node.temperatureVarMap[child] = node.temperatureSDMap[child]*node.temperatureSDMap[child];
                                saGetMetadata(&msg, SA_ATR_AGG_TEMP_SKEW, &node.temperatureSkewMap[child]);
                            }

                            //Record power data (implicit unpacking in the API).
                            {
                                //record aggregated data.
                                saGetMetadata(&msg, SA_ATR_AGG_POW_MEAN, &node.powerAvgMap[child]);
                                node.powerTotalMap[child] = node.powerAvgMap[child]*N_BLOCKS_IN_UNIT;
                                saGetMetadata(&msg, SA_ATR_AGG_POW_SD, & node.powerSDMap[child]);
                                node.powerVarMap[child] = node.powerSDMap[child]*node.powerSDMap[child];
                                saGetMetadata(&msg, SA_ATR_AGG_POW_SKEW, &node.powerSkewMap[child]);
                            }

                            break;
                        }
                        default:
                            printf("unt%ld.blk%ld: WARNING: chip role received unknown metadata message!\n", agent.uid, agent.bid);
                    }
                }
            }
        }

        {
            //If the power goal change, set the powerGoal variable and send a message to the units.
            if (node.powerGoal != POWER_GOAL)
            {
                node.powerGoal = POWER_GOAL;
                fprintf(agent.logfile, "[RMD_CONTROL_EVENT] [CHIP_POWER_GOAL_CHANGE] %f at cycle %ld \n", node.powerGoal, readClockMSR()*INST_PER_MEGA_INST);
                for (u64 child = 0 ; child < N_UNITS_IN_CHIP ; child++)
                {
                    //Message to be sent
                    saBlkCtrlMetadata ctrlMsg = SA_BLK_CTRL_METADATA_INITIALIZER;
                    u64 typeIns = SA_ATR_FUB_CTRL_POWER_GOAL_VALID;
                    //Type of control Power Goal. Value node.powerGoal/N_UNITS_IN_CHIP
                    FLOAT_TYPE powerPerUnit = node.powerGoal/N_UNITS_IN_CHIP;
                    saSetMetadata(&ctrlMsg, SA_ATR_FUB_CTRL, &typeIns);
                    saSetMetadata(&ctrlMsg, SA_ATR_FUB_POWER_GOAL, &powerPerUnit);

                    node.childBuffers[child].push_back(*reinterpret_cast<saMetadata*>(&ctrlMsg));
                }
            }

            //Compute aggregate statistics.

            //#if LOGGING_LEVEL == 1
            ////Print statistics to the log.
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Temperature Average of %.2lfC at cycle %ld.\n",
            //        TEMPERATURE_OPERATION-node.temperatureAvg, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Temperature Variance of %.2lfC^2 at cycle %ld.\n",
            //        node.temperatureVar, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Temperature SD of %.2lfC at cycle %ld.\n",
            //        node.temperatureSD, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Temperature Skew of %.2lfC^3 at cycle %ld.\n",
            //        node.temperatureSkew, readClockMSR()*INST_PER_MEGA_INST);
            //#endif

            //#if LOGGING_LEVEL == 1
            ////Print statistics to the log.
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Power Total of %lfW at cycle %ld.\n",
            //        node.powerTotal, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Power Average of %lfW at cycle %ld.\n",
            //        node.powerAvg, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Power Variance of %lfW^2 at cycle %ld.\n",
            //        node.powerVar, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Power SD of %lfW at cycle %ld.\n",
            //        node.powerSD, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Chip] Power Skew of %lfW^3 at cycle %ld.\n",
            //        node.powerSkew, readClockMSR()*INST_PER_MEGA_INST);
            //#endif
        }
    }
    //Flush messages
    node.flush();
}

auto inline unitControl() -> void
{
    if(readClockMSR()*INST_PER_MEGA_INST < MAX_XE_CLOCK_SPEED_MHZ * 1e5)
        return;

    if (!branchNodeState.underControl)
    {
        #if ENABLE_ADAPT_POLICY == 0
        //Do nothing
        #elif ENABLE_ADAPT_POLICY == 1
        if ((readClockMSR()*INST_PER_MEGA_INST) % (UNIT_CONTROL_CLOCK  + agent.id*100) == 0)
        {
            static thread_local s64 dampener = 1;
            static thread_local unsigned int seed = agent.id+10;
            int randomUnit=rand_r(&seed)%N_CORES_IN_BLOCK;
            if(adjustMultiplier(branchNodeState.powerTotal, branchNodeState.powerGoal, branchNodeState.currentMultipliers[randomUnit], dampener))
            {
                //Message to be sent
                saBlkCtrlMetadata msgPowerGoal = SA_BLK_CTRL_METADATA_INITIALIZER;
                u64 typeIns = SA_ATR_FUB_CTRL_POWER_GOAL_VALID;
                //Type of control Power Goal. Value node.powerGoal/N_BLOCKS_IN_UNIT
                FLOAT_TYPE powerPerBlock= branchNodeState.powerGoal*branchNodeState.currentMultipliers[randomUnit]/N_BLOCKS_IN_UNIT;
                saSetMetadata(&msgPowerGoal, SA_ATR_FUB_CTRL, &typeIns);
                saSetMetadata(&msgPowerGoal, SA_ATR_FUB_POWER_GOAL, &powerPerBlock);
                branchNodeState.childBuffers[randomUnit].push_back(*reinterpret_cast<saMetadata*>(&msgPowerGoal));
            }
        }
        #endif
    }
}

auto inline unitRole() -> void
{
    if(agent.done == true)
        return;
    agent.role = ROLE_STATE_UNIT;

    auto& node = branchNodeState; // get node state.

    static thread_local FLOAT_TYPE oldTemp[N_BLOCKS_IN_UNIT]  = {0.0};
    static thread_local FLOAT_TYPE oldPower[N_BLOCKS_IN_UNIT] = {0.0};
    //Check if we should send data to chip agent.
    bool sendData = false;

    if (agent.bid == 0) //Congratulations you were picked as the controller of your unit.
    {
        //Compute aggregate statistics.

        //Temperature related.
        node.temperatureAvg  = computeAverage(node.temperatureMap,
                                              N_BLOCKS_IN_UNIT);
        node.temperatureVar  = computeVariance(node.temperatureMap,
                                               node.temperatureAvg, N_BLOCKS_IN_UNIT);
        node.temperatureSD   = sqrt(node.temperatureVar);
        node.temperatureSkew = computeSkew(node.temperatureMap,
                                           node.temperatureAvg, N_BLOCKS_IN_UNIT);
        //Power related.
        node.powerTotal = 0.0;
        for (u64 i=0; i<N_BLOCKS_IN_UNIT; i++)
           node.powerTotal += node.powerMap[i];
        node.powerAvg  = node.powerTotal/N_BLOCKS_IN_UNIT;
        node.powerVar  = computeVariance(node.powerMap,
                                         node.powerAvg, N_BLOCKS_IN_UNIT);
        node.powerSD   = sqrt(node.powerVar);
        node.powerSkew = computeSkew(node.powerMap,
                                     node.powerAvg, N_BLOCKS_IN_UNIT);

          //control policy
          unitControl();

          /*We check the mail*/
          //Grab parent slot location.
          saLocation parentSlot = saGetParentLocation();

          //Fetch message from slot.
          saMetadata msg;
          if(saRecvMetadata(parentSlot,(void*)&msg) == 0)
          {
              sendData = true;
              //Success.
              if (*(u64*)&msg != 0)
              {
                  //Found message.
                  u64 attr;
                  ///FIXME: add metadata APIs for checking this type of thing?
                  //Retrieve metadata type.
                  saGetMetadata(&msg, SA_ATR_METADATA_TYPE, &attr);
                  switch(attr)
                  {
                      //Incoming control instruction.
                      case SA_METADATA_TYPE_BLK_CTRL:
                      {
                          //Grab the type of instruction.
                          u64 typeIns;
                          saGetMetadata(&msg, SA_ATR_FUB_CTRL, &typeIns);
                          switch (typeIns)
                          {
                               //Change the power goal
                               case SA_ATR_FUB_CTRL_POWER_GOAL_VALID:
                                   //Set power goal to the value in the message. Inform the change to the childrens.
                                   FLOAT_TYPE newPowerGoal;
                                   saGetMetadata(&msg, SA_ATR_FUB_POWER_GOAL, &newPowerGoal);
                                   if (node.powerGoal != newPowerGoal)
                                   {
                                       node.powerGoal = newPowerGoal*UNIT_POWER_GOAL_SCALE;
                                       fprintf(agent.logfile, "[RMD_CONTROL_EVENT] [UNIT_POWER_GOAL_CHANGE] %f at cycle %ld \n", node.powerGoal, readClockMSR()*INST_PER_MEGA_INST);
                                       //Set a new power goal for the children.
                                       for (u64 i = 0 ; i < N_BLOCKS_IN_UNIT ; i++)
                                       {
                                            //Message to be sent
                                            saBlkCtrlMetadata msgPowerGoal = SA_BLK_CTRL_METADATA_INITIALIZER;
                                            typeIns = SA_ATR_FUB_CTRL_POWER_GOAL_VALID;
                                            //Type of control Power Goal. Value node.powerGoal/N_BLOCKS_IN_UNIT
                                            FLOAT_TYPE powerPerBlock=node.powerGoal/N_BLOCKS_IN_UNIT;
                                            saSetMetadata(&msgPowerGoal, SA_ATR_FUB_CTRL, &typeIns);
                                            saSetMetadata(&msgPowerGoal, SA_ATR_FUB_POWER_GOAL, &powerPerBlock);

                                            node.childBuffers[i].push_back(*reinterpret_cast<saMetadata*>(&msgPowerGoal));
                                       }
                                   }
                                   break;

                               default:
                                 printf("unt%ld.blk%ld: WARNING: unit role received unknown control message!\n", agent.uid, agent.bid);
                          }
                          break;
                      }
                  }
              }
          }


        //Iterate over child mailboxes.
        for(u64 child=0; child<N_BLOCKS_IN_UNIT; child++)
        {
            //Grab child slot location.
            saLocation childSlot = saGetChildLocation(child);

            //Fetch message from slot.
            saMetadata msg;
            if(saRecvMetadata(childSlot,(void*)&msg) == 0)
            {
                sendData = true;
               //Success.
               if (*(u64*)&msg != 0)
               {
                    //Found message.
                    u64 attr;

                    ///FIXME: add metadata APIs for checking this type of thing?
                    //Retrieve metadata type.
                    saGetMetadata(&msg, SA_ATR_METADATA_TYPE, &attr);
                    switch(attr)
                    {
                        //Incoming block information packet.
                        case SA_METADATA_TYPE_BLK_INFO:
                        {
                            //Decode and handle the temperature (implicit unpacking in the API).
                            {
                                // Grab temperature directly (No need to unpack).
                                saGetMetadata(&msg, SA_ATR_FUB_TEMP_SCORE, &node.temperatureMap[child]);

                                //                                      //Compute real temperature from delta.
                                //                                      FLOAT_TYPE temperature = TEMPERATURE_OPERATION - node.temperatureMap[child];
                                //
                                //                                      if(temperature > 90.0)
                                //                                      {
                                //                                           // send clock off message to block
                                //                                           saBlkCtrlMetadata ctrlMsg = SA_BLK_CTRL_METADATA_INITIALIZER;
                                //                                           attr = SA_ATR_FUB_DVFS_SCORE_NONE;
                                //                                           saSetMetadata(&ctrlMsg, SA_ATR_FUB_DVFS_SCORE, &attr);
                                //                                           node.childBuffers[child].push_back(*reinterpret_cast<saMetadata*>(&ctrlMsg));
                                //                                      }
                                //                                      else if(temperature > 70.0)
                                //                                      {
                                //                                           // send dvfs message to block
                                //                                           saBlkCtrlMetadata ctrlMsg = SA_BLK_CTRL_METADATA_INITIALIZER;
                                //                                           attr = SA_ATR_FUB_DVFS_SCORE_HALF;
                                //                                           saSetMetadata(&ctrlMsg, SA_ATR_FUB_DVFS_SCORE, &attr);
                                //                                           node.childBuffers[child].push_back(*reinterpret_cast<saMetadata*>(&ctrlMsg));
                                //                                      }
                            }

                            //Decode and handle the power.
                            {
                               // Grab power.
                               saGetMetadata(&msg, SA_ATR_FUB_POW_SCORE, &node.powerMap[child]);
                            }
                            sendData = true;
                            break;
                       }

                       default:
                           printf("unt%ld.blk%ld: WARNING: unit role received unknown metadata message!\n", agent.uid, agent.bid);
                    }
               }
            }
        }

        for(u64 child=0; child<N_BLOCKS_IN_UNIT; child++)
        {
            if (fabs(oldTemp[child]-node.temperatureMap[child]) > TEMPERATURE_SWING || fabs(oldPower[child]-node.powerMap[child]) > POWER_SWING)
            {
                sendData = true;
                break;
            }
        }
        if (sendData == true)
        {
            for(u64 child=0; child<N_BLOCKS_IN_UNIT; child++)
            {
                oldTemp[child]  = node.temperatureMap[child];
                oldPower[child] = node.powerMap[child];
            }

            //#if LOGGING_LEVEL == 1
            ////Print statistics to the log.
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Temperature Average of %.2lfC at cycle %ld.\n",
            //  TEMPERATURE_OPERATION-node.temperatureAvg, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Temperature Variance of %.2lfC^2 at cycle %ld.\n",
            //  node.temperatureVar, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Temperature SD of %.2lfC at cycle %ld.\n",
            //  node.temperatureSD, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Temperature Skew of %.2lfC^3 at cycle %ld.\n",
            //  node.temperatureSkew, readClockMSR()*INST_PER_MEGA_INST);
            //#endif

            //#if LOGGING_LEVEL == 1
            ////Print statistics to the log.
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Power Total of %lfW at cycle %ld.\n",
            //  node.powerTotal, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Power Average of %lfW at cycle %ld.\n",
            //  node.powerAvg, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Power Variance of %lfW^2 at cycle %ld.\n",
            //  node.powerVar, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Power SD of %lfW at cycle %ld.\n",
            //  node.powerSD, readClockMSR()*INST_PER_MEGA_INST);
            //fprintf(agent.logfile, "[RMD_TRACE_AGGREGATE] [Unit] Power Skew of %lfW^3 at cycle %ld.\n",
            //  node.powerSkew, readClockMSR()*INST_PER_MEGA_INST);
            //#endif

            //Transfer our aggregate temperatures.
            {
               saMetadata msg = SA_AGG_INFO_METADATA_INITIALIZER;

               //Push temperature and power to next level CE (packing is transparently done in the API).
               saSetMetadata(&msg, SA_ATR_AGG_TEMP_MEAN, &node.temperatureAvg);
               saSetMetadata(&msg, SA_ATR_AGG_TEMP_SD, &node.temperatureSD);
               saSetMetadata(&msg, SA_ATR_AGG_TEMP_SKEW, &node.temperatureSkew);
               saSetMetadata(&msg, SA_ATR_AGG_POW_MEAN, &node.powerAvg);
               saSetMetadata(&msg, SA_ATR_AGG_POW_SD, &node.powerSD);
               saSetMetadata(&msg, SA_ATR_AGG_POW_SKEW, &node.powerSkew);

               node.push(msg);
            }
        }
    }
    //flush messages
    node.flush();
}

auto inline blockControl() -> void
{
//FLOAT_TYPE tempDelta = readTemperatureMSR(); //read from MSR (should be 7 bits max).
//FLOAT_TYPE temperature = TEMPERATURE_JUNCTION - tempDelta;
//Turn on all units for work.
//     if (temperature < 51) //cool as a whistle.
//     {
//         for(u64 i=0; i<8; i++)
//         {
//             agent.xe[i].state = XE_STATE_FULL;
//         }
//

    if(readClockMSR()*INST_PER_MEGA_INST < MAX_XE_CLOCK_SPEED_MHZ * 1e5)
    {
        for(u64 i=0; i<N_CORES_IN_BLOCK; i++)
        {
            //enable all XE's at full freq and voltage.
            agent.xe[i].state = XE_STATE_FULL;
        }
        return;
    }
    //FIXME POWER CONTROL
    #if ENABLE_ADAPT_POLICY == 0
    for(u64 i=0; i<N_CORES_IN_BLOCK; i++)
    {
        //enable all XE's at full freq and voltage.
        agent.xe[i].state = XE_STATE_FULL;
    }
    #elif ENABLE_ADAPT_POLICY == 1
    if ((readClockMSR()*INST_PER_MEGA_INST) % (BLOCK_CONTROL_CLOCK + agent.id*100) == 0)
    {
            static thread_local s64 dampener = 0;
            dampener--;
            if(dampener < 0)
                dampener = 0;
            FLOAT_TYPE power = readPowerMSR();
            if ((power > leafNodeState.powerGoal*0.90 && power < leafNodeState.powerGoal*1.10) || dampener != 0)
                return;

            if (power < leafNodeState.powerGoal)
            {
                for(u64 i=0; i<N_CORES_IN_BLOCK; i++)
                {
                    if (agent.xe[i].state != XE_STATE_FULL)
                    {
                        agent.xe[i].state = XE_STATE_FULL;
                        fprintf(agent.logfile, "[RMD_CONTROL_EVENT] [STATE_CHANGE] FULL at cycle %ld \n", readClockMSR()*INST_PER_MEGA_INST);
                        dampener = DAMPENER;
                        break;
                    }
                }
            }
            else
            {
                for(u64 i=0; i<N_CORES_IN_BLOCK; i++)
                {
                    if (agent.xe[i].state != XE_STATE_HALF)
                    {
                        agent.xe[i].state = XE_STATE_HALF;
                        fprintf(agent.logfile, "[RMD_CONTROL_EVENT] [STATE_CHANGE] HALF at cycle %ld \n", readClockMSR()*INST_PER_MEGA_INST);
                        dampener = DAMPENER;
                        break;
                    }
                }
            }
    }
    #endif
}

auto inline blockRole() -> void
{
    if(agent.done == true)
        return;
    agent.role = ROLE_STATE_BLOCK;

    auto& node = leafNodeState;

    static thread_local FLOAT_TYPE oldTemp  = 0.0;
    static thread_local FLOAT_TYPE oldPower = 0.0;

    FLOAT_TYPE tempDelta = readTemperatureMSR(); //read from MSR (should be 7 bits max).
    FLOAT_TYPE temperature = TEMPERATURE_JUNCTION - tempDelta;

    //control policy
    blockControl();

    /*We check the email*/
    //Grab parent slot location.
    saLocation parentSlot = saGetParentLocation();

    //Fetch message from slot.

    saMetadata msg;
    if(saRecvMetadata(parentSlot,(void*)&msg) == 0)
    {
         //Success.
         if (*(u64*)&msg != 0)
         {
             //Found message.
             u64 attr;

             ///FIXME: add metadata APIs for checking this type of thing?
             //Retrieve metadata type.
             saGetMetadata(&msg, SA_ATR_METADATA_TYPE, &attr);
             switch(attr)
             {
                  //Incoming control instruction
                  case SA_METADATA_TYPE_BLK_CTRL:
                  {
                      //Grab the type of instruction.
                      u64 typeIns;
                      saGetMetadata(&msg, SA_ATR_FUB_CTRL, &typeIns);
                      switch (typeIns)
                      {
                        //Change the power goal
                        case SA_ATR_FUB_CTRL_POWER_GOAL_VALID:
                             //Set power goal to the value in the message.
                             saGetMetadata(&msg, SA_ATR_FUB_POWER_GOAL, &node.powerGoal);
                             //node.powerGoal*=BLOCK_POWER_GOAL_SCALE;
                             fprintf(agent.logfile, "[RMD_CONTROL_EVENT] [BLOCK_POWER_GOAL_CHANGE] %f at cycle %ld \n", node.powerGoal, readClockMSR()*INST_PER_MEGA_INST);
                             break;
                        case SA_ATR_FUB_CTRL_UNDER_CONTROL_VALID:
                             unsigned int underControl;
                             saGetMetadata(&msg, SA_ATR_FUB_UNDER_CONTROL,&underControl);
                             node.underControl=underControl;
                             fprintf(agent.logfile, "[RMD_CONTROL_EVENT] [BLOCK_UNDER_CONTROL_CHANGE] %d at cycle %ld \n", underControl, readClockMSR()*INST_PER_MEGA_INST);
                             break;
                        //TODO DVFS The Unit. (NOT BEEING USED)
                        case SA_ATR_FUB_CTRL_DVFS_VALID:
                             saGetMetadata(&msg, SA_ATR_FUB_DVFS_SCORE, &attr);
                             if(attr == SA_ATR_FUB_DVFS_SCORE_NONE)
                             {
                                 for(u64 i=0; i<8; i++)
                                 {
                                      agent.xe[i].state = XE_STATE_NONE;
                                 }
                             }
                             else if(attr == SA_ATR_FUB_DVFS_SCORE_HALF)
                             {
                                 for(u64 i=0; i<8; i++)
                                 {
                                      agent.xe[i].state = XE_STATE_HALF;
                                 }
                             }
                             else if (attr == SA_ATR_FUB_DVFS_SCORE_FULL)
                             {
                                 for(u64 i=0; i<8; i++)
                                 {
                                      agent.xe[i].state = XE_STATE_FULL;
                                 }
                             }
                             break;
                        default:
                             printf("unt%ld.blk%ld: WARNING: Block role received unknown control message!\n", agent.uid, agent.bid);
                      }
                      break;
                  }
                  default:
                      printf("unt%ld.blk%ld: WARNING: block role received unknown metadata message %ld!\n", agent.uid, agent.bid, attr);
             }
         }
    }

    //Grab the power usage of the block.
    FLOAT_TYPE power = readPowerMSR();

    //Check if we should send data to our node agent.
    if (fabs(oldTemp-tempDelta) > TEMPERATURE_SWING || fabs(oldPower-power) > POWER_SWING)
    {
        oldTemp  = tempDelta;
        oldPower = power;
        //Transfer our temperature and power.
        {
            saMetadata msg = SA_BLK_INFO_METADATA_INITIALIZER;

            //Note: 'real' temperatures above the maximum operating temp are
            // rounded down for sharing purposes with other CEs. This shouldn't
            // matter because if we are at or above the maximum operating
            // temperature then we are in a critical state and need to shutdown
            // immediately. In reality, we should never reach these temperatures
            // unless something has gone horribly wrong!
            if(temperature < TEMPERATURE_OPERATION)
                tempDelta = TEMPERATURE_OPERATION-temperature; // recompute delta relative to max operating temperature.
            else
                tempDelta = 0; //a delta of zero is the maximum temperature.

            //Push temperature and power to next level CE (packing is transparently done in the API).
            saSetMetadata(&msg, SA_ATR_FUB_TEMP_SCORE, &tempDelta);
            saSetMetadata(&msg, SA_ATR_FUB_POW_SCORE, &power);
            node.push(msg);
        }
    }

    //flush messages
    node.flush();
}

auto verifyAggregateData() -> void
{
    //Note: verification is only valid IF done by the root node.
    auto& node = rootNodeState; //get node state.

    FLOAT_TYPE temperatureMap[N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP];
    for (u64 tid=0; tid<N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP; tid++)
        temperatureMap[tid] = agentMap[tid]->temperature;
    FLOAT_TYPE avg  = computeAverage(temperatureMap, N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP);
    FLOAT_TYPE var  = computeVariance(temperatureMap, avg, N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP);

    //Normalize to differential form.
    FLOAT_TYPE normMap[N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP];
    for (u64 tid=0; tid<N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP; tid++)
        normMap[tid] =  TEMPERATURE_OPERATION-temperatureMap[tid];

    FLOAT_TYPE skew = computeSkew(normMap, TEMPERATURE_OPERATION-avg, N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP);
    FLOAT_TYPE SD = sqrt(var);
    printf("  * temp avg (actual : aggregate): %f C : %f C\n", avg, TEMPERATURE_OPERATION-node.temperatureAvg);
    printf("  * temp sd  (actual : aggregate): %f C : %f C\n", SD, node.temperatureSD);
    printf("  * temp var (actual : aggregate): %f : %f\n", var, node.temperatureVar);

    //Normalize for print out purposes.
    printf("  * temp skew  (actual : aggregate) %f : %f\n", -skew/(SD*SD*SD), -node.temperatureSkew/(node.temperatureSD*node.temperatureSD*node.temperatureSD));

    printf("  * pow total %f W\n", node.powerTotal);
    printf("  * pow avg   %f W\n", node.powerAvg);
    printf("  * pow sd    %f W\n", node.powerSD);
    printf("  * pow var   %f\n", node.powerVar);

}

/* Fancy regular barrier for C++11 */
auto inline barrier(u64 count) -> void
{
    /* Timing Related.........................................................*/
    {
    #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
       if (agent.id==0)
       start=std::chrono::high_resolution_clock::now();
    #endif
    }
    /*........................................................................*/

    static u64 checkedInCount = 0;          // Count of threads having reached the barrier.
    static u64 eventCount = 0;              // Event count of barriers reached.
    static std::condition_variable condVar; // Conditional wait variable.
    static std::mutex mutex;                // Mutual exclusion variable.

    /**************************************************************************/
    std::unique_lock<std::mutex> lock(mutex); //unlocks when destructed.
    checkedInCount++; //increment count of threads checked in.

    // Check if count has been reached.
    if (checkedInCount < count && done != N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP)
    {
        // Count hasn't been reached -- so we need to wait.

        // establish a predicate to check to for to continue. Namely that the
        // even count changes. We need different predicates each iteration to
        // avoid race defects.
        u64 predicate = eventCount;
        do
            condVar.wait(lock);
        while(predicate == eventCount && done != N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP && done != N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP); // avoid spurious wakeup rechecking and locking.
    }
    else
    {
        // Count reached -- so we need to signal everyone.

        eventCount++; // increment the event count.
        checkedInCount = 0; //reset the count of checked in threads.
        condVar.notify_all(); //notify everyone.
    }
    /**************************************************************************/

    /* Timing Related.........................................................*/
    {
        #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
        if (agent.id==0)
        {
            stop=std::chrono::high_resolution_clock::now();

            times.barriers+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop-start)).count();
        }
        #endif
    }
    /*........................................................................*/
}

/* Fancy Atomic based barrier */
///auto barrier(u64 count) -> void
///{
///    /* Timing Related.........................................................*/
///    {
///    #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
///       if (agent.id==0)
///       start=std::chrono::high_resolution_clock::now();
///    #endif
///    }
///    /*........................................................................*/
///
///    static std::atomic<u64> checkedInCount; // Count of threads having reached the barrier.
///    static std::atomic<u64> eventCount;     // Event count of barriers reached.
///
///
///    /**************************************************************************/
///    checkedInCount++; //increment count of threads checked in.
///
///    // Check if count has been reached.
///    if (checkedInCount < count)
///    {
///        // Count hasn't been reached -- so we need to wait.
///
///        // establish a predicate to check to for to continue. Namely that the
///        // even count changes. We need different predicates each iteration to
///        // avoid race defects.
///        u64 predicate = eventCount;
///        do
///            std::this_thread::yield();
///        while(predicate == eventCount && checkedInCount != 0); // avoid spurious wakeup rechecking and locking.
///    }
///    else
///    {
///        // Count reached -- so we need to signal everyone.
///
///        eventCount++; // increment the event count.
///        checkedInCount = 0; //reset the count of checked in threads.
///    }
///    /**************************************************************************/
///
///    /* Timing Related.........................................................*/
///    {
///        #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
///        if (agent.id==0)
///        {
///            stop=std::chrono::high_resolution_clock::now();
///
///            times.barriers+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop-start)).count();
///        }
///        #endif
///    }
///    /*........................................................................*/
///}

static u64 checkedInCount[BARRIER_COUNT] = {0};          // Count of threads having reached the barrier.
static u64 eventCount[BARRIER_COUNT] = {0};              // Event count of barriers reached.
static std::condition_variable condVar[BARRIER_COUNT];   // Conditional wait variable.
static std::mutex mutex[BARRIER_COUNT];                  // Mutual exclusion variable.

/* Fancy 2 level tree barrier for C++11 */
auto inline tree_barrier() -> void
{
    /* Timing Related.........................................................*/
    {
    #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
       if (agent.id==0)
       start=std::chrono::high_resolution_clock::now();
    #endif
    }
    /*........................................................................*/

    /**************************************************************************/

    u64 bar_id = agent.id%BARRIER_COUNT;

    std::unique_lock<std::mutex> lock(mutex[bar_id]); //unlocks when destructed.
    checkedInCount[bar_id]++; //increment count of threads checked in.

    // Check if count has been reached.
    if (checkedInCount[bar_id] < N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP/BARRIER_COUNT && done != N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP)
    {
        // Count hasn't been reached -- so we need to wait.

        // establish a predicate to check to for to continue. Namely that the
        // even count changes. We need different predicates each iteration to
        // avoid race defects.
        u64 predicate = eventCount[bar_id];
        do
            condVar[bar_id].wait(lock);
        while(predicate == eventCount[bar_id] && done != N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP); // avoid spurious wakeup rechecking and locking.
    }
    else
    {
        // Count reached -- so we need to signal everyone.

        barrier(BARRIER_COUNT);

        eventCount[bar_id]++; // increment the event count.
        checkedInCount[bar_id] = 0;   //reset the count of checked in threads.
        condVar[bar_id].notify_all(); //notify everyone.
    }
    /**************************************************************************/

    /* Timing Related.........................................................*/
    {
        #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
        if (agent.id==0)
        {
            stop=std::chrono::high_resolution_clock::now();

            times.barriers+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop-start)).count();
        }
        #endif
    }
    /*........................................................................*/
}

/* Thread entry. Logical ID passed in. */
auto engine (u64 id) -> void
{
    /**************************************************************************/
    barrier(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP);
    /**************************************************************************/

    agent.id = id;                               // 1-dimensional numeric id.
    agentMap[id] = &agent;                       // add agent to the agentMap.
    agent.uid = id/N_BLOCKS_IN_UNIT;             // unit id.
    agent.bid = id - agent.uid*N_BLOCKS_IN_UNIT; // block id.

    #if LOGGING_LEVEL == 1
    //Out log for this engine (block).
    char logfile[1024];
    const char* SAFE_LOGS_PATH = getenv("SAFE_LOGS_PATH");
    if (SAFE_LOGS_PATH == 0) SAFE_LOGS_PATH = "./logs";
    sprintf (logfile, "%s/%s.log.brd00.chp00.unt%02lu.blk%02ld", SAFE_LOGS_PATH, OUT_FILE_PREFIX, agent.uid, agent.bid);
    if((agent.logfile = fopen(logfile, "w")) == NULL)
        fatal(logfile);

    //print layout.
    if(agent.id == 0)
    {
        fprintf(agent.logfile, "[RMD_SIMULATION_INFO] Simulated block layout: %ld by %ld.\n", chip_layout.unit_height_num_blocks, chip_layout.unit_width_num_blocks);
        fprintf(agent.logfile, "[RMD_SIMULATION_INFO] Simulated unit layout: %ld by %ld.\n", chip_layout.chip_height_num_units, chip_layout.chip_width_num_units);
    }
    #endif

    /**************************************************************************/
    barrier(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP);
    /**************************************************************************/

    //Initialize things - only the first agent.
    if(agent.id == 0)
    {
        done = 0; // set done variable.
        for(u64 i=0; i<N_UNITS_IN_CHIP; i++)
            dram_ports[i] = DRAM_PORTS;
        printf("==> Distributing work to nodes...\n");
        pushWorkRoundRobin();
        scheduleWork();
        printf("==> Initializing node state...\n");
    }
    initializeAgent(); //initialize agent variables.

    /**************************************************************************/
    barrier(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP);
    /**************************************************************************/

    {
        u64 tasksLeft = 0;
        for(u64 i = 0 ; i < N_UNITS_IN_CHIP*N_BLOCKS_IN_UNIT ; i++)
                for(u64 j = 0 ; j < N_CORES_IN_BLOCK ; j++)
                    tasksLeft += agentMap[i]->xe[j].taskQueue.size();

        if(agent.id == 0)
        {
            printf("==> Running simulation with: %ld tasks...\n", tasksLeft);
            printf("---------------------------\n");
        }
    }

    /**************************************************************************/
    barrier(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP);
    /**************************************************************************/

    while (true)
    {
        /* Timing Related.....................................................*/
        {

            #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
            if (agent.id==0)
                start=std::chrono::high_resolution_clock::now();
            #endif

            #if EXECUTION_TIMES == 3
            if (agent.id==0)
                start2=std::chrono::high_resolution_clock::now();
            #endif
        }
        /*....................................................................*/

        //...................................................................../
        // Do tasks for chip role -- if this agent has a chip role ............/
        chipRole();
        //...................................................................../

        /* Timing Related.....................................................*/
        {
            #if EXECUTION_TIMES == 3
            if (agent.id==0)
            {
                stop2=std::chrono::high_resolution_clock::now();
                times.rolesChip+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop2-start2)).count();
            }
            #endif

            #if EXECUTION_TIMES == 3
            if (agent.id==0)
                start2=std::chrono::high_resolution_clock::now();
            #endif
        }
        /*....................................................................*/

        //...................................................................../
        // Do tasks for unit role -- if this agent has a unit role ............/
        unitRole();
        //...................................................................../

        /* Timing Related.....................................................*/
        {
            #if EXECUTION_TIMES == 3
            if (agent.id==0)
            {
                stop2=std::chrono::high_resolution_clock::now();
                times.rolesUnit+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop2-start2)).count();
            }
            #endif

            #if EXECUTION_TIMES == 3
            if (agent.id==0)
                start2=std::chrono::high_resolution_clock::now();
            #endif
        }
        /*....................................................................*/

        //...................................................................../
        // Do tasks for block role -- if this agent has a block role ........../
        blockRole();
        //...................................................................../

        /* Timing Related.....................................................*/
        {
            #if EXECUTION_TIMES == 3
            if (agent.id==0)
            {
                stop2=std::chrono::high_resolution_clock::now();
                times.rolesBlock+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop2-start2)).count();
            }
            #endif

            #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
            if (agent.id==0)
            {
                stop=std::chrono::high_resolution_clock::now();
                times.roles+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop-start)).count();
            }
            #endif
        }
        /*....................................................................*/

        if (readClockMSR() % BARRIER_INTERVALS == 0)
        {
            /******************************************************************/
            #if TREE_BARRIERS == 1
            tree_barrier(); // Only barrier at sync interval.
            #else
            barrier(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP); // Only barrier at sync interval.
            #endif
            /******************************************************************/
        }

        #if LOGGING_LEVEL == 1
        ///Write out to log file.
        if((readClockMSR()*INST_PER_MEGA_INST) % LOGGING_INTERVAL == 0 && agent.done == false)
        {
            fprintf(agent.logfile, "[RMD_TRACE_TEMPERATURE] Temperature of %fC at cycle %ld.\n", agent.temperature, readClockMSR()*INST_PER_MEGA_INST);
            fprintf(agent.logfile, "[RMD_TRACE_POWER] Power of %fW at cycle %ld.\n", readPowerMSR(), readClockMSR()*INST_PER_MEGA_INST);
        }
        #endif

        /* Timing Related.....................................................*/
        {
            #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
            if (agent.id==0)
                start=std::chrono::high_resolution_clock::now();
            #endif
        }
        /*....................................................................*/

        //Note: this needs to be done in lock step with pushing of work because
        //      our queues are not thread safe.
        ///Schedule work for this cycle.
        ExecuteWork();

        /* Timing Related.....................................................*/
        {
            #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
            if (agent.id==0)
            {
                stop=std::chrono::high_resolution_clock::now();
                times.simulation+=(std::chrono::duration_cast< std::chrono::duration< double > > (stop-start)).count();
            }
            #endif
        }
        /*....................................................................*/

        if (readClockMSR() % BARRIER_INTERVALS == 0)
        {
            /******************************************************************/
            #if TREE_BARRIERS == 1
            tree_barrier(); // Only barrier at sync interval.
            #else
            barrier(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP); // Only barrier at sync interval.
            #endif
            /******************************************************************/
        }

        /* Timing Related.....................................................*/
        {
            #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
                if (agent.id==0)
                    start=std::chrono::high_resolution_clock::now();
            #endif
        }
        /*....................................................................*/

        ///Push some work (in the form of 8 tasks) to an 'elected' block.
        if(agent.id == 0) //only the first guy.
        {
            if (readClockMSR() % BARRIER_INTERVALS == 0)
            {
                //system("clear");
                u64 tasksLeft = 0;
                double power = 0.0;
                for(u64 i = 0 ; i < N_UNITS_IN_CHIP*N_BLOCKS_IN_UNIT ; i++)
                {
                    power += agentMap[i]->accumulatedEnergy;
                    for(u64 j = 0 ; j < N_CORES_IN_BLOCK ; j++)
                        tasksLeft += agentMap[i]->xe[j].taskQueue.size() - agentMap[i]->xe[j].taskCounter;
                }
                power=power*(MAX_XE_CLOCK_SPEED_MHZ*1E-6/ROLLING_ENERGY_WINDOW/INST_PER_MEGA_INST);
                printf("tasks left to execute:   %ld (temperature: %f C)...\n", tasksLeft, TEMPERATURE_OPERATION-rootNodeState.temperatureAvg);
                printf("current simulation time: %f ms...\n", (FLOAT_TYPE)readClockMSR()*INST_PER_MEGA_INST/((FLOAT_TYPE)MAX_XE_CLOCK_SPEED_MHZ*1000));
                printf("cycle count:             %ld\n", readClockMSR()*INST_PER_MEGA_INST);
                printf("total power (agg : real): %fW : %fW < %fW\n", rootNodeState.powerTotal, power, rootNodeState.powerGoal);
                power = 0.0;
                for(u64 i=0; i<N_BLOCKS_IN_UNIT; i++)
                    power+=agentMap[i]->accumulatedEnergy;
                power=power*(MAX_XE_CLOCK_SPEED_MHZ*1E-6/ROLLING_ENERGY_WINDOW/INST_PER_MEGA_INST);
                printf("unit 0 power (agg : real): %fW : %fW < goal: %fW\n", branchNodeState.powerTotal, power, branchNodeState.powerGoal);
                printf("blk 0 power (real): %fW < goal: %fW\n", readPowerMSR(), leafNodeState.powerGoal);
                printf("---------------------------\n");
                printf("  -----------------------------------------------------------------------------------------------------\n");
                for(AgentMap* down = &agent; down != NULL; down = down->btmNeighbor)
                {
                    printf(" | ");
                    for(AgentMap* right = down; right->rhtNeighbor; right = right->rhtNeighbor)
                    {
                        printf("%05.1f ",  right->temperature);
                        if (right->uid != right->rhtNeighbor->uid)
                            printf(" | ");
                    }
                    printf(" | \n");
                    if(!down->btmNeighbor || down->uid != down->btmNeighbor->uid)
                        printf("  -----------------------------------------------------------------------------------------------------\n");
                }
                fflush(stdout);
            }
        }

        updateClockMSR(); //Update clock.

        //Check if simulation is done.
        if(done == N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP)
        {
            /******************************************************************/
            #if TREE_BARRIERS == 1
            tree_barrier(); // Only barrier at sync interval.
            #else
            barrier(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP); // Only barrier at sync interval.
            #endif
            /******************************************************************/
            if (agent.id == 0)
            {
                printf("---------------------------\n");
                printf("==> Verifying aggregated information...\n");
                verifyAggregateData();
                printf("==> Printing statistics of execution...\n");
                u64 tasksExecuted = 0;
                u64 instsExecuted = 0;
                for (u64 i=0; i<N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP; i++)
                {
                    tasksExecuted+=agentMap[i]->statistics.tasksExecuted;
                    instsExecuted+=agentMap[i]->statistics.instsExecuted;
                }
                printf("  * Executed %ld tasks\n", tasksExecuted);
                printf("  * Executed %ld instructions\n", instsExecuted*INST_PER_MEGA_INST);
                printf("  * Executed %ld cycles\n", readClockMSR()*INST_PER_MEGA_INST);

                #if EXECUTION_TIMES == 1 || EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
                //Stop the timer for total execution time and print result
                auto simulationEndTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast< std::chrono::duration < double > > (simulationEndTime-simulationStartTime);
                std::cout << "  * Real Execution Time: " << duration.count() << " seconds \n";
                #endif
                printf("  * Simulated chip time: %lf ms\n", (FLOAT_TYPE)readClockMSR()*INST_PER_MEGA_INST/((FLOAT_TYPE)MAX_XE_CLOCK_SPEED_MHZ*1000));
                printf("==> Printing simulation timing statistics...\n");
                printf("  * Work Execution Time: %lf seconds\n", times.simulation);
                printf("  * Barriers Time:       %lf seconds\n", times.barriers);
                #if EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3
                printf("  * Roles Time total:    %lf seconds\n", times.roles);
                #if EXECUTION_TIMES == 3
                printf("    * Chip role Time:    %lf seconds\n", times.rolesChip);
                printf("    * Unit role Time:    %lf seconds\n", times.rolesUnit);
                printf("    * Block role Time:   %lf seconds\n", times.rolesBlock);
                #endif
                #endif

                printf("---------------------------\n");
                printf("Exiting program...\n");
                exit(0);
            }
            return;
        }
    }

    #if LOGGING_LEVEL == 1
    //close log.
    fclose(agent.logfile);
    #endif
}

