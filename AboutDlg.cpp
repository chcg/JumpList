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

#include "AboutDlg.h"

extern NppData nppData;
extern HINSTANCE dllInstance;

// get version from resource
bool GetProductVersion(std::basic_string<TCHAR> &_prodVer);

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
  if (uMessage == WM_COMMAND)
  {
    switch (LOWORD(wParam)) 
    {
      case IDOK:
	  case IDCANCEL:
      {
        EndDialog(hDlg, 1);
        return 1;
      }
      default:
        break;
    }
  }

  else if (uMessage == WM_SYSCOMMAND)
  {
    if (wParam == SC_CLOSE)
    {
      EndDialog(hDlg, 0);
      return 1;
    }
  }

  else if (uMessage == WM_INITDIALOG)
  {
    CenterWindow(hDlg, nppData._nppHandle, 0);

	HWND hCtrl = ::GetDlgItem(hDlg, IDC_BUILD_INFO);
	
	std::basic_string<TCHAR> prodVer;
	GetProductVersion(prodVer);

	if (hCtrl)
		::SetWindowText(hCtrl, (std::basic_string<TCHAR>(TEXT("Version ")) + prodVer + TEXT("\r\nBuilt on ") +
		                        TEXT(__DATE__) + TEXT(" at ") + TEXT(__TIME__)).c_str());
  }

  return 0;
}

bool GetProductVersion(std::basic_string<TCHAR> &_prodVer)
{
    // get the filename of the dll
    TCHAR szFilename[65536] = {0};
    if (!::GetModuleFileName(dllInstance, szFilename, MAX_PATH))
        return false;
    
    // allocate a block of memory for the version info
    DWORD dummy;
    DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
    if (!dwSize)
        return false;
    
	std::vector<BYTE> data(dwSize);

    // load the version info
    if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0]))
        return false;
    
    // get the name and version strings
    LPVOID pvProductName = NULL;
    unsigned int iProductNameLen = 0;
    LPVOID pvProductVersion = NULL;
    unsigned int iProductVersionLen = 0;

    // 000904b0 == language ID
    if (!VerQueryValue(&data[0], _T("\\StringFileInfo\\000904b0\\ProductName"), &pvProductName, &iProductNameLen) ||
        !VerQueryValue(&data[0], _T("\\StringFileInfo\\000904b0\\ProductVersion"), &pvProductVersion, &iProductVersionLen))
        return false;

    _prodVer = (LPCTSTR)pvProductVersion;

    return true;
}
