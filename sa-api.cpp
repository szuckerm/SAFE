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


#include "sa-api.h"
#include <stdio.h>

#include "ss-conf.h"
#include "ss-agent.h"
#include "ss-pack.h"

/* C style declarations because these implement the SA API. */

u8 saSetMetadata(void* metadata, saMetadataAttr attr, void* val)
{
    if (metadata != NULL)
    {
        switch (((saMetadata*)metadata)->SA_ATR_METADATA_TYPE)
        {
            case SA_METADATA_TYPE_EDT_INFO:
            {
                saEdtInfoMetadata* md = (saEdtInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_EDT_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE:        { md->SA_ATR_METADATA_TYPE        = *(u64*)val; break; }
                    case SA_ATR_URGENCY:              { md->SA_ATR_URGENCY              = *(u64*)val; break; }
                    case SA_ATR_UTILITY_SCORE:        { md->SA_ATR_UTILITY_SCORE        = *(u64*)val; break; }
                    case SA_ATR_OPS_FP:               { md->SA_ATR_OPS_FP               = *(u64*)val; break; }
                    case SA_ATR_OPS_INT:              { md->SA_ATR_OPS_INT              = *(u64*)val; break; }
                    case SA_ATR_MEM_DMA_SCORE:        { md->SA_ATR_MEM_DMA_SCORE        = *(u64*)val; break; }
                    case SA_ATR_MEM_LOCAL_SCORE:      { md->SA_ATR_MEM_LOCAL_SCORE      = *(u64*)val; break; }
                    case SA_ATR_MEM_REMOTE_SCORE:     { md->SA_ATR_MEM_REMOTE_SCORE     = *(u64*)val; break; }
                    case SA_ATR_POW_STATIC:           { md->SA_ATR_POW_STATIC           = *(u64*)val; break; }
                    case SA_ATR_POW_DYNAMIC_INTERNAL: { md->SA_ATR_POW_DYNAMIC_INTERNAL = *(u64*)val; break; }
                    case SA_ATR_POW_DYNAMIC_EXTERNAL: { md->SA_ATR_POW_DYNAMIC_EXTERNAL = *(u64*)val; break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for EDT metadata.\n");
                        return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_BLK_INFO:
            {
                saBlkInfoMetadata* md = (saBlkInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_BLK_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE:    { md->SA_ATR_METADATA_TYPE    = *(u64*)val; break; }
                    case SA_ATR_URGENCY:          { md->SA_ATR_URGENCY          = *(u64*)val; break; }
                    case SA_ATR_UTILITY_SCORE:    { md->SA_ATR_UTILITY_SCORE    = *(u64*)val; break; }
                    case SA_ATR_MEM_DMA_SCORE:    { md->SA_ATR_MEM_DMA_SCORE    = *(u64*)val; break; }
                    case SA_ATR_MEM_LOCAL_SCORE:  { md->SA_ATR_MEM_LOCAL_SCORE  = *(u64*)val; break; }
                    case SA_ATR_MEM_REMOTE_SCORE: { md->SA_ATR_MEM_REMOTE_SCORE = *(u64*)val; break; }
                    case SA_ATR_FUB_XE_COUNT:     { md->SA_ATR_FUB_XE_COUNT     = *(u64*)val; break; }
                    case SA_ATR_FUB_FPU_COUNT:    { md->SA_ATR_FUB_FPU_COUNT    = *(u64*)val; break; }
                    case SA_ATR_FUB_BSM_COUNT:    { md->SA_ATR_FUB_BSM_COUNT    = *(u64*)val; break; }
                    case SA_ATR_FUB_DVFS_SCORE:   { md->SA_ATR_FUB_DVFS_SCORE   = *(u64*)val; break; }
                    case SA_ATR_FUB_POW_SCORE:    { md->SA_ATR_FUB_POW_SCORE    = packPowerData(*(FLOAT_TYPE*)val, MAX_POWER_PER_BLOCK, 16); break; }
                    case SA_ATR_FUB_TEMP_SCORE:   { md->SA_ATR_FUB_TEMP_SCORE   = *(FLOAT_TYPE*)val; break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for BLK INFO metadata.\n");
                        return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_AGG_INFO:
            {
                saAggInfoMetadata* md = (saAggInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_AGG_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE: { md->SA_ATR_METADATA_TYPE = *(u64*)val; break; }
                    case SA_ATR_URGENCY:       { md->SA_ATR_URGENCY       = *(u64*)val; break; }
                    case SA_ATR_AGG_POW_MEAN:  { md->SA_ATR_AGG_POW_MEAN  = packPowerData(*(FLOAT_TYPE*)val, MAX_POWER_PER_BLOCK, 16); break; }
                    case SA_ATR_AGG_POW_SD:    { md->SA_ATR_AGG_POW_SD    = packPowerData(*(FLOAT_TYPE*)val, MAX_POWER_PER_BLOCK, 12); break; }
                    case SA_ATR_AGG_POW_SKEW:  { md->SA_ATR_AGG_POW_SKEW  = packSkewData(*(FLOAT_TYPE*)val); break; }
                    case SA_ATR_AGG_TEMP_MEAN: { md->SA_ATR_AGG_TEMP_MEAN = packTemperatureData(*(FLOAT_TYPE*)val); break; }
                    case SA_ATR_AGG_TEMP_SD:   { md->SA_ATR_AGG_TEMP_SD   = packTemperatureData(*(FLOAT_TYPE*)val); break; }
                    case SA_ATR_AGG_TEMP_SKEW: { md->SA_ATR_AGG_TEMP_SKEW = packSkewData(*(FLOAT_TYPE*)val); break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for AGG INFO metadata.\n");
                        return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_BLK_CTRL:
            {
                saBlkCtrlMetadata* md = (saBlkCtrlMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_BLK_CTRL);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE:    { md->SA_ATR_METADATA_TYPE     = *(u64*)val; break; }
                    case SA_ATR_URGENCY:          { md->SA_ATR_URGENCY           = *(u64*)val; break; }
                    case SA_ATR_FUB_CTRL:         { md->SA_ATR_FUB_CTRL          = *(u64*)val; break; }
                    case SA_ATR_FUB_XE_COUNT:     { md->SA_ATR_FUB_XE_COUNT      = *(u64*)val; break; }
                    case SA_ATR_FUB_FPU_COUNT:    { md->SA_ATR_FUB_FPU_COUNT     = *(u64*)val; break; }
                    case SA_ATR_FUB_BSM_COUNT:    { md->SA_ATR_FUB_BSM_COUNT     = *(u64*)val; break; }
                    case SA_ATR_FUB_DVFS_SCORE:   { md->SA_ATR_FUB_DVFS_SCORE    = *(u64*)val; break; }
                    case SA_ATR_FUB_POWER_GOAL:   { md->SA_ATR_FUB_POWER_GOAL    = packPowerData(*(FLOAT_TYPE*)val, MAX_POWER_PER_BLOCK, 16); break; }
                    case SA_ATR_FUB_UNDER_CONTROL:{ md->SA_ATR_FUB_UNDER_CONTROL = *(u64*)val; break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for CTRL metadata.\n");
                        return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_ERR_INFO:
            {
                saErrInfoMetadata* md = (saErrInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_ERR_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE:    { md->SA_ATR_METADATA_TYPE = *(u64*)val; break; }
                    case SA_ATR_URGENCY:          { md->SA_ATR_URGENCY       = *(u64*)val; break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for CTRL metadata.\n");
                        return -1;
                }
                return 0;
            }
        }
        printf("WARNING: unsupported metadata type.\n");
        return -1;
    }
    printf("WARNING: NULL metadata pointer.\n");
    return -1;
}

//Code is almost exactly the same as saSetMetadata.
u8 saGetMetadata(void* metadata, saMetadataAttr attr, void* val)
{
    if (metadata != NULL)
    {
        switch (((saMetadata*)metadata)->SA_ATR_METADATA_TYPE)
        {

            case SA_METADATA_TYPE_EDT_INFO:
            {
                saEdtInfoMetadata* md = (saEdtInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_EDT_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE:        { *(u64*)val = md->SA_ATR_METADATA_TYPE; break; }
                    case SA_ATR_URGENCY:              { *(u64*)val = md->SA_ATR_URGENCY; break; }
                    case SA_ATR_UTILITY_SCORE:        { *(u64*)val = md->SA_ATR_UTILITY_SCORE; break; }
                    case SA_ATR_OPS_FP:               { *(u64*)val = md->SA_ATR_OPS_FP; break; }
                    case SA_ATR_OPS_INT:              { *(u64*)val = md->SA_ATR_OPS_INT; break; }
                    case SA_ATR_MEM_DMA_SCORE:        { *(u64*)val = md->SA_ATR_MEM_DMA_SCORE; break; }
                    case SA_ATR_MEM_LOCAL_SCORE:      { *(u64*)val = md->SA_ATR_MEM_LOCAL_SCORE; break; }
                    case SA_ATR_MEM_REMOTE_SCORE:     { *(u64*)val = md->SA_ATR_MEM_REMOTE_SCORE; break; }
                    case SA_ATR_POW_STATIC:           { *(u64*)val = md->SA_ATR_POW_STATIC; break; }
                    case SA_ATR_POW_DYNAMIC_INTERNAL: { *(u64*)val = md->SA_ATR_POW_DYNAMIC_INTERNAL; break; }
                    case SA_ATR_POW_DYNAMIC_EXTERNAL: { *(u64*)val = md->SA_ATR_POW_DYNAMIC_EXTERNAL; break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for EDT metadata.\n");
                        return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_BLK_INFO:
            {
                saBlkInfoMetadata* md = (saBlkInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_BLK_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE:    { *(u64*)val    = md->SA_ATR_METADATA_TYPE; break; }
                    case SA_ATR_URGENCY:          { *(u64*)val    = md->SA_ATR_URGENCY; break; }
                    case SA_ATR_UTILITY_SCORE:    { *(u64*)val    = md->SA_ATR_UTILITY_SCORE; break; }
                    case SA_ATR_MEM_DMA_SCORE:    { *(u64*)val    = md->SA_ATR_MEM_DMA_SCORE; break; }
                    case SA_ATR_MEM_LOCAL_SCORE:  { *(u64*)val    = md->SA_ATR_MEM_LOCAL_SCORE; break; }
                    case SA_ATR_MEM_REMOTE_SCORE: { *(u64*)val    = md->SA_ATR_MEM_REMOTE_SCORE; break; }
                    case SA_ATR_FUB_XE_COUNT:     { *(u64*)val    = md->SA_ATR_FUB_XE_COUNT; break; }
                    case SA_ATR_FUB_FPU_COUNT:    { *(u64*)val    = md->SA_ATR_FUB_FPU_COUNT; break; }
                    case SA_ATR_FUB_BSM_COUNT:    { *(u64*)val    = md->SA_ATR_FUB_BSM_COUNT; break; }
                    case SA_ATR_FUB_DVFS_SCORE:   { *(u64*)val    = md->SA_ATR_FUB_DVFS_SCORE; break; }
                    case SA_ATR_FUB_POW_SCORE:    { *(FLOAT_TYPE*)val = unpackPowerData(md->SA_ATR_FUB_POW_SCORE, MAX_POWER_PER_BLOCK, 16); break; }
                    case SA_ATR_FUB_TEMP_SCORE:   { *(FLOAT_TYPE*)val = md->SA_ATR_FUB_TEMP_SCORE; break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for BLK INFO metadata.\n");
                        return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_AGG_INFO:
            {
                saAggInfoMetadata* md = (saAggInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_AGG_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE: { *(u64*)val    = md->SA_ATR_METADATA_TYPE; break; }
                    case SA_ATR_URGENCY:       { *(u64*)val    = md->SA_ATR_URGENCY; break; }
                    case SA_ATR_AGG_POW_MEAN:  { *(FLOAT_TYPE*)val = unpackPowerData(md->SA_ATR_AGG_POW_MEAN, MAX_POWER_PER_BLOCK, 16); break; }
                    case SA_ATR_AGG_POW_SD:    { *(FLOAT_TYPE*)val = unpackPowerData(md->SA_ATR_AGG_POW_SD, MAX_POWER_PER_BLOCK, 12); break; }
                    case SA_ATR_AGG_POW_SKEW:  { *(FLOAT_TYPE*)val = unpackSkewData(md->SA_ATR_AGG_POW_SKEW); break; }
                    case SA_ATR_AGG_TEMP_MEAN: { *(FLOAT_TYPE*)val = unpackTemperatureData(md->SA_ATR_AGG_TEMP_MEAN); break; }
                    case SA_ATR_AGG_TEMP_SD:   { *(FLOAT_TYPE*)val = unpackTemperatureData(md->SA_ATR_AGG_TEMP_SD); break; }
                    case SA_ATR_AGG_TEMP_SKEW: { *(FLOAT_TYPE*)val = unpackSkewData(md->SA_ATR_AGG_TEMP_SKEW); break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for AGG INFO metadata.\n");
                        return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_BLK_CTRL:
            {
                saBlkCtrlMetadata* md = (saBlkCtrlMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_BLK_CTRL);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE:    { *(u64*)val = md->SA_ATR_METADATA_TYPE; break; }
                    case SA_ATR_URGENCY:          { *(u64*)val = md->SA_ATR_URGENCY; break; }
                    case SA_ATR_FUB_CTRL:         { *(u64*)val = md->SA_ATR_FUB_CTRL; break; }
                    case SA_ATR_FUB_XE_COUNT:     { *(u64*)val = md->SA_ATR_FUB_XE_COUNT; break; }
                    case SA_ATR_FUB_FPU_COUNT:    { *(u64*)val = md->SA_ATR_FUB_FPU_COUNT; break; }
                    case SA_ATR_FUB_BSM_COUNT:    { *(u64*)val = md->SA_ATR_FUB_BSM_COUNT; break; }
                    case SA_ATR_FUB_DVFS_SCORE:   { *(u64*)val = md->SA_ATR_FUB_DVFS_SCORE; break; }
                    case SA_ATR_FUB_POWER_GOAL:   { *(FLOAT_TYPE*)val = unpackPowerData(md->SA_ATR_FUB_POWER_GOAL, MAX_POWER_PER_BLOCK, 16); break; }
                    case SA_ATR_FUB_UNDER_CONTROL:{ *(u64*)val = md->SA_ATR_FUB_UNDER_CONTROL; break; }
                    default:
                      printf("WARNING: attempt to retrieve unsupported attribute for CTRL metadata.\n");
                      return -1;
                }
                return 0;
            }
            case SA_METADATA_TYPE_ERR_INFO:
            {
                saErrInfoMetadata* md = (saErrInfoMetadata*)metadata;
                assert(md->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_ERR_INFO);
                switch (attr)
                {
                    case SA_ATR_METADATA_TYPE: { *(u64*)val = md->SA_ATR_METADATA_TYPE; break; }
                    case SA_ATR_URGENCY:       { *(u64*)val = md->SA_ATR_URGENCY; break; }
                    default:
                        printf("WARNING: attempt to retrieve unsupported attribute for CTRL metadata.\n");
                        return -1;
                }
                return 0;
            }
        }
        printf("WARNING: unsupported metadata type.\n");
        return -1;
    }
    printf("WARNING: NULL metadata pointer.\n");
    return -1;
}


#include <map>
typedef std::map <ocrGuid_t*, saEdtInfoMetadata> _saGuidMap;
_saGuidMap _saEdtToMetadataMap;

u8 ocrInitMetadata(ocrGuid_t* guid, saEdtInfoMetadata* metadata)
{
    if (metadata != NULL)
    {
        assert(metadata->SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_EDT_INFO);
        _saEdtToMetadataMap.at(guid) = *metadata;
    }
    else
    {
        saEdtInfoMetadata defaultMetadata = {0};
        defaultMetadata.SA_ATR_METADATA_TYPE = SA_METADATA_TYPE_EDT_INFO;
        _saEdtToMetadataMap[guid] = defaultMetadata;
    }
    return 0;
}

u8 ocrSetMetadata(ocrGuid_t* guid, saMetadataAttr attr, u16* val)
{
    _saGuidMap::iterator it = _saEdtToMetadataMap.find(guid);
    assert(it->second.SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_EDT_INFO); //sanity.
    if (it != _saEdtToMetadataMap.end())
    {
        return saSetMetadata(&it->second, attr, val);
    }
    else
    {
        return -1;
    }
}

u8 ocrGetMetadata(ocrGuid_t* guid, saMetadataAttr attr, u16* val)
{
    _saGuidMap::iterator it = _saEdtToMetadataMap.find(guid);
    assert(it->second.SA_ATR_METADATA_TYPE==SA_METADATA_TYPE_EDT_INFO); //sanity.
    if (it != _saEdtToMetadataMap.end())
    {
        return saGetMetadata(&it->second, attr, val);
    }
    else
    {
        return -1;
    }
}

/*
    The new mailbox layout is such that the child 'owns' the slots. This means
    that the slots are allocated in the child's local memory. For each role
    the child has a 64-bit read and write slot in the following configuration:
    [Br][Bw][Ur][Uw][Cr][Cw]

    In other words, there are r/w block, unit, and chip slots allocated consecutively.
*/

u8 saSendMetadata(saLocation location, void* metadata)
{
    //new simplified mailbox layout...
    //read and write slots flip depending on whether this is for a parent location.
    u64* slot = (location.parent?((u64*)location.offset)+1:((u64*)location.offset));

    if(*slot)
        return -1;
    *slot = *(u64*) metadata;

    return 0;
}

u8 saRecvMetadata(saLocation location, void* metadata)
{
    //new simplified mailbox layout...
    //read and write slots flip depending on whether this is for a parent location.
    u64* slot = (location.parent?((u64*)location.offset):((u64*)location.offset)+1);

    *(u64*)metadata = *slot;
    *slot = 0x0;

    return 0;
}

saLocation saGetParentLocation()
{
    saLocation location;
    location.parent = 1;
    location.offset = (u64)(agent.memory + agent.role*2);

    return location;
}

saLocation saGetChildLocation(u32 num)
{
    saLocation location;
    location.parent = 0;
    switch(agent.role-1)
    {
        case ROLE_STATE_BLOCK:
            location.offset = (u64)(agentMap[agent.uid*N_BLOCKS_IN_UNIT+num]->memory);
            break;
        case ROLE_STATE_UNIT:
            location.offset = (u64)(agentMap[num*N_BLOCKS_IN_UNIT]->memory + 2);
            break;
        default:
            printf("WARNING: We shouldn't reach here!\n");
            assert(0);
    }

    return location;
}

/* Simple testing harness below */

int bitCount (u64 num)
{
    if(!num)
        return 0;
    u64 count=1;
    while (num >>= 1)
        count++;
    return count;
}

#define _SETTEST_OCR(ATR, VAL) \
    while(1) \
    { \
    printf("OCR API: Setting and retrieving %s with value of %d: ", #ATR, VAL); \
    test = VAL; \
    if(ocrSetMetadata(&harness, ATR, &test) != 0) \
    { \
        printf(" ocrSetMetadata returned an error code!\n"); \
        break; \
    } \
    test = 0; \
    if(ocrGetMetadata(&harness, ATR, &test) != 0) \
    { \
        printf(" ocrGetMetadata returned an error code!\n"); \
        break; \
    } \
    printf("returned %d, %s \n", test, (test == VAL) ? "Success" : "FAILED"); \
    break; \
    }
#define _TEST_OCR(ATR, VAL) \
    while(1) \
    { \
    printf("OCR API: Retrieving %s with value of %d: ", #ATR, VAL); \
    test = 0; \
    if(ocrGetMetadata(&harness, ATR, &test) != 0) \
    { \
        printf(" ocrGetMetadata returned an error code!\n"); \
        break; \
    } \
    printf("returned %d, %s \n", test, (test == VAL) ? "Success" : "FAILED"); \
    break; \
    }
#define _SETTEST_SA(META, ATR, VAL) \
    while(1) \
    { \
    printf("SA API: setting and retrieving %s with value of %d: ", #ATR, VAL); \
    test = VAL; \
    if(saSetMetadata(&META, ATR, &test) != 0) \
    { \
        printf(" saSetMetadata returned an error code!\n"); \
        break; \
    } \
    test = 0; \
    if(saGetMetadata(&META, ATR, &test) != 0) \
    { \
        printf(" saGetMetadata returned an error code!\n"); \
        break; \
    } \
    printf("returned %d, %s \n", test, (test == VAL) ? "Success" : "FAILED"); \
    break; \
    }
#define _TEST_SA(META, ATR, VAL) \
    while(1) \
    { \
    printf("SA API: Retrieving %s with value of %d: ", #ATR, VAL); \
    test = 0; \
    if(saGetMetadata(&META, ATR, &test) != 0) \
    { \
        printf(" saGetMetadata returned an error code!\n"); \
        break; \
    } \
    printf("returned %d, %s \n", test, (test == VAL) ? "Success" : "FAILED"); \
    break; \
    }
/*
void harness()
{
    u16 test;

    //Various Sanity info.
    assert(sizeof(saMetadata)*8==64); //local to this file.
    assert(sizeof(saEdtInfoMetadata)*8==64);
    assert(sizeof(saBlkInfoMetadata)*8==64);
    assert(sizeof(saBlkCtrlMetadata)*8==64);
    assert(sizeof(saErrInfoMetadata)*8==64);
    assert(sizeof(saLocation)*8==64);

    //must fit all possible attributes.
    assert(bitCount(SA_ATR_LAST)<sizeof(saMetadataAttr)*8);

    printf("Address of harness function: \t%lx\n", &harness);

    //OCR API tests.
    ocrInitMetadata(&harness, NULL);
    _TEST_OCR(SA_ATR_METADATA_TYPE, SA_METADATA_TYPE_EDT_INFO);
    _SETTEST_OCR(SA_ATR_OPS_FP, 1);
    _SETTEST_OCR(SA_ATR_OPS_INT, 1);
    _SETTEST_OCR(SA_ATR_UTILITY_SCORE, 15);
    saEdtInfoMetadata a1 = SA_EDT_INFO_METADATA_BANDWIDTH_INITIALIZER;
    ocrInitMetadata(&harness, &a1);
    _TEST_OCR(SA_ATR_METADATA_TYPE, SA_METADATA_TYPE_EDT_INFO);
    _TEST_OCR(SA_ATR_MEM_REMOTE_SCORE, 0b11);

    //BLK tests.
    saBlkInfoMetadata a2 = SA_BLK_INFO_METADATA_BANDWIDTH_INITIALIZER;
    _TEST_SA(a2, SA_ATR_METADATA_TYPE, SA_METADATA_TYPE_BLK_INFO);
    _TEST_SA(a2, SA_ATR_MEM_REMOTE_SCORE, 0b11);
    _SETTEST_SA(a2, SA_ATR_UTILITY_SCORE, 12);
    _SETTEST_SA(a2, SA_ATR_FUB_DVFS_SCORE, 0b111);

    saBlkCtrlMetadata a3 = SA_BLK_CTRL_METADATA_INITIALIZER_ON;
    _TEST_SA(a3, SA_ATR_METADATA_TYPE, SA_METADATA_TYPE_BLK_CTRL);
    _TEST_SA(a3, SA_ATR_FUB_CTRL, SA_ATR_FUB_CTRL_XE_VALID|SA_ATR_FUB_CTRL_FPU_VALID|SA_ATR_FUB_CTRL_BSM_VALID|SA_ATR_FUB_CTRL_DVFS_VALID);
    _SETTEST_SA(a3, SA_ATR_FUB_CTRL, SA_ATR_FUB_CTRL_XE_VALID);

    saErrInfoMetadata a4 = SA_ERR_INFO_METADATA_INITIALIZER;
    _TEST_SA(a4, SA_ATR_METADATA_TYPE, SA_METADATA_TYPE_ERR_INFO);

    //Call directly
    saEdtInfoMetadata md = SA_EDT_INFO_METADATA_INITIALIZER;
    u16 val;
    u8 ret = saGetMetadata(&md, SA_ATR_METADATA_TYPE, &val);
    val = 0b11;
    saSetMetadata(&md, SA_ATR_MEM_DMA_SCORE, &val);
}
*/
#undef _SETTEST_OCR
#undef _TEST_OCR
#undef _SETTEST_SA
#undef _TEST_SA
