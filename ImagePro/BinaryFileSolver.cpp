#include "stdafx.h"
#include "BinaryFileSolver.h"
#include <fstream>
#include <iostream>


int BinaryFileSolver::GetInFileByteCount()
{
	return this->inFileByteCount;
}

int BinaryFileSolver::GetOutFileByteCount()
{
	return this->outFileByteCount;
}

uint8_t * BinaryFileSolver::GetBinaryFileContent()
{
	return this->BinaryFileContent;
}


void BinaryFileSolver::setInFilePath(string filePath)
{
	this->inFilePath = filePath;
}

void BinaryFileSolver::setOutFilePath(string filePath)
{
	this->outFilePath = filePath;
}

void BinaryFileSolver::ClearBeforeRead()
{
	this->inFilePath = "";
	this->inFileByteCount = 0;
	delete[] BinaryFileContent;
	BinaryFileContent = nullptr;
}

BinaryFileSolver::BinaryFileSolver()
{
	
}


BinaryFileSolver::~BinaryFileSolver()
{

}

int BinaryFileSolver::LoadFile()
{
	ifstream input(inFilePath, ios::in | ios::binary | ios::ate);
	if (!input.is_open())
		return -1;
	this->inFileByteCount = input.tellg();
	input.seekg(0, ios::beg);
	BinaryFileContent = new uint8_t[inFileByteCount];
	input.read((char*)BinaryFileContent, inFileByteCount);
	input.close();
	return 0;
}
int BinaryFileSolver::AppendFile(uint8_t Content[], int ContentCount)
{
	ofstream out(outFilePath, ios::out | ios::binary | ios::app);
	if (!out.is_open())
		return -1;
	out.write((char*)Content, sizeof(uint8_t)*ContentCount);
	outFileByteCount += ContentCount;
	out.close();
	return 0;
}