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
#include "MInGame.h"


void CInGameMenu::Initialize_t(void)
{
  // intialize main menu
  gm_mgTitle.mg_strText = TRANS("[GAME]");
  gm_mgTitle.mg_boxOnScreen = BoxTitle(0.0f);
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  gm_mgResumeGame.mg_strText = TRANS("RESUME GAME");
  gm_mgResumeGame.mg_bfsFontSize = BFS_LARGE;
  gm_mgResumeGame.mg_boxOnScreen = BoxBigLeft(5.5f);
  gm_mgResumeGame.mg_strTip = TRANS("Resume current game");
  gm_lhGadgets.AddTail(gm_mgResumeGame.mg_lnNode);
  gm_mgResumeGame.mg_pmgUp = &gm_mgQuit;
  gm_mgResumeGame.mg_pmgDown = &gm_mgLoadGame;
  gm_mgResumeGame.mg_pActivatedFunction = NULL;
  gm_mgResumeGame.mg_iCenterI = -1;

  gm_mgLoadGame.mg_strText = TRANS("LOAD GAME");
  gm_mgLoadGame.mg_bfsFontSize = BFS_LARGE;
  gm_mgLoadGame.mg_boxOnScreen = BoxBigLeft(6.5f);
  gm_mgLoadGame.mg_strTip = TRANS("Load a saved game");
  gm_lhGadgets.AddTail(gm_mgLoadGame.mg_lnNode);
  gm_mgLoadGame.mg_pmgUp = &gm_mgResumeGame;
  gm_mgLoadGame.mg_pmgDown = &gm_mgSaveGame;
  gm_mgLoadGame.mg_pActivatedFunction = NULL;
  gm_mgLoadGame.mg_iCenterI = -1;

  gm_mgSaveGame.mg_strText = TRANS("SAVE GAME");
  gm_mgSaveGame.mg_bfsFontSize = BFS_LARGE;
  gm_mgSaveGame.mg_boxOnScreen = BoxBigLeft(7.5f);
  gm_mgSaveGame.mg_strTip = TRANS("Save current game");
  gm_lhGadgets.AddTail(gm_mgSaveGame.mg_lnNode);
  gm_mgSaveGame.mg_pmgUp = &gm_mgLoadGame;
  gm_mgSaveGame.mg_pmgDown = &gm_mgOptions;
  gm_mgSaveGame.mg_pActivatedFunction = NULL;
  gm_mgSaveGame.mg_iCenterI = -1;

  gm_mgOptions.mg_strText = TRANS("OPTIONS");
  gm_mgOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgOptions.mg_boxOnScreen = BoxBigLeft(8.5f);
  gm_mgOptions.mg_strTip = TRANS("Adjust video, audio and input options");
  gm_lhGadgets.AddTail(gm_mgOptions.mg_lnNode);
  gm_mgOptions.mg_pmgUp = &gm_mgSaveGame;
  gm_mgOptions.mg_pmgDown = &gm_mgQuit;
  gm_mgOptions.mg_pActivatedFunction = NULL;
  gm_mgOptions.mg_iCenterI = -1;

  gm_mgQuit.mg_strText = TRANS("QUIT GAME");
  gm_mgQuit.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuit.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgQuit.mg_strTip = TRANS("Quit to main menu");
  gm_lhGadgets.AddTail(gm_mgQuit.mg_lnNode);
  gm_mgQuit.mg_pmgUp = &gm_mgOptions;
  gm_mgQuit.mg_pmgDown = &gm_mgResumeGame;
  gm_mgQuit.mg_pActivatedFunction = NULL;
  gm_mgQuit.mg_iCenterI = -1;
}
