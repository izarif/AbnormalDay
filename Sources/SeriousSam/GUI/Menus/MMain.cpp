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
#include "MenuStuff.h"
#include "MMain.h"


void CMainMenu::Initialize_t(void)
{
  // intialize main menu
  /*
  gm_mgTitle.mg_strText = "SERIOUS SAM - BETA";  // nothing to see here, kazuya
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_lhGadgets.AddTail( gm_mgTitle.mg_lnNode);
  */

  extern CTString sam_strVersion;
  gm_mgVersionLabel.mg_strText = sam_strVersion;
  gm_mgVersionLabel.mg_boxOnScreen = BoxVersion();
  gm_mgVersionLabel.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgVersionLabel.mg_iCenterI = -1;
  gm_mgVersionLabel.mg_bEnabled = FALSE;
  gm_mgVersionLabel.mg_bLabel = TRUE;
  gm_lhGadgets.AddTail(gm_mgVersionLabel.mg_lnNode);

  gm_mgNewGame.mg_strText = TRANS("NEW GAME");
  gm_mgNewGame.mg_bfsFontSize = BFS_LARGE;
  gm_mgNewGame.mg_boxOnScreen = BoxBigLeft(5.5f);
  gm_mgNewGame.mg_strTip = TRANS("Start a new game");
  gm_lhGadgets.AddTail(gm_mgNewGame.mg_lnNode);
  gm_mgNewGame.mg_pmgUp = &gm_mgQuit;
  gm_mgNewGame.mg_pmgDown = &gm_mgLoadGame;
  gm_mgNewGame.mg_pActivatedFunction = NULL;
  gm_mgNewGame.mg_iCenterI = -1;

  gm_mgLoadGame.mg_strText = TRANS("LOAD GAME");
  gm_mgLoadGame.mg_bfsFontSize = BFS_LARGE;
  gm_mgLoadGame.mg_boxOnScreen = BoxBigLeft(6.5f);
  gm_mgLoadGame.mg_strTip = TRANS("Load a saved game");
  gm_lhGadgets.AddTail(gm_mgLoadGame.mg_lnNode);
  gm_mgLoadGame.mg_pmgUp = &gm_mgNewGame;
  gm_mgLoadGame.mg_pmgDown = &gm_mgOptions;
  gm_mgLoadGame.mg_pActivatedFunction = NULL;
  gm_mgLoadGame.mg_iCenterI = -1;

  gm_mgOptions.mg_strText = TRANS("OPTIONS");
  gm_mgOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgOptions.mg_boxOnScreen = BoxBigLeft(7.5f);
  gm_mgOptions.mg_strTip = TRANS("Adjust video, audio and input options");
  gm_lhGadgets.AddTail(gm_mgOptions.mg_lnNode);
  gm_mgOptions.mg_pmgUp = &gm_mgLoadGame;
  gm_mgOptions.mg_pmgDown = &gm_mgCredits;
  gm_mgOptions.mg_pActivatedFunction = NULL;
  gm_mgOptions.mg_iCenterI = -1;

  gm_mgCredits.mg_strText = TRANS("CREDITS");
  gm_mgCredits.mg_bfsFontSize = BFS_LARGE;
  gm_mgCredits.mg_boxOnScreen = BoxBigLeft(8.5f);
  gm_mgCredits.mg_strTip = TRANS("Show the credits");
  gm_lhGadgets.AddTail(gm_mgCredits.mg_lnNode);
  gm_mgCredits.mg_pmgUp = &gm_mgOptions;
  gm_mgCredits.mg_pmgDown = &gm_mgQuit;
  gm_mgCredits.mg_pActivatedFunction = NULL;
  gm_mgCredits.mg_iCenterI = -1;

  gm_mgQuit.mg_strText = TRANS("QUIT GAME");
  gm_mgQuit.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuit.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgQuit.mg_strTip = TRANS("Quit the game");
  gm_lhGadgets.AddTail(gm_mgQuit.mg_lnNode);
  gm_mgQuit.mg_pmgUp = &gm_mgCredits;
  gm_mgQuit.mg_pmgDown = &gm_mgNewGame;
  gm_mgQuit.mg_pActivatedFunction = NULL;
  gm_mgQuit.mg_iCenterI = -1;
}
