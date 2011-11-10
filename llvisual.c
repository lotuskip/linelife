/* A tool to generate png images from Linelife sequences. See the README file
 * for more information. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define PNG_DEBUG 3
#include <png.h>

int usage()
{
	puts("Usage: ./llvisual <filename> [-s stepsize(>=1)] [-h height(>=1)]");
	return 1;
}

void free_rowptrs(unsigned char **rp, int size)
{
	int i;
	for(i = 0; i < size; ++i)
		free(rp[i]);
	free(rp);
}

void abort_(char *msg)
{
	puts(msg);
	abort();
}


int main(int argc, char* argv[])
{
	int fileind = 0;
	int scaling = 1;
	int height = 50, width;
	int i;
	float f;
	char ch;
	unsigned char **rowptrs;
	FILE *file;
	png_structp png_ptr;
    png_infop info_ptr;

	for(i = 1; i < argc; ++i)
	{
		if(!strcmp(argv[i], "-s")) /* stepsize */
		{
			if(++i == argc || (scaling = atoi(argv[i])) < 1)
				return usage();
		}
		else if(!strcmp(argv[i], "-h")) /* height of image */
		{
			if(++i == argc || (height = atoi(argv[i])) < 1)
				return usage();
		}
		/* else it's just the output filename */
		else if(fileind)
			return usage(); /* don't accept multiple filenames! */
		else
			fileind = i;
	}

	if(!fileind)
		return usage();
	if(!(file = fopen(argv[fileind], "wb")))
	{
		puts("Could not open specified file for writing!");
		return 1;
	}

	/* rowptrs will store the image data */
	rowptrs = calloc(height, sizeof(char*));
	rowptrs[0] = NULL;

	/* read data from stdin */
	f = i = width = 0;
	for(;;)
	{
		/* input might end with a newline or not */
		if((ch = getchar()) == EOF || ch == '\n')
			break;

		if(ch == '1')
			f += 1.0f/scaling;
		else if(ch != '0')
			fputs("Warning: unknown characters read!\n", stderr);
		
		if(++i == scaling)
		{
			rowptrs[0] = realloc(rowptrs[0], ++width);
			rowptrs[0][width-1] = f*255;
			f = i = 0;
		}
	}
	if(!width)
	{
		fputs("Error: produced image would have width=0!\n", stderr);
		free(rowptrs);
		return 1;
	}
	printf("Image size is %ix%i\n", width, height);
	/* our rows are equal, so copy them: */
	for(i = 1; i < height; ++i)
	{
		rowptrs[i] = malloc(width);
		memcpy(rowptrs[i], rowptrs[0], width);
	}

	/* start writing the png output */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr)
	{
		puts("Error allocating png pointer!");
		free_rowptrs(rowptrs, height);
		return 1;
	}
    info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		puts("Error allocating info pointer!");
		free_rowptrs(rowptrs, height);
		return 1;
    }

	if(setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during init_io");
	png_init_io(png_ptr, file);

	if(setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing header");
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	if(setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing bytes");
	png_write_image(png_ptr, rowptrs);

	if(setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during end of write");
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free_rowptrs(rowptrs, height);
	return 0;
}
