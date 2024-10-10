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
#include "MControls.h"

extern CTFileName _fnmControlsToCustomize;


void CControlsMenu::Initialize_t(void)
{
  // intialize player and controls menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.mg_strText = TRANS("[CONTROLS]");
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  gm_mgButtons.mg_strText = TRANS("CUSTOMIZE BUTTONS");
  gm_mgButtons.mg_boxOnScreen = BoxMediumLeft(8.5f);
  gm_mgButtons.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgButtons.mg_iCenterI = -1;
  gm_lhGadgets.AddTail(gm_mgButtons.mg_lnNode);
  gm_mgButtons.mg_pmgUp = &gm_mgBack;
  gm_mgButtons.mg_pmgDown = &gm_mgAdvanced;
  gm_mgButtons.mg_pActivatedFunction = NULL;
  gm_mgButtons.mg_strTip = TRANS("Customize buttons");

  gm_mgAdvanced.mg_strText = TRANS("ADVANCED JOYSTICK SETUP");
  gm_mgAdvanced.mg_iCenterI = -1;
  gm_mgAdvanced.mg_boxOnScreen = BoxMediumLeft(9.5f);
  gm_mgAdvanced.mg_bfsFontSize = BFS_MEDIUM;
  gm_lhGadgets.AddTail(gm_mgAdvanced.mg_lnNode);
  gm_mgAdvanced.mg_pmgUp = &gm_mgButtons;
  gm_mgAdvanced.mg_pmgDown = &gm_mgSensitivity;
  gm_mgAdvanced.mg_pActivatedFunction = NULL;
  gm_mgAdvanced.mg_strTip = TRANS("Adjust advanced settings for joystick axis");

  gm_mgSensitivity.mg_boxOnScreen = BoxMediumLeft(10.5f);
  gm_mgSensitivity.mg_strText = TRANS("SENSITIVITY");
  gm_mgSensitivity.mg_pmgUp = &gm_mgAdvanced;
  gm_mgSensitivity.mg_pmgDown = &gm_mgInvertTrigger;
  gm_mgSensitivity.mg_strTip = TRANS("Sensitivity for all axis");
  gm_lhGadgets.AddTail(gm_mgSensitivity.mg_lnNode);
  gm_mgSensitivity.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgInvertTrigger, 11.5f, gm_mgSensitivity, gm_mgSmoothTrigger,
    TRANS("INVERT LOOK"), astrNoYes);
  gm_mgInvertTrigger.mg_strTip = TRANS("Invert up/down looking");
  gm_mgInvertTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgSmoothTrigger, 12.5f, gm_mgInvertTrigger, gm_mgAccelTrigger,
    TRANS("SMOOTH AXIS"), astrNoYes);
  gm_mgSmoothTrigger.mg_strTip = TRANS("Smooth mouse/joystick movements");
  gm_mgSmoothTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgAccelTrigger, 13.5f, gm_mgSmoothTrigger, gm_mgIFeelTrigger,
    TRANS("MOUSE ACCELERATION"), astrNoYes);
  gm_mgAccelTrigger.mg_strTip = TRANS("Allow mouse acceleration");
  gm_mgAccelTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgIFeelTrigger, 14.5f, gm_mgAccelTrigger, gm_mgBack,
    TRANS("ENABLE IFEEL"), astrNoYes);
  gm_mgIFeelTrigger.mg_strTip = TRANS("Enable support for iFeel tactile feedback mouse");
  gm_mgIFeelTrigger.mg_iCenterI = -1;

  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_pmgUp = &gm_mgIFeelTrigger;
  gm_mgBack.mg_pmgDown = &gm_mgButtons;
  gm_mgBack.mg_strTip = TRANS("Return to previous menu");
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_iCenterI = -1;
}

void CControlsMenu::StartMenu(void)
{
  gm_pmgSelectedByDefault = &gm_mgButtons;
  INDEX iPlayer = _pGame->gm_iSinglePlayer;
  if (_iLocalPlayer >= 0 && _iLocalPlayer<4) {
    iPlayer = _pGame->gm_aiMenuLocalPlayers[_iLocalPlayer];
  }
  _fnmControlsToCustomize.PrintF("UserData\\Controls\\Controls%d.ctl", iPlayer); // [Cecil] From user data

  ControlsMenuOn();
  ObtainActionSettings();
  CGameMenu::StartMenu();
}

void CControlsMenu::EndMenu(void)
{
  ApplyActionSettings();

  ControlsMenuOff();

  CGameMenu::EndMenu();
}

void CControlsMenu::ObtainActionSettings(void)
{
  CControls &ctrls = _pGame->gm_ctrlControlsExtra;

  gm_mgSensitivity.mg_iMinPos = 0;
  gm_mgSensitivity.mg_iMaxPos = 50;
  gm_mgSensitivity.mg_iCurPos = ctrls.ctrl_fSensitivity / 2;
  gm_mgSensitivity.ApplyCurrentPosition();

  gm_mgInvertTrigger.mg_iSelected = ctrls.ctrl_bInvertLook ? 1 : 0;
  gm_mgSmoothTrigger.mg_iSelected = ctrls.ctrl_bSmoothAxes ? 1 : 0;
  gm_mgAccelTrigger.mg_iSelected = _pShell->GetINDEX("inp_bAllowMouseAcceleration") ? 1 : 0;
  gm_mgIFeelTrigger.mg_bEnabled = _pShell->GetINDEX("sys_bIFeelEnabled") ? 1 : 0;
  gm_mgIFeelTrigger.mg_iSelected = _pShell->GetFLOAT("inp_fIFeelGain")>0 ? 1 : 0;

  gm_mgInvertTrigger.ApplyCurrentSelection();
  gm_mgSmoothTrigger.ApplyCurrentSelection();
  gm_mgAccelTrigger.ApplyCurrentSelection();
  gm_mgIFeelTrigger.ApplyCurrentSelection();
}

void CControlsMenu::ApplyActionSettings(void)
{
  CControls &ctrls = _pGame->gm_ctrlControlsExtra;

  FLOAT fSensitivity =
    FLOAT(gm_mgSensitivity.mg_iCurPos - gm_mgSensitivity.mg_iMinPos) /
    FLOAT(gm_mgSensitivity.mg_iMaxPos - gm_mgSensitivity.mg_iMinPos)*100.0f;

  BOOL bInvert = gm_mgInvertTrigger.mg_iSelected != 0;
  BOOL bSmooth = gm_mgSmoothTrigger.mg_iSelected != 0;
  BOOL bAccel = gm_mgAccelTrigger.mg_iSelected != 0;
  BOOL bIFeel = gm_mgIFeelTrigger.mg_iSelected != 0;

  if (INDEX(ctrls.ctrl_fSensitivity) != INDEX(fSensitivity)) {
    ctrls.ctrl_fSensitivity = fSensitivity;
  }
  ctrls.ctrl_bInvertLook = bInvert;
  ctrls.ctrl_bSmoothAxes = bSmooth;
  _pShell->SetINDEX("inp_bAllowMouseAcceleration", bAccel);
  _pShell->SetFLOAT("inp_fIFeelGain", bIFeel ? 1.0f : 0.0f);
  ctrls.CalculateInfluencesForAllAxis();
}