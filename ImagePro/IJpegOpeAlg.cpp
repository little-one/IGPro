#include "stdafx.h"
#include "IJpegOpeAlg.h"


IJpegOpeAlg::IJpegOpeAlg()
{
}

IJpegOpeAlg::~IJpegOpeAlg()
{
}

void IJpegOpeAlg::GetImageInfo(jpeg_decompress_struct * cinfo, JDIMENSION * ImageWidth, JDIMENSION * ImageHeight, int * ImageComponents, J_COLOR_SPACE * ImageColorSpace) 
{
	*ImageWidth = cinfo->image_width;
	*ImageHeight = cinfo->image_height;
	*ImageComponents = cinfo->num_components;
	*ImageColorSpace = cinfo->out_color_space;
}

void IJpegOpeAlg::SetImageInfo(jpeg_compress_struct * cinfo, const JDIMENSION ImageWidth, const JDIMENSION ImageHeight, const int ImageComponents, const J_COLOR_SPACE ImageColorSpace) 
{
	cinfo->image_width = ImageWidth;
	cinfo->image_height = ImageHeight;
	cinfo->input_components = ImageComponents;
	cinfo->in_color_space = ImageColorSpace;
	jpeg_set_defaults(cinfo);
}

FILE * IJpegOpeAlg::GetFile(char * filePath, char * fileMode)
{
	FILE* file;
	fopen_s(&file, filePath, fileMode);
	return file;
}

void IJpegOpeAlg::ClosFile(FILE * file)
{
	fclose(file);
}

jvirt_barray_ptr * IJpegOpeAlg::ReadImageByDCTcoefficient(jpeg_decompress_struct * cinfo)
{
	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(cinfo);
	return coeff_arrays;
}

void IJpegOpeAlg::ReadImageByRGB(jpeg_decompress_struct * cinfo, JSAMPLE** R, JSAMPLE ** G, JSAMPLE ** B, int * RGBLength)
{
	if (cinfo->out_color_space != JCS_RGB)
		return;
	jpeg_start_decompress(cinfo);
	int row_stride;
	JSAMPARRAY buffer;
	row_stride = cinfo->output_width * cinfo->output_components;
	*RGBLength = cinfo->output_width * cinfo->output_height;
	*R = new JSAMPLE[*RGBLength];
	*G = new JSAMPLE[*RGBLength];
	*B = new JSAMPLE[*RGBLength];
	buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr)cinfo, JPOOL_IMAGE, row_stride, 1);
	int counter = 0;
	while (cinfo->output_scanline < cinfo->output_height)
	{
		(void)jpeg_read_scanlines(cinfo, buffer, 1);
		for (int i = 0; i < cinfo->output_width; i++)
		{
			*R[counter*cinfo->output_width + i] = buffer[0][i * 3 + 0];
			*G[counter*cinfo->output_width + i] = buffer[0][i * 3 + 1];
			*B[counter*cinfo->output_width + i] = buffer[0][i * 3 + 2];
		}
	}
}

void IJpegOpeAlg::WriteImageByDCTcoefficient(jpeg_compress_struct * cinfo, jpeg_decompress_struct* tcinfo, jvirt_barray_ptr * coeff_array)
{
	jpeg_copy_critical_parameters(tcinfo, cinfo);
	jpeg_write_coefficients(cinfo, coeff_array);
}

void IJpegOpeAlg::WriteImageByRGB(jpeg_compress_struct * cinfo, const JSAMPLE * R, const JSAMPLE * G, const JSAMPLE * B)
{
	if (cinfo->in_color_space != JCS_RGB)
		return;
	jpeg_start_compress(cinfo,TRUE);
	JSAMPROW row_pointer[1];
	int row_stride = cinfo->image_width*cinfo->input_components;
	row_pointer[0] = new JSAMPLE[row_stride];
	int counter = 0;
	while (cinfo->next_scanline < cinfo->image_height)
	{
		for (int i = 0; i < cinfo->image_width; i++)
		{
			row_pointer[0][i * 3 + 0] = R[i];
			row_pointer[0][i * 3 + 1] = G[i];
			row_pointer[0][i * 3 + 2] = B[i];
			(void)jpeg_write_scanlines(cinfo, row_pointer, 1);
		}
	}
}

void IJpegOpeAlg::FinishDecompress(jpeg_decompress_struct * cinfo)
{
	jpeg_finish_decompress(cinfo);
	jpeg_destroy_decompress(cinfo);
}

void IJpegOpeAlg::FinishCompress(jpeg_compress_struct * cinfo)
{
	jpeg_finish_compress(cinfo);
	jpeg_destroy_compress(cinfo);
}

jvirt_barray_ptr * IJpegOpeAlg::WriteAlgHead(jvirt_barray_ptr * coeff_arrays, jpeg_decompress_struct* cinfo, ALGORITHM_NAME AlgType)
{
	//将AlgType转为二进制的八位数用char表示
	Encrypt_Decrpty ed;
	uint8_t chararr[ALGHEAD_TYPE_LENGTH / 8] = { AlgType };
	char binaryarr[ALGHEAD_TYPE_LENGTH];
	ed.byteStream2BinaryString(chararr, ALGHEAD_TYPE_LENGTH / 8, binaryarr, ALGHEAD_TYPE_LENGTH, 0);

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
					if (HideCounter == ALGHEAD_TYPE_LENGTH)
						break;
					if (blockptr[bi] == 0)
						continue;
					short blockp = blockptr[bi];
					switch (binaryarr[HideCounter])
					{
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
					case '1':
						if (blockptr[bi] % 2 == 0)
						{
							blockptr[bi] += 1;
						}
						break;
					default:
						break;
					}
					HideCounter++;
				}
				if (HideCounter == ALGHEAD_TYPE_LENGTH)
					break;
			}
			if (HideCounter == ALGHEAD_TYPE_LENGTH)
				break;
		}
		if (HideCounter == ALGHEAD_TYPE_LENGTH)
			break;
	}
	return coeff_arrays;
}

ALGORITHM_NAME IJpegOpeAlg::ReadAlgHead(jvirt_barray_ptr * coeff_arrays, jpeg_decompress_struct* cinfo)
{
	char AlgHeadInfo[ALGHEAD_TYPE_LENGTH];
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
					if (HideCounter == ALGHEAD_TYPE_LENGTH)
						break;
					if (blockptr[bi] == 0)
						continue;
					short blockp = blockptr[bi];
					short bitFlg = blockp % 2;
					if (bitFlg == 0)
					{
						AlgHeadInfo[HideCounter] = '0';
					}
					else
					{
						AlgHeadInfo[HideCounter] = '1';
					}
					HideCounter++;
				}
				if (HideCounter == ALGHEAD_TYPE_LENGTH)
					break;
			}
			if (HideCounter == ALGHEAD_TYPE_LENGTH)
				break;
		}
		if (HideCounter == ALGHEAD_TYPE_LENGTH)
			break;
	}
	uint8_t type[ALGHEAD_TYPE_LENGTH / 8];
	Encrypt_Decrpty ed;
	ed.byteStream2BinaryString(type, ALGHEAD_TYPE_LENGTH / 8, AlgHeadInfo, ALGHEAD_TYPE_LENGTH, 1);
	return (ALGORITHM_NAME)type[ALGHEAD_TYPE_LENGTH / 8 - 1];
}

