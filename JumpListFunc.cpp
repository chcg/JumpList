//This file is part of notepad++ plugin NppJumpList
//Copyright © 2010 ahv <ahvsevolod@ya.ru>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "JumpListFunc.h"

LPCTSTR wszAppID = TEXT("DonHo.NotepadPlusPlus.NppJumpList.Ahv");
TAvailTasks availTasks;
extern SettingsManager *settings;
extern HINSTANCE dllInstance;

// this function will be called by jump list tasks via rundll32.exe. launches npp if it isn't running already.
void CALLBACK ParseJPCmdW(HWND hwnd, HINSTANCE hinst, LPWSTR lpCmdLine, int nCmdShow);
// sends jump list commands to npp
void SendNppCmd(LPTSTR cmd);

HRESULT _AddCategoryToList(ICustomDestinationList*, IObjectArray*);
HRESULT FillJumpListTasks(IObjectCollection* poc);
HRESULT CreateShellLink(PCTSTR pszArguments, PCTSTR pszTitle, IShellLink **ppsl);
bool CreateJumpList();
bool DestroyJumpList();

// fill map of all possible jump list tasks (add new here)
void FillAvailTasksMap();
void AddAvailTask(LPCTSTR _taskName, LPCTSTR _cmdName, UINT _msg, WPARAM _wParam, LPARAM _lParam);


bool InitJumpList()
{
	HRESULT hr = ::SetCurrentProcessExplicitAppUserModelID(wszAppID);
	
	if FAILED(hr)
		return false;

	hr = ::CoInitialize(NULL);

	if FAILED(hr)
		return false;
	
	FillAvailTasksMap();

	return true;
}

bool CreateJumpList()
{
	ICustomDestinationList* pcdl = NULL;
	
	HRESULT hr = ::CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));

	if FAILED(hr)
		return false;
	
	hr = pcdl->SetAppID(wszAppID);

	if FAILED(hr)
		return false;
	
	UINT uMaxSlots;
	IObjectArray *poaRemoved;
	hr = pcdl->BeginList(&uMaxSlots, IID_PPV_ARGS(&poaRemoved));

	if FAILED(hr)
		return false;
	
	if (settings->showDefRecent)
	{
		hr = pcdl->AppendKnownCategory(KDC_RECENT);
	
		if FAILED(hr)
			return false;
	}

	//hr = _AddCategoryToList(pcdl, poaRemoved);

	if (settings->showTasks)
	{
		IObjectCollection *poc;
		hr = ::CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
	
		if FAILED(hr)
			return false;

		FillJumpListTasks(poc);

		pcdl->AddUserTasks(poc);

		if FAILED(hr)
			return false;
	}

	pcdl->CommitList();

    poaRemoved->Release();

	return true;
}

bool DestroyJumpList()
{
	ICustomDestinationList* pcdl = NULL;
	
	HRESULT hr = ::CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));

	if FAILED(hr)
		return false;
	
	hr = pcdl->SetAppID(wszAppID);

	if FAILED(hr)
		return false;
	
	hr = pcdl->DeleteList(wszAppID);

	if FAILED(hr)
		return false;
	
	return true;
}

bool DeinitJumpList()
{
	::CoUninitialize();

	return true;
}

// This is the helper function that actually appends the items to a collection object
HRESULT _AddCategoryToList(ICustomDestinationList *pcdl, IObjectArray *poaRemoved)
{
    /*IObjectCollection *poc;
    HRESULT hr = CoCreateInstance
                    (CLSID_EnumerableObjectCollection, 
                    NULL, 
                    CLSCTX_INPROC_SERVER, 
                    IID_PPV_ARGS(&poc));
    if (SUCCEEDED(hr))
    {
        for (UINT i = 0; i < ARRAYSIZE(c_rgpszFiles); i++)
        {
            IShellItem *psi;
            if (SUCCEEDED(SHCreateItemInKnownFolder(
                                FOLDERID_Documents, 
                                KF_FLAG_DEFAULT, 
                                c_rgpszFiles[i], 
                                IID_PPV_ARGS(&psi)))
                )
            {
                if(!_IsItemInArray(psi, poaRemoved))
                {
                    poc->AddObject(psi);
                }

                psi->Release();
            }
        }

        IObjectArray *poa;
        hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
        if (SUCCEEDED(hr))
        {
            pcdl->AppendCategory(L"Custom category", poa);
            poa->Release();
        }
        poc->Release();
    }
	return hr;*/
	return 1;
}

