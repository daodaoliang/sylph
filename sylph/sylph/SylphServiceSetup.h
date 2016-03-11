/**
 * @file     SylphServiceSetup.h
 * @brief    Windows Service Setup tools 
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-10
 *
 * https://github.com/horigome/sylph
 */
#pragma once
#include "stdafx.h"

/**
 * Service インストール済みか確認する
 *
 * @param[in] service_name ... サービス名
 * @retval ... TRUE:Install済み 
 */
inline BOOL 
sy_sv_is_setup( _In_ LPCTSTR service_name ) {

    SC_HANDLE _hSCM = 
        ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( !_hSCM ) 
        return FALSE;

    SC_HANDLE _hService = 
        ::OpenService( _hSCM, service_name, SERVICE_QUERY_CONFIG );

    if ( !_hService ) {
        ::CloseServiceHandle( _hSCM );
        return FALSE;
    }

    ::CloseServiceHandle( _hService );
    ::CloseServiceHandle( _hSCM    );
    return TRUE;
}

/**
 * @brief Serviceインストール
 *
 * @param[in] service_name ... Install ServiceName
 * @param[in] start_type ... Service StartType.
 *                      SERVICE_AUTO_START     0x00000002
 *                      SERVICE_DEMAND_START   0x00000003 (default)
 *                      SERVICE_DISABLED       0x00000004
 */
inline HRESULT 
sy_sv_install( _In_ LPCTSTR service_name,
               _In_ DWORD   start_type = SERVICE_DEMAND_START ) {

    if ( sy_sv_is_setup( service_name ) ) {
        _SLOG( TEXT("[INF] %s is already installed.\n"), service_name);
        return S_FALSE;
    }

    // 現在のアプリケーションパスを取得し、　
    // パスの前後に ""を入れる
    TCHAR _file_path[ MAX_PATH + _ATL_QUOTES_SPACE ];
    DWORD _path_len = ::GetModuleFileName( NULL, _file_path + 1, MAX_PATH );
    if( _path_len == 0 || _path_len == MAX_PATH )
        return E_FAIL;

    _file_path[ 0 ]             = _T('\"');
    _file_path[ _path_len + 1 ] = _T('\"');
    _file_path[ _path_len + 2 ] = 0;

    // 1. SCMを開く
    SC_HANDLE _hSCM = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( !_hSCM ) {
        _SLOG( TEXT("[ERR] OpenSCManager failed. : %s\n"), service_name);
        return HRESULT_FROM_WIN32( ::GetLastError( ) );
    }

    // 2. Serviceを作成
    SC_HANDLE _hService = ::CreateService( _hSCM, 
                                service_name,       // service name 
                                service_name,       // display name
                                SERVICE_ALL_ACCESS, 
                                SERVICE_WIN32_OWN_PROCESS,
                                start_type,        // StartType
                                SERVICE_ERROR_NORMAL,
                                _file_path, 
                                NULL, 
                                NULL, 
                                _T("RPCSS\0"), 
                                NULL, 
                                NULL );

    if ( !_hService  ) {
        _SLOG( TEXT("[ERR] CreateService failed. : %s\n"), service_name);
        ::CloseServiceHandle( _hSCM );
        return HRESULT_FROM_WIN32( ::GetLastError( ) );
    }

    _SLOG( TEXT("[O K] => Service installed. : %s\n"), service_name);
    ::CloseServiceHandle( _hService );
    ::CloseServiceHandle( _hSCM );
    return S_OK;
}

/**
 * @brief Serviceアンインストール
 *
 * @param[in] service_name ... サービス名
 */
inline HRESULT 
sy_sv_uninstall( _In_ LPCTSTR service_name ) {

    if ( !sy_sv_is_setup( service_name ) ) {
        _SLOG( TEXT("[INF] %s is not entry.\n"), service_name);
        return S_FALSE;
    }

    HRESULT   _hr       = S_OK;
    SC_HANDLE _hSCM     = NULL,
              _hService = NULL; 

    // 1. SCMを開く
    _hSCM = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( !_hSCM ) {
        _SLOG( TEXT("[ERR] OpenSCManager failed. : %s\n"), service_name);
        _hr = HRESULT_FROM_WIN32( ::GetLastError( ) );
        goto SY_SV_UNINST_EXIT;
    }

    // 2. Serviceを開く
    _hService = ::OpenService( _hSCM, service_name, SERVICE_STOP|DELETE );

    if ( !_hService ) {
        _SLOG( TEXT("[ERR] OpenService failed. : %s\n"), service_name);
        _hr = HRESULT_FROM_WIN32( ::GetLastError( ) );
        goto SY_SV_UNINST_EXIT;
    }

    // 3. Serviceを停止する
    SERVICE_STATUS _status;
    if ( !::ControlService( _hService, SERVICE_CONTROL_STOP, &_status ) ) {
        DWORD dwError = ::GetLastError();
        if (!((dwError == ERROR_SERVICE_NOT_ACTIVE) ||  
              (dwError == ERROR_SERVICE_CANNOT_ACCEPT_CTRL 
               && _status.dwCurrentState == SERVICE_STOP_PENDING))) {

            // PENDING..
            MessageBox( NULL, TEXT("Pending."), NULL, MB_OK );
        }
    }

    // 4. Serviceを削除する
    if ( !::DeleteService( _hService ) ) {
        _hr = HRESULT_FROM_WIN32( ::GetLastError() );
    }

    _SLOG( TEXT("[O K] => Service uninstalled. : %s\n"), service_name);
SY_SV_UNINST_EXIT:
    if ( _hService ) ::CloseServiceHandle( _hService );
    if ( _hSCM     ) ::CloseServiceHandle( _hSCM );

    return _hr;
}

