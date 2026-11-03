// Explorer property handler for .qsys files.
//
// Reads the Designer version from the file header and exposes it read-only as
//   System.Comment                  "Q-SYS Designer 9.10.2"
//   System.Software.ProductVersion  "9.10.2"
//
// Registration (regsvr32, elevated) writes:
//   HKLM\Software\Classes\CLSID\{clsid}\InprocServer32
//   HKLM\...\PropertySystem\PropertyHandlers\.qsys = {clsid}
//   HKLM\Software\Classes\SystemFileAssociations\.qsys  FullDetails / PreviewDetails / InfoTip
// SystemFileAssociations is used instead of the ProgID because Designer's ProgID
// changes with each installed version ("Q-SYS Design 10.4").
#include <windows.h>
#include <shlobj.h>
#include <propsys.h>
#include <propkey.h>
#include <propvarutil.h>
#include <new>
#include <string>
#include "qsys_version.h"

#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "shlwapi.lib")

// {6F1E4B7A-2C3D-4E8F-9A1B-5C7D3E2F8A91}
static const CLSID CLSID_QsysPropertyHandler =
    { 0x6f1e4b7a, 0x2c3d, 0x4e8f, { 0x9a, 0x1b, 0x5c, 0x7d, 0x3e, 0x2f, 0x8a, 0x91 } };
// System.Software.ProductVersion is registered by Windows but not declared in propkey.h.
static const PROPERTYKEY PKEY_Software_ProductVersion =
    { { 0x0cef7d53, 0xfa64, 0x11d1, { 0xa2, 0x03, 0x00, 0x00, 0xf8, 0x1f, 0xed, 0xee } }, 8 };

static const wchar_t kClsidStr[] = L"{6F1E4B7A-2C3D-4E8F-9A1B-5C7D3E2F8A91}";
static const wchar_t kExt[] = L".qsys";
static const wchar_t kFriendlyName[] = L"Q-SYS Design Property Handler";

static const wchar_t kFullDetails[] =
    L"prop:System.PropGroup.Description;System.Comment;System.Software.ProductVersion;"
    L"System.PropGroup.FileSystem;System.ItemNameDisplay;System.ItemTypeText;System.ItemFolderPathDisplay;"
    L"System.Size;System.DateCreated;System.DateModified;System.FileAttributes";
static const wchar_t kPreviewDetails[] =
    L"prop:System.Comment;System.Software.ProductVersion;System.DateModified;System.Size";
static const wchar_t kInfoTip[] =
    L"prop:System.ItemTypeText;System.Comment;System.Size;System.DateModified";

static HMODULE g_module;
static long g_dllRefs;

class QsysPropertyHandler : public IPropertyStore, public IPropertyStoreCapabilities, public IInitializeWithStream
{
public:
    QsysPropertyHandler() { InterlockedIncrement(&g_dllRefs); }

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (!ppv)
            return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IPropertyStore)
            *ppv = static_cast<IPropertyStore*>(this);
        else if (riid == IID_IPropertyStoreCapabilities)
            *ppv = static_cast<IPropertyStoreCapabilities*>(this);
        else if (riid == IID_IInitializeWithStream)
            *ppv = static_cast<IInitializeWithStream*>(this);
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_refs); }
    IFACEMETHODIMP_(ULONG) Release()
    {
        long r = InterlockedDecrement(&m_refs);
        if (r == 0)
            delete this;
        return r;
    }

    // IInitializeWithStream
    IFACEMETHODIMP Initialize(IStream* stream, DWORD /*grfMode*/)
    {
        if (m_cache)
            return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);

        HRESULT hr = PSCreateMemoryPropertyStore(IID_PPV_ARGS(&m_cache));
        if (FAILED(hr))
            return hr;

        uint8_t buf[QSYS_HEADER_READ];
        ULONG total = 0;
        while (total < sizeof(buf))
        {
            ULONG got = 0;
            hr = stream->Read(buf + total, sizeof(buf) - total, &got);
            if (FAILED(hr) || got == 0)
                break;
            total += got;
        }

        // An unreadable or foreign file just gets no properties; that is not an error to Explorer.
        // No C++ exception may escape into the host process.
        try
        {
            QsysVersion v;
            if (ParseQsysVersion(buf, total, v))
            {
                std::wstring ver = v.ToString();
                std::wstring comment = L"Q-SYS Designer " + ver;
                SetCached(PKEY_Comment, comment.c_str());
                SetCached(PKEY_Software_ProductVersion, ver.c_str());
            }
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    // IPropertyStore
    IFACEMETHODIMP GetCount(DWORD* count) { return m_cache ? m_cache->GetCount(count) : E_UNEXPECTED; }
    IFACEMETHODIMP GetAt(DWORD i, PROPERTYKEY* key) { return m_cache ? m_cache->GetAt(i, key) : E_UNEXPECTED; }
    IFACEMETHODIMP GetValue(REFPROPERTYKEY key, PROPVARIANT* pv)
    {
        if (!m_cache)
            return E_UNEXPECTED;
        return m_cache->GetValue(key, pv);
    }
    IFACEMETHODIMP SetValue(REFPROPERTYKEY, REFPROPVARIANT) { return STG_E_ACCESSDENIED; }
    IFACEMETHODIMP Commit() { return STG_E_ACCESSDENIED; }

    // IPropertyStoreCapabilities
    IFACEMETHODIMP IsPropertyWritable(REFPROPERTYKEY) { return S_FALSE; }

private:
    ~QsysPropertyHandler()
    {
        if (m_cache)
            m_cache->Release();
        InterlockedDecrement(&g_dllRefs);
    }

    void SetCached(REFPROPERTYKEY key, const wchar_t* value)
    {
        PROPVARIANT pv;
        if (SUCCEEDED(InitPropVariantFromString(value, &pv)))
        {
            m_cache->SetValueAndState(key, &pv, PSC_NORMAL);
            PropVariantClear(&pv);
        }
    }

    long m_refs = 1;
    IPropertyStoreCache* m_cache = nullptr;
};

