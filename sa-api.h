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


#ifndef _SA_API_GUARD_
#define _SA_API_GUARD_
///FIXME: POWER/CLOCK GATE; Do we need finer grain controls: powergate vs clockgate.
///FIXME: add function to retrieve super CE location.
///FIXME: What if metadata doesn't exist at a given block yet?
/// How should we handle this situation?
///FIXME: how will this work with multiple control engines in the same block.

///****************************************************************************
///****************************************************************************
//+---------------------------------------------------------------------------+
//| file: Self-Awareness API                                                  |
//|                                                                           |
//| Description:                                                              |
//|     This document describes interfaces that enable a system to be         |
//|     Self-Aware in both terms of introspection and adaptation.             |
//|                                                                           |
//|     At a high level, the system will be organized into a hierarchy of     |
//|     control engines. In this scheme each node will have N number of       |
//|     children and a single parent that be communicated directly with using |
//|     mailboxes. Observation and control information will be sent up and    |
//|     down this hierarchy using metadata stored in 64 bit packets.          |
//|                                                                           |
//|     Toward this end, the interfaces been designed to facilitate storing   |
//|     and retrieving this information at a control engine (CE) level, as    |
//|     well as, to abstract away the details for communication.              |
//|                                                                           |
//| Author(s):                                                                |
//|     Aaron Myles Landwehr (*aron@udel.edu*)                                |
//|                                                                           |
//| Date:                                                                     |
//|     February -- March, 2014                                               |
//+---------------------------------------------------------------------------+
/******************************************************************************/
/******************************************************************************/
#include "ss-types.h"
#include <assert.h>

/******************************************************************************/
/* Internal CE Interfaces. */
/******************************************************************************/

// Performance records are used to for book keeping purposes. They contain
// non-aggregated data. In SS these hold relevant PMU information as well
// as any other relevant block specific information.
/*
typedef struct observationRecord
{
    u64 LOCAL_READ_COUNT;
    u64 BLOCK_READ_COUNT;
    u64 REMOTE_READ_COUNT;
    u64 REMOTE_READ_WAS_LOCAL_COUNT;

    u64 LOCAL_WRITE_COUNT;
    u64 BLOCK_WRITE_COUNT;
    u64 REMOTE_WRITE_COUNT;
    u64 REMOTE_WRITE_WAS_LOCAL_COUNT;

    u64 DMA_CONTIGUOUS_COUNT;
    u64 DMA_STRIDED_COUNT;
    u64 DMA_LOCAL_COUNT;
    //reads
    u64 DMA_BLOCK_READ_COUNT;
    u64 DMA_REMOTE_READ_COUNT;
    //writes
    u64 DMA_BLOCK_WRITE_COUNT;
    u64 DMA_REMOTE_WRITE_COUNT;
    //Reads/writes
    u64 DMA_BLOCK_READ_WRITE_COUNT;
    u64 DMA_BLOCK_READ_REMOTE_WRITE_COUNT;
    u64 DMA_REMOTE_READ_BLOCK_WRITE_COUNT;
    u64 DMA_REMOTE_READ_WRITE_COUNT;

    u64 FP_RETIRED_COUNT;
    u64 INT_RETIRED_COUNT;

    u64 CORE_STALL_CYCLES;
    u64 INSTRUCTIONS_EXECUTED;

    //....
} observationRecord;
*/