HRESULT FillJumpListTasks(IObjectCollection* poc)
{
	HRESULT hr;
	IShellLink * psl;

	static std::basic_string<TCHAR> cmdLine;
	static bool cmdLinePrepared = false;

	if (!cmdLinePrepared)
	{
		TCHAR path[MAX_PATH];
		::GetModuleFileName(dllInstance, path, sizeof(path)-1); // get NppJumpList.dll path
		
		cmdLine = TEXT("\"");
		cmdLine += path;
		cmdLine += TEXT("\",ParseJPCmd ");

		::GetModuleFileName(NULL, path, sizeof(path)-1); // get notepad++.exe path
		
		cmdLine += TEXT(" \"");
		cmdLine += path;
		cmdLine += TEXT("\" ");
		
		cmdLinePrepared = true;
	}

	for (int i = 0; i < settings->tasks.size(); ++i)
	{
		hr = CreateShellLink((cmdLine + settings->tasks[i]).c_str(), availTasks[settings->tasks[i]].taskName.c_str(), &psl);

		if (SUCCEEDED(hr))
		{
	        hr = poc->AddObject(psl);
		    psl->Release();
		}
	}

	return 1;
}

HRESULT CreateShellLink(PCTSTR pszArguments, PCTSTR pszTitle, IShellLink **ppsl)
{
    IShellLink *psl;
    HRESULT hr = CoCreateInstance(
                    CLSID_ShellLink, 
                    NULL, 
                    CLSCTX_INPROC_SERVER, 
                    IID_PPV_ARGS(&psl));
    if (SUCCEEDED(hr))
    {
        hr = psl->SetPath(L"rundll32");
            if (SUCCEEDED(hr))
            {
                hr = psl->SetArguments(pszArguments);
                if (SUCCEEDED(hr))
                {
                    // The title property is required on Jump List items 
                    // provided as an IShellLink instance. This value is used 
                    // as the display name in the Jump List.
                    IPropertyStore *pps;
                    hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
                    if (SUCCEEDED(hr))
                    {
                        PROPVARIANT propvar;
                        hr = InitPropVariantFromString(pszTitle, &propvar);
                        if (SUCCEEDED(hr))
                        {
                            hr = pps->SetValue(PKEY_Title, propvar);
                            if (SUCCEEDED(hr))
                            {
                                hr = pps->Commit();
                                if (SUCCEEDED(hr))
                                {
                                    hr = psl->QueryInterface
                                            (IID_PPV_ARGS(ppsl));
                                }
                            }
                            PropVariantClear(&propvar);
                        }
                        pps->Release();
                    }
                }
            }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        psl->Release();
    }
    return hr;
}

