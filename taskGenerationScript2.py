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
def write_shuffled_energy(file, opCountsMap):
#{
    workQueue = [];
    for key, value in opCountsMap.iteritems():
    #{
        entry = [key,"stub"];
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

if (len(sys.argv)) < 4:
#{
    print sys.argv[0] + " <task name> <inst count> <inst>:<percent> ...";
    exit(0);
#}

#Create input directory.
if not os.path.exists("input"):
#{
    os.makedirs("input");
#}

task_name = sys.argv[1];
inst_count = int(sys.argv[2]);

print "Task name: " + task_name;
print "Instruction count: " + str(inst_count);

opCountsMap = {};
percentage = 0;
for i in range (3, len(sys.argv)):
#{
    tup = sys.argv[i].split(":");

    #Record exact op count to generate.
    opCount = int(tup[1])*inst_count/100;
    opCountsMap[tup[0]] = opCount;

    #Print count for each instruction.
    print "\t " + tup[0] + ": " + str(opCount) + " (" + tup[1] + "%)";

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
write_shuffled_energy(file, opCountsMap);