///****************************************************************************
///****************************************************************************
//+---------------------------------------------------------------------------+
//| Section: Self-Awareness API layer                                         |
//|                                                                           |
//| This section describes the different types of metadata and the interfaces |
//| to manipulate said metadata and to send it to different locations in the  |
//| system. The purposes of the interfaces are to enable control engines to   |
//| send information relevant to introspection to other control engines.      |
//|                                                                           |
//| At a high level there are two types of metadata: observation and control  |
//| metadata. These are further split into logical categories depending on    |
//| what kind of control or information they provide. For example,            |
//| observations pertaining to EDTs are separated from those pertaining to    |
//  directly to blocks in the system.                                         |
//+---------------------------------------------------------------------------+
/******************************************************************************/
/******************************************************************************/

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Enumeration: saMetadataType                                               |
//|                                                                           |
//| Description:                                                              |
//|     metadata types.                                                       |
//|                                                                           |
//| Types:                                                                    |
//|     SA_METADATA_TYPE_EDT_INFO  - For sending EDT specific information.    |
//|     SA_METADATA_TYPE_BLK_INFO  - For sending block information.           |
//|     SA_METADATA_TYPE_AGG_INFO  - For sending aggregated information.      |
//|     SA_METADATA_TYPE_CAP_INFO  - For sending resource capability          |
//|                                  information.                             |
//|     SA_METADATA_TYPE_ERR_INFO  - Error type.                              |
//|     SA_METADATA_TYPE_BLK_CTRL -  For sending control information about    |
//|                                  what FUBs to turn on/off.                |
//|     SA_METADATA_TYPE_INF_CTRL -  For requesting information from a        |
//|                                  engine.                                  |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef enum saMetadataType
{
    SA_METADATA_TYPE_NONE , /*to guarantee valid messages are never 0x0*/
    SA_METADATA_TYPE_EDT_INFO ,
    SA_METADATA_TYPE_BLK_INFO ,
    SA_METADATA_TYPE_AGG_INFO ,
    SA_METADATA_TYPE_CAP_INFO ,
    SA_METADATA_TYPE_ERR_INFO ,
    SA_METADATA_TYPE_BLK_CTRL ,
    SA_METADATA_TYPE_INF_CTRL ,
    SA_METADATA_TYPE_LAST
} saMetadataType;

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Enumeration: saMetadataAttrList                                           |
//|                                                                           |
//| Description:                                                              |
//|     List of all possible metadata attributes that can be set or           |
//|     retrieved. These are to be used with saSetMetadata() and              |
//|     saGetMetadata().                                                      |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures.             |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|     SA_ATR_UTILITY_SCORE        - Utilization score. Specific meaning to  |
//|                                   be defined.                             |
//|     SA_ATR_REQ_METADATA_TYPE    - Attribute to request metadata           |
//|                                   information be sent to the requesting   |
//|                                   CE.                                     |
//|     SA_ATR_OPS_FP               - Whether there are floating point ops.   |
//|     SA_ATR_OPS_INT              - Whether there are integer ops.          |
//|     SA_ATR_MEM_DMA_SCORE        - DMA operation score.                    |
//|     SA_ATR_MEM_LOCAL_SCORE      - Local memory operation score.           |
//|     SA_ATR_MEM_REMOTE_SCORE     - Remote memory operation score.          |
//|     SA_ATR_POW_STATIC           - Static power leakage score.             |
//|     SA_ATR_POW_DYNAMIC_INTERNAL - Internal (intra-block) dynamic power    |
//|                                   score.                                  |
//|     SA_ATR_POW_DYNAMIC_EXTERNAL - External (inter-block) dynamic power    |
//|                                   score.                                  |
//|     SA_ATR_FUB_CTRL             - STUB.                                   |
//|     SA_ATR_FUB_XE_COUNT         - Count of XEs. Exact meaning depends     |
//|                                   on metadatype type.                     |
//|     SA_ATR_FUB_FPU_COUNT        - Count of FPUs. Exact meaning depends    |
//|                                   on the metadata type.                   |
//|     SA_ATR_FUB_BSM_COUNT        - Count of BSMs. Exact meaning depends    |
//|                                   on the metadata type.                   |
//|     SA_ATR_FUB_DVFS_SCORE       - Voltage and frequency scaling score.    |
//|                                   Encodes the possible DVFS states for    |
//|                                   the block.                              |
//|     SA_ATR_FUB_POW_SCORE       -  Power in watts.                         |
//|     SA_ATR_AGG_POW_MEAN        -  Weighted Average power.                 |
//|     SA_ATR_AGG_POW_SD          -  Power standard deviation.               |
//|     SA_ATR_AGG_POW_SKEW        -  Power skew.                             |
//|     SA_ATR_FUB_TEMP_SCORE       - Temperature differential in Celsius.    |
//|     SA_ATR_AGG_TEMP_MEAN        - Weighted Average temperature.           |
//|     SA_ATR_AGG_TEMP_SD          - Temperature standard deviation.         |
//|     SA_ATR_AGG_TEMP_SKEW        - Temperature skew.                       |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef enum saMetadataAttrList
{
    //Header attributes:
    SA_ATR_METADATA_TYPE,
    SA_ATR_URGENCY,
    //Metadata attributes:
    SA_ATR_UTILITY_SCORE        ,
    SA_ATR_REQ_METADATA_TYPE    ,
    SA_ATR_OPS_FP               ,
    SA_ATR_OPS_INT              ,
    SA_ATR_MEM_DMA_SCORE        ,
    SA_ATR_MEM_LOCAL_SCORE      ,
    SA_ATR_MEM_REMOTE_SCORE     ,
    SA_ATR_POW_STATIC           ,
    SA_ATR_POW_DYNAMIC_INTERNAL ,
    SA_ATR_POW_DYNAMIC_EXTERNAL ,
    SA_ATR_FUB_CTRL             ,
    SA_ATR_FUB_XE_COUNT         ,
    SA_ATR_FUB_FPU_COUNT        ,
    SA_ATR_FUB_BSM_COUNT        ,
    SA_ATR_FUB_DVFS_SCORE       ,
    SA_ATR_FUB_POWER_GOAL       ,
    SA_ATR_FUB_POW_SCORE        ,
    SA_ATR_FUB_UNDER_CONTROL    ,
    SA_ATR_AGG_POW_MEAN         ,
    SA_ATR_AGG_POW_SD           ,
    SA_ATR_AGG_POW_SKEW         ,
    SA_ATR_FUB_TEMP_SCORE       ,
    SA_ATR_AGG_TEMP_MEAN        ,
    SA_ATR_AGG_TEMP_SD          ,
    SA_ATR_AGG_TEMP_SKEW        ,
    //
    SA_ATR_LAST
} saMetadataAttrList;

