/* Copyright (c) 2002-2012 Croteam Ltd.
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "StdH.h"
#include <Engine/CurrentVersion.h>
#include "MenuPrinting.h"
#include "Credits.h"
#include "MCredits.h"

void CCreditsMenu::Initialize_t(void)
{
  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.63f);
  gm_mgBack.mg_strTip = "";
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_pmgUp = &gm_mgBack;
  gm_mgBack.mg_pmgDown = &gm_mgBack;
  gm_mgBack.mg_iCenterI = -1;
  gm_mgBack.mg_pActivatedFunction = NULL;
}

void CCreditsMenu::StartMenu(void)
{
  Credits_On(3);

  try {
    soMusic.Play_t(CTFILENAME("Music\\Credits.mp3"), SOF_NONGAME | SOF_MUSIC | SOF_LOOP);
  }
  catch (char* strError) {
    CPrintF("%s\n", strError);
  }

  CGameMenu::StartMenu();
}

void CCreditsMenu::EndMenu(void)
{
  Credits_Off();
  CGameMenu::EndMenu();
}
