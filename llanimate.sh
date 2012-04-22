#!/bin/bash
# Script to make an animation from the step-wise output of linelife. The 
# patterns are taken from stdin (and they should be in the format linelife
# produces).
# Usage something like:
#  ./linel 11001101101 -n 20 -s | ./llanimate.sh 10 -s 1 -h 100
# where the 1st argument "10" is the delay between frames, in 1/100th seconds,
# and the rest of the arguments are to be passed to llvis for generating the
# frames. The animation will contain one frame per generation.
#
# Requirements: llvis to make the pictures, imagemagick (convert) to convert
# png->gif, gifsicle to create the animation, awk, head, and some other
# standard utils.
if ! which convert > /dev/null 2> /dev/null
	then
		echo "imagemagick (convert) is required for this script to work."
	exit
fi
if ! which gifsicle > /dev/null 2> /dev/null
	then
		echo "gifsicle is required for this script to work."
	exit
fi
if ! which llvis > /dev/null 2> /dev/null
	then
		echo "llvis must be in your PATH for this script to work."
	exit
fi

if test $# -lt 1; then
	echo "missing delay argument"
elif test $1 -lt 1; then
	echo "invalid delay argument"
else
	delay=$1
	shift
#Store everything except "summary line", remove all but patterns and shifts:
	head -n -1 | sed 's/^.* shift //' | sed 's/)://' > lla_tmp
#Find minimum shift value (giving the position of the left edge):
	minshift=$(awk 'm=="" || $1 < m {m=$1} END{print m}' FS=" " lla_tmp)
#Find maximum [shift]+[length] value (giving right edge position):
	maxright=$(awk 'm=="" || $1+length($2) > m {m=$1+length($2)}\
		END{print m}' FS=" " lla_tmp)

# Now we add the required amount of '0's to each line and pass them to llvis.
# Note that we *intentionally* add one extra '0' to both ends! If you don't
# want this, then just change the ">=" and "<=" to ">" and "<".
	awk "{for(i = \$1; i >= $minshift; --i) printf \"0\"; printf \"%s\", \$2;\
		for(i = \$1+length(\$2); i <= $maxright; ++i) printf \"0\";\
		print \"\"}" FS=" " lla_tmp > lla_tmp2
# Pass each generate line to llvis:
	exec < lla_tmp2
	n=1
	while read p; do
		echo $p | llvis frame$(printf "%05d" "$n").png $* > /dev/null
		n=$((n+1))
	done

# Convert to gif:
	for f in frame*.png; do
		convert $f $f.gif
	done
# Animate:
	gifsicle -l -d $delay frame*.png.gif > animation.gif

# Remove temporary files:
	rm -f lla_tmp lla_tmp2 frame*.png frame*.png.gif
fi
