
/*

Hi Mr. Fafe,

I think it might be out of the scope for this problem. So did not really try it out, but commented about it here.
generateFilteredImage() function has this comment. --> Figure out the next row in the result image that needs to be computed.

It sounds like the structure will be like this.
If 8 threads is input, Thread#0 will do row 1, row (1*8), row (2*8), ... etc.


1. What if the algorithm weren't a simple filtering algorithm, but slow and complicated process like light transport calculation for 200 virtual lights in the space?
(Where one thread could be finished much later than the others 7 threads.)

2. What if the output size is huge like 100000x100000 and then one thread is slow because of L1/2 cache misses and even memory misses. (Since the code may not load the whole image at once due to the amount of the image data size.)

These situations will have bad load-balance and some threads might be significantly slower than others.

In situations like this, would it be better for one thread to only finish one row?
(That way, each thread is independent from each other.)

Of course, in this simple test case, it wouldn't make sense since creating a thread is so slow.

I would just like to leave some of my thoughts.

*/

#include <string>
#include <stdint.h>

const size_t BYTES_PER_PIXEL = 3; // 24 bit color depth :D

void writeBitmap(std::string in_filename, size_t in_width, size_t in_height, unsigned char * in_data);
void generateLowResBitmap(size_t in_width, size_t in_height, unsigned char * in_data);

#define TAEKYU_DEBUG
#ifdef TAEKYU_DEBUG
#define CHECK_BOUND(x,y,xbound,ybound)		( (x >= 0 && x < xbound) && (y >= 0 && y < ybound) )
#else
#define CHECK_BOUND(x,y,xbound,ybound)	true
#endif

void inline pixelCopy(unsigned char * dst, const unsigned char * src)
{
	for(int i = 0 ; i < BYTES_PER_PIXEL ; i++) {
		dst[i] = src[i];
	}
}


void inline interpolate4Pixels(unsigned char *dst, const unsigned char * NW, const unsigned char * NE, const unsigned char * SW, const unsigned char * SE, float x, float y)
{
	unsigned char mixNorth[3] = { 0 };
	for(size_t i = 0 ; i < BYTES_PER_PIXEL ; i++) {
		mixNorth[i] = NW[i] * (1.f - x) + NE[i] * x;
	}

	unsigned char mixSouth[3] = { 0 };
	for(size_t i = 0 ; i < BYTES_PER_PIXEL ; i++) {
		mixSouth[i] = SW[i] * (1.f - x) + SE[i] * x;
	}

	for(size_t i = 0 ; i < BYTES_PER_PIXEL ; i++) {
		dst[i] = mixNorth[i] * (1.f - y) + mixSouth[i] * y;
	}
}



void generateFilteredImage(int in_num_threads, size_t in_src_width, size_t in_src_height, const unsigned char * in_source, size_t in_dest_width, size_t in_dest_height, unsigned char * in_dest)
{
	// Start in_num_threads threads to filter one row at a time until the result image is complete.
	// Each thread should:
	// - Figure out the next row in the result image that needs to be computed
	// - Determine the color for each pixel in that row
	// - Set those pixels in in_dest
	// - Keep doing that until all the rows in the bitmap are complete

	size_t bound = in_src_height * in_src_width * BYTES_PER_PIXEL;

	for(size_t y = 0 ; y < in_dest_height ; y++) {
		for(size_t x = 0 ; x < in_dest_width ; x++) {

			float dsrcX = ( (float)(x + 0.5) / in_dest_width) * (in_src_width - 1) ;
			float dsrcY = ( (float)(y + 0.5) / in_dest_height) * (in_src_height - 1);

			int srcX = (int) dsrcX;
			int srcY = (int) dsrcY;

			float weightX = dsrcX - srcX;
			float weightY = dsrcY - srcY;
			
			// Source image
			int srcOffset = (srcY * in_src_width + srcX) * BYTES_PER_PIXEL;

			unsigned char NW[BYTES_PER_PIXEL], NE[BYTES_PER_PIXEL], SW[BYTES_PER_PIXEL], SE[BYTES_PER_PIXEL];

			pixelCopy(NW, in_source + srcOffset);
			if (CHECK_BOUND(srcX + 1, srcY, in_src_width, in_src_height) )
				pixelCopy(NE, in_source + srcOffset + BYTES_PER_PIXEL);
			else {
				pixelCopy(NE, NW);
				printf("Out of bound : (%d , %d) \n", srcX, srcY); exit(-1);
			}

			if (CHECK_BOUND(srcX, srcY + 1, in_src_width, in_src_height) )
				pixelCopy(SW, in_source + srcOffset + in_src_width * BYTES_PER_PIXEL);
			else {
				pixelCopy(SW, NW);
				printf("Out of bound : (%d , %d) \n", srcX, srcY); exit(-1);
			}

			if (CHECK_BOUND(srcX + 1, srcY + 1, in_src_width, in_src_height) )
				pixelCopy(SE, in_source + srcOffset + (in_src_width + 1) * BYTES_PER_PIXEL);
			else {
				pixelCopy(SE, NW);
				printf("Out of bound : (%d , %d) \n", srcX, srcY); exit(-1);
			}

			int dstOffset = (y * in_dest_width + x) * BYTES_PER_PIXEL;
			interpolate4Pixels(in_dest + dstOffset, NW, NE, SW, SE, dsrcX - srcX, dsrcY - srcY);
		}
	}
}

