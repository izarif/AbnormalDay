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


#include "StdAfx.h"
#include "LCDDrawing.h"
#include <locale.h>

#define USECUSTOMTEXT 0

extern CGame *_pGame;

#if USECUSTOMTEXT
  static CTString _strCustomText = "";
#endif
static CDrawPort *_pdpLoadingHook = NULL;  // drawport for loading hook
extern BOOL _bUserBreakEnabled;


#define REFRESHTIME (0.2f)

void RemapLevelNames(INDEX &iLevel)
{
  switch( iLevel) {
  case 10:  iLevel =  1;  break;
  case 11:  iLevel =  2;  break;
  case 12:  iLevel =  3;  break;
  case 13:  iLevel =  4;  break;
  case 14:  iLevel =  5;  break;
  case 15:  iLevel =  6;  break;
  case 21:  iLevel =  7;  break;
  case 22:  iLevel =  8;  break;
  case 23:  iLevel =  9;  break;
  case 24:  iLevel = 10;  break;
  case 31:  iLevel = 11;  break;
  case 32:  iLevel = 12;  break;
  case 33:  iLevel = 13;  break;
  default:  iLevel = -1;	break;
  }
}


static void LoadingHook_t(CProgressHookInfo *pphi)
{
  // if user presses escape
  ULONG ulCheckFlags = 0x8000;
  if (pphi->phi_fCompleted>0) {
    ulCheckFlags |= 0x0001;
  }
  if (_bUserBreakEnabled && (GetAsyncKeyState(VK_ESCAPE)&ulCheckFlags)) {
    // break loading
    throw TRANS("User break!");
  }

#if USECUSTOMTEXT
  // if no custom loading text
  if (_strCustomText=="") {
    // load it
    try {
      _strCustomText.Load_t(CTFILENAME("Data\\LoadingText.txt"));
    } catch (const char *strError) {
      _strCustomText = strError;
    }
  }
#endif

  // measure time since last call
  static CTimerValue tvLast((__int64) 0);
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

  // if not first or final update, and not enough time passed
  if (pphi->phi_fCompleted!=0 && pphi->phi_fCompleted!=1 &&
     (tvNow-tvLast).GetSeconds() < REFRESHTIME) {
    // do nothing
    return;
  }
  tvLast = tvNow;

  // skip if cannot lock drawport
  CDrawPort *pdp = _pdpLoadingHook;                           
  ASSERT(pdp!=NULL);
  CDrawPort dpHook(pdp, TRUE);
  if( !dpHook.Lock()) return;

  // clear screen
  dpHook.Fill(C_BLACK|255);

  // get session properties currently loading
  CSessionProperties *psp = (CSessionProperties *)_pNetwork->GetSessionProperties();
  ULONG ulLevelMask = psp->sp_ulLevelsMask;
  INDEX iLevel = -1;
  if (psp->sp_bCooperative) {
    INDEX iLevel = -1;
    INDEX iLevelNext = -1;
    CTString strLevelName = _pNetwork->ga_fnmWorld.FileName();
    CTString strNextLevelName = _pNetwork->ga_fnmNextLevel.FileName();
    
    // second encounter
    INDEX u, v;
    u = v = -1;
    strLevelName.ScanF("%01d_%01d_", &u, &v);
    iLevel = u*10+v;
    RemapLevelNames(iLevel);
    u = v = -1;
    strNextLevelName.ScanF("%01d_%01d_", &u, &v);
    iLevelNext = u*10+v;
    RemapLevelNames(iLevelNext);
    
    //strLevelName.ScanF("%02d_", &iLevel);
    //strNextLevelName.ScanF("%02d_", &iLevelNext);
   
    if (iLevel>0) {
      ulLevelMask|=1<<(iLevel-1);
    }
    if (iLevelNext>0) {
      ulLevelMask|=1<<(iLevelNext-1);
    }
  }

  if (ulLevelMask!=0 && !_pNetwork->IsPlayingDemo()) {
    // map hook
    extern void RenderLoadingScreen(CDrawPort *pdp, CProgressHookInfo *pphi);

    RenderLoadingScreen(&dpHook, pphi);

    // finish rendering
    dpHook.Unlock();
    dpHook.dp_Raster->ra_pvpViewPort->SwapBuffers();

    // keep current time
    tvLast = _pTimer->GetHighPrecisionTimer();
    return;
  }

  // get sizes
  PIX pixScreenSizeI = dpHook.GetWidth();
  PIX pixScreenSizeJ = dpHook.GetHeight();
  CFontData *pfd = _pfdConsoleFont;
  PIX pixCharSizeI = pfd->fd_pixCharWidth  + pfd->fd_pixCharSpacing;

  FLOAT fScaleW = pixScreenSizeI / 640.0f;
  FLOAT fScaleH = pixScreenSizeJ / 480.0f;

  PIX pixCharSizeJ = (pfd->GetHeight() - 1) * fScaleH;
  PIX pixBarSizeJ = 22 * fScaleH;

  COLOR colBcg = SE_COL_GREEN_DARK | 255;
  COLOR colBar = SE_COL_GREEN_LIGHT | 255;
  COLOR colLines = colBar; //C_vdGREEN|0xff;
  COLOR colText = SE_COL_GREEN_LIGHT | 255;
  COLOR colEsc = SE_COL_GREEN_LIGHT | 255;

  PIX pixBarBorderW = 1;
  PIX pixPaddingI = 6 * fScaleW;
  PIX pixPaddingJ = 6 * fScaleH;
  PIX pixBarSizeI = pixScreenSizeI - (pixPaddingI * 2);
  PIX pixI = pixPaddingI - (3 * fScaleW);
  PIX pixJ = pixScreenSizeJ - (pixPaddingJ * 2) - pixBarSizeJ - pixCharSizeJ;

  dpHook.SetFont(_pfdConsoleFont);
  dpHook.SetTextScaling(fScaleH);
  dpHook.SetTextAspect(1.0f);

  if (_bUserBreakEnabled && !_pGame->gm_bFirstLoading) {
    dpHook.PutText(TRANS("[Esc] - Abort"), pixI, pixJ, colEsc);
  }

  pixI += (3 * fScaleW);
  pixJ += pixCharSizeJ + pixPaddingJ;

  dpHook.Fill(pixI, pixJ, pixBarSizeI, pixBarSizeJ, colBcg);
  dpHook.Fill(pixI, pixJ, pixScreenSizeI * pphi->phi_fCompleted, pixBarSizeJ, colBar);
  dpHook.DrawBorder(pixI, pixJ, pixBarSizeI, pixBarSizeJ, colLines);

  // print status text
  CTString strDesc(0, "%s", (const char *) pphi->phi_strDescription);
  strupr((char*)(const char*)strDesc);
  CTString strPerc(0, "%3.0f%%", pphi->phi_fCompleted*100);
  //dpHook.PutText(strDesc, pixCharSizeI/2, pixSizeJ-pixBarSizeJ-2-pixCharSizeJ, C_GREEN|255);
  //dpHook.PutTextCXY(strPerc, pixSizeI/2, pixSizeJ-pixBarSizeJ/2+1, C_GREEN|255);

  pixI += pixBarBorderW + pixPaddingI - (3 * fScaleW);
  pixJ += pixBarBorderW + pixPaddingJ;

  dpHook.PutText(strDesc, pixI, pixJ, colText);

  PIX pixDescW = dpHook.GetTextWidth(strDesc);
  pixI += pixDescW + (pixPaddingI * 2);

  dpHook.PutText(strPerc, pixI, pixJ, colText);

/*  
  //_pGame->LCDPrepare(1.0f);
  //_pGame->LCDSetDrawport(&dpHook);
  
  // fill the box with background dirt and grid
  //_pGame->LCDRenderClouds1();
  //_pGame->LCDRenderGrid();

  // draw progress bar
  PIX pixBarCentI = pixBoxSizeI*1/2;
  PIX pixBarCentJ = pixBoxSizeJ*3/4;
  PIX pixBarSizeI = pixBoxSizeI*7/8;
  PIX pixBarSizeJ = pixBoxSizeJ*3/8;
  PIX pixBarMinI = pixBarCentI-pixBarSizeI/2;
  PIX pixBarMaxI = pixBarCentI+pixBarSizeI/2;
  PIX pixBarMinJ = pixBarCentJ-pixBarSizeJ/2;
  PIX pixBarMaxJ = pixBarCentJ+pixBarSizeJ/2;

  dpBox.Fill(pixBarMinI, pixBarMinJ, 
    pixBarMaxI-pixBarMinI, pixBarMaxJ-pixBarMinJ, C_BLACK|255);
  dpBox.Fill(pixBarMinI, pixBarMinJ, 
    (pixBarMaxI-pixBarMinI)*pphi->phi_fCompleted, pixBarMaxJ-pixBarMinJ, C_GREEN|255);

  // put more dirt
  _pGame->LCDRenderClouds2Light();

  // draw borders
  COLOR colBorders = LerpColor(C_GREEN, C_BLACK, 200);
  _pGame->LCDDrawBox(0,-1, PIXaabbox2D(
    PIX2D(pixBarMinI, pixBarMinJ), 
    PIX2D(pixBarMaxI, pixBarMaxJ)), 
    colBorders|255);
  _pGame->LCDDrawBox(0,-1, PIXaabbox2D(
    PIX2D(0,0), PIX2D(dpBox.GetWidth(), dpBox.GetHeight())), 
    colBorders|255);

  // print status text
  dpBox.SetFont( _pfdDisplayFont);
  dpBox.SetTextScaling( 1.0f);
  dpBox.SetTextAspect( 1.0f);
  // print status text
  CTString strRes;
  strRes.PrintF( "%s", (const char *) pphi->phi_strDescription);
  //strupr((char*)(const char*)strRes);
  dpBox.PutTextC( strRes, 160, 17, C_GREEN|255);
  strRes.PrintF( "%3.0f%%", pphi->phi_fCompleted*100);
  dpBox.PutTextCXY( strRes, pixBarCentI, pixBarCentJ, C_GREEN|255);
  dpBox.Unlock();

  if( Flesh.gm_bFirstLoading) {
#if USECUSTOMTEXT
    FLOAT fScaling = (FLOAT)slSizeI/640.0f;
    dpHook.Lock();
    dpHook.SetFont( _pfdDisplayFont);
    dpHook.SetTextScaling( fScaling);
    dpHook.SetTextAspect( 1.0f);
    //dpHook.Fill( 0, 0, slSizeI, pixCenterJ, C_vdGREEN|255, C_vdGREEN|255, C_vdGREEN|0, C_vdGREEN|0);
    dpHook.PutTextC( TRANS( "SERIOUS SAM - TEST VERSION"), pixCenterI, 5*fScaling, C_WHITE|255);
    dpHook.PutTextC( TRANS( "THIS IS NOT A DEMO VERSION, THIS IS A COMPATIBILITY TEST!"), pixCenterI, 25*fScaling, C_WHITE|255);
    dpHook.PutTextC( TRANS( "Serious Sam (c) 2000 Croteam LLC, All Rights Reserved.\n"), pixCenterI, 45*fScaling, C_WHITE|255);
    dpHook.PutText( _strCustomText, 1*fScaling, 85*fScaling, C_GREEN|255);
    dpHook.Unlock();
#endif
  } else if (_bUserBreakEnabled) {
    FLOAT fScaling = (FLOAT)slSizeI/640.0f;
    dpHook.Lock();
    dpHook.SetFont( _pfdDisplayFont);
    dpHook.SetTextScaling( fScaling);
    dpHook.SetTextAspect( 1.0f);
    //dpHook.Fill( 0, 0, slSizeI, pixCenterJ, C_vdGREEN|255, C_vdGREEN|255, C_vdGREEN|0, C_vdGREEN|0);
    dpHook.PutTextC( TRANS( "PRESS ESC TO ABORT"), pixCenterI, pixCenterJ+pixBoxSizeJ+5*fScaling, C_WHITE|255);
  }
  */

  dpHook.Unlock();
  // finish rendering
  dpHook.dp_Raster->ra_pvpViewPort->SwapBuffers();

  // keep current time
  tvLast = _pTimer->GetHighPrecisionTimer();
}

// loading hook functions
void CGame::EnableLoadingHook(CDrawPort *pdpDrawport)
{
  _pdpLoadingHook = pdpDrawport;
  SetProgressHook(LoadingHook_t);
}

void CGame::DisableLoadingHook(void)
{
  SetProgressHook(NULL);
  _pdpLoadingHook = NULL;
}
