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
#include "MAudioOptions.h"

extern void RefreshSoundFormat(void);


void CAudioOptionsMenu::Initialize_t(void)
{
  // [Cecil] Create array of available sound APIs
  const INDEX ctAPIs = CAbstractSoundAPI::E_SND_MAX;

  if (astrSoundAPIRadioTexts == NULL) {
    astrSoundAPIRadioTexts = new CTString[ctAPIs];

    for (INDEX iAPI = 0; iAPI < ctAPIs; iAPI++) {
      const CTString &strAPI = CAbstractSoundAPI::GetApiName((CAbstractSoundAPI::ESoundAPI)iAPI);
      astrSoundAPIRadioTexts[iAPI] = TRANSV(strAPI);
    }
  }

  // intialize Audio options menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle(2.63f);
  gm_mgTitle.mg_strText = TRANS("# AUDIO");
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  gm_mgAudioAutoTrigger.mg_pmgUp = &gm_mgBack;
  gm_mgAudioAutoTrigger.mg_pmgDown = &gm_mgFrequencyTrigger;
  gm_mgAudioAutoTrigger.mg_boxOnScreen = BoxMediumLeft(9.1f);
  gm_lhGadgets.AddTail(gm_mgAudioAutoTrigger.mg_lnNode);
  gm_mgAudioAutoTrigger.mg_astrTexts = astrNoYes;
  gm_mgAudioAutoTrigger.mg_ctTexts = sizeof(astrNoYes) / sizeof(astrNoYes[0]);
  gm_mgAudioAutoTrigger.mg_iSelected = 0;
  gm_mgAudioAutoTrigger.mg_strLabel = Translate("ETRS" "AUTO-ADJUST", 4);
  gm_mgAudioAutoTrigger.mg_strValue = astrNoYes[0];
  gm_mgAudioAutoTrigger.mg_strTip = TRANS("Adjust quality to fit your system");
  gm_mgAudioAutoTrigger.mg_pOnTriggerChange = NULL;
  gm_mgAudioAutoTrigger.mg_iCenterI = -1;

  gm_mgFrequencyTrigger.mg_pmgUp = &gm_mgAudioAutoTrigger;
  gm_mgFrequencyTrigger.mg_pmgDown = &gm_mgAudioAPITrigger;
  gm_mgFrequencyTrigger.mg_boxOnScreen = BoxMediumLeft(10.1f);
  gm_lhGadgets.AddTail(gm_mgFrequencyTrigger.mg_lnNode);
  gm_mgFrequencyTrigger.mg_astrTexts = astrFrequencyRadioTexts;
  gm_mgFrequencyTrigger.mg_ctTexts = sizeof(astrFrequencyRadioTexts) / sizeof(astrFrequencyRadioTexts[0]);
  gm_mgFrequencyTrigger.mg_iSelected = 0; gm_mgFrequencyTrigger.mg_strLabel = Translate("ETRS" "FREQUENCY", 4);
  gm_mgFrequencyTrigger.mg_strValue = astrFrequencyRadioTexts[0];
  gm_mgFrequencyTrigger.mg_strTip = TRANS("Select sound quality or turn sound off");
  gm_mgFrequencyTrigger.mg_pOnTriggerChange = NULL;
  gm_mgFrequencyTrigger.mg_iCenterI = -1;

  gm_mgAudioAPITrigger.mg_pmgUp = &gm_mgFrequencyTrigger;
  gm_mgAudioAPITrigger.mg_pmgDown = &gm_mgWaveVolume;
  gm_mgAudioAPITrigger.mg_boxOnScreen = BoxMediumLeft(11.1f);
  gm_lhGadgets.AddTail(gm_mgAudioAPITrigger.mg_lnNode);
  gm_mgAudioAPITrigger.mg_astrTexts = astrSoundAPIRadioTexts;
  gm_mgAudioAPITrigger.mg_iSelected = 0; gm_mgAudioAPITrigger.mg_strLabel = Translate("ETRS" "SOUND SYSTEM", 4);
  gm_mgAudioAPITrigger.mg_strValue = astrSoundAPIRadioTexts[0];
  gm_mgAudioAPITrigger.mg_strTip = TRANS("Choose sound system to use");
  gm_mgAudioAPITrigger.mg_pOnTriggerChange = NULL;
  gm_mgAudioAPITrigger.mg_ctTexts = ctAPIs; // [Cecil] Amount of available APIs
  gm_mgAudioAPITrigger.mg_iCenterI = -1;

  gm_mgWaveVolume.mg_boxOnScreen = BoxMediumLeft(12.1f);
  gm_mgWaveVolume.mg_strText = TRANS("SOUND EFFECTS VOLUME");
  gm_mgWaveVolume.mg_strTip = TRANS("Adjust volume of sound effects");
  gm_mgWaveVolume.mg_pmgUp = &gm_mgAudioAPITrigger;
  gm_mgWaveVolume.mg_pmgDown = &gm_mgMPEGVolume;
  gm_mgWaveVolume.mg_pOnSliderChange = NULL;
  gm_mgWaveVolume.mg_pActivatedFunction = NULL;
  gm_lhGadgets.AddTail(gm_mgWaveVolume.mg_lnNode);
  gm_mgWaveVolume.mg_iCenterI = -1;

  gm_mgMPEGVolume.mg_boxOnScreen = BoxMediumLeft(13.1f);
  gm_mgMPEGVolume.mg_strText = TRANS("MUSIC VOLUME");
  gm_mgMPEGVolume.mg_strTip = TRANS("Adjust volume of music");
  gm_mgMPEGVolume.mg_pmgUp = &gm_mgWaveVolume;
  gm_mgMPEGVolume.mg_pmgDown = &gm_mgApply;
  gm_mgMPEGVolume.mg_pOnSliderChange = NULL;
  gm_mgMPEGVolume.mg_pActivatedFunction = NULL;
  gm_lhGadgets.AddTail(gm_mgMPEGVolume.mg_lnNode);
  gm_mgMPEGVolume.mg_iCenterI = -1;

  gm_mgApply.mg_bfsFontSize = BFS_LARGE;
  gm_mgApply.mg_boxOnScreen = BoxBigLeft(8.5f);
  gm_mgApply.mg_strText = TRANS("APPLY");
  gm_mgApply.mg_strTip = TRANS("Apply options");
  gm_lhGadgets.AddTail(gm_mgApply.mg_lnNode);
  gm_mgApply.mg_pmgUp = &gm_mgMPEGVolume;
  gm_mgApply.mg_pmgDown = &gm_mgBack;
  gm_mgApply.mg_pActivatedFunction = NULL;
  gm_mgApply.mg_iCenterI = -1;

  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_strTip = TRANS("Return to main menu");
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_pmgUp = &gm_mgApply;
  gm_mgBack.mg_pmgDown = &gm_mgAudioAutoTrigger;
  gm_mgBack.mg_pActivatedFunction = NULL;
  gm_mgBack.mg_iCenterI = -1;
}

void CAudioOptionsMenu::StartMenu(void)
{
  RefreshSoundFormat();
  CGameMenu::StartMenu();
}