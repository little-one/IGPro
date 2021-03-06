#include "stdafx.h"
#include "HighCapabilityCompositeAlg.h"
#include "MainFrm.h"
#include <fstream>
#include <math.h>
#include <map>

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

void HighCapabilityCompositeAlg::test()
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

	//ShowMessage(CharToCString(FinalBinaryStream, 16));

	if (fileList.CarrierImagePathList.size() == 1)
	{
		int BCount = GetEffectiveBitsCount(fileList.CarrierImagePathList[0]);		//可修改有效位数
		//ShowMessage(BCount);
		//计算量表可修改有效位数
		int TEffiBitCount = TableCalEffectiveBitsCount(fileList.CarrierImagePathList[0], TableK);
		//ShowMessage(TEffiBitCount);
		int fileBitCount = 0;
		//计算文件大小
		fileBitCount = FinalBitCount;

		int MPayLoad = 0;
		int TPayLoad = 0;
		
		//计算载体两部分的载荷
		{
			MPayLoad = MatrixCalPayLoad(fileList.CarrierImagePathList[0], MatrixK);
			double tt = (double)TEffiBitCount / Table_NBit;
			double ft = floor(tt);
			TPayLoad = ft*Table_DBit / 3;
		}

		int MBit = 0;
		int TBit = 0;
		//计算各自承担的密文比特数，自动缩放为8的倍数
		{
			double MRate = (double)(MPayLoad) / ((double)MPayLoad + TPayLoad);
			MBit = floor(MRate*fileBitCount);
			if (MBit % 8 != 0)
				MBit += (8 - MBit % 8);
			TBit = fileBitCount - MBit;
		}
		//ShowMessage(MBit);
		//ShowMessage(TBit);

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

		//计算24个r的值
		int Y_Tr[12];
		int C_Tr[12];
		{
			JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
			int offset = 0;
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				Y_Tr[offset] = qTbl->quantval[j] - this->TableK * floor((double)(qTbl->quantval[j]) / (this->TableK));
				offset++;
			}
			qTbl = cinfo.quant_tbl_ptrs[1];
			offset = 0;
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				C_Tr[offset] = qTbl->quantval[j] - this->TableK * floor((double)(qTbl->quantval[j]) / (this->TableK));
				offset++;
			}
		}

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
			sc.Convert_CharToBinaryStream(Ms, &Stream_Ms);
			char* Stream_MEmbMessBitCount = nullptr;
			sc.Convert_IntToBinaryStream(MEmbMessBitCount, &Stream_MEmbMessBitCount);
			char* Stream_Tk = nullptr;
			sc.Convert_HalfCharToBinaryStream(Tk, &Stream_Tk);
			char* Stream_Ts = nullptr;
			sc.Convert_CharToBinaryStream(Ts, &Stream_Ts);
			char* Stream_TEmbMessBitCount = nullptr;
			sc.Convert_IntToBinaryStream(TEmbMessBitCount, &Stream_TEmbMessBitCount);	

			//将24个r处理成4*24的字符然后统一重组成一个字符串
			char* Stream_Tr = nullptr;
			{
				char** sTr = &Stream_Tr;
				*sTr = new char[COMPOSITE_TABLE_LOSE_R + 1];
				(*sTr)[COMPOSITE_TABLE_LOSE_R] = '\0';
				int offset = 0;

				char* t_ptr = nullptr;
				for (int i = 0; i < 12; i++)
				{
					int tr = Y_Tr[i];
					sc.Convert_IntToKStream(tr, 4, &t_ptr);
					for (int j = 0; j < 4; j++)
					{
						(*sTr)[offset] = t_ptr[j];
						offset++;
					}
				}
				for (int i = 0; i < 12; i++)
				{
					int tr = C_Tr[i];
					sc.Convert_IntToKStream(tr, 4, &t_ptr);
					for (int j = 0; j < 4; j++)
					{
						(*sTr)[offset] = t_ptr[j];
						offset++;
					}
				}
				delete[] t_ptr;
			}

			int PositionCounter = 0;
			int HideCounter_BlockNum = 0;
			int HideCounter_BlockCount = 0;
			int HideCounter_Mk = 0;
			int HideCounter_Ms = 0;
			int HideCounter_MEmbMessBitCount = 0;
			int HideCounter_Tk = 0;
			int HideCounter_Ts = 0;
			int HideCounter_TEmbMessBitCount = 0;
			int HideCounter_Tr = 0;

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
							if (blockp == 0)
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
								switch (Stream_BlockCount[HideCounter_BlockCount])
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
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
								+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
							{
								switch (Stream_Tr[HideCounter_Tr])
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
								HideCounter_Tr++;
								PositionCounter++;
							}
							else
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
			delete[] Stream_Tr;

		}

		//vector<int> MTestArray;
		//int MTestCounter = 0;
		//M算法嵌入
		bool breakFlg = false;		//结束标识
		{
			bool strideFlg = false;

			int dropCounter = 0;
			int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
				+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
				+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
				+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

			int HideCounter = 0;		//密文计数器，用来记录当前记录的密文指针
			int GroupCounter = 0;		//控制组采集，数据采集够一组之后就置零
			int StrideCounter = 0;		//控制步长跳跃

			int PGroupBitCount_PayLoad = pow(2, this->MatrixK) - 1;		//M算法中每组载体图像的位数  k位密文需要 2^k-1位嵌入
			JCOEFPTR* SrcDataGroup = (JCOEFPTR*)(cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_IMAGE, sizeof(JCOEFPTR) * PGroupBitCount_PayLoad);
			char* SecretBitGroup = nullptr;
			char* ZOSrcBitGroup = nullptr;

			//int HideBitFinalTest = 0;
			//int TotalKexiugaiweiTest = 0;

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
							if (blockp == 0)
								continue;
							//TotalKexiugaiweiTest++;
							//跳头
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //进入数据区
							{
								if (!strideFlg)				//当前为正常写入状态
								{
									SrcDataGroup[GroupCounter] = &(blockptr[bi]);	//进行组采集
									GroupCounter++;

									//组采集完毕，对该组数据进行嵌入操作并将组计数器置零
									if (GroupCounter == PGroupBitCount_PayLoad)
									{

										if (!GetSecretBitGroup(&SecretBitGroup, this->MatrixK, FinalBinaryStream, MBit, HideCounter))		//未发生填充，即剩余密文多于K个
										{
											HideCounter += this->MatrixK;
											//HideBitFinalTest = HideCounter;
										}
										else
										{
											breakFlg = true;
										}

										//处理该组密文
										{
											GetZOStreamGroup(&ZOSrcBitGroup, SrcDataGroup, PGroupBitCount_PayLoad);		//先将载体处理为01序列

																														//矩阵编码
											int position = MCoding(SecretBitGroup, ZOSrcBitGroup, this->MatrixK);


											if (position > -1)		//=-1时不需要修改，否则修改相应的位置
											{
												if (ZOSrcBitGroup[position] == '1')		//说明该位是正奇位
												{
													*(SrcDataGroup[position]) = *(SrcDataGroup[position]) + 1;
												}
												else if (ZOSrcBitGroup[position] == '2')		//说明该位是负奇位
												{
													*(SrcDataGroup[position]) = *(SrcDataGroup[position]) - 1;
												}
												else if (ZOSrcBitGroup[position] == '0')		//说明该位是偶数位
												{
													*(SrcDataGroup[position]) = *(SrcDataGroup[position]) + 1;
												}
											}

										}

										GroupCounter = 0;	//组采集计数器置零
										delete[] SecretBitGroup;
										delete[] ZOSrcBitGroup;
										strideFlg = true;
									}
									if (breakFlg)
										break;
								}
								else           //嵌入一个密文分组之后，跳过stride个步长
								{
									if (StrideCounter >= MStride - 1)
									{
										strideFlg = false;
										StrideCounter = 0;
									}
									else
									{
										StrideCounter++;
									}
								}
							}
							if (breakFlg)
								break;
						}
						if (breakFlg)
							break;
					}
					if (breakFlg)
						break;
				}
				if (breakFlg)
					break;
			}
			//ShowMessage(HideCounter);
			//ShowMessage(MBit);
		}

		//T算法嵌入
		bool TbreakFlg = false;
		{
			int Cbl[2][64];
			{
				JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
				for (int i = 0; i < 64; i++)
				{
					Cbl[0][i] = qTbl->quantval[i];
				}
				qTbl = cinfo.quant_tbl_ptrs[1];
				for (int i = 0; i < 64; i++)
				{
					Cbl[1][i] = qTbl->quantval[i];
				}
			}
			int Tbl[2][64];
			{
				JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
				for (int i = 0; i < 64; i++)
				{
					Tbl[0][i] = qTbl->quantval[i];
				}
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					Tbl[0][j] = floor((double)(qTbl->quantval[j]) / this->TableK);
				}
				qTbl = cinfo.quant_tbl_ptrs[1];
				for (int i = 0; i < 64; i++)
				{
					Tbl[1][i] = qTbl->quantval[i];
				}
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					Tbl[1][j] = floor((double)(qTbl->quantval[j]) / this->TableK);
				}
			}

			bool strideFlg = false;

			//int blockCounter = 0;		//块内计数器，记录当前访问到块内的位置，63之后置零
			int FileStreamCounter = MBit;			//记录T算法加密的bit数
			//int FilePositionCounter = MBit;		//密文计数器，从M算法的末尾开始继续加密
			int strideCounter = 0;		//跳步计数器
			int dropCounter = 0;
			int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
				+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
				+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
				+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

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
							if (blockp == 0)
								continue;
							//ShowMessage(dropBit);
							//跳头
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //进入数据区
							{

								//ShowMessage(TStride);
								if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)		
								{
									int loss = 0;
									int oTbl = 0;
									//int oldQ = 0;
									if (ci == 0)
									{
										loss = Y_Tr[bi - R_STARTPOSITION];
										oTbl = Tbl[0][bi];
										//oldQ = Cbl[0][bi];
									}
									else
									{
										loss = C_Tr[bi - R_STARTPOSITION];
										oTbl = Tbl[1][bi];
										//oldQ = Cbl[1][bi];
									}
									double lb = (double)loss*blockptr[bi];
									double to = (double)this->TableK*oTbl;
									int suffex = this->TableK * round(lb / to);
									//blockptr[bi] = this->TableK * blockptr[bi];
									blockptr[bi] = this->TableK * blockptr[bi] + suffex;

								}
							}
						}
					}
				}
			}
			
			dropCounter = 0;
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
							if (blockp == 0)
								continue;
							//ShowMessage(dropBit);
							//跳头
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //进入数据区
							{
								if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)
								{
									if (!strideFlg)
									{
										char fC = FinalBinaryStream[FileStreamCounter];
										if (fC == '1')		//为0时不采取任何操作
										{
											if (blockptr[bi] >0)
												blockptr[bi] = blockptr[bi] + 1;
											else
												blockptr[bi] = blockptr[bi] - 1;
										}

										FileStreamCounter++;
										strideFlg = true;

										if (FileStreamCounter >= MBit + TBit)
										{
											TbreakFlg = true;
										}
									}
									else      //跳步
									{
										if (strideCounter >= TStride - 1)
										{
											strideFlg = false;
											strideCounter = 0;
										}
										else
										{
											strideCounter++;
										}
									}
									//FileStreamCounter++;
								}
							}
							if (TbreakFlg)
								break;
						}
						if (TbreakFlg)
							break;
					}
					if (TbreakFlg)
						break;
				}
				if (TbreakFlg)
					break;
			}
		}
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
		//修改量化表
		{
			JQUANT_TBL* qTbl = coutfo.quant_tbl_ptrs[0];
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				qTbl->quantval[j] = floor((double)(qTbl->quantval[j]) / this->TableK);
			}
			qTbl = cinfo.quant_tbl_ptrs[1];
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				qTbl->quantval[j] = floor((double)(qTbl->quantval[j]) / this->TableK);
			}
		}
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
		//计算每个载体各自的MT区域有效载荷
		vector<int> MEffectiveBitCountVec;		//M算法可修改位数
		vector<int> TEffectiveBitCountVec;		//T算法可修改位数
		for (auto i = fileList.CarrierImagePathList.begin(); i != fileList.CarrierImagePathList.end(); i++)
		{
			int MBcount = GetEffectiveBitsCount(*i);
			MEffectiveBitCountVec.push_back(MBcount);
			int TBcount = TableCalEffectiveBitsCount(*i, TableK);
			TEffectiveBitCountVec.push_back(TBcount);
		}

		//计算每个载体负责的总比特数
		vector<int> FileBitCountVec;
		{
			int totalBit = 0;

			//计算每个载体的总载荷
			vector<int> tempVec;
			for (int i = 0; i < fileList.CarrierImagePathList.size(); i++)
			{
				int tmp = MEffectiveBitCountVec[i] + TEffectiveBitCountVec[i];
				tempVec.push_back(tmp);
				totalBit += tmp;
			}
			
			int tmpsum = 0;
			//计算前N-1个载体各自需要负责的比特数
			for (int i = 0; i < fileList.CarrierImagePathList.size()-1; i++)	
			{
				double rate = (double)tempVec[i] / (double)totalBit;
				int fileBit = ceil(rate * FinalBitCount);
				FileBitCountVec.push_back(fileBit);
				tmpsum += fileBit;
			}
			FileBitCountVec.push_back(FinalBitCount - tmpsum);		//最后一个载体所需要负责的比特数
		}

		{
			ofstream of;
			of.open("F:\\test\\before.txt");
			for (int i = 0; i < FinalBitCount; i++)
			{
				of << FinalBinaryStream[i];
			}
			of.close();
		}

		char* FilePosition = FinalBinaryStream;		//文件流访问指针，记录当前访问的二进制流的位置
		char GloBlockNum = 0;		
		char GloBlockCount = fileList.CarrierImagePathList.size();

		for (int i = 0; i < fileList.CarrierImagePathList.size(); i++)
		{
			//第 i 个载媒负责的密文比特数
			int fileBitCount = FileBitCountVec[i];

			//第 i 个载媒的各算法区的有效载荷
			int MPayLoad = MEffectiveBitCountVec[i];
			int TPayLoad = TEffectiveBitCountVec[i];

			//第 i 个载媒的各算法区负责的密文比特数，自动缩放为8的倍数
			int MBit = 0;
			int TBit = 0;
			{
				double MRate = (double)(MPayLoad) / ((double)MPayLoad + TPayLoad);
				MBit = floor(MRate*fileBitCount);
				if (MBit % 8 != 0)
					MBit += (8 - MBit % 8);
				TBit = fileBitCount - MBit;
			}
			
			//第 i 个载媒的个算法区的嵌入步长
			int MStride = 0;
			{
				int GroupNum = ceil(MBit / MatrixK);
				int effiBitPerGroup = floor((double)(MPayLoad) / GroupNum);
				MStride = effiBitPerGroup - pow(2, MatrixK) + 1;
			}
			int TStride = 0;
			{
				int GroupNum = ceil(TBit / Table_DBit);
				int effiBitPerGroup = floor((double)TPayLoad / GroupNum);
				TStride = effiBitPerGroup - Table_NBit;
			}

			//CString str;
			//CString strFileBitCount;
			//CString strMpayload;
			//CString strTpayload;
			//CString strMBit;
			//CString strTBit;
			//CString strMStride;
			//CString strTStride;
			//strFileBitCount.Format(_T("%d"), fileBitCount);
			//strMpayload.Format(_T("%d"), MPayLoad);
			//strTpayload.Format(_T("%d"), TPayLoad);
			//strMBit.Format(_T("%d"), MBit);
			//strTBit.Format(_T("%d"), TBit);
			//strMStride.Format(_T("%d"), MStride);
			//strTStride.Format(_T("%d"), TStride);
			//str = _T("FileBitCount: ") + strFileBitCount + _T(" MPayLoad: ") + strMpayload + _T(" TPayLoad: ") + strTpayload + _T(" MBit: ") + strMBit + _T(" TBit: ") + strTBit + _T(" MStride: ") + strMStride + _T(" TStride: ") + strTStride;
			//ShowMessage(str);

			//第 i 个载媒负责的密文
			char* SecretBitStream = new char[fileBitCount];
			{
				for (int i = 0; i < fileBitCount; i++)
				{
					SecretBitStream[i] = (*FilePosition);
					FilePosition++;
				}
			}

			FILE* file;
			{
				string fName = fileList.CarrierImagePathList[i];
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

			//计算24个r的值
			int Y_Tr[12];
			int C_Tr[12];
			{
				JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
				int offset = 0;
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					Y_Tr[offset] = qTbl->quantval[j] - this->TableK * floor((double)(qTbl->quantval[j]) / (this->TableK));
					offset++;
				}
				qTbl = cinfo.quant_tbl_ptrs[1];
				offset = 0;
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					C_Tr[offset] = qTbl->quantval[j] - this->TableK * floor((double)(qTbl->quantval[j]) / (this->TableK));
					offset++;
				}
			}

			//写入复合算法头部
			{
				char blockNum = GloBlockNum;
				char blockCount = GloBlockCount;
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
				sc.Convert_CharToBinaryStream(Ms, &Stream_Ms);
				char* Stream_MEmbMessBitCount = nullptr;
				sc.Convert_IntToBinaryStream(MEmbMessBitCount, &Stream_MEmbMessBitCount);
				char* Stream_Tk = nullptr;
				sc.Convert_HalfCharToBinaryStream(Tk, &Stream_Tk);
				char* Stream_Ts = nullptr;
				sc.Convert_CharToBinaryStream(Ts, &Stream_Ts);
				char* Stream_TEmbMessBitCount = nullptr;
				sc.Convert_IntToBinaryStream(TEmbMessBitCount, &Stream_TEmbMessBitCount);

				//将24个r处理成4*24的字符然后统一重组成一个字符串
				char* Stream_Tr = nullptr;
				{
					char** sTr = &Stream_Tr;
					*sTr = new char[COMPOSITE_TABLE_LOSE_R + 1];
					(*sTr)[COMPOSITE_TABLE_LOSE_R] = '\0';
					int offset = 0;

					char* t_ptr = nullptr;
					for (int i = 0; i < 12; i++)
					{
						int tr = Y_Tr[i];
						sc.Convert_IntToKStream(tr, 4, &t_ptr);
						for (int j = 0; j < 4; j++)
						{
							(*sTr)[offset] = t_ptr[j];
							offset++;
						}
					}
					for (int i = 0; i < 12; i++)
					{
						int tr = C_Tr[i];
						sc.Convert_IntToKStream(tr, 4, &t_ptr);
						for (int j = 0; j < 4; j++)
						{
							(*sTr)[offset] = t_ptr[j];
							offset++;
						}
					}
					delete[] t_ptr;
				}

				int PositionCounter = 0;
				int HideCounter_BlockNum = 0;
				int HideCounter_BlockCount = 0;
				int HideCounter_Mk = 0;
				int HideCounter_Ms = 0;
				int HideCounter_MEmbMessBitCount = 0;
				int HideCounter_Tk = 0;
				int HideCounter_Ts = 0;
				int HideCounter_TEmbMessBitCount = 0;
				int HideCounter_Tr = 0;

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
								if (blockp == 0)
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
									switch (Stream_BlockCount[HideCounter_BlockCount])
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
								else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
									+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
									+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
									+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
								{
									switch (Stream_Tr[HideCounter_Tr])
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
									HideCounter_Tr++;
									PositionCounter++;
								}
								else
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
				delete[] Stream_Tr;
			}

			//M算法嵌入
			bool breakFlg = false;		//结束标识
			{
				bool strideFlg = false;

				int dropCounter = 0;
				int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

				int HideCounter = 0;		//密文计数器，用来记录当前记录的密文指针
				int GroupCounter = 0;		//控制组采集，数据采集够一组之后就置零
				int StrideCounter = 0;		//控制步长跳跃

				int PGroupBitCount_PayLoad = pow(2, this->MatrixK) - 1;		//M算法中每组载体图像的位数  k位密文需要 2^k-1位嵌入
				JCOEFPTR* SrcDataGroup = (JCOEFPTR*)(cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_IMAGE, sizeof(JCOEFPTR) * PGroupBitCount_PayLoad);
				char* SecretBitGroup = nullptr;
				char* ZOSrcBitGroup = nullptr;
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
								if (blockp == 0)
									continue;
								//TotalKexiugaiweiTest++;
								//跳头
								if (dropCounter < dropBit)
								{
									dropCounter++;
									continue;
								}
								else        //进入数据区
								{
									if (!strideFlg)				//当前为正常写入状态
									{
										SrcDataGroup[GroupCounter] = &(blockptr[bi]);	//进行组采集
										GroupCounter++;

										//组采集完毕，对该组数据进行嵌入操作并将组计数器置零
										if (GroupCounter == PGroupBitCount_PayLoad)
										{

											if (!GetSecretBitGroup(&SecretBitGroup, this->MatrixK, SecretBitStream, MBit, HideCounter))		//未发生填充，即剩余密文多于K个
											{
												HideCounter += this->MatrixK;
												//HideBitFinalTest = HideCounter;
											}
											else
											{
												breakFlg = true;
											}

											//处理该组密文
											{
												GetZOStreamGroup(&ZOSrcBitGroup, SrcDataGroup, PGroupBitCount_PayLoad);		//先将载体处理为01序列

												//矩阵编码
												int position = MCoding(SecretBitGroup, ZOSrcBitGroup, this->MatrixK);


												if (position > -1)		//=-1时不需要修改，否则修改相应的位置
												{
													if (ZOSrcBitGroup[position] == '1')		//说明该位是正奇位
													{
														*(SrcDataGroup[position]) = *(SrcDataGroup[position]) + 1;
													}
													else if (ZOSrcBitGroup[position] == '2')		//说明该位是负奇位
													{
														*(SrcDataGroup[position]) = *(SrcDataGroup[position]) - 1;
													}
													else if (ZOSrcBitGroup[position] == '0')		//说明该位是偶数位
													{
														*(SrcDataGroup[position]) = *(SrcDataGroup[position]) + 1;
													}
												}

											}

											GroupCounter = 0;	//组采集计数器置零
											delete[] SecretBitGroup;
											delete[] ZOSrcBitGroup;
											strideFlg = true;
										}
										if (breakFlg)
											break;
									}
									else           //嵌入一个密文分组之后，跳过stride个步长
									{
										if (StrideCounter >= MStride - 1)
										{
											strideFlg = false;
											StrideCounter = 0;
										}
										else
										{
											StrideCounter++;
										}
									}
								}
								if (breakFlg)
									break;
							}
							if (breakFlg)
								break;
						}
						if (breakFlg)
							break;
					}
					if (breakFlg)
						break;
				}
			}

			//T算法嵌入
			bool TbreakFlg = false;
			{
				int Cbl[2][64];
				{
					JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
					for (int i = 0; i < 64; i++)
					{
						Cbl[0][i] = qTbl->quantval[i];
					}
					qTbl = cinfo.quant_tbl_ptrs[1];
					for (int i = 0; i < 64; i++)
					{
						Cbl[1][i] = qTbl->quantval[i];
					}
				}
				int Tbl[2][64];
				{
					JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
					for (int i = 0; i < 64; i++)
					{
						Tbl[0][i] = qTbl->quantval[i];
					}
					for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
					{
						Tbl[0][j] = floor((double)(qTbl->quantval[j]) / this->TableK);
					}
					qTbl = cinfo.quant_tbl_ptrs[1];
					for (int i = 0; i < 64; i++)
					{
						Tbl[1][i] = qTbl->quantval[i];
					}
					for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
					{
						Tbl[1][j] = floor((double)(qTbl->quantval[j]) / this->TableK);
					}
				}

				bool strideFlg = false;

				//int blockCounter = 0;		//块内计数器，记录当前访问到块内的位置，63之后置零
				int FileStreamCounter = MBit;			//记录T算法加密的bit数
														//int FilePositionCounter = MBit;		//密文计数器，从M算法的末尾开始继续加密
				int strideCounter = 0;		//跳步计数器
				int dropCounter = 0;
				int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

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
								if (blockp == 0)
									continue;
								//ShowMessage(dropBit);
								//跳头
								if (dropCounter < dropBit)
								{
									dropCounter++;
									continue;
								}
								else        //进入数据区
								{
									if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)
									{
										int loss = 0;
										int oTbl = 0;
										//int oldQ = 0;
										if (ci == 0)
										{
											loss = Y_Tr[bi - R_STARTPOSITION];
											oTbl = Tbl[0][bi];
										}
										else
										{
											loss = C_Tr[bi - R_STARTPOSITION];
											oTbl = Tbl[1][bi];
										}
										double lb = (double)loss*blockptr[bi];
										double to = (double)this->TableK*oTbl;
										int suffex = this->TableK * round(lb / to);
										blockptr[bi] = this->TableK * blockptr[bi] + suffex;

									}
								}
							}
						}
					}
				}

				dropCounter = 0;
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
								if (blockp == 0)
									continue;
								//跳头
								if (dropCounter < dropBit)
								{
									dropCounter++;
									continue;
								}
								else        //进入数据区
								{
									if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)
									{
										if (!strideFlg)
										{
											char fC = SecretBitStream[FileStreamCounter];
											if (fC == '1')		//为0时不采取任何操作
											{
												if (blockptr[bi] >0)
													blockptr[bi] = blockptr[bi] + 1;
												else
													blockptr[bi] = blockptr[bi] - 1;
											}

											FileStreamCounter++;
											strideFlg = true;

											if (FileStreamCounter >= MBit + TBit)
											{
												TbreakFlg = true;
											}
										}
										else      //跳步
										{
											if (strideCounter >= TStride - 1)
											{
												strideFlg = false;
												strideCounter = 0;
											}
											else
											{
												strideCounter++;
											}
										}
									}
								}
								if (TbreakFlg)
									break;
							}
							if (TbreakFlg)
								break;
						}
						if (TbreakFlg)
							break;
					}
					if (TbreakFlg)
						break;
				}
			}
			fclose(file);

			//将带有秘密信息的新载荷输出
			FILE* outFile;
			{
				string fName = fileList.NewFilePathList[i];
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
			//修改量化表
			{
				JQUANT_TBL* qTbl = coutfo.quant_tbl_ptrs[0];
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					qTbl->quantval[j] = floor((double)(qTbl->quantval[j]) / this->TableK);
				}
				qTbl = cinfo.quant_tbl_ptrs[1];
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					qTbl->quantval[j] = floor((double)(qTbl->quantval[j]) / this->TableK);
				}
			}
			jpeg_write_coefficients(&coutfo, coeff_arrays);

			jpeg_finish_compress(&coutfo);
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_compress(&coutfo);
			jpeg_destroy_decompress(&cinfo);
			fclose(outFile);

			GloBlockNum += 1;
		}
	}


	delete[] FinalBinaryStream;
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
		char blockNum=0;
		char blockCount=0;
		char Mk=0;
		char Ms=0;
		int MEmbMessBitCount=0;
		char Tk=0;
		char Ts=0;
		int TEmbMessBitCount=0;
		int Y_Tr[12];
		int C_Tr[12];
		{
			char* Stream_BlockNum = new char[COMPOSITE_BLOCK_NUM + 1];
			char* Stream_BlockCount = new char[COMPOSITE_BLOCK_COUNTE + 1];
			char* Stream_Mk = new char[COMPOSITE_MATRIX_K + 1];
			char* Stream_Ms = new char[COMPOSITE_MATRIX_STRIDE + 1];
			char* Stream_MEmbMessBitCount = new char[COMPOSITE_MATRIX_CONTENT + 1];
			char* Stream_Tk = new char[COMPOSITE_TABLE_K + 1];
			char* Stream_Ts = new char[COMPOSITE_TABLE_STRIDE + 1];
			char* Stream_TEmbMessBitCount = new char[COMPOSITE_TABLE_CONTENT + 1];
			char* Stream_Tr = new char[COMPOSITE_TABLE_LOSE_R + 1];

			int PositionCounter = 0;
			int HideCounter_BlockNum = 0;
			int HideCounter_BlockCount = 0;
			int HideCounter_Mk = 0;
			int HideCounter_Ms = 0;
			int HideCounter_MEmbMessBitCount = 0;
			int HideCounter_Tk = 0;
			int HideCounter_Ts = 0;
			int HideCounter_TEmbMessBitCount = 0;
			int HideCounter_Tr = 0;
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
							if (blockp == 0)
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
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
								+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									Stream_Tr[HideCounter_Tr] = '0';
								}
								else
								{
									Stream_Tr[HideCounter_Tr] = '1';
								}
								HideCounter_Tr++;
								PositionCounter++;
							}
							else
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
			blockCount = sc.Convert_BinaryStreamToHalfChar(Stream_BlockCount);
			Mk = sc.Convert_BinaryStreamToHalfChar(Stream_Mk);
			Ms = sc.Convert_BinaryStreamToChar(Stream_Ms);
			MEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_MEmbMessBitCount);
			Tk = sc.Convert_BinaryStreamToHalfChar(Stream_Tk);
			Ts = sc.Convert_BinaryStreamToChar(Stream_Ts);
			TEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_TEmbMessBitCount);

			//将Tr拆分成24个int
			{
				char* tmp = new char[5];
				tmp[5] = '\0';
				int position = 0;
				for(int i = 0; i < 12; i++)
				{
					for (int j = 0; j < 4; j++)
					{
						tmp[j] = Stream_Tr[position + j];
					}
					int ttr = sc.Convert_KStreamToInt(tmp, 4);
					Y_Tr[i] = ttr;
					position += 4;
				}
				for (int i = 0; i < 12; i++)
				{
					for (int j = 0; j < 4; j++)
					{
						tmp[j] = Stream_Tr[position + j];
					}
					int ttr = sc.Convert_KStreamToInt(tmp, 4);
					C_Tr[i] = ttr;
					position += 4;
				}
			}

			//vector<int> t;
			//for (int i = 0; i < 12; i++)
			//	t.push_back(Y_Tr[i]);
			//for (int i = 0; i < 12; i++)
			//	t.push_back(C_Tr[i]);
			//ShowMessage(t);
			/*
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
			*/
			

			delete[] Stream_BlockNum;
			delete[] Stream_BlockCount;
			delete[] Stream_Mk;
			delete[] Stream_Ms;
			delete[] Stream_MEmbMessBitCount;
			delete[] Stream_Tk;
			delete[] Stream_Ts;
			delete[] Stream_TEmbMessBitCount;
			delete[] Stream_Tr;
		}

		//读取T算法数据
		int Tbl[2][64];				//Q'
		{
			JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
			for (int i = 0; i < 64; i++)
			{
				Tbl[0][i] = qTbl->quantval[i];
			}
			qTbl = cinfo.quant_tbl_ptrs[1];
			for (int i = 0; i < 64; i++)
			{
				Tbl[1][i] = qTbl->quantval[i];
			}
		}
		int Cbl[2][64];		//Q
		{
			int Table_K = (int)Tk;
			JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
			for (int i = 0; i < 64; i++)
			{
				Cbl[0][i] = qTbl->quantval[i];
			}
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				Cbl[0][j] = Cbl[0][j] * Table_K;
			}
			qTbl = cinfo.quant_tbl_ptrs[1];
			for (int i = 0; i < 64; i++)
			{
				Cbl[1][i] = qTbl->quantval[i];
			}
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				Cbl[1][j] = Cbl[1][j] * Table_K;
			}
		}
		char* TBinaryStream = new char[TEmbMessBitCount];
		bool TbreakFlg = false;
		{
			int Table_K = (int)Tk;
			int Table_Stride = (int)Ts;

			bool strideFlg = false;

			int FileStreamCounter = 0;
			//int FilePositionCounter = MEmbMessBitCount;		//密文计数器，从M算法的末尾开始继续加密
			int strideCounter = 0;		//跳步计数器
			int dropCounter = 0;
			int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
				+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
				+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
				+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

			//vector<int> testarray;
			//int testcounter = 0;


			//密文抽取
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
							if (blockp == 0)
								continue;
							//ShowMessage(dropBit);
							//跳头
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //进入数据区
							{

								if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)
								{
									if (!strideFlg)
									{
										int tC = blockptr[bi] % Table_K;
										if (tC == -1)
										{
											TBinaryStream[FileStreamCounter] = '1';
											blockptr[bi] = blockptr[bi] + 1;
										}
										else if (tC == 1)
										{
											TBinaryStream[FileStreamCounter] = '1';
											blockptr[bi] = blockptr[bi] - 1;
										}
										else
										{
											TBinaryStream[FileStreamCounter] = '0';
										}

										FileStreamCounter++;
										strideFlg = true;
										if (FileStreamCounter >= TEmbMessBitCount)
										{
											TbreakFlg = true;
										}
									}
									else      //跳步
									{
										if (strideCounter >= Table_Stride - 1)
										{
											strideFlg = false;
											strideCounter = 0;
										}
										else
										{
											strideCounter++;
										}
									}
								}
							}
							if (TbreakFlg)
								break;
						}
						if (TbreakFlg)
							break;
					}
					if (TbreakFlg)
						break;
				}
				if (TbreakFlg)
					break;
			}

			dropCounter = 0;
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
							if (blockp == 0)
								continue;
							//ShowMessage(dropBit);
							//跳头
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //进入数据区
							{
								if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)
								{
									int loss = 0;
									int oTbl = 0;
									//int oldQ = 0;
									if (ci == 0)
									{
										loss = Y_Tr[bi - R_STARTPOSITION];
										oTbl = Tbl[0][bi];
										//oldQ = Cbl[0][bi];
									}
									else
									{
										loss = C_Tr[bi - R_STARTPOSITION];
										oTbl = Tbl[1][bi];
										//oldQ = Cbl[1][bi];
									}
									double rq = (double)loss / oTbl;
									double krq = Table_K + rq;
									double dkrq = (double)(blockptr[bi]) / krq;
									blockptr[bi] = round(dkrq);

								}
							}
						}
					}
				}
			}

			//vector<int> M_Testarray;
			//vector<int> M_Testarray_bx;
			//vector<int> M_Testarray_by;
			//vector<int> M_Testarray_ci;
			//vector<int> M_Testarray_bi;
			//int counter = 0;
			//bool flag = false;
			//{
			//	bool breakFlg = false;
			//	int dropCounter = 0;
			//	int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
			//		+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
			//		+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
			//		+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;
			//	for (int ci = 0; ci < 3; ci++)
			//	{
			//		JBLOCKARRAY buffer;
			//		JCOEFPTR blockptr;
			//		jpeg_component_info* compptr;
			//		compptr = cinfo.comp_info + ci;
			//		for (int by = 0; by < compptr->height_in_blocks; by++)
			//		{
			//			buffer = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			//			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			//			{
			//				blockptr = buffer[by][bx];
			//				for (int bi = 0; bi < 64; bi++)
			//				{
			//					short blockp = blockptr[bi];
			//					if (blockp == 0)
			//						continue;
			//					//TotalKexiugaiweiTest++;
			//					//跳头
			//					if (dropCounter < dropBit)
			//					{
			//						dropCounter++;
			//						continue;
			//					}
			//					else        //进入数据区
			//					{
			//						if (by == 5 && bx == 105 && bi == 24)
			//						{
			//							flag = true;
			//						}
			//						if (flag)
			//						{
			//							M_Testarray_bi.push_back(bi);
			//							M_Testarray_bx.push_back(bx);
			//							M_Testarray_by.push_back(by);
			//							M_Testarray_ci.push_back(ci);
			//							M_Testarray.push_back(blockptr[bi]);
			//							//ShowMessage(1);
			//							if (counter >= 60)
			//							{
			//								breakFlg = true;
			//								flag = false;
			//							}
			//							counter++;
			//						}
			//					}
			//					if (breakFlg)
			//						break;
			//				}
			//				if (breakFlg)
			//					break;
			//			}
			//			if (breakFlg)
			//				break;
			//		}
			//		if (breakFlg)
			//			break;
			//	}
			//}
			//for (int i = 0; i < 60; i++)
			//{
			//	CString str;
			//	CString strci;
			//	CString strbi;
			//	CString strbx;
			//	CString strby;
			//	CString strnum;
			//	strci.Format(_T("%d"), M_Testarray_ci[i]);
			//	strbi.Format(_T("%d"), M_Testarray_bi[i]);
			//	strbx.Format(_T("%d"), M_Testarray_bx[i]);
			//	strby.Format(_T("%d"), M_Testarray_by[i]);
			//	strnum.Format(_T("%d"), M_Testarray[i]);
			//	str += _T("ci: ");
			//	str += strci;
			//	str += _T(" by: ");
			//	str += strby;
			//	str += _T(" bx: ");
			//	str += strbx;
			//	str += _T(" bi: ");
			//	str += strbi;
			//	str += _T(" num: ");
			//	str += strnum;
			//	str += _T("\r\n");
			//	ShowMessage(str);
			//}

		}
		//纯调试
		/*
		vector<int> M_Testarray;
		vector<int> M_Testarray_bx;
		vector<int> M_Testarray_by;
		vector<int> M_Testarray_ci;
		vector<int> M_Testarray_bi;
		{
			bool breakFlg = false;
			int dropCounter = 0;
			int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
				+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
				+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
				+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;
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
							if (blockp == 0)
								continue;
							//TotalKexiugaiweiTest++;
							//跳头
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //进入数据区
							{
								if (bi >= R_STARTPOSITION&&bi < R_ENDPOSITION)
								{
									M_Testarray_bi.push_back(bi);
									M_Testarray_bx.push_back(bx);
									M_Testarray_by.push_back(by);
									M_Testarray_ci.push_back(ci);
									M_Testarray.push_back(blockptr[bi]);
									if (bi == R_ENDPOSITION - 1)
										breakFlg = true;
								}
							}
							if (breakFlg)
								break;
						}
						if (breakFlg)
							break;
					}
					if (breakFlg)
						break;
				}
				if (breakFlg)
					break;
			}
		}
		for (int i = 0; i < 20; i++)
		{
			CString str;
			CString strci;
			CString strbi;
			CString strbx;
			CString strby;
			CString strnum;
			strci.Format(_T("%d"), M_Testarray_ci[i]);
			strbi.Format(_T("%d"), M_Testarray_bi[i]);
			strbx.Format(_T("%d"), M_Testarray_bx[i]);
			strby.Format(_T("%d"), M_Testarray_by[i]);
			strnum.Format(_T("%d"), M_Testarray[i]);
			str += _T("ci: ");
			str += strci;
			str += _T(" by: ");
			str += strby;
			str += _T(" bx: ");
			str += strbx;
			str += _T(" bi: ");
			str += strbi;
			str += _T(" num: ");
			str += strnum;
			ShowMessage(str);
		}
		*/

		//读取M算法数据
		char* MBinaryStream = new char[MEmbMessBitCount];
		bool breakFlg = false;		//结束标识
		{
			bool strideFlg = false;

			StreamConvert sc;
			int mk = sc.Convert_CharToInt(Mk);
			int ms = sc.Convert_CharToInt(Ms);

			int dropCounter = 0;
			int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
				+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
				+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
				+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;
			//int GroupCounterTotal = 0;

			int HideCounter = 0;		//密文计数器，用来记录当前记录的密文指针
			int StrideCounter = 0;		//步长计数器
			int GroupCounter = 0;		//控制组采集，数据采集够一组之后就置零
			int PGroupBitCount_PayLoad = pow(2, mk) - 1;		//M算法中每组载体图像的位数  k位密文需要 2^k-1位嵌入
			JCOEFPTR* SrcDataGroup = (JCOEFPTR*)(cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_IMAGE, sizeof(JCOEFPTR) * PGroupBitCount_PayLoad);
			char* SecretBitGroup = nullptr;
			char* ZOSrcBitGroup = nullptr;

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
							if (blockp == 0)
								continue;
							if (dropCounter < dropBit)		//跳头
							{
								dropCounter++;
								continue;
							}
							else        //进入数据区
							{

								if (!strideFlg)
								{
									SrcDataGroup[GroupCounter] = &(blockptr[bi]);	//进行组采集
									GroupCounter++;
									if (GroupCounter == PGroupBitCount_PayLoad)
									{
										GetZOStreamGroup(&ZOSrcBitGroup, SrcDataGroup, PGroupBitCount_PayLoad);
										MDeCoding(&SecretBitGroup, ZOSrcBitGroup, mk);
										int actCount = SetSecretBitGroup(SecretBitGroup, mk, MBinaryStream, MEmbMessBitCount, HideCounter);

										if (actCount == mk)		//说明写入正常
										{
											HideCounter += mk;
										}
										else      //说明已写到末尾出现删位情况，需要跳出
										{
											breakFlg = true;
										}
										GroupCounter = 0;
										//GroupCounterTotal++;
										delete[] SecretBitGroup;
										delete[] ZOSrcBitGroup;
										strideFlg = true;
									}
									if (breakFlg)
										break;
								}
								else
								{
									if (StrideCounter >= ms - 1)
									{
										strideFlg = false;
										StrideCounter = 0;
									}
									else
									{
										StrideCounter++;
									}
								}

							}
							if (breakFlg)
								break;
						}
						if (breakFlg)
							break;
					}
					if (breakFlg)
						break;
				}
				if (breakFlg)
					break;
			}

		}
		fclose(file);
		//CString str = _T("");
		//for (int i = 0; i < 32; i++)
		//{
		//	CString ch;
		//	ch.Format(_T("%c"), TBinaryStream[i]);
		//	str += ch;
		//}
		//ShowMessage(str);
		//ShowMessage(MBinaryStream, 32);
		//ofstream out("F:\\test\\after.txt");
		//for (int i = 0; i < MEmbMessBitCount; i++)
		//	out << MBinaryStream[i];
		//out.close();
		int BinaryBitCount = MEmbMessBitCount + TEmbMessBitCount;
		char* BinaryStream = new char[MEmbMessBitCount + TEmbMessBitCount];
		for (int i = 0; i < MEmbMessBitCount; i++)
		{
			BinaryStream[i] = MBinaryStream[i];
		}
		for (int i = 0; i < TEmbMessBitCount; i++)
		{
			BinaryStream[MEmbMessBitCount + i] = TBinaryStream[i];
		}
		delete[] TBinaryStream;
		delete[] MBinaryStream;
		//写回文件
		{
			StreamConvert sc;
			uint8_t* BitArray = new uint8_t[BinaryBitCount / 8];
			//ShowMessage(BinaryBitCount);
			sc.byteStreamToBinaryString(BitArray, BinaryBitCount / 8, BinaryStream, BinaryBitCount, 1);
			//ShowMessage(UCharToCString(BitArray, 16));
			//拆分后缀和信息部分
			uint8_t* SuffixArry = nullptr;
			uint8_t* ContentArry = nullptr;
			int SuffixArryCount = 0;
			int ContentArryCount = 0;
			{
				int PointCounter = 0;
				for (int i = 0; i < BinaryBitCount / 8; i++)
				{
					if (BitArray[i] == '.')
						PointCounter++;
					if (PointCounter == 2)
					{
						SuffixArryCount = i + 1;
						ContentArryCount = BinaryBitCount / 8 - SuffixArryCount;
						break;
					}
				}
				SuffixArry = new uint8_t[SuffixArryCount];
				//ShowMessage(SuffixArryCount);
				
				for (int i = 0; i < SuffixArryCount; i++)
				{
					SuffixArry[i] = BitArray[i];
				}
				//ShowMessage(UCharToCString(SuffixArry, SuffixArryCount));
				ContentArry = new uint8_t[ContentArryCount];
				for (int i = 0; i < ContentArryCount; i++)
				{
					ContentArry[i] = BitArray[i + SuffixArryCount];
				}
			}

			string fileName = fileList.HidenFilePath;
			string Suffix = "";
			for (int i = 0; i < SuffixArryCount; i++)
			{
				string s = "";
				s+=SuffixArry[i];
				Suffix += s;
			}
			string outPath = fileName + Suffix;

			BinaryFileSolver outSolver;
			outSolver.setOutFilePath(outPath);
			outSolver.AppendFile(ContentArry, ContentArryCount);
			delete[] BitArray;
			delete[] SuffixArry;
			delete[] ContentArry;
		}
		delete[] BinaryStream;

		//写回原图片
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
		//jpeg_compress_struct coutfo;
		//InitCompressInfo(&coutfo, outFile);
		coutfo.image_width = cinfo.image_width;
		coutfo.image_height = cinfo.image_height;
		coutfo.input_components = cinfo.num_components;
		coutfo.in_color_space = cinfo.out_color_space;
		jpeg_set_defaults(&coutfo);
		jpeg_copy_critical_parameters(&cinfo, &coutfo);
		//修改量化表
		{
			JQUANT_TBL* qTbl = coutfo.quant_tbl_ptrs[0];
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				qTbl->quantval[j] = Cbl[0][j - R_STARTPOSITION];
			}
			qTbl = cinfo.quant_tbl_ptrs[1];
			for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
			{
				qTbl->quantval[j] = Cbl[1][j - R_STARTPOSITION];
			}
		}
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
		//抽取密文
		int BlkCount = 0;
		int FinalBinaryBitCount = 0;
		map<int, pair<char*, int>> BitStreamBlockDic;
		for (int i = 0; i < fileList.CarrierImagePathList.size(); i++)
		{
			//打开第 i 个载媒
			FILE* file;
			{
				string fName = fileList.CarrierImagePathList[i];
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
			char blockNum = 0;
			char blockCount = 0;
			char Mk = 0;
			char Ms = 0;
			int MEmbMessBitCount = 0;
			char Tk = 0;
			char Ts = 0;
			int TEmbMessBitCount = 0;
			int Y_Tr[12];
			int C_Tr[12];
			{
				char* Stream_BlockNum = new char[COMPOSITE_BLOCK_NUM + 1];
				char* Stream_BlockCount = new char[COMPOSITE_BLOCK_COUNTE + 1];
				char* Stream_Mk = new char[COMPOSITE_MATRIX_K + 1];
				char* Stream_Ms = new char[COMPOSITE_MATRIX_STRIDE + 1];
				char* Stream_MEmbMessBitCount = new char[COMPOSITE_MATRIX_CONTENT + 1];
				char* Stream_Tk = new char[COMPOSITE_TABLE_K + 1];
				char* Stream_Ts = new char[COMPOSITE_TABLE_STRIDE + 1];
				char* Stream_TEmbMessBitCount = new char[COMPOSITE_TABLE_CONTENT + 1];
				char* Stream_Tr = new char[COMPOSITE_TABLE_LOSE_R + 1];

				int PositionCounter = 0;
				int HideCounter_BlockNum = 0;
				int HideCounter_BlockCount = 0;
				int HideCounter_Mk = 0;
				int HideCounter_Ms = 0;
				int HideCounter_MEmbMessBitCount = 0;
				int HideCounter_Tk = 0;
				int HideCounter_Ts = 0;
				int HideCounter_TEmbMessBitCount = 0;
				int HideCounter_Tr = 0;
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
								if (blockp == 0)
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
								else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
									+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
									+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
									+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//写入量表损失记录
								{
									short bitFlg = blockp % 2;
									if (bitFlg == 0)
									{
										Stream_Tr[HideCounter_Tr] = '0';
									}
									else
									{
										Stream_Tr[HideCounter_Tr] = '1';
									}
									HideCounter_Tr++;
									PositionCounter++;
								}
								else
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
				blockCount = sc.Convert_BinaryStreamToHalfChar(Stream_BlockCount);
				Mk = sc.Convert_BinaryStreamToHalfChar(Stream_Mk);
				Ms = sc.Convert_BinaryStreamToChar(Stream_Ms);
				MEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_MEmbMessBitCount);
				Tk = sc.Convert_BinaryStreamToHalfChar(Stream_Tk);
				Ts = sc.Convert_BinaryStreamToChar(Stream_Ts);
				TEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_TEmbMessBitCount);

				//将Tr拆分成24个int
				{
					char* tmp = new char[5];
					tmp[5] = '\0';
					int position = 0;
					for (int i = 0; i < 12; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							tmp[j] = Stream_Tr[position + j];
						}
						int ttr = sc.Convert_KStreamToInt(tmp, 4);
						Y_Tr[i] = ttr;
						position += 4;
					}
					for (int i = 0; i < 12; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							tmp[j] = Stream_Tr[position + j];
						}
						int ttr = sc.Convert_KStreamToInt(tmp, 4);
						C_Tr[i] = ttr;
						position += 4;
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
				delete[] Stream_Tr;
			}
			
			//CString str;
			//CString strFileBitCount;
			//CString strMpayload;
			//CString strTpayload;
			//CString strMBit;
			//CString strTBit;
			//CString strMStride;
			//CString strTStride;
			//CString str1;
			//CString str2;
			////strFileBitCount.Format(_T("%d"), fileBitCount);
			//strMpayload.Format(_T("%d"), (int)blockNum);
			//strTpayload.Format(_T("%d"), (int)blockCount);
			//strMBit.Format(_T("%d"), (int)Mk);
			//strTBit.Format(_T("%d"), (int)Ms);
			//strMStride.Format(_T("%d"), MEmbMessBitCount);
			//strTStride.Format(_T("%d"), (int)Tk);
			//str1.Format(_T("%d"), (int)Ts);
			//str2.Format(_T("%d"), TEmbMessBitCount);
			//str = strMpayload + _T(" ") + strTpayload + _T(" ") + strMBit + _T(" ") + strTBit + _T(" ") + strMStride + _T(" ") + strTStride + _T(" ") + str1 + _T(" ") + str2;
			//ShowMessage(str);

			BlkCount = blockCount;
			//读取T算法数据
			int Tbl[2][64];				//Q'
			{
				JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
				for (int i = 0; i < 64; i++)
				{
					Tbl[0][i] = qTbl->quantval[i];
				}
				qTbl = cinfo.quant_tbl_ptrs[1];
				for (int i = 0; i < 64; i++)
				{
					Tbl[1][i] = qTbl->quantval[i];
				}
			}
			int Cbl[2][64];		//Q
			{
				int Table_K = (int)Tk;
				JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
				for (int i = 0; i < 64; i++)
				{
					Cbl[0][i] = qTbl->quantval[i];
				}
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					Cbl[0][j] = Cbl[0][j] * Table_K;
				}
				qTbl = cinfo.quant_tbl_ptrs[1];
				for (int i = 0; i < 64; i++)
				{
					Cbl[1][i] = qTbl->quantval[i];
				}
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					Cbl[1][j] = Cbl[1][j] * Table_K;
				}
			}

			char* TBinaryStream = new char[TEmbMessBitCount];
			bool TbreakFlg = false;
			{
				int Table_K = (int)Tk;
				int Table_Stride = (int)Ts;

				bool strideFlg = false;

				int FileStreamCounter = 0;
				//int FilePositionCounter = MEmbMessBitCount;		//密文计数器，从M算法的末尾开始继续加密
				int strideCounter = 0;		//跳步计数器
				int dropCounter = 0;
				int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

				//vector<int> testarray;
				//int testcounter = 0;


				//密文抽取
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
								if (blockp == 0)
									continue;
								//ShowMessage(dropBit);
								//跳头
								if (dropCounter < dropBit)
								{
									dropCounter++;
									continue;
								}
								else        //进入数据区
								{

									if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)
									{
										if (!strideFlg)
										{
											int tC = blockptr[bi] % Table_K;
											if (tC == -1)
											{
												TBinaryStream[FileStreamCounter] = '1';
												blockptr[bi] = blockptr[bi] + 1;
											}
											else if (tC == 1)
											{
												TBinaryStream[FileStreamCounter] = '1';
												blockptr[bi] = blockptr[bi] - 1;
											}
											else
											{
												TBinaryStream[FileStreamCounter] = '0';
											}

											FileStreamCounter++;
											strideFlg = true;
											if (FileStreamCounter >= TEmbMessBitCount)
											{
												TbreakFlg = true;
											}
										}
										else      //跳步
										{
											if (strideCounter >= Table_Stride - 1)
											{
												strideFlg = false;
												strideCounter = 0;
											}
											else
											{
												strideCounter++;
											}
										}
									}
								}
								if (TbreakFlg)
									break;
							}
							if (TbreakFlg)
								break;
						}
						if (TbreakFlg)
							break;
					}
					if (TbreakFlg)
						break;
				}

				dropCounter = 0;
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
								if (blockp == 0)
									continue;
								//ShowMessage(dropBit);
								//跳头
								if (dropCounter < dropBit)
								{
									dropCounter++;
									continue;
								}
								else        //进入数据区
								{
									if (bi >= R_STARTPOSITION && bi < R_ENDPOSITION)
									{
										int loss = 0;
										int oTbl = 0;
										//int oldQ = 0;
										if (ci == 0)
										{
											loss = Y_Tr[bi - R_STARTPOSITION];
											oTbl = Tbl[0][bi];
											//oldQ = Cbl[0][bi];
										}
										else
										{
											loss = C_Tr[bi - R_STARTPOSITION];
											oTbl = Tbl[1][bi];
											//oldQ = Cbl[1][bi];
										}
										double rq = (double)loss / oTbl;
										double krq = Table_K + rq;
										double dkrq = (double)(blockptr[bi]) / krq;
										blockptr[bi] = round(dkrq);

									}
								}
							}
						}
					}
				}


			}

			//读取M算法数据
			char* MBinaryStream = new char[MEmbMessBitCount];
			bool breakFlg = false;		//结束标识
			{
				bool strideFlg = false;

				StreamConvert sc;
				int mk = sc.Convert_CharToInt(Mk);
				int ms = sc.Convert_CharToInt(Ms);

				int dropCounter = 0;
				int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;
				//int GroupCounterTotal = 0;

				int HideCounter = 0;		//密文计数器，用来记录当前记录的密文指针
				int StrideCounter = 0;		//步长计数器
				int GroupCounter = 0;		//控制组采集，数据采集够一组之后就置零
				int PGroupBitCount_PayLoad = pow(2, mk) - 1;		//M算法中每组载体图像的位数  k位密文需要 2^k-1位嵌入
				JCOEFPTR* SrcDataGroup = (JCOEFPTR*)(cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_IMAGE, sizeof(JCOEFPTR) * PGroupBitCount_PayLoad);
				char* SecretBitGroup = nullptr;
				char* ZOSrcBitGroup = nullptr;

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
								if (blockp == 0)
									continue;
								if (dropCounter < dropBit)		//跳头
								{
									dropCounter++;
									continue;
								}
								else        //进入数据区
								{

									if (!strideFlg)
									{
										SrcDataGroup[GroupCounter] = &(blockptr[bi]);	//进行组采集
										GroupCounter++;
										if (GroupCounter == PGroupBitCount_PayLoad)
										{
											GetZOStreamGroup(&ZOSrcBitGroup, SrcDataGroup, PGroupBitCount_PayLoad);
											MDeCoding(&SecretBitGroup, ZOSrcBitGroup, mk);
											int actCount = SetSecretBitGroup(SecretBitGroup, mk, MBinaryStream, MEmbMessBitCount, HideCounter);

											if (actCount == mk)		//说明写入正常
											{
												HideCounter += mk;
											}
											else      //说明已写到末尾出现删位情况，需要跳出
											{
												breakFlg = true;
											}
											GroupCounter = 0;
											//GroupCounterTotal++;
											delete[] SecretBitGroup;
											delete[] ZOSrcBitGroup;
											strideFlg = true;
										}
										if (breakFlg)
											break;
									}
									else
									{
										if (StrideCounter >= ms - 1)
										{
											strideFlg = false;
											StrideCounter = 0;
										}
										else
										{
											StrideCounter++;
										}
									}

								}
								if (breakFlg)
									break;
							}
							if (breakFlg)
								break;
						}
						if (breakFlg)
							break;
					}
					if (breakFlg)
						break;
				}

			}
			fclose(file);
			int BinaryBitCount = MEmbMessBitCount + TEmbMessBitCount;
			char* BinaryStream = new char[MEmbMessBitCount + TEmbMessBitCount];
			for (int i = 0; i < MEmbMessBitCount; i++)
			{
				BinaryStream[i] = MBinaryStream[i];
			}
			for (int i = 0; i < TEmbMessBitCount; i++)
			{
				BinaryStream[MEmbMessBitCount + i] = TBinaryStream[i];
			}
			delete[] TBinaryStream;
			delete[] MBinaryStream;

			//将提取结果存入字典中
			pair<char*, int> tmpPair;
			tmpPair.first = BinaryStream;
			tmpPair.second = MEmbMessBitCount + TEmbMessBitCount;
			BitStreamBlockDic.insert(pair<int, pair<char*, int>>(blockNum, tmpPair));
			FinalBinaryBitCount += tmpPair.second;

			//写回原图片
			FILE* outFile;
			{
				string fName = fileList.NewFilePathList[i];
				const char* p = fName.data();
				fopen_s(&outFile, p, "wb");
			}
			jpeg_compress_struct coutfo;
			jpeg_error_mgr jerr1;
			coutfo.err = jpeg_std_error(&jerr1);
			jpeg_create_compress(&coutfo);
			jpeg_stdio_dest(&coutfo, outFile);
			coutfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;
			//jpeg_compress_struct coutfo;
			//InitCompressInfo(&coutfo, outFile);
			coutfo.image_width = cinfo.image_width;
			coutfo.image_height = cinfo.image_height;
			coutfo.input_components = cinfo.num_components;
			coutfo.in_color_space = cinfo.out_color_space;
			jpeg_set_defaults(&coutfo);
			jpeg_copy_critical_parameters(&cinfo, &coutfo);
			//修改量化表
			{
				JQUANT_TBL* qTbl = coutfo.quant_tbl_ptrs[0];
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					qTbl->quantval[j] = Cbl[0][j - R_STARTPOSITION];
				}
				qTbl = cinfo.quant_tbl_ptrs[1];
				for (int j = R_STARTPOSITION; j < R_ENDPOSITION; j++)
				{
					qTbl->quantval[j] = Cbl[1][j - R_STARTPOSITION];
				}
			}
			jpeg_write_coefficients(&coutfo, coeff_arrays);

			jpeg_finish_compress(&coutfo);
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_compress(&coutfo);
			jpeg_destroy_decompress(&cinfo);

			fclose(outFile);
		}

		//密文重组
		char* BinaryStream = new char[FinalBinaryBitCount];
		{
			char* PositionPointer = BinaryStream;
			for (int i = 0; i < BlkCount; i++)
			{
				char* tStream = BitStreamBlockDic[i].first;
				int tStreamCount = BitStreamBlockDic[i].second;
				for (int j = 0; j < tStreamCount; j++)
				{
					*PositionPointer = tStream[j];
					PositionPointer++;
				}

				delete[] tStream;
			}
		}

		{
			ofstream of;
			of.open("F:\\test\\after.txt");
			for (int j = 0; j < FinalBinaryBitCount; j++)
			{
				of << BinaryStream[j];
			}
			of.close();
		}

		//写回文件
		{
			StreamConvert sc;
			uint8_t* BitArray = new uint8_t[FinalBinaryBitCount / 8];
			//ShowMessage(BinaryBitCount);
			sc.byteStreamToBinaryString(BitArray, FinalBinaryBitCount / 8, BinaryStream, FinalBinaryBitCount, 1);
			//ShowMessage(UCharToCString(BitArray, 16));
			//拆分后缀和信息部分
			uint8_t* SuffixArry = nullptr;
			uint8_t* ContentArry = nullptr;
			int SuffixArryCount = 0;
			int ContentArryCount = 0;
			{
				int PointCounter = 0;
				for (int i = 0; i < FinalBinaryBitCount / 8; i++)
				{
					if (BitArray[i] == '.')
						PointCounter++;
					if (PointCounter == 2)
					{
						SuffixArryCount = i + 1;
						ContentArryCount = FinalBinaryBitCount / 8 - SuffixArryCount;
						break;
					}
				}
				SuffixArry = new uint8_t[SuffixArryCount];
				//ShowMessage(SuffixArryCount);

				for (int i = 0; i < SuffixArryCount; i++)
				{
					SuffixArry[i] = BitArray[i];
				}
				//ShowMessage(UCharToCString(SuffixArry, SuffixArryCount));
				ContentArry = new uint8_t[ContentArryCount];
				for (int i = 0; i < ContentArryCount; i++)
				{
					ContentArry[i] = BitArray[i + SuffixArryCount];
				}
			}

			string fileName = fileList.HidenFilePath;
			string Suffix = "";
			for (int i = 0; i < SuffixArryCount; i++)
			{
				string s = "";
				s += SuffixArry[i];
				Suffix += s;
			}
			string outPath = fileName + Suffix;

			BinaryFileSolver outSolver;
			outSolver.setOutFilePath(outPath);
			outSolver.AppendFile(ContentArry, ContentArryCount);
			delete[] BitArray;
			delete[] SuffixArry;
			delete[] ContentArry;
		}
		//delete[] BinaryStream;

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
	cinfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;
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
					if (blockp == 0)
						continue;
					if (EBlockCounter < 13)
					{
						EBlockCounter++;
					}
					else
					{
						EBlockCounter = 0;
						break;
					}
					if (EBlockCounter > 1 && blockp >= K)
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
	cinfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;
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
					if (blockp == 0)
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

