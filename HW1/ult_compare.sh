#!/bin/bash
#
####################################################################################################
# C Shai Yehezkel
#
# Script      :  ult_compare.sh
# Version     :  2.3
# Arguements  :  prog1,prog2 [2]
# Description :  Compares two programs with given test in the directory this script is executed.
#				 Also checks for memory leakage in each test, and informs the user whic program
#                Was more consuming memory-wise.
#
# Sub-scripts :  Valgrind
#
#
####################################################################################################

#Check arguements existence

if [ -z "$1" ]; then
    echo "$1 doesn't exist!"
    exit -1
fi
if [ -z "$2" ]; then
    echo "Running on single file $1"
fi

#Variables defenitions
LOG_NAME="ult_compare.log"
SEARCH_WORD="bytes allocated"
VALGRIND_LOG="valgrind.log"
VALGRIND="valgrind --log-file=$VALGRIND_LOG --error-exitcode=1 --leak-check=yes --track-origins=yes "
FILES=./test*.in
prog1_mem_favor=0
prog2_mem_favor=0

touch $VALGRIND_LOG	#In order to suppress output of VALGRIND. Will be deleted
rm $LOG_NAME 2>/dev/null #Initialze log


for f in $FILES # Loop over all test files
do
  echo "Processing $f file..."
  test_num=`(echo ${f} | cut -d'.' -f 2 | cut -d't' -f 3)`
  execute_first="$VALGRIND ./$1 "
  execute_second="$VALGRIND ./$2 "

  log_prog_1="$1_test$test_num.out"
  log_prog_2="$2_test$test_num.out"

  `$execute_first <$f >$log_prog_1`


   if [ "$?" == 1 ] # Check error output of VALGRIND
	then
	    echo "Test $test_num : $1 memory leakage! see $LOG_NAME"
	    cp $VALGRIND_LOG	"$1_test${test_num}_$VALGRIND_LOG"
	    echo "Test $test_num : $1 memory leakage!" >> $LOG_NAME
	    #read varname
	fi
	bytes_prog1=`grep -P "bytes allocated" $VALGRIND_LOG | cut -d" " -f11 | tr -d -c 0-9`
	if [ ! -z "$2" ]; then
		`$execute_second <$f >$log_prog_2`	
	   	if [ "$?" == 1 ]  # Check error output of VALGRIND
			then
			    echo "Test $test_num : $2 memory leakage! see $LOG_NAME"
			    cp $VALGRIND_LOG	"$2_test${test_num}_$VALGRIND_LOG"
			    echo "Test $test_num : $2 memory leakage!" >> $LOG_NAME
			    #read varname
		fi
		bytes_prog2=`grep -P "bytes allocated" $VALGRIND_LOG | cut -d" " -f11 | tr -d -c 0-9`


		if [ $bytes_prog1 -lt $bytes_prog2 ]; then #Check which program uses more bytes
			prog1_mem_favor=$((prog1_mem_favor+1))
		fi

		if [ $bytes_prog2 -lt $bytes_prog1 ]; then #Check which program uses more bytes
			prog2_mem_favor=$((prog1_mem_favor+1))
		fi
			
		DIFF=`diff ${1}_test$test_num.out ${2}_test$test_num.out`
		if [ "$DIFF" != "" ] 
			then
			    echo "Test $test_num : Different outputs! see $LOG_NAME"
			    echo "Test $test_num : Different outputs!" >> $LOG_NAME
			    #read varname
		fi
		if [ "$DIFF" == "" ] 
			then
				rm $log_prog_1
				rm $log_prog_2
		fi
	fi
	if [ -z "$2" ]; then
		rm $log_prog_1
	fi
done
if [ -f $LOG_NAME ]; then
	echo ""
    echo "Errors starts here!"
    echo ""
    cat $LOG_NAME	
    echo ""
    echo "Errors ends here!"
    exit -2
fi

if [ ! -z "$2" ]; then
#Inform user which prog was more memory consuming
	if [ $prog1_mem_favor -lt $prog2_mem_favor ]; then #Check which program uses more bytes
		echo "$2 Uses more bytes most of the times!"
	fi
	if [ $prog2_mem_favor -lt $prog1_mem_favor ]; then #Check which program uses more bytes
		echo "$1 Uses more bytes most of the times!"
	fi
fi
rm $VALGRIND_LOG
exit 0