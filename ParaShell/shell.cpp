#include <iostream>
#include <windows.h>
#include "shell.h"
#include "error.h"
#include "pe_utilities.h"
#include "config.h"

// һЩ��Ҫд��shell������֮��ļ��
const DWORD ulShellDataGap = 0x10;

/*
	Description:	����shell����
*/
int buildShell(void* _pImageBase, std::vector<DataToShellNode> &_rvDataToShell, void **_ppShellSection)
{
	/* ����������������ܴ�С */
	DWORD shellrawsize = (DWORD)(&Label_Shell_End) - (DWORD)(&Label_Shell_Start);
	DWORD shelldatasize = 0;
	for (auto iter = _rvDataToShell.begin(); iter != _rvDataToShell.end(); iter++)
	{
		shelldatasize += ulShellDataGap;
		shelldatasize += iter->nData;
	}
	DWORD shellwholesize = shellrawsize + shelldatasize;

	// ����һ�����ڴ�����������
	CreateNewSection(_pImageBase, shellwholesize, _ppShellSection);

	// ��ԭ��shell������д��shellӳ����
	memcpy(*_ppShellSection, (&Label_Shell_Start), shellrawsize);
	
	/* ����αװ������ֶ� */
	if (!fixFakedImpTabItem(_pImageBase, *_ppShellSection))
	{
		return ERR_UNKNOWN;
	}

	/* ������������ֶ� */
	if (!fixShellData(_pImageBase, *_ppShellSection))
	{
		return ERR_UNKNOWN;
	}

	/*  ����Ҫд��shell������д��*/
	DWORD ShellDataOffset = shellrawsize + ulShellDataGap;
	std::vector<DataToShellNode>::iterator iter = _rvDataToShell.begin();
	while (_rvDataToShell.end() != iter)
	{
		if (ShellDataType::MImp == iter->DataType)
		{
			if (!buildImpTab(
				_pImageBase,
				iter->pData,
				iter->nData,
				*_ppShellSection,
				ShellDataOffset))
			{
				return ERR_UNKNOWN;
			}
		}
		else if (ShellDataType::MReloc == iter->DataType)
		{
			if (!buildRelocTab(
				_pImageBase,
				iter->pData,
				iter->nData,
				*_ppShellSection,
				ShellDataOffset))
			{
				return ERR_UNKNOWN;
			}
		}

		ShellDataOffset += iter->nData + ulShellDataGap;
		iter++;
	}
	
	const PIMAGE_NT_HEADERS pNTHeader = getNTHeader(_pImageBase);
	const PIMAGE_SECTION_HEADER pLastSecHeader = getLastSecHeader(_pImageBase);	
	pNTHeader->OptionalHeader.AddressOfEntryPoint = pLastSecHeader->VirtualAddress;
	pNTHeader->OptionalHeader.BaseOfCode = pLastSecHeader->VirtualAddress;
	
	return	ERR_SUCCESS;
}

/*
description:	����αװ������ֶ�
params:			[in]void* pImageBase
*				[in + out]void* pSecShell
returns:		bool
*/
bool fixFakedImpTabItem(void* pImageBase, void* pSecShell)
{
	if (!pImageBase || !pSecShell)
	{
		return false;
	}
	
	const PIMAGE_NT_HEADERS pNTHeader = getNTHeader(pImageBase);
	const PIMAGE_SECTION_HEADER pLastSecHeader = getLastSecHeader(pImageBase);	
	const DWORD Offset = pLastSecHeader->VirtualAddress;
	Induction_Import* pFakedImpTab = (Induction_Import*)((DWORD)(pSecShell) + \
		(DWORD)(&Label_Induction_Import_Start) - (DWORD)(&Label_Shell_Start));
	
	pFakedImpTab->ImpD[0].FirstThunk += Offset;
	pFakedImpTab->ImpD[0].OriginalFirstThunk += Offset;
	pFakedImpTab->ImpD[0].Name += Offset;
	pFakedImpTab->Thunk[0].u1.AddressOfData += Offset;
	pFakedImpTab->Thunk[1].u1.AddressOfData += Offset;
	pFakedImpTab->Thunk[2].u1.AddressOfData += Offset;

	return true;


}

/*
description:	������������ֶ�
params:			[in]void* pImageBase
*				[in + out]void* pSecShell
returns:		bool
*/
bool fixShellData(void* pImageBase, void* pSecShell)
{
	if (!pImageBase || !pSecShell)
	{
		return false;
	}

	const PIMAGE_NT_HEADERS pNTHeader = getNTHeader(pImageBase);
	const PIMAGE_SECTION_HEADER pLastSecHeader = getLastSecHeader(pImageBase);	

	PInduction_Data pInductionData = (PInduction_Data)((DWORD)(pSecShell) + (DWORD)(&Label_Induction_Data_Start) - (DWORD)(&Label_Shell_Start));
	pInductionData->LuanchBase = (DWORD)(&Label_Luanch_Start) - (DWORD)(&Label_Shell_Start);
	pInductionData->nLuanchOriginalSize = (DWORD)(&Label_Luanch_End) - (DWORD)(&Label_Luanch_Start);

	PLuanch_Data pLuanchData = (PLuanch_Data)((DWORD)(pSecShell) + (DWORD)(&Lable_Luanch_Data_Start) - (DWORD)(&Label_Shell_Start));
	pLuanchData->OEP = pNTHeader->OptionalHeader.AddressOfEntryPoint;
	pLuanchData->OriginalImageBase = pNTHeader->OptionalHeader.ImageBase;
	pLuanchData->IsDLL = pNTHeader->FileHeader.Characteristics & IMAGE_FILE_DLL ? 1 : 0; // TODO: ֧��DLL
	
	return true;
}