void HighCapabilityCompositeAlg::ShowMessage(vector<int> intList)
{
	CMainFrame* pMainfram = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CString finalstr;
	for (auto i = intList.begin(); i != intList.end(); i++)
	{
		CString str;
		str.Format(_T("%d"), *i);
		finalstr += str;
		finalstr += _T(" ");
	}
	MessageBox(pMainfram->GetSafeHwnd(), finalstr, _T("123"), 1);
}

void HighCapabilityCompositeAlg::ShowMessage(int num)
{
	CMainFrame* pMainfram = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CString str;
	str.Format(_T("%d"), num);
	MessageBox(pMainfram->GetSafeHwnd(), str, _T("123"), 1);
}

void HighCapabilityCompositeAlg::ShowMessage(char ch)
{
	CMainFrame* pMainfram = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CString str;
	str.Format(_T("%c"), ch);
	MessageBox(pMainfram->GetSafeHwnd(), str, _T("123"), 1);
}

void HighCapabilityCompositeAlg::ShowMessage(const char * charry, int count)
{
	CMainFrame* pMainfram = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CString str = _T("");
	for (int i = 0; i < count; i++)
	{
		CString ch;
		ch.Format(_T("%c"), charry[i]);
		str = str + ch;
		str = str + _T(" ");
	}
	MessageBox(pMainfram->GetSafeHwnd(), str, _T("123"), 1);
}

