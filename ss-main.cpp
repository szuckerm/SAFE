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


#include "ss-main.h"
#include <thread>
#include <cstdlib>
#include <unistd.h>


auto initializeLayout() -> void
{
    //Initialize layout parameters.
    chip_layout.chip_height_mm         = sqrt(S_CHIP_IN_MM);
    chip_layout.chip_width_mm          = sqrt(S_CHIP_IN_MM);
    chip_layout.chip_height_num_units  = sqrt(N_UNITS_IN_CHIP);
    chip_layout.chip_width_num_units   = sqrt(N_UNITS_IN_CHIP);
    chip_layout.unit_height_num_blocks = sqrt(N_BLOCKS_IN_UNIT);
    chip_layout.unit_width_num_blocks  = sqrt(N_BLOCKS_IN_UNIT);
    chip_layout.unit_height_mm         = chip_layout.chip_height_mm / chip_layout.chip_height_num_units;
    chip_layout.unit_width_mm          = chip_layout.chip_width_mm / chip_layout.chip_width_num_units;
    chip_layout.block_height_mm        = chip_layout.unit_height_mm / chip_layout.unit_height_num_blocks;
    chip_layout.block_width_mm         = chip_layout.unit_width_mm / chip_layout.unit_width_num_blocks;

    //Make sure layout is square.
    assert(chip_layout.chip_height_num_units*chip_layout.chip_width_num_units == N_UNITS_IN_CHIP);
    assert(chip_layout.unit_height_num_blocks*chip_layout.unit_width_num_blocks == N_BLOCKS_IN_UNIT);

    printf("---------------------------\n");
    printf("  * Chip Size:\t%.2lfmm by %.2lfmm\n", chip_layout.chip_height_mm, chip_layout.chip_width_mm);
    printf("  * Unit Size:\t%.2lfmm by %.2lfmm\n", chip_layout.unit_height_mm, chip_layout.unit_width_mm);
    printf("  * Block Size:\t%.2lfmm by %.2lfmm\n", chip_layout.block_height_mm, chip_layout.block_width_mm);
    printf("  * Total Count:\t%ld blocks\n", N_UNITS_IN_CHIP*N_BLOCKS_IN_UNIT);
    printf("  * Unit Count:\t%ld by %ld units\n", chip_layout.chip_height_num_units, chip_layout.chip_width_num_units);
    printf("  * Block Count:\t%ld by %ld blocks\n", chip_layout.unit_height_num_blocks, chip_layout.unit_width_num_blocks);
    printf("---------------------------\n");
}

auto sanityChecks() -> void
{
    //Various Sanity info.
    assert(sizeof(saMetadata)*8==64);
    assert(sizeof(saEdtInfoMetadata)*8==64);
    assert(sizeof(saBlkInfoMetadata)*8==64);
    assert(sizeof(saAggInfoMetadata)*8==64);
    assert(sizeof(saBlkCtrlMetadata)*8==64);
    assert(sizeof(saErrInfoMetadata)*8==64);
    assert(sizeof(saLocation)*8==72);
}


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("sim <work queue>");
        exit(0);
    }
    printf("==> Sanity checking API...\n");
    sanityChecks();

    const char* SAFE_LOGS_PATH = getenv("SAFE_LOGS_PATH");
    if (SAFE_LOGS_PATH == 0) SAFE_LOGS_PATH = "./logs";
    //Make logs directory.
    mkdir(SAFE_LOGS_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    printf("==> Initializing chip layout...\n");
    initializeLayout();
    
    printf("==> Initializing temperature model...");
    temperatureModelInit(); 
    printf(" done...\n");
    printf("---------------------------\n");

    if (argc == 1)
        assert(0);
    else
    {
        printf("==> Reading instructions table file...\n");
        openInstructionsTableFile();
        readInstructionsTable(); // Check regular instructions.
        printf("==> Verifying instruction table...");
        verifyInstructionsTable();
        printf(" done...\n");
        printf("---------------------------\n");
        printf("==> Reading instructions file\n");
        openInstructionsFile(argv[1]);
        parseInputQueue();
        verifyInstructionsTable(); // Check mega instructions.
        
        closeInstructionsFile();
        closeInstructionsTableFile();
    }

    #if EXECUTION_TIMES == 1 || EXECUTION_TIMES == 2 || EXECUTION_TIMES == 3 
      //Start the timer for total execution time
      simulationStartTime = std::chrono::high_resolution_clock::now();
    #endif

    printf("---------------------------\n");
    printf("==> Starting threads...\n");
    std::thread thread[N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP];

    //Create the first engine with work.
    thread[0] = std::thread(&engine, 0);

    //Create the rest of the engines without work.
    for (u64 tid=1; tid<N_BLOCKS_IN_UNIT*N_UNITS_IN_CHIP; tid++)
    {
        thread[tid] = std::thread(&engine, tid);
        
        /*
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (u64 off=0; off<=18; off+=6)
        {
            if( (tid>=0+off && tid<=5+off) || (tid>=24+off && tid<=29+off) )
            {
                for(u64 proc=0+off; proc<=5+off; proc++)
                    CPU_SET(proc, &cpuset);
                for(u64 proc=24+off; proc<=29+off; proc++)
                    CPU_SET(proc, &cpuset);
            }
        }
        pthread_setaffinity_np(thread[tid].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        */
    }

    for (;;) pause();

    return EXIT_SUCCESS;
}
