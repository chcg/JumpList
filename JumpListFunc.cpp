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
#include "menuCmdID.h"
#include <string>

LPCTSTR wszAppID = TEXT("DonHo.NotepadPlusPlus.NppJumpList.Ahv");
TAvailTasks availTasks;
extern SettingsManager *settings;
extern HINSTANCE dllInstance;
extern NppData nppData;

const TCHAR MODE_TASK_CMD = TEXT('T');
const TCHAR MODE_RECENT_CMD = TEXT('R');

const UINT recentID = IDM_FILEMENU_LASTONE + 1; // ID of first recent files menu item
const UINT recentMax = 30;
const int NPP_MENUINDEX_FILE = 0;

TCHAR DEFAULT_ICON_PATH[MAX_PATH] = TEXT("");
const int DEFAULT_ICON_RESID = 100; // npp main icon

bool initedCOM = false, inited = false;

TCHAR DLL_NAME[MAX_PATH] = {0};

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
HRESULT CreateShellLink(PCTSTR _path, PCTSTR _arguments, PCTSTR _title, IShellLink **_ppsl, LPCTSTR _iconFilePath, int _iconIndex);
bool IsLinkRemoved(PCTSTR _testStr, IObjectArray *_removedArr);

// so they do
bool CreateJumpList();
bool DestroyJumpList();

// fill map of all possible jump list tasks (add new here)
void FillAvailTasksMap();
void AddAvailTask(LPCTSTR _taskName, LPCTSTR _cmdName, UINT _msg, WPARAM _wParam, LPARAM _lParam, LPCTSTR _iconFilePath, int _iconResID);

