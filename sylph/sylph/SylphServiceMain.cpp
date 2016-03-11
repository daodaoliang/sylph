/**
 * @file     SylphServiceMain.cpp
 * @brief    Windows Service Setup tools 
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-10
 *
 * https://github.com/horigome/sylph
 */
#include "stdafx.h"
#include "resource.h"
#include "SylphServiceSetup.h"
#include "SylphServiceControl.h"
#include "SylphProcessManager.h"

// Globals
LPTSTR      SYCONFIG_XML        = TEXT("syconfig.xml");
CAtlString  SERVICE_NAME        = TEXT("Sylph");
SYCONFIGS   SYLPH_PROC_CONFIG;

// Prototype ---
int         run_console ( void ); 
HRESULT     load_config ( SYCONFIGS&, CAtlString&  service_name, DWORD& start_type );
VOID WINAPI ServiceMain ( DWORD argc, LPTSTR *argv );

/**
 * @brief Sylph Service Modile class.
 */
class CsySylphService : public CsyServiceControl {
    
    CsylphProcessManager    m_proc; ///< Process Management 
protected:
    
    /** サービス開始時に呼ばれます。 */
    virtual HRESULT OnStart( void ) override { 

        for ( auto& conf : SYLPH_PROC_CONFIG ) 
            m_proc.AddProcessEntry( conf );

        EVENT_INF(TEXT("Service  Started."));
        return S_OK; 
    }

    /** サービス停止時に呼ばれます。 */
    virtual void OnStop( void ) override {
        EVENT_INF(TEXT("Service  Stoped."));
        m_proc.PurgeProcesses( );
        __super::OnStop( );
    }

public:
    CsySylphService         ( void ) = default;
    virtual ~CsySylphService( void ) = default;

    /** サービス名取得。XMLより名前を取得します 
        実装しない場合は、"Sylph" になります。
     */
    virtual LPCTSTR GetServiceName( void ) const override { 
        return SERVICE_NAME; 
    }
};

/**
 * @brief main function
 *
 * options:
 * ----------------------------------------------------------------------
 *   /install   ... Install a service
 *   /uninstall ... UnInstall a service
 *   /console   ... console test mode(for debug)
 *   /version   ... version information
 *
 */
extern "C"
int _tmain( _In_ int        argc, 
            _In_ _TCHAR*    argv[] ) {

    CsyCoInitializer _USE_COM;


    DWORD _start_type = SERVICE_DEMAND_START;
    HRESULT _hr = load_config( SYLPH_PROC_CONFIG, SERVICE_NAME, _start_type );
    if ( FAILED( _hr ) ) {
        _SLOG( TEXT("[ERR] Load configfile failed. in %08x\n"),_hr );
        return _hr;
    }

    //
    // Commandline Option
    //
    if ( argc >= 2 ) {
        if ( ::_tcscmp( TEXT("/install"), argv[1] ) == 0 ) {
            BOOL _ret = sy_sv_install( SERVICE_NAME, _start_type )  ;
            _SLOG( TEXT("Service Install. %d.\n"), _ret );
            return _ret;
        }
        else if ( ::_tcscmp( TEXT("/uninstall"), argv[1] ) == 0 ) {
            BOOL _ret = sy_sv_uninstall( SERVICE_NAME ) ;
            _SLOG( TEXT("Service UnInstall. %d.\n"), _ret );
            return _ret;
        }
        else if ( ::_tcscmp( TEXT("/console"), argv[1] ) == 0 ) {
            return run_console( );
        }
        else if ( ::_tcscmp( TEXT("/version"), argv[1] ) == 0 ) {
            CAtlString _ver;
            _ver.LoadString( IDS_VERSION );
            _tprintf_s( TEXT("Sylph service wrapper. Ver %s\n"),_ver );
            return 0;
        }
    }

    //
    // Service Start
    //
    TCHAR _SV_NAME [ 512 ];
    ::ZeroMemory( _SV_NAME, sizeof( _SV_NAME ) );
    ::_tcscpy_s ( _SV_NAME, _countof(_SV_NAME), SERVICE_NAME );

    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { _SV_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain },    // ServiceMain エントリポイント
        { NULL,         NULL                                  }     //< 最後のエントリはNULL値にする
    };

    if ( ::StartServiceCtrlDispatcher ( ServiceTable ) == FALSE) {
       int _err = ::GetLastError ( );
        _SLOG( TEXT("[ERR] StartServiceCtrlDispatcher failed. in %d\n"), _err );
       return _err;
    }

    return 0;
}

/**
 * @brief ServiceMain function 
 * ※ mainのStartServiceCtrlDispatcher()で指定される
 */
VOID WINAPI ServiceMain ( _In_ DWORD   argc, 
                          _In_ LPTSTR *argv  ) {
    // running service 
    CsySylphService _sv;
    _sv.WaitForCompleation( );
}

/**
 * @brief syconfig.xmlを読み込む（プロセス起動定義）
 */
HRESULT load_config( _Out_ SYCONFIGS&   configs, 
                     _Out_ CAtlString&  service_name,
                     _Out_ DWORD&       start_type ) {

    HRESULT _hr = S_OK;
    ::CoInitialize( NULL );
    { 
        CComPtr< IXMLDOMDocument2 > _xml;
        _hr = sy_xml_open( sy_get_running_dir() + TEXT("\\") + SYCONFIG_XML, &_xml );

        if ( _hr == S_OK ) {

            // parse.

            // ==> <sylph><service><config> ...  
            {
                // service_name
                CComPtr<IXMLDOMNode> _sv_name_p;
                if ( _xml->get_lastChild( &_sv_name_p ) == S_OK ) {
                    service_name = sy_xml_get_nodetext( 
                        _sv_name_p, TEXT("/sylph/service/config/service_name") );
                }
                if ( service_name.IsEmpty() ) service_name = TEXT("SylphService");

                CComPtr<IXMLDOMNode> _start_type_p;
                if ( _xml->get_lastChild( &_start_type_p ) == S_OK ) {

                    CAtlString _st_type_str = sy_xml_get_nodetext( 
                        _sv_name_p, TEXT("/sylph/service/config/start_type") );

                    start_type = ::_ttoi( _st_type_str );
                    switch ( start_type ) {
                    case SERVICE_AUTO_START:
                    case SERVICE_DEMAND_START:
                    case SERVICE_DISABLED:
                        break;
                    default:
                        start_type = SERVICE_DEMAND_START; 
                    }
                } else {
                    start_type = SERVICE_DEMAND_START; 
                }
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

    CAtlString _ver;
    _ver.LoadString( IDS_VERSION );
    _SLOG(TEXT("\n\n| Sylph Service Wrapper. \n| 2016 Version %s \n\n"), _ver );
    _SDBG(TEXT("--- Sylph ver. %s\n"), _ver );

    // Run Processes.
    _SLOG( TEXT("* Service name > %s\n"), SERVICE_NAME);
    _SLOG( TEXT("* Start Pricesses.\n"));
    CsylphProcessManager    _proc;
    for ( auto& conf : SYLPH_PROC_CONFIG ) _proc.AddProcessEntry( conf );

    _SLOG( TEXT("\n\n| please type any key.\n\n\n") );
    ::_getch();

    // Stop processes.
    _proc.PurgeProcesses();

    return 0;
}