typedef u16 saMetadataAttr;

//The header.
#define SA_HEADER \
    u16 SA_ATR_METADATA_TYPE : 3; \
    u16 SA_ATR_URGENCY       : 1;

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saMetadata                                                     |
//|                                                                           |
//| Description:                                                              |
//|     Generic metadata structure holding only header information.           |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures. _3 bits_    |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|                                   _1 bit_.                                |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saMetadata
{
    SA_HEADER;
    u64 SA_ATR_RESERVED             : 60;
} saMetadata;

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saEdtInfoMetadata                                              |
//|                                                                           |
//| Description:                                                              |
//|     Metadata structure for storing observed EDT specific information.     |
//|     Subject to change without notice. The fields are meant to give a      |
//|     self-aware system feedback about the effects of an EDT on the system. |
//|     This can be as simple as whether the EDT requires FP operations to    |
//|     something as complex as the power accumulation relative to the block. |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures. _3 bits_    |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|                                   _1 bit_.                                |
//|     SA_ATR_UTILITY_SCORE        - Utilization score. Specific meaning to  |
//|                                   be defined. _7 bits_.                   |
//|     SA_ATR_OPS_FP               - Whether there are floating point ops.   |
//|                                   _1 bit_.                                |
//|     SA_ATR_OPS_INT              - Whether there are integer ops. _1 bit_. |
//|     SA_ATR_MEM_DMA_SCORE        - DMA operation score. ranging from 0 to  |
//|                                   2. _2 bits_.                            |
//|     SA_ATR_MEM_LOCAL_SCORE      - Local memory operation score. ranging   |
//|                                   from 0 to 2. _2 bits_.                  |
//|     SA_ATR_MEM_REMOTE_SCORE     - Remote memory operation score. ranging  |
//|                                   from 0 to 2. _2 bits_.                  |
//|     SA_ATR_POW_STATIC           - Static power leakage score in ranging   |
//|                                   from 0 to 15. _4 bits_.                 |
//|     SA_ATR_POW_DYNAMIC_INTERNAL - Internal (intra-block) dynamic power    |
//|                                   score ranging from 0 to 15.             |
//|                                   This field is for power generated       |
//|                                   within the block. _4 bits_.             |
//|     SA_ATR_POW_DYNAMIC_EXTERNAL - External (inter-block) dynamic power    |
//|                                   score ranging from 0 to 15.             |
//|                                   This field is for rating power          |
//|                                   generated outside of the block.         |
//|                                   _4 bits_.                               |
//|     SA_ATR_RESERVED             - Reserved bits. Pads the structure to 64 |
//|                                   bits.                                   |
//|                                                                           |
//| Metadata Initializer Macros:                                              |
//|     SA_EDT_METADATA_INITIALIZER           - Default initializer.          |
//|     SA_EDT_METADATA_BANDWIDTH_INITIALIZER - Bandwidth intensive           |
//|                                             initializer.                  |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saEdtInfoMetadata
{
    SA_HEADER;
    u64 SA_ATR_UTILITY_SCORE        : 7;
    u64 SA_ATR_OPS_FP               : 1;
    u64 SA_ATR_OPS_INT              : 1;
    u64 SA_ATR_MEM_DMA_SCORE        : 2;
    u64 SA_ATR_MEM_LOCAL_SCORE      : 2;
    u64 SA_ATR_MEM_REMOTE_SCORE     : 2;
    u64 SA_ATR_POW_STATIC           : 4;
    u64 SA_ATR_POW_DYNAMIC_INTERNAL : 4;
    u64 SA_ATR_POW_DYNAMIC_EXTERNAL : 4;
    u64 SA_ATR_RESERVED             : 33;
} saEdtInfoMetadata;

