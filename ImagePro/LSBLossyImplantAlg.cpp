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

//void LSBLossyImplantAlg::test(jpeg_decompress_struct* cinfo, FILE* file)
//{
//	jpeg_error_mgr jerr;
//	cinfo->err = jpeg_std_error(&jerr);
//	jpeg_create_decompress(cinfo);
//	jpeg_stdio_src(cinfo, file);
//	jpeg_read_header(cinfo, TRUE);
//	//cinfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;
//	return;
//}

void LSBLossyImplantAlg::InitDecompressInfo(jpeg_decompress_struct* cinfo)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_decompress(cinfo);
	return;
}

void LSBLossyImplantAlg::InitDecompressInfo(jpeg_decompress_struct* cinfo, FILE* file)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_decompress(cinfo);
	jpeg_stdio_src(cinfo, file);
	jpeg_read_header(cinfo, TRUE);
	cinfo->mem->max_memory_to_use = VIRT_MEMORY_SIZE;
	return;
}

void LSBLossyImplantAlg::InitCompressInfo(jpeg_compress_struct* cinfo, FILE * file)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_compress(cinfo);
	jpeg_stdio_dest(cinfo, file);
	cinfo->mem->max_memory_to_use = VIRT_MEMORY_SIZE;
	return;
}

void LSBLossyImplantAlg::InitCompressInfo(jpeg_compress_struct* cinfo)
{
	jpeg_error_mgr jerr;
	cinfo->err = jpeg_std_error(&jerr);
	jpeg_create_compress(cinfo);
	return;
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

	if (fileList.CarrierImagePathList.size() == 1)	//������
	{
		//��ʼ��ѹ���ṹ
		FILE* file;
		{
			string fName = fileList.CarrierImagePathList[0];
			const char* p = fName.data();
			if (fopen_s(&file, p, "rb") != 0)
				return;
		}
		//jpeg_decompress_struct cinfo;
		//InitDecompressInfo(&cinfo, file);
		//jpeg_decompress_struct cinfo;
		//test(&cinfo, file);
		//jpeg_decompress_struct* cinfo = InitDecompressInfo(file);
		jpeg_decompress_struct cinfo;
		jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);
		jpeg_stdio_src(&cinfo, file);
		jpeg_read_header(&cinfo, TRUE);
		cinfo.mem->max_memory_to_use = VIRT_MEMORY_SIZE;
		
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

		//������������
		jvirt_barray_ptr* coeff_arrays;
		coeff_arrays = jpeg_read_coefficients(&cinfo);

		//д���㷨��Ϣͷ
		WriteAlgHead(coeff_arrays, &cinfo, GetAlgName());

		//д��Ƕ��λ����
		{
			StreamConvert sc;
			char* CountBitStream = sc.Convert_IntToBinaryStream(FinalBitCount);

			int BitDropOutNum = ALGHEAD_TYPE_LENGTH;
			int dropCounter = 0;		//��������������Ҫ�Ƚ��㷨��Ϣͷ��8bit����ȥ

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
							if (dropCounter < ALGHEAD_TYPE_LENGTH)
							{
								dropCounter++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ��8bit�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								switch (CountBitStream[HideCounter])
								{
								case '1':
									if (blockptr[bi] % 2 == 0)
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
								HideCounter++;
							}
							if (HideCounter == ALGHEAD_CONTENT_LENGTH)
								break;
						}
						if (HideCounter == ALGHEAD_CONTENT_LENGTH)
							break;
					}
					if (HideCounter == ALGHEAD_CONTENT_LENGTH)
						break;
				}
				if (HideCounter == ALGHEAD_CONTENT_LENGTH)
					break;
			}
		}

		//д����
		{
			StreamConvert sc;
			char num = (char)0;
			char* BinaryStream = sc.Convert_CharToBinaryStream(num);
			
			int BitDropOutNumType = ALGHEAD_TYPE_LENGTH;
			int dropCounterType = 0;		//��������������Ҫ�Ƚ��㷨��Ϣͷ��8bit����ȥ

			int BitDropOutNumBCount = ALGHEAD_CONTENT_LENGTH;
			int dropCounterBCount = 0;

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
							if (dropCounterType < BitDropOutNumType)
							{
								dropCounterType++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ��8bit�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								if (dropCounterBCount < BitDropOutNumBCount)	//����������ʾ�㷨������������Ϣͷ
								{
									dropCounterBCount++;
									continue;
								}
								else
								{
									switch (BinaryStream[HideCounter])
									{
									case '1':
										if (blockptr[bi] % 2 == 0)
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
									HideCounter++;
								}

							}
							if (HideCounter == ALGHEAD_BLOCK_NUM)
								break;
						}
						if (HideCounter == ALGHEAD_BLOCK_NUM)
							break;
					}
					if (HideCounter == ALGHEAD_BLOCK_NUM)
						break;
				}
				if (HideCounter == ALGHEAD_BLOCK_NUM)
					break;
			}
		}

		//д���ܿ���
		{
			StreamConvert sc;
			char num = (char)1;
			char* BinaryStream = sc.Convert_CharToBinaryStream(num);

			int BitDropOutNum = ALGHEAD_TYPE_LENGTH + ALGHEAD_CONTENT_LENGTH + ALGHEAD_BLOCK_NUM;
			int dropCounter = 0;

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
							if (dropCounter < BitDropOutNum)
							{
								dropCounter++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								switch (BinaryStream[HideCounter])
								{
								case '1':
									if (blockptr[bi] % 2 == 0)
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
								HideCounter++;
							}
							if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
								break;
						}
						if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
							break;
					}
					if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
						break;
				}
				if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
					break;
			}
		}

		//д����Ϣ
		{
			int DropOutBitCount = ALGHEAD_TYPE_LENGTH + ALGHEAD_CONTENT_LENGTH + ALGHEAD_BLOCK_NUM + ALGHEAD_BLOCK_TOTALCOUNT;
			int dropCounter = 0;

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
							if (dropCounter < DropOutBitCount)
							{
								dropCounter++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ��48bit�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								switch (FinalBinaryStream[HideCounter])
								{
								case '1':
									if (blockptr[bi] % 2 == 0)
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
								HideCounter++;
							}
							if (HideCounter == FinalBitCount)
								break;
						}
						if (HideCounter == FinalBitCount)
							break;
					}
					if (HideCounter == FinalBitCount)
						break;
				}
				if (HideCounter == FinalBitCount)
					break;
			}
		}

		delete[] FinalBinaryStream;
		//FinishDecompress(cinfo);
		fclose(file);
		
		FILE* outFile; 
		{
			string fName = fileList.NewFilePathList[0];
			const char* p = fName.data();
			fopen_s(&outFile, p, "wb");
		}
		/*jpeg_compress_struct coutfo = InitCompressInfo(outFile);*/
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
		jpeg_write_coefficients(&coutfo, coeff_arrays);

		jpeg_finish_compress(&coutfo);
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_compress(&coutfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(outFile);
	}
	else if(fileList.CarrierImagePathList.size()==0)
	{
		//�׳��쳣
		//��Ҫ����
		return;
	}
	else  //������
	{

	}
}

