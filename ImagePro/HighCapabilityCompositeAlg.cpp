#include "stdafx.h"
#include "HighCapabilityCompositeAlg.h"
#include "MainFrm.h"
#include <math.h>
#include <iostream>

void HighCapabilityCompositeAlg::InitDecompressInfo(jpeg_decompress_struct * cinfo)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_decompress(cinfo);
	return;
}

void HighCapabilityCompositeAlg::InitCompressInfo(jpeg_compress_struct * cinfo)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_compress(cinfo);
	return;
}

void HighCapabilityCompositeAlg::InitDecompressInfo(jpeg_decompress_struct * cinfo, FILE * file)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_decompress(cinfo);
	jpeg_stdio_src(cinfo, file);
	jpeg_read_header(cinfo, TRUE);
	cinfo->mem->max_memory_to_use = VIRT_MEMORY_SIZE;
	return;
}

void HighCapabilityCompositeAlg::InitCompressInfo(jpeg_compress_struct * cinfo, FILE * file)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_compress(cinfo);
	jpeg_stdio_dest(cinfo, file);
	cinfo->mem->max_memory_to_use = VIRT_MEMORY_SIZE;
	return;
}

HighCapabilityCompositeAlg::HighCapabilityCompositeAlg()
{
}

HighCapabilityCompositeAlg::HighCapabilityCompositeAlg(FileList * fileList, int MatrixK, int TableK)
{
	this->SetFileList(*fileList);
	this->MatrixK = MatrixK;
	this->TableK = TableK;
	for (int i = 0; i < fileList->CarrierImagePathList.size(); i++)
	{
		string fileP = fileList->CarrierImagePathList[i];
		FILE* file;
		{
			const char* p = fileP.data();
			fopen_s(&file, p, "rb");
		}
		int BCount = CalEffectiveBitsCount(file);
		fclose(file);
		this->EffectiveBitsCountVec.push_back(BCount);
	}
	StreamConvert sc;
	Table_DBit = sc.BitRepresentationMap[TableK].first;
	Table_NBit = sc.BitRepresentationMap[TableK].second;
}

HighCapabilityCompositeAlg::~HighCapabilityCompositeAlg()
{
}