// Initializers:
#define SA_EDT_INFO_METADATA_INITIALIZER {SA_METADATA_TYPE_EDT_INFO}
#define SA_EDT_INFO_METADATA_BANDWIDTH_INITIALIZER {SA_METADATA_TYPE_EDT_INFO,0,0,0,0,0,0,0b11}

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saBlkInfoMetadata                                              |
//|                                                                           |
//| Description:                                                              |
//|     Metadata structure for storing observed block specific information.   |
//|     Subject to change without notice.                                     |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures. _3 bits_    |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|                                   _1 bit_.                                |
//|     SA_ATR_UTILITY_SCORE        - Utilization score. Specific meaning to  |
//|                                   be defined. _7 bits_.                   |
//|     SA_ATR_MEM_DMA_SCORE        - DMA operation score. ranging from 0 to  |
//|                                   2. _2 bits_.                            |
//|     SA_ATR_MEM_LOCAL_SCORE      - Local memory operation score. ranging   |
//|                                   from 0 to 2. _2 bits_.                  |
//|     SA_ATR_MEM_REMOTE_SCORE     - Remote memory operation score. ranging  |
//|                                   from 0 to 2. _2 bits_.                  |
//|     SA_ATR_FUB_XE_COUNT         - Count of XEs currently on within the    |
//|                                   block. _3 bits_.                        |
//|     SA_ATR_FUB_FPU_COUNT        - Count of FPUs currently on within the   |
//|                                   block. _3 bits_.                        |
//|     SA_ATR_FUB_BSM_COUNT        - Count of BSMs currently on within the   |
//|                                   block. _2 bits_.                        |
//|     SA_ATR_FUB_DVFS_SCORE       - Voltage and frequency scaling score.    |
//|                                   Encodes the possible DVFS states for    |
//|                                   the block. _3 bits_.                    |
//|     SA_ATR_FUB_POW_SCORE        - Power in watts.                         |
//|                                   _8 bits_.                               |
//|     SA_ATR_FUB_TEMP_SCORE       - Temperature differential in Celsius.    |
//|                                   _6 bits_.                               |
//|     SA_ATR_RESERVED             - Reserved bits. Pads the structure to 64 |
//|                                   bits.                                   |
//|                                                                           |
//| Metadata Initializer Macros:                                              |
//|     SA_BLK_INFO_METADATA_INITIALIZER           - Default initializer.     |
//|     SA_BLK_INFO_METADATA_BANDWIDTH_INITIALIZER - Bandwidth intensive      |
//|                                             initializer.                  |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saBlkInfoMetadata
{
    SA_HEADER;
    u64 SA_ATR_UTILITY_SCORE        : 7;
    u64 SA_ATR_MEM_DMA_SCORE        : 2;
    u64 SA_ATR_MEM_LOCAL_SCORE      : 2;
    u64 SA_ATR_MEM_REMOTE_SCORE     : 2;
    u64 SA_ATR_FUB_XE_COUNT         : 3;
    u64 SA_ATR_FUB_FPU_COUNT        : 3;
    u64 SA_ATR_FUB_BSM_COUNT        : 2;
    u64 SA_ATR_FUB_DVFS_SCORE       : 3;
    u64 SA_ATR_FUB_POW_SCORE        : 16;
    u64 SA_ATR_FUB_TEMP_SCORE       : 6;
    u64 SA_ATR_RESERVED             : 14;
} saBlkInfoMetadata;

