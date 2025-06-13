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

static CTextureObject atoLoading[24];
static CTextureObject *ptoLoading;

BOOL LoadLoadingTextures(void)
{
  try {
    atoLoading[0].SetData_t(CTFILENAME("Textures\\Loading\\Frame1.tex"));
    atoLoading[1].SetData_t(CTFILENAME("Textures\\Loading\\Frame2.tex"));;
    atoLoading[2].SetData_t(CTFILENAME("Textures\\Loading\\Frame3.tex"));
    atoLoading[3].SetData_t(CTFILENAME("Textures\\Loading\\Frame4.tex"));
    atoLoading[4].SetData_t(CTFILENAME("Textures\\Loading\\Frame5.tex"));
    atoLoading[5].SetData_t(CTFILENAME("Textures\\Loading\\Frame6.tex"));
    atoLoading[6].SetData_t(CTFILENAME("Textures\\Loading\\Frame7.tex"));
    atoLoading[7].SetData_t(CTFILENAME("Textures\\Loading\\Frame8.tex"));
    atoLoading[8].SetData_t(CTFILENAME("Textures\\Loading\\Frame9.tex"));
    atoLoading[9].SetData_t(CTFILENAME("Textures\\Loading\\Frame10.tex"));
    atoLoading[10].SetData_t(CTFILENAME("Textures\\Loading\\Frame11.tex"));
    atoLoading[11].SetData_t(CTFILENAME("Textures\\Loading\\Frame12.tex"));
    atoLoading[12].SetData_t(CTFILENAME("Textures\\Loading\\Frame13.tex"));
    atoLoading[13].SetData_t(CTFILENAME("Textures\\Loading\\Frame14.tex"));
    atoLoading[14].SetData_t(CTFILENAME("Textures\\Loading\\Frame15.tex"));;
    atoLoading[15].SetData_t(CTFILENAME("Textures\\Loading\\Frame16.tex"));
    atoLoading[16].SetData_t(CTFILENAME("Textures\\Loading\\Frame17.tex"));
    atoLoading[17].SetData_t(CTFILENAME("Textures\\Loading\\Frame18.tex"));
    atoLoading[18].SetData_t(CTFILENAME("Textures\\Loading\\Frame19.tex"));
    atoLoading[19].SetData_t(CTFILENAME("Textures\\Loading\\Frame20.tex"));
    atoLoading[20].SetData_t(CTFILENAME("Textures\\Loading\\Frame21.tex"));
    atoLoading[21].SetData_t(CTFILENAME("Textures\\Loading\\Frame22.tex"));
    atoLoading[22].SetData_t(CTFILENAME("Textures\\Loading\\Frame23.tex"));
    atoLoading[23].SetData_t(CTFILENAME("Textures\\Loading\\Frame24.tex"));

    // force constant textures
    ((CTextureData*)atoLoading[0].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[1].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[2].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[3].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[4].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[5].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[6].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[7].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[8].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[9].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[10].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[11].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[12].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[13].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[14].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[15].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[16].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[17].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[18].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[19].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[20].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[21].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[22].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoLoading[23].GetData())->Force(TEX_CONSTANT);
  }
  catch (char *strError) {
    CPrintF("%s\n", strError);

    return FALSE;
  }

  return TRUE;
}

void FreeLoadingTextures(void)
{
  atoLoading[0].SetData(NULL);
  atoLoading[1].SetData(NULL);
  atoLoading[2].SetData(NULL);
  atoLoading[3].SetData(NULL);
  atoLoading[4].SetData(NULL);
  atoLoading[5].SetData(NULL);
  atoLoading[6].SetData(NULL);
  atoLoading[7].SetData(NULL);
  atoLoading[8].SetData(NULL);
  atoLoading[9].SetData(NULL);
  atoLoading[10].SetData(NULL);
  atoLoading[11].SetData(NULL);
  atoLoading[12].SetData(NULL);
  atoLoading[13].SetData(NULL);
  atoLoading[14].SetData(NULL);
  atoLoading[15].SetData(NULL);
  atoLoading[16].SetData(NULL);
  atoLoading[17].SetData(NULL);
  atoLoading[18].SetData(NULL);
  atoLoading[19].SetData(NULL);
  atoLoading[20].SetData(NULL);
  atoLoading[21].SetData(NULL);
  atoLoading[22].SetData(NULL);
  atoLoading[23].SetData(NULL);
}

void RenderLoadingScreen(CDrawPort *pdp, CProgressHookInfo *pphi)
{
  if(!LoadLoadingTextures())
  {
    FreeLoadingTextures();

    return;
  }

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

  ptoLoading = &atoLoading[0];

  if (pphi != NULL)
  {
    INDEX iLoadingTexture = pphi->phi_fCompleted * ARRAYCOUNT(atoLoading);

    ptoLoading = &atoLoading[iLoadingTexture];
  }

  pdp->PutTexture(ptoLoading, PIXaabbox2D(PIX2D(pixI, pixJ), PIX2D(pixI + pixLogoSizeI, pixJ + pixLogoSizeJ)));

  pixJ += pixLogoSizeJ + pixPaddingJ;

  pdp->PutText(TRANS("Loading"), pixI, pixJ, colText);

  FreeLoadingTextures();
}
