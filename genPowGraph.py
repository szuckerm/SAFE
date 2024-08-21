#!/usr/bin/python
#============================================================#
# Python 2.7.x
# Run as follows: python read_temperatures.py xe-matrix-vector.*
#============================================================#
#General imports.
from __future__ import print_function
import collections; #for OrderedDicts.
import datetime;    #For recording script speed data.
import re           #Regex for parsing files.
import os           #For movie writer stall workaround.
import sys          #For passed in arguments.
import pylab

#============================================================#
#Delimiter Characters 
prefix_temperatures  = "\[RMD_TRACE_POWER\]" #prefix delimiter.
#============================================================#

#Dictionary to temperatures where key='cycle' and value='temperature matrix'.
data = collections.OrderedDict();

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

# Globals
blk_width  = 0;
blk_height = 0;
unt_width  = 0;
unt_height = 0;


def get_hms(seconds):
    minutes = seconds/60;
    seconds -= minutes*60;
    hours = minutes/60;
    minutes -= hours*60;
    return [hours, minutes, seconds];

#============================================================#

def parse_temperatures(logfile):
#{
    global blk_width, blk_height, unt_width, unt_height;

    eprint( "Parsing file " + logfile  + "...")
    #Figure out layout id.
    tuple = re.findall(r'brd\d+.chp\d+.unt\d+.blk\d+.*', logfile);
    if(len(tuple) == 0):
        return;
    tuple = re.findall(r'\d+', tuple[0]);
    
    brd = int(tuple[0]); chp = int(tuple[1]);
    unt = int(tuple[2]); blk = int(tuple[3]);

    #compute row and column numbers for the given id.
    col = blk%blk_width + unt%unt_width*blk_width;
    row = blk/blk_width + unt/unt_width*blk_width;

    datafile = file(logfile);
    for line in datafile:                                           #iterate each line of file.
    #{
        for string in re.findall(prefix_temperatures+" Power (.*)", line): #iterate each encapsulated string in line.
            tuple = re.findall(r'\d+[.]*\d*', string);
            temperature = float(tuple[0]); cycle = int(float(tuple[1]));
            if(row >= blk_height*unt_height or col >= blk_width*unt_width):
                continue;
            if cycle in data:
                data[cycle][row, col] = temperature;
            else:
                data[cycle] = pylab.np.empty((blk_height*unt_height, blk_width*unt_width));
                data[cycle].fill(0);
                data[cycle][row, col] = temperature;
    #}
#}

#============================================================#

def parse_dimensions(logfile):
#{
    global blk_width, blk_height, unt_width, unt_height;

    #Figure out layout id.
    tuple = re.findall(r'brd\d+.chp\d+.unt\d+.blk\d+.*', logfile);

    if(len(tuple) == 0):
        return;

    datafile = file(logfile);
    for line in datafile:                                         #iterate each line of file.
    #{
        for line in re.findall(prefix_temperatures+" Simulated (.*)", line): #iterate each encapsulated string in line.
            tuple = re.findall(r'.\d+', line);
            if "block layout" in line:
                blk_width = int(tuple[0]);
                blk_height = int(tuple[1]);
            elif "unit layout" in line:
                unt_width = int(tuple[0]);
                unt_height = int(tuple[1]);
    #}
#}

CLOCK_SPEED = 4200.0*1e3

def avg_temps():
#{
    print ("Power Over Time")
    print ( "time (ms) , Power (W)")
    for cycle in data:
    #{
        accum = 0.0
        for row in range(0, blk_width*unt_width):
            for col in range(0, blk_height*unt_height):
                accum = accum + data[cycle][row,col]
        print ( cycle/CLOCK_SPEED, ",", accum)
    #}
#}

#============================================================#

#def animation_update(data, ax, last_cycle, stdout):
##{
#    #Set data.
#    ax.set_title(title%data[0]);
#
#    ax.collections = [];
#    ret = ax.pcolormesh(data[1], vmin=min_temperature, vmax=max_temperature, edgecolors='black', linewidth=1.5, cmap=mycmap);
#    
#    #Get estimate time to finish.
#    time_elapsed = datetime.datetime.now() - begin_time;
#    percent_complete = (data[0]/last_cycle);
#    seconds_elapsed = int(time_elapsed.total_seconds());
#    t1 = get_hms(seconds_elapsed); #elapsed time.
#    if (percent_complete>0.005):
#    #{
#        t2 = get_hms(int(seconds_elapsed * (1/percent_complete-1))); #ETA.
#        stdout.write("\rPercent done {:.2f}% (Elapsed Time: {:02}:{:02}:{:02}) (ETA: {:02}:{:02}:{:02})"
#                         .format(percent_complete*100, t1[0], t1[1], t1[2], t2[0], t2[1], t2[2]));
#        stdout.flush();
#    #}
#    else:
#    #{
#        stdout.write("\rPercent done {:.2f}% (Elapsed Time: {:02}:{:02}:{:02}) (ETA: ...)"
#                         .format(percent_complete*100, t1[0], t1[1], t1[2]));
#        stdout.flush();
#    #}
#
#    return ret;
##}

