#pragma once
#include "resource.h"
#include "sylph_i.h"

using namespace ATL;

/**
 * @brief COM Object ※特に使わない
 */
class ATL_NO_VTABLE CSylphObject :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSylphObject, &CLSID_SylphObject>,
    public IDispatchImpl<ISylphObject, &IID_ISylphObject, 
                    &LIBID_sylphLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
    CSylphObject         ( void ) = default;
    virtual ~CSylphObject( void ) = default;

    DECLARE_REGISTRY_RESOURCEID(IDR_SYLPHOBJECT)
    BEGIN_COM_MAP(CSylphObject)
        COM_INTERFACE_ENTRY(ISylphObject)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    void FinalRelease( void ) { } 
};

OBJECT_ENTRY_AUTO(__uuidof(SylphObject), CSylphObject)
