#include "stdafx.h"
#include "HighCapabilityCompositeAlg.h"
#include "MainFrm.h"

void HighCapabilityCompositeAlg::InitDecompressInfo(jpeg_decompress_struct * cinfo)
{
}

void HighCapabilityCompositeAlg::InitCompressInfo(jpeg_compress_struct * cinfo)
{
}

void HighCapabilityCompositeAlg::InitDecompressInfo(jpeg_decompress_struct * cinfo, FILE * file)
{
}

void HighCapabilityCompositeAlg::InitCompressInfo(jpeg_compress_struct * cinfo, FILE * file)
{
}

HighCapabilityCompositeAlg::HighCapabilityCompositeAlg()
{
}


HighCapabilityCompositeAlg::~HighCapabilityCompositeAlg()
{
}

void HighCapabilityCompositeAlg::ExecuteEmbedingAlg()
{
	FileList fileList = this->GetFileList();
	//char* SuffixArr = nullptr;
	//int SuffixCount = 0;
	if (fileList.CarrierImagePathList.size() == 1)
	{
		FILE* file;
		{
			string fName = fileList.CarrierImagePathList[0];
			const char* p = fName.data();
			if (fopen_s(&file, p, "rb") != 0)
				return;
		}
		jpeg_decompress_struct cinfo;
		jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);
		jpeg_stdio_src(&cinfo, file);
		jpeg_read_header(&cinfo, TRUE);
		cinfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;
		for (int i = 0; i < NUM_QUANT_TBLS; i++)
		{
			JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[i];
			if (qTbl == NULL)
				break;
			for (int j = 0; j < DCTSIZE2; j++)
			{

			}
			
		}
	}
}

void HighCapabilityCompositeAlg::ExecuteExtractingAlg()
{
}

int HighCapabilityCompositeAlg::CalPayLoad(FILE * file)
{
	return 0;
}

int HighCapabilityCompositeAlg::CalPayLoads(FileList * fileList)
{
	return 0;
}

ALGORITHM_NAME HighCapabilityCompositeAlg::GetAlgName()
{
	return ALGORITHM_NAME();
}
