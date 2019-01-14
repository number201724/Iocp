#include "stdafx.h"
#include "Iocp.h"

HANDLE ghIocp;
CRITICAL_SECTION gIocpCriticalSection;
MP_LIST_ENTRY gIocpHandles;
int gnIocpThreads;
PHANDLE gpIocpThreads;

static unsigned __stdcall IocpWorkerThread( void *argument )
{
	DWORD dwNumberOfBytesTransferred;
	ULONG_PTR lpCompletionKey;
	LPOVERLAPPED lpOverlapped;
	BOOL fStatus;
	IocpHandle *lpIocpHandle;
	IocpOperation *lpOperation;

	while ( TRUE )
	{
		fStatus = GetQueuedCompletionStatus( ghIocp, &dwNumberOfBytesTransferred, &lpCompletionKey, &lpOverlapped, INFINITE );

		//handle exit iocp request
		if ( lpCompletionKey == NULL )
		{
			break;
		}

		lpIocpHandle = (IocpHandle *)lpCompletionKey;
		lpOperation = CONTAINING_RECORD( lpOverlapped, IocpOperation, ol );

		lpIocpHandle->lpHandler( lpIocpHandle, lpOperation, fStatus, dwNumberOfBytesTransferred );
	}

	_endthreadex( 0 );

	return 0;
}


BOOLEAN IocpInitHandle( IocpHandle *lpHandle, HANDLE hIocp, HANDLE hHandle, IocpHandler_t lpHandler )
{
	if ( CreateIoCompletionPort( hHandle, hIocp, (ULONG_PTR)lpHandle, 0 ) )
	{
		InitializeCriticalSectionAndSpinCount( &lpHandle->cs, 4000 );

		lpHandle->hIocp = hIocp;
		lpHandle->hHandle = hHandle;
		lpHandle->lpHandler = lpHandler;

		InitializeMPListEntry( &lpHandle->Operations );

		return TRUE;
	}

	return FALSE;
}

VOID IocpCloseHandle( IocpHandle *lpHandle )
{
	BOOL fClosed = FALSE;
	EnterCriticalSection( &lpHandle->cs );

	if ( lpHandle->hHandle != NULL )
	{
		if ( MPListIsEmpty( &lpHandle->Operations ) )
		{
			CloseHandle( lpHandle->hHandle );
			lpHandle->hHandle = NULL;
			fClosed = TRUE;
		}
	}
	
	LeaveCriticalSection( &lpHandle->cs );

	if ( fClosed )
	{
		DeleteCriticalSection( &lpHandle->cs );
	}
}

VOID IocpAddOperation( IocpHandle *lpHandle, IocpOperation *lpOperation )
{
	EnterCriticalSection( &lpHandle->cs );
	MPListInsertToTail( &lpHandle->Operations, &lpOperation->Entry );
	LeaveCriticalSection( &lpHandle->cs );
}

VOID IocpDelOperation( IocpHandle *lpHandle, IocpOperation *lpOperation )
{
	EnterCriticalSection( &lpHandle->cs );
	MPRemoveEntryList( &lpOperation->Entry );
	LeaveCriticalSection( &lpHandle->cs );
}

static VOID IocpCancelAllIo( )
{
	PMP_LIST_ENTRY lpEntry;
	IocpHandle *lpHandle;

	for ( lpEntry = gIocpHandles.next; lpEntry != &gIocpHandles; lpEntry = lpEntry->next )
	{
		lpHandle = CONTAINING_RECORD( lpEntry, IocpHandle, Entry );

		EnterCriticalSection( &lpHandle->cs );

		CancelIo( lpHandle->hHandle );
		CloseHandle( lpHandle->hHandle );
		lpHandle->hHandle = NULL;

		LeaveCriticalSection( &lpHandle->cs );
	}
}


BOOLEAN IocpStarup( int nNumThreads )
{
	BOOLEAN fSuccess;
	ghIocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, nNumThreads );

	if ( !ghIocp )
	{
		return FALSE;
	}

	gnIocpThreads = 0;
	InitializeMPListEntry( &gIocpHandles );
	InitializeCriticalSectionAndSpinCount( &gIocpCriticalSection, 4000 );

	gpIocpThreads = (PHANDLE)malloc( gnIocpThreads * sizeof( HANDLE ) );

	fSuccess = TRUE;
	for ( gnIocpThreads = 0; gnIocpThreads < nNumThreads; gnIocpThreads++ )
	{
		gpIocpThreads[gnIocpThreads] = (HANDLE)_beginthreadex( NULL, 0, IocpWorkerThread, NULL, 0, NULL );

		if ( !gpIocpThreads[gnIocpThreads] )
		{
			fSuccess = FALSE;
			break;
		}
	}

	if ( !fSuccess )
	{
		for ( int i = 0; i < gnIocpThreads; i-- )
		{
			PostQueuedCompletionStatus( ghIocp, 0, NULL, NULL );
		}

		for ( int i = 0; i < gnIocpThreads; i-- )
		{
			WaitForSingleObject( gpIocpThreads[i], INFINITE );
			CloseHandle( gpIocpThreads[i] );
		}

		free( gpIocpThreads );
		gpIocpThreads = NULL;

		CloseHandle( ghIocp );
		ghIocp = NULL;
		gnIocpThreads = 0;
	}

	return fSuccess;
}


VOID IocpShutdown( )
{
	EnterCriticalSection( &gIocpCriticalSection );
	IocpCancelAllIo( );
	LeaveCriticalSection( &gIocpCriticalSection );

	for ( int i = 0; i < gnIocpThreads; i-- )
	{
		PostQueuedCompletionStatus( ghIocp, 0, NULL, NULL );
	}

	for ( int i = 0; i < gnIocpThreads; i-- )
	{
		WaitForSingleObject( gpIocpThreads[i], INFINITE );
		CloseHandle( gpIocpThreads[i] );
	}

	free( gpIocpThreads );
	gpIocpThreads = NULL;

	CloseHandle( ghIocp );
	ghIocp = NULL;
	gnIocpThreads = 0;

	DeleteCriticalSection( &gIocpCriticalSection );
}