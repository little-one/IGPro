#include "stdafx.h"
#include "StreamConvert.h"
#include <vector>
//
/******************************************************************************************************************/
/*函数名称： byteStream2BinaryString()													     				      */
/*函数功能： 将一串字符逐字节转为二进制数字流，或者将一串二进制数字流每八个一组转成一串字符					      */
/*函数返回： -1表示异常，0表示正常																			      */
/*byteBuf[]：存放字符数据的数组																				      */
/*byteLen：	 如果type=0则表示总共有多少个需要转成二进制的字符，如果type=1则表示转换完成后共得到多少个字符		  */
/*charBuf[]：存放二进制序列的数组，每8个char表示byteBuf中的一个uint8_t字符									      */
/*charLen：  如果type=0则表示转换完成后共得到多少位二进制数，如果type=1则表示总共有多少位二进制数需要转换成字符   */
/*type：     值位0或1，值为0时将字符串转成二进制流，值为1时将二进制流转为字符串								      */
/******************************************************************************************************************/
int StreamConvert::byteStreamToBinaryString(uint8_t byteBuf[], int byteLen, char charBuf[], int charLen, int type)
{
	switch (type)
	{
	case 0:
		if (charLen < 8 * byteLen)
			return -1;
		else
		{
			int charBufIndex = 0;
			for (int i = 0; i < byteLen; i++)
				for (int j = 7; j >= 0; j--)
				{
					if (byteBuf[i] >> j & 0x1)
						charBuf[charBufIndex++] = '1';
					else
						charBuf[charBufIndex++] = '0';
				}
		}
		break;
	case 1:
		memset(byteBuf, 0, byteLen);
		if (8 * byteLen < charLen)
			return -1;
		char tmpStr[9] = "";
		int byteBufIndex = 0;
		for (int i = 0; i < charLen; i += 8)
		{
			strncpy_s(tmpStr, charBuf + i, 8);
			for (int j = 0; j < 8; j++)
				byteBuf[byteBufIndex] += tmpStr[j] - '0' << 7 - j;
			++byteBufIndex;
		}
		break;
	}
	return 0;
}

