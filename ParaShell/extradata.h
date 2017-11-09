#ifndef __EXTRADATA_H__
#define __EXTRADATA_H__

#include <windows.h>

/*
	Description:	���ļ��ж�ȡ��������
	Parameters:		[in]HANDLE	_hFile
					[in]void*	_imagebase
					[out]void**  _pExtraData
					[out]DWORD*	_ulExtraDataSize
*/
int ReadExtraData(HANDLE _hFile, void* _imagebase, void **_pExtraData, DWORD *_ulExtraDataSize);


/*
	Description:	�Ѷ�������д���ļ�
*/
int WriteExtraData(HANDLE _hFile, void *_pExtraData, DWORD ulExtraDataSize);


#endif // __EXTRADATA_H__
