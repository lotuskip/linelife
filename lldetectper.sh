#!/bin/dash
# A script to detect periodicity in Linelife patterns, which are read from
# stdin. The only and mandatory argument is the periodicity to look for,
# eg. 2. Each pattern is run with linel for that many generations, and a
# very crude comparison is used to check for periodicity. This method might
# give some false positives on occasion, but should correctly detect
# periodicity always. Note that for instance a periodic pattern with period 3
# will NOT be detected as periodic with period 6 (which, admittedly, IS wrong).
# The searched period should be the smallest possible (the base period) in order
# to be detected.
#
# Note also that, as the shift is not taken into account at all, moving
# patterns are reported as periodic.
#
# As it is, the script prints the NON-periodic patterns to stdout and
# "discards" the periodic ones (they are printed to stderr, though). This
# should be easy to change.
if test $# -lt 1; then
	echo "No periodicity given" >&2
else
	n=$1
	while read s; do
# Running for N generations generates N+1 lines of output. Sort the produced
# patterns, remove duplicates, and if what is left is N lines, the pattern is
# likely periodic with period N (or: it happens to go extinct just at generation
# N, so that the run actually only generates N lines to begin with, or: it is M
# steps away from becoming periodic with period N-M).
		if test $(./linel $s -n $n -s | cut -f 7 -d ' ' | sort | uniq | wc -l) -eq $n
			then
			echo "discarded $s" >&2
		else
			echo $s
		fi
	done
fi