/****************************************************************************************************************************/
/*函数名称： bitConvert_BinaryToNDecimal													     							*/
/*函数功能： 将一串字符逐字节转为二进制数字流，或者将一串二进制数字流每八个一组转成一串字符								    */
/*函数返回： 正常则返回0，异常则返回-1																						*/
/*Binary[]：存放二进制流的数组																							    */
/*BinaryBitCount：	 二进制流的bit数																					    */
/*NDecimal[]：存放N进制流的数组																								*/
/*NDecimalBitCount：  N进制流的bit数																					    */
/*type：     值位0或1，值为0时将二进制流转为N进制流，值为1时将N进制流转为二进制流											*/
/*特别说明！！！！！二进制数组的位数必须为8的倍数(默认是由逐字节转换得来的二进制流，所以必为8的倍数)，否则返回-1			*/
/****************************************************************************************************************************/
int StreamConvert::bitConvert_BinaryToNDecimal(char Binary[], int BinaryBitCount, char NDecimal[], int NDecimalBitCount, int type)
{
	if (BinaryBitCount % 8 != 0)
		return -1;
	int DBit = this->BinaryBit;
	int NBit = this->N_DecimalBit;
	
	if (type == 0)		//二进制转N进制并写入NDecimal中
	{
		if (BinaryBit == -1 || N_DecimalBit == -1)
			return -1;
		int GroupCount = ceil((1.0*BinaryBitCount) / (1.0*DBit));
		//对二进制流数据做一个拷贝，如果它不能被DBit整除，则补零对齐，之后的操作都对BinaryStream执行
		vector<char> BinaryStream(Binary, Binary + BinaryBitCount);
		if (BinaryBitCount%DBit != 0)
		{
			for (int i = 0; i < DBit - BinaryBitCount%DBit; i++)
				BinaryStream.push_back('0');
		}

		for (int i = 0; i < GroupCount; i += 1)
		{
			int DBasePosition = i*DBit;		//每个分组的首比特位
			int NBasePosition = i*NBit;
			int DecimalNum = 0;
			for (int t = DBit - 1; t >= 0; t--)		//将该分组的二进制bit按照大端方式解出其十进制数
			{
				DecimalNum += pow(2, (DBit - t - 1))*(BinaryStream[DBasePosition + t] - '0');
			}
			for (int t = 0; t < NBit; t++)
			{
				int bitN = pow(this->K, NBit - t - 1);
				if (NBasePosition + t < NDecimalBitCount)	//数组越界保护
				{
					NDecimal[NBasePosition + t] = DecimalNum / bitN + '0';
					DecimalNum = DecimalNum % bitN;
				}
				else
					return -1;
			}
		}
	}
	else if (type == 1)		//N进制转二进制并写入Binary中
	{
		if (NDecimalBitCount % this->N_DecimalBit != 0)
			return -1;
		int GroupCount = ceil((1.0*NDecimalBitCount) / (1.0*NBit));
		vector<char> BinaryStream;
		for (int i = 0; i < GroupCount; i++)
		{
			int NBasePosition = i*NBit;
			int DecimalNum = 0;
			for (int t = NBit - 1; t >= 0; t--)			//将该分组的N进制bit按大端方式解出十进制数
			{
				DecimalNum += pow(this->K, (NBit - t - 1))*(NDecimal[NBasePosition + t] - '0');
			}
			for (int t = 0; t < DBit; t++)				//转为二进制后逐位存入BinaryStream中
			{
				int bitD = pow(2, DBit - t - 1);
				BinaryStream.push_back(DecimalNum / bitD + '0');
				DecimalNum = DecimalNum % bitD;
			}
		}
		if (BinaryBitCount > BinaryStream.size())
			return -1;
		for (int i = 0; i < BinaryBitCount; i++)	//将二进制流中字符逐位写入Binary数组中，BinaryStream中多余的补零位自动被剔除
			Binary[i] = BinaryStream[i];
		
	}
	else
		return -1;
	return 0;
}

int StreamConvert::Cal_BinaryBitCount(int NDecimalBitCount)
{
	if (this->BinaryBit == -1 || this->N_DecimalBit == -1)
		return -1;
	if (NDecimalBitCount%this->N_DecimalBit != 0)
		return -1;
	return this->BinaryBit * NDecimalBitCount / this->N_DecimalBit / 8 * 8;
}
int StreamConvert::Cal_NDecimalBitCount(int BinaryBitCount)
{
	if (this->BinaryBit == -1 || this->N_DecimalBit == -1)
		return -1;
	if (BinaryBitCount%BinaryBit == 0)
		return BinaryBitCount / this->BinaryBit * this->N_DecimalBit;
	else
		return (BinaryBitCount / this->BinaryBit + 1) * this->N_DecimalBit;
}

StreamConvert::StreamConvert()
{
	InitializeBitMap();
}
StreamConvert::~StreamConvert()
{
}

