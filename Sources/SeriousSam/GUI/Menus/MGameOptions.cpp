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
#include "MGameOptions.h"

void CGameOptionsMenu::Initialize_t(void)
{
  gm_mgTitle.mg_strText = TRANS("[GAME OPTIONS]");
  gm_mgTitle.mg_boxOnScreen = BoxTitle(0.0f);
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  TRIGGER_MG(gm_mgAutoSave, 10.0f, gm_mgBack, gm_mgSharpTurning, TRANS("AUTO SAVE"), astrNoYes);
  gm_mgAutoSave.mg_strTip = TRANS("Auto save game at specific locations");
  gm_mgAutoSave.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgSharpTurning, 11.0f, gm_mgAutoSave, gm_mgBloodAndGore, TRANS("SHARP TURNING"), astrNoYes);
  gm_mgSharpTurning.mg_strTip = TRANS("Use prescanning to eliminate mouse lag");
  gm_mgSharpTurning.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgBloodAndGore, 12.0f,
    gm_mgSharpTurning, gm_mgGibs, TRANS("BLOOD AND GORE"), astrBloodAndGoreRadioTexts);
  gm_mgBloodAndGore.mg_strTip = TRANS("Appearance of blood and gore");
  gm_mgBloodAndGore.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgGibs, 13.0f,
    gm_mgBloodAndGore, gm_mgApply, TRANS("GIBS"), astrNoYes);
  gm_mgGibs.mg_strTip = TRANS("Gibbing of enemies");
  gm_mgGibs.mg_iCenterI = -1;

  gm_mgApply.mg_bfsFontSize = BFS_LARGE;
  gm_mgApply.mg_boxOnScreen = BoxBigLeft(8.5f);
  gm_mgApply.mg_strText = TRANS("APPLY");
  gm_mgApply.mg_strTip = TRANS("Apply options");
  gm_lhGadgets.AddTail(gm_mgApply.mg_lnNode);
  gm_mgApply.mg_pmgUp = &gm_mgGibs;
  gm_mgApply.mg_pmgDown = &gm_mgBack;
  gm_mgApply.mg_pActivatedFunction = NULL;
  gm_mgApply.mg_iCenterI = -1;

  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_strTip = TRANS("Return to previous menu");
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_pmgUp = &gm_mgApply;
  gm_mgBack.mg_pmgDown = &gm_mgAutoSave;
  gm_mgBack.mg_pActivatedFunction = NULL;
  gm_mgBack.mg_iCenterI = -1;
}
