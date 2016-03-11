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
    UINT       m_max_retry;
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
public:
    /** constructor */
    CsyProcess( void ) : m_event( NULL ) {
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
        this->Stop();
        m_event = ::CreateEvent( NULL, NULL, FALSE, NULL );
        m_config = config;
        return CsyThread::Begin( );    
    }

    /**
     * @brief Stop Process
     */
    void Stop( void ) {
        if ( m_event ) {
            // wait for completion..
            ::SetEvent( m_event );
            CsyThread::Join();

            ::CloseHandle( m_event );
        }
        m_event = NULL;
    }

protected:
    /** 
     * @brief Thread hundler
     */
    virtual DWORD run( _In_ void* argment = NULL ) override {

        _SLOG( TEXT("==> Start > %s\n"), m_config.m_commandline );
        HRESULT _h = sy_create_process( m_config.m_commandline, m_proc_info ); 
        if ( FAILED( _h ) ) {
            _SLOG( TEXT("! Process Start Failed. in %08x\n"), _h );
            return 1;   // process create failed.
        }

        _SLOG( TEXT("==> [%d] Process Started.\n"), m_proc_info.dwProcessId );

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
                    _SLOG( TEXT("==> [%d] KILL Process \n"), m_proc_info.dwProcessId );
                    ::TerminateProcess( m_proc_info.hProcess, 0L );
                }
            }
            break;
        };

        if ( m_proc_info.hProcess ) 
            ::CloseHandle( m_proc_info.hProcess );

        _SLOG( TEXT("==> [%d] Process exit : code %d\n"), 
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

    void ForEach( std::function<void(CsyProcess*)> func ) {
        std::for_each( m_processes.begin(), m_processes.end  (), func );
    }
};