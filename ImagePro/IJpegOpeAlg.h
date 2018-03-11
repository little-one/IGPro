#pragma once

#ifndef HIDENFILEPROCESS
#include "BinaryFileSolver.h"
#include "Encrypt_Decrpty.h"
#include "FolderMonitor.h"
#include "StreamConvert.h"
#endif // !HIDENFILEPROCESS

#ifndef EXTERNJPEGLIB
extern "C"
{
#include "jpeglib.h" 
}
#endif // !EXTERNJPEGLIB

class IJpegOpeAlg
{

public:
	IJpegOpeAlg();
	virtual ~IJpegOpeAlg();
	virtual jpeg_decompress_struct InitDecompressInfo() = 0;
	virtual jpeg_compress_struct InitCompressInfo() = 0;
};

