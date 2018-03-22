#include "stdafx.h"
#include "LSBLossyImplantAlg.h"


LSBLossyImplantAlg::LSBLossyImplantAlg()
{
	
}

LSBLossyImplantAlg::LSBLossyImplantAlg(FileList * fileList)
{
	this->SetFileList(*fileList);
}


LSBLossyImplantAlg::~LSBLossyImplantAlg()
{

}

jpeg_decompress_struct * LSBLossyImplantAlg::InitDecompressInfo()	
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	return &cinfo;
}

jpeg_decompress_struct* LSBLossyImplantAlg::InitDecompressInfo(FILE* file)
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, TRUE);
	return &cinfo;
}

jpeg_compress_struct * LSBLossyImplantAlg::InitCompressInfo(FILE * file)
{
	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);
	return &cinfo;
}

void LSBLossyImplantAlg::ExecuteEmbedingAlg()		
{
	//FileList fileList = this->GetFileList();
	//for (auto i = fileList.CarrierImagePathList.begin(); i != fileList.CarrierImagePathList.end(); i++)
	//{
	//	string fName = *i;
	//	FILE* file;
	//	const char* p = fName.data();
	//	fopen_s(&file, p, "rb");
	//}
	FileList fileList = this->GetFileList();
	if (fileList.CarrierImagePathList.size() == 1)	//单载体
	{
		
	}
	else if(fileList.CarrierImagePathList.size()==0)
	{
		//抛出异常
		return;
	}
	else  //多载体
	{

	}
}

void LSBLossyImplantAlg::ExecuteExtractingAlg()
{

}

int LSBLossyImplantAlg::CalPayLoad(FILE * file)
{
	jpeg_decompress_struct* cinfo = InitDecompressInfo(file);
	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(cinfo);
	int HideCounter = 0;
	for (int ci = 0; ci < 3; ci++)
	{
		JBLOCKARRAY buffer;
		JCOEFPTR blockptr;
		jpeg_component_info* compptr;
		compptr = cinfo->comp_info + ci;
		for (int by = 0; by < compptr->height_in_blocks; by++)
		{
			buffer = (cinfo->mem->access_virt_barray)((j_common_ptr)cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			{
				blockptr = buffer[by][bx];
				for (int bi = 0; bi < 64; bi++)
				{
					if (blockptr[bi] == 0)
						continue;
					HideCounter++;
				}
			}
		}
	}
	FinishDecompress(cinfo);
	return HideCounter;
}

int LSBLossyImplantAlg::CalPayLoads(FileList * fileList)
{
	int HideCounter = 0;
	for (auto i = fileList->CarrierImagePathList.begin(); i != fileList->CarrierImagePathList.end(); i++)
	{
		string fName = *i;
		FILE* file;
		const char* p = fName.data();
		fopen_s(&file, p, "rb");
		HideCounter += CalPayLoad(file);
		fclose(file);
	}
	return HideCounter;
}


ALGORITHM_NAME LSBLossyImplantAlg::GetAlgName()
{
	return ALG_DCTLSB;
}

jpeg_compress_struct * LSBLossyImplantAlg::InitCompressInfo()
{
	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	return &cinfo;
}

