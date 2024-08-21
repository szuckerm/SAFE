#!/usr/bin/python
#============================================================#
# Python 2.7.x
#============================================================#
#General imports.
import sys          #For passed in arguments.
import os
import csv
import random

directory = "input";
task_suffix = ".task";
queue_suffix = ".queue";

#============================================================#
def write_shuffled_tasks(file, taskCountsMap):
#{
    workQueue = [];
    for key, value in taskCountsMap.iteritems():
    #{
        entry = [key, "stub"];
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

if (len(sys.argv)) < 4:
#{
    print sys.argv[0] + " <queue name> <task count> <task>:<percent> ...";
    exit(0);
#}

#Create input directory.
if not os.path.exists("input"):
#{
    os.makedirs("input");
#}

queue_name = sys.argv[1];
task_count = int(sys.argv[2]);

print "Queue name: " + queue_name;
print "Task count: " + str(task_count);

print "Task mix breakdown:";
taskCountsMap = {};
percentage = 0;
for i in range (3, len(sys.argv)):
#{
    tup = sys.argv[i].split(":");

    #Record exact task count to generate.
    taskCount = int(tup[1])*task_count/100;
    taskCountsMap[tup[0]] = taskCount;

    #Print count for each task.
    print "\t " + tup[0] + ": " + str(taskCount) + " (" + tup[1] + "%)";

    #check for task existence.
    if os.path.isfile(directory + "/" + tup[0] + task_suffix) == False:
    #{
        print "Error: specified task '" + tup[0] +"' not found in directory '" + directory + "'..."
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

print "Generating queue..."
file = "{}/{}{}".format(directory, queue_name, queue_suffix);
write_shuffled_tasks(file, taskCountsMap);
#============================================================#
