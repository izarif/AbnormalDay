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

extern CTextureObject toLoading;

void RenderLoadingScreen(CDrawPort *pdp, CProgressHookInfo *pphi)
{
  PIX pixScreenSizeI = pdp->GetWidth();
  PIX pixScreenSizeJ = pdp->GetHeight();

  FLOAT fScaleW = pixScreenSizeI / 640.0f;
  FLOAT fScaleH = pixScreenSizeJ / 480.0f;

  CFontData* pfd = _pfdConsoleFont;
  PIX pixCharSizeJ = (pfd->GetHeight() - 1) * fScaleH;

  COLOR colText = SE_COL_GREEN_LIGHT | 255;

  PIX pixPaddingI = 6 * fScaleW;
  PIX pixPaddingJ = 6 * fScaleH;
  PIX pixLogoSizeI = 50 * fScaleW;
  PIX pixLogoSizeJ = 50 * fScaleH;
  PIX pixI = pixPaddingI - (3 * fScaleW);
  PIX pixJ = pixScreenSizeJ - (pixPaddingJ * 2) - pixLogoSizeJ - pixCharSizeJ;

  pdp->SetFont(_pfdConsoleFont);
  pdp->SetTextScaling(fScaleH);
  pdp->SetTextAspect(1.0f);

  pdp->Fill(C_BLACK | 225);

  if (pphi != NULL)
  {
    CTextureData* ptdLoading = (CTextureData*)toLoading.GetData();
    INDEX iLoadingFrame = pphi->phi_fCompleted * ptdLoading->td_ctFrames;

    ptdLoading->SetAsCurrent(iLoadingFrame);
  }

  pdp->PutTexture(&toLoading, PIXaabbox2D(PIX2D(pixI, pixJ), PIX2D(pixI + pixLogoSizeI, pixJ + pixLogoSizeJ)));

  pixJ += pixLogoSizeJ + pixPaddingJ;

  pdp->PutText(TRANS("Loading"), pixI, pixJ, colText);
}
