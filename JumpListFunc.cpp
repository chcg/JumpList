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
extern NppData nppData;

const TCHAR MODE_TASK_CMD = TEXT('T');
const TCHAR MODE_RECENT_CMD = TEXT('R');
const UINT recentID = IDM_FILEMENU_LASTONE + 1; // ID of first recent files menu item
const UINT recentMax = 30;

// this function will be called by jump list tasks via rundll32.exe. launches npp if it isn't running already.
void CALLBACK ParseJPCmdW(HWND hwnd, HINSTANCE hinst, LPWSTR lpCmdLine, int nCmdShow);
// send jump list commands to npp
void SendNppTaskCmd(LPTSTR cmd);
void SendNppRecentCmd(LPTSTR _idStr, LPTSTR _menuStr);
HWND FindNppWindow();

// hook callback functions for tracking recent list changes
LRESULT CALLBACK NppHookCallWndRetProc(__in  int nCode, __in  WPARAM wParam, __in  LPARAM lParam);
LRESULT CALLBACK NppHookMessageProc(__in  int nCode, __in  WPARAM wParam, __in  LPARAM lParam);

// populates custom recent list
HRESULT AddCategoryToList(ICustomDestinationList*, IObjectArray*);

// populates tasks list
HRESULT FillJumpListTasks(IObjectCollection* poc);

// helper functions for those populators
std::basic_string<TCHAR> GetDefArgsLine(TCHAR _mode);
HRESULT CreateShellLink(PCTSTR _path, PCTSTR _arguments, PCTSTR _title, IShellLink **_ppsl);
bool IsLinkRemoved(PCTSTR _testStr, IObjectArray *_removedArr);

// so they do
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

	if (settings->showDefFrequent)
	{
		hr = pcdl->AppendKnownCategory(KDC_FREQUENT);

		if FAILED(hr)
			return false;
	}

	if (settings->showCustomRecent)
	{
		hr = AddCategoryToList(pcdl, poaRemoved);
		
		if FAILED(hr)
			return false;
		
		// intercept messages for tracking recent list changes
		::SetWindowsHookEx(WH_CALLWNDPROCRET, (HOOKPROC)NppHookCallWndRetProc, NULL, ::GetCurrentThreadId());
		::SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC)NppHookMessageProc, NULL, ::GetCurrentThreadId());
	}

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

