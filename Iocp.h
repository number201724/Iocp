#pragma once
struct IocpHandle;
struct IocpOperation;


typedef VOID( *IocpHandler_t )(IocpHandle *lpIocpHandle, IocpOperation *lpOperation, BOOL fStatus, DWORD dwNumberOfBytesTransferred);

struct IocpHandle
{
	MP_LIST_ENTRY Entry;
	HANDLE hIocp;
	HANDLE hHandle;
	MP_LIST_ENTRY Operations;
	IocpHandler_t lpHandler;
	CRITICAL_SECTION cs;
};

struct IocpOperation
{
	MP_LIST_ENTRY Entry;
	OVERLAPPED ol;
	BYTE nCmd;
};

/*
	IocpHandle
*/
BOOLEAN IocpInitHandle( IocpHandle *lpHandle, HANDLE hIocp, HANDLE hHandle, IocpHandler_t lpHandler );
BOOLEAN IocpCloseHandle( IocpHandle *lpHandle );
VOID IocpAddOperation( IocpHandle *lpHandle, IocpOperation *lpOperation );
VOID IocpDelOperation( IocpHandle *lpHandle, IocpOperation *lpOperation );


/*
	IocpManager
*/
BOOLEAN IocpStarup( int nNumThreads );
VOID IocpShutdown( );


extern HANDLE ghIocp;