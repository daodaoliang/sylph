/**
 * @file     SyProcessManager.h
 * @brief    Process management classes
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-07 
 *
 * https://github.com/horigome/sylph
 */
#pragma once
#include "stdafx.h"

/**
 * @brief プロセス毎の設定情報クラス。
 */
class CsyProcConfig {
public:
    CAtlString m_commandline;
    UINT       m_max_retry;      // not impl.
public:
    CsyProcConfig( _In_ LPCTSTR commandline = NULL,
                   _In_ UINT    max_retry   = 0 ) 
        : m_commandline( commandline ),
          m_max_retry  ( max_retry   ) { }

    ~CsyProcConfig( void ) = default;
        
    void Clear( void ) {
        m_commandline = TEXT("");
    }
};

typedef std::vector<CsyProcConfig> SYCONFIGS;

/**
 * @brief プロセスクラス。
 *        プロセス毎にスレッドで終了待ちを行うクラス
 */
class CsyProcess : public CsyThread {
    HANDLE              m_event;        ///< end trigger
    PROCESS_INFORMATION m_proc_info;    ///< process information
    CsyProcConfig       m_config;
    HRESULT             m_status;
public:
    /** constructor */
    CsyProcess( void ) : m_event( INVALID_HANDLE_VALUE ) {
        ::ZeroMemory( &m_proc_info, sizeof(m_proc_info) ); 
    }

    /** destructor. 実行中のプロセスはKillされる。*/
    virtual ~CsyProcess( void ) {
        this->Stop( );
    }

    /**
     * @brief プロセスIDを取得
     */
    UINT IsProcessID( void ) const {
        return m_proc_info.dwProcessId;
    }

    /**
     * @brief 起動したプロセスのスレッドIDを取得
     */
    UINT IsThreadIDByProcess ( void ) const {
        return m_proc_info.dwThreadId;
    }

    /**
     * @brief プロセスが実行中か確認
     * @retval TRUE ... Process is Running.
     */
    BOOL IsRunning( void ) const {
        if ( m_proc_info.hProcess ) {
            DWORD _exit_code = 0;
            if ( ::GetExitCodeProcess( m_proc_info.hProcess, &_exit_code ) )
                return _exit_code == STILL_ACTIVE ? TRUE : FALSE;
        }
        return FALSE;
    }

    /**
     * @brief Start Process
     */
    HRESULT Start( _In_ const CsyProcConfig& config ) { 

        HRESULT _hr           = S_OK;
        HANDLE  _idle_trigger = INVALID_HANDLE_VALUE;
        this->Stop();

        // Thread StopEvent
        m_config = config;
        m_event  = ::CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( m_event == INVALID_HANDLE_VALUE )  {
            _hr = HRESULT_FROM_WIN32( ::GetLastError() );
            goto START_EXIT;
        }

        // スレッド起動先で、プロセス起動が完了するまで待ち合わせる
        // ※同期しないと、スレッド関数内でプロセスが正常に起動したか分からない
        _idle_trigger = ::CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( _idle_trigger == INVALID_HANDLE_VALUE ) {
            _hr = HRESULT_FROM_WIN32( ::GetLastError() );
            goto START_EXIT;
        }

        if ( FAILED((_hr = CsyThread::Begin( &_idle_trigger ) )) ) {
            goto START_EXIT;
        }

        sy_single_join( _idle_trigger );
        if ( FAILED( m_status ) ) goto START_EXIT;

        ::CloseHandle( _idle_trigger ); 
        return S_OK;


    START_EXIT:
        if ( m_event != INVALID_HANDLE_VALUE ) 
            ::CloseHandle( m_event ); 
        if ( _idle_trigger != INVALID_HANDLE_VALUE ) 
            ::CloseHandle( _idle_trigger ); 

        m_event = INVALID_HANDLE_VALUE;
        return _hr;
    }

