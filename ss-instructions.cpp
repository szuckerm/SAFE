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


#include "ss-instructions.h"
#include <cstdlib>

LookupContainer<InstType> instructionSet;
LookupContainer<TaskType> taskSet;
InstType noopInstruction;
FILE* stream;
FILE* instructionTableFile;
TaskQueueType taskPool;

auto fatal(const char *msg) -> void
{
    perror(msg);
    exit(errno);
}

auto initNoopReference() -> void
{
    noopInstruction = instructionSet.lookup("no_op");
}

auto parseTask(char* name) -> TaskType
{
    //Open task file.
    char inputfile[1024];
    const char* SAFE_INPUT_DIRPATH = getenv("SAFE_INPUT_DIRPATH");
    //Set default input path if not specified
    if (SAFE_INPUT_DIRPATH == 0) {SAFE_INPUT_DIRPATH = "./input"; printf("==> Setting default input folder ./input/ \n");}
    sprintf (inputfile, "%s/%s%s", SAFE_INPUT_DIRPATH, name, TASK_FILE_SUFFIX);
    FILE* stream = fopen(inputfile, "r");
    if(stream == NULL)
        fatal(inputfile);

    //Read each line of the task file for the details of the task.
    TaskType task;
    char* line = inputfile; //reuse.

    u64 consolidatedInstructions = 0;
    InstType megaInst = {0};
    std::string megaInstName = "";
    u64 numberInstructions = 0;
    while (fgets(line, 1024, stream))
    {
        if(strlen(line) > 0 && line[0] != '\n')
        {
            std::string instName = strtok(line," \t");

            //get the instructions' values from the instructionsMap. Check if it is valid instruction.
            auto id = instructionSet.lookup_id(instName); //lookup id.
            if ( id != 0 )
            {
                //grab instruction reference.
                auto inst = instructionSet.lookup(id);

                //Add information to mega instruction.
                megaInstName += instName;
                megaInst.type            += inst.type*inst.latency;
                megaInst.fullStateEnergy += inst.fullStateEnergy;
                megaInst.halfStateEnergy += inst.halfStateEnergy;
                megaInst.latency         += inst.latency;
                megaInst.multiplier      += inst.multiplier;

                consolidatedInstructions++; //Mark that we've got an instruction.
                
                // Check if mega instruction limit reached.
                if (consolidatedInstructions == INST_PER_MEGA_INST)
                {
                    //Consolidate mega instruction.
                    megaInst.latency    /= consolidatedInstructions; // correct latency.
                    megaInst.multiplier /= consolidatedInstructions; // correct multiplier.
                    megaInst.type       /= consolidatedInstructions; // correct type (used to dram port access)

                    // Add energy and cycles to task.
                    task.fullEnergy  += megaInst.fullStateEnergy*megaInst.latency; //accumulate total possible energy for the task.
                    task.totalCycles += megaInst.latency;
                    
                    //Check if mega instruction is part of the instruction set yet if not add.
                    auto id = instructionSet.lookup_id(megaInstName);
                    if(id == 0)
                        id = instructionSet.add(megaInstName, megaInst);
                    
                    //Add instruction to the ask.
                    task.instructions.push_back(id);
                    
                    //Reset mega instruction.
                    megaInstName = "";
                    megaInst = {0};
                    consolidatedInstructions = 0;
                }
                numberInstructions++;
            }
            else
            {
                printf("Unknown instruction: %s in Task: %s \n",instName.c_str(),name);
            }
        }
    }
    
    //Check if mega instruction was being built when the file ended and add it to task if so.
    if(consolidatedInstructions != 0)
    {
        //Consolidate mega instruction.
        megaInst.latency    /= consolidatedInstructions; // correct latency.
        megaInst.multiplier /= consolidatedInstructions; // correct multiplier.
        
        // Add energy and cycles to task.
        task.fullEnergy  += megaInst.fullStateEnergy*megaInst.latency; //accumulate total possible energy for the task.
        task.totalCycles += megaInst.latency;
        
        //Check if mega instruction is part of the instruction set yet if not add.
        auto id = instructionSet.lookup_id(megaInstName);
        if(id == 0)
            id = instructionSet.add(megaInstName, megaInst);
        
        //Add instruction to the ask.
        task.instructions.push_back(id);
    }
    
    printf("  * Found new task '%s' consisting of %ld instructions...\n", name, numberInstructions);
    return task;
}