bool InitJumpList()
{
	inited = initedCOM = false;

	HRESULT hr = ::SetCurrentProcessExplicitAppUserModelID(wszAppID);

	if FAILED(hr)
		return false;

	initedCOM = SUCCEEDED(::CoInitializeEx(0, COINIT_APARTMENTTHREADED));

	::GetModuleFileName(NULL, DEFAULT_ICON_PATH, ARRAYSIZE(DEFAULT_ICON_PATH));

	TCHAR dllPath[MAX_PATH];
	::GetModuleFileName(dllInstance, dllPath, ARRAYSIZE(dllPath));
	_tcscpy(DLL_NAME, ::PathFindFileName(dllPath));

	FillAvailTasksMap();

	inited = true;
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
	if (initedCOM)
	{
		::CoUninitialize();
		initedCOM = false;
	}

	inited = false;

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
	HMENU hNppFileMenu = ::GetSubMenu(hNppMainMenu, NPP_MENUINDEX_FILE);

	std::vector<std::wstring> recentFilePaths;
	std::vector<std::wstring> recentFileNames;
	std::vector<std::wstring> recentFileIDs;
	std::map<int, int> menuPosToVecPos;
	recentFilePaths.clear();
	recentFileNames.clear();
	recentFileIDs.clear();
	menuPosToVecPos.clear();

	for (UINT i = 0; i < recentMax; ++i)
	{
		wchar_t buf[1024] = {0};
		std::wstring numStr = TEXT("");

		MENUITEMINFO itemInfo = {0};
		itemInfo.cbSize = sizeof(MENUITEMINFO);
		itemInfo.fMask = MIIM_STRING;
		itemInfo.dwTypeData = buf;
		itemInfo.cch = sizeof(buf);

		if (::GetMenuItemInfo(hNppFileMenu, recentID + i, false, &itemInfo))
		{
			recentFilePaths.push_back(buf);

			size_t pos = recentFilePaths.back().rfind(TEXT('\\'));

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
			for (size_t j = 0; j < recentFilePaths.back().size(); ++j)
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

			menuPosToVecPos.insert(std::pair<int, int>(_ttoi(numStr.c_str()), static_cast<int>(recentFilePaths.size() - 1)));

			recentFileNames.push_back(recentFilePaths.back().substr(pos+1));

			_itot(recentID + i, buf, 10);
			recentFileIDs.push_back(buf);
		}
	}

	// TODO: get shell icons
	// upd: 1. the most proper way is to just register npp as non-default handler for file types on "file opened" notification,
	//      and use default recent list or use shellitems in custom list.
	//      2. get hIcon's and save needed icons in tmp file. ugly and not always correct.

	for (std::map<int, int>::iterator it = menuPosToVecPos.begin(); it != menuPosToVecPos.end(); ++it)
	{
		int vecPos = it->second;

		std::wstring linkStr = GetDefArgsLine(MODE_RECENT_CMD) + recentFileIDs[vecPos] + TEXT(" \"") + recentFilePaths[vecPos] + TEXT("\"");

		if (IsLinkRemoved(linkStr.c_str(), poaRemoved))
			continue;

		hr = CreateShellLink(TEXT("rundll32"),
							 linkStr.c_str(),
							 recentFileNames[vecPos].c_str(),
							 &psl,
							 DEFAULT_ICON_PATH, DEFAULT_ICON_RESID);

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
	JPTaskProps taskProps;

	for (size_t i = 0; i < settings->tasks.size(); ++i)
	{
		taskProps = availTasks[settings->tasks[i]];

		hr = CreateShellLink(TEXT("rundll32"),
							 (GetDefArgsLine(MODE_TASK_CMD) + settings->tasks[i]).c_str(),
							 taskProps.taskName.c_str(),
							 &psl,
							 taskProps.iconFilePath.c_str(), taskProps.iconResID);

		if (SUCCEEDED(hr))
		{
	        hr = poc->AddObject(psl);
		    psl->Release();
		}
	}

	return 1;
}

HRESULT CreateShellLink(PCTSTR _path, PCTSTR _arguments, PCTSTR _title, IShellLink **_ppsl, LPCTSTR _iconFilePath, int _iconIndex)
{
	IShellLink *psl = NULL;
	IPropertyStore *pps = NULL;
	PROPVARIANT propvar = {0};

	HRESULT hr = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));

	__try
	{
		if (FAILED(hr))
			__leave;

		hr = psl->SetPath(_path);

		if (FAILED(hr))
			__leave;

		hr = psl->SetArguments(_arguments);

		if (FAILED(hr))
			__leave;

		hr = psl->SetIconLocation(_iconFilePath, -_iconIndex); // minus sign means it's resource id

		if (FAILED(hr))
			__leave;

		hr = psl->QueryInterface(IID_PPV_ARGS(&pps));

		if (FAILED(hr))
			__leave;

		hr = ::InitPropVariantFromString(_title, &propvar);

		if (FAILED(hr))
			__leave;

		hr = pps->SetValue(PKEY_Title, propvar);

		if (FAILED(hr))
			__leave;

		hr = pps->Commit();

		if (FAILED(hr))
			__leave;

		hr = psl->QueryInterface(IID_PPV_ARGS(_ppsl));
	}
	__finally
	{
		if (pps != NULL)
			pps->Release();

		if (psl != NULL)
			psl->Release();

		if (propvar.vt != VT_EMPTY)
			::PropVariantClear(&propvar);
	}

	return hr;
}

