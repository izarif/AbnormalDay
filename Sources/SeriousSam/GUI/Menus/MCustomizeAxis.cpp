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
#include "MCustomizeAxis.h"


void CCustomizeAxisMenu::Initialize_t(void)
{
  // intialize axis menu
  gm_mgTitle.mg_strText = TRANS("[CUSTOMIZE AXIS]");
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  TRIGGER_MG(gm_mgActionTrigger, 8.5f, gm_mgSmoothTrigger, gm_mgMountedTrigger, TRANS("ACTION"), astrNoYes);
  gm_mgActionTrigger.mg_strTip = TRANS("Choose action to customize");
  gm_mgActionTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgMountedTrigger, 9.5f, gm_mgActionTrigger, gm_mgSensitivity, TRANS("MOUNTED TO"), astrNoYes);
  gm_mgMountedTrigger.mg_strTip = TRANS("Choose controller axis that will perform the action");
  gm_mgMountedTrigger.mg_iCenterI = -1;

  gm_mgActionTrigger.mg_astrTexts = new CTString[AXIS_ACTIONS_CT];
  gm_mgActionTrigger.mg_ctTexts = AXIS_ACTIONS_CT;

  gm_mgActionTrigger.mg_pPreTriggerChange = NULL;
  gm_mgActionTrigger.mg_pOnTriggerChange = NULL;

  // for all available axis type controlers
  for (INDEX iControler = 0; iControler<AXIS_ACTIONS_CT; iControler++) {
    gm_mgActionTrigger.mg_astrTexts[iControler] = TranslateConst(CTString(_pGame->gm_astrAxisNames[iControler]), 0);
  }
  gm_mgActionTrigger.mg_iSelected = 3;

  INDEX ctAxis = _pInput->GetAvailableAxisCount();
  gm_mgMountedTrigger.mg_astrTexts = new CTString[ctAxis];
  gm_mgMountedTrigger.mg_ctTexts = ctAxis;
  // for all axis actions that can be mounted
  for (INDEX iAxis = 0; iAxis<ctAxis; iAxis++) {
    gm_mgMountedTrigger.mg_astrTexts[iAxis] = _pInput->GetAxisTransName(iAxis);
  }

  gm_mgSensitivity.mg_boxOnScreen = BoxMediumLeft(10.5f);
  gm_mgSensitivity.mg_strText = TRANS("SENSITIVITY");
  gm_mgSensitivity.mg_pmgUp = &gm_mgMountedTrigger;
  gm_mgSensitivity.mg_pmgDown = &gm_mgDeadzone;
  gm_lhGadgets.AddTail(gm_mgSensitivity.mg_lnNode);
  gm_mgSensitivity.mg_strTip = TRANS("Set sensitivity for this axis");
  gm_mgSensitivity.mg_iCenterI = -1;

  gm_mgDeadzone.mg_boxOnScreen = BoxMediumLeft(11.5f);
  gm_mgDeadzone.mg_strText = TRANS("DEAD ZONE");
  gm_mgDeadzone.mg_pmgUp = &gm_mgSensitivity;
  gm_mgDeadzone.mg_pmgDown = &gm_mgInvertTrigger;
  gm_lhGadgets.AddTail(gm_mgDeadzone.mg_lnNode);
  gm_mgDeadzone.mg_strTip = TRANS("Set dead zone for this axis");
  gm_mgDeadzone.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgInvertTrigger, 12.5f, gm_mgDeadzone, gm_mgRelativeTrigger, TRANS("INVERTED"), astrNoYes);
  gm_mgInvertTrigger.mg_strTip = TRANS("Choose whether to invert this axis or not");
  gm_mgInvertTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgRelativeTrigger, 13.5f, gm_mgInvertTrigger, gm_mgSmoothTrigger, TRANS("RELATIVE"), astrNoYes);
  gm_mgRelativeTrigger.mg_strTip = TRANS("Select relative or absolute axis reading");
  gm_mgRelativeTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgSmoothTrigger, 14.5f, gm_mgRelativeTrigger, gm_mgBack, TRANS("SMOOTH"), astrNoYes);
  gm_mgSmoothTrigger.mg_strTip = TRANS("Filter readings on this axis");
  gm_mgSmoothTrigger.mg_iCenterI = -1;

  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_pmgUp = &gm_mgSmoothTrigger;
  gm_mgBack.mg_pmgDown = &gm_mgActionTrigger;
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_strTip = TRANS("Return to previous menu");
  gm_mgBack.mg_iCenterI = -1;
}

CCustomizeAxisMenu::~CCustomizeAxisMenu(void)
{
  delete[] gm_mgActionTrigger.mg_astrTexts;
  delete[] gm_mgMountedTrigger.mg_astrTexts;
}

