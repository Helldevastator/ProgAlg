#include <iostream>
#include <memory.h>
#include <cassert>
#include <iomanip>
#include <cmath>
#include "FreeImagePlus.h"
#include "Stopwatch.h"

using namespace std;

////////////////////////////////////////////////////////////////////////
// Windows Types
#ifndef WIN32
typedef DWORD COLORREF;

/*
typedef struct tagRGBQUAD {
	BYTE    rgbBlue;
	BYTE    rgbGreen;
	BYTE    rgbRed;
	BYTE    rgbReserved;
} RGBQUAD;
*/
#endif

typedef struct TempPixel {
	int blue;
	int green;
	int red;

	void copy(const TempPixel& pixel) {
		blue = pixel.blue;
		green = pixel.green;
		red = pixel.red;
	}
};

////////////////////////////////////////////////////////////////////////
static BYTE dist(int x, int y) {
	int d = (int)sqrt(x*x + y*y);
	return (d < 256) ? d : 255;
}

////////////////////////////////////////////////////////////////////////
static void processSerial(const fipImage& input, fipImage& output) {
	assert(input.getWidth() == output.getWidth() && input.getHeight() == output.getHeight() && input.getImageSize() == output.getImageSize());
	assert(input.getBitsPerPixel() == 32);

	const int fSize = 3;
	const int fSize2 = fSize/2;
	const int hFilter[][fSize] = {
		{ 1, 1, 1 },
		{ 0, 0, 0 },
		{ -1,-1,-1 }
	};
	const int vFilter[][fSize] = {
		{ 1, 0,-1 },
		{ 1, 0,-1 },
		{ 1, 0,-1 }
	};


	for (unsigned int v = fSize2; v < output.getHeight() - fSize2; v++) {
		for (unsigned int u = fSize2; u < output.getWidth() - fSize2; u++) {
			RGBQUAD iC;
			int hC[3] = { 0, 0, 0 };
			int vC[3] = { 0, 0, 0 };

			for (unsigned int j = 0; j < fSize; j++) {
				for (unsigned int i = 0; i < fSize; i++) {
					input.getPixelColor(u + i - fSize2, v + j - fSize2, &iC);
					hC[0] += hFilter[j][i]*iC.rgbBlue;
					vC[0] += vFilter[j][i]*iC.rgbBlue;
					hC[1] += hFilter[j][i]*iC.rgbGreen;
					vC[1] += vFilter[j][i]*iC.rgbGreen;
					hC[2] += hFilter[j][i]*iC.rgbRed;
					vC[2] += vFilter[j][i]*iC.rgbRed;
				}
			}
			RGBQUAD oC = { dist(hC[0], vC[0]), dist(hC[1], vC[1]), dist(hC[2], vC[2]), 255 };
			//RGBQUAD oC = { vC[0], vC[1], vC[2], 255 };
			output.setPixelColor(u, v, &oC);
		}
	}
}


static void DoHorizontalFilter(BYTE * ptrLastLine, BYTE * ptrNextLine, TempPixel & pixel) {
	pixel.blue = *ptrLastLine - *ptrNextLine;
	pixel.green = *(ptrLastLine + 1) - *(ptrNextLine + 1);
	pixel.red = *(ptrLastLine + 2) - *(ptrNextLine + 2);
}

