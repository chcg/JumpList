//This file is part of notepad++ plugin NppJumpList
//Copyright � 2010 ahv <ahvsevolod@ya.ru>
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

#ifndef JUMPLISTFUNC_H
#define JUMPLISTFUNC_H

//
// All definitions of plugin interface
//

#include "PluginInterface.h"
#include "SettingsManager.h"
#include "resource.h"

struct JPTaskProps
{
	UINT msg = 0;
	WPARAM wParam = 0;
	LPARAM lParam = 0;
	std::basic_string<TCHAR> taskName;
	std::basic_string<TCHAR> iconFilePath;
	int iconResID = 0;
};

typedef std::map<std::basic_string<TCHAR>, JPTaskProps> TAvailTasks;

bool InitJumpList();
bool DeinitJumpList();
void ApplyJumpListSettings();

#endif //JUMPLISTFUNC_H