HRESULT AddCategoryToList(ICustomDestinationList *pcdl, IObjectArray *poaRemoved)
{
    IObjectCollection *poc;
    HRESULT hr = ::CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&poc));
	IShellLink * psl;

	if (FAILED(hr))
		return hr;

	HMENU hNppMainMenu = ::GetMenu(nppData._nppHandle);
	HMENU hNppFileMenu = ::GetSubMenu(hNppMainMenu, 0); // MENUINDEX_FILE = 0

	std::vector<tstring> recentFilePaths;
	std::vector<tstring> recentFileNames;
	std::vector<tstring> recentFileIDs;
	std::map<int, int> menuPosToVecPos;
	recentFilePaths.clear();
	recentFileNames.clear();
	recentFileIDs.clear();
	menuPosToVecPos.clear();
	
	for (int i = 0; i < recentMax; ++i)
	{
		TCHAR buf[1024] = {0};
		tstring numStr = TEXT("");
		
		MENUITEMINFO itemInfo = {0};
		itemInfo.cbSize = sizeof(MENUITEMINFO);
		itemInfo.fMask = MIIM_STRING;
		itemInfo.dwTypeData = buf;
		itemInfo.cch = sizeof(buf);

		if (::GetMenuItemInfo(hNppFileMenu, recentID + i, false, &itemInfo))
		{
			recentFilePaths.push_back(buf);
			
			int pos = recentFilePaths.back().rfind(TEXT('\\'));
			
			if (pos == recentFilePaths.back().npos)
			{
				pos = recentFilePaths.back().rfind(TEXT(": "));

				if (pos == recentFilePaths.back().npos)
				{
					recentFilePaths.pop_back();
					continue;
				}
				else
				{
					pos++; // because of 2-char search
				}
			}

			// extract number
			for (int j = 0; j < recentFilePaths.back().size(); ++j)
			{
				#ifdef UNICODE
				if (iswdigit(recentFilePaths.back()[j]))
				#else
				if (isdigit(recentFilePaths.back()[j]))
				#endif
				{
					numStr += recentFilePaths.back()[j];
					continue;
				}

				if (recentFilePaths.back()[j] != TEXT('&'))
					break;
			}
			
			menuPosToVecPos.insert(std::pair<int, int>(_ttoi(numStr.c_str()), recentFilePaths.size() - 1));
			
			recentFileNames.push_back(recentFilePaths.back().substr(pos+1));

			_itot(recentID + i, buf, 10);
			recentFileIDs.push_back(buf);
		}
	}
	
	for (std::map<int, int>::iterator it = menuPosToVecPos.begin(); it != menuPosToVecPos.end(); ++it)
    {
		int vecPos = it->second;

        if (IsLinkRemoved(recentFileNames[vecPos].c_str(), poaRemoved))
			continue;
		
		hr = ::CreateShellLink(TEXT("rundll32"),
							   (GetDefArgsLine(MODE_RECENT_CMD) + recentFileIDs[vecPos] /*+ TEXT(" ") + recentFileNames[i]*/ + TEXT(" \"") + recentFilePaths[vecPos] + TEXT("\"")).c_str(),
							   recentFileNames[vecPos].c_str(),
							   &psl);

		if (SUCCEEDED(hr))
		{
			poc->AddObject(psl);

			psl->Release();
		}
    }

    IObjectArray *poa;
    hr = poc->QueryInterface(IID_PPV_ARGS(&poa));

    if (SUCCEEDED(hr))
    {
		pcdl->AppendCategory(settings->showDefRecent ? TEXT("Recent++") : TEXT("Recent"), poa);
        poa->Release();
    }
    poc->Release();

	return hr;
}

HRESULT FillJumpListTasks(IObjectCollection* poc)
{
	HRESULT hr;
	IShellLink * psl;

	for (int i = 0; i < settings->tasks.size(); ++i)
	{
		hr = ::CreateShellLink(TEXT("rundll32"),
							   (GetDefArgsLine(MODE_TASK_CMD) + settings->tasks[i]).c_str(),
							   availTasks[settings->tasks[i]].taskName.c_str(),
							   &psl);

		if (SUCCEEDED(hr))
		{
	        hr = poc->AddObject(psl);
		    psl->Release();
		}
	}

	return 1;
}

