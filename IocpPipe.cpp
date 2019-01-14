#include "stdafx.h"
#include "IocpPipe.h"

BOOLEAN IocpPipeConnectHandler( IocpPipeHandle *lpIocpHandle,
						 IocpPipeOperation *lpOperation,
						 BOOL fStatus,
								DWORD dwNumberOfBytesTransferred )
{
	IocpPipeRead( lpIocpHandle, NULL );
	return TRUE;
}

BOOLEAN IocpPipeReadHandler( IocpPipeHandle *lpIocpHandle,
					  IocpPipeOperation *lpPipeOperation,
					  BOOL fStatus,
					  DWORD dwNumberOfBytesTransferred )
{
	// MessageBoxA( NULL, (LPCSTR)lpPipeOperation->lpBuffer, "", MB_OK );
	return TRUE;
}

BOOLEAN IocpPipeWriteHandler( IocpPipeHandle *lpIocpHandle,
					   IocpPipeOperation *lpPipeOperation,
					   BOOL fStatus,
					   DWORD dwNumberOfBytesTransferred )
{
	return TRUE;
}

VOID IocpPipeFreeOperation( IocpPipeOperation *lpPipeOperation )
{
	switch ( lpPipeOperation->op.nCmd )
	{
		case IoPipeConnect:
			free( lpPipeOperation );
			break;
		case IoPipeRead:
		case IoPipeWrite:
			free( lpPipeOperation->lpBuffer );
			free( lpPipeOperation );
			break;
		default:
			break;
	}
}

void IocpPipeFreeHandle( IocpPipeHandle *lpIocpHandle )
{
	free( lpIocpHandle );
}

VOID IocpPipeHandler( IocpPipeHandle *lpIocpHandle,
					  IocpPipeOperation *lpPipeOperation,
					  BOOL fStatus,
					  DWORD dwNumberOfBytesTransferred )
{
	DWORD dwErrorNo;

	if ( !fStatus )
	{
		dwErrorNo = GetLastError( );

		EnterCriticalSection( &lpIocpHandle->Handle.cs );
		MPRemoveEntryList( &lpPipeOperation->op.Entry );
		LeaveCriticalSection( &lpIocpHandle->Handle.cs );
		IocpPipeFreeOperation( lpPipeOperation );

		CancelIo( lpIocpHandle->Handle.hHandle );

		if ( IocpCloseHandle( &lpIocpHandle->Handle ) )
		{
			IocpPipeFreeHandle( lpIocpHandle );
		}

		return;
	}

	switch ( lpPipeOperation->op.nCmd )
	{
		case IoPipeConnect:
			fStatus = IocpPipeConnectHandler( lpIocpHandle,
									   lpPipeOperation,
									   fStatus,
									   dwNumberOfBytesTransferred
			);
			break;
		case IoPipeRead:
			fStatus = IocpPipeReadHandler( lpIocpHandle,
									lpPipeOperation,
									fStatus,
									dwNumberOfBytesTransferred
			);
			break;
		case IoPipeWrite:
			fStatus = IocpPipeWriteHandler( lpIocpHandle,
									 lpPipeOperation,
									 fStatus,
									 dwNumberOfBytesTransferred
			);
			break;
	}


	if ( fStatus && lpPipeOperation->op.nCmd == IoPipeRead )
	{
		fStatus = IocpPipeRead( lpIocpHandle, lpPipeOperation );
		if ( fStatus )
		{
			return;
		}
	}

	EnterCriticalSection( &lpIocpHandle->Handle.cs );
	MPRemoveEntryList( &lpPipeOperation->op.Entry );
	LeaveCriticalSection( &lpIocpHandle->Handle.cs );

	IocpPipeFreeOperation( lpPipeOperation );

	if ( !fStatus )
	{
		if ( IocpCloseHandle( &lpIocpHandle->Handle ) )
		{
			IocpPipeFreeHandle( lpIocpHandle );
		}
	}
}

IocpPipeHandle *IocpAssocatePipe( HANDLE hPipe )
{
	IocpPipeHandle *lpPipeHandle = (IocpPipeHandle *)malloc( sizeof( IocpPipeHandle ) );

	if ( !IocpInitHandle( &lpPipeHandle->Handle, ghIocp, hPipe, (IocpHandler_t)IocpPipeHandler ) )
	{
		free( lpPipeHandle );
		return NULL;
	}

	return lpPipeHandle;
}

