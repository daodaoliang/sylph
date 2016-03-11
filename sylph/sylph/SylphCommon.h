/**
 * @file     SylphCommon.h
 * @brief    Common Functions
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-07 
 *
 * https://github.com/horigome/sylph
 */
#pragma once
#include <Windows.h>
#include <process.h>

#include "msxml2.h"     // use MSXML
#pragma comment (lib,"msxml2.lib")

/**
 * @brief CoInitialize/CoUnInitializeをスコープ内で実施するクラス
 */
class CsyCoInitializer {
public:
    CsyCoInitializer ( void ) { ::CoInitialize( NULL ); }
    ~CsyCoInitializer( void ) { ::CoUninitialize( );    }
};

/**
 * @brief Application実行パスを取得します。
 */
inline
const CAtlString sy_get_running_dir( void ) {
    TCHAR _path[ MAX_PATH ];
    ::ZeroMemory( _path, sizeof( _path ) );
    ::GetModuleFileName( NULL, _path, _countof( _path ) );
    ::PathRemoveFileSpec( _path );

    return CAtlString( _path );
}

/** 
*  @brief 複数の同期待ち合わせ
*
*  @param[in] num_hundles ... 同期ハンドル数
*  @param[in] handles_p ... 同期ハンドルの配列ポインタ
*  @param[in] is_wait_all ... ** Unsupported Set to FALSE
*  @param[in] milisec ... 待機時間(ms) (Default:無限)
*  @param[in] is_check_msg ... メッセージキューをチェックする (Default:チェックする)
*/
inline DWORD 
sy_multi_join ( _In_ DWORD     num_hundles, 
             _In_ HANDLE*   handles_p, 
             _In_ BOOL      is_wait_all     = FALSE, 
             _In_ DWORD     milisec         = INFINITE, 
             _In_ BOOL      is_check_msg    = TRUE )
{
    DWORD _result = WAIT_FAILED;

    if ( !is_check_msg ) 
        return ::WaitForMultipleObjects( num_hundles, handles_p, is_wait_all, milisec );

    MSG _msg;
    ::PeekMessage ( &_msg, NULL, WM_USER, WM_USER, PM_NOREMOVE );

MJ_LOOP:
    _result = ::MsgWaitForMultipleObjects ( 
        num_hundles, handles_p, is_wait_all, milisec, QS_ALLEVENTS );

    if ( _result == ( WAIT_OBJECT_0 + num_hundles ) ) {

        while ( ::PeekMessage ( &_msg, NULL, 0, 0, PM_REMOVE ) ) { 

            if ( _msg.message == WM_QUIT )
                return _result;

            ::TranslateMessage ( &_msg );
            ::DispatchMessage  ( &_msg );
        }
        goto MJ_LOOP;
    }

    return _result;
}

/**
* @brief １つの同期待ち合わせ
*
* @param[in] handle ... 同期ハンドル
* @param[in] milisec ... 待機時間(ms) (Default:無限)
* @param[in] is_check_msg ... メッセージキューをチェックしない (Default:チェックする)
*/
inline DWORD 
sy_single_join ( _In_ HANDLE   handle, 
                 _In_ DWORD    milisec       = INFINITE, 
                 _In_ BOOL     is_check_msg  = TRUE ) {
    return sy_multi_join ( 1, &handle, FALSE, milisec, is_check_msg );
}

/**
 * @brief Simple Windows Thread Class.
 */
class CsyThread 
{
    HANDLE               m_hThread;     ///< this thread handle
    unsigned int         m_ThreadID;    ///< this thread ID.

protected:

    /** 
    * @brief スレッド実行関数。生成されたスレッドで実行されます。
    * @param[in] argment ... 生成時に渡したスレッドパラメータ
    * 
    7 @retval Thread終了コード 0... 正常
    */
    virtual DWORD run( _In_ void* argment = NULL ) = 0;

public:
    /** constructor */
    CsyThread( void ) : m_hThread( NULL ), m_ThreadID( 0 ) { }

    /** destructor (default) */
    virtual ~CsyThread( void ) = default;

    /** Is Thread Alive? */
    BOOL IsAlive( void ) const {
        if ( !m_hThread ) return FALSE;

        DWORD _code( 0 );
        if ( ::GetExitCodeThread( m_hThread, &_code ) )
            return ( _code == STILL_ACTIVE ? TRUE : FALSE );

        return FALSE;
    }

