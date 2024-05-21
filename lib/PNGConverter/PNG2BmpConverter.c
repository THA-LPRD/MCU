#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <png.h>

#define  BLACK   "00" // 0x0 // 00
#define  WHITE   "01" //0x1 // 01
#define  YELLOW  "10" //0x2 // 10
#define  RED     "11" // 0x3 // 11

//struct for Saving RGBA values for a single pixel 
struct Pixel {
	png_byte Red;
	png_byte Green;
	png_byte Blue;
	png_byte Alpha;
};
typedef struct Pixel Pixel;

struct PNG_info {
	int width;
	int height;
	bool RGBA;
	bool greyscale;
};
typedef struct PNG_info PNG_info;

Pixel* readPNG(char* png_name, int rowValue)
{
	//Opening the PNG file to read binary out
	FILE* PNGfile = fopen(png_name, "rb");
	if (PNGfile == NULL)
	{
		printf("Error: PNG file couldnt be opened");
		exit(1);
	}
	
	//Read Setup //http://www.libpng.org/pub/png/libpng-1.2.5-manual.html#section-4.1

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("Error: png_structp error");
		fclose(PNGfile);
		exit(1);
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr,NULL, NULL);
		printf("Error: png_infop error");
		fclose(PNGfile);
		exit(1);
	}

	png_infop end_info = png_create_info_struct(png_ptr);
	if (end_info == NULL)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,NULL);
		printf("Error: png_endinfop error");
		fclose(PNGfile);
		exit(1);
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,&end_info);
		printf("Error: setjmp error");
		fclose(PNGfile);
		exit(1);
	}
	
	//init libpng IO and telling it to read from the first pixel
	png_init_io(png_ptr, PNGfile);
	png_read_info(png_ptr, info_ptr);

	//reading png info
	int PNG_Width = png_get_image_width(png_ptr, info_ptr);
	int PNG_Height = png_get_image_height(png_ptr, info_ptr);
	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	//allocate Pixel Array for one row
	Pixel* pixelRowArray = (Pixel*)malloc(PNG_Width * sizeof(struct Pixel));
	if (pixelRowArray == NULL)
	{
		printf("Error: pixelRowArray couldnt get allocated");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(PNGfile);
		exit(1);
	}

	//converting data for safety or further processign

	// Some systems dont support 16 bit, so if the png is 16bit it
	// gets converted to 8 with this function 
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	//palette based pngs get convertet to RGB(if needed)
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	//if a greay scaled png has a bit depth less then 8 it gets 
	//filled to 8 for better processing
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	//if a png has alpha values, they then get converted into
	// an alpha channel 
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	//checks if fillers in bits are needed
	if (color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

	//for consistincy with pixel struct, if a png is purley greyscaled
	// it gets converted to RGBA 
	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	//reading the rows from png and allocating memory for it
	//Writing the Values from the rows into an Pixel Array
	//Values from rows get written in pixel and free'ed directly = Allocated mem at the same time gets / PNG Height
	png_bytep row_pointer = (png_bytep)malloc(png_get_rowbytes(png_ptr, info_ptr));

	if (row_pointer == NULL)
	{
		printf("Error: row_pointer could not be allocated");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(PNGfile);
		free(row_pointer);
		exit(1);
	}

	//going through the rows (reading info to NULL until desired row)
	for (int i = 0; i < rowValue; ++i) 
	{
		png_read_row(png_ptr, NULL, NULL);
	}

	png_read_row(png_ptr, row_pointer, NULL);

	int index = 0; // rowValue * pngwidth?

	for (int x = 0; x < PNG_Width * 4; x += 4)
	{
		pixelRowArray[index].Red = row_pointer[x];
		pixelRowArray[index].Green = row_pointer[x + 1];
		pixelRowArray[index].Blue = row_pointer[x + 2];
		pixelRowArray[index].Alpha = row_pointer[x + 3];
		index++;
	}

	// reading png data into row_pointers and closing file

	free(row_pointer);

	fclose(PNGfile);	

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	return pixelRowArray;
}