void FillAvailTasksMap()
{
	availTasks.clear();

	TCHAR dllPath[MAX_PATH] = TEXT("");
	::GetModuleFileName(dllInstance, dllPath, ARRAYSIZE(dllPath));

	PWSTR sysDir;
	TCHAR shellDllPath[MAX_PATH] = TEXT("");
	if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_System, 0, NULL, &sysDir)))
	{
		_tcscat(shellDllPath, sysDir);
		_tcscat(shellDllPath, TEXT("\\shell32.dll"));
		::CoTaskMemFree(sysDir);
	}

	//----------| Task Name ------------------------| Command Name -------------| Message --------------| wParam ---| lParam -----------------------| Icon File Path ---| Icon Resource ID -|
	AddAvailTask( TEXT("New file")					, TEXT("newfile")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_NEW					, dllPath			, IDI_ICON_NEWFILE	);
	AddAvailTask( TEXT("Save all")					, TEXT("saveall")			, NPPM_SAVEALLFILES		, 0			, 0								, dllPath			, IDI_ICON_SAVEALL	);
	AddAvailTask( TEXT("Close all")					, TEXT("closeall")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_CLOSEALL				, dllPath			, IDI_ICON_CLOSEALL	);
	AddAvailTask( TEXT("Open all recent")			, TEXT("openallrecent")		, NPPM_MENUCOMMAND		, 0			, IDM_OPEN_ALL_RECENT_FILE		, dllPath			, IDI_ICON_OPENFILE	);
	AddAvailTask( TEXT("Open file")					, TEXT("openfile")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_OPEN					, dllPath			, IDI_ICON_OPENFILE	);
	AddAvailTask( TEXT("Reload from disk")			, TEXT("reloadfromdisk")	, NPPM_MENUCOMMAND		, 0			, IDM_FILE_RELOAD				, shellDllPath		, 16739				);
	AddAvailTask( TEXT("Save file")					, TEXT("savefile")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_SAVE					, dllPath			, IDI_ICON_SAVEFILE	);
	AddAvailTask( TEXT("Save file as")				, TEXT("saveas")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_SAVEAS				, dllPath			, IDI_ICON_SAVEFILE	);
	AddAvailTask( TEXT("Save a copy as")			, TEXT("savecopyas")		, NPPM_MENUCOMMAND		, 0			, IDM_FILE_SAVECOPYAS			, dllPath			, IDI_ICON_SAVEFILE	);
	AddAvailTask( TEXT("Close file")				, TEXT("closefile")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_CLOSE				, dllPath			, IDI_ICON_CLOSEFILE);
	AddAvailTask( TEXT("Close all but active")		, TEXT("closeallbutactive")	, NPPM_MENUCOMMAND		, 0			, IDM_FILE_CLOSEALL_BUT_CURRENT	, dllPath			, IDI_ICON_CLOSEALL	);
	AddAvailTask( TEXT("Delete from disk")			, TEXT("deletefromdisk")	, NPPM_MENUCOMMAND		, 0			, IDM_FILE_DELETE				, shellDllPath   	, 240               );
	AddAvailTask( TEXT("Load session")				, TEXT("loadsession")		, NPPM_MENUCOMMAND		, 0			, IDM_FILE_LOADSESSION			, dllPath			, IDI_ICON_OPENFILE	);
	AddAvailTask( TEXT("Save session")				, TEXT("savesession")		, NPPM_MENUCOMMAND		, 0			, IDM_FILE_SAVESESSION			, dllPath			, IDI_ICON_SAVEFILE	);
	AddAvailTask( TEXT("Print file")				, TEXT("printfile")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_PRINT				, dllPath			, IDI_ICON_PRINTNOW	);
	AddAvailTask( TEXT("Print file now")			, TEXT("printnow")			, NPPM_MENUCOMMAND		, 0			, IDM_FILE_PRINTNOW				, dllPath			, IDI_ICON_PRINTNOW	);
	AddAvailTask( TEXT("Empty recent files list")	, TEXT("emptyrecentlist")	, NPPM_MENUCOMMAND		, 0			, IDM_CLEAN_RECENT_FILE_LIST	, shellDllPath   	, 261               );
	//----------| Task Name ------------------------| Command Name -------------| Message --------------| wParam ---| lParam -----------------------| Icon File Path ---| Icon Resource ID -|
}

void AddAvailTask(LPCTSTR _taskName, LPCTSTR _cmdName, UINT _msg, WPARAM _wParam, LPARAM _lParam, LPCTSTR _iconFilePath, int _iconResID)
{
	JPTaskProps tmpTaskProps;

	tmpTaskProps.taskName = _taskName;
	tmpTaskProps.msg = _msg;
	tmpTaskProps.wParam = _wParam;
	tmpTaskProps.lParam = _lParam;
	tmpTaskProps.iconFilePath = _iconFilePath;
	tmpTaskProps.iconResID = _iconResID;
	availTasks.insert(TAvailTasks::value_type(_cmdName, tmpTaskProps));
}

