#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

set -e
set -u

NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
#username=$(cat conf/username.txt)

#Assignment 4 specific variables
ASSIGNMENT4WRITEDIR=/tmp/assignment4-result.txt
ASSIGNMENT4CONFDIR=/etc/finder-app/conf
usernameASSIGNMENT4=$(cat $ASSIGNMENT4CONFDIR/username.txt)
BINDIR=/usr/bin

if [ $# -lt 3 ]
then
	echo "Using default value ${WRITESTR} for string to write"
	if [ $# -lt 1 ]
	then
		echo "Using default value ${NUMFILES} for number of files to write"
	else
		NUMFILES=$1
	fi	
else
	NUMFILES=$1
	WRITESTR=$2
	ASSIGNMENT4WRITEDIR=/tmp/assignment4-result.txt
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${ASSIGNMENT4WRITEDIR}"

rm -rf "${ASSIGNMENT4WRITEDIR}"

# create $ASSIGNMENT4WRITEDIR if not assignment1
assignment=`cat ${ASSIGNMENT4CONFDIR}/assignment.txt`

if [ $assignment != 'assignment1' ]
then
	mkdir -p "$ASSIGNMENT4WRITEDIR"

	#The ASSIGNMENT4WRITEDIR is in quotes because if the directory path consists of spaces, then variable substitution will consider it as multiple argument.
	#The quotes signify that the entire string in ASSIGNMENT4WRITEDIR is a single string.
	#This issue can also be resolved by using double square brackets i.e [[ ]] instead of using quotes.
	if [ -d "$ASSIGNMENT4WRITEDIR" ]
	then
		echo "$ASSIGNMENT4WRITEDIR created"
	else
		exit 1
	fi
fi


for i in $( seq 1 $NUMFILES)
do
	$BINDIR/writer "$ASSIGNMENT4WRITEDIR/${usernameASSIGNMENT4}$i.txt" "$WRITESTR"
done

OUTPUTSTRING=$($BINDIR/finder.sh "$ASSIGNMENT4WRITEDIR" "$WRITESTR")

# remove temporary directories
rm -rf "${ASSIGNMENT4WRITEDIR}"

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
	echo "success"
	exit 0
else
	echo "failed: expected  ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
	exit 1
fi
