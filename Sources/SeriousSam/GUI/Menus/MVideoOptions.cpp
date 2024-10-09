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
#include "MVideoOptions.h"

// [Cecil] Screen resolution lists and window modes
#include "ScreenResolutions.h"

extern void InitVideoOptionsButtons();
extern void UpdateVideoOptionsButtons(INDEX iSelected);


void CVideoOptionsMenu::Initialize_t(void)
{
  // [Cecil] Create array of available graphics APIs
  const INDEX ctAPIs = GAT_MAX;

  if (astrDisplayAPIRadioTexts == NULL) {
    astrDisplayAPIRadioTexts = new CTString[ctAPIs];

    for (INDEX iAPI = 0; iAPI < ctAPIs; iAPI++) {
      const CTString &strAPI = CGfxLibrary::GetApiName((GfxAPIType)iAPI);
      astrDisplayAPIRadioTexts[iAPI] = TRANSV(strAPI);
    }
  }

  // intialize video options menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.mg_strText = TRANS("[VIDEO]");
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  TRIGGER_MG(gm_mgDisplayAPITrigger, 6.0f,
    gm_mgApply, gm_mgDisplayAdaptersTrigger, TRANS("VIDEO SYSTEM"), astrDisplayAPIRadioTexts);
  gm_mgDisplayAPITrigger.mg_strTip = TRANS("Choose video system to use");
  gm_mgDisplayAPITrigger.mg_ctTexts = ctAPIs; // [Cecil] Amount of available APIs
  gm_mgDisplayAPITrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgDisplayAdaptersTrigger, 7.0f,
    gm_mgDisplayAPITrigger, gm_mgDisplayPrefsTrigger, TRANS("DISPLAY ADAPTER"), astrNoYes);
  gm_mgDisplayAdaptersTrigger.mg_strTip = TRANS("Choose display adapter to use");
  gm_mgDisplayAdaptersTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgDisplayPrefsTrigger, 8.0f,
    gm_mgDisplayAdaptersTrigger, gm_mgAspectRatiosTrigger, TRANS("PREFERENCES"), astrDisplayPrefsRadioTexts);
  gm_mgDisplayPrefsTrigger.mg_strTip = TRANS("Balance between speed and rendering quality");
  gm_mgDisplayPrefsTrigger.mg_iCenterI = -1;

  // [Cecil] Aspect ratio list
  TRIGGER_MG(gm_mgAspectRatiosTrigger, 9.0f,
    gm_mgDisplayPrefsTrigger, gm_mgResolutionsTrigger, TRANS("ASPECT RATIO"), _astrAspectRatios);
  gm_mgAspectRatiosTrigger.mg_strTip = TRANS("Select video mode aspect ratio");
  gm_mgAspectRatiosTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgResolutionsTrigger, 10.0f,
    gm_mgAspectRatiosTrigger, gm_mgWindowModeTrigger, TRANS("RESOLUTION"), astrNoYes);
  gm_mgResolutionsTrigger.mg_strTip = TRANS("Select video mode resolution");
  gm_mgResolutionsTrigger.mg_iCenterI = -1;

  // [Cecil] Changed fullscreen switch to window modes
  TRIGGER_MG(gm_mgWindowModeTrigger, 11.0f,
    gm_mgResolutionsTrigger, gm_mgBitsPerPixelTrigger, TRANS("WINDOW MODE"), _astrWindowModes);
  gm_mgWindowModeTrigger.mg_strTip = TRANS("Run game in a window or in full screen");
  gm_mgWindowModeTrigger.mg_iCenterI = -1;

  TRIGGER_MG(gm_mgBitsPerPixelTrigger, 12.0f,
    gm_mgWindowModeTrigger, gm_mgVideoRendering, TRANS("BITS PER PIXEL"), astrBitsPerPixelRadioTexts);
  gm_mgBitsPerPixelTrigger.mg_strTip = TRANS("Select number of colors used for display");
  gm_mgBitsPerPixelTrigger.mg_iCenterI = -1;

  gm_mgDisplayPrefsTrigger.mg_pOnTriggerChange = NULL;
  gm_mgDisplayAPITrigger.mg_pOnTriggerChange = NULL;
  gm_mgDisplayAdaptersTrigger.mg_pOnTriggerChange = NULL;
  gm_mgWindowModeTrigger.mg_pOnTriggerChange = NULL; // [Cecil]
  gm_mgAspectRatiosTrigger.mg_pOnTriggerChange = NULL; // [Cecil]
  gm_mgResolutionsTrigger.mg_pOnTriggerChange = NULL;
  gm_mgBitsPerPixelTrigger.mg_pOnTriggerChange = NULL;

  gm_mgVideoRendering.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgVideoRendering.mg_boxOnScreen = BoxMediumLeft(13.0f);
  gm_mgVideoRendering.mg_pmgUp = &gm_mgBitsPerPixelTrigger;
  gm_mgVideoRendering.mg_pmgDown = &gm_mgApply;
  gm_mgVideoRendering.mg_strText = TRANS("RENDERING OPTIONS");
  gm_mgVideoRendering.mg_strTip = TRANS("Adjust rendering settings");
  gm_lhGadgets.AddTail(gm_mgVideoRendering.mg_lnNode);
  gm_mgVideoRendering.mg_pActivatedFunction = NULL;
  gm_mgVideoRendering.mg_iCenterI = -1;

  gm_mgApply.mg_bfsFontSize = BFS_LARGE;
  gm_mgApply.mg_boxOnScreen = BoxBigLeft(8.5f);
  gm_mgApply.mg_pmgUp = &gm_mgVideoRendering;
  gm_mgApply.mg_pmgDown = &gm_mgBack;
  gm_mgApply.mg_strText = TRANS("APPLY");
  gm_mgApply.mg_strTip = TRANS("Apply options");
  gm_lhGadgets.AddTail(gm_mgApply.mg_lnNode);
  gm_mgApply.mg_pActivatedFunction = NULL;
  gm_mgApply.mg_iCenterI = -1;

  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_pmgUp = &gm_mgApply;
  gm_mgBack.mg_pmgDown = &gm_mgDisplayAPITrigger;
  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_strTip = TRANS("Return to previous menu");
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_pActivatedFunction = NULL;
  gm_mgBack.mg_iCenterI = -1;
}

void CVideoOptionsMenu::StartMenu(void)
{
  InitVideoOptionsButtons();

  CGameMenu::StartMenu();

  UpdateVideoOptionsButtons(-1);
}