class ClassFactory : public IClassFactory
{
public:
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (!ppv)
            return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() { return 2; }
    IFACEMETHODIMP_(ULONG) Release() { return 1; }

    IFACEMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv)
    {
        *ppv = nullptr;
        if (outer)
            return CLASS_E_NOAGGREGATION;
        QsysPropertyHandler* h = new (std::nothrow) QsysPropertyHandler();
        if (!h)
            return E_OUTOFMEMORY;
        HRESULT hr = h->QueryInterface(riid, ppv);
        h->Release();
        return hr;
    }
    IFACEMETHODIMP LockServer(BOOL lock)
    {
        if (lock)
            InterlockedIncrement(&g_dllRefs);
        else
            InterlockedDecrement(&g_dllRefs);
        return S_OK;
    }
};

static ClassFactory g_factory;

// ---- registry helpers ------------------------------------------------------

static HRESULT SetRegString(HKEY root, const std::wstring& subkey, const wchar_t* name, const wchar_t* value)
{
    HKEY key;
    LONG rc = RegCreateKeyExW(root, subkey.c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value),
                            (DWORD)((wcslen(value) + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
    }
    return HRESULT_FROM_WIN32(rc);
}

static const std::wstring kClsidKey = std::wstring(L"Software\\Classes\\CLSID\\") + kClsidStr;
static const std::wstring kHandlerKey =
    std::wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\") + kExt;
static const std::wstring kAssocKey = std::wstring(L"Software\\Classes\\SystemFileAssociations\\") + kExt;

// ---- exports ---------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv)
{
    if (clsid != CLSID_QsysPropertyHandler)
    {
        *ppv = nullptr;
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    return g_factory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow()
{
    return g_dllRefs == 0 ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer()
{
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(g_module, path, ARRAYSIZE(path)))
        return HRESULT_FROM_WIN32(GetLastError());

    HRESULT hr;
    const std::wstring inproc = kClsidKey + L"\\InprocServer32";
    if (FAILED(hr = SetRegString(HKEY_LOCAL_MACHINE, kClsidKey, nullptr, kFriendlyName)) ||
        FAILED(hr = SetRegString(HKEY_LOCAL_MACHINE, inproc, nullptr, path)) ||
        FAILED(hr = SetRegString(HKEY_LOCAL_MACHINE, inproc, L"ThreadingModel", L"Both")) ||
        FAILED(hr = SetRegString(HKEY_LOCAL_MACHINE, kHandlerKey, nullptr, kClsidStr)) ||
        FAILED(hr = SetRegString(HKEY_LOCAL_MACHINE, kAssocKey, L"FullDetails", kFullDetails)) ||
        FAILED(hr = SetRegString(HKEY_LOCAL_MACHINE, kAssocKey, L"PreviewDetails", kPreviewDetails)) ||
        FAILED(hr = SetRegString(HKEY_LOCAL_MACHINE, kAssocKey, L"InfoTip", kInfoTip)))
        return hr;

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return S_OK;
}

STDAPI DllUnregisterServer()
{
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, kClsidKey.c_str());
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, kHandlerKey.c_str());

    HKEY key;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kAssocKey.c_str(), 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
    {
        RegDeleteValueW(key, L"FullDetails");
        RegDeleteValueW(key, L"PreviewDetails");
        RegDeleteValueW(key, L"InfoTip");
        RegCloseKey(key);
    }
    // Removes the key only if nothing else lives there.
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, kAssocKey.c_str());

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return S_OK;
}
