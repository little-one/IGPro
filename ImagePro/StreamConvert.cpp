#include "stdafx.h"
#include "StreamConvert.h"
#include <vector>
//
/******************************************************************************************************************/
/*�������ƣ� byteStream2BinaryString()													     				      */
/*�������ܣ� ��һ���ַ����ֽ�תΪ�����������������߽�һ��������������ÿ�˸�һ��ת��һ���ַ�					      */
/*�������أ� -1��ʾ�쳣��0��ʾ����																			      */
/*byteBuf[]������ַ����ݵ�����																				      */
/*byteLen��	 ���type=0���ʾ�ܹ��ж��ٸ���Ҫת�ɶ����Ƶ��ַ������type=1���ʾת����ɺ󹲵õ����ٸ��ַ�		  */
/*charBuf[]����Ŷ��������е����飬ÿ8��char��ʾbyteBuf�е�һ��uint8_t�ַ�									      */
/*charLen��  ���type=0���ʾת����ɺ󹲵õ�����λ�������������type=1���ʾ�ܹ��ж���λ����������Ҫת�����ַ�   */
/*type��     ֵλ0��1��ֵΪ0ʱ���ַ���ת�ɶ���������ֵΪ1ʱ����������תΪ�ַ���								      */
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
/*�������ƣ� bitConvert_BinaryToNDecimal													     							*/
/*�������ܣ� ��һ���ַ����ֽ�תΪ�����������������߽�һ��������������ÿ�˸�һ��ת��һ���ַ�								    */
/*�������أ� �����򷵻�0���쳣�򷵻�-1																						*/
/*Binary[]����Ŷ�������������																							    */
/*BinaryBitCount��	 ����������bit��																					    */
/*NDecimal[]�����N������������																								*/
/*NDecimalBitCount��  N��������bit��																					    */
/*type��     ֵλ0��1��ֵΪ0ʱ����������תΪN��������ֵΪ1ʱ��N������תΪ��������											*/
/*�ر�˵�����������������������λ������Ϊ8�ı���(Ĭ���������ֽ�ת�������Ķ������������Ա�Ϊ8�ı���)�����򷵻�-1			*/
/****************************************************************************************************************************/
int StreamConvert::bitConvert_BinaryToNDecimal(char Binary[], int BinaryBitCount, char NDecimal[], int NDecimalBitCount, int type)
{
	if (BinaryBitCount % 8 != 0)
		return -1;
	int DBit = this->BinaryBit;
	int NBit = this->N_DecimalBit;
	
	if (type == 0)		//������תN���Ʋ�д��NDecimal��
	{
		if (BinaryBit == -1 || N_DecimalBit == -1)
			return -1;
		int GroupCount = ceil((1.0*BinaryBitCount) / (1.0*DBit));
		//�Զ�������������һ����������������ܱ�DBit������������룬֮��Ĳ�������BinaryStreamִ��
		vector<char> BinaryStream(Binary, Binary + BinaryBitCount);
		if (BinaryBitCount%DBit != 0)
		{
			for (int i = 0; i < DBit - BinaryBitCount%DBit; i++)
				BinaryStream.push_back('0');
		}

		for (int i = 0; i < GroupCount; i += 1)
		{
			int DBasePosition = i*DBit;		//ÿ��������ױ���λ
			int NBasePosition = i*NBit;
			int DecimalNum = 0;
			for (int t = DBit - 1; t >= 0; t--)		//���÷���Ķ�����bit���մ�˷�ʽ�����ʮ������
			{
				DecimalNum += pow(2, (DBit - t - 1))*(BinaryStream[DBasePosition + t] - '0');
			}
			for (int t = 0; t < NBit; t++)
			{
				int bitN = pow(this->K, NBit - t - 1);
				if (NBasePosition + t < NDecimalBitCount)	//����Խ�籣��
				{
					NDecimal[NBasePosition + t] = DecimalNum / bitN + '0';
					DecimalNum = DecimalNum % bitN;
				}
				else
					return -1;
			}
		}
	}
	else if (type == 1)		//N����ת�����Ʋ�д��Binary��
	{
		if (NDecimalBitCount % this->N_DecimalBit != 0)
			return -1;
		int GroupCount = ceil((1.0*NDecimalBitCount) / (1.0*NBit));
		vector<char> BinaryStream;
		for (int i = 0; i < GroupCount; i++)
		{
			int NBasePosition = i*NBit;
			int DecimalNum = 0;
			for (int t = NBit - 1; t >= 0; t--)			//���÷����N����bit����˷�ʽ���ʮ������
			{
				DecimalNum += pow(this->K, (NBit - t - 1))*(NDecimal[NBasePosition + t] - '0');
			}
			for (int t = 0; t < DBit; t++)				//תΪ�����ƺ���λ����BinaryStream��
			{
				int bitD = pow(2, DBit - t - 1);
				BinaryStream.push_back(DecimalNum / bitD + '0');
				DecimalNum = DecimalNum % bitD;
			}
		}
		if (BinaryBitCount > BinaryStream.size())
			return -1;
		for (int i = 0; i < BinaryBitCount; i++)	//�������������ַ���λд��Binary�����У�BinaryStream�ж���Ĳ���λ�Զ����޳�
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
	/*		n		������		N����		 */
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
pair<int, int> StreamConvert::GetBitRepresentation(int N)		//���ض�������N���ƻ�תʱ���ֽ��Ƶ�λ���Ķ�Ӧ��ϵ,��BitMap�в�����N����ת�����򷵻�pair<int,int>(-1,-1)
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

