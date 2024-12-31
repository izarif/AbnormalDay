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
  gm_mgTitle.mg_boxOnScreen = BoxTitle(2.5f);
  gm_mgTitle.mg_strText = TRANS("# CONTROLS");
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  gm_mgButtons.mg_strText = TRANS("CUSTOMIZE BUTTONS");
  gm_mgButtons.mg_boxOnScreen = BoxMediumLeft(8.7f);
  gm_mgButtons.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgButtons.mg_iCenterI = -1;
  gm_lhGadgets.AddTail(gm_mgButtons.mg_lnNode);
  gm_mgButtons.mg_pmgUp = &gm_mgBack;
  gm_mgButtons.mg_pmgDown = &gm_mgAdvanced;
  gm_mgButtons.mg_pActivatedFunction = NULL;
  gm_mgButtons.mg_strTip = TRANS("Customize buttons");

  gm_mgAdvanced.mg_strText = TRANS("ADVANCED JOYSTICK SETUP");
  gm_mgAdvanced.mg_iCenterI = -1;
  gm_mgAdvanced.mg_boxOnScreen = BoxMediumLeft(9.7f);
  gm_mgAdvanced.mg_bfsFontSize = BFS_MEDIUM;
  gm_lhGadgets.AddTail(gm_mgAdvanced.mg_lnNode);
  gm_mgAdvanced.mg_pmgUp = &gm_mgButtons;
  gm_mgAdvanced.mg_pmgDown = &gm_mgSensitivity;
  gm_mgAdvanced.mg_pActivatedFunction = NULL;
  gm_mgAdvanced.mg_strTip = TRANS("Adjust settings for joystick axis");

  gm_mgSensitivity.mg_boxOnScreen = BoxMediumLeft(10.7f);
  gm_mgSensitivity.mg_strText = TRANS("SENSITIVITY");
  gm_mgSensitivity.mg_pmgUp = &gm_mgAdvanced;
  gm_mgSensitivity.mg_pmgDown = &gm_mgInvertTrigger;
  gm_mgSensitivity.mg_strTip = TRANS("Sensitivity for all axis");
  gm_lhGadgets.AddTail(gm_mgSensitivity.mg_lnNode);
  gm_mgSensitivity.mg_iCenterI = -1;

  gm_mgInvertTrigger.mg_pmgUp = &gm_mgSensitivity;
  gm_mgInvertTrigger.mg_pmgDown = &gm_mgSmoothTrigger;
  gm_mgInvertTrigger.mg_boxOnScreen = BoxMediumLeft(11.7f);
  gm_lhGadgets.AddTail(gm_mgInvertTrigger.mg_lnNode);
  gm_mgInvertTrigger.mg_astrTexts = astrNoYes; 
  gm_mgInvertTrigger.mg_ctTexts = sizeof(astrNoYes) / sizeof(astrNoYes[0]);
  gm_mgInvertTrigger.mg_iSelected = 0; gm_mgInvertTrigger.mg_strLabel = Translate("ETRS" "INVERT LOOK", 4);
  gm_mgInvertTrigger.mg_strValue = astrNoYes[0];
  gm_mgInvertTrigger.mg_strTip = TRANS("Invert up/down looking");
  gm_mgInvertTrigger.mg_iCenterI = -1;

  gm_mgSmoothTrigger.mg_pmgUp = &gm_mgInvertTrigger;
  gm_mgSmoothTrigger.mg_pmgDown = &gm_mgAccelTrigger;
  gm_mgSmoothTrigger.mg_boxOnScreen = BoxMediumLeft(12.7f);
  gm_lhGadgets.AddTail(gm_mgSmoothTrigger.mg_lnNode);
  gm_mgSmoothTrigger.mg_astrTexts = astrNoYes;
  gm_mgSmoothTrigger.mg_ctTexts = sizeof(astrNoYes) / sizeof(astrNoYes[0]);
  gm_mgSmoothTrigger.mg_iSelected = 0;
  gm_mgSmoothTrigger.mg_strLabel = Translate("ETRS" "SMOOTH AXIS", 4);
  gm_mgSmoothTrigger.mg_strValue = astrNoYes[0];
  gm_mgSmoothTrigger.mg_strTip = TRANS("Smooth mouse/joystick movements");
  gm_mgSmoothTrigger.mg_iCenterI = -1;

  gm_mgAccelTrigger.mg_pmgUp = &gm_mgSmoothTrigger;
  gm_mgAccelTrigger.mg_pmgDown = &gm_mgPredefined;
  gm_mgAccelTrigger.mg_boxOnScreen = BoxMediumLeft(13.7f);
  gm_lhGadgets.AddTail(gm_mgAccelTrigger.mg_lnNode);
  gm_mgAccelTrigger.mg_astrTexts = astrNoYes;
  gm_mgAccelTrigger.mg_ctTexts = sizeof(astrNoYes) / sizeof(astrNoYes[0]);
  gm_mgAccelTrigger.mg_iSelected = 0;
  gm_mgAccelTrigger.mg_strLabel = Translate("ETRS" "MOUSE ACCELERATION", 4);
  gm_mgAccelTrigger.mg_strValue = astrNoYes[0];
  gm_mgAccelTrigger.mg_strTip = TRANS("Allow mouse acceleration");
  gm_mgAccelTrigger.mg_iCenterI = -1;

  gm_mgPredefined.mg_strText = TRANS("LOAD PREDEFINED SETTINGS");
  gm_mgPredefined.mg_iCenterI = -1;
  gm_mgPredefined.mg_boxOnScreen = BoxMediumLeft(14.7f);
  gm_mgPredefined.mg_bfsFontSize = BFS_MEDIUM;
  gm_lhGadgets.AddTail(gm_mgPredefined.mg_lnNode);
  gm_mgPredefined.mg_pmgUp = &gm_mgAccelTrigger;
  gm_mgPredefined.mg_pmgDown = &gm_mgBack;
  gm_mgPredefined.mg_pActivatedFunction = NULL;
  gm_mgPredefined.mg_strTip = TRANS("Load one of predefined control settings");

  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_strTip = TRANS("Return to main menu");
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_pmgUp = &gm_mgAccelTrigger;
  gm_mgBack.mg_pmgDown = &gm_mgButtons;
  gm_mgBack.mg_pActivatedFunction = NULL;
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

  gm_mgInvertTrigger.ApplyCurrentSelection();
  gm_mgSmoothTrigger.ApplyCurrentSelection();
  gm_mgAccelTrigger.ApplyCurrentSelection();
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

  if (INDEX(ctrls.ctrl_fSensitivity) != INDEX(fSensitivity)) {
    ctrls.ctrl_fSensitivity = fSensitivity;
  }
  ctrls.ctrl_bInvertLook = bInvert;
  ctrls.ctrl_bSmoothAxes = bSmooth;
  _pShell->SetINDEX("inp_bAllowMouseAcceleration", bAccel);
  ctrls.CalculateInfluencesForAllAxis();
}