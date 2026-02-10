#!/bin/sh

if [ $# -lt 2 ] 
then
    echo "Not enough arguments supplied, expected 2, found $#"
    echo "Usage: $0 <filesdir> <searchstr>"
    exit 1
fi

Directory=$1

if [ ! -d "$Directory" ]
then
    echo "$Directory does not represent a directory in the filesystem"
    exit 1
fi

nr_files=$(find "$Directory" -type f | wc -l)
nr_lines=$(grep -r "$2" "$1" | wc -l)

echo "The number of files are $nr_files and the number of matching lines are $nr_lines"