HRESULT CreateShellLink(PCTSTR _path, PCTSTR _arguments, PCTSTR _title, IShellLink **_ppsl)
{
    IShellLink *psl;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));

    if (SUCCEEDED(hr))
    {
		hr = psl->SetPath(_path);
		
        if (SUCCEEDED(hr))
        {
            hr = psl->SetArguments(_arguments);
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
                    hr = InitPropVariantFromString(_title, &propvar);
                    if (SUCCEEDED(hr))
                    {
                        hr = pps->SetValue(PKEY_Title, propvar);
                        if (SUCCEEDED(hr))
                        {
                            hr = pps->Commit();
                            if (SUCCEEDED(hr))
                            {
                                hr = psl->QueryInterface(IID_PPV_ARGS(_ppsl));
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
	
	//----------| Task Name -----------------------| Command Name -------------| Message --------------| wParam -------| lParam -----------------------|
	AddAvailTask( TEXT("New file"),					TEXT("newfile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_NEW					);
	AddAvailTask( TEXT("Save all"),					TEXT("saveall"),			NPPM_SAVEALLFILES,		0,				0								);
	AddAvailTask( TEXT("Close all"),				TEXT("closeall"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_CLOSEALL				);
	AddAvailTask( TEXT("Open all recent"),			TEXT("openallrecent"),		NPPM_MENUCOMMAND,		0,				IDM_OPEN_ALL_RECENT_FILE		);
	AddAvailTask( TEXT("Open file"),				TEXT("openfile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_OPEN					);
	AddAvailTask( TEXT("Reload from disk"),			TEXT("reloadfromdisk"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_RELOAD					);
	AddAvailTask( TEXT("Save file"),				TEXT("savefile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVE					);
	AddAvailTask( TEXT("Save file as"),				TEXT("saveas"),				NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVEAS					);
	AddAvailTask( TEXT("Save a copy as"),			TEXT("savecopyas"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVECOPYAS				);
	AddAvailTask( TEXT("Close file"),				TEXT("closefile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_CLOSE					);
	AddAvailTask( TEXT("Close all but active"),		TEXT("closeallbutactive"),	NPPM_MENUCOMMAND,		0,				IDM_FILE_CLOSEALL_BUT_CURRENT	);
	AddAvailTask( TEXT("Delete from disk"),			TEXT("deletefromdisk"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_DELETE					);
	AddAvailTask( TEXT("Load session"),				TEXT("loadsession"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_LOADSESSION			);
	AddAvailTask( TEXT("Save session"),				TEXT("savesession"),		NPPM_MENUCOMMAND,		0,				IDM_FILE_SAVESESSION			);
	AddAvailTask( TEXT("Print file"),				TEXT("printfile"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_PRINT					);
	AddAvailTask( TEXT("Print file now"),			TEXT("printnow"),			NPPM_MENUCOMMAND,		0,				IDM_FILE_PRINTNOW				);
	AddAvailTask( TEXT("Empty recent files list"),	TEXT("emptyrecentlist"),	NPPM_MENUCOMMAND,		0,				IDM_CLEAN_RECENT_FILE_LIST		);
	//----------| Task Name -----------------------| Command Name -------------| Message --------------| wParam -------| lParam -----------------------|
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
	
	LPTSTR *args;
	int nArgs;
	
	args = ::CommandLineToArgvW(lpszCmdLine, &nArgs);
	
	if (!args)
		return;
	
	__try
	{
		if (nArgs < 1)
			return;
		
		// args[0] == mode
		// args[1] == path to notepad++.exe
		
		switch (*args[0])
		{
		case MODE_TASK_CMD:
			{
				// args[2] == jump list command (availTasks key)
				if (nArgs != 3)
					return;
				
				break;
			}
		case MODE_RECENT_CMD:
			{
				// args[2] == menu id
				// args[3] == recent file path (could be shortened by menu)
				if (nArgs != 4)
					return;
				
				break;
			}
		}
		
		if ((::OpenMutex(0, false, TEXT("nppInstance")) == NULL) && (::GetLastError() == ERROR_FILE_NOT_FOUND))
		{
			if (int(::ShellExecute(NULL, TEXT("open"), args[1], NULL, NULL, SW_SHOWNORMAL)) <= 32)
				return;
			
			::Sleep(100);
		}
		
		switch (*args[0])
		{
		case MODE_TASK_CMD:
			{
				SendNppTaskCmd(args[2]);
				break;
			}
		case MODE_RECENT_CMD:
			{
				SendNppRecentCmd(args[2], args[3]);
				break;
			}
		}
	}
	__finally
	{
		::LocalFree(args);
	}
}

void SendNppTaskCmd(LPTSTR cmd)
{
	HWND hNpp = FindNppWindow();

	if (hNpp)
	{
		JPTaskProps tmpTaskProps = availTasks[tstring(cmd)];

		::SendMessage(hNpp, tmpTaskProps.msg, tmpTaskProps.wParam, tmpTaskProps.lParam);
	}
}

void SendNppRecentCmd(LPTSTR _idStr, LPTSTR _menuStr)
{
	HWND hNpp = FindNppWindow();

	if (!hNpp)
		exit;

	UINT itemId = _ttoi(_idStr);

	HMENU hNppMainMenu = ::GetMenu(hNpp);
	HMENU hNppFileMenu = ::GetSubMenu(hNppMainMenu, 0); // MENUINDEX_FILE = 0

	MENUITEMINFO itemInfo = {0};
	TCHAR buf[1024] = {0};
	itemInfo.cbSize = sizeof(MENUITEMINFO);
	itemInfo.fMask = MIIM_STRING;
	itemInfo.dwTypeData = buf;
	itemInfo.cch = sizeof(buf);
	
	if (!::GetMenuItemInfo(hNppFileMenu, itemId, false, &itemInfo))
		exit;

	if (_tcscmp(buf, _menuStr))
		exit; // wrong menu item, abort

	::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, itemId);
}

HWND FindNppWindow()
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
	}

	return hNpp;
}

void ApplyJumpListSettings()
{
	if (settings->enableJP)
		CreateJumpList();
	else
		DestroyJumpList();
}

LRESULT CALLBACK NppHookCallWndRetProc(__in  int nCode, __in  WPARAM wParam, __in  LPARAM lParam)
{
	if (nCode >= HC_ACTION)
	{
		CWPRETSTRUCT* retStruct = ((CWPRETSTRUCT*)lParam);
		
		if ((retStruct->message == NPPM_MENUCOMMAND) && (retStruct->lParam == IDM_CLEAN_RECENT_FILE_LIST)) // something (plugin) sent menu command
			CreateJumpList();
	}	

	return ::CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK NppHookMessageProc(__in  int nCode, __in  WPARAM wParam, __in  LPARAM lParam)
{
	if (nCode >= HC_ACTION)
	{
		MSG* msg = ((MSG*)lParam);
		
		if ((msg->message == WM_COMMAND) && (LOWORD(msg->wParam) == IDM_CLEAN_RECENT_FILE_LIST)) // notepad++ menu command itself
			::SendMessage(msg->hwnd, NPPM_MENUCOMMAND, 0, IDM_CLEAN_RECENT_FILE_LIST); // because at this stage menu hasn't been updated yet
	}	

	return ::CallNextHookEx(0, nCode, wParam, lParam);
}

bool IsLinkRemoved(PCTSTR _testStr, IObjectArray *_removedArr)
{
	IShellLink *link;
	UINT arrSz;
	TCHAR buf[1024];
	
	if (FAILED(_removedArr->GetCount(&arrSz)))
		return false;
	
	for (UINT i = 0; i < arrSz; ++i)
	{
		if (FAILED(_removedArr->GetAt(i, IID_PPV_ARGS(&link))))
			continue;
		
		if (SUCCEEDED(link->GetArguments(buf, sizeof(buf))))
		{
			if (!_tcscmp(_testStr, buf))
			{
				link->Release();
				return true;
			}
		}
		
		link->Release();
	}

	return false;
}

std::basic_string<TCHAR> GetDefArgsLine(TCHAR _mode)
{
	static std::basic_string<TCHAR> cmdLine;
	
	TCHAR path[MAX_PATH];

	// first append NppJumpList.dll path
	::GetModuleFileName(dllInstance, path, sizeof(path)-1);
		
	cmdLine = TEXT("\"");
	cmdLine += path;
	cmdLine += TEXT("\",ParseJPCmd ");

	// then arguments
	// mode
	cmdLine += _mode;
	cmdLine += TEXT(' ');
		
	// notepad++.exe path
	::GetModuleFileName(NULL, path, sizeof(path));
		
	cmdLine += TEXT(" \"");
	cmdLine += path;
	cmdLine += TEXT("\" ");
	
	// final mode specific arguments get appended someplace else
	
	return cmdLine;
}