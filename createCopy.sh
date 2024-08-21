#!/bin/bash
# create a new copy of SAFE for experiments. 

#Check the number of parameters
numArgs=$#
if [[ $numArgs -ne 1 ]]; then 
	printf "Use: $0 [DESTINATION/PATH] \n"
	exit;
fi

#check if the directory exist, if not, create it. 
Directory=$1
printf "Checking directory ... \n"
if [[ ! -d $Directory ]]; then
	read -p "Directory $Directory doesn't exist. do you want to create it? (Y/N) " -n 1 -r
	echo
	if [[ ! $REPLY =~ ^[Yy]$ ]]; then
		printf "Provide a valid directory.\n"
		exit 1;
	else
		printf "Creating directory ... \n"
		`mkdir $Directory`
	fi
fi

##Check if the directory is empty (To avoid a mess)
printf "Checking if directory is empty... \n"
if [[ $(ls -A $Directory) ]]; then 
	printf "Not Empty \n Provide a valid directory.\n"
	exit 1;
fi

##Copying the files to the new directory. 
printf "Copying the files to $Directory ...\n"
`cp -r *.cpp *.c *.h Makefile *.py createCopy.sh input docs  instructionNewFormat_3_0_8_PerOpClassEnergyMap_TechScaled.txt execute.sh LICENSE README AUTHORS $Directory`
