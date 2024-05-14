#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Common_Typedef.h"

struct PacketData
{
	void Set(PacketData& value)
	{
		Set(value.m_uiSessionIndex, value.m_uiDataSize, value.m_pData);
	}

	void Set(const UINT32 uiSessionIndex, const UINT32 uiDataSize, _char* pData)
	{
		m_uiSessionIndex = uiSessionIndex;
		m_uiDataSize = uiDataSize;

		m_pData = new _char[m_uiDataSize];
		CopyMemory(m_pData, pData, m_uiDataSize);
	}

	void Release()
	{
		delete[] m_pData;
	}

	UINT32 m_uiSessionIndex = 0;
	UINT32 m_uiDataSize = 0;
	_char* m_pData = nullptr;
};