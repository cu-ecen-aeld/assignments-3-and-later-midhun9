#!/bin/bash
# writer script to write the text string passed to the file passed as first argument. If the file does not exist create a new file and write the string
# Paramaeters : first argument  - full path to a file (including filename) on the filesystem
#               second argument - text string which will be written in the file specified above 
# Author: Midhun Kumar Koneru



writefile="$1"
DIR="`dirname "$1"`"
writestr="$2"

mkdir -p "$DIR"

if [ $# -eq 2 ]; then
	if [ ! -f "$writefile" ]; then
		echo "passed file does not exist, creating new file"
		touch "$writefile"
	fi
else
        echo "Number of Arguments invalid: should be equal to 2"
	echo "argument 1 is full path to file, argument 2 is text string to be written"
        exit 1
fi

if [ ! -f $writefile ]; then
	echo "couldnot create requested file"
	exit 1
fi

echo $writestr > $writefile

echo "Text written successfully"

