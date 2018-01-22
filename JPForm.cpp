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

#include "JPForm.h"

extern NppData nppData;
extern SettingsManager *settings;
extern TAvailTasks availTasks;

bool settingsChanged;
std::vector<std::basic_string<TCHAR> > availIndex; // listbox task index -> availTasks map index

void UserInputToSettings();
void GetCheckBoxValue(int _buttonID, bool &_var);
void MoveTask(HWND _hSrcList, HWND _hDestList);
void MoveAllTasks(HWND _hSrcList, HWND _hDestList);
void OrderTask(HWND _hList, bool _up);

HWND hDlg
	,hListTskAvail
	,hListTskShown;

INT_PTR CALLBACK JPFormProc(HWND _hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	if (uMessage == WM_COMMAND)
	{
		switch (LOWORD(wParam)) 
		{
		// dialog buttons
		case IDOK:
			{
				if (settingsChanged)
				{
					UserInputToSettings();
					settings->Save();
				}

				EndDialog(hDlg, 2);
				return 1;
			}
		case IDCANCEL:
			{
				EndDialog(hDlg, 1);
				return 1;
			}

		// checkboxes
		case IDC_ENABLEJP_CB:
		case IDC_DEF_RECENT_CB:
		case IDC_TASKS_CB:
			{
				settingsChanged = true;
				break;
			}

		// list buttons
		case IDC_BUT_DEL_TSK:
			{
				MoveTask(hListTskShown, hListTskAvail);
				break;
			}
		case IDC_BUT_ADD_TSK:
			{
				MoveTask(hListTskAvail, hListTskShown);
				break;
			}
		case IDC_BUT_DELALL_TSK:
			{
				MoveAllTasks(hListTskShown, hListTskAvail);
				break;
			}
		case IDC_BUT_ADDALL_TSK:
			{
				MoveAllTasks(hListTskAvail, hListTskShown);
				break;
			}
		case IDC_BUT_UP_TSK:
			{
				OrderTask(hListTskShown, true);
				break;
			}
		case IDC_BUT_DOWN_TSK:
			{
				OrderTask(hListTskShown, false);
				break;
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
		hDlg = _hDlg;

		settingsChanged = false;

		// reset controls state according to current settings

		::CheckDlgButton(hDlg, IDC_ENABLEJP_CB,		settings->enableJP		? BST_CHECKED : BST_UNCHECKED);
		::CheckDlgButton(hDlg, IDC_DEF_RECENT_CB,	settings->showDefRecent	? BST_CHECKED : BST_UNCHECKED);
		::CheckDlgButton(hDlg, IDC_TASKS_CB,		settings->showTasks		? BST_CHECKED : BST_UNCHECKED);

		availIndex.clear();

		hListTskAvail = ::GetDlgItem(hDlg, IDC_LIST_TSK_AVAIL);

		hListTskShown = ::GetDlgItem(hDlg, IDC_LIST_TSK_SHOWN);

		if (hListTskAvail)
		{
			ListBox_ResetContent(hListTskAvail);

			for (TAvailTasks::iterator it = availTasks.begin(); it != availTasks.end(); ++it)
			{
				int pos = ListBox_AddString(hListTskAvail, it->second.taskName.c_str());

				availIndex.push_back(it->first);

				ListBox_SetItemData(hListTskAvail, pos, availIndex.size() - 1);
			}
		}

		if (hListTskShown)
		{
			ListBox_ResetContent(hListTskShown);

			for (int i = 0; i < settings->tasks.size(); ++i)
			{
				ListBox_SelectString(hListTskAvail, -1, availTasks[settings->tasks[i]].taskName.c_str());
				
				MoveTask(hListTskAvail, hListTskShown);
			}
		}

		// visual stuff ahead

		CenterWindow(hDlg, nppData._nppHandle, 0);

		HWND hEd;

		hEd = ::GetDlgItem(hDlg, IDC_BUT_UP_TSK);
		if (hEd)
			::SetWindowText(hEd, TEXT("˄"));

		hEd = ::GetDlgItem(hDlg, IDC_BUT_DOWN_TSK);
		if (hEd)
			::SetWindowText(hEd, TEXT("˅"));
	}

	return 0;
}

void UserInputToSettings()
{
	GetCheckBoxValue(IDC_ENABLEJP_CB,	settings->enableJP);
	GetCheckBoxValue(IDC_DEF_RECENT_CB,	settings->showDefRecent);
	GetCheckBoxValue(IDC_TASKS_CB,		settings->showTasks);

	settings->tasks.clear();

	for (int i = 0; i < ListBox_GetCount(hListTskShown); ++i)
	{
		int itemIndex = ListBox_GetItemData(hListTskShown, i);
		settings->tasks.push_back(availIndex[itemIndex]);
	}
}

void GetCheckBoxValue(int _buttonID, bool &_var)
{
	UINT cbState;
	
	cbState = ::IsDlgButtonChecked(hDlg, _buttonID);
	
	if (cbState == BST_CHECKED)
		_var = true;
	else if (cbState == BST_UNCHECKED)
		_var = false;
}

void MoveTask(HWND _hSrcList, HWND _hDestList)
{
	if (!_hSrcList || !_hDestList)
		return;
	
	int srcPos = ListBox_GetCurSel(_hSrcList);
	
	if (srcPos == LB_ERR)
		return;

	int itemIndex = ListBox_GetItemData(_hSrcList, srcPos);
	
	int destPos = ListBox_AddString(_hDestList, availTasks[availIndex[itemIndex]].taskName.c_str());
	ListBox_SetItemData(_hDestList, destPos, itemIndex);

	ListBox_DeleteString(_hSrcList, srcPos);

	settingsChanged = true;
}

void MoveAllTasks(HWND _hSrcList, HWND _hDestList)
{
	while (ListBox_GetCount(_hSrcList) > 0)
	{
		ListBox_SetCurSel(_hSrcList, 0);
		MoveTask(_hSrcList, _hDestList);
	}
}

void OrderTask(HWND _hList, bool _up)
{
	int pos = ListBox_GetCurSel(_hList);
	
	if (pos == LB_ERR)
		return;

	if (_up && (pos == 0))
		return;

	if (!_up && (pos  == (ListBox_GetCount(_hList) - 1)))
		return;

	int itemIndex = ListBox_GetItemData(_hList, pos);

	ListBox_DeleteString(_hList, pos);

	if (_up)
		pos--;
	else
		pos++;

	int destPos = ListBox_InsertString(_hList, pos, availTasks[availIndex[itemIndex]].taskName.c_str());
	ListBox_SetItemData(_hList, destPos, itemIndex);

	ListBox_SetCurSel(_hList, pos);

	settingsChanged = true;
}
