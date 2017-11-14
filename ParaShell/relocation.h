#ifndef __RELOCATION_H__
#define __RELOCATION_H__

#include <exception>
#include <vector>
#include <windows.h>

#pragma pack(push)
#pragma pack(1)
#pragma warning(push)
#pragma warning(disable:4200)

/* ����б����ض�λ��ṹ */
struct Shell_MutatedRelocTab_NODE
{
	BYTE   type;
	DWORD  FirstTypeRVA;
	WORD   Offset[];
};

#pragma pack(4)

/* �ض���ԭʼ�ض�λ��ṹ */
struct IMAGE_BASE_RELOCATION2
{
	DWORD   VirtualAddress;
	DWORD   SizeOfBlock;
	WORD    TypeOffset[];
};
// typedef IMAGE_BASE_RELOCATION2 UNALIGNED * PIMAGE_BASE_RELOCATION2;
typedef IMAGE_BASE_RELOCATION2 *PIMAGE_BASE_RELOCATION2;

#pragma warning(pop)
#pragma pack(pop)

struct MutatedRelocTabInfo
{
	void*	pMutatedRelocTab;
	DWORD	nMutatedRelocTab;

	/*
	TODO:
	1. �쳣���
	*/
	MutatedRelocTabInfo(DWORD sz) :
		pMutatedRelocTab(0), nMutatedRelocTab(sz)
	{
		pMutatedRelocTab = new char[nMutatedRelocTab];
		if (pMutatedRelocTab)
			memset(pMutatedRelocTab, 0, nMutatedRelocTab);
	}

	~MutatedRelocTabInfo()
	{
		if (pMutatedRelocTab)
			delete[] pMutatedRelocTab;
	}
};
//typedef MutatedRelocTabInfo *PMutatedRelocTabInfo;

#pragma warning(push)
#pragma warning(disable:4290)

class RelocTab
{
public:
	RelocTab(void* pImageBase);
	
	bool reset(void* pImageBase);

	bool dumpInShellForm(void* pMem);

	DWORD getMutatedRelocTabSizeInShell();

	bool clrOriginalRelocTab(void* pImageBase);

private:
	
	struct IMAGE_BASE_RELOCATION_MUTATED
	{
		BYTE   type;
		DWORD  FirstTypeRVA;
		std::vector<WORD>   Offset;

		IMAGE_BASE_RELOCATION_MUTATED() :
			type(IMAGE_REL_BASED_ABSOLUTE),
			FirstTypeRVA(0),
			Offset()
		{}

		void clear()
		{
			type = IMAGE_REL_BASED_ABSOLUTE;
			FirstTypeRVA = 0;
			Offset.clear();
		}
	};

	bool marshallMutatedRelocTab(void* pImageBase) throw(std::exception);

	std::vector<IMAGE_BASE_RELOCATION_MUTATED> m_vMutatedRelocTab;
};

#pragma warning(pop)

#endif // __RELOCATION_H__