    /** Getting Thread Handle */
    HANDLE  IsHandle( void ) const { 
        return m_hThread; 
    }

    /** Getting this thread ID */
    unsigned int IsThreadID( void ) const { 
        return m_ThreadID; 
    } 

    /**
    * @brief １つのスレッドを開始します。
    *
    * @param[in] pArg ... スレッドパラメータ（Option）
    * @retval S_OK ... 成功
    */
    HRESULT Begin( _In_ void* argument = NULL ) {

        if ( m_hThread ) return E_FAIL;

        TTHREAD_ARG  _args = { 
            ::CreateEvent( NULL, FALSE, FALSE, NULL ), this, argument };

        m_hThread = (HANDLE)::_beginthreadex(
                NULL, 0, threadProc, &_args, 0, &m_ThreadID );

        if ( !m_hThread ) {
            ::CloseHandle( _args.hEv );
            return HRESULT_FROM_WIN32( ::GetLastError( ) );
        }

        // Thread パラメータ受け渡しの同期を行う
        sy_single_join( _args.hEv );
        ::CloseHandle( _args.hEv );

        return S_OK;
    }

    /**
    * @brief スレッド終了待ち合わせ
    *
    * @param[in] pExitCode ... スレッド終了コード（Option）
    *
    * @retval TRUE ... 成功
    */
    BOOL Join ( _Out_ LPDWORD exit_code_p = NULL ) {
        if ( !m_hThread ) return FALSE;

        if ( this->IsAlive( ) ) sy_single_join( m_hThread );
        if ( exit_code_p ) ::GetExitCodeThread( m_hThread, exit_code_p );

        m_hThread  = NULL;
        m_ThreadID = 0;
        return TRUE;
    }

    /**
     * @brief スレッドをTerminateする
     */
    void Kill( _In_ DWORD end_code = 0 ) {
        if ( m_hThread )
            ::TerminateThread( m_hThread, end_code );
    }

private:
    /** Threadパラメータ */
    struct TTHREAD_ARG {
        HANDLE          hEv;  
        CsyThread*      pThisObj;  
        void*           pArg;
    };

    /** Thread関数 **/
    static unsigned __stdcall threadProc( _In_ void* argment ) {
        if ( auto _obj = reinterpret_cast< TTHREAD_ARG* >( argment ) ) {
            TTHREAD_ARG localArg = *_obj;       // copy to local
            ::SetEvent( _obj->hEv );
            localArg.hEv = NULL;

            if ( localArg.pThisObj )
                ::_endthreadex( localArg.pThisObj->run( localArg.pArg ) );
        }
        return 0;
    }
};

/**
 * @brief Processを生成します
 *
 * @param[in] command ... 実行コマンド
 * @param[out] proc_info ... 生成したプロセス情報
 * @param[in] current_dir ... 起動時のカレントディレクトリ（NULL.. ModuleFilePath）
 */
inline HRESULT 
sy_create_process(  _In_    LPCTSTR                 command,
                    _Inout_ PROCESS_INFORMATION&    proc_info,
                    _In_    LPCTSTR                 current_dir = NULL ) {


    STARTUPINFO    _si;
    GetStartupInfo( &_si );

    size_t _arg_len = ::_tcslen( command ) + 1;
    LPTSTR _arg_p   = new TCHAR[ _arg_len ];
    if ( !_arg_p ) {
        return E_OUTOFMEMORY;
    }
    ::ZeroMemory( _arg_p, _arg_len );
    ::_tcscpy_s ( _arg_p, _arg_len, command );

    ZeroMemory( &proc_info, sizeof( proc_info ) );

    // カレントパスを設定
    ::SetCurrentDirectory( current_dir ? current_dir : sy_get_running_dir() );

    // Process 生成（Window非表示)
    BOOL _ret = ::CreateProcess( NULL, _arg_p, NULL, NULL, 
                    FALSE, CREATE_NO_WINDOW, NULL, NULL, 
                    &_si, &proc_info ) ;

    delete [] _arg_p;

    if ( !_ret ) {
        return HRESULT_FROM_WIN32( ::GetLastError() );
    }

    return S_OK;
}

/**
 * @brief XMLを読み込んで、DOMを生成します。
 *
 * @param[in] xml_file_name ... 読み込むXMLファイル名
 * @param[out] xml_document_pp ... XML DOM DOcument (後で解放すること）
 */
