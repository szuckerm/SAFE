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


#ifndef _SS_INSTRUCTIONS_GUARD_
#define _SS_INSTRUCTIONS_GUARD_
#include "ss-conf.h"
#include "ss-temp.h"
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <cassert>

/* Anatomy of an instruction in the simulator. */
//FIXME THE NAME OF THE VARIABLES IS NOT CONSISTENT, LATENCY SHOULDNT BE REDUCED EVERY CYCLE
typedef struct __attribute__ ((__packed__)) InstType
{
    s64 type;
    FLOAT_TYPE halfStateEnergy; //Energy to get from half state.
    FLOAT_TYPE fullStateEnergy; //Energy numbers for task at full frequency.
    s64 latency;     //latency of the instruction.
    s64 multiplier;  //decrease factor for the task in full state energy.
} InstType;

/* Anatomy of a task in the simulator. */
typedef struct TaskType
{
    std::vector<ID_TYPE> instructions; //set of instructions of the task.
    FLOAT_TYPE fullEnergy = 0.0;       //energy assuming task is run at full freq.
    u64 totalCycles = 0;               //total cycles of the task assuming full freq.
} TaskType;

/* Anatomy of a task queue in the simulator. */
typedef std::vector<ID_TYPE> TaskQueueType;
/* Map of task types -- so we don't re-parse. */

/* Combined map and set for looking IDs from instructions/tasks from name or hash */
template <class TYPE>
class LookupContainer
{
    public:
        std::unordered_map<std::size_t, ID_TYPE> map;
        std::vector<TYPE> set;
    
    // During initialization, fill the first spot (0) as empty in the container.
    LookupContainer()
    {
        TYPE empty;
        memset(&empty, 0x0, sizeof(TYPE));
        set.push_back(empty);
        map[0]=set.size()-1;
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
    }
    
    // Add a new empty entry. This can be filled later by reference.
    auto add(std::string name) -> ID_TYPE
    {
        std::size_t hash = std::hash<std::string>{}(name);
        TYPE empty;
        memset(&empty, 0x0, sizeof(TYPE));
        set.push_back(empty);
        map[hash]=set.size()-1;
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
        assert(map.count(hash) != 0);
        return set.size()-1;
    }
    
    // Add a new entry and fill it in with the passed in values.
    auto add(std::string name, TYPE inst) -> ID_TYPE
    {
        std::size_t hash = std::hash<std::string>{}(name);
        set.push_back(inst);
        map[hash]=set.size()-1;
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
        assert(map.count(hash) != 0);
        return set.size()-1;
    }
    
    // Return the number of entries contained.
    auto size() -> u64
    {
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
        return set.size();
    }
    
    // Unsafe lookup of entry by ID (fastest). This will break horribly if passed an invalid ID.
    auto lookup(ID_TYPE id) -> TYPE&
    {
        return set.at(id);
    }
    
    // Safe lookup of entry by hash. Do not use this for speed critical code.
    auto lookup(std::size_t hash) -> TYPE&
    {
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
        if(map.count(hash) != 0)
        {
            return lookup(map.at(hash));
        }
        else
        {
            printf("Warning: instruction lookup failed for hash %lx\n", hash);
            return set[0];
        }
    }
    
    // Safe look up of entry by string. Do not use this for speed critical code.
    auto lookup(std::string name) -> TYPE&
    {
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
        std::size_t hash = std::hash<std::string>{}(name);
        return lookup(hash);
    }
    
    // safe lookup of ID by hash. Do not use this for speed critical code.
    auto lookup_id(std::size_t hash) -> ID_TYPE
    {
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
        if(map.count(hash) != 0)
            return map.at(hash);
        else
            return 0;
    }
    
    // Safe lookup of ID by string. Do not use this for speed critical code.
    auto lookup_id(std::string name) -> ID_TYPE
    {
        assert(map.size() == set.size()); // make sure the size of the map and set are the same.
        std::size_t hash = std::hash<std::string>{}(name);
        return lookup_id(hash);
    }
};

extern LookupContainer<InstType> instructionSet;
extern LookupContainer<TaskType> taskSet;
extern InstType noopInstruction;

extern FILE* stream;
extern FILE* instructionTableFile;
extern u64 totalLoadedInstructions;
extern TaskQueueType taskPool;
auto parseInputQueue()-> bool;
auto parseTask(char* name) -> TaskType;
auto openInstructionsFile(char * name) -> bool;
auto closeInstructionsFile() -> void;
auto fatal(const char *msg) -> void;
auto readInstructionsTable() -> bool;
auto verifyInstructionsTable() -> void;
auto openInstructionsTableFile() -> bool;
auto closeInstructionsTableFile() -> void;
auto initInstructionsContainers() -> void;
#endif