void CALLBACK ParseJPCmdW (
   HWND /*hwnd*/,           // handle to owner window
   HINSTANCE /*hinst*/,     // instance handle for the DLL
   LPWSTR lpszCmdLine,  // string the DLL will parse
   int /*nCmdShow*/         // show state
)
{
	FillAvailTasksMap();

	int nArgs = 0;

	LPTSTR* args = ::CommandLineToArgvW(lpszCmdLine, &nArgs);

	if (!args)
		return;

	if (nArgs < 1)
		return;

	__try
	{
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
			if (INT_PTR(::ShellExecute(NULL, TEXT("open"), args[1], NULL, NULL, SW_SHOWNORMAL)) <= 32)
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
		JPTaskProps tmpTaskProps = availTasks[std::wstring(cmd)];

		::SendMessage(hNpp, tmpTaskProps.msg, tmpTaskProps.wParam, tmpTaskProps.lParam);
	}
}

void SendNppRecentCmd(LPTSTR /*_idStr*/, LPTSTR _menuStr)
{
	HWND hNpp = FindNppWindow();

	if (!hNpp)
		return;

	HMENU hNppMainMenu = ::GetMenu(hNpp);
	HMENU hNppFileMenu = ::GetSubMenu(hNppMainMenu, NPP_MENUINDEX_FILE);

	bool foundItem = false;
	UINT itemID = 0;

	for (int i = 0; i < recentMax; ++i)
	{
		TCHAR buf[1024] = {0};
		MENUITEMINFO itemInfo = {0};
		itemInfo.cbSize = sizeof(MENUITEMINFO);
		itemInfo.fMask = MIIM_STRING;
		itemInfo.dwTypeData = buf;
		itemInfo.cch = sizeof(buf);

		if (!::GetMenuItemInfo(hNppFileMenu, recentID + i, false, &itemInfo))
			continue;

		if (!_tcscmp(buf, _menuStr))
		{
			foundItem = true;
			itemID = recentID + i;
			break;
		}
	}

	if (foundItem)
		::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, itemID);
	//else // TODO: something's wrong, recreate jump list - can't do it here, we're in rundll32 process right now.
	//	ApplyJumpListSettings();
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
	if (!inited)
		return;

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

		// something (plugin) sent menu command
		if ((retStruct->message == NPPM_MENUCOMMAND) && (retStruct->lParam == IDM_CLEAN_RECENT_FILE_LIST))
			ApplyJumpListSettings();

		// something is opening file via menu command or someone is opening file via custom jump list
		// this is needed for correct custom jump list in case file can't be opened
		if ((retStruct->message == NPPM_MENUCOMMAND) && (retStruct->lParam > IDM_FILEMENU_LASTONE) && (retStruct->lParam < (IDM_FILEMENU_LASTONE + recentMax)))
			ApplyJumpListSettings();
	}

	return ::CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK NppHookMessageProc(__in  int nCode, __in  WPARAM wParam, __in  LPARAM lParam)
{
	if (nCode >= HC_ACTION)
	{
		MSG* msg = ((MSG*)lParam);

		// notepad++ menu commands themselfes
		// PostMessage because at this stage the menu hasn't been updated yet

		if ((msg->message == WM_COMMAND) && (LOWORD(msg->wParam) == IDM_CLEAN_RECENT_FILE_LIST))
			::PostMessage(nppData._nppHandle, NPPM_MSGTOPLUGIN, (WPARAM)DLL_NAME, (LPARAM)1);

		if ((msg->message == WM_COMMAND) && (LOWORD(msg->wParam) > IDM_FILEMENU_LASTONE) && (LOWORD(msg->wParam) < (IDM_FILEMENU_LASTONE + recentMax)))
			::PostMessage(nppData._nppHandle, NPPM_MSGTOPLUGIN, (WPARAM)DLL_NAME, (LPARAM)1);
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

		if (SUCCEEDED(link->GetArguments(buf, ARRAYSIZE(buf))))
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
	::GetModuleFileName(dllInstance, path, ARRAYSIZE(path));

	cmdLine = TEXT("\"");
	cmdLine += path;
	cmdLine += TEXT("\",ParseJPCmd ");

	// then arguments
	// mode
	cmdLine += _mode;
	cmdLine += TEXT(' ');

	// notepad++.exe path
	::GetModuleFileName(NULL, path, ARRAYSIZE(path));

	cmdLine += TEXT(" \"");
	cmdLine += path;
	cmdLine += TEXT("\" ");

	// final mode specific arguments get appended somewhere else

	return cmdLine;
}
