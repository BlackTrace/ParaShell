#include "import.h"
#include "error.h"
#include "pe_utilities.h"


/*
description:	�������촦��
params:			void* _pImageBase
returns:		ERR_SUCCESS
*/
int MutateImport(void *_pImageBase, PMutateImportInfo _pMutateImportInfo)
{
	PIMAGE_NT_HEADERS pNTHeader = getNTHeader(_pImageBase);
	PIMAGE_IMPORT_DESCRIPTOR pIID = (PIMAGE_IMPORT_DESCRIPTOR)RVAToPtr(_pImageBase, pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	PIMAGE_THUNK_DATA pThunk;
	std::vector<MutatedImpTab_DLLNode> vMuateImport;
	MutatedImpTab_DLLNode tmpImportNode;
	MutatedImpTab_DLLNode_APINode tmpImpThunkNode;

	/*  ��ԭ�����ؼ���Ϣ����  */
	while (0 != pIID->FirstThunk)
	{
		memset(&tmpImportNode, 0, sizeof(MutatedImpTab_DLLNode));

		strcpy_s(tmpImportNode.DLLName, (char*)RVAToPtr(_pImageBase, pIID->Name));
		tmpImportNode.FirstThunk = pIID->FirstThunk;

		pThunk = (PIMAGE_THUNK_DATA)RVAToPtr(_pImageBase, pIID->FirstThunk);
		while (pThunk->u1.AddressOfData)
		{
			memset(&tmpImpThunkNode, 0, sizeof(MutatedImpTab_DLLNode_APINode));
			
			if (!IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal))
			{	// STRING
				strcpy_s(tmpImpThunkNode.FuncName, (char*)RVAToPtr(_pImageBase, pThunk->u1.AddressOfData + 2));
			}
			else
			{	// ORDINAL
				tmpImpThunkNode.Ordinal = pThunk->u1.Ordinal;
			}
			
			tmpImportNode.vThunks.push_back(tmpImpThunkNode);

			pThunk++;
		}

		vMuateImport.push_back(tmpImportNode);

		pIID++;
	}

	/*  Ϊ�������������ڴ�ռ䣬����������Ա�����ʽд������ڴ�ռ���  */
	_pMutateImportInfo->nMutateImport = CalcMutateImpSize(vMuateImport);
	_pMutateImportInfo->pMutateImport = new char[_pMutateImportInfo->nMutateImport];
	memset(_pMutateImportInfo->pMutateImport, 0, _pMutateImportInfo->nMutateImport);
	char* pData = (char*)(_pMutateImportInfo->pMutateImport);
	for (std::vector<MutatedImpTab_DLLNode>::iterator iterD = vMuateImport.begin(); iterD < vMuateImport.end(); iterD++)
	{
		*(DWORD*)pData = iterD->FirstThunk;
		pData += sizeof(DWORD);
		strcpy_s(pData, 32, iterD->DLLName);
		pData += 32 * sizeof(char);
		*(DWORD*)pData = iterD->vThunks.size();
		pData += sizeof(DWORD);
		for (std::vector<MutatedImpTab_DLLNode_APINode>::iterator iterT = iterD->vThunks.begin(); iterT < iterD->vThunks.end(); iterT++)
		{
			memcpy(pData, iterT->FuncName, sizeof(*iterT));
			pData += sizeof(*iterT);
		}
	}


	return ERR_SUCCESS;
}

/*
description:	��ȡ��������ݵ�����(�����ʽ)
params:			[in]void* pImageBase	// �ļ��ڴ����ָ��
*				[out]std::vector<MutatedImpTab_DLLNode>& rvMutatedImpTab
returns:		ERR_SUCCESS
*				ERR_INVALIDPARAMS
*/
int marshallMutatedImpTab(void* pImageBase, std::vector<MutatedImpTab_DLLNode>& rvMutatedImpTab)
{
	if (!pImageBase)
	{
		return ERR_INVALIDPARAMS;
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
		memset(&tmpImportNode, 0, sizeof(MutatedImpTab_DLLNode));

		strcpy_s(tmpImportNode.DLLName, (char*)RVAToPtr(pImageBase, pIID->Name));
		tmpImportNode.FirstThunk = pIID->FirstThunk;

		pThunk = (PIMAGE_THUNK_DATA)RVAToPtr(pImageBase, pIID->FirstThunk);
		while (pThunk->u1.AddressOfData)
		{
			memset(&tmpImpThunkNode, 0, sizeof(MutatedImpTab_DLLNode_APINode));
			
			if (!IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal))
			{	// STRING
				strcpy_s(tmpImpThunkNode.FuncName, (char*)RVAToPtr(_pImageBase, pThunk->u1.AddressOfData + 2));
			}
			else
			{	// ORDINAL
				tmpImpThunkNode.Ordinal = pThunk->u1.Ordinal;
			}
			
			tmpImportNode.vThunks.push_back(tmpImpThunkNode);

			pThunk++;
		}

		vMuateImport.push_back(tmpImportNode);

		pIID++;
	}
}

/*
	Description:	����������������Ҫ�Ĵ�С
*/
unsigned long CalcMutateImpSize(std::vector<MutatedImpTab_DLLNode> &_rvMuateImport)
{
	unsigned long ulMutateImpSize = 0;
	ulMutateImpSize += \
		2 * sizeof(DWORD) * _rvMuateImport.size() \
		+ 32 * sizeof(char) * _rvMuateImport.size();
	
	for (std::vector<MutatedImpTab_DLLNode>::iterator iter = _rvMuateImport.begin(); iter < _rvMuateImport.end(); iter++)
	{
		ulMutateImpSize += 32 * sizeof(char) * iter->vThunks.size();
	}

	return ulMutateImpSize;
}