void CCustomizeAxisMenu::ObtainActionSettings(void)
{
  ControlsMenuOn();
  CControls &ctrls = _pGame->gm_ctrlControlsExtra;
  INDEX iSelectedAction = gm_mgActionTrigger.mg_iSelected;
  INDEX iMountedAxis = ctrls.ctrl_aaAxisActions[iSelectedAction].aa_iAxisAction;

  gm_mgMountedTrigger.mg_iSelected = iMountedAxis;

  gm_mgSensitivity.mg_iMinPos = 0;
  gm_mgSensitivity.mg_iMaxPos = 50;
  gm_mgSensitivity.mg_iCurPos = ctrls.ctrl_aaAxisActions[iSelectedAction].aa_fSensitivity / 2;
  gm_mgSensitivity.ApplyCurrentPosition();

  gm_mgDeadzone.mg_iMinPos = 0;
  gm_mgDeadzone.mg_iMaxPos = 50;
  gm_mgDeadzone.mg_iCurPos = ctrls.ctrl_aaAxisActions[iSelectedAction].aa_fDeadZone / 2;
  gm_mgDeadzone.ApplyCurrentPosition();

  gm_mgInvertTrigger.mg_iSelected = ctrls.ctrl_aaAxisActions[iSelectedAction].aa_bInvert ? 1 : 0;
  gm_mgRelativeTrigger.mg_iSelected = ctrls.ctrl_aaAxisActions[iSelectedAction].aa_bRelativeControler ? 1 : 0;
  gm_mgSmoothTrigger.mg_iSelected = ctrls.ctrl_aaAxisActions[iSelectedAction].aa_bSmooth ? 1 : 0;

  gm_mgActionTrigger.ApplyCurrentSelection();
  gm_mgMountedTrigger.ApplyCurrentSelection();
  gm_mgInvertTrigger.ApplyCurrentSelection();
  gm_mgRelativeTrigger.ApplyCurrentSelection();
  gm_mgSmoothTrigger.ApplyCurrentSelection();
}

void CCustomizeAxisMenu::ApplyActionSettings(void)
{
  CControls &ctrls = _pGame->gm_ctrlControlsExtra;
  INDEX iSelectedAction = gm_mgActionTrigger.mg_iSelected;
  INDEX iMountedAxis = gm_mgMountedTrigger.mg_iSelected;
  FLOAT fSensitivity =
    FLOAT(gm_mgSensitivity.mg_iCurPos - gm_mgSensitivity.mg_iMinPos) /
    FLOAT(gm_mgSensitivity.mg_iMaxPos - gm_mgSensitivity.mg_iMinPos)*100.0f;
  FLOAT fDeadZone =
    FLOAT(gm_mgDeadzone.mg_iCurPos - gm_mgDeadzone.mg_iMinPos) /
    FLOAT(gm_mgDeadzone.mg_iMaxPos - gm_mgDeadzone.mg_iMinPos)*100.0f;

  BOOL bInvert = gm_mgInvertTrigger.mg_iSelected != 0;
  BOOL bRelative = gm_mgRelativeTrigger.mg_iSelected != 0;
  BOOL bSmooth = gm_mgSmoothTrigger.mg_iSelected != 0;

  ctrls.ctrl_aaAxisActions[iSelectedAction].aa_iAxisAction = iMountedAxis;
  if (INDEX(ctrls.ctrl_aaAxisActions[iSelectedAction].aa_fSensitivity) != INDEX(fSensitivity)) {
    ctrls.ctrl_aaAxisActions[iSelectedAction].aa_fSensitivity = fSensitivity;
  }
  if (INDEX(ctrls.ctrl_aaAxisActions[iSelectedAction].aa_fDeadZone) != INDEX(fDeadZone)) {
    ctrls.ctrl_aaAxisActions[iSelectedAction].aa_fDeadZone = fDeadZone;
  }
  ctrls.ctrl_aaAxisActions[iSelectedAction].aa_bInvert = bInvert;
  ctrls.ctrl_aaAxisActions[iSelectedAction].aa_bRelativeControler = bRelative;
  ctrls.ctrl_aaAxisActions[iSelectedAction].aa_bSmooth = bSmooth;
  ctrls.CalculateInfluencesForAllAxis();

  ControlsMenuOff();
}

void CCustomizeAxisMenu::StartMenu(void)
{
  ObtainActionSettings();

  CGameMenu::StartMenu();
}

void CCustomizeAxisMenu::EndMenu(void)
{
  ApplyActionSettings();
  CGameMenu::EndMenu();
}