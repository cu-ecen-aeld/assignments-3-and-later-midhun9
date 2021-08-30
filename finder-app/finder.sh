#!/bin/bash
# finder script to find number of files and number of matching lines with the search string passed
# Paramaeters : first argument  - path to a directory on a file system
#		second argument - text string which will be searched within the files in the above directory 
# Author: Midhun Kumar Koneru

filesdir="$1"
searchstr="$2"

if [ $# -eq 2 ]; then
	if [ ! -d $1 ]; then
		echo "passed directory does not exist"
		exit 1
	fi
else
	echo "Number of Arguments invalid: should be equal to 2"
	echo "argument 1 is path to directory on file system, argument 2 is text string to be searched within the directory"
	exit 1
fi

function total_num_of_files() {
	find $filesdir -type f | wc -l
}

function total_num_matching_lines() {
	grep -R $searchstr $filesdir | wc -l
}

total_files=$(total_num_of_files)
matching_lines=$(total_num_matching_lines)

#echo "The number of files are $(find $1 -type f | wc -l) and the number of matching lines are $(grep -R $2 $1 | wc -l)" #instead of functions we can directly use this line to get the results
echo "The number of files are $total_files and the number of matching lines are $matching_lines"
