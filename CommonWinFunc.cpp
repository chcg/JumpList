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

#include "CommonWinFunc.h"

BOOL CenterWindow(HWND hChildWnd, HWND hParentWnd, BOOL bRepaint)
{
  RECT rectParent;
  RECT rect;
  INT  height, width;
  INT  x, y;

  ::GetWindowRect(hParentWnd, &rectParent);
  ::GetWindowRect(hChildWnd, &rect);
  width = rect.right - rect.left;
  height = rect.bottom - rect.top;
  x = ((rectParent.right - rectParent.left) - width) / 2;
  x += rectParent.left;
  y = ((rectParent.bottom - rectParent.top) - height) / 2;
  y += rectParent.top;
  return ::MoveWindow(hChildWnd, x, y, width, height, bRepaint);
}