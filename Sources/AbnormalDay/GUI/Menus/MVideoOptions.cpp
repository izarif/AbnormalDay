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
  gm_mgTitle.mg_boxOnScreen = BoxTitle(1.52f);
  gm_mgTitle.mg_strText = TRANS("# VIDEO");
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  gm_mgDisplayAPITrigger.mg_pmgUp = &gm_mgBack;
  gm_mgDisplayAPITrigger.mg_pmgDown = &gm_mgDisplayAdaptersTrigger;
  gm_mgDisplayAPITrigger.mg_boxOnScreen = BoxMediumLeft(6.1f);
  gm_lhGadgets.AddTail(gm_mgDisplayAPITrigger.mg_lnNode);
  gm_mgDisplayAPITrigger.mg_astrTexts = astrDisplayAPIRadioTexts;
  gm_mgDisplayAPITrigger.mg_iSelected = 0;
  gm_mgDisplayAPITrigger.mg_strLabel = Translate("ETRS" "GRAPHICS SYSTEM", 4);
  gm_mgDisplayAPITrigger.mg_strValue = astrDisplayAPIRadioTexts[0];
  gm_mgDisplayAPITrigger.mg_strTip = TRANS("Choose graphics system to be used");
  gm_mgDisplayAPITrigger.mg_ctTexts = ctAPIs; // [Cecil] Amount of available APIs
  gm_mgDisplayAPITrigger.mg_iCenterI = -1;

  gm_mgDisplayAdaptersTrigger.mg_pmgUp = &gm_mgDisplayAPITrigger;
  gm_mgDisplayAdaptersTrigger.mg_pmgDown = &gm_mgDisplayPrefsTrigger;
  gm_mgDisplayAdaptersTrigger.mg_boxOnScreen = BoxMediumLeft(7.1f);
  gm_lhGadgets.AddTail(gm_mgDisplayAdaptersTrigger.mg_lnNode);
  gm_mgDisplayAdaptersTrigger.mg_astrTexts = astrNoYes;
  gm_mgDisplayAdaptersTrigger.mg_ctTexts = sizeof(astrNoYes) / sizeof(astrNoYes[0]);
  gm_mgDisplayAdaptersTrigger.mg_iSelected = 0;
  gm_mgDisplayAdaptersTrigger.mg_strLabel = Translate("ETRS" "DISPLAY ADAPTER", 4);
  gm_mgDisplayAdaptersTrigger.mg_strValue = astrNoYes[0];
  gm_mgDisplayAdaptersTrigger.mg_strTip = TRANS("Choose display adapter to be used");
  gm_mgDisplayAdaptersTrigger.mg_iCenterI = -1;

  gm_mgDisplayPrefsTrigger.mg_pmgUp = &gm_mgDisplayAdaptersTrigger;
  gm_mgDisplayPrefsTrigger.mg_pmgDown = &gm_mgAspectRatiosTrigger;
  gm_mgDisplayPrefsTrigger.mg_boxOnScreen = BoxMediumLeft(8.1f);
  gm_lhGadgets.AddTail(gm_mgDisplayPrefsTrigger.mg_lnNode);
  gm_mgDisplayPrefsTrigger.mg_astrTexts = astrDisplayPrefsRadioTexts;
  gm_mgDisplayPrefsTrigger.mg_ctTexts = sizeof(astrDisplayPrefsRadioTexts) / sizeof(astrDisplayPrefsRadioTexts[0]);
  gm_mgDisplayPrefsTrigger.mg_iSelected = 0;
  gm_mgDisplayPrefsTrigger.mg_strLabel = Translate("ETRS" "PREFERENCES", 4);
  gm_mgDisplayPrefsTrigger.mg_strValue = astrDisplayPrefsRadioTexts[0];
  gm_mgDisplayPrefsTrigger.mg_strTip = TRANS("Balance between speed and rendering quality");
  gm_mgDisplayPrefsTrigger.mg_iCenterI = -1;

  // [Cecil] Aspect ratio list
  gm_mgAspectRatiosTrigger.mg_pmgUp = &gm_mgDisplayPrefsTrigger;
  gm_mgAspectRatiosTrigger.mg_pmgDown = &gm_mgResolutionsTrigger;
  gm_mgAspectRatiosTrigger.mg_boxOnScreen = BoxMediumLeft(9.1f);
  gm_lhGadgets.AddTail(gm_mgAspectRatiosTrigger.mg_lnNode);
  gm_mgAspectRatiosTrigger.mg_astrTexts = _astrAspectRatios;
  gm_mgAspectRatiosTrigger.mg_ctTexts = sizeof(_astrAspectRatios) / sizeof(_astrAspectRatios[0]);
  gm_mgAspectRatiosTrigger.mg_iSelected = 0; gm_mgAspectRatiosTrigger.mg_strLabel = Translate("ETRS" "ASPECT RATIO", 4);
  gm_mgAspectRatiosTrigger.mg_strValue = _astrAspectRatios[0];
  gm_mgAspectRatiosTrigger.mg_strTip = TRANS("Select video aspect ratio");
  gm_mgAspectRatiosTrigger.mg_iCenterI = -1;

  gm_mgResolutionsTrigger.mg_pmgUp = &gm_mgAspectRatiosTrigger;
  gm_mgResolutionsTrigger.mg_pmgDown = &gm_mgWindowModeTrigger;
  gm_mgResolutionsTrigger.mg_boxOnScreen = BoxMediumLeft(10.1f);
  gm_lhGadgets.AddTail(gm_mgResolutionsTrigger.mg_lnNode);
  gm_mgResolutionsTrigger.mg_astrTexts = astrNoYes;
  gm_mgResolutionsTrigger.mg_ctTexts = sizeof(astrNoYes) / sizeof(astrNoYes[0]);
  gm_mgResolutionsTrigger.mg_iSelected = 0;
  gm_mgResolutionsTrigger.mg_strLabel = Translate("ETRS" "RESOLUTION", 4);
  gm_mgResolutionsTrigger.mg_strValue = astrNoYes[0];
  gm_mgResolutionsTrigger.mg_strTip = TRANS("Select video resolution");
  gm_mgResolutionsTrigger.mg_iCenterI = -1;

  // [Cecil] Changed fullscreen switch to window modes
  gm_mgWindowModeTrigger.mg_pmgUp = &gm_mgResolutionsTrigger;
  gm_mgWindowModeTrigger.mg_pmgDown = &gm_mgBitsPerPixelTrigger;
  gm_mgWindowModeTrigger.mg_boxOnScreen = BoxMediumLeft(11.1f);
  gm_lhGadgets.AddTail(gm_mgWindowModeTrigger.mg_lnNode);
  gm_mgWindowModeTrigger.mg_astrTexts = _astrWindowModes;
  gm_mgWindowModeTrigger.mg_ctTexts = sizeof(_astrWindowModes) / sizeof(_astrWindowModes[0]);
  gm_mgWindowModeTrigger.mg_iSelected = 0;
  gm_mgWindowModeTrigger.mg_strLabel = Translate("ETRS" "WINDOW MODE", 4);
  gm_mgWindowModeTrigger.mg_strValue = _astrWindowModes[0];
  gm_mgWindowModeTrigger.mg_strTip = TRANS("Run game in a window or in full screen");
  gm_mgWindowModeTrigger.mg_iCenterI = -1;

  gm_mgBitsPerPixelTrigger.mg_pmgUp = &gm_mgWindowModeTrigger;
  gm_mgBitsPerPixelTrigger.mg_pmgDown = &gm_mgVideoRendering;
  gm_mgBitsPerPixelTrigger.mg_boxOnScreen = BoxMediumLeft(12.1f);
  gm_lhGadgets.AddTail(gm_mgBitsPerPixelTrigger.mg_lnNode);
  gm_mgBitsPerPixelTrigger.mg_astrTexts = astrBitsPerPixelRadioTexts;
  gm_mgBitsPerPixelTrigger.mg_ctTexts = sizeof(astrBitsPerPixelRadioTexts) / sizeof(astrBitsPerPixelRadioTexts[0]);
  gm_mgBitsPerPixelTrigger.mg_iSelected = 0;
  gm_mgBitsPerPixelTrigger.mg_strLabel = Translate("ETRS" "BITS PER PIXEL", 4);
  gm_mgBitsPerPixelTrigger.mg_strValue = astrBitsPerPixelRadioTexts[0];
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
  gm_mgVideoRendering.mg_boxOnScreen = BoxMediumLeft(13.1f);
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

  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_strTip = TRANS("Return to main menu");
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_pmgUp = &gm_mgApply;
  gm_mgBack.mg_pmgDown = &gm_mgDisplayAPITrigger;
  gm_mgBack.mg_pActivatedFunction = NULL;
  gm_mgBack.mg_iCenterI = -1;
}

void CVideoOptionsMenu::StartMenu(void)
{
  InitVideoOptionsButtons();

  CGameMenu::StartMenu();

  UpdateVideoOptionsButtons(-1);
}