void LSBLossyImplantAlg::ExecuteExtractingAlg()
{
	FileList fileList = this->GetFileList();
	if (fileList.CarrierImagePathList.size() == 1)
	{
		//��ʼ��ѹ���ṹ
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

		int BinaryBitCount = 0;
		//��ȡ��Чλ��
		{
			/*StreamConvert sc;
			char* CountBitStream = sc.Convert_IntToBinaryStream(BinaryBitCount);*/

			char BitCountCharArry[ALGHEAD_CONTENT_LENGTH + 1] = "";

			int BitDropOutNum = ALGHEAD_TYPE_LENGTH;
			int dropCounter = 0;		//��������������Ҫ�Ƚ��㷨��Ϣͷ��8bit����ȥ

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
							if (dropCounter < BitDropOutNum)
							{
								dropCounter++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ��8bit�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									BitCountCharArry[HideCounter] = '0';
								}
								else
								{
									BitCountCharArry[HideCounter] = '1';
								}
								HideCounter++;
							}
							if (HideCounter == ALGHEAD_CONTENT_LENGTH)
								break;
						}
						if (HideCounter == ALGHEAD_CONTENT_LENGTH)
							break;
					}
					if (HideCounter == ALGHEAD_CONTENT_LENGTH)
						break;
				}
				if (HideCounter == ALGHEAD_CONTENT_LENGTH)
					break;
			}
			StreamConvert sc;
			BinaryBitCount = sc.Convert_BinaryStreamToInt(BitCountCharArry);
		}

		short BlockNum = 0;
		//��ȡ��ǰ���
		{
			/*StreamConvert sc;
			char* CountBitStream = sc.Convert_IntToBinaryStream(BinaryBitCount);*/

			char BlockNumCharArry[ALGHEAD_BLOCK_NUM + 1] = "";

			int BitDropOutNum = ALGHEAD_TYPE_LENGTH + ALGHEAD_CONTENT_LENGTH;
			int dropCounter = 0;		//��������������Ҫ�Ƚ��㷨��Ϣͷ��8bit����ȥ

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
							if (dropCounter < BitDropOutNum)
							{
								dropCounter++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ��8bit�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									BlockNumCharArry[HideCounter] = '0';
								}
								else
								{
									BlockNumCharArry[HideCounter] = '1';
								}
								HideCounter++;
							}
							if (HideCounter == ALGHEAD_BLOCK_NUM)
								break;
						}
						if (HideCounter == ALGHEAD_BLOCK_NUM)
							break;
					}
					if (HideCounter == ALGHEAD_BLOCK_NUM)
						break;
				}
				if (HideCounter == ALGHEAD_BLOCK_NUM)
					break;
			}
			StreamConvert sc;
			BlockNum = (short)sc.Convert_BinaryStreamToChar(BlockNumCharArry);
		}

		short TotalBlockCount = 0;
		//��ȡ�ܿ��
		{
			/*StreamConvert sc;
			char* CountBitStream = sc.Convert_IntToBinaryStream(BinaryBitCount);*/

			char BlockNumCharArry[ALGHEAD_BLOCK_TOTALCOUNT + 1] = "";

			int BitDropOutNum = ALGHEAD_TYPE_LENGTH + ALGHEAD_CONTENT_LENGTH + ALGHEAD_BLOCK_NUM;
			int dropCounter = 0;		//��������������Ҫ�Ƚ��㷨��Ϣͷ��8bit����ȥ

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
							if (dropCounter < BitDropOutNum)
							{
								dropCounter++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ��8bit�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									BlockNumCharArry[HideCounter] = '0';
								}
								else
								{
									BlockNumCharArry[HideCounter] = '1';
								}
								HideCounter++;
							}
							if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
								break;
						}
						if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
							break;
					}
					if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
						break;
				}
				if (HideCounter == ALGHEAD_BLOCK_TOTALCOUNT)
					break;
			}
			StreamConvert sc;
			TotalBlockCount = (short)sc.Convert_BinaryStreamToChar(BlockNumCharArry);
		}

		char* BinaryStream = new char[BinaryBitCount];
		//��ȡǶ����Ϣ
		{
			int DropOutBitCount = ALGHEAD_TYPE_LENGTH + ALGHEAD_CONTENT_LENGTH + ALGHEAD_BLOCK_NUM + ALGHEAD_BLOCK_TOTALCOUNT;
			int dropCounter = 0;

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
							if (dropCounter < DropOutBitCount)
							{
								dropCounter++;
								continue;
							}
							else    //�������������С���㷨��Ϣͷ��48bit�����ѭ��ֱ����ȫ�����㷨��Ϣͷ
							{
								short bitFlg = blockp % 2;
								if (bitFlg == 0)
								{
									BinaryStream[HideCounter] = '0';
								}
								else
								{
									BinaryStream[HideCounter] = '1';
								}
								HideCounter++;
							}
							if (HideCounter == BinaryBitCount)
								break;
						}
						if (HideCounter == BinaryBitCount)
							break;
					}
					if (HideCounter == BinaryBitCount)
						break;
				}
				if (HideCounter == BinaryBitCount)
					break;
			}
		}

		//д���ļ�
		{
			StreamConvert sc;
			uint8_t* BitArray = new uint8_t[BinaryBitCount / 8];
			sc.byteStreamToBinaryString(BitArray, BinaryBitCount / 8, BinaryStream, BinaryBitCount, 1);

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
						SuffixArryCount = i+1;
						ContentArryCount = BinaryBitCount / 8 - SuffixArryCount;
						break;
					}
				}
				SuffixArry = new uint8_t[SuffixArryCount];
				for (int i = 0; i < SuffixArryCount; i++)
				{
					SuffixArry[i] = BitArray[i];
				}
				ContentArry = new uint8_t[ContentArryCount];
				for (int i = 0; i < ContentArryCount; i++)
				{
					ContentArry[i] = BitArray[i + SuffixArryCount];
				}
			}
			
			string fileName = fileList.HidenFilePath;
			string Suffix((char*)SuffixArry);
			string outPath = fileName + Suffix;

			BinaryFileSolver outSolver;
			outSolver.setOutFilePath(outPath);
			outSolver.AppendFile(ContentArry, ContentArryCount);
			delete[] BitArray;
			delete[] SuffixArry;
			delete[] ContentArry;
		}

		delete[] BinaryStream;
		fclose(file);
		//д��ԭͼƬ
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
		jpeg_write_coefficients(&coutfo, coeff_arrays);

		jpeg_finish_compress(&coutfo);
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_compress(&coutfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(outFile);

	}
	else if(fileList.CarrierImagePathList.size()==0)
	{
		//��Ҫ����
	}
	else
	{

	}
}

int LSBLossyImplantAlg::CalPayLoad(FILE * file)
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, TRUE);
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
					if (blockptr[bi] == 0)
						continue;
					HideCounter++;
				}
			}
		}
	}
	FinishDecompress(&cinfo);
	HideCounter -= ALGHEAD_TYPE_LENGTH;
	HideCounter -= ALGHEAD_CONTENT_LENGTH;
	HideCounter -= ALGHEAD_BLOCK_NUM;
	HideCounter -= ALGHEAD_BLOCK_TOTALCOUNT;
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



