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

#include "SettingsManager.h"

extern TAvailTasks availTasks;

SettingsManager::SettingsManager(LPCTSTR _iniPath)
{
	iniPath = _iniPath;

	// default settings
	enableJP = true;
	showDefRecent = false;
	showTasks = true;
	showDefFrequent = false;
	showCustomRecent = true;

	tasks.clear();
	tasks.push_back(TEXT("newfile"));
}

SettingsManager::SettingsManager()
{
}

SettingsManager::~SettingsManager()
{
}

void SettingsManager::Load() // assumes that the default settings are set
{
	IniReadBool(TEXT("Main"), TEXT("EnableJP"), enableJP);
	IniReadBool(TEXT("Main"), TEXT("ShowDefRecent"), showDefRecent);
	IniReadBool(TEXT("Main"), TEXT("ShowTasks"), showTasks);
	IniReadBool(TEXT("Main"), TEXT("ShowDefFrequent"), showDefFrequent);
	IniReadBool(TEXT("Main"), TEXT("ShowCustomRecent"), showCustomRecent);

	TCHAR strNum[64];
	int i = 0;
	std::basic_string<TCHAR> tmpStr;
	
	while (IniReadString(TEXT("Tasks"), _itot(i, strNum, 10), tmpStr))
	{
		// since there are saved tasks then delete default ones in first iteration
		if (!i)
			tasks.clear();

		// check if such task exists before loading
		if (availTasks.find(tmpStr) != availTasks.end())
			tasks.push_back(tmpStr);

		i++;
	}
}

void SettingsManager::Save()
{
	IniWriteBool(TEXT("Main"), TEXT("EnableJP"), enableJP);
	IniWriteBool(TEXT("Main"), TEXT("ShowDefRecent"), showDefRecent);
	IniWriteBool(TEXT("Main"), TEXT("ShowTasks"), showTasks);
	IniWriteBool(TEXT("Main"), TEXT("ShowDefFrequent"), showDefFrequent);
	IniWriteBool(TEXT("Main"), TEXT("ShowCustomRecent"), showCustomRecent);
	
	TCHAR sectionStr[65536] = {0}, strNum[64];

	for (int i = 0; i < tasks.size(); ++i)
	{
		_tcscat(sectionStr, _itot(i, strNum, 10));
		_tcscat(sectionStr, TEXT("="));
		_tcscat(sectionStr, tasks[i].c_str());
		_tcscat(sectionStr, TEXT("\r\n\0"));
	}

	_tcscat(sectionStr, TEXT("\0"));

	::WritePrivateProfileSection(TEXT("Tasks"), sectionStr, iniPath.c_str());
}

void SettingsManager::IniWriteBool(LPTSTR _section, LPTSTR _key, bool _var)
{
	::WritePrivateProfileString(_section, _key, _var?TEXT("1"):TEXT("0"), iniPath.c_str());
}

void SettingsManager::IniWriteString(LPTSTR _section, LPTSTR _key, std::basic_string<TCHAR> _var)
{
	::WritePrivateProfileString(_section, _key, _var.c_str(), iniPath.c_str());
}

bool SettingsManager::IniReadBool(LPTSTR _section, LPTSTR _key, bool &_var)
{
	TCHAR retStr[64];
	
	::GetPrivateProfileString(_section, _key, TEXT("INI_ERROR_DEFAULT_VALUE"), retStr, sizeof(retStr), iniPath.c_str());
	
	if (_tcscmp(retStr, TEXT("INI_ERROR_DEFAULT_VALUE")) == 0)
		return false;

	if (_tcscmp(retStr, TEXT("1")) == 0)
		_var = true;
	else if  (_tcscmp(retStr, TEXT("0")) == 0)
		_var = false;
	else
		return false;
	
	return true;
}

bool SettingsManager::IniReadString(LPTSTR _section, LPTSTR _key, std::basic_string<TCHAR> &_var)
{
	TCHAR retStr[64];
	
	::GetPrivateProfileString(_section, _key, TEXT("INI_ERROR_DEFAULT_VALUE"), retStr, sizeof(retStr), iniPath.c_str());
	
	if (_tcscmp(retStr, TEXT("INI_ERROR_DEFAULT_VALUE")) == 0)
		return false;

	_var = retStr;
	
	return true;
}