BOOLEAN IocpPipeConnect( IocpPipeHandle *lpPipeHandle )
{
	IocpPipeOperation *lpPipeOperation = (IocpPipeOperation *)malloc( sizeof( IocpPipeOperation ) );

	lpPipeOperation->op.nCmd = IoPipeConnect;
	memset( &lpPipeOperation->op.ol, 0, sizeof( lpPipeOperation->op.ol ) );

	EnterCriticalSection( &lpPipeHandle->Handle.cs );
	IocpAddOperation( &lpPipeHandle->Handle, &lpPipeOperation->op );
	BOOL fSuccess = ConnectNamedPipe( lpPipeHandle->Handle.hHandle, &lpPipeOperation->op.ol );
	LeaveCriticalSection( &lpPipeHandle->Handle.cs );

	if ( !fSuccess )
	{
		DWORD dwErrorNo = GetLastError( );
		if ( dwErrorNo != ERROR_IO_PENDING )
		{
			EnterCriticalSection( &lpPipeHandle->Handle.cs );
			IocpDelOperation( &lpPipeHandle->Handle, &lpPipeOperation->op );
			LeaveCriticalSection( &lpPipeHandle->Handle.cs );

			free( lpPipeOperation );
			return FALSE;
		}
	}

	return TRUE;
}

BOOLEAN IocpPipeRead( IocpPipeHandle *lpPipeHandle, IocpPipeOperation *lpPipeOperation )
{
	BOOL fCreateNew = FALSE;
	if ( lpPipeOperation == NULL )
	{
		lpPipeOperation = (IocpPipeOperation *)malloc( sizeof( IocpPipeOperation ) );
		lpPipeOperation->lpBuffer = NULL;
		lpPipeOperation->nSize = 0;

		fCreateNew = TRUE;
	}

	lpPipeOperation->op.nCmd = IoPipeRead;
	memset( &lpPipeOperation->op.ol, 0, sizeof( lpPipeOperation->op.ol ) );

	if ( fCreateNew )
	{
		lpPipeOperation->lpBuffer = malloc( 3072 );
		lpPipeOperation->nSize = 3072;
	}
	

	EnterCriticalSection( &lpPipeHandle->Handle.cs );

	if ( fCreateNew )
	{
		IocpAddOperation( &lpPipeHandle->Handle, &lpPipeOperation->op );
	}

	BOOL fSuccess = ReadFile( lpPipeHandle->Handle.hHandle,
							  lpPipeOperation->lpBuffer,
							  lpPipeOperation->nSize,
							  NULL,
							  &lpPipeOperation->op.ol );

	LeaveCriticalSection( &lpPipeHandle->Handle.cs );

	if ( !fSuccess )
	{
		DWORD dwErrorNo = GetLastError( );
		if ( dwErrorNo != ERROR_IO_PENDING )
		{
			EnterCriticalSection( &lpPipeHandle->Handle.cs );
			IocpDelOperation( &lpPipeHandle->Handle, &lpPipeOperation->op );
			LeaveCriticalSection( &lpPipeHandle->Handle.cs );

			free( lpPipeOperation->lpBuffer );
			free( lpPipeOperation );
			return FALSE;
		}
	}

	return TRUE;
}

BOOLEAN IocpPipeWrite( IocpPipeHandle *lpPipeHandle,LPVOID lpBuffer, DWORD nSize )
{
	IocpPipeOperation *lpPipeOperation = (IocpPipeOperation *)malloc( sizeof( IocpPipeOperation ) );

	lpPipeOperation->op.nCmd = IoPipeWrite;
	memset( &lpPipeOperation->op.ol, 0, sizeof( lpPipeOperation->op.ol ) );

	lpPipeOperation->nSize = nSize;
	lpPipeOperation->lpBuffer = malloc( nSize );
	memcpy( lpPipeOperation->lpBuffer, lpBuffer, nSize );

	EnterCriticalSection( &lpPipeHandle->Handle.cs );

	IocpAddOperation( &lpPipeHandle->Handle, &lpPipeOperation->op );

	BOOL fSuccess = WriteFile( lpPipeHandle->Handle.hHandle,
							   lpPipeOperation->lpBuffer,
							   lpPipeOperation->nSize,
							   NULL,
							   &lpPipeOperation->op.ol );

	LeaveCriticalSection( &lpPipeHandle->Handle.cs );

	if ( !fSuccess )
	{
		DWORD dwErrorNo = GetLastError( );
		if ( dwErrorNo != ERROR_IO_PENDING )
		{
			EnterCriticalSection( &lpPipeHandle->Handle.cs );
			IocpDelOperation( &lpPipeHandle->Handle, &lpPipeOperation->op );
			LeaveCriticalSection( &lpPipeHandle->Handle.cs );

			free( lpPipeOperation->lpBuffer );
			free( lpPipeOperation );
			return FALSE;
		}
	}

	return TRUE;
}