void HighCapabilityCompositeAlg::ExecuteEmbedingAlg()
{
	FileList fileList = this->GetFileList();

	//获取文件后缀名保存到信息最前面
	char* SuffixArr = nullptr;
	int SuffixCount = 0;
	{
		string s = fileList.HidenFilePath;
		int PointIndex = s.find('.');
		uint8_t* tmpStr = nullptr;
		int charCount = 0;
		if (PointIndex == -1)  //说明原文件名无后缀名
		{
			tmpStr = new uint8_t[2];
			tmpStr[0] = '.';
			tmpStr[1] = '.';
			charCount = 2;
		}
		else
		{
			tmpStr = new uint8_t[s.size() - PointIndex + 1];
			for (int i = PointIndex; i < s.size(); i++)
			{
				tmpStr[i - PointIndex] = s[i];
			}
			tmpStr[s.size() - PointIndex] = '.';
			charCount = s.size() - PointIndex + 1;
		}
		SuffixCount = charCount * 8;
		SuffixArr = new char[SuffixCount];
		StreamConvert sc;
		sc.byteStreamToBinaryString(tmpStr, charCount, SuffixArr, SuffixCount, 0);
		delete[] tmpStr;
	}

	char* BinaryArray = nullptr;
	int BinaryBitCount = 0;
	//获取待加密的二进制流
	{
		BinaryFileSolver fSolver;
		fSolver.ClearBeforeRead();
		fSolver.setInFilePath(fileList.HidenFilePath);
		if (fSolver.LoadFile() != 0)
		{
			//需要完善
			return;
		}
		uint8_t* FileContent = fSolver.GetBinaryFileContent();
		int FileByteCount = fSolver.GetInFileByteCount();
		int FileBitCount = FileByteCount * 8;
		char* BitStream = new char[FileBitCount];
		StreamConvert scv;
		scv.byteStreamToBinaryString(FileContent, FileByteCount, BitStream, FileBitCount, 0);
		delete[] FileContent;
		BinaryArray = BitStream;
		BinaryBitCount = FileBitCount;
	}

	char* FinalBinaryStream = nullptr;
	int FinalBitCount = 0;
	//合并后缀数组和文件内容数组
	{
		FinalBitCount = SuffixCount + BinaryBitCount;
		FinalBinaryStream = new char[FinalBitCount];
		for (int i = 0; i < SuffixCount; i++)
		{
			FinalBinaryStream[i] = SuffixArr[i];
		}
		for (int i = 0; i < BinaryBitCount; i++)
		{
			FinalBinaryStream[i + SuffixCount] = BinaryArray[i];
		}
		delete[] SuffixArr;
		delete[] BinaryArray;
	}
	
	if (fileList.CarrierImagePathList.size() == 1)
	{
		int BCount = GetEffectiveBitsCount(fileList.CarrierImagePathList[0]);		//可修改有效位数

		//计算量表可修改有效位数
		int TEffiBitCount = TableCalEffectiveBitsCount(fileList.CarrierImagePathList[0], TableK);

		int fileBitCount = 0;
		//计算文件大小
		{
			CFileStatus status;
			CString strFile = _T("");
			CA2T szr(fileList.CarrierImagePathList[0].c_str());
			strFile = (LPCTSTR)szr;
			CFile::GetStatus(strFile, status);
			long iSizeOfFile;
			iSizeOfFile = status.m_size;
			fileBitCount = iSizeOfFile * 8;
		}

		int MPayLoad = 0;
		int TPayLoad = 0;
		//计算载体两部分的载荷
		{
			MPayLoad = MatrixCalPayLoad(fileList.CarrierImagePathList[0], MatrixK);
			TPayLoad = floor((double)TEffiBitCount / Table_NBit)*Table_DBit;
		}

		int MBit = 0;
		int TBit = 0;
		//计算各自承担的密文比特数
		{
			double MRate = (double)(MPayLoad) / ((double)MPayLoad + TPayLoad);
			MBit = floor(MRate*fileBitCount);
			TBit = fileBitCount - MBit;
		}

		int MStride = 0;
		//计算步长
		{
			int GroupNum = ceil(MBit / MatrixK);
			int effiBitPerGroup = floor((double)(BCount) / GroupNum);
			MStride = effiBitPerGroup - pow(2, MatrixK) + 1;
		}
		int TStride = 0;
		//计算步长
		{
			int GroupNum = ceil(TBit / Table_DBit);
			int effiBitPerGroup = floor((double)TEffiBitCount / GroupNum);
			TStride = effiBitPerGroup - Table_NBit;
		}
		
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

		//读出量化因子
		jvirt_barray_ptr* coeff_arrays;
		coeff_arrays = jpeg_read_coefficients(&cinfo);

		//写入算法信息头
		WriteAlgHead(coeff_arrays, &cinfo, GetAlgName());

		//写入复合算法头部
		{
			char blockNum = 0;
			char blockCount = 1;
			char Mk = this->MatrixK;
			char Ms = MStride;
			int MEmbMessBitCount = MBit;
			char Tk = this->TableK;
			char Ts = TStride;
			int TEmbMessBitCount = TBit;

			StreamConvert sc;
			char* Stream_BlockNum = nullptr;
			sc.Convert_HalfCharToBinaryStream(blockNum, &Stream_BlockNum);
			char* Stream_BlockCount = nullptr;
			sc.Convert_HalfCharToBinaryStream(blockCount, &Stream_BlockCount);
			char* Stream_Mk = nullptr;
			sc.Convert_HalfCharToBinaryStream(Mk, &Stream_Mk);
			char* Stream_Ms = nullptr;
			sc.Convert_HalfCharToBinaryStream(Ms, &Stream_Ms);
			char* Stream_MEmbMessBitCount = nullptr;
			sc.Convert_IntToBinaryStream(MEmbMessBitCount, &Stream_MEmbMessBitCount);
			char* Stream_Tk = nullptr;
			sc.Convert_HalfCharToBinaryStream(Tk, &Stream_Tk);
			char* Stream_Ts = nullptr;
			sc.Convert_HalfCharToBinaryStream(Ts, &Stream_Ts);
			char* Stream_TEmbMessBitCount = nullptr;
			sc.Convert_IntToBinaryStream(TEmbMessBitCount, &Stream_TEmbMessBitCount);

			int PositionCounter = 0;
			int HideCounter_BlockNum = 0;
			int HideCounter_BlockCount = 0;
			int HideCounter_Mk = 0;
			int HideCounter_Ms = 0;
			int HideCounter_MEmbMessBitCount = 0;
			int HideCounter_Tk = 0;
			int HideCounter_Ts = 0;
			int HideCounter_TEmbMessBitCount = 0;

			for (int ci = 0; ci < 3; ci++)
			{
				JBLOCKARRAY buffer;
				JCOEFPTR blockptr;
				jpeg_component_info* compptr;
				compptr = cinfo.comp_info + ci;
				for (int by = 0; by < compptr->height_in_blocks; by++)
				{
					buffer = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
					for (int bx = 0; bx < compptr->width_in_blocks; bx++)
					{
						blockptr = buffer[by][bx];
						for (int bi = 0; bi < 64; bi++)
						{
							short blockp = blockptr[bi];
							if (blockptr == 0)
								continue;
							if (PositionCounter < ALGHEAD_TYPE_LENGTH)	//跳过8位的算法描述位
							{
								PositionCounter++;
								continue;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM)	//写入分块块号
							{
								switch (Stream_BlockNum[HideCounter_BlockNum])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_BlockNum++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE)		//写入分块总块数
							{
								switch (Stream_BlockNum[HideCounter_BlockCount])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_BlockCount++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K)		//写入矩阵K
							{
								switch (Stream_Mk[HideCounter_Mk])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_Mk++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE)		//写入矩阵步长
							{
								switch (Stream_Ms[HideCounter_Ms])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_Ms++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT)		//写入矩阵嵌入密文数
							{
								switch (Stream_MEmbMessBitCount[HideCounter_MEmbMessBitCount])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_MEmbMessBitCount++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K)		//写入量表K
							{
								switch (Stream_Tk[HideCounter_Tk])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_Tk++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE)		//写入量表步长
							{
								switch (Stream_Ts[HideCounter_Ts])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_Ts++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
								+ COMPOSITE_TABLE_CONTENT)		//写入量表密文位数
							{
								switch (Stream_TEmbMessBitCount[HideCounter_TEmbMessBitCount])
								{
								case '1':
									if (blockp % 2 == 0)
									{
										blockptr[bi] += 1;
									}
									break;
								case '0':
									if (blockp % 2 == 1)
									{
										blockptr[bi] += 1;
									}
									else if (blockp % 2 == -1)
									{
										blockptr[bi] -= 1;
									}
									break;
								default:
									break;
								}
								HideCounter_TEmbMessBitCount++;
								PositionCounter++;
							}
							else if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
								+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
							{
								break;
							}
						}
						if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
							+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
							+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
							+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
						{
							break;
						}
					}
					if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
						+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
						+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
						+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
					{
						break;
					}
				}
				if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
				{
					break;
				}
			}

			delete[] Stream_BlockNum;
			delete[] Stream_BlockCount;
			delete[] Stream_Mk;
			delete[] Stream_Ms;
			delete[] Stream_MEmbMessBitCount;
			delete[] Stream_Tk;
			delete[] Stream_Ts;
			delete[] Stream_TEmbMessBitCount;

		}

		//牛逼呀
		fclose(file);
		FILE* outFile;
		{
			string fName = fileList.NewFilePathList[0];
			const char* p = fName.data();
			fopen_s(&outFile, p, "wb");
		}
		jpeg_compress_struct coutfo;
		jpeg_error_mgr jerr1;
		coutfo.err = jpeg_std_error(&jerr1);
		jpeg_create_compress(&coutfo);
		jpeg_stdio_dest(&coutfo, outFile);
		coutfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;

		coutfo.image_width = cinfo.image_width;
		coutfo.image_height = cinfo.image_height;
		coutfo.input_components = cinfo.num_components;
		coutfo.in_color_space = cinfo.out_color_space;
		jpeg_set_defaults(&coutfo);
		jpeg_copy_critical_parameters(&cinfo, &coutfo);
		jpeg_write_coefficients(&coutfo, coeff_arrays);

		jpeg_finish_compress(&coutfo);
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_compress(&coutfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(outFile);
	}
	else if (fileList.CarrierImagePathList.size() == 0)
	{

	}
	else
	{

	}
}

void HighCapabilityCompositeAlg::ExecuteExtractingAlg()
{
	FileList fileList = this->GetFileList();
	if (fileList.CarrierImagePathList.size() == 1)
	{
		FILE* file;
		{
			string fName = fileList.CarrierImagePathList[0];
			const char* p = fName.data();
			fopen_s(&file, p, "rb");
		}
		jpeg_decompress_struct cinfo;
		jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);
		jpeg_stdio_src(&cinfo, file);
		jpeg_read_header(&cinfo, TRUE);
		cinfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;

		//读出量化因子
		jvirt_barray_ptr* coeff_arrays;
		coeff_arrays = jpeg_read_coefficients(&cinfo);

		//读取算法头部各部分信息
		char blockNum;
		char blockCount;
		char Mk;
		char Ms;
		int MEmbMessBitCount;
		char Tk;
		char Ts;
		int TEmbMessBitCount;
		{
			char* Stream_BlockNum = new char[COMPOSITE_BLOCK_NUM + 1];
			char* Stream_BlockCount = new char[COMPOSITE_BLOCK_COUNTE + 1];
			char* Stream_Mk = new char[COMPOSITE_MATRIX_K + 1];
			char* Stream_Ms = new char[COMPOSITE_MATRIX_STRIDE + 1];
			char* Stream_MEmbMessBitCount = new char[COMPOSITE_MATRIX_CONTENT + 1];
			char* Stream_Tk = new char[COMPOSITE_TABLE_K + 1];
			char* Stream_Ts = new char[COMPOSITE_TABLE_CONTENT + 1];
			char* Stream_TEmbMessBitCount = new char[COMPOSITE_TABLE_LOSE_R];

			int PositionCounter = 0;
			int HideCounter_BlockNum = 0;
			int HideCounter_BlockCount = 0;
			int HideCounter_Mk = 0;
			int HideCounter_Ms = 0;
			int HideCounter_MEmbMessBitCount = 0;
			int HideCounter_Tk = 0;
			int HideCounter_Ts = 0;
			int HideCounter_TEmbMessBitCount = 0;

			for (int ci = 0; ci < 3; ci++)
			{
				JBLOCKARRAY buffer;
				JCOEFPTR blockptr;
				jpeg_component_info* compptr;
				compptr = cinfo.comp_info + ci;
				for (int by = 0; by < compptr->height_in_blocks; by++)
				{
					buffer = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
					for (int bx = 0; bx < compptr->width_in_blocks; bx++)
					{
						blockptr = buffer[by][bx];
						for (int bi = 0; bi < 64; bi++)
						{
							short blockp = blockptr[bi];
							if (blockptr == 0)
								continue;
							if (PositionCounter < ALGHEAD_TYPE_LENGTH)	//跳过8位的算法描述位
							{
								PositionCounter++;
								continue;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM)
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_BlockNum[HideCounter_BlockNum] = '0';
								}
								else
								{
									Stream_BlockNum[HideCounter_BlockNum] = '1';
								}
								HideCounter_BlockNum++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE)		//写入分块总块数
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_BlockCount[HideCounter_BlockCount] = '0';
								}
								else
								{
									Stream_BlockCount[HideCounter_BlockCount] = '1';
								}
								HideCounter_BlockCount++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K)		//写入矩阵K
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_Mk[HideCounter_Mk] = '0';
								}
								else
								{
									Stream_Mk[HideCounter_Mk] = '1';
								}
								HideCounter_Mk++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE)		//写入矩阵步长
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_Ms[HideCounter_Ms] = '0';
								}
								else
								{
									Stream_Ms[HideCounter_Ms] = '1';
								}
								HideCounter_Ms++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT)		//写入矩阵嵌入密文数
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_MEmbMessBitCount[HideCounter_MEmbMessBitCount] = '0';
								}
								else
								{
									Stream_MEmbMessBitCount[HideCounter_MEmbMessBitCount] = '1';
								}
								HideCounter_MEmbMessBitCount++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K)		//写入量表K
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_Tk[HideCounter_Tk] = '0';
								}
								else
								{
									Stream_Tk[HideCounter_Tk] = '1';
								}
								HideCounter_Tk++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE)		//写入量表步长
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_Ts[HideCounter_Ts] = '0';
								}
								else
								{
									Stream_Ts[HideCounter_Ts] = '1';
								}
								HideCounter_Ts++;
								PositionCounter++;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
								+ COMPOSITE_TABLE_CONTENT)		//写入量表密文位数
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_TEmbMessBitCount[HideCounter_TEmbMessBitCount] = '0';
								}
								else
								{
									Stream_TEmbMessBitCount[HideCounter_TEmbMessBitCount] = '1';
								}
								HideCounter_TEmbMessBitCount++;
								PositionCounter++;
							}
							else if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
								+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
							{
								break;
							}
						}
						if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
							+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
							+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
							+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
						{
							break;
						}
					}
					if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
						+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
						+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
						+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
					{
						break;
					}
				}
				if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
				{
					break;
				}
			}

			StreamConvert sc;
			blockNum = sc.Convert_BinaryStreamToHalfChar(Stream_BlockNum);
			char blockCount = sc.Convert_BinaryStreamToHalfChar(Stream_BlockCount);
			char Mk = sc.Convert_BinaryStreamToHalfChar(Stream_Mk);
			char Ms = sc.Convert_BinaryStreamToHalfChar(Stream_Ms);
			int MEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_MEmbMessBitCount);
			char Tk = sc.Convert_BinaryStreamToHalfChar(Stream_Tk);
			char Ts = sc.Convert_BinaryStreamToHalfChar(Stream_Ts);
			int TEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_TEmbMessBitCount);

			CString str_BlockNum;
			CString str_BlockCount;
			CString str_Mk;
			CString str_Ms;
			CString str_MEmbMessBitCount;
			CString str_Tk;
			CString str_Ts;
			CString str_TEmbMessBitCount;

			str_BlockNum.Format(_T("%d"), (int)blockNum);
			str_BlockCount.Format(_T("%d"), (int)blockCount);
			str_Mk.Format(_T("%d"), (int)Mk);
			str_Ms.Format(_T("%d"), (int)Ms);
			str_MEmbMessBitCount.Format(_T("%d"), MEmbMessBitCount);
			str_Tk.Format(_T("%d"), (int)Tk);
			str_Ts.Format(_T("%d"), (int)Ts);
			str_TEmbMessBitCount.Format(_T("%d"), TEmbMessBitCount);

			CString sb = str_BlockNum + _T(" ") + str_BlockCount + _T(" ") + str_Mk + _T(" ") + str_Ms + _T(" ") + str_MEmbMessBitCount 
				+ _T(" ") + str_Tk + _T(" ") + str_Ts + _T(" ") + str_TEmbMessBitCount;
			ShowMessage(sb);

			delete[] Stream_BlockNum;
			delete[] Stream_BlockCount;
			delete[] Stream_Mk;
			delete[] Stream_Ms;
			delete[] Stream_MEmbMessBitCount;
			delete[] Stream_Tk;
			delete[] Stream_Ts;
			delete[] Stream_TEmbMessBitCount;
		}

		fclose(file);
	}
	else if (fileList.CarrierImagePathList.size() == 0)
	{

	}
	else
	{

	}
}