/*
description:	���������Ϣ���õ����ӿǳ�����
params:			[in]void* pImageBase
*				[in]const void* pImpTabData
*				[in]const DWORD nImpTabData
*				[in + out]void* pSecShell
*				[in + out]DWORD Offset
returns:		bool
*/
bool buildImpTab(
	void* pImageBase,
	const void* pImpTabData,
	const DWORD nImpTabData,
	void* pSecShell,
	DWORD Offset)
{
	if (!pImageBase || !pImpTabData || !nImpTabData || !pSecShell || !Offset)
	{
		return false;
	}

	const PIMAGE_NT_HEADERS pNTHeader = getNTHeader(pImageBase);
	const PIMAGE_SECTION_HEADER pLastSecHeader = getLastSecHeader(pImageBase);	

	/*  ʹԭ��������������д  */
	/*if (!MakeSecWritable(pImageBase,
		pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress))
	{
		return false;
	}
	if (!MakeSecWritable(pImageBase,
		pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress))
	{
		return false;
	}*/

	/* ���Ʊ�����������ݵ�������� */
	memcpy((char*)((DWORD)pSecShell + Offset), pImpTabData, nImpTabData);
	
	/* ������Ƕ��ж�Ӧ�ֶ� */
	PLuanch_Data pLuanchData = (PLuanch_Data)((DWORD)(pSecShell) + (DWORD)(&Lable_Luanch_Data_Start) - (DWORD)(&Label_Shell_Start));
	pLuanchData->MInfo.ImpTab = ISMUTATEIMPORT ? MInfo_ImpTabType::MIITT_MUTATED : MInfo_ImpTabType::MIITT_NOTHING;
	pLuanchData->Nodes[ShellDataType::MImp].Type = ShellDataType::MImp;
	pLuanchData->Nodes[ShellDataType::MImp].OriginalAddr = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	pLuanchData->Nodes[ShellDataType::MImp].MutatedAddr = pLastSecHeader->VirtualAddress + Offset;
#ifdef _DEBUG
	std::cout << "MImp::MutatedAddr:\t" << std::hex << pLuanchData->Nodes[ShellDataType::MImp].MutatedAddr << std::endl;
#endif 

	/*  �޸�PEͷ  */
	pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = pLastSecHeader->VirtualAddress + (DWORD)(&Label_Induction_Import_Start) - (DWORD)(&Label_Shell_Start);
	pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)(&Label_Induction_Import_End) - (DWORD)(&Label_Induction_Import_Start);
	pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = pLastSecHeader->VirtualAddress + (DWORD)(&Label_Induction_Import_End) - (DWORD)(&Label_Shell_Start);
	pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = sizeof(Induction_Import::Thunk);

	return true;
}

/*
description:	���ض�λ����Ϣ���õ����ӿǳ�����
params:			[in]void* pImageBase
*				[in]const void* pRelocTabData
*				[in]const DWORD nRelocTabData
*				[in + out]void* pSecShell
*				[in + out]DWORD Offset
returns:		bool
*/
bool buildRelocTab(
	void* pImageBase,
	const void* pRelocTabData,
	const DWORD nRelocTabData,
	void* pSecShell,
	DWORD Offset)
{
	if (!pImageBase || !pRelocTabData || !nRelocTabData || !pSecShell || !Offset)
	{
		return false;
	}

	const PIMAGE_NT_HEADERS pNTHeader = getNTHeader(pImageBase);
	const PIMAGE_SECTION_HEADER pLastSecHeader = getLastSecHeader(pImageBase);	
	
	/* ���Ʊ����ض�λ�����ݵ�������� */
	memcpy((char*)((DWORD)pSecShell + Offset), pRelocTabData, nRelocTabData);
	
	/* ������Ƕ��ж�Ӧ�ֶ� */
	PLuanch_Data pLuanchData = (PLuanch_Data)((DWORD)(pSecShell) + (DWORD)(&Lable_Luanch_Data_Start) - (DWORD)(&Label_Shell_Start));
	pLuanchData->MInfo.RelocTab = ISMUTATERELOC ? MInfo_RelocTabType::MIRTT_MUTATED : MInfo_RelocTabType::MIRTT_NOTHING;
	pLuanchData->Nodes[ShellDataType::MReloc].Type = ShellDataType::MReloc;
	pLuanchData->Nodes[ShellDataType::MReloc].OriginalAddr = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;;	
	pLuanchData->Nodes[ShellDataType::MReloc].MutatedAddr = pLastSecHeader->VirtualAddress + Offset;
#ifdef _DEBUG
	std::cout << "MReloc::MutatedAddr:\t" << std::hex << pLuanchData->Nodes[ShellDataType::MReloc].MutatedAddr << std::endl;
#endif 

	/*  �޸�PEͷ  */
	pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
	pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;

	return true;
}