    /**
     * @brief Stop Process
     */
    void Stop( void ) {
        if ( m_event  != INVALID_HANDLE_VALUE ) {
            // wait for completion..
            ::SetEvent( m_event );
            CsyThread::Join();

            ::CloseHandle( m_event );
            m_event = INVALID_HANDLE_VALUE;
        }
    }

protected:
    /** 
     * @brief Thread hundler
     */
    virtual DWORD run( _In_ void* argment = NULL ) override {

        // receive event trigger from Begin()
        HANDLE *_idle_trigger_p = (HANDLE*)argment;
        ATLASSERT( _idle_trigger_p );

        _SLOG( TEXT("==> Start > %s\n"), m_config.m_commandline );
        HRESULT _h = sy_create_process( m_config.m_commandline, m_proc_info ); 
        if ( FAILED( _h ) ) {
            _SLOG( TEXT("! Process Start Failed. in %08x\n"), _h );
            m_status = _h;
            ::SetEvent( *_idle_trigger_p ); 
            return 1;   // process create failed.
        }

        _SLOG( TEXT("==> [PID:%d] Process Started.\n"), m_proc_info.dwProcessId );
        ::SetEvent( *_idle_trigger_p ); 

        HANDLE _events[2] = {
            m_proc_info.hProcess,
            m_event
        };

        DWORD _exit_code = 0;
        DWORD _result = ::WaitForMultipleObjects( 2, _events, FALSE, INFINITE );
        switch ( _result ) {
        // sig: exit a process
        case WAIT_OBJECT_0 + 0:
            ::GetExitCodeProcess( m_proc_info.hProcess, &_exit_code );
            break;

        // sig: terminate to process.
        case WAIT_OBJECT_0 + 1: {
        default:
            if ( ::GetExitCodeProcess( m_proc_info.hProcess, &_exit_code ) )
                if ( _exit_code == STILL_ACTIVE ) {
                    _SLOG( TEXT("==> [PID:%d] KILL Process \n"), m_proc_info.dwProcessId );
                    ::TerminateProcess( m_proc_info.hProcess, 0L );
                }
            }
            break;
        };

        if ( m_proc_info.hProcess ) 
            ::CloseHandle( m_proc_info.hProcess );

        _SLOG( TEXT("==> [PID:%d] Process exit : code %d\n"), 
                            m_proc_info.dwProcessId, _exit_code );
        ::ZeroMemory( &m_proc_info, sizeof( m_proc_info ) );

        return _exit_code;
    }
};

/**
 * @brief プロセス管理クラス。複数のプロセスクラスを管理します。
 */
class CsylphProcessManager {

   std::vector<CsyProcess*> m_processes;

public:
    /** constructor (default) */
    CsylphProcessManager         ( void ) = default;

    /** destructor. 全てのプロセスは解放される */
    virtual ~CsylphProcessManager( void ) {
        this->PurgeProcesses();
    }
    
    /**
     * @brief 指定プロセスを開始し、管理リストに追加します。　
     */
    HRESULT AddProcessEntry( _In_ const CsyProcConfig& config ) {
        auto _p = new CsyProcess();
        if ( !_p )
            return E_OUTOFMEMORY;

        HRESULT _hr = _p->Start( config ); 
        if ( FAILED( _hr ) ) {
            delete _p;
            return _hr;
        }
        
        m_processes.push_back( _p );
        _SLOG(TEXT("==> [PID:%d] Add ProcessEntry \n"), _p->IsProcessID( ) );
        return S_OK;
    }

    /**
     * @brief 全てのプロセスを停止し、プロセスリストを破棄します。
     */
    void PurgeProcesses( void ) {
        std::for_each( 
            m_processes.begin(), m_processes.end  (), [](CsyProcess* p) {
                if ( p ) {
                    p->Stop();
                    delete p;
                }
            });

        m_processes.clear();
    }

    /**
     * @brief process list を列挙します
     */
    void ForEach( std::function<void(CsyProcess*)> func ) {
        std::for_each( m_processes.begin(), m_processes.end  (), func );
    }
};