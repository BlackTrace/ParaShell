#include <cstdlib>
#include <windows.h>
#include "pe_utilities.h"
#include "error.h"

/*
	Description:	ȡ�����뺯��
*/
UINT AlignSize(UINT nSize, UINT nAlign)
{
	return ((nSize + nAlign - 1) / nAlign * nAlign);
}


/* 
	Description:	RVA->ָ����ж�Ӧλ�õ�ָ��						   
*/
char* RVAToPtr(const void* imagebase, const DWORD dwRVA)
{
	return ((char*)imagebase + dwRVA);
}


/*
	Description:	��ȡNTͷָ��
*/
PIMAGE_NT_HEADERS getNTHeader(const void* imagebase)
{
	return (PIMAGE_NT_HEADERS)((char*)imagebase + ((PIMAGE_DOS_HEADER)imagebase)->e_lfanew);
}

/*
	Description:	��ȡsection��ָ��
*/
PIMAGE_SECTION_HEADER getSecHeader(const void* _imagebase)
{
	PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)getNTHeader(_imagebase);

	return (PIMAGE_SECTION_HEADER)((char*)pNTHeaders + sizeof(IMAGE_NT_HEADERS));
	
}


/*
	Description:	��ȡ���һ���������ָ��
*/
PIMAGE_SECTION_HEADER getLastSecHeader(const void* _pImageBase)
{
	PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)getNTHeader(_pImageBase);
	PIMAGE_SECTION_HEADER pSecHeader = getSecHeader(_pImageBase);

	pSecHeader += pNTHeaders->FileHeader.NumberOfSections - 1;

	return pSecHeader;
}


/*
	Description:	������ȥ��β�����õ����ֽڣ����¼�������Ĵ�С             
*/
unsigned int CalcMinSizeOfData(char* pSectionData, const unsigned int nSectionSize)
{

	if (IsBadReadPtr(pSectionData, nSectionSize))
	{
		return nSectionSize;
	}

	char*	pData = pSectionData + nSectionSize - 1;
	unsigned int	nSize = nSectionSize;

	while (nSize > 0 && *pData == 0)
	{
		pData--;
		nSize--;
	}

	return nSize;
}


const int nListNum = 6;
const char* szSecNameList[nListNum] =
{
	".text",
	".data",
	".rdata",
	"CODE",
	"DATA",
	".reloc"
};
/*
	Description:	�жϵ�ǰ���������ܷ�ѹ��
*/
bool IsSectionPackable(PIMAGE_SECTION_HEADER pSecHeader)
{
	// �������ƥ����������ƣ����ʾ���������ѹ��
	for (UINT nIndex = 0; nIndex < nListNum; nIndex++)
	{

		/*��Щ�������ܻ���.rdata�����飬�������ϲ��˾Ͳ��������ж���
		if (!IsMergeSection)
		{
			if ((nExportAddress >= pSecHeader->VirtualAddress) && (nExportAddress < (pSecHeader->VirtualAddress + pSecHeader->Misc.VirtualSize)))
				return FALSE;
		}
		*/

		if (strncmp((char *)pSecHeader->Name, szSecNameList[nIndex], strlen(szSecNameList[nIndex])) == 0)
		{
			return true;
		}
	}

	return false;
}


/*
	Description:	�����ļ�
*/
int BackUpFile(TCHAR *szFilePath)
{
	TCHAR *szFilebakName = new TCHAR[MAX_PATH * sizeof(TCHAR)];
	
	ZeroMemory(szFilebakName, MAX_PATH * sizeof(TCHAR));

	lstrcpy(szFilebakName, szFilePath);
	lstrcat(szFilebakName, TEXT(".bak"));
	CopyFile(szFilePath, szFilebakName, FALSE);

	delete []szFilebakName;

	return ERR_SUCCESS;
}

/*
	Description:	��ȡDOSͷ��С
*/
unsigned int GetDosHeaderSize(void* _pImageBase)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)_pImageBase;

	return pDosHeader->e_lfanew;
}


/*
	Description:	��ȡNTͷ��С
*/
unsigned int GetNTHeaderSize(void* _pImageBase)
{
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)getNTHeader(_pImageBase);

	unsigned int NTHeaderSize = sizeof(pNTHeader->Signature) + sizeof(pNTHeader->FileHeader) + pNTHeader->FileHeader.SizeOfOptionalHeader;

	return NTHeaderSize;
}


/*
	Description:	��ȡ������С
*/
unsigned int GetSectionTableSize(void* _pImageBase)
{
	// TODO
	return ERR_SUCCESS;
}


