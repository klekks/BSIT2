#include "EventSink.h"

ULONG EventSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release()
{
    LONG lRef = InterlockedDecrement(&m_lRef);
    if (lRef == 0)
        delete this;
    return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = (IWbemObjectSink*)this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else return E_NOINTERFACE;
}


HRESULT EventSink::Indicate(long lObjectCount,
    IWbemClassObject** apObjArray)
{
    HRESULT hres = S_OK;
    BSTR ptr;

    for (int i = 0; i < lObjectCount; i++)
    {
        printf("===================EVENT INFO=====================\n");
        apObjArray[i]->GetObjectText(0, &ptr);
        printf("%S\n", ptr);
        printf("=================END EVENT INFO===================\n");
    }

    return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
    /* [in] */ LONG lFlags,
    /* [in] */ HRESULT hResult,
    /* [in] */ BSTR strParam,
    /* [in] */ IWbemClassObject __RPC_FAR* pObjParam
)
{
    if (lFlags == WBEM_STATUS_COMPLETE)
    {
        printf("Call complete. hResult = 0x%X\n", hResult);
    }
    else if (lFlags == WBEM_STATUS_PROGRESS)
    {
        printf("Call in progress.\n");
    }

    return WBEM_S_NO_ERROR;
}