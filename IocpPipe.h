#pragma once
#include "Iocp.h"

#define IoPipeConnect 0
#define IoPipeRead 1
#define IoPipeWrite 2

struct IocpPipeOperation
{
	IocpOperation op;
	PVOID lpBuffer;
	DWORD nSize;
};

struct IocpPipeHandle
{
	IocpHandle Handle;
};



IocpPipeHandle *IocpAssocatePipe( HANDLE hPipe );
BOOLEAN IocpPipeConnect( IocpPipeHandle *lpPipeHandle );
BOOLEAN IocpPipeRead( IocpPipeHandle *lpPipeHandle, IocpPipeOperation *lpPipeOperation );