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

extern FuncItem funcItem[nbFunc];
extern NppData nppData;
HINSTANCE dllInstance = NULL;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	dllInstance = (HINSTANCE)hModule;
	
	switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
		{
		  TCHAR procPath[65535] = {0};
		  ::GetModuleFileName(NULL, procPath, sizeof(procPath)-1);

		  if (_tcsicmp(PathFindFileName(procPath), TEXT("rundll32.exe")) != 0)
		    pluginInit(hModule);
		}
        break;

      case DLL_PROCESS_DETACH:
		commandMenuCleanUp();
        pluginCleanUp();
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

//#include <fstream>
//using namespace std;
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	/*int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return;
    HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;*/

	/*char tmp[123];
	itoa(notifyCode->nmhdr.code, tmp, 10);

	static int qqq = 0;*/

    //::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)(&tmp[0]));*/

	/*ofstream qwe("C:\\wwww.txt", ios::app);
	qwe << qqq << ":\t" << tmp << endl;
	qwe.close();

	qqq++;*/

	/*if ((notifyCode->nmhdr.code == NPPN_FILEBEFOREOPEN) || (notifyCode->nmhdr.code == NPPN_FILEOPENED)) 
	{
		setFormatGal();
	}*/
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
*/
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
