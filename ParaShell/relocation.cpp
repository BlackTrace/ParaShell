#include "relocation.h"
#include "pe_utilities.h"

RelocTab::RelocTab(void* pImageBase) : m_vMutatedRelocTab()
{
	marshallMutatedRelocTab(pImageBase);
}

bool RelocTab::reset(void* pImageBase)
{
	m_vMutatedRelocTab.clear();
	marshallMutatedRelocTab(pImageBase);
}

bool RelocTab::dumpInShellForm(void* pMem)
{
	if (!pMem)
	{
		return false;
	}

	Shell_MutatedRelocTab_NODE* pData = (Shell_MutatedRelocTab_NODE*)pMem;

	for (auto iter = m_vMutatedRelocTab.begin(); iter != m_vMutatedRelocTab.end(); ++iter)
	{
		// type
		pData->type = iter->type;

		// FirstTypeRVA
		pData->FirstTypeRVA = iter->FirstTypeRVA;

		// Offset
		int i = 0;
		for (auto iterO = iter->Offset.begin(); iterO != iter->Offset.end(); ++i, ++iterO)
		{
			pData->Offset[i] = *iterO;
		}
		pData->Offset[i] = 0; // ���ֶα�ʾ����
	}

	return true;
}

DWORD RelocTab::getMutatedRelocTabSizeInShell()
{
	//nNewItemSize = (pBaseReloc->SizeOfBlock - 8) / 2;//������������Ҫ���ֽڳ�
	DWORD dwRes = 0;

	for (auto iter = m_vMutatedRelocTab.begin(); iter != m_vMutatedRelocTab.end(); iter++)
	{
		dwRes += sizeof(iter->type);
		dwRes += sizeof(iter->FirstTypeRVA);
		
		for (auto iterB = iter->Offset.begin(); iterB != iter->Offset.end(); ++iterB)
		{
			dwRes += sizeof(BYTE);
		}

		// ���ֶα�ʾ�����
		dwRes += sizeof(BYTE);
	}

	// �տ��ʾ����
	dwRes += sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE);

	return dwRes;
}

bool RelocTab::clrOriginalRelocTab(void* pImageBase)
{
	if (!pImageBase)
	{
		return false;
	}

	return true;
}

bool RelocTab::marshallMutatedRelocTab(void* pImageBase)
{
	if (!pImageBase)
	{
		return false;
	}
	
	const PIMAGE_NT_HEADERS		pNTHeader = getNTHeader(pImageBase);
	PIMAGE_DATA_DIRECTORY		pRelocDir = NULL;
	PIMAGE_BASE_RELOCATION2		pBaseReloc = NULL;
	IMAGE_BASE_RELOCATION_MUTATED	tmpMutatedRelocTab_Node;

	pRelocDir = &pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	pBaseReloc = (PIMAGE_BASE_RELOCATION2)RVAToPtr(pImageBase, pRelocDir->VirtualAddress);
	
	// ���û���ض�λ���ݣ���ֱ�ӷ���
	if (0 == pRelocDir->VirtualAddress)
	{
		return true;
	}

	while (pBaseReloc->VirtualAddress != 0)
	{
		DWORD i = 0;
		DWORD accumulation = 0;
		tmpMutatedRelocTab_Node.clear();
		
		// type
		tmpMutatedRelocTab_Node.type = pBaseReloc->TypeOffset[i] & 0xf000;	
		if (IMAGE_REL_BASED_HIGHLOW != tmpMutatedRelocTab_Node.type)
		{
			m_vMutatedRelocTab.clear();
			return false;
		}

		// FirstTypeRVA
		accumulation = pBaseReloc->VirtualAddress + (pBaseReloc->TypeOffset[i] & 0x0fff);
		tmpMutatedRelocTab_Node.FirstTypeRVA = accumulation;

		while (true)
		{
			i++;
			const WORD tmpType = pBaseReloc->TypeOffset[i] & 0xf000;
			const DWORD tmpOffset = pBaseReloc->TypeOffset[i] & 0x0fff;

			if (IMAGE_REL_BASED_HIGHLOW == tmpType)
			{	// x86
				tmpMutatedRelocTab_Node.Offset.push_back(
					(BYTE)((tmpOffset + pBaseReloc->VirtualAddress) - accumulation));
				accumulation = pBaseReloc->VirtualAddress + (pBaseReloc->TypeOffset[i] & 0x0fff);
			}
			else if (IMAGE_REL_BASED_ABSOLUTE == tmpType)
			{
				break;
			}
			else
			{
				m_vMutatedRelocTab.clear();
				return false;
			}
		}

		m_vMutatedRelocTab.push_back(tmpMutatedRelocTab_Node);
		pBaseReloc = (PIMAGE_BASE_RELOCATION2)((DWORD)pBaseReloc + pBaseReloc->SizeOfBlock);
	}

	return true;
}