void HighCapabilityCompositeAlg::ShowMessage(double num)
{
	CMainFrame* pMainfram = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CString str;
	str.Format(_T("%f"), num);
	MessageBox(pMainfram->GetSafeHwnd(), str, _T("123"), 1);
}

CString HighCapabilityCompositeAlg::CharToCString(char * str, int length)
{
	CString final;
	for (int i = 0; i < length; i++)
	{
		CString s;
		s.Format(_T("%c"), str[i]);
		final += s;
		final += _T(" ");
	}
	return final;
}

CString HighCapabilityCompositeAlg::ShortToCString(short * str, int length)
{
	CString final;
	for (int i = 0; i < length; i++)
	{
		CString s;
		s.Format(_T("%d"), str[i]);
		final += s;
		final += _T(" ");
	}
	return final;
}

CString HighCapabilityCompositeAlg::UCharToCString(unsigned char * str, int length)
{
	CString final;
	for (int i = 0; i < length; i++)
	{
		CString s;
		s.Format(_T("%c"), str[i]);
		final += s;
		final += _T(" ");
	}
	return final;
}

inline bool HighCapabilityCompositeAlg::GetSecretBitGroup(char ** SecretStream, int bitCount, const char const * fileStream, int fileBitCount, int position)		//如果fileStream剩余的字节已经不足以作为一个bitCount长度的分组，则用2补齐并返回true
{
	*SecretStream = new char[bitCount+1];
	(*SecretStream)[bitCount] = '\0';
	bool endFlg = false;
	for (int i = 0; i < bitCount; i++)
	{
		if (position + i >= fileBitCount)
		{
			(*SecretStream)[i] = '0';
			endFlg = true;
		}
		else
			(*SecretStream)[i] = fileStream[position + i];
	}
	return endFlg;
}