static void DoVerticalSum(BYTE * ptrLastLine, BYTE * currentLine, BYTE * ptrNextLine, TempPixel & pixel) {
	pixel.blue = *ptrLastLine + *currentLine + *ptrNextLine;
	pixel.green = *(ptrLastLine + 1) + *(currentLine + 1) + *(ptrNextLine + 1);
	pixel.red = *(ptrLastLine + 2) + *(currentLine + 2) + *(ptrNextLine + 2);
}
////////////////////////////////////////////////////////////////////////
static void processSerialOpt(const fipImage& input, fipImage& output) {
	const int bypp = 4;
	assert(input.getWidth() == output.getWidth() && input.getHeight() == output.getHeight() && input.getImageSize() == output.getImageSize());
	assert(input.getBitsPerPixel() == bypp*8);

	const int fSize = 3;
	const int fSize2 = fSize / 2;
	const int hFilter[][fSize] = {
		{ 1, 1, 1 },
		{ 0, 0, 0 },
		{ -1,-1,-1 }
	};
	const int vFilter[][fSize] = {
		{ 1, 0,-1 },
		{ 1, 0,-1 },
		{ 1, 0,-1 }
	};

	TempPixel sumLast;
	TempPixel sumCurrent;
	TempPixel sumNext;
	//hfilter
	for (unsigned int v = fSize2; v < output.getHeight() - fSize2; v++) {
		BYTE * lineBefore = input.getScanLine(v - fSize2);
		BYTE * nextLine = input.getScanLine(v + fSize2);
		BYTE * currentResult = output.getScanLine(v);

		DoHorizontalFilter(lineBefore, nextLine, sumLast);
		lineBefore += 4;nextLine += 4;
		DoHorizontalFilter(lineBefore, nextLine, sumCurrent);
		lineBefore += 4;nextLine += 4;
		currentResult += 4;

		//horizontal
		for (unsigned int u = fSize2 + fSize2; u < output.getWidth(); u++) {
			DoHorizontalFilter(lineBefore, nextLine, sumNext);
			lineBefore += 4;nextLine += 4;

			*currentResult = sumLast.blue + sumCurrent.blue + sumNext.blue; 
			*(++currentResult) = sumLast.green + sumCurrent.green + sumNext.green;
			*(++currentResult) = sumLast.red + sumCurrent.red + sumNext.red;
			currentResult += 2;

			sumLast.copy(sumCurrent);
			sumCurrent.copy(sumNext);
		}
	}

	//vFilter
	for (unsigned int v = fSize2; v < output.getHeight() - fSize2; v++) {
		BYTE * beforeLine = input.getScanLine(v - fSize2);
		BYTE * currentLine = input.getScanLine(v);
		BYTE * nextLine = input.getScanLine(v + fSize2);
		BYTE * currentResult = output.getScanLine(v);

		DoVerticalSum(beforeLine, currentLine, nextLine, sumLast);
		beforeLine += 4; currentLine += 4; nextLine += 4;
		DoVerticalSum(beforeLine, currentLine, nextLine, sumCurrent);
		beforeLine += 4; currentLine += 4; nextLine += 4;
		currentResult += 4;
		for (unsigned int u = fSize2 + fSize2; u < output.getWidth(); u++) {
			DoVerticalSum(beforeLine, currentLine, nextLine, sumNext);
			beforeLine += 4; currentLine += 4; nextLine += 4;
			
			//combine results
			BYTE tmp = *currentResult;
			*currentResult = dist(tmp, sumLast.blue - sumNext.blue);currentResult++;
			tmp = *currentResult;
			*currentResult = dist(tmp, sumLast.green - sumNext.green);currentResult++;
			tmp = *currentResult;
			*currentResult = dist(tmp, sumLast.red - sumNext.red);currentResult++;
			tmp = *currentResult;
			*currentResult = 255;currentResult++;
			


				/*
			*currentResult =  sumLast.blue - sumNext.blue;currentResult++;
			*currentResult = sumLast.green - sumNext.green;currentResult++;
			*currentResult = sumLast.red - sumNext.red;currentResult++;
			*currentResult = 255;currentResult++;
			*/
			


			sumLast.copy(sumCurrent);
			sumCurrent.copy(sumNext);
		}
	}
	
}

////////////////////////////////////////////////////////////////////////
static void processParallel(const fipImage& input, fipImage& output) {
	const int bypp = 4;
	assert(input.getWidth() == output.getWidth() && input.getHeight() == output.getHeight() && input.getImageSize() == output.getImageSize());
	assert(input.getBitsPerPixel() == bypp*8);

	// TODO
}

////////////////////////////////////////////////////////////////////////
static bool operator==(const fipImage& im1, const fipImage& im2) {
	assert(im1.getWidth() == im2.getWidth() && im1.getHeight() == im2.getHeight() && im1.getImageSize() == im2.getImageSize());
	assert(im1.getBitsPerPixel() == 32);

	for(unsigned int i = 0; i < im1.getHeight(); i++) {
		COLORREF *row1 = reinterpret_cast<COLORREF*>(im1.getScanLine(i));
		COLORREF *row2 = reinterpret_cast<COLORREF*>(im2.getScanLine(i));
		for(unsigned int j = 0; j < im1.getWidth(); j++) {
			if (row1[j] != row2[j]) return false;
		}
	}
	return true;
}

static bool operator!=(const fipImage& im1, const fipImage& im2) {
	return !(im1 == im2);
}

////////////////////////////////////////////////////////////////////////
int imageProcessing(int argc, const char* argv[]) {
	cout << "Image processing started" << endl;
	if (argc < 3) {
		cerr << "Usage: " << argv[0] << " input-file-name output-file-name" << endl;
		return -1;
	}

	Stopwatch sw;
	fipImage image;

	// load image
	if (!image.load(argv[1])) {
		cerr << "Image not found: " << argv[1] << endl;
		return -1;
	}

	// create output images
	fipImage out1(image), out2(image), out3(image);

	// process image sequentially and produce out1
	cout << "Start sequential process" << endl;
	sw.Start();
	processSerial(image, out1);
	sw.Stop();
	double seqTime = sw.GetElapsedTimeMilliseconds();
	cout << seqTime << " ms" << endl;

	// process image sequentially but optimized and produce out2
	cout << "Start optimized sequential process" << endl;
	sw.Start();
	processSerialOpt(image, out2);
	sw.Stop();
	double seqOptTime = sw.GetElapsedTimeMilliseconds();
	cout << seqOptTime << " ms, speedup = " << seqTime/seqOptTime << endl;

	// compare out1 with out2
	cout << boolalpha << "The two operations produce the same results: " << (out1 == out2) << endl;

	// process image in parallel and produce out3
	cout << "Start parallel process" << endl;
	sw.Start();
	processParallel(image, out3);
	sw.Stop();
	cout << sw.GetElapsedTimeMilliseconds() << " ms, speedup = " << seqOptTime/sw.GetElapsedTimeMilliseconds() << endl;

	// compare out1 with out3
	cout << boolalpha << "The two operations produce the same results: " << (out1 == out3) << endl;

	// save output image
	if (!out1.save(argv[2])) {
		cerr << "Image not saved: " << argv[2] << endl;
		return -1;
	}

	return 0;
}