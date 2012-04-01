#include <png.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char ** get_alpha(const char * filename, int *imgwidth, int *imgheight) {
	FILE *fp = fopen(filename, "rb");
	if (!fp)
	{
		fprintf (stderr, "could not open file \"%s\"\n", filename);
		return (NULL);
	}
	
	char header [8];
	int header_read = fread(header, 1, 8, fp);
	
	if (header_read != 8)
	{
		fprintf (stderr, "could read only %i bytes of the 8 byte png header of \"%s\"\n", header_read, filename);
		return NULL;
	}
	
	int is_png = !png_sig_cmp(header, 0, 8);

	if (!is_png)
	{
		fprintf (stderr, "file \"%s\" does not seem to be a png file\n", filename);
		return (NULL);
	}
	
	png_structp png_ptr = png_create_read_struct
		(PNG_LIBPNG_VER_STRING, NULL,
		NULL, NULL);

	if (!png_ptr) {
		fprintf (stderr, "could not allocate png reading data structure\n");
		return (NULL);
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr)
	{
		fprintf (stderr, "could not allocate png info data structure\n");
		png_destroy_read_struct(&png_ptr,
			(png_infopp)NULL, (png_infopp)NULL);
		return (NULL);
	}
	
	png_init_io(png_ptr, fp);
	
	png_set_sig_bytes(png_ptr, 8);
	
	png_read_png(png_ptr, info_ptr, /*png_transforms*/ 0, NULL);
	
	int bit_depth, color_type;
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)imgwidth, (png_uint_32*)imgheight,
		&bit_depth, &color_type, NULL,
		NULL, NULL); /* these casts are okay because int = uint32 for not too high numbers */
	
	if (color_type != PNG_COLOR_TYPE_RGBA)
	{
		fprintf (stderr, "only RGBA pngs are acceptable\n");
		return NULL;
	}
	
	if (bit_depth == 16)
	#if PNG_LIBPNG_VER >= 10504
		png_set_scale_16(png_ptr);
	#else
		png_set_strip_16(png_ptr);
	#endif
	
	// TODO: put this into the copy to alpha loop below and read only one row at once to reduce memory usage
	png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
	
	unsigned char ** alpha = (unsigned char **) malloc (*imgwidth*sizeof(unsigned char*));
	for (int x=0; x<*imgwidth; x++)
	{
             alpha[x] = (unsigned char*) malloc (*imgheight*sizeof(unsigned char));
        }
        
        for (int y=0; y<*imgheight; y++)
        {
        	for (int x=0; x<*imgwidth; x++)
        	{
        		/* 4 bytes per pixel, alpha is the last and third when counting from 0 */
        		alpha[x][y] = row_pointers[y][x*4+3];
        	}
        }
	
	png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp)NULL);
	
	return alpha;
}