void FillAvailTasksMap()
{
	availTasks.clear();
	
	//---------- Task Name -----------------------| Command Name -------------| Message --------------| wParam -------| lParam ------------------------|
	AddAvailTask(TEXT("New file"),					TEXT("newfile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_NEW);
	AddAvailTask(TEXT("Save all"),					TEXT("saveall"),			NPPM_SAVEALLFILES,		0,				0);
	AddAvailTask(TEXT("Close all"),					TEXT("closeall"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_CLOSEALL);
	AddAvailTask(TEXT("Open all recent"),			TEXT("openallrecent"),		NPPM_MENUCOMMAND,		0,				IDM_OPEN_ALL_RECENT_FILE);
	AddAvailTask(TEXT("Open file"),					TEXT("openfile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_OPEN);
	AddAvailTask(TEXT("Reload from disk"),			TEXT("reloadfromdisk"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_RELOAD);
	AddAvailTask(TEXT("Save file"),					TEXT("savefile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVE);
	AddAvailTask(TEXT("Save file as"),				TEXT("saveas"),				NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVEAS);
	AddAvailTask(TEXT("Save a copy as"),			TEXT("savecopyas"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVECOPYAS);
	AddAvailTask(TEXT("Close file"),				TEXT("closefile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_CLOSE);
	AddAvailTask(TEXT("Close all but active"),		TEXT("closeallbutactive"),	NPPM_MENUCOMMAND,		0,				IDM_FILE_CLOSEALL_BUT_CURRENT);
	AddAvailTask(TEXT("Delete from disk"),			TEXT("deletefromdisk"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_DELETE);
	AddAvailTask(TEXT("Load session"),				TEXT("loadsession"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_LOADSESSION);
	AddAvailTask(TEXT("Save session"),				TEXT("savesession"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVESESSION);
	AddAvailTask(TEXT("Print file"),				TEXT("printfile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_PRINT);
	AddAvailTask(TEXT("Print file now"),			TEXT("printnow"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_PRINTNOW);
	AddAvailTask(TEXT("Empty recent files list"),	TEXT("emptyrecentlist"),	NPPM_MENUCOMMAND,		0,				IDM_CLEAN_RECENT_FILE_LIST);
	//---------- Task Name -----------------------| Command Name -------------| Message --------------| wParam -------| lParam ------------------------|
}

void AddAvailTask(LPCTSTR _taskName, LPCTSTR _cmdName, UINT _msg, WPARAM _wParam, LPARAM _lParam)
{
	JPTaskProps tmpTaskProps;
	
	tmpTaskProps.taskName = _taskName;
	tmpTaskProps.msg = _msg;
	tmpTaskProps.wParam = _wParam;
	tmpTaskProps.lParam = _lParam;
	availTasks.insert(TAvailTasks::value_type(_cmdName, tmpTaskProps));
}

void CALLBACK ParseJPCmdW (
   HWND hwnd,           // handle to owner window
   HINSTANCE hinst,     // instance handle for the DLL
   LPWSTR lpszCmdLine,  // string the DLL will parse
   int nCmdShow         // show state
)
{
	FillAvailTasksMap();
	
	LPWSTR *args;
	int nArgs;
	
	args = ::CommandLineToArgvW(lpszCmdLine, &nArgs);
	
	if (!args)
		return;
	
	if (nArgs != 2)
		return;
	
	// args[0] == path to notepad++.exe
	// args[1] == jump list command (availTasks key)
	
	__try
	{
		if ((::OpenMutex(0, false, TEXT("nppInstance")) == NULL) && (::GetLastError() == ERROR_FILE_NOT_FOUND))
		{
			if (int(::ShellExecute(NULL, TEXT("open"), args[0], NULL, NULL, SW_SHOWNORMAL)) <= 32)
				return;
		
			::Sleep(100);

			SendNppCmd(args[1]);
		}
		else
		{
			SendNppCmd(args[1]);
		}
	}
	__finally
	{
		LocalFree(args);
	}
}

void SendNppCmd(LPTSTR cmd)
{
	TCHAR nppClassName[] = TEXT("Notepad++");

	HWND hNpp = ::FindWindow(nppClassName, NULL);
	
	for (int i = 0 ; !hNpp && i < 5 ; i++)
	{
		Sleep(100);
		hNpp = ::FindWindow(nppClassName, NULL);
	}
    
	if (hNpp)
    {
		int sw;

		if (::IsZoomed(hNpp))
			sw = SW_MAXIMIZE;
		else if (::IsIconic(hNpp))
			sw = SW_RESTORE;
		else
			sw = SW_SHOW;

		::ShowWindow(hNpp, sw);

		::SetForegroundWindow(hNpp);

		JPTaskProps tmpTaskProps = availTasks[std::basic_string<TCHAR>(cmd)];

		::SendMessage(hNpp, tmpTaskProps.msg, tmpTaskProps.wParam, tmpTaskProps.lParam);
	}
}

void ApplyJumpListSettings()
{
	if (settings->enableJP)
		CreateJumpList();
	else
		DestroyJumpList();
}
