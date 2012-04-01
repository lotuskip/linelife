/* This program has been used in exhaustive pattern testing for Linelife.
 * It generates all possible patterns of a given length, omitting however
 * reverse duplicates.
 *  Usage:
 *   ./llgen N
 * where N is the sequence length. Most likely you want to create a FIFO pipe,
 * forward the output into it, and use it as a batch file for linelife,
 * forwarding again the output somewhere (probably grepping it on the way).
 *
 * Compile with "cc llgen.c -o llgen -O2". */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int len, i, old_i, wgt;
	char *seq;
	unsigned long num = 0, printed = 0;

	/* length 1024 would be absolutely ridiculous, but having a ridiculous
 	 * upper bound is better than no bound */
	if(argc < 2 || (len = atoi(argv[1])) < 3 || len > 1024)
	{
		fputs("Required argument missing or invalid.", stderr);
		return 1;
	}

	seq = malloc(len+1);
	seq[len] = '\0';
	/* if the pattern is of length len, the endpoints must be alive: */
	seq[0] = seq[len-1] = '1';
	for(i = 1; i < len-1; ++i)
		seq[i] = '0';

	i = 1;
	wgt = 0; /* the "weight" of the pattern; used to determine whether
		a reverse pattern exists. The computation of this below
		might seem a bit involved, but it's not too bad. */
	/* Our generation algorithm is basically a binary ++ operator: */
	while(i < len-1)
	{
		if(seq[i] == '0')
		{
			seq[i] = '1';
			wgt += i - len/2 + (i>=(len/2))*(!(len%2));
			/* A bit of explanation:
 			 * "(i>=(len/2))*(!(len%2))" is just 0 or 1.
 			 * It is 1 when 'len' is even and i >= len/2,
 			 * 0 otherwise. So this is a fix on the right
 			 * hand side weighs for even-length patterns. */
		}
		else
		{
			seq[(old_i = i)] = '0';
			wgt -= i/* - len/2*/ + (i>=(len/2))*(!(len%2));
			/* (len/2 gets deducted below, so no need to add it here) */
			while(++i < len-1 && seq[i] == '1')
			{
				seq[i] = '0';
				wgt -= i - len/2 + (i>=(len/2))*(!(len%2));
			}
			if(i == len-1) /* without this we never stop! */
				break;
			seq[i] = '1';
			wgt += i/* - len/2*/ + (i>=(len/2))*(!(len%2));
			i = old_i;
		}

		/*printf("%s (%i)\n", seq, wgt);*/
		if(wgt >= 0)
		{
			puts(seq);
			++printed;
		}
		/* else the pattern is left-weighted and has a reverse that is
		 * right-weighted, so lets not print both. */

		/* Print progress information when generating a lot of data */
		if(!((++num)%20736))
			fprintf(stderr, "%u done (%u printed)\n", num, printed);
	}
	free(seq);
	return 0;
}