inline int HighCapabilityCompositeAlg::SetSecretBitGroup(const char * const SecretStream, int bitCount, char* fileStream, int fileBitCount, int position)		//返回嵌入的有效字数
{
	int count = 0;
	for (int i = 0; i < bitCount; i++)
	{
		//vector<int> test;
		//test.push_back(position);
		//test.push_back(i);
		//test.push_back(fileBitCount);
		//ShowMessage(test);
		if (position + i < fileBitCount)
		{
			fileStream[position + i] = SecretStream[i];
			count++;
		}
		else
			break;
	}
	return count;
}

inline void HighCapabilityCompositeAlg::GetZOStreamGroup(char ** ZOStream, const JCOEFPTR const * SrcDataGroup, int PGroupPayload)
{
	(*ZOStream) = new char[PGroupPayload];
	short tmp = 0;
	for (int i = 0; i < PGroupPayload; i++)
	{
		tmp = *(SrcDataGroup[i]);
		if (tmp % 2 == 1)
		{
			(*ZOStream)[i] = '1';
		}
		else if (tmp % 2 == -1)
		{
			(*ZOStream)[i] = '2';
		}
		else if (tmp % 2 == 0)
		{
			(*ZOStream)[i] = '0';
		}
	}
}

inline int HighCapabilityCompositeAlg::MCoding(const char * const SecretStream, const char * const ZOStream, int K)
{
	StreamConvert sc;
	int ZOStreamBCount = pow(2, K) - 1;
	int current = 0;
	for (int i = 0, counter = 1; i < ZOStreamBCount; i++, counter++)
	{
		if (ZOStream[i] == '1')		//说明原载体位是正奇数位
		{
			current ^= counter;
		}
		else if (ZOStream[i] == '2')		//说明原载体位是负奇数位
		{
			current ^= counter;
		}
		else if (ZOStream[i] == '0')		//说明原载体位是偶数位
		{
			continue;
		}
	}

	int secret = sc.Convert_KStreamToInt(SecretStream, K);
	current ^= secret;
	current -= 1;
	return current;
}

inline void HighCapabilityCompositeAlg::MDeCoding(char** SecreStream, const char * const ZOStream, int K)
{
	StreamConvert sc;
	int ZOStreamBitCount = pow(2, K) - 1;
	*SecreStream = new char[K];
	int current = 0;
	for (int i = 0, counter = 1; i < ZOStreamBitCount; i++, counter++)
	{
		if (ZOStream[i] == '1' || ZOStream[i] == '2')
		{
			current ^= counter;
		}
	}
	char* cstr_current = nullptr;
	sc.Convert_IntToBinaryStream(current, &cstr_current);
	for (int i = K - 1, j = BITCOUNT_INT - 1; i >= 0; i--, j--)
	{
		(*SecreStream)[i] = cstr_current[j];
	}
	delete[] cstr_current;
	return;
}