int HighCapabilityCompositeAlg::CalPayLoad(string filePath)
{
	return 0;
}

int HighCapabilityCompositeAlg::CalPayLoads(FileList * fileList)
{
	return 0;
}

ALGORITHM_NAME HighCapabilityCompositeAlg::GetAlgName()
{
	return ALG_COMPOSITE;
}

int HighCapabilityCompositeAlg::MatrixCalPayLoad(string filePath, int K)		//返回能嵌入的密文的位数
{
	int BCount = GetEffectiveBitsCount(filePath);
	int BitsPerGroup = (int)(pow(2, K) - 1);
	int GroupCount = (int)floor(BCount*1.0 / BitsPerGroup);
	return GroupCount*K;
}

int HighCapabilityCompositeAlg::TableCalEffectiveBitsCount(string filePath, int K)		//必须是在filePath被打开之前使用，否则会出现占用错误
{
	FILE* file;
	{
		const char* p = filePath.data();
		fopen_s(&file, p, "rb");
		rewind(file);
	}
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, FALSE);
	cinfo.mem->max_memory_to_use = 10000000;
	//InitDecompressInfo(&cinfo, file);
	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(&cinfo);
	int HideCounter = 0;
	int EBlockCounter = 0;		//每个块最多选择12位，用来控制12位嵌入
	for (int ci = 0; ci < 3; ci++)
	{
		JBLOCKARRAY buffer;
		JCOEFPTR blockptr;
		jpeg_component_info* compptr;
		compptr = cinfo.comp_info + ci;
		for (int by = 0; by < compptr->height_in_blocks; by++)
		{
			buffer = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			{
				blockptr = buffer[by][bx];
				EBlockCounter = 0;
				for (int bi = 0; bi < 64; bi++)
				{
					short blockp = blockptr[bi];
					//CString caonima;
					//caonima.Format(_T("%d", blockp));
					//ShowMessage(caonima);
					if (blockptr == 0)
						continue;
					if (EBlockCounter < 12)
					{
						EBlockCounter++;
					}
					else
					{
						EBlockCounter = 0;
						break;
					}
					if (blockp >= K)
						HideCounter++;
				}
			}
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(file);
	HideCounter -= ALGHEAD_TYPE_LENGTH;
	HideCounter = HideCounter - COMPOSITE_BLOCK_NUM - COMPOSITE_BLOCK_COUNTE - COMPOSITE_MATRIX_K - COMPOSITE_MATRIX_STRIDE
		- COMPOSITE_MATRIX_CONTENT - COMPOSITE_TABLE_K - COMPOSITE_TABLE_STRIDE - COMPOSITE_TABLE_CONTENT - COMPOSITE_TABLE_LOSE_R;
	return HideCounter;
}

jvirt_barray_ptr* HighCapabilityCompositeAlg::MatrixCodeEmbed(jvirt_barray_ptr * coeff_arrays, jpeg_decompress_struct* cinfo)
{

}

jvirt_barray_ptr* HighCapabilityCompositeAlg::TableEmbed(jvirt_barray_ptr * coeff_arrays, jpeg_decompress_struct* cinfo)
{

}

int HighCapabilityCompositeAlg::CalEffectiveBitsCount(FILE * file)
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, FALSE);
	//InitDecompressInfo(&cinfo, file);
	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(&cinfo);
	int HideCounter = 0;
	for (int ci = 0; ci < 3; ci++)
	{
		JBLOCKARRAY buffer;
		JCOEFPTR blockptr;
		jpeg_component_info* compptr;
		compptr = cinfo.comp_info + ci;
		for (int by = 0; by < compptr->height_in_blocks; by++)
		{
			buffer = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			{
				blockptr = buffer[by][bx];
				for (int bi = 0; bi < 64; bi++)
				{
					short blockp = blockptr[bi];
					if (blockptr == 0)
						continue;
					HideCounter++;
				}
			}
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	//FinishDecompress(&cinfo);
	HideCounter -= ALGHEAD_TYPE_LENGTH;
	HideCounter = HideCounter - COMPOSITE_BLOCK_NUM - COMPOSITE_BLOCK_COUNTE - COMPOSITE_MATRIX_K - COMPOSITE_MATRIX_STRIDE
		- COMPOSITE_MATRIX_CONTENT - COMPOSITE_TABLE_K - COMPOSITE_TABLE_STRIDE - COMPOSITE_TABLE_CONTENT - COMPOSITE_TABLE_LOSE_R;
	return HideCounter;
}

int HighCapabilityCompositeAlg::GetEffectiveBitsCount(string filePath)
{
	int index = -1;
	FileList fL = this->GetFileList();
	for (int i = 0; i < fL.CarrierImagePathList.size(); i++)
	{
		if (filePath == fL.CarrierImagePathList[i])
		{
			index = i;
			break;
		}
	}
	return this->EffectiveBitsCountVec[index];
}

void HighCapabilityCompositeAlg::ShowMessage(CString str)
{
	CMainFrame* pMainfram = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	MessageBox(pMainfram->GetSafeHwnd(), str, _T("123"), 1);
}