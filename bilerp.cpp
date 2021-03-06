#include <string>
#include <stdint.h>
#include <ctime>
#include <vector>
#include <thread>
#include <mutex>
#include "Defs.h"
#include "ThreadMgr.h"

using namespace std;

void writeBitmap(std::string in_filename, size_t in_width, size_t in_height, unsigned char * in_data);
void generateLowResBitmap(size_t in_width, size_t in_height, unsigned char * in_data);

inline void interpolate4Pixels(unsigned char *dst, const vec3 & NW, const vec3 & NE, const vec3 & SW, const vec3 & SE, float x, float y);
void EnlargeSrcImage( size_t in_src_width, size_t in_src_height, const unsigned char * in_source, int & outSrcWidth, int & outSrcHeight, vector< vector<vec3> > & outSrcImg );
void generateFilteredImageOneRowThreaded(const ThreadArgList & argList);



void generateFilteredImage(int in_num_threads, size_t in_src_width, size_t in_src_height, const unsigned char * in_source, size_t in_dest_width, size_t in_dest_height, unsigned char * in_dest)
{
	// Start in_num_threads threads to filter one row at a time until the result image is complete.
	// Each thread should:
	// - Figure out the next row in the result image that needs to be computed
	// - Determine the color for each pixel in that row
	// - Set those pixels in in_dest
	// - Keep doing that until all the rows in the bitmap are complete
	if(in_num_threads <= 0)
		throw "# of threads should be 1 or higher.";
	int srcWidth, srcHeight;
	vector< vector<vec3> > srcImg;
	EnlargeSrcImage(in_src_width, in_src_height, in_source, srcWidth, srcHeight, srcImg);

	tFinished = new bool[in_dest_height];
	for(size_t i = 0 ; i < in_dest_height ; i++)
		tFinished[i] = false;
	
	IThreadMgrForRows * threadMgr = new ThreadMgrForRows(in_num_threads, srcWidth, srcHeight, srcImg, in_dest_width, in_dest_height, in_dest);
	threadMgr->Init();

	while( ! threadMgr->AllRowsProcessed() ) {
		// Threading.
		threadMgr->RunThread(generateFilteredImageOneRowThreaded);
	}
	threadMgr->JoinSynchronos();
	delete threadMgr;
	delete [] tFinished; tFinished = NULL;
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

	clock_t begin = clock();
	try {
	generateFilteredImage(8,generated_width, generated_height, generated_image, result_width, result_height, result_image);
	}
	catch(char * err) {
		printf("%s\n", err);
		exit(-1);
	}
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	printf("TIme elapsed : %f\n", elapsed_secs);
	
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


void EnlargeSrcImage( size_t in_src_width, size_t in_src_height, const unsigned char * in_source, int & outSrcWidth, int & outSrcHeight, vector< vector<vec3> > & outSrcImg )
{
	// enlarge the source image to properly interpolate from the boundary.
	outSrcWidth = 2 * (in_src_width + 1);
	outSrcHeight = 2 * (in_src_height + 1);
	// init src image.
	outSrcImg.resize(outSrcHeight);
	for(size_t i = 0 ; i < outSrcHeight ; i++) {
		outSrcImg[i].resize(outSrcWidth);
	}

	for(size_t y = 0 ; y < in_src_height; y++) {
		for(size_t x = 0 ; x < in_src_width ; x++) {
			int ix = x * 2 + 1;
			int iy = y * 2 + 1;

			outSrcImg[iy][ix].Set(&in_source[(y * in_src_width + x) * BYTES_PER_PIXEL]);
			outSrcImg[iy][ix+1] = outSrcImg[iy][ix];
			outSrcImg[iy+1][ix] = outSrcImg[iy][ix];
			outSrcImg[iy+1][ix+1] = outSrcImg[iy][ix];
		}
	}

	auto fillBoardRow = [&]( vector< vector<vec3> > & inImage, int destY, int srcY) {
		if(inImage.empty()) throw "image should have at least a row.";
		if(inImage[0].empty()) throw "image should have at least a row.";

		int height = inImage.size();
		int width = inImage[0].size();
		for(size_t i = 0 ; i < width ; i++) {
			inImage[destY][i] = inImage[srcY][i];
		}
	};

	auto fillBoardCol = [&]( vector< vector<vec3> > & inImage, int destX, int srcX) {
		if(inImage.empty()) throw "image should have at least a row.";
		if(inImage[0].empty()) throw "image should have at least a row.";

		int height = inImage.size();
		int width = inImage[0].size();
		for(size_t i = 0 ; i < height; i++) {
			inImage[i][destX] = inImage[i][srcX];
		}
	};

	fillBoardRow(outSrcImg, outSrcHeight - 1, outSrcHeight - 2);
	fillBoardRow(outSrcImg, 0, 1);
	fillBoardCol(outSrcImg, outSrcWidth - 1, outSrcWidth - 2);
	fillBoardCol(outSrcImg, 0, 1);
}

void interpolate4Pixels(unsigned char *dst, const vec3 & NW, const vec3 & NE, const vec3 & SW, const vec3 & SE, float x, float y)
{
	vec3 mixNorth = NW * (1.f - x) + NE * x;
	vec3 mixSouth = SW * (1.f - x) + SE * x;
	vec3 res = mixNorth * (1.f - y) + mixSouth * y;
	dst[0] = res.b;
	dst[1] = res.g;
	dst[2] = res.r;
}

void generateFilteredImageOneRowThreaded(const ThreadArgList & argList)
{
	size_t inRow = argList.curRow;
	IThreadMgrForRows * threadMgr = argList.threadMgr;
	size_t inSrcWidth = argList.srcWidth;
	size_t inSrcHeight = argList.srcHeight;
	vector<vector<vec3>> & inSource = argList.source;
	size_t inDestWidth = argList.destWidth;
	size_t inDestHeight = argList.destHeight;
	unsigned char * in_dest = argList.dest;

	if(inRow >= inDestHeight) return;

	for(size_t x = 0 ; x < inDestWidth ; x++) {

		float dsrcX = ( (float)(x) / (inDestWidth - 1)) * (inSrcWidth - 2) + 1;
		float dsrcY = ( (float)(inRow) / (inDestHeight - 1)) * (inSrcHeight - 2) + 1;

		int srcXFloor = ((int)(dsrcX / 2)) * 2;
		int srcXCeil = ((int)(dsrcX / 2 + 1)) * 2;
		float weightX = (dsrcX - srcXFloor) / 2; // srcXCeil - srcXFloor is always 2.
		int srcYFloor = ((int)(dsrcY / 2)) * 2;
		int srcYCeil = ((int)(dsrcY / 2 + 1)) * 2;
		float weightY = (dsrcY - srcYFloor) / 2; // srcXCeil - srcXFloor is always 2.

		srcXCeil = (srcXCeil >= inSrcWidth) ? inSrcWidth - 1 : srcXCeil;
		srcYCeil = (srcYCeil >= inSrcHeight) ? inSrcHeight - 1 : srcYCeil;

		interpolate4Pixels(in_dest + (inRow * inDestWidth + x) * BYTES_PER_PIXEL,
			inSource[srcYFloor][srcXFloor],
			inSource[srcYFloor][srcXCeil],
			inSource[srcYCeil][srcXFloor],
			inSource[srcYCeil][srcXCeil],
			weightX, weightY);
	}

	threadMgr->SetFinished(inRow);
}



#pragma endregion