inline HRESULT 
sy_xml_open( _In_  LPCTSTR             xml_file_name, 
             _Out_ IXMLDOMDocument2    **xml_document_pp ) 
{
    if ( !xml_document_pp ) return E_POINTER;
    USES_CONVERSION;
    try {
        CComPtr< IXMLDOMDocument2 > _xml_doc;

        ATLENSURE_SUCCEEDED( _xml_doc.CoCreateInstance( CLSID_DOMDocument30 ) );
        ATLENSURE_SUCCEEDED( _xml_doc->put_async           ( VARIANT_FALSE ) );
        ATLENSURE_SUCCEEDED( _xml_doc->put_resolveExternals( VARIANT_FALSE ) );

        // Use. "XPath"
        ATLENSURE_SUCCEEDED( _xml_doc->setProperty( 
                CComBSTR( L"SelectionLanguage" ), CComVariant( L"XPath" ) ) );

        // Load from xml file
        VARIANT_BOOL _is_success = VARIANT_FALSE;
        ATLENSURE_SUCCEEDED( _xml_doc->load( 
                        CComVariant( CT2CW( xml_file_name ) ), &_is_success ) ); 

        if ( _is_success != VARIANT_TRUE ) {
            CComPtr< IXMLDOMParseError> _error;
            LONG _line(0), _pos(0), _err_code( 0 );
            _xml_doc->get_parseError( &_error );
            if ( _errno != NULL ) {
                CComBSTR _reason;
                _error->get_line     ( &_line     );
                _error->get_linepos  ( &_pos      );
                _error->get_errorCode( &_err_code );
                _error->get_reason   ( &_reason   );
                _SLOG( TEXT("! XML load failed. [%d:%d] code:%d\n"), 
                    _line, _pos, _err_code);
                return _err_code;
            }
            return E_FAIL;
        }

        ATLENSURE_SUCCEEDED( _xml_doc.QueryInterface( xml_document_pp ) ); 

    } catch ( CAtlException& e ) {
        return e.m_hr; 

    } catch ( ... ) {
        return E_FAIL;
    }
    return S_OK;
}

/**
 * @brief XML NodeListをParseし、関数をコールバックします。
 *
 * @param[in] xml_doc_p ... NodeListを検索する対象のXML DOM Document
 * @param[in] nodename ... nodelistを示すXPATH（例：/sylph/service/entry/process )
 * @param[out] func ... 関数オブジェクト。NodeList毎に呼ばれます。
 */
inline HRESULT
sy_xml_foreach_nodes( _In_ IXMLDOMDocument2*    xml_doc_p,
                      _In_ LPTSTR               nodename,
                      _In_ std::function<HRESULT(IXMLDOMNode*, long)> func ) {

    if ( !xml_doc_p ) return E_POINTER;

    CComPtr<IXMLDOMNodeList> nodelist_p;
    HRESULT _hr = xml_doc_p->selectNodes( CComBSTR( nodename ), &nodelist_p ); 
    if ( FAILED( _hr ) ) 
        return _hr;

    long _num = 0;
    nodelist_p->get_length( &_num );

    for ( long i=0; i<_num; i++ ) {
        IXMLDOMNode* _node_p = NULL;
        _hr = nodelist_p->get_item( i,  &_node_p );

        if ( _hr == S_OK ) { 
            _hr = func( _node_p, i );
            _node_p->Release( );
        }

        if ( FAILED( _hr ) ) return _hr;
    }

    return S_OK;
}

/**
 * @brief XML Textを得ます。　
 *
 * @param[in] node_p ... 取得するテキストを含むNode
 * @param[in] name ... Node名を示すXPath
 *
 * @retval ... テキスト（エラーの場合は、空白)
 */
inline CAtlString 
sy_xml_get_nodetext( _In_ IXMLDOMNode* node_p, _In_ LPCTSTR name ) {

    USES_CONVERSION;
    if ( !node_p ) return CAtlString(TEXT(""));

    CComBSTR _val;
    CComPtr<IXMLDOMNode> _node;
    if ( node_p->selectSingleNode( CComBSTR( name ), &_node ) == S_OK )  
        _node->get_text( &_val );

    return CAtlString( OLE2CT(_val) );
}


