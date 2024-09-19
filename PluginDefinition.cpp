//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
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

#include "PluginDefinition.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

extern HINSTANCE dllInstance;

SettingsManager *settings;

void pluginInit()
{
	TCHAR configPath[MAX_PATH]{}, iniPath[MAX_PATH]{};
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);
	
	if (::PathFileExists(configPath) == FALSE)
		::CreateDirectory(configPath, NULL);
	
	_tcscpy(iniPath, configPath);
	_tcscat(iniPath, TEXT("\\NppJumpList.ini"));
	
	if (::PathFileExists(iniPath) == FALSE)
	{
		HANDLE	hFile			= NULL;
		#ifdef UNICODE
		CHAR	szBOM[]			= {0xFF, 0xFE};
		DWORD	dwByteWritten	= 0;
		#endif
			
		if (hFile != INVALID_HANDLE_VALUE)
		{
			hFile = ::CreateFile(iniPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			#ifdef UNICODE
			::WriteFile(hFile, szBOM, sizeof(szBOM), &dwByteWritten, NULL);
			#endif
			::CloseHandle(hFile);
		}
	}

	InitJumpList();

	settings = new SettingsManager(iniPath);
	settings->Load();
	
	ApplyJumpListSettings();
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
	DeinitJumpList();

	delete settings;
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Settings"), OpenSettingsDlg, NULL, false);
	setCommand(1, TEXT("About"), OpenAboutDlg, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void OpenSettingsDlg()
{
	INT_PTR nRet = ::DialogBox((HINSTANCE)dllInstance, MAKEINTRESOURCE(IDD_JPForm), nppData._nppHandle, JPFormProc);

	if (nRet == 2) // if ok'ed in dialog
	  ApplyJumpListSettings();
}

void OpenAboutDlg()
{
	INT_PTR nRet = ::DialogBox((HINSTANCE)dllInstance, MAKEINTRESOURCE(IDD_AboutDlg), nppData._nppHandle, AboutDlgProc);
}