#============================================================#
#def create_graph():
##{
#    global blk_width, blk_height, unt_width, unt_height;
#
#    #Create figure.
#    fig = matplotlib.pyplot.figure();
#
#    #Figure out max length and specify file name format.
#    digit_length = len(str(max(data.iterkeys())));
#    format = "block-heat-%%0%dd.png" %(digit_length);
#
#    # Using contourf to provide my colorbar info, then clearing the figure
#    Z = [[0,0],[0,0]];
#    levels = range(min_temperature,max_temperature+step,step);
#    color_bar = matplotlib.pyplot.contourf(Z, levels, cmap=mycmap);
#    matplotlib.pyplot.clf();
#
#    #Add temperature range scale.
#    fig.subplots_adjust(right=0.8);
#    colorbar_ax = fig.add_axes([0.85, 0.15, 0.05, 0.7]);
#    fig.colorbar(color_bar, cax=colorbar_ax).set_label(temperature_units);
#
#    #Create axes and set it up.
#    ##Need to specify limits below or some installations of matplotlib won't create the mesh correctly.
#    ax = fig.add_subplot(111, xlim=[0,blk_width*unt_width],ylim=[0,blk_height*unt_height]);
#
#    #Order the blocks so that the first is on top-left.
#    ax.invert_yaxis();
#    ax.xaxis.tick_top();
#
#    #Disable Tick text.
#    ax.set_xticklabels([], minor=False);
#    ax.set_yticklabels([], minor=False);
#
#    # Workaround for non-'_file' encoders which stall because stderr stdout PIPE buffers filling...
#    stdout = sys.stdout;
#    matplotlib.verbose.level = "debug";
#    sys.stdout = matplotlib.verbose.fileo = open(os.devnull, 'w');
#
#    anim = matplotlib.animation.FuncAnimation(fig, animation_update, data.items(), fargs=[ax, float(data.keys()[-1]), stdout], interval =500);
#
#    # Set up formatting for the movie files
#    for encoder in matplotlib.animation.writers.list():
#    #{
#            #encode.
#            Writer = matplotlib.animation.writers[encoder];
#            writer = Writer(fps=2);
#            anim.save('block-heat.mp4', writer=writer);
#            
#            break;
#        #}
#    #}
#
#    # Workaround for non-'_file' encoders which stall because stderr stdout PIPE buffers filling...
#    sys.stdout = stdout;
#
#    print;
#}
#============================================================#

#============================================================#
#if len(matplotlib.animation.writers.list()) == 0:
#{
#    print "Error generating plot, please install MEncoder or FFMpeg...";
#    sys.exit();
#}

begin_time = datetime.datetime.now();

eprint( "Generating graph...");

blk_width  = 4;
blk_height = 4;
unt_width  = 4;
unt_height = 4;

if (blk_width == 0 or blk_height == 0):
#{
    eprint( "Error: invalid block dimensions of {} by {} specified!".format(blk_width, blk_height));
    exit(0);
#}
if (unt_width == 0 or unt_height == 0):
#{
    eprint( "Error: invalid unit dimensions of {} by {} specified!".format(unt_width, unt_height));
    exit(0);
#}

eprint( "Block layout: {} by {}...".format(blk_width, blk_height));
eprint( "Unit layout: {} by {}...".format(unt_width, unt_height));

for arg in sys.argv:
#{
    parse_temperatures(arg);
#}

max_cycle = str(max(data.iterkeys()));

#print "Last cycle:", max_cycle

avg_temps()

#create_graph();
end_time = datetime.datetime.now();

t = get_hms(int((end_time - begin_time).total_seconds()));
eprint( "Time to generate graph : {:02}:{:02}:{:02}...".format(t[0], t[1], t[2]));
#============================================================#
