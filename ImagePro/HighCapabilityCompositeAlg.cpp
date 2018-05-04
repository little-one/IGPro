#include "stdafx.h"
#include "HighCapabilityCompositeAlg.h"
#include "MainFrm.h"
#include <math.h>

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

	//��ȡ�ļ���׺�����浽��Ϣ��ǰ��
	char* SuffixArr = nullptr;
	int SuffixCount = 0;
	{
		string s = fileList.HidenFilePath;
		int PointIndex = s.find('.');
		uint8_t* tmpStr = nullptr;
		int charCount = 0;
		if (PointIndex == -1)  //˵��ԭ�ļ����޺�׺��
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
	//��ȡ�����ܵĶ�������
	{
		BinaryFileSolver fSolver;
		fSolver.ClearBeforeRead();
		fSolver.setInFilePath(fileList.HidenFilePath);
		if (fSolver.LoadFile() != 0)
		{
			//��Ҫ����
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
	//�ϲ���׺������ļ���������
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
		int BCount = GetEffectiveBitsCount(fileList.CarrierImagePathList[0]);		//���޸���Чλ��
		//ShowMessage(BCount);
		//����������޸���Чλ��
		int TEffiBitCount = TableCalEffectiveBitsCount(fileList.CarrierImagePathList[0], TableK);
		//ShowMessage(TEffiBitCount);
		int fileBitCount = 0;
		//�����ļ���С
		fileBitCount = FinalBitCount;

		int MPayLoad = 0;
		int TPayLoad = 0;
		//�������������ֵ��غ�
		{
			MPayLoad = MatrixCalPayLoad(fileList.CarrierImagePathList[0], MatrixK);
			TPayLoad = floor((double)TEffiBitCount / Table_NBit)*Table_DBit;
		}
		//ShowMessage(fileBitCount);
		//ShowMessage(MPayLoad);
		//ShowMessage(TPayLoad);
		int MBit = 0;
		int TBit = 0;
		//������Գе������ı��������Զ�����Ϊ8�ı���
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
		//���㲽��
		{
			int GroupNum = ceil(MBit / MatrixK);
			int effiBitPerGroup = floor((double)(BCount) / GroupNum);
			MStride = effiBitPerGroup - pow(2, MatrixK) + 1;
		}
		int TStride = 0;
		//���㲽��
		{
			int GroupNum = ceil(TBit / Table_DBit);
			int effiBitPerGroup = floor((double)TEffiBitCount / GroupNum);
			TStride = effiBitPerGroup - Table_NBit;
		}
		//ShowMessage(MStride);
		/*
		CString str_BCount;
		CString str_TEffiBitCount;
		CString str_fileBitCount;
		CString str_MPayLoad;
		CString str_TPayLoad;
		CString str_MBit;
		CString str_TBit;
		CString str_MStride;
		CString str_TStride;

		str_BCount.Format(_T("%d"), BCount);
		str_TEffiBitCount.Format(_T("%d"), TEffiBitCount);
		str_fileBitCount.Format(_T("%d"), fileBitCount);
		str_MPayLoad.Format(_T("%d"), MPayLoad);
		str_TPayLoad.Format(_T("%d"), TPayLoad);
		str_MBit.Format(_T("%d"), MBit);
		str_TBit.Format(_T("%d"), TBit);
		str_MStride.Format(_T("%d"), MStride);
		str_TStride.Format(_T("%d"), TStride);

		CString total = str_BCount + " " + str_TEffiBitCount + " " + str_fileBitCount + " " + str_MPayLoad + " " + str_TPayLoad + " " + str_MBit + " " + str_TBit
		+ " " + str_MStride + " " + str_TStride;
		ShowMessage(total);
		*/

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


		//������������
		jvirt_barray_ptr* coeff_arrays;
		coeff_arrays = jpeg_read_coefficients(&cinfo);

		//д���㷨��Ϣͷ
		WriteAlgHead(coeff_arrays, &cinfo, GetAlgName());

		//����24��r��ֵ
		int Y_Tr[12];
		int C_Tr[12];
		{
			JQUANT_TBL* qTbl = cinfo.quant_tbl_ptrs[0];
			int offset = 0;
			for (int j = 1; j < 13; j++)
			{
				Y_Tr[offset] = qTbl->quantval[j] - this->TableK * floor((double)(qTbl->quantval[j]) / (this->TableK));
				offset++;
			}
			qTbl = cinfo.quant_tbl_ptrs[1];
			offset = 0;
			for (int j = 1; j < 13; j++)
			{
				C_Tr[offset] = qTbl->quantval[j] - this->TableK * floor((double)(qTbl->quantval[j]) / (this->TableK));
				offset++;
			}
		}

		//д�븴���㷨ͷ��
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

			//��24��r�����4*24���ַ�Ȼ��ͳһ�����һ���ַ���
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
							if (PositionCounter < ALGHEAD_TYPE_LENGTH)	//����8λ���㷨����λ
							{
								PositionCounter++;
								continue;
							}
							else if (PositionCounter < ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM)	//д��ֿ���
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
								+ COMPOSITE_BLOCK_COUNTE)		//д��ֿ��ܿ���
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
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K)		//д�����K
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
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE)		//д����󲽳�
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
								+ COMPOSITE_MATRIX_CONTENT)		//д�����Ƕ��������
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
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K)		//д������K
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
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE)		//д��������
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
								+ COMPOSITE_TABLE_CONTENT)		//д����������λ��
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
								+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
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
							+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
						{
							break;
						}
					}
					if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
						+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
						+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
						+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
					{
						break;
					}
				}
				if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
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

		//M�㷨Ƕ��
		bool breakFlg = false;		//������ʶ
		{
			bool strideFlg = false;

			int dropCounter = 0;
			int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
				+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
				+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
				+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

			int HideCounter = 0;		//���ļ�������������¼��ǰ��¼������ָ��
			int GroupCounter = 0;		//������ɼ������ݲɼ���һ��֮�������
			int StrideCounter = 0;		//���Ʋ�����Ծ

			int PGroupBitCount_PayLoad = pow(2, this->MatrixK) - 1;		//M�㷨��ÿ������ͼ���λ��  kλ������Ҫ 2^k-1λǶ��
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
							//��ͷ
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //����������
							{
								if (!strideFlg)				//��ǰΪ����д��״̬
								{
									SrcDataGroup[GroupCounter] = &(blockptr[bi]);	//������ɼ�
									GroupCounter++;

									//��ɼ���ϣ��Ը������ݽ���Ƕ��������������������
									if (GroupCounter == PGroupBitCount_PayLoad)
									{

										if (!GetSecretBitGroup(&SecretBitGroup, this->MatrixK, FinalBinaryStream, MBit, HideCounter))		//δ������䣬��ʣ�����Ķ���K��
										{
											HideCounter += this->MatrixK;
											//HideBitFinalTest = HideCounter;
										}
										else
										{
											breakFlg = true;
										}

										//�����������
										{
											GetZOStreamGroup(&ZOSrcBitGroup, SrcDataGroup, PGroupBitCount_PayLoad);		//�Ƚ����崦��Ϊ01����
																						
											//�������
											int position = MCoding(SecretBitGroup, ZOSrcBitGroup, this->MatrixK);
											
											
											if (position > -1)		//=-1ʱ����Ҫ�޸ģ������޸���Ӧ��λ��
											{
												if (ZOSrcBitGroup[position] == '1')		//˵����λ������λ
												{
													*(SrcDataGroup[position]) = *(SrcDataGroup[position]) + 1;
												}
												else if (ZOSrcBitGroup[position] == '2')		//˵����λ�Ǹ���λ
												{
													*(SrcDataGroup[position]) = *(SrcDataGroup[position]) - 1;
												}
												else if (ZOSrcBitGroup[position] == '0')		//˵����λ��ż��λ
												{
													*(SrcDataGroup[position]) = *(SrcDataGroup[position]) + 1;
												}
											}

										}

										GroupCounter = 0;	//��ɼ�����������
										delete[] SecretBitGroup;
										delete[] ZOSrcBitGroup;
										strideFlg = true;
									}
									if (breakFlg)
										break;
								}
								else           //Ƕ��һ�����ķ���֮������stride������
								{
									if (StrideCounter >= MStride-1)
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
			//ShowMessage(MBit);
			//ShowMessage(HideBitFinalTest);
			//ShowMessage(TotalKexiugaiweiTest);
		}

		//T�㷨Ƕ��
		bool TbreakFlg = false;
		{
			bool strideFlg = false;

			//�޸�������
			//{
			//	JQUANT_TBL* qTbl = coutfo.quant_tbl_ptrs[0];
			//	for (int j = 1; j < 13; j++)
			//	{
			//		qTbl->quantval[j] = floor((double)(qTbl->quantval[j]) / this->TableK);
			//	}
			//	qTbl = cinfo.quant_tbl_ptrs[1];
			//	for (int j = 1; j < 13; j++)
			//	{
			//		qTbl->quantval[j] = floor((double)(qTbl->quantval[j]) / this->TableK);
			//	}
			//}

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
							//��ͷ
							if (dropCounter < dropBit)
							{
								dropCounter++;
								continue;
							}
							else        //����������
							{
								
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
		//�޸�������
		{
			JQUANT_TBL* qTbl = coutfo.quant_tbl_ptrs[0];
			for (int j = 1; j < 13; j++)
			{
				qTbl->quantval[j] = floor((double)(qTbl->quantval[j]) / this->TableK);
			}
			qTbl = cinfo.quant_tbl_ptrs[1];
			for (int j = 1; j < 13; j++)
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

		//������������
		jvirt_barray_ptr* coeff_arrays;
		coeff_arrays = jpeg_read_coefficients(&cinfo);

		//��ȡ�㷨ͷ����������Ϣ
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
							if (PositionCounter < ALGHEAD_TYPE_LENGTH)	//����8λ���㷨����λ
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
								+ COMPOSITE_BLOCK_COUNTE)		//д��ֿ��ܿ���
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
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K)		//д�����K
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
								+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE)		//д����󲽳�
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
								+ COMPOSITE_MATRIX_CONTENT)		//д�����Ƕ��������
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
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K)		//д������K
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
								+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE)		//д��������
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
								+ COMPOSITE_TABLE_CONTENT)		//д����������λ��
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
								+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
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
							+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
						{
							break;
						}
					}
					if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
						+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
						+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
						+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
					{
						break;
					}
				}
				if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
					+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
					+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
					+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R)		//д��������ʧ��¼
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

			//��Tr��ֳ�24��int
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

		//��ȡM�㷨����
		char* MBinaryStream = new char[MEmbMessBitCount];
		bool breakFlg = false;		//������ʶ
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

			int HideCounter = 0;		//���ļ�������������¼��ǰ��¼������ָ��
			int StrideCounter = 0;		//����������
			int GroupCounter = 0;		//������ɼ������ݲɼ���һ��֮�������
			int PGroupBitCount_PayLoad = pow(2, mk) - 1;		//M�㷨��ÿ������ͼ���λ��  kλ������Ҫ 2^k-1λǶ��
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
							if (dropCounter < dropBit)		//��ͷ
							{
								dropCounter++;
								continue;
							}
							else        //����������
							{
								if (!strideFlg)
								{
									SrcDataGroup[GroupCounter] = &(blockptr[bi]);	//������ɼ�
									GroupCounter++;
									if (GroupCounter == PGroupBitCount_PayLoad)
									{
										GetZOStreamGroup(&ZOSrcBitGroup, SrcDataGroup, PGroupBitCount_PayLoad);
										MDeCoding(&SecretBitGroup, ZOSrcBitGroup, mk);
										int actCount = SetSecretBitGroup(SecretBitGroup, mk, MBinaryStream, MEmbMessBitCount, HideCounter);

										if (actCount == mk)		//˵��д������
										{
											HideCounter += mk;
										}
										else      //˵����д��ĩβ����ɾλ�������Ҫ����
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
									if (StrideCounter >= ms)
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
		//ShowMessage(TEmbMessBitCount);
		char* TBinaryStream = new char[TEmbMessBitCount];
		for (int i = 0; i < TEmbMessBitCount; i++)
		{
			TBinaryStream[i] = '0';
		}

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
		//д���ļ�
		{
			StreamConvert sc;
			uint8_t* BitArray = new uint8_t[BinaryBitCount / 8];

			sc.byteStreamToBinaryString(BitArray, BinaryBitCount / 8, BinaryStream, BinaryBitCount, 1);
			//ShowMessage(UCharToCString(BitArray, 16));
			//��ֺ�׺����Ϣ����
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

int HighCapabilityCompositeAlg::MatrixCalPayLoad(string filePath, int K)		//������Ƕ������ĵ�λ��
{
	int BCount = GetEffectiveBitsCount(filePath);
	int BitsPerGroup = (int)(pow(2, K) - 1);
	int GroupCount = (int)floor(BCount*1.0 / BitsPerGroup);
	return GroupCount*K;
}

int HighCapabilityCompositeAlg::TableCalEffectiveBitsCount(string filePath, int K)		//��������filePath����֮ǰʹ�ã���������ռ�ô���
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
	int EBlockCounter = 0;		//ÿ�������ѡ��12λ����������12λǶ��
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

inline bool HighCapabilityCompositeAlg::GetSecretBitGroup(char ** SecretStream, int bitCount, const char const * fileStream, int fileBitCount, int position)		//���fileStreamʣ����ֽ��Ѿ���������Ϊһ��bitCount���ȵķ��飬����2���벢����true
{
	*SecretStream = new char[bitCount+1];
	(*SecretStream)[bitCount] = '\0';
	bool endFlg = false;
	for (int i = 0; i < bitCount; i++)
	{
		if (position + i >= fileBitCount)
		{
			(*SecretStream)[i] = '2';
			endFlg = true;
		}
		else
			(*SecretStream)[i] = fileStream[position + i];
	}
	return endFlg;
}

inline int HighCapabilityCompositeAlg::SetSecretBitGroup(const char * const SecretStream, int bitCount, char* fileStream, int fileBitCount, int position)		//����Ƕ�����Ч����
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
		if (ZOStream[i] == '1')		//˵��ԭ����λ��������λ
		{
			current ^= counter;
		}
		else if (ZOStream[i] == '2')		//˵��ԭ����λ�Ǹ�����λ
		{
			current ^= counter;
		}
		else if (ZOStream[i] == '0')		//˵��ԭ����λ��ż��λ
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
