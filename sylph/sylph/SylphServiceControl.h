/**
 * @file     SylphServiceControl.h
 * @brief    Windows Service 
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-10
 *
 * https://github.com/horigome/sylph
 */
#pragma once
#include "stdafx.h"

/**
 * @brief Windows Service Contol class
 */
class CsyServiceControl : public CsyThread{

    CAtlString            m_ServiceName;
    SERVICE_STATUS        m_ServiceStatus;
    SERVICE_STATUS_HANDLE m_StatusHandle     = NULL;
    HANDLE                m_ServiceStopEvent = INVALID_HANDLE_VALUE; 

protected:
    /**
     * @brief サービス開始時に呼ばれます。（派生クラスはOverrideできます）
     */
    virtual HRESULT OnStart( void ) { return S_OK; }

    /**
     * @brief サービス停止時に呼ばれます。（派生クラスはOverrideできます）　
     */
    virtual void OnStop( void ) {
        if ( m_ServiceStopEvent ) ::SetEvent( m_ServiceStopEvent ); 
    }

public:
    CsyServiceControl         ( void ) {
        ::ZeroMemory( &m_ServiceStatus, sizeof( m_ServiceStatus ) );
        m_ServiceName = TEXT("Sylph");
    }

    /** destructor. Serviceは停止されます */
    virtual ~CsyServiceControl( void ) {
        this->OnStop( );
        this->Join();
    }

    /** 
     * @brief サービス名取得。（派生クラスはこの関数をOverrideします）
     */
    virtual LPCTSTR GetServiceName( void ) const {
        return m_ServiceName; 
    }

    /** 
     * @brief サービス処理実行。ServiceMainから実行します。
     *        Serviceが実行中 Waitします。
     */
    HRESULT WaitForCompleation( void ) {

        EVENT_DBG( TEXT("ServiceMain Begin") );
        // ==> サービスハンドラ関数の登録 ※Context : this pointer 
        m_StatusHandle = ::RegisterServiceCtrlHandlerEx ( 
                                    GetServiceName( ), 
                                    CsyServiceControl::ServiceCtrlHandler, 
                                    this );

        ZeroMemory ( &m_ServiceStatus, sizeof (m_ServiceStatus) );
        m_ServiceStatus.dwServiceType               = SERVICE_WIN32_OWN_PROCESS;
        m_ServiceStatus.dwControlsAccepted          = 0;
        m_ServiceStatus.dwCurrentState              = SERVICE_START_PENDING;
        m_ServiceStatus.dwWin32ExitCode             = 0;
        m_ServiceStatus.dwServiceSpecificExitCode   = 0;
        m_ServiceStatus.dwCheckPoint                = 0;

        if ( !::SetServiceStatus ( m_StatusHandle, &m_ServiceStatus )) {
            EVENT_WAR( TEXT("[START_PENDING] SetServiceStatus Failed. %d"), 
                ::GetLastError() );
        }

        //
        // ==> サービス開始（Event objectを使い、Threadを待機します）
        //
        m_ServiceStatus.dwControlsAccepted  = SERVICE_ACCEPT_STOP;
        m_ServiceStatus.dwCurrentState      = SERVICE_RUNNING; // RUNNING
        m_ServiceStatus.dwWin32ExitCode     = 0;
        m_ServiceStatus.dwCheckPoint        = 0;

        if ( !::SetServiceStatus ( m_StatusHandle, &m_ServiceStatus ) ) {
            EVENT_WAR( TEXT("[RUNNING] SetServiceStatus Failed. %d"), 
                ::GetLastError() );
        }

        m_ServiceStopEvent = ::CreateEvent ( NULL, TRUE, FALSE, NULL );
        ATLASSERT( m_ServiceStopEvent != INVALID_HANDLE_VALUE ); 

        try {
            ATLENSURE_SUCCEEDED( this->OnStart( )     ); // Call Start Handler
            ATLENSURE_SUCCEEDED( this->Begin  ( NULL )); // Start ServiceThread
            this->Join   (  );

            if ( m_ServiceStopEvent ) ::CloseHandle ( m_ServiceStopEvent );
            m_ServiceStopEvent = NULL;

        } catch ( CAtlException& e ) {

            EVENT_ERR( TEXT("Process Start Failed. 0x%08x"), e.m_hr );
            m_ServiceStatus.dwControlsAccepted = 0;
            m_ServiceStatus.dwCurrentState     = SERVICE_STOP_PENDING;
            m_ServiceStatus.dwWin32ExitCode    = 0;
            m_ServiceStatus.dwCheckPoint       = 4;

            if ( !::SetServiceStatus ( m_StatusHandle, &m_ServiceStatus ) ) {
                EVENT_WAR( TEXT("[STOP_PENDING] SetServiceStatus Failed. %d"), 
                    ::GetLastError() );
            }
        }

        // ==> 停止後、サービスの状態を停止に遷移させる
        m_ServiceStatus.dwControlsAccepted  = 0;
        m_ServiceStatus.dwCurrentState      = SERVICE_STOPPED; // STOPED
        m_ServiceStatus.dwWin32ExitCode     = 0;
        m_ServiceStatus.dwCheckPoint        = 3;

        if ( !::SetServiceStatus ( m_StatusHandle, &m_ServiceStatus ) ) {
            EVENT_WAR( TEXT("[STOPED] SetServiceStatus Failed. %d"), 
                ::GetLastError() );
        }

        EVENT_DBG( TEXT("ServiceMain Exit") );
        return S_OK;
    }

private:

    /** Service Wait Thread */
    virtual DWORD run( _In_ void* argment  ) override {
        switch( ::WaitForSingleObject( this->m_ServiceStopEvent, INFINITE ) ) {
        case WAIT_OBJECT_0 + 0:
            _SDBG( TEXT("Service Thread break.\n") );
            return 0;
        default:
            break;
        }
        return 1;
    }

    /**
     * @brief Serviceハンドラ。OnStop()などを実行します。
     */
    static DWORD WINAPI ServiceCtrlHandler ( 
                              _In_ DWORD     CtrlCode, 
                              _In_ DWORD     EventType,
                              _In_ LPVOID    lpEventData,
                              _In_ LPVOID    context_p   ) {

        auto _service_p = reinterpret_cast<CsyServiceControl*>( context_p );
        if ( !_service_p ) {
            EVENT_ERR( TEXT("ServiceCtrlHandler Context is NULL.") ); 
            return ERROR_INVALID_ADDRESS;
        }

        switch ( CtrlCode ) {

        // * Service Stop
        case SERVICE_CONTROL_STOP :

            if ( SERVICE_STATUS* _s = &_service_p->m_ServiceStatus ) { 

                if ( _s->dwCurrentState != SERVICE_RUNNING )
                   break;

                _s->dwControlsAccepted  = 0;
                _s->dwCurrentState      = SERVICE_STOP_PENDING;     // B-1 STOP_PENDING
                _s->dwWin32ExitCode     = 0;
                _s->dwCheckPoint        = 4;                        // 4?

                if ( !::SetServiceStatus ( _service_p->m_StatusHandle, _s ) ) {
                    EVENT_WAR( TEXT("[STOP_PENDING] SetServiceStatus Failed. %d"), 
                        ::GetLastError() );
                }
            }

            // ==> Stop Event Signal.　
            _service_p->OnStop( );

            return NO_ERROR;

        default:
             break;
        }
        return ERROR_CALL_NOT_IMPLEMENTED ;
    }
};
