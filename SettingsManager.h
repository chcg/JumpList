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

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H
//-------------------------------------------------------------------------
#include "PluginInterface.h"
#include "JumpListFunc.h"

class SettingsManager
{
private:
	std::basic_string<TCHAR> iniPath;

	void IniWriteBool(LPTSTR, LPTSTR, bool);
	void IniWriteString(LPTSTR, LPTSTR, std::basic_string<TCHAR>);
	bool IniReadBool(LPTSTR, LPTSTR, bool&);
	bool IniReadString(LPTSTR, LPTSTR, std::basic_string<TCHAR>&);
public:
	bool enableJP
		,showDefRecent
		,showTasks
		,showCustomRecent
		,showDefFrequent;

	std::vector<std::basic_string<TCHAR> > tasks;

	SettingsManager(LPCTSTR);
	SettingsManager();
	~SettingsManager();

	void Load();
	void Save();
};

//-------------------------------------------------------------------------
#endif //SETTINGSMANAGER_H