char* ColorTest(Pixel Pixel)
{
	  // Normalized values for black, white, yellow, and red
    static int black_threshold = 30; // threshold to consider a pixel black
    static int white_threshold = 225; // threshold to consider a pixel white
    static int yellow_r_min = 200, yellow_g_min = 200, yellow_b_max = 50; // thresholds for yellow
    static int red_r_min = 200, red_g_max = 50, red_b_max = 50; // thresholds for red
    
    if (Pixel.Red < black_threshold && Pixel.Green < black_threshold && Pixel.Blue < black_threshold) {
		return BLACK;
    }
    if (Pixel.Red > white_threshold && Pixel.Green > white_threshold && Pixel.Blue > white_threshold) {
        return WHITE;
    }
    if (Pixel.Red > yellow_r_min && Pixel.Green > yellow_g_min && Pixel.Blue < yellow_b_max) {
        return YELLOW;
    }
    if (Pixel.Red > red_r_min && Pixel.Green < red_g_max && Pixel.Blue < red_b_max) {
		return RED;
    }

    return "ERROR";
}

char* binaryToHex(char* binary) {
	// Allocate memory for the hexadecimal string
	char* hex = (char*)malloc(3 * sizeof(char));
	if (hex == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	// Convert binary to hexadecimal
	unsigned long decimal = strtoul(binary, NULL, 2);
	sprintf(hex, "%02lx", decimal);

	return hex;
}

void WriteBMP(FILE* BMPArrayFile, Pixel* PixelRowValues, int RowWidth)
{
	char colorByte[8] = { 0 };

	for (int i = 0; i < RowWidth; i += 4) {
		char color[4][3]; // 2 characters for each color plus null terminator
		for (int j = 0; j < 4; j++) {
			strncpy(color[j], ColorTest(PixelRowValues[i + j]), 2);
			color[j][2] = '\0'; // Null-terminate the string
		}

		for (int j = 0; j < 4; j++) {
			strncpy(colorByte + j * 2, color[j], 2);
		}

		char* hex = binaryToHex(colorByte); // Assuming binaryToHex is defined elsewhere

		fprintf(BMPArrayFile, "0x%s, ", hex);

		free(hex);
	}
}

PNG_info* readPNGinfo(char* png_name)
{
	PNG_info* PNGinfo = (PNG_info*)malloc(sizeof(PNG_info));

	FILE* PNGfile = fopen(png_name, "rb");
	if (PNGfile == NULL)
	{
		printf("Error: PNG file couldnt be opened");
		exit(1);
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("Error: png_structp error");
		fclose(PNGfile);
		exit(1);
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		printf("Error: png_infop error");
		fclose(PNGfile);
		exit(1);
	}

	png_init_io(png_ptr, PNGfile);
	png_read_info(png_ptr, info_ptr);

	int color_type = png_get_color_type(png_ptr, info_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) 
	{
		PNGinfo->greyscale = true;
		PNGinfo->RGBA = false;
	}
	
	if (color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_RGB) 
	{
		PNGinfo->greyscale = false;
		PNGinfo->RGBA = true;
	}

	PNGinfo->width = png_get_image_width(png_ptr, info_ptr);
	PNGinfo->height = png_get_image_height(png_ptr, info_ptr);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	fclose(PNGfile);

	return PNGinfo;
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("Error: Use ./exe C:/../file.png\n");
		return 1;
	}

	// Get attributes of PNG
	PNG_info* PNGinfo = readPNGinfo(argv[1]);
	if (PNGinfo == NULL) {
		printf("Error: Failed to read PNG info\n");
		return 1;
	}

	// Open the output file 
	FILE* BMPArrayFile = fopen("BMPconvertfile.txt", "w");
	if (BMPArrayFile == NULL) {
		printf("Error: Couldn't open output file for writing\n");
		free(PNGinfo);
		return 1;
	}

	// Main loop for reading and writing
	for (int i = 0; i < PNGinfo->height; i++)
	{
		Pixel* PixelRowValues = readPNG(argv[1], i, PNGinfo->width);
		if (PixelRowValues == NULL) {
			printf("Error: Failed to read PNG row\n");
			free(PNGinfo);
			fclose(BMPArrayFile);
			return 1;
		}
		WriteBMP(BMPArrayFile, PixelRowValues, PNGinfo->width);
		free(PixelRowValues);
	}

	
	fclose(BMPArrayFile);
	free(PNGinfo);

	printf("End\n");

	return 0;
}

//		printf("RGBA test: %u %u %u %u\n", PixelRowValues[i].Red, PixelRowValues[i].Green, PixelRowValues[i].Blue, PixelRowValues[i].Alpha);