// Initializers:
#define SA_BLK_INFO_METADATA_INITIALIZER {SA_METADATA_TYPE_BLK_INFO}
#define SA_BLK_INFO_METADATA_BANDWIDTH_INITIALIZER {SA_METADATA_TYPE_BLK_INFO,0,0,0,0,0b11}

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saAggInfoMetadata                                              |
//|                                                                           |
//| Description:                                                              |
//|     Metadata structure for storing aggregated information up the control  |
//|     hierarchy.                                                            |
//|     Subject to change without notice.                                     |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures. _3 bits_    |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|                                   _1 bit_.                                |
//|     SA_ATR_AGG_POW_SCORE        - Weighted average power.                 |
//|                                   _12 bits_.                              |
//|     SA_ATR_AGG_POW_SD           - Power standard deviation                |
//|                                   _12 bits_.                              |
//|     SA_ATR_AGG_POW_SKEW         - Power skew.                             |
//|                                   _12 bits_.                              |
//|     SA_ATR_AGG_TEMP_MEAN        - Weighted average temperature.           |
//|                                   _8 bits_.                               |
//|     SA_ATR_AGG_TEMP_SD          - Temperature standard deviation          |
//|                                   _8 bits_.                               |
//|     SA_ATR_AGG_TEMP_SKEW        - Temperature skew.                       |
//|                                   _8 bits_.                               |
//|     SA_ATR_RESERVED             - Reserved bits. Pads the structure to 64 |
//|                                   bits.                                   |
//|                                                                           |
//| Metadata Initializer Macros:                                              |
//|     SA_AGG_INFO_METADATA_INITIALIZER           - Default initializer.     |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saAggInfoMetadata
{
    SA_HEADER;
    u64 SA_ATR_AGG_POW_MEAN         : 16;
    u64 SA_ATR_AGG_POW_SD           : 12;
    u64 SA_ATR_AGG_POW_SKEW         : 8;
    u64 SA_ATR_AGG_TEMP_MEAN        : 8;
    u64 SA_ATR_AGG_TEMP_SD          : 8;
    u64 SA_ATR_AGG_TEMP_SKEW        : 8;
} saAggInfoMetadata;

// Initializers:
#define SA_AGG_INFO_METADATA_INITIALIZER {SA_METADATA_TYPE_AGG_INFO}

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saCapInfoMetadata                                              |
//|                                                                           |
//| Description:                                                              |
//|     Metadata structure for storing the capabilities of the resources the  |
//|     given CE oversees. At the the lowest level this entails direct block  |
//|     information. At higher levels this entails aggregated                 |
//|     information about capabilities.                                       |
//|     Subject to change without notice.                                     |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures. _3 bits_    |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|                                   _1 bit_.                                |
//|     SA_ATR_FUB_XE_COUNT         - Count of XEs available underneath this  |
//|                                    CE. _12 bits_.                         |
//|     SA_ATR_FUB_FPU_COUNT        - Count of FPUs available underneath this |
//|                                    CE. _12 bits_.                         |
//|     SA_ATR_FUB_BSM_COUNT        - Count of BSMs available underneath this |
//|                                    CE. _12 bits_.                         |
//|     SA_ATR_RESERVED             - Reserved bits. Pads the structure to 64 |
//|                                   bits.                                   |
//|                                                                           |
//| Metadata Initializer Macros:                                              |
//|     SA_CAP_INFO_METADATA_INITIALIZER - Default initializer.               |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saCapInfoMetadata
{
    SA_HEADER;
    u64 SA_ATR_FUB_XE_COUNT         : 12;
    u64 SA_ATR_FUB_FPU_COUNT        : 12;
    u64 SA_ATR_FUB_BSM_COUNT        : 12;
    u64 SA_ATR_RESERVED             : 24;
} saCapInfoMetadata;

#define SA_CAP_INFO_METADATA_INITIALIZER {SA_METADATA_TYPE_CAP_INFO}


///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saErrInfoMetadata                                              |
//|                                                                           |
//| Description:                                                              |
//|     Metadata structure for storing error related information.             |
//|     Subject to change without notice.                                     |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_RESERVED             - Reserved bits. Pads the structure to 64 |
//|                                   bits.                                   |
//|                                                                           |
//| Metadata Initializer Macros:                                              |
//|     SA_ERR_INFO_METADATA_INITIALIZER - Default initializer.               |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saErrInfoMetadata
{
    SA_HEADER;
    u64 SA_ATR_RESERVED : 60;

} saErrInfoMetadata;

