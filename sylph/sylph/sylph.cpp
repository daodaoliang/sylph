/**
 * @file     sylph.cpp
 * @brief    Windows Service Wrapper main.
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-07 
 */
#include "stdafx.h"
#include "resource.h"
#include "sylph_i.h"

#include <conio.h>

using namespace ATL;
#include <stdio.h>

#include "SyProcessManager.h"

/**
 * note. 
 *
 *   コマンドライン（管理者権限）
 *   サービスの登録
 *     > sylph.exe /Service
 *   サービスの解除
 *     > sylph.exe /UnRegServer
 *   サービスの開始
 *     > net start sylph
 *   サービスの停止
 *     > net stop sylph
 */

#define EVENT_INF(...) LogEvent2( EVENTLOG_INFORMATION_TYPE, __VA_ARGS__ )
#define EVENT_ERR(...) LogEvent2( EVENTLOG_ERROR_TYPE,       __VA_ARGS__ )
#define EVENT_WAR(...) LogEvent2( EVENTLOG_WARNING_TYPE,     __VA_ARGS__ )
#ifdef _DEBUG
#define EVENT_DBG(...) LogEvent2( EVENTLOG_INFORMATION_TYPE, __VA_ARGS__ )
#else
#define EVENT_DBG(...) __noop
#endif

/** 設定ファイル名 */
#define SYCONFIG_XML TEXT("syconfig.xml")

// function protorype.
HRESULT load_config( _Out_ SYCONFIGS&   configs, 
                     _Out_ CAtlString&  service_name );

/**
 * @brief Service Module
 */
class CsylphModule : public ATL::CAtlServiceModuleT< CsylphModule, IDS_SERVICENAME >
{
    SYCONFIGS               m_confs;
    CsylphProcessManager    m_proc_man;     ///< Process Manager
public :
    DECLARE_LIBID(LIBID_sylphLib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SYLPH, "{9B4678B4-9063-4FD3-A9AC-9ED5A324AD5D}")

    /** 
     * @brief constructor. * Load "syconfig.xml"
     */
    CsylphModule( void ) {
        CAtlString _sv_name;
        load_config( m_confs, _sv_name );
        ::_tcscpy_s( m_szServiceName, _countof( m_szServiceName ), _sv_name );
    }

    /** destructor (default) */
    virtual ~CsylphModule( void ) = default;

    /** セキュリティ設定　※必要に応じて設定する */
    HRESULT InitializeSecurity( void ) throw() { return S_OK; }

    /**
     * @brief サービス main。サービス開始時に必ず実行されます。(PreMessageLoopの前）
     */
    void ServiceMain( _In_ DWORD dwArgc, _In_ LPTSTR* lpszArgv ) throw( ) {
        __super::ServiceMain( dwArgc, lpszArgv ); 
    }

    /** サービス開始時に実行される。 */
    HRESULT PreMessageLoop  ( int nShowCmd ) {

        HRESULT _hr = __super::PreMessageLoop( nShowCmd );
        if ( FAILED( _hr ) ) {
            EVENT_ERR( TEXT( "PreMessageLoop failed. %08x"), _hr );
            return _hr;
        }

        m_proc_man.PurgeProcesses();
        for ( auto& conf : m_confs ) {
            _hr = m_proc_man.AddProcessEntry( conf );
            if ( FAILED ( _hr ) ) {
                EVENT_ERR( TEXT( "! Process start failed. %08x > %s"), 
                    _hr, conf.m_commandline);

                m_proc_man.PurgeProcesses();
                return _hr;
            }
        }

        EVENT_DBG( TEXT("-- Service Started. ") );
        return _hr;
    }

    /** サービス終了時に事項される。 */
    HRESULT PostMessageLoop ( void ) { 
        m_proc_man.PurgeProcesses( );
        EVENT_DBG( TEXT("-- Service Stoped. ") );
        return __super::PostMessageLoop( ); 
    }

    // ----------------------------------------------------------------------
    // EventHandler 
    // ----------------------------------------------------------------------

    /**
     * @brief サービス停止処理（SERVICE_CONTROL_STOP）
     */
    void OnStop() throw( ) {
       EVENT_DBG( TEXT("-- Service Stoped. ") );
        __super::OnStop();
    }

    /** サービス一時停止処理（SERVICE_CONTROL_PAUSE） */
    void OnPause( void ) throw() { __super::OnPause(); }

    /** サービス再開処理（SERVICE_CONTROL_CONTINUE） */
    void OnContinue( void ) throw() { __super::OnContinue(); }

    /** サービス問い合わせ処理（SERVICE_CONTROL_INTERROGATE） */
    void OnInterrogate( void ) throw() { __super::OnInterrogate(); }

    /** サービスシャットダウン処理（SERVICE_CONTROL_SHUTDOWN） */
    void OnShutdown( void ) throw() { __super::OnShutdown(); }

    /** 不明なサービス処理 */
    void OnUnknownRequest( _In_ DWORD dwOpcode ) throw() {
        __super::OnUnknownRequest(dwOpcode);
    }


    // ----------------------------------------------------------------------
    // Helper 
    // ----------------------------------------------------------------------