StreamConvert::StreamConvert(int N)
{
	InitializeBitMap();
	this->K = N;
	pair<int, int> temp = GetBitRepresentation(N);
	this->BinaryBit = temp.first;
	this->N_DecimalBit = temp.second;
}
inline void StreamConvert::InitializeBitMap()
{
	/*****************************************/
	/*		n		二进制		N进制		 */
	/*		3		   3          2		     */
	/*		4		   2          1		     */
	/*		5		   2          1		     */
	/*		6		   5          2		     */
	/*		8		   3          1		     */
	/*		12		   7          2		     */
	/*		16		   4          1		     */
	/*****************************************/
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(2, pair<int, int>(1, 1)));
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(3, pair<int, int>(3, 2)));
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(4, pair<int, int>(2, 1)));
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(5, pair<int, int>(2, 1)));
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(6, pair<int, int>(5, 2)));
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(8, pair<int, int>(3, 1)));
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(12, pair<int, int>(7, 2)));
	this->BitRepresentationMap.insert(pair<int, pair<int, int> >(16, pair<int, int>(4, 1)));
}
pair<int, int> StreamConvert::GetBitRepresentation(int N)		//返回二进制与N进制互转时两种进制的位数的对应关系,若BitMap中不存在N进制转换，则返回pair<int,int>(-1,-1)
{
	map<int, pair<int, int> >::iterator itor = this->BitRepresentationMap.find(N);
	if (itor != this->BitRepresentationMap.end())
		return this->BitRepresentationMap[N];
	else
		return pair<int, int>(-1, -1);
}

void StreamConvert::Convert_IntToBinaryStream(int num, char** BinaryStream)
{
	*BinaryStream = new char[BITCOUNT_INT+1];
	(*BinaryStream)[BITCOUNT_INT] = '\0';
	int counter = 0;
	for (int i = BITCOUNT_INT - 1; i >= 0; i--)
	{
		if (num >> i & 0x1)
			(*BinaryStream)[counter++] = '1';
		else
			(*BinaryStream)[counter++] = '0';
	}
}
int StreamConvert::Convert_BinaryStreamToInt(char * BStream)
{
	int num = 0;
	int counter = 0;
	char tmpStr[BITCOUNT_INT + 1] = "";
	strncpy_s(tmpStr, BStream, BITCOUNT_INT);
	for (int i = 0; i < BITCOUNT_INT; i++)
		num += tmpStr[i] - '0' << (BITCOUNT_INT - 1) - i;
	return num;
}

void StreamConvert::Convert_CharToBinaryStream(char num, char** BinaryStream)
{
	*BinaryStream = new char[8 * sizeof(char) + 1];
	(*BinaryStream)[8 * sizeof(char)] = '\0';
	int counter = 0;
	for (int i = 8 * sizeof(char) - 1; i >= 0; i--)
	{
		if (num >> i & 0x1)
			(*BinaryStream)[counter++] = '1';
		else
			(*BinaryStream)[counter++] = '0';
	}
}
char StreamConvert::Convert_BinaryStreamToChar(char * BStream)
{
	int num = 0;
	//int counter = 0;
	char tmpStr[8 * sizeof(char) + 1] = "";
	strncpy_s(tmpStr, BStream, 8 * sizeof(char));
	for (int i = 0; i < 8 * sizeof(char); i++)
		num += tmpStr[i] - '0' << (8 * sizeof(char) - 1) - i;
	char cnum = (char)num;
	return cnum;
}

void StreamConvert::Convert_HalfCharToBinaryStream(char num, char** BinaryStream)
{
	*BinaryStream = new char[BITCOUNT_HALFCHAR+1];
	(*BinaryStream)[BITCOUNT_HALFCHAR] = '\0';
	int counter = 0;
	for (int i = BITCOUNT_HALFCHAR - 1; i >= 0; i--)
	{
		if (num >> i & 0x1)
			(*BinaryStream)[counter++] = '1';
		else
			(*BinaryStream)[counter++] = '0';
	}
}
char StreamConvert::Convert_BinaryStreamToHalfChar(char * BStream)
{
	char num = 0;
	int counter = 0;
	char tmpStr[BITCOUNT_HALFCHAR + 1] = "";
	strncpy_s(tmpStr, BStream, BITCOUNT_HALFCHAR);
	for (int i = 0; i < BITCOUNT_HALFCHAR; i++)
	{
		num += (tmpStr[i] - '0') << ((BITCOUNT_HALFCHAR - 1) - i);
	}
	return num;
}

int StreamConvert::Convert_CharToInt(char cnum)
{
	int num = (int)cnum;
	if (num < 0)
		num = 256 + num;
	return num;
}

