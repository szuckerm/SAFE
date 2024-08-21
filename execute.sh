#!/bin/bash

#Take the variable 
SAFE_PATH=/home/jmonsal/intel/projdocs/sa-api
QUEUE_NAME=fullL4-2

cd $SAFE_PATH
export SAFE_LOGS_PATHDIR=$SAFE_PATH/logs
export SAFE_INPUT_PATH=$SAFE_PATH/input

#execute
#cd /home/jmonsal/intel/projdocs/sa-api/
#./safe Queue1.NoLocality.BLAS2
./safe $QUEUE_NAME
