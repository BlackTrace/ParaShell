#include "import.h"
#include "error.h"
#include "pe_utilities.h"


/*
description:	ctor,��ȡԭʼ�������ʼ���������������
params:			[in]void* pImageBase
*/
ImpTab::ImpTab(void* pImageBase) :
	m_vMutatedImpTab()
{
	marshallMutatedImpTab(pImageBase);
}

/*
description:	�ѱ����������������ǽṹ��ʽת�浽�ڴ���
params:			[in]void* pMem
returns:		bool
*/
bool ImpTab::dumpInShellForm(void* pMem)
{
	if (!pMem)
	{
		return false;
	}

	/*  Ϊ�������������ڴ�ռ䣬����������Ա�����ʽд������ڴ�ռ���  */
	Shell_MutatedImpTab_DLLNode* pData = (Shell_MutatedImpTab_DLLNode*)pMem;
	for (std::vector<MutatedImpTab_DLLNode>::iterator iterD = m_vMutatedImpTab.begin(); iterD < m_vMutatedImpTab.end(); iterD++)
	{
		/* FirstThunk */
		pData->FirstThunk = iterD->FirstThunk;
		
		/* DLLName */
		memset(pData->DLLName, 0, sizeof(pData->DLLName));
		if (iterD->DLLName.size() >= sizeof(pData->DLLName))
		{
			return false;
		}
		iterD->DLLName.copy((char*)(pData->DLLName), iterD->DLLName.size());
		
		/* nFunc */
		pData->nFunc = iterD->vThunks.size();

		/* FuncName */
		int i = 0;
		for (std::vector<MutatedImpTab_DLLNode_APINode>::iterator iterT = iterD->vThunks.begin(); 
			iterT < iterD->vThunks.end(); i++, iterT++)
		{
			memset(&pData->FuncName[i], 0, sizeof(pData->FuncName[i]));
			if (iterT->isString())
			{	// string
				if (iterT->APIName.size() >= sizeof(pData->FuncName[i]))
				{
					return false;
				}
				iterT->APIName.copy((char*)(pData->FuncName[i].ProcName), iterT->APIName.size());
			}
			else
			{	// ordinal
				pData->FuncName[i].Ordinal = iterT->Ordinal;
			}
		}
		pData = (Shell_MutatedImpTab_DLLNode*)(
			(DWORD)&pData->FuncName[i-1] 
			+ sizeof(pData->FuncName[i-1]));
	}

	return true;
}

/*
description:	���¶���ȥԭʼ��������ݣ���ʼ���������������
params:			[in]void* pImageBase
returns:		bool
*/
bool ImpTab::reset(void* pImageBase)
{
	if (!pImageBase)
	{
		return false;
	}

	m_vMutatedImpTab.clear();
	marshallMutatedImpTab(pImageBase);

	return true;
}

/*
description:	��ȡ���������������д�С
returns:		��С
*/
DWORD ImpTab::getMutatedImpTabSizeInShell()
{
	DWORD dwMutateImpSize = 0;

	// DLLNode�ܴ�С
	// Ԥ��һ����λ��С���սڵ�
	dwMutateImpSize += (m_vMutatedImpTab.size() + 1) * (2 * sizeof(DWORD) + 32 * sizeof(BYTE));
	
	// APINode�ܴ�С
	for (std::vector<MutatedImpTab_DLLNode>::iterator iter = m_vMutatedImpTab.begin(); iter < m_vMutatedImpTab.end(); iter++)
	{
		dwMutateImpSize += 32 * sizeof(char) * iter->vThunks.size();
	}

	return dwMutateImpSize;
}

/*
description:	��ȡ��������ݵ�����(�����ʽ)
params:			[in]void* pImageBase	// �ļ��ڴ����ָ��
returns:		bool
*/
bool ImpTab::marshallMutatedImpTab(void* pImageBase)
{
	if (!pImageBase || m_vMutatedImpTab.size())
	{
		return false;
	}
	
	const PIMAGE_NT_HEADERS pNTHeader = getNTHeader(pImageBase);
	PIMAGE_IMPORT_DESCRIPTOR pIID = (PIMAGE_IMPORT_DESCRIPTOR)RVAToPtr(pImageBase, 
		pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	PIMAGE_THUNK_DATA pThunk;
	MutatedImpTab_DLLNode tmpImportNode;
	MutatedImpTab_DLLNode_APINode tmpImpThunkNode;
	
	/*  ��ԭ�����ؼ���Ϣ����  */
	while (0 != pIID->FirstThunk)
	{
		tmpImportNode.clear();

		tmpImportNode.DLLName.assign((char*)RVAToPtr(pImageBase, pIID->Name));
		tmpImportNode.FirstThunk = pIID->FirstThunk;

		pThunk = (PIMAGE_THUNK_DATA)RVAToPtr(pImageBase, pIID->FirstThunk);
		while (pThunk->u1.AddressOfData)
		{
			tmpImpThunkNode.clear();
			
			if (!IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal))
			{	// STRING
				tmpImpThunkNode.APIName.assign((char*)RVAToPtr(pImageBase, pThunk->u1.AddressOfData + 2));
			}
			else
			{	// ORDINAL
				tmpImpThunkNode.Ordinal = pThunk->u1.Ordinal;
			}
			
			tmpImportNode.vThunks.push_back(tmpImpThunkNode);

			pThunk++;
		}

		m_vMutatedImpTab.push_back(tmpImportNode);

		pIID++;
	}

	return true;
}