#pragma region main
int main(int argc, char ** argv)
{
	const size_t result_height = 512;
	const size_t result_width = 512;
	const size_t generated_height = 3;
	const size_t generated_width = 3;

	// Generate a 3x3 lowres image
	// Use Bilinear Interpolation to create a larger version (512x512)
	// Write the result image out to "bilerp.bmp"

	unsigned char * generated_image = new unsigned char[generated_width*generated_height*BYTES_PER_PIXEL];
	memset(generated_image,0,generated_width*generated_height*BYTES_PER_PIXEL);

	generateLowResBitmap(generated_width,generated_height,generated_image);

	unsigned char * result_image = new unsigned char[result_width*result_height*BYTES_PER_PIXEL];
	memset(result_image,0,result_width*result_height*BYTES_PER_PIXEL);

	generateFilteredImage(8,generated_width, generated_height, generated_image, result_width, result_height, result_image);
	
	delete[] generated_image;

	writeBitmap("bilerp.bmp",result_width,result_height,result_image);

	delete[] result_image;

	return 0;
}
#pragma endregion

#pragma region Lowres Bitmap Creator
void generateLowResBitmap(size_t in_width, size_t in_height, unsigned char * in_data)
{
	for(size_t y = 0; y < in_height; ++y)
	{
		for(size_t x = 0; x < in_width; ++x)
		{
			int val = y*in_width+x; // Used to generate the color
			int offset = val*BYTES_PER_PIXEL;
			in_data[offset+0] = 0;
			in_data[offset+1] = 0;
			in_data[offset+2] = 0;
			if((val&1)==1)
				in_data[offset+0] = 255; // Blue
			if((val&2)==2)
				in_data[offset+1] = 255; // Green
			if((val&4)==4)
				in_data[offset+2] = 255; // Red
		}
	}
}
#pragma endregion

#pragma region Bitmap Writer
#pragma pack(push, 2)
struct BmpFileHeader {
	char magic[2];
	uint32_t filesize;
	uint16_t reserved_1;
	uint16_t reserved_2;
	uint32_t offset; // of the data partition
};

struct BmpInfoHeader {
	uint32_t size; // of the BmpInfoHeader
	uint32_t width;
	uint32_t height;
	uint16_t num_color_planes; // Must be 1
	uint16_t depth;
	unsigned char unused[24]; // This is not functionality we're using here :)
};
#pragma pack(pop)

void writeBitmap(std::string in_filename, size_t in_width, size_t in_height, unsigned char * in_data)
{
	size_t filesize = 54 + BYTES_PER_PIXEL*in_width*in_height; // Header + 3 Color

	// Using C style files here because fwrite is more conducive to this than ofstream::write
	//  since it has a count (used in the padding portion)
	FILE * f = NULL;
#ifdef _MSC_VER
	errno_t err = fopen_s(&f,in_filename.c_str(),"wb");
	if(f==NULL || err != 0)
		return;
#else//_MSC_VER
	f = fopen(in_filename.c_str(),"wb");
	if(f==NULL)
		return;
#endif//_MSC_VER

	struct BmpFileHeader file_header;
	memset(&file_header,0,sizeof(struct BmpFileHeader));
	file_header.magic[0] = 'B'; file_header.magic[1] = 'M';
	file_header.offset = sizeof(struct BmpFileHeader) + sizeof(BmpInfoHeader);
	fwrite(&file_header,1,sizeof(struct BmpFileHeader),f);

	struct BmpInfoHeader info_header;
	memset(&info_header,0,sizeof(struct BmpInfoHeader));
	info_header.size = sizeof(struct BmpInfoHeader);
	info_header.width = in_width;
	info_header.height = in_height;
	info_header.num_color_planes = 1;
	info_header.depth = 24;
	fwrite(&info_header,1,sizeof(struct BmpInfoHeader),f);

	unsigned char padding[] = {0,0,0,};
	for(size_t i=0;i<in_height;++i)
	{
		// in_height-i-1 because the scanlines are bottom up in a bitmap :)
		fwrite(in_data+(in_width)*(in_height-i-1)*BYTES_PER_PIXEL,BYTES_PER_PIXEL,in_width,f);
		// The rows need to be 4 byte aligned
		fwrite(padding,1,(4-(in_width*BYTES_PER_PIXEL)%4)%4,f);
	}

	fclose(f);
	return;
}
#pragma endregion