#define SA_ERR_INFO_METADATA_INITIALIZER {SA_METADATA_TYPE_ERR_INFO}

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saBlkCtrlMetadata                                              |
//|                                                                           |
//| Description:                                                              |
//|     Metadata structure for storing Block FUB control information.         |
//|     Subject to change without notice.                                     |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures. _3 bits_    |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|                                   _1 bit_.                                |
//|     SA_ATR_FUB_CTRL             - STUB. _4 bits_.                         |
//|     SA_ATR_FUB_XE_COUNT         - Count of XEs to keep on within the      |
//|                                   block. _3 bits_.                        |
//|     SA_ATR_FUB_FPU_COUNT        - Count of FPUs to keep on within the     |
//|                                   block. _3 bits_.                        |
//|     SA_ATR_FUB_BSM_COUNT        - Count of BSMs to keep on within the     |
//|                                   block. _2 bits_.                        |
//|     SA_ATR_FUB_DVFS_SCORE       - Voltage and frequency scaling score.    |
//|                                   Encodes the possible DVFS states for    |
//|                                   the block. _3 bits_.                    |
//|     SA_ATR_RESERVED             - Reserved bits. Pads the structure to 64 |
//|                                   bits.                                   |
//|                                                                           |
//| Metadata Initializer Macros:                                              |
//|     SA_BLK_CTRL_METADATA_INITIALIZER - Default initializer.               |
//|     SA_BLK_CTRL_METADATA_INITIALIZER_ON - All FUBs on initializer.        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saBlkCtrlMetadata
{
    SA_HEADER;
    u64 SA_ATR_FUB_CTRL         : 6; //Each bit says which metadata fields are valid.
    u64 SA_ATR_FUB_XE_COUNT     : 3;
    u64 SA_ATR_FUB_FPU_COUNT    : 3;
    u64 SA_ATR_FUB_BSM_COUNT    : 2; //memory banks on for the BSM.
    u64 SA_ATR_FUB_DVFS_SCORE   : 2; //current voltage and frequency scaling for the block.
    u64 SA_ATR_FUB_POWER_GOAL   : 16; //For setting up the power goal of a lower level.
    u64 SA_ATR_FUB_UNDER_CONTROL: 1;
    u64 SA_ATR_RESERVED         : 27;

} saBlkCtrlMetadata;

//FUB control bitmasks.
#define SA_ATR_FUB_CTRL_XE_VALID 0b100000
#define SA_ATR_FUB_CTRL_FPU_VALID 0b010000
#define SA_ATR_FUB_CTRL_BSM_VALID 0b001000
#define SA_ATR_FUB_CTRL_DVFS_VALID 0b000100
#define SA_ATR_FUB_CTRL_POWER_GOAL_VALID 0b000010
#define SA_ATR_FUB_CTRL_UNDER_CONTROL_VALID 0b000001

#define SA_ATR_FUB_DVFS_SCORE_FULL 0b11
#define SA_ATR_FUB_DVFS_SCORE_HALF 0b01
#define SA_ATR_FUB_DVFS_SCORE_NONE 0b00

#define SA_BLK_CTRL_METADATA_INITIALIZER {SA_METADATA_TYPE_BLK_CTRL,0b0,\
					  SA_ATR_FUB_CTRL_XE_VALID|\
					  SA_ATR_FUB_CTRL_FPU_VALID|\
					  SA_ATR_FUB_CTRL_BSM_VALID|\
					  SA_ATR_FUB_CTRL_DVFS_VALID|\
					  SA_ATR_FUB_CTRL_POWER_GOAL_VALID|\
					  SA_ATR_FUB_CTRL_UNDER_CONTROL_VALID,\
					  0b111,\
					  0b111,\
					  0b11,\
					  SA_ATR_FUB_DVFS_SCORE_FULL,\
                                          0,\
                                          0b0,\
                                          0}
