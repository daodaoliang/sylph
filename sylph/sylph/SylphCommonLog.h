/**
 * @file     SylphCommonLog.h
 * @brief    Common LOG Functions
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-07 
 *
 * https://github.com/horigome/sylph
 */
#pragma once
#include <Windows.h>

extern CAtlString  SERVICE_NAME;


/** Trace Header */
inline CAtlString 
_TRACE_HEAD( void ) {
    SYSTEMTIME _st;
    ::ZeroMemory( &_st, sizeof( _st ) );
    ::GetLocalTime( &_st );
    CAtlString _s;
    _s.Format( TEXT("[%04d-%02d-%02d %02d:%02d:%02d]: "),
        _st.wYear, _st.wMonth, _st.wDay, _st.wHour, _st.wMinute, _st.wSecond );
    return _s;
}

/** 
 * @brief Trace Template Function
 */
template <typename TFunc>
inline void 
_TRACE_FT_( _In_ TFunc f, _In_ LPCTSTR format_s, ... )
{
    TCHAR*      _msg_p =  NULL;
    va_list     _arg_list;

    va_start( _arg_list, format_s);

        const size_t _msg_len = _vsctprintf( format_s, _arg_list ) + 1;
        const size_t _size    =  _msg_len * sizeof( TCHAR ); 

        _msg_p  = ( TCHAR* )::malloc( _size );
        if ( !_msg_p ) goto  _TRACEF_END;

        ::ZeroMemory  ( _msg_p, _size );
        ::_vstprintf_s( _msg_p, _msg_len, format_s, _arg_list );

        f( _msg_p );

_TRACEF_END:
        if ( _msg_p ) ::free( _msg_p );

    va_end( _arg_list );
}

// std out
#define _TRACE_F_( fmt, ...) _TRACE_FT_( [](LPTSTR msg) { ::_tprintf_s( msg ); }, fmt, __VA_ARGS__ )
// DebugOut
#define _TRACE_D_( fmt, ...) _TRACE_FT_( [](LPTSTR msg) { ::OutputDebugString( msg ); }, fmt, __VA_ARGS__ )


/**
 * @brief Eventlog Output
 */
#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl 
_EVENT_F( _In_ WORD wType, 
          _In_z_ _Printf_format_string_ LPCTSTR pszFormat, ...) throw( ) {

    const int LOG_EVENT_MSG_SIZE = 256;

    TCHAR   _msg       [ LOG_EVENT_MSG_SIZE ];
    LPTSTR  _strings   [ 1 ];
    va_list _args;

    va_start( _args, pszFormat );
        _vsntprintf_s( _msg, LOG_EVENT_MSG_SIZE, LOG_EVENT_MSG_SIZE-1, 
                        pszFormat, _args );
    va_end( _args );

    _msg [ LOG_EVENT_MSG_SIZE - 1 ] = 0;
    _strings[ 0 ]                   = _msg;

    if ( HANDLE _event_h = ::RegisterEventSource( NULL, SERVICE_NAME ) ) {
        ::ReportEvent( _event_h, wType, 0, 0, NULL, 1, 0, 
                        (LPCTSTR*) &_strings[ 0 ], NULL);
        ::DeregisterEventSource( _event_h );
    }
}
#pragma warning(pop)  // disable 4793


// *---------------------------------------------------------------------------
// * Macro
// *---------------------------------------------------------------------------

#ifdef UNICODE
    #define WIDEN2(x) L ## x
    #define WIDEN(x) WIDEN2(x)

    #define __FUNC__ WIDEN(__FUNCTION__)
    #define __UFILE__ WIDEN(__FILE__)
#else
    #define __UFUNC__ __FILE__
#endif

// StdOut/DebugPrint
#define _SLOG( fmt, ...) _TRACE_F_( TEXT("%s") fmt, _TRACE_HEAD(), __VA_ARGS__ )
#define _SDBG( fmt, ...) _TRACE_D_( TEXT("%s") fmt, _TRACE_HEAD(), __VA_ARGS__ )

#define _STRACE( fmt, ...) _TRACE_F_( TEXT("[%s-(%04d)]: ") fmt, __UFILE__, __LINE__, __VA_ARGS__ )


// Event Out
#define EVENT_INF(...) _EVENT_F( EVENTLOG_INFORMATION_TYPE, __VA_ARGS__ )
#define EVENT_ERR(...) _EVENT_F( EVENTLOG_ERROR_TYPE,       __VA_ARGS__ )
#define EVENT_WAR(...) _EVENT_F( EVENTLOG_WARNING_TYPE,     __VA_ARGS__ )
#ifdef _DEBUG
#define EVENT_DBG(...) _EVENT_F( EVENTLOG_INFORMATION_TYPE, __VA_ARGS__ )
#else
#define EVENT_DBG(...) __noop
#endif