/*
	Description:	�������������������,new�����������ڴ棬��Ҫ������delete
*/
unsigned int CreateNewSection(void* _pImageBase, const DWORD _secsize, void **_ppNewSection)
{
	PIMAGE_NT_HEADERS pNTHeader = getNTHeader(_pImageBase);
	PIMAGE_SECTION_HEADER pNewSecHeader = getLastSecHeader(_pImageBase) + 1;
	PIMAGE_SECTION_HEADER pLastValidSecHeader = getLastSecHeader(_pImageBase);

	/*  ���������������ƶ�  */
	/* �����һ�����鿪ʼ�����һ�������ƶ�*/
	/*
	for (int i = pNTHeader->FileHeader.NumberOfSections; i > 0; i--, pLastValidSecHeader--)
	{
		memcpy(pLastValidSecHeader + 1, pLastValidSecHeader, sizeof(IMAGE_SECTION_HEADER));
	}
	*/

	/* 
		��Щ�ļ��������ǿ����顣��.textbbs 
		��ҪŲ�����һ����Ч������
	*/
	while (0 == pLastValidSecHeader->PointerToRawData)
	{
		pLastValidSecHeader--;
	}

	/*  ��д��������Ϣ  */
	memset(pNewSecHeader, 0, sizeof(IMAGE_SECTION_HEADER));
	/* Name, VirtualAddress, VirtualSize, RawAddress, RawSize, Characteristics */
	const char newsecname[8] = { ".shell" };
	memcpy(pNewSecHeader->Name, newsecname, 8);
	pNewSecHeader->VirtualAddress = pLastValidSecHeader->VirtualAddress + AlignSize(pLastValidSecHeader->Misc.VirtualSize, pNTHeader->OptionalHeader.SectionAlignment);
	pNewSecHeader->Misc.VirtualSize = AlignSize(_secsize, pNTHeader->OptionalHeader.SectionAlignment);
	pNewSecHeader->PointerToRawData = pLastValidSecHeader->PointerToRawData + AlignSize(pLastValidSecHeader->SizeOfRawData, pNTHeader->OptionalHeader.FileAlignment);
	pNewSecHeader->SizeOfRawData = AlignSize(_secsize, pNTHeader->OptionalHeader.FileAlignment);
	pNewSecHeader->Characteristics = 0xE0000020;


	/*  �����������ڴ�  */
	DWORD ulNewSecSize = AlignSize(_secsize, pNTHeader->OptionalHeader.SectionAlignment);
	void* ptr = new char[ulNewSecSize];
	*_ppNewSection = ptr;
	memset(*_ppNewSection, 0, ulNewSecSize);


	/*  �޸�PEͷ�����  */
	/* SizeOfImage, NumberOfSections, SizeOfCode */
	pNTHeader->OptionalHeader.SizeOfImage = AlignSize(pNTHeader->OptionalHeader.SizeOfImage + ulNewSecSize, pNTHeader->OptionalHeader.SectionAlignment);
	pNTHeader->FileHeader.NumberOfSections++;
	pNTHeader->OptionalHeader.SizeOfCode += ulNewSecSize;

	return ERR_SUCCESS;
}


/*
	Description:	��������ڴ���ںϵ�һ��
*/
void* MergeMemBlock(void* _pImageBase, void* _pShellSection)
{
	PIMAGE_NT_HEADERS pNTHeader = getNTHeader(_pImageBase);
	PIMAGE_SECTION_HEADER pShellSecHeader = getLastSecHeader(_pImageBase);
	DWORD ulNewImageSize = pNTHeader->OptionalHeader.SizeOfImage;
	DWORD ulOriginalImageSize = ulNewImageSize - AlignSize(pShellSecHeader->Misc.VirtualSize, pNTHeader->OptionalHeader.SectionAlignment);
	DWORD ulShellSize = pShellSecHeader->SizeOfRawData;

	// ������ӳ����ڴ�ռ�
	void* pNewMemBlock = new unsigned char[ulNewImageSize];
	memset(pNewMemBlock, 0, ulNewImageSize);

	// ����ԭImageBase
	memcpy(pNewMemBlock, _pImageBase, ulOriginalImageSize);

	// ����ShellSection
	void* pNewShellPosition = (void*)((DWORD)pNewMemBlock + ulOriginalImageSize);
	memcpy(pNewShellPosition, _pShellSection, ulShellSize);

	return pNewMemBlock;
}

/*
	Description:	��ָ������������Ϊ��д
*/
bool MakeSecWritable(void *_pImageBase, DWORD Offset)
{
	if (!_pImageBase || !Offset)
	{
		return false;
	}

	PIMAGE_NT_HEADERS pNTHeader = getNTHeader(_pImageBase);
	PIMAGE_SECTION_HEADER pSecHeader = getSecHeader(_pImageBase);

	while (!(
		Offset >= pSecHeader->VirtualAddress \
		&& Offset <= (pSecHeader->VirtualAddress + pSecHeader->Misc.VirtualSize)))
	{
		pSecHeader++;
	}
	if (Offset >= pSecHeader->VirtualAddress \
		&& Offset <= (pSecHeader->VirtualAddress + pSecHeader->Misc.VirtualSize))
	{
		pSecHeader->Characteristics |= IMAGE_SCN_MEM_WRITE;
	}

	return true;
}