#define SA_BLK_CTRL_METADATA_INITIALIZER_ON SA_BLK_CTRL_METADATA_INITIALIZER

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saInfCtrlMetadata                                              |
//|                                                                           |
//| Description:                                                              |
//|     Metadata structure for directing a control engine to send specific    |
//|     observation information to this control engine. This information is   |
//|     specified using the metadata type ids.                                |
//|     Subject to change without notice.                                     |
//|                                                                           |
//| Attributes:                                                               |
//|     SA_ATR_METADATA_TYPE        - This field specifies the metadata type. |
//|                                   This is header information and common   |
//|                                   to all metadata structures. _3 bits_    |
//|     SA_ATR_URGENCY              - Marks whether this metadata needs to be |
//|                                   urgently handled by a control engine.   |
//|                                   _1 bit_.                                |
//|     SA_ATR_REQ_METADATA_TYPE    - Attribute to request metadata           |
//|                                   information be sent to the requesting   |
//|                                   CE. _10 bits_.                          |
//|     SA_ATR_RESERVED             - Reserved bits. Pads the structure to 64 |
//|                                   bits.                                   |
//|                                                                           |
//| Metadata Initializer Macros:                                              |
//|     SA_INF_CTRL_METADATA_INITIALIZER - Default initializer.               |
//+---------------------------------------------------------------------------+
/******************************************************************************/
typedef struct __attribute__ ((__packed__)) saInfCtrlMetadata
{
    SA_HEADER;
    u64 SA_ATR_REQ_METADATA_TYPE : 10;
    u64 SA_ATR_RESERVED          : 50;

} saInfCtrlMetadata;

#define SA_INF_CTRL_METADATA_INITIALIZER {SA_METADATA_TYPE_INF_CTRL}

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Structure: saLocation                                                     |
//|                                                                           |
//| Description:                                                              |
//|     This structure packs where to recv/send messages.                     |
//|                                                                           |
//| Control Bits:                                                             |
//|     control    - direct where the message is sent. _5 bits_.              |
//|     rack_num   - Rack to send to. _8 bits_.                               |
//|     unit_num   - Unit to send to. _7 bits_.                               |
//|     board_num  - Board to send to. _5 bits_.                              |
//|     chip_num   - Chip to send to. _4 bits_.                               |
//|     block_num  - Block to send to. _5 bits_.                              |
//|     reserved   - Reserved bits. Pads the structure to 64 bits.            |
//+---------------------------------------------------------------------------+
/******************************************************************************/
///FIXME: Change documentation above.
typedef struct __attribute__ ((__packed__)) saLocation
{
   u8 parent:1; //Whether this is a parent location.
   u64 offset;
} saLocation;

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: saGetParentLocation                                             |
//|     Retrieves this CE's parent mailbox location.                          |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called when the parent's mailbox needs to be determined.        |
//|                                                                           |
//| Returns:                                                                  |
//|     the encoded location of the parent mailbox.                           |
//+---------------------------------------------------------------------------+
/******************************************************************************/
saLocation saGetParentLocation();

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: saGetChildLocation                                              |
//|     Retrieves the given child mailbox location for this CE.               |
//|                                                                           |
//| Parameters:                                                               |
//|     num - the child to retrieve the location of.                          |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called when a child's mailbox needs to be determined.           |
//|                                                                           |
//| Returns:                                                                  |
//|     the encoded location of the specified child's mailbox.                |
//+---------------------------------------------------------------------------+
/******************************************************************************/
saLocation saGetChildLocation(u32 num);

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: saSetMetadata                                                   |
//|     Sets the metadata attribute to the corresponding value.               |
//|                                                                           |
//| Parameters:                                                               |
//|     metadata - Modified: metadata structure to modify.                    |
//|     attr     - attribute to set.                                          |
//|     val      - value to set the attribute to.                             |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called any time a metadata attribute needs to be set.           |
//|                                                                           |
//| Returns:                                                                  |
//|     0 on success and and an error code on failure.                        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
u8 saSetMetadata(void* metadata, saMetadataAttr attr, void* val);

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: saGetMetadata                                                   |
//|     Returns the metadata attribute's current value.                       |
//|                                                                           |
//| Parameters:                                                               |
//|     metadata - metadata structure.                                        |
//|     attr     - attribute to return the value for.                         |
//|     val      - Return value: current value of the attribute.              |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called anytime a metadata attribute needs to be looked at.      |
//|                                                                           |
//| Returns:                                                                  |
//|     0 on success and and an error code on failure.                        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
u8 saGetMetadata(void* metadata, saMetadataAttr attr, void* val);

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: saSendMetadata                                                  |
//|                                                                           |
//| Description:                                                              |
//|     Sends the specified metadata to the inbox at the                      |
//|     corresponding destination.                                            |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called to send metadata to a specific CE.                       |
//|     To be called any time metadata needs to be sent to a CE.              |
//|                                                                           |
//| Parameters:                                                               |
//|     location - the system destination.                                    |
//|     metadata - metadata to send.                                          |
//|                                                                           |
//| Returns:                                                                  |
//|     0 on success and and an error code on failure.                        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
u8 saSendMetadata(saLocation location, void* metadata);

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: saRecvMetadata                                                  |
//|                                                                           |
//| Description:                                                              |
//|     Retrieves the specified metadata from the outbox                      |
//|     at the corresponding destination.                                     |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called to check for metadata that needs to be handled.          |
//|     To be called on a time interval.                                      |
//|                                                                           |
//| Parameters:                                                               |
//|     location - the system destination.                                    |
//|     metadata - allocated memory to store metadata in.                     |
//|                                                                           |
//| Returns:                                                                  |
//|     0 on success and and an error code on failure.                        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
u8 saRecvMetadata(saLocation location, void* metadata);

