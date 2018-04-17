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
	
	if (fileList.CarrierImagePathList.size() == 1)
	{
		int BCount = GetEffectiveBitsCount(fileList.CarrierImagePathList[0]);		//���޸���Чλ��

		//����������޸���Чλ��
		int TEffiBitCount = TableCalEffectiveBitsCount(fileList.CarrierImagePathList[0], TableK);

		int fileBitCount = 0;
		//�����ļ���С
		fileBitCount = FinalBitCount;
		/*{
			CFileStatus status;
			CString strFile = _T("");
			CA2T szr(fileList.HidenFilePath.c_str());
			strFile = (LPCTSTR)szr;
			CFile::GetStatus(strFile, status);
			long iSizeOfFile;
			iSizeOfFile = status.m_size;
			fileBitCount = iSizeOfFile * 8;
		}*/

		int MPayLoad = 0;
		int TPayLoad = 0;
		//�������������ֵ��غ�
		{
			MPayLoad = MatrixCalPayLoad(fileList.CarrierImagePathList[0], MatrixK);
			TPayLoad = floor((double)TEffiBitCount / Table_NBit)*Table_DBit;
		}

		int MBit = 0;
		int TBit = 0;
		//������Գе������ı�����
		{
			double MRate = (double)(MPayLoad) / ((double)MPayLoad + TPayLoad);
			MBit = floor(MRate*fileBitCount);
			TBit = fileBitCount - MBit;
		}

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
							else if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
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

		}

		//M�㷨Ƕ��
		bool breakFlg = false;		//������ʶ
		{
			int dropCounter = 0;
			int dropBit = ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
				+ COMPOSITE_BLOCK_COUNTE + COMPOSITE_MATRIX_K + COMPOSITE_MATRIX_STRIDE
				+ COMPOSITE_MATRIX_CONTENT + COMPOSITE_TABLE_K + COMPOSITE_TABLE_STRIDE
				+ COMPOSITE_TABLE_CONTENT + COMPOSITE_TABLE_LOSE_R;

			int HideCounter = 0;		//���ļ�������������¼��ǰ��¼������ָ��
			int GroupCounter = 0;		//������ɼ������ݲɼ���һ��֮�������
			int PGroupBitCount_PayLoad = pow(2, this->MatrixK) - 1;		//M�㷨��ÿ������ͼ���λ��  kλ������Ҫ 2^k-1λǶ��
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
							if (blockptr == 0)
								continue;
							if (dropCounter < dropBit)		//��ͷ
							{
								dropCounter++;
								continue;
							}
							else        //����������
							{
								SrcDataGroup[GroupCounter] = &(blockptr[bi]);	//������ɼ�
								GroupCounter++;

								if (GroupCounter == PGroupBitCount_PayLoad)		//��ɼ���ϣ��Ը������ݽ���Ƕ��������������������
								{
									if (!GetSecretBitGroup(&SecretBitGroup, this->MatrixK, FinalBinaryStream, FinalBitCount, HideCounter))		//δ������䣬��ʣ�����Ķ���K��
									{
										HideCounter += this->MatrixK;
									}
									else
									{
										breakFlg = true;
									}
									//�����������
									{
										GetZOStreamGroup(&ZOSrcBitGroup, SrcDataGroup, PGroupBitCount_PayLoad);		//�Ƚ����崦��Ϊ01����

										//�������

									}

									GroupCounter = 0;	//��ɼ�����������
									delete[] SecretBitGroup;

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
				if (breakFlg)
					break;
			}
			/*
			ShowMessage(_T("here we go"));
			*(test[0]) = 3;
			HideCounter = 0;
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
			if (blockptr == 0)
			continue;
			if (dropCounter < dropBit)		//��ͷ
			{
			dropCounter++;
			continue;
			}
			else        //����������
			{
			HideCounter++;
			//����
			{
			CString str;
			str.Format(_T("%d"), blockp);
			ShowMessage(str);
			}
			if (HideCounter == 8)
			break;
			}
			if (HideCounter == 8)
			break;
			}
			if (HideCounter == 8)
			break;
			}
			if (HideCounter == 8)
			break;
			}
			if (HideCounter == 8)
			break;
			}
			*/

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
							else if (PositionCounter == ALGHEAD_TYPE_LENGTH + COMPOSITE_BLOCK_NUM
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
			char blockCount = sc.Convert_BinaryStreamToHalfChar(Stream_BlockCount);
			char Mk = sc.Convert_BinaryStreamToHalfChar(Stream_Mk);
			char Ms = sc.Convert_BinaryStreamToChar(Stream_Ms);
			int MEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_MEmbMessBitCount);
			char Tk = sc.Convert_BinaryStreamToHalfChar(Stream_Tk);
			char Ts = sc.Convert_BinaryStreamToChar(Stream_Ts);
			int TEmbMessBitCount = sc.Convert_BinaryStreamToInt(Stream_TEmbMessBitCount);

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
	cinfo.mem->max_memory_to_use = 10000000;
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