//��/*
//	Description:	�ض�λ����촦����
//*/
//bool MutateRelocation()
//{
//	PIMAGE_DATA_DIRECTORY		pRelocDir = NULL;
//	PIMAGE_BASE_RELOCATION2		pBaseReloc = NULL;
//
//	PCHAR						pRelocBufferMap = NULL;
//	PCHAR						pData = NULL;
//	UINT						nRelocSize = NULL;
//	UINT						nSize = 0;
//	UINT						nType = 0;
//	UINT						nIndex = 0;
//	UINT						nTemp = 0;
//	UINT						nNewItemOffset = 0;
//	UINT						nNewItemSize = 0;
//
//
//	pRelocDir = &m_pntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
//	nRelocSize = pRelocDir->Size;
//	pBaseReloc = (PIMAGE_BASE_RELOCATION2)RVAToPtr(pRelocDir->VirtualAddress);
//
//	// ���û���ض�λ���ݣ���ֱ�ӷ���
//	if (pRelocDir->VirtualAddress == 0)
//	{
//		return TRUE;
//	}
//	//������ʱ����ռ�
//	pRelocBufferMap = new char[nRelocSize];
//	if (pRelocBufferMap == NULL)
//	{
//		return FALSE;
//	}
//	ZeroMemory(pRelocBufferMap, nRelocSize);
//
//	// 
//	pData = pRelocBufferMap;
//
//	while (pBaseReloc->VirtualAddress != 0)
//	{
//		nNewItemSize = (pBaseReloc->SizeOfBlock - 8) / 2;//������������Ҫ���ֽڳ�
//
//		while (nNewItemSize != 0)
//		{
//			nType = pBaseReloc->TypeOffset[nIndex] >> 0x0c;//ȡtype
//
//			if (nType == 0x3)
//			{
//				//ȡ��ItemOffset�����ϱ����ض�λ��ʼ��ַ ����ȥnTemp,�õ���ֵ׼���ŵ����ض�λ��ṹ��
//				nNewItemOffset = ((pBaseReloc->TypeOffset[nIndex] & 0x0fff) + pBaseReloc->VirtualAddress) - nTemp;
//				if (nNewItemOffset > 0xff)//����Ǳ����ض�λ���ݵ�һ��
//				{
//					*(BYTE *)(pData) = 3;
//					pData += sizeof(BYTE);
//					*(DWORD *)pData = (DWORD)(nNewItemOffset);
//					pData += sizeof(DWORD);
//
//				}
//				else
//				{
//					*(BYTE *)(pData) = (BYTE)(nNewItemOffset);
//					pData += sizeof(BYTE);
//				}
//				nTemp += nNewItemOffset;
//			}
//			nNewItemSize--;
//			nIndex++;
//		}
//
//		nIndex = 0;
//		pBaseReloc = (PIMAGE_BASE_RELOCATION2)((DWORD)pBaseReloc + pBaseReloc->SizeOfBlock);
//	}
//
//	memset((PCHAR)RVAToPtr(pRelocDir->VirtualAddress), 0, nRelocSize);
//	memcpy((PCHAR)RVAToPtr(pRelocDir->VirtualAddress), pRelocBufferMap, nRelocSize);
//	delete pRelocBufferMap;
//
//	return true;
//}