///****************************************************************************
///****************************************************************************
//+---------------------------------------------------------------------------+
//| Section: OCR API layer                                                    |
//|                                                                           |
//| Within the OCR API, self-awareness (SA) is enabled by exposing and        |
//| associating metadata with EDT templates. In the context of SA, metadata   |
//| should be collected and stored at the block level for EDTs and migrated   |
//| up/down the control hierarchy when applicable.                            |
//+---------------------------------------------------------------------------+
/******************************************************************************/
/******************************************************************************/

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: ocrInitMetadata                                                 |
//|                                                                           |
//| Description:                                                              |
//|     Internally associates EDT metadata with the given OCR GUID within a   |
//|     given architectural block. Must be called before any setter/getters.  |
//|                                                                           |
//| Parameters:                                                               |
//|     guid - EDT template GUID.                                             |
//|     metadata - full bit field to initialize the EDT metadata.             |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called the first time an EDT is seen by the CE.                 |
//|                                                                           |
//| Returns:                                                                  |
//|     0 on success and and an error code on failure.                        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
u8 ocrInitMetadata(ocrGuid_t* guid, saEdtInfoMetadata* metadata);

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: ocrEdtSetMetadata                                               |
//|                                                                           |
//| Description:                                                              |
//|     Modifies the value of the given EDT metadata attribute that is        |
//|     associated with the given OCR GUID within a given architectural       |
//|     block. This will return an error if the metadata has not been         |
//|     previously initialized or if the specified attribute doesn't exist.   |
//|     if the stored number in val is larger than the corresponding          |
//|     attribute, it will silently clip the upper bits.                      |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called when local EDT metadata needs to be updated. This will   |
//|     occur the first time a CE sees a new EDT type to set defaults and     |
//|     after an EDT finishes running to reflect system observations.         |
//|                                                                           |
//| Parameters:                                                               |
//|     guid - EDT template GUID.                                             |
//|     attr - attribute to set.                                              |
//|     val  - value to set the attribute to.                                 |
//|                                                                           |
//| Returns:                                                                  |
//|     0 on success and and an error code on failure.                        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
u8 ocrEdtSetMetadata(ocrGuid_t* guid, saMetadataAttr attr, u16* val);

///****************************************************************************
//+---------------------------------------------------------------------------+
//| Function: ocrEdtGetMetadata                                               |
//|                                                                           |
//| Description:                                                              |
//|     Returns the current value of the given EDT metadata attribute that is |
//|     associated with the given OCR GUID within a given architectural       |
//|     block. This will return an error if the metadata has not been         |
//|     previously initialized or if the specified attribute doesn't exist.   |
//|                                                                           |
//| Call Information:                                                         |
//|     To be called any time metadata information about an EDT needs to be   |
//|     retrieved. This can occur if an EDT finishes, a request is made for   |
//|     the EDT information from a parent CE, or a time interval as passed.   |
//|                                                                           |
//| Parameters:                                                               |
//|     guid - EDT template GUID.                                             |
//|     attr - attribute to return the value for.                             |
//|     val  - Return value: current value of the attribute.                  |
//|                                                                           |
//| Returns:                                                                  |
//|     0 on success and and an error code on failure.                        |
//+---------------------------------------------------------------------------+
/******************************************************************************/
u8 ocrEdtGetMetadata(ocrGuid_t* guid, saMetadataAttr attr, u16* val);
#endif
