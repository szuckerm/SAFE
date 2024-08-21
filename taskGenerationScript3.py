import random

# Number of instructions
NUM_INSTS = 10000

# Inst percentages and names
FAR_MEM_OPS = [26,  'dram', 'dram-bulk']
MID_MEM_OPS = [12,  'bsm-bulk', 'bsm']
CLS_MEM_OPS = [12,  'spad-bulk', 'spad', 'lmem-write', 'lmem-read']
CACHE_OPS =   [12,  'l4cache-data-bulk', 'l4cache-data', 
                   'l3cache-data-bulk', 'l3cache-data',
                   'l2cache-data-bulk', 'l2cache-data',
                   'l1cache-data-bulk', 'l1cache-data'
              ]
ADV_FP_OPS  = [10,  'sincos_fp', 'div_fp', 'sqrt_fp', 'fma_fp', 'arith-adv_fp']
SIM_FP_OPS  = [16,  'arith-simple_fp']
ADV_INT_OPS = [6, 'div_int', 'fma_int', 'arith-adv_int']
SIM_INT_OPS = [6, 'arith-simple_int', 'bitops']
ACC_OPS =     [0,  'rmem', 'rmem_sp', 'lmem', 'lmem_sp', 'cache', 'cache_sp']

#===============================================================================
#===============================================================================
#===============================================================================

TOTAL_PERCENTAGE = FAR_MEM_OPS[0] + MID_MEM_OPS[0] + CLS_MEM_OPS[0] + CACHE_OPS[0] + ADV_FP_OPS[0] + SIM_FP_OPS[0] +ADV_INT_OPS[0] + SIM_INT_OPS[0] + ACC_OPS[0]

ALL_OPS = [FAR_MEM_OPS, MID_MEM_OPS, CLS_MEM_OPS, CACHE_OPS, ADV_FP_OPS, SIM_FP_OPS, ADV_INT_OPS, SIM_INT_OPS, ACC_OPS]

if (TOTAL_PERCENTAGE != 100):
    print "Percentage should add to 100 but adds to " + str(TOTAL_PERCENTAGE) + "..."
    exit(0)

current_id = 0
inst_name_map = dict()
inst_list = list()

for inst_type in ALL_OPS:
    initial_id = current_id
    # assign ids to a map of ID --> inst name
    for i in range(1, len(inst_type)):
        inst_name_map[current_id] = inst_type[i]
        current_id = current_id + 1
    # Generate list of instruction IDs from inst_count.
    inst_count = int(NUM_INSTS * ((inst_type[0])/100.0))
    inst_list = inst_list + ([random.randrange(initial_id, current_id) for _ in range(0,inst_count)])

# Randomize list of instructions...
random.shuffle(inst_list)

for id in inst_list:
    print inst_name_map[id] + '\tstub'
