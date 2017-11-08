#ifndef __IMPORT_H__
#define __IMPORT_H__

#include <Windows.h>
#include <string>
#include <vector>

#pragma pack(push)
#pragma pack(1)
#pragma warning(push)
#pragma warning(disable:4200)

/* ����б��������ṹ */

struct Shell_MutatedImpTab_DLLNode_APINode
{
	union
	{
		DWORD Ordinal;
		BYTE ProcName[32];
	};
};

struct Shell_MutatedImpTab_DLLNode
{
	DWORD FirstThunk;
	BYTE DLLName[32];
	DWORD nFunc;
	Shell_MutatedImpTab_DLLNode_APINode FuncName[];
};

#pragma warning(pop)
#pragma pack(pop)

struct MutatedImpTabInfo
{
	void *pMutatedImpTab;
	unsigned long nMutatedImpTab;

	/*
	TODO:
	1. �쳣���
	*/
	MutatedImpTabInfo(unsigned long sz) :
		pMutatedImpTab(0), nMutatedImpTab(sz)
	{
		pMutatedImpTab = new char[nMutatedImpTab];
		memset(pMutatedImpTab, 0, nMutatedImpTab);
	}

	~MutatedImpTabInfo()
	{
		delete[] pMutatedImpTab;
	}
};
//typedef MutatedImpTabInfo *PMutatedImpTabInfo;

class ImpTab
{
public:
	/*
	description:	ctor,��ȡԭʼ�������ʼ���������������
	params:			[in]void* pImageBase
	Todo:
	*	1.�����⣬�׳��쳣
	*/
	ImpTab(void* pImageBase);

	/*
	description:	�ѱ����������������ǽṹ��ʽת�浽�ڴ���
	params:			[in]void* pMem
	returns:		bool
	*/
	bool dumpInShellForm(void* pMem);

	/*
	description:	���¶���ȥԭʼ��������ݣ���ʼ���������������
	params:			[in]void* pImageBase
	returns:		bool
	*/
	bool reset(void* pImageBase);

	/*
	description:	��ȡ���������������д�С
	returns:		��С
	*/
	DWORD getMutatedImpTabSizeInShell();

	/*
	description:	���ԭʼ���������
	params:			[in]void* pImageBase
	returns:		bool
	*/
	bool clrOriginalImpTab(void* pImageBase);

private:
	struct MutatedImpTab_DLLNode_APINode
	{
		DWORD		Ordinal;
		std::string	APIName;

		MutatedImpTab_DLLNode_APINode() :
			Ordinal(0), APIName()
		{}

		void clear()
		{
			Ordinal = 0;
			APIName.clear();
		}

		bool isString()
		{
			return (0 == Ordinal && APIName.size());
		}
	};

	struct MutatedImpTab_DLLNode
	{
		DWORD	FirstThunk;
		std::string	DLLName;
		std::vector<MutatedImpTab_DLLNode_APINode>	vThunks;

		MutatedImpTab_DLLNode() :
			FirstThunk(0), DLLName(), vThunks()
		{}

		void clear()
		{
			FirstThunk = 0;
			DLLName.clear();
			vThunks.clear();
		}
	};
	typedef MutatedImpTab_DLLNode UNALIGNED *PMutatedImpTab_DLLNode;

	/*
	description:	��ȡ��������ݵ�����(�����ʽ)
	params:			[in]void* pImageBase	// �ļ��ڴ����ָ��
	returns:		bool
	*/
	bool marshallMutatedImpTab(void* pImageBase);
	
	std::vector<MutatedImpTab_DLLNode> m_vMutatedImpTab;
};

#endif // __IMPORT_H__
