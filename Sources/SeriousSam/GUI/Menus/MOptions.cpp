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
#include "MOptions.h"


void COptionsMenu::Initialize_t(void)
{
  // intialize options menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle(2.85f);
  gm_mgTitle.mg_strText = TRANS("[OPTIONS]");
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  gm_mgGameOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgGameOptions.mg_boxOnScreen = BoxBigLeft(5.5f);
  gm_mgGameOptions.mg_pmgUp = &gm_mgBack;
  gm_mgGameOptions.mg_pmgDown = &gm_mgAudioOptions;
  gm_mgGameOptions.mg_strText = TRANS("GAME OPTIONS");
  gm_mgGameOptions.mg_strTip = TRANS("Change game options");
  gm_lhGadgets.AddTail(gm_mgGameOptions.mg_lnNode);
  gm_mgGameOptions.mg_pActivatedFunction = NULL;
  gm_mgGameOptions.mg_iCenterI = -1;

  gm_mgVideoOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgVideoOptions.mg_boxOnScreen = BoxBigLeft(6.5f);
  gm_mgVideoOptions.mg_pmgUp = &gm_mgGameOptions;
  gm_mgVideoOptions.mg_pmgDown = &gm_mgAudioOptions;
  gm_mgVideoOptions.mg_strText = TRANS("VIDEO OPTIONS");
  gm_mgVideoOptions.mg_strTip = TRANS("Set video mode and driver");
  gm_lhGadgets.AddTail(gm_mgVideoOptions.mg_lnNode);
  gm_mgVideoOptions.mg_pActivatedFunction = NULL;
  gm_mgVideoOptions.mg_iCenterI = -1;

  gm_mgAudioOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgAudioOptions.mg_boxOnScreen = BoxBigLeft(7.5f);
  gm_mgAudioOptions.mg_pmgUp = &gm_mgVideoOptions;
  gm_mgAudioOptions.mg_pmgDown = &gm_mgControls;
  gm_mgAudioOptions.mg_strText = TRANS("AUDIO OPTIONS");
  gm_mgAudioOptions.mg_strTip = TRANS("Set audio quality and volume");
  gm_lhGadgets.AddTail(gm_mgAudioOptions.mg_lnNode);
  gm_mgAudioOptions.mg_pActivatedFunction = NULL;
  gm_mgAudioOptions.mg_iCenterI = -1;

  gm_mgControls.mg_bfsFontSize = BFS_LARGE;
  gm_mgControls.mg_boxOnScreen = BoxBigLeft(8.5f);
  gm_mgControls.mg_pmgUp = &gm_mgAudioOptions;
  gm_mgControls.mg_pmgDown = &gm_mgBack;
  gm_mgControls.mg_strText = TRANS("CONTROLS");
  gm_mgControls.mg_strTip = TRANS("Adjust controls");
  gm_lhGadgets.AddTail(gm_mgControls.mg_lnNode);
  gm_mgControls.mg_pActivatedFunction = NULL;
  gm_mgControls.mg_iCenterI = -1;

  gm_mgBack.mg_bfsFontSize = BFS_LARGE;
  gm_mgBack.mg_boxOnScreen = BoxBigLeft(9.5f);
  gm_mgBack.mg_pmgUp = &gm_mgControls;
  gm_mgBack.mg_pmgDown = &gm_mgGameOptions;
  gm_mgBack.mg_strText = TRANS("BACK");
  gm_mgBack.mg_strTip = TRANS("Return to previous menu");
  gm_lhGadgets.AddTail(gm_mgBack.mg_lnNode);
  gm_mgBack.mg_pActivatedFunction = NULL;
  gm_mgBack.mg_iCenterI = -1;
}