auto parseInputQueue()-> bool
{
    //Read each line of the queue file for each task.
    char line[1024];

    u64 totalLoadedInstructions=0;
    u64 loadedInstructions = 0;
    u64 totalCycles        = 0;
    FLOAT_TYPE totalEnergy = 0.0;
    bool loaded            = false;

    while((!feof(stream)))
    {
        if(fgets(line, 1024, stream)!=NULL)
        {
            if(strlen(line) > 0 && line[0] != '\n')
            {
                char* name = strtok(line," \t");
                auto id = taskSet.lookup_id(name);
                //Check if task already in task map.
                if(id == 0)
                {
                    //Task type not found... load the file
                    id = taskSet.add(name, parseTask(name));
                }

                // Grab statistics.
                loadedInstructions += taskSet.lookup(id).instructions.size();
                totalEnergy        += taskSet.lookup(id).fullEnergy;
                totalCycles        += taskSet.lookup(id).totalCycles;

                // Push task ID in task pool.
                taskPool.push_back(id);
                loaded=true;
            }
        }
    }
    totalLoadedInstructions+=loadedInstructions;
    initNoopReference(); //static reference to noop
   
    // Print useful stats...;
    printf("---------------------------\n");
    printf("* Estimated Total Instructions: %ld\n", totalLoadedInstructions*INST_PER_MEGA_INST*TASK_MULTIPLIER);
    printf("* Estimated Total Cycles: %ld\n", totalCycles*TASK_MULTIPLIER*INST_PER_MEGA_INST/(N_CORES_IN_BLOCK*N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP));
    printf("* Estimated Total Possible Energy (assuming full operation): %f pJ\n", totalEnergy*TASK_MULTIPLIER);
    printf("* Estimated Avg energy per block (assuming full operation):  %f pJ\n", totalEnergy*TASK_MULTIPLIER/(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP));
    printf("* Estimated Avg Possible temperature differential: %f C\n", estimateTemperature(totalEnergy*TASK_MULTIPLIER/(N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP), totalCycles*TASK_MULTIPLIER/(N_CORES_IN_BLOCK*N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP)));
    return loaded;
}

auto openInstructionsFile(char * name) -> bool
{
  //Open queue file.
  char inputfile[1024];
  const char* SAFE_INPUT_DIRPATH = getenv("SAFE_INPUT_DIRPATH");
  if (SAFE_INPUT_DIRPATH == 0) SAFE_INPUT_DIRPATH = "./input";
  sprintf (inputfile, "%s/%s%s", SAFE_INPUT_DIRPATH, name, QUEUE_FILE_SUFFIX);
  printf("==> Reading work from '%s'...\n", inputfile);
  stream = fopen(inputfile, "r");
  if(stream == NULL)
  {
        fatal(inputfile);
        return false;
  }
  return true;
}

auto closeInstructionsFile() -> void
{
  if (stream != NULL)
    fclose(stream);
}


auto openInstructionsTableFile() -> bool
{
  //Open the table where all the instructions' energy are specified. 
  instructionTableFile = fopen(INSTRUCTION_TABLE_FILE_NAME,"r");
  if (instructionTableFile==NULL)
  {
      fatal(INSTRUCTION_TABLE_FILE_NAME);
      return false;
  } 
  return true;
}

auto readInstructionsTable() -> bool
{
    //Read the instructions' energy values according to the technology being use. Ignore comment and empty lines
    char line[1024];

    //Read the whole file
    while (!feof(instructionTableFile))
    {
        //If there is a file to read 
        if (fgets(line,1024,instructionTableFile)!=NULL)
        {
         //If the line is not empty, nor a comment or the header it has to be an operation line 
         if (strlen(line) > 0 && line[0] != '\n' && line[0] != '/'  && line[1] != '/')
         {
            char * op          = strtok(line," \t");
            u64 type           = atoi(strtok(NULL," \t"));
            u64 latency        = atoi(strtok(NULL," \t"));
            FLOAT_TYPE regNum  = atof(strtok(NULL," \t"));
            FLOAT_TYPE fetch   = atof(strtok(NULL," \t"));
            FLOAT_TYPE decode  = atof(strtok(NULL," \t"));
            FLOAT_TYPE issue   = atof(strtok(NULL," \t"));
            FLOAT_TYPE RF      = atof(strtok(NULL," \t"));
            FLOAT_TYPE exec    = atof(strtok(NULL," \t"));
            FLOAT_TYPE msr     = atof(strtok(NULL," \t"));
            FLOAT_TYPE commit  = atof(strtok(NULL," \t"));
            
            //Check if an instruction already exist in the table. If so we just add missing values.
            auto id = instructionSet.lookup_id(op);
            
            //Add empty instruction or reference existing instruction.
            InstType& inst = (id == 0 ? instructionSet.lookup(instructionSet.add(op)) : instructionSet.lookup(id));
            
            FLOAT_TYPE totalEnergy = fetch + decode + ((1.0 + regNum)*issue) + (regNum * RF) + exec + msr + commit;
            if(type==0)
            {
                // for FULL state.
                inst.type = 0;
                inst.multiplier=latency; //store full state latency as the multiplier for the instruction.
                inst.fullStateEnergy=totalEnergy*(1+STATIC_ENERGY_FACTOR_FULL);
            }
            else if (type==1)
            {
                // for actual NTV state.
                inst.type = 0;
                inst.latency=latency; // Store half state value as actual latency for the instruction.
                inst.halfStateEnergy=totalEnergy*(1+STATIC_ENERGY_FACTOR_HALF);
            }
            else if (type==2)
            {
                // for memory instructions with no NTV state.
                inst.type = 0;
                inst.latency=latency; // Store the actual latency for the instruction.
                inst.multiplier=1; // Store multiplier as 1 so the time is the same regardless of NTV state.
                inst.fullStateEnergy=totalEnergy; // store the same energy for both full and half.
                inst.halfStateEnergy=totalEnergy;
            }
            else if (type==3)
            {
                // for memory instructions with no NTV state.
                inst.type = 1;
                inst.latency=latency; // Store the actual latency for the instruction.
                inst.multiplier=1; // Store multiplier as 1 so the time is the same regardless of NTV state.
                inst.fullStateEnergy=totalEnergy; // store the same energy for both full and half.
                inst.halfStateEnergy=totalEnergy;
            }
            
            if (inst.multiplier && inst.latency)
            {
                inst.fullStateEnergy /= (inst.latency / inst.multiplier); // full state energy is divided over the number of 'full' state cycles.
                inst.halfStateEnergy /= inst.latency;                     // half state energy is divided over the number of 'half' state cycles or total latency.
                printf("  * Found instruction\t%16s\tenergy: %7.5f, %7.5f\n", op, inst.fullStateEnergy, inst.halfStateEnergy);
            }
         }
      }
  }
  return true;
}

auto verifyInstructionsTable() -> void
{
    // Check instructions. index 0 is empty so skip it.
    for(ID_TYPE id = 1; id< instructionSet.size(); id++)
    {
        auto inst = instructionSet.lookup(id);
        if (inst.multiplier == 0 || inst.latency == 0 ||
            inst.fullStateEnergy == 0.0 || inst.halfStateEnergy == 0.0)
            printf("\n  * Error verifying instruction id %d: multiplier %ld, latency %ld, full energy %f, half energy %f...\n",
                   id, inst.multiplier, inst.latency, inst.fullStateEnergy, inst.halfStateEnergy);
    }
}

auto closeInstructionsTableFile() -> void
{
   if (instructionTableFile != NULL)
   {
       fclose(instructionTableFile);
   }
}
