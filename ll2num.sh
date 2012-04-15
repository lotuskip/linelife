#!/bin/dash
# This script reads a Linelife sequence of 1:s and 0:s, interprets that as a
# binary number, and outputs it both in decimal and hexadecimal (unsigned).
# This can be useful for representing long patterns in a more condensed form.
#
# Requires the 'bc' calculator (aside from standard UNIX tools).
#
# Usage: something like
#   echo 1010011100110010 | ./ll2num.sh
# or
#   ./ll2num.sh < mystoredsequence
#
# By default, both hex and decimal are printed and labeled; use '-x' to print
# only hex without labeling it (just prints a number) and '-d' for the same
# with decimal. If '-d' and '-x' are both present, both are printed without
# labeling (decimal first, then hex).
#
#parse arguments:
dodec=0
dohex=0
while [ "$*" != "" ]; do
	if test "$1" = "-d"; then
		dodec=1
	elif test "$1" = "-x"; then
		dohex=1
	fi
	shift
done

# read the sequences:
while read seq; do
if test $(echo $seq | grep '[^01]' | wc -w) -gt 0; then
	echo "Invalid sequence!"
else
	printf '%s: ' "$seq"
#convert and print what was requested:
	if [ $dodec -eq 0 -a $dohex -eq 0 ]; then
		echo -n " Decimal: "
	fi
	if [ $dohex -ne 1 -o $dodec -eq 1 ]; then
		printf "%s " "$(echo "obase=10; ibase=2; $seq" | bc)"
	fi
	if [ $dodec -eq 0 -a $dohex -eq 0 ]; then
		echo -n " Hex: "
	fi
	if [ $dohex -eq 1 -o $dodec -ne 1 ]; then
		printf "%s" "$(echo "obase=16; ibase=2; $seq" | bc)"
	fi
	echo ""
fi
done
