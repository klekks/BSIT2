#include "EventSink.h"
#include <vector>
#include "params.h"

static std::vector<std::string> files;

std::string base_directory;
std::string filenames;

void get_file_names()
{
    base_directory.resize(128);
    filenames.resize(256);
    GetPrivateProfileString("additional", "base_directory", "", &base_directory[0], 4096, CONFIG_FILE_NAME);
    GetPrivateProfileString("additional", "file", "", &filenames[0], 4096, CONFIG_FILE_NAME);

    filenames += ",dir\\\\log.txt";

    while (!filenames.empty())
    {
        std::string file = &base_directory[0];
        size_t idx = filenames.find(',');

        if (idx == -1)
        {
            file += filenames;
            filenames.clear();
        }
        else
        {
            file.insert(file.end(), filenames.begin(), filenames.begin() + idx);
            filenames.erase(filenames.begin(), filenames.begin() + idx + 1);
        }
        
        files.push_back(file);
    }
   
}

int __main(int iArgCnt, char** argv)
{

    get_file_names();

    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        cout << "Failed to initialize COM library. Error code = 0x"
            << hex << hres << endl;
        return 1;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM negotiates service
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );


    if (FAILED(hres))
    {
        cout << "Failed to initialize security. Error code = 0x"
            << hex << hres << endl;
        CoUninitialize();
        return 1;                      // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object. "
            << "Err code = 0x"
            << hex << hres << endl;
        CoUninitialize();
        return 1;                 // Program has failed.
    }

    // Step 4: ---------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;

    // Connect to the local root\cimv2 namespace
    // and obtain pointer pSvc to make IWbemServices calls.
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x"
            << hex << hres << endl;
        pLoc->Release();
        CoUninitialize();
        return 1;                // Program has failed.
    }

    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx 
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx 
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x"
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 6: -------------------------------------------------
    // Receive event notifications -----------------------------

    // Use an unsecured apartment for security
    IUnsecuredApartment* pUnsecApp = NULL;

    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL,
        CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment,
        (void**)&pUnsecApp);
    
    EventSink* pSink = new EventSink;
    pSink->AddRef();

    IUnknown* pStubUnk = NULL;
    pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

    IWbemObjectSink* pStubSink = NULL;
    pStubUnk->QueryInterface(IID_IWbemObjectSink,
        (void**)&pStubSink);

    std::string base_query("SELECT * FROM __InstanceModificationEvent WITHIN 1 WHERE TargetInstance ISA 'CIM_DataFile' AND TargetInstance.Name=''");
    std::string dir_query ("SELECT * FROM __InstanceModificationEvent WITHIN 1 WHERE TargetInstance ISA 'CIM_DataFile' AND TargetInstance.Name~''");

    for (const auto& file : files)
    {
        std::string query;
        
        if (file.find('.') != -1)
            query = base_query;
        else
            query = dir_query;

        query.insert(query.size() - 1, file);

        pSvc->ExecNotificationQueryAsync(
            _bstr_t("WQL"),
            _bstr_t(query.c_str()),
            WBEM_FLAG_SEND_STATUS,
            NULL,
            pStubSink);
    }


    // Wait for the event
    Sleep(100000);

    hres = pSvc->CancelAsyncCall(pStubSink);

    // Cleanup
    // ========

    pSvc->Release();
    pLoc->Release();
    pUnsecApp->Release();
    pStubUnk->Release();
    pSink->Release();
    pStubSink->Release();
    CoUninitialize();

    return 0;   // Program successfully completed.

}