    /**
     * @brief イベントログ出力（CAtlServiceModuleTより移植）
     *        Typeを指定できるように拡張しました。IDが0になるため表示はリソースが不明と出ます。
     * @param[in] wType ... EVENTLOG_INFORMATION_TYPEなどのイベントログタイプ
     * @param[in] pszFormat ... フォーマット文字列
     */
#pragma warning(push)  // disable 4793
#pragma warning(disable : 4793)
    void __cdecl LogEvent2( 
        _In_ WORD wType,
        _In_z_ _Printf_format_string_ LPCTSTR pszFormat, ...) throw() {

        const int LOG_EVENT_MSG_SIZE = 256;

        TCHAR   chMsg[LOG_EVENT_MSG_SIZE];
        HANDLE  hEventSource;
        LPTSTR  lpszStrings[1];
        va_list pArg;

        va_start(pArg, pszFormat);
            _vsntprintf_s( chMsg, LOG_EVENT_MSG_SIZE, 
                    LOG_EVENT_MSG_SIZE-1, pszFormat, pArg);
        va_end(pArg);

        chMsg[ LOG_EVENT_MSG_SIZE - 1 ] = 0;
        lpszStrings[0] = chMsg;

        if (!m_bService) {
            _putts(chMsg);
        }

        /* Get a handle to use with ReportEvent(). */
        hEventSource = RegisterEventSource(NULL, m_szServiceName);
        if (hEventSource != NULL) {
            /* Write to event log. */
            ReportEvent(hEventSource, wType, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
            DeregisterEventSource(hEventSource);
        }
    }
#pragma warning(pop)  // disable 4793
};

// Singleton
CsylphModule _AtlModule;


/**
 * @brief syconfig.xmlを読み込む（プロセス起動定義）
 */
HRESULT load_config( _Out_ SYCONFIGS&   configs, 
                     _Out_ CAtlString&  service_name ) {

    HRESULT _hr = S_OK;
    ::CoInitialize( NULL );
    { 
        CComPtr< IXMLDOMDocument2 > _xml;
        _hr = sy_xml_open( sy_get_running_dir() + TEXT("\\") + SYCONFIG_XML, &_xml );

        if ( _hr == S_OK ) {

            // parse.
            // ==> <sylph><service><config> ...  
            {
                CComPtr<IXMLDOMNode> _node_p;
                if ( _xml->get_lastChild( &_node_p ) == S_OK ) {
                    service_name = sy_xml_get_nodetext( 
                        _node_p, TEXT("/sylph/service/config/service_name") );
                }
                if ( service_name.IsEmpty() ) service_name = TEXT("SylphService");
            }

            // ==> <sylph><service><entry>...</entry></service></service>
            _hr = sy_xml_foreach_nodes( _xml, TEXT("/sylph/service/entry/process"), 
                [&configs](IXMLDOMNode* node_p, long /*idx*/) -> HRESULT {

                // .. <command>xxxx</command>
                    auto _cmd = sy_xml_get_nodetext( node_p, TEXT("command") );
                    if ( _cmd.GetLength() ) 
                        configs.push_back( CsyProcConfig( _cmd ) );

                    return S_OK;
            } );
        }
    }
    ::CoUninitialize();
    return _hr;
}

/**
 * @brief Console run. (for debug)
 *        for "/console"  commandline option
 */
int run_console( void ) {

    int _con = sy_create_console();

    CAtlString _ver;
    _ver.LoadString( IDS_VERSION );
    _SLOG(TEXT("\n\n| Sylph Service Wrapper. \n| 2016 Version %s \n\n"), _ver );

    CAtlString              _sv_name;
    SYCONFIGS               _confs;
    CsylphProcessManager    _proc;

    HRESULT _hr = load_config( _confs, _sv_name );
    if ( FAILED( _hr ) ) {
        // XML Load Fail.
        _SLOG( TEXT("! syconfig.xml load failed. in %08x\n"), _hr );
    } else {
        // Run Processes.
        _SLOG( TEXT("* Service name > %s\n"), _sv_name);
        _SLOG( TEXT("* Start Pricesses.\n"));
        for ( auto& conf : _confs ) _proc.AddProcessEntry( conf );
    }

    _SLOG( TEXT("\n\n| please type any key.\n\n\n") );
    ::_getch();

    // Stop processes.
    _proc.PurgeProcesses();

    sy_close_console( _con );
    return 0;
}

/**
 * @brief win main
 */
extern "C" int WINAPI 
_tWinMain(  _In_ HINSTANCE  /*hInstance*/, 
            _In_ HINSTANCE  /*hPrevInstance*/, 
            _In_ LPTSTR     /*lpCmdLine*/, 
            _In_ int        nShowCmd )
{
    // commnd line parse.
    int _num_args = 0;
    if ( LPWSTR* _args = ::CommandLineToArgvW( GetCommandLineW( ), &_num_args ) ) {
        for ( int i = 0; i < _num_args; i++ ) {

            // /console 
            // ==> console mode (for debug )
            if ( ::wcscmp( _args[ i ], L"/console" ) == 0 ) {
                int _r = run_console( );
                ::LocalFree( _args );
                return _r;
            }
            // /gen
            // ==> generate template (xml)
            else if ( ::wcscmp( _args[ i ], L"/gen" ) == 0 ) {
                // not impl.
                // :-(
            }
            
        }
        ::LocalFree( _args ); 
    }
   
    // Service mode
    return _AtlModule.WinMain(nShowCmd);
}

