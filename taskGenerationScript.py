#!/usr/bin/python
#============================================================#
# Python 2.7.x
#============================================================#
#General imports.
import sys          #For passed in arguments.
import os
import csv
import random

directory = "input"
task_suffix = ".task";

#============================================================#
def write_shuffled_energy(file, opCountsMap, opFullEnergyMap, opHalfEnergyMap):
#{
    workQueue = [];
    for key, value in opCountsMap.iteritems():
    #{
        entry = [opFullEnergyMap[key],opHalfEnergyMap[key]];
        workQueue.extend([entry]*value);
    #}

    #Randomize the work order.
    random.shuffle(workQueue);

    with open(file, 'wb') as csvfile:
    #{
        writer = csv.writer(csvfile, delimiter='\t');
        writer.writerows(workQueue);
    #}
#}
#============================================================#

#============================================================#
def map_op_energy(file, processSize):
#{
    col = -1;
    map = {};
    with open(file, 'rb') as csvfile:
    #{
        reader = csv.reader(csvfile, delimiter='\t')
        for row in reader:
        #{
            if row[0].rstrip() == 'OP':
            #{
                for i in range(len(row)):
                #{
                    if row[i] == processSize:
                        col = i;
                        break;
                #}

                continue;
            #}

            if col != -1:
            #{
                map[row[0]] = row[col];
            #}
        #}
    #}

    if len(map) == 0:
    #{
        print "Error: process size " + processSize + " not found..."
        exit(0);
    #}
    return map;
#}
#============================================================#

#============================================================#

if (len(sys.argv)) < 6:
#{
    print sys.argv[0] + " <task name> <inst count> <full op header> <ntv op header> <inst>:<percent> ...";
    exit(0);
#}

#Create input directory.
if not os.path.exists("input"):
#{
    os.makedirs("input");
#}

task_name = sys.argv[1];
inst_count = int(sys.argv[2]);
full_header = sys.argv[3];
half_header = sys.argv[4];

print "Task name: " + task_name;
print "Instruction count: " + str(inst_count);
print "Full OP header: " + full_header;
print "Half OP header: " + half_header;

#Read in energy maps.
print "Mapping OP class energy..."
opFullEnergyMap = map_op_energy('instructionNewFormat_3_0_8_PerOpClassEnergyMap_TechScaled.txt', full_header);
opHalfEnergyMap = map_op_energy('instructionNewFormat_3_0_8_PerOpClassEnergyMap_TechScaled.txt', half_header);

print "Instruction mix breakdown:";
opCountsMap = {};
percentage = 0;
for i in range (5, len(sys.argv)):
#{
    tup = sys.argv[i].split(":");

    #Record exact op count to generate.
    opCount = int(tup[1])*inst_count/100;
    opCountsMap[tup[0]] = opCount;

    #Print count for each instruction.
    print "\t " + tup[0] + ": " + str(opCount) + " (" + tup[1] + "%)";

    #check for instruction existence.
    if tup[0] not in opFullEnergyMap or tup[0] not in opHalfEnergyMap:
    #{
        print "Error: specified instruction '" + tup[0] +"' not found in map..."
        exit(0);
    #}

    #Must add to 100%...
    percentage = percentage + int(tup[1]);
#}

if percentage != 100:
#{
    print "Error: instruction mix must add to 100%...";
    exit(0);
#}

print "Generating task..."

file = "{}/{}{}".format(directory, task_name, task_suffix);
write_shuffled_energy(file, opCountsMap, opFullEnergyMap, opHalfEnergyMap);
#============================================================#
