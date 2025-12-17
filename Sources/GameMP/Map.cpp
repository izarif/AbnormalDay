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

static CTextureObject atoIcons[13];
static CTextureObject _toPathDot;
static CTextureObject _toMapBcgLD;
static CTextureObject _toMapBcgLU;
static CTextureObject _toMapBcgRD;
static CTextureObject _toMapBcgRU;

PIX aIconCoords[][2] =
{
  {0, 0},      // 00: Last Episode
  {168, 351},  // 01: Palenque 01 
  {42, 345},   // 02: Palenque 02 
  {41, 263},   // 03: Teotihuacan 01      
  {113, 300},  // 04: Teotihuacan 02      
  {334, 328},  // 05: Teotihuacan 03      
  {371, 187},  // 06: Ziggurat	 
  {265, 111},  // 07: Atrium		 
  {119, 172},  // 08: Gilgamesh	 
  {0, 145},    // 09: Babel		   
  {90, 30},    // 10: Citadel		 
  {171, 11},   // 11: Land of Damned		     
  {376, 0},    // 12: Cathedral	 
};

#define LASTEPISODE_BIT 0
#define PALENQUE01_BIT 1
#define PALENQUE02_BIT  2
#define TEOTIHUACAN01_BIT 3
#define TEOTIHUACAN02_BIT 4
#define TEOTIHUACAN03_BIT 5
#define ZIGGURAT_BIT 6
#define ATRIUM_BIT 7
#define GILGAMESH_BIT 8
#define BABEL_BIT 9
#define CITADEL_BIT 10
#define LOD_BIT 11
#define CATHEDRAL_BIT 12

INDEX  aPathPrevNextLevels[][2] = 
{
  {LASTEPISODE_BIT, PALENQUE01_BIT},      // 00
  {PALENQUE01_BIT, PALENQUE02_BIT},       // 01
  {PALENQUE02_BIT, TEOTIHUACAN01_BIT },   // 02
  {TEOTIHUACAN01_BIT, TEOTIHUACAN02_BIT}, // 03
  {TEOTIHUACAN02_BIT, TEOTIHUACAN03_BIT}, // 04
  {TEOTIHUACAN03_BIT, ZIGGURAT_BIT},      // 05
  {ZIGGURAT_BIT, ATRIUM_BIT},             // 06
  {ATRIUM_BIT, GILGAMESH_BIT},            // 07
  {GILGAMESH_BIT, BABEL_BIT},             // 08
  {BABEL_BIT, CITADEL_BIT},               // 09
  {CITADEL_BIT, LOD_BIT},                 // 10
  {LOD_BIT, CATHEDRAL_BIT},               // 11

};

PIX aPathDots[][10][2] = 
{
  // 00: Palenque01 - Palenque02
  {
    {-1,-1},
  },

  // 01: Palenque01 - Palenque02
  {
    {211,440},
    {193,447},
    {175,444},
    {163,434},
    {152,423},
    {139,418},
    {-1,-1},
  },
  
  // 02: Palenque02 - Teotihuacan01
  {
    {100,372},
    {102,363},
    {108,354},
    {113,345},
    {106,338},
    {-1,-1},
  },

  // 03: Teotihuacan01 - Teotihuacan02
  {
    {153,337},
    {166,341},
    {180,346},
    {194,342},
    {207,337},
    {-1,-1},
  },

  // 04: Teotihuacan02 - Teotihuacan03
  {
    {279,339},
    {287,347},
    {296,352},
    {307,365},
    {321,367},
    {335,362},
    {-1,-1},
  },

  // 05: Teotihuacan03 - Ziggurat
  {
    {-1,-1},
  },

  // 06: Ziggurat - Atrium
  {
    {412,285},
    {396,282},
    {383,273},
    {368,266},
    {354,264},
    {-1,-1},
  },

  // 07: Atrium - Gilgamesh
  {
    {276,255},
    {262,258},
    {248,253},
    {235,245},
    {222,240},
    {-1,-1},
  },

  // 08: Gilgamesh - Babel
  {
    {152,245},
    {136,248},
    {118,253},
    {100,251},
    {85,246},
    {69,243},
    {-1,-1},
  },

  // 09: Babel - Citadel
  {
    {-1,-1},
  },

  // 10: Citadel - Lod
  {
    {190,130},
    {204,126},
    {215,119},
    {232,116},
    {241,107},
    {-1,-1},
  },

  // 11: Lod - Cathedral
  {
    {330,108},
    {341,117},
    {353,126},
    {364,136},
    {377,146},
    {395,147},
    {-1,-1},
  },

};

BOOL ObtainMapData(void)
{
  try {
    atoIcons[0].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading0.tex"));
    atoIcons[1].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading1.tex"));
    atoIcons[2].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading2.tex"));
    atoIcons[3].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading3.tex"));
    atoIcons[4].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading4.tex"));
    atoIcons[5].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading5.tex"));
    atoIcons[6].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading6.tex"));
    atoIcons[7].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading7.tex"));
    atoIcons[8].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading8.tex"));
    atoIcons[9].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading9.tex"));
    atoIcons[10].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading10.tex"));
    atoIcons[11].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading11.tex"));
    atoIcons[12].SetData_t(CTFILENAME("Textures\\Computer\\Loading\\Loading12.tex"));
    _toPathDot  .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\PathDot.tex"));
    _toMapBcgLD .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgLD.tex"));
    _toMapBcgLU .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgLU.tex"));
    _toMapBcgRD .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgRD.tex"));
    _toMapBcgRU .SetData_t(CTFILENAME("TexturesMP\\Computer\\Map\\MapBcgRU.tex"));
    // force constant textures
    ((CTextureData*)atoIcons[0].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[1].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[2].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[3].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[4].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[5].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[6].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[7].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[8].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[9].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[10].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[11].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)atoIcons[12].GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toPathDot  .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgLD .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgLU .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgRD .GetData())->Force(TEX_CONSTANT);
    ((CTextureData*)_toMapBcgRU .GetData())->Force(TEX_CONSTANT);
  } 
  catch (char *strError) {
    CPrintF("%s\n", strError);
    return FALSE;
  }
  return TRUE;
}

void ReleaseMapData(void)
{
  atoIcons[0].SetData(NULL);
  atoIcons[1].SetData(NULL);
  atoIcons[2].SetData(NULL);
  atoIcons[3].SetData(NULL);
  atoIcons[4].SetData(NULL);
  atoIcons[5].SetData(NULL);
  atoIcons[6].SetData(NULL);
  atoIcons[7].SetData(NULL);
  atoIcons[8].SetData(NULL);
  atoIcons[9].SetData(NULL);
  atoIcons[10].SetData(NULL);
  atoIcons[11].SetData(NULL);
  atoIcons[12].SetData(NULL);
  _toPathDot.SetData(NULL);
  _toMapBcgLD.SetData(NULL);
  _toMapBcgLU.SetData(NULL);
  _toMapBcgRD.SetData(NULL);
  _toMapBcgRU.SetData(NULL);
}

void RenderMap(CDrawPort *pdp, ULONG ulLevelMask, CProgressHookInfo *pphi)
{
  if( !ObtainMapData())
  {
    ReleaseMapData();
    return;
  }

  PIX pixScreenSizeI = pdp->GetWidth();
  PIX pixScreenSizeJ = pdp->GetHeight();

  FLOAT fScaleW = pixScreenSizeI / 640.0f;
  FLOAT fScaleH = pixScreenSizeJ / 480.0f;

  PIX pixFontSizeJ = _pfdConsoleFont->GetHeight();
  PIX pixCharSizeJ = (pixFontSizeJ - 1) * fScaleH;

  COLOR colText = SE_COL_GREEN_LIGHT | 255;

  PIX pixPaddingI = 6 * fScaleW;
  PIX pixPaddingJ = 6 * fScaleH;
  PIX pixLogoSizeI = 32 * fScaleW;
  PIX pixLogoSizeJ = 32 * fScaleH;
  PIX pixI = pixPaddingI;
  PIX pixJ = pixScreenSizeJ - pixLogoSizeJ - pixCharSizeJ - pixPaddingJ * 2;

  pdp->SetFont(_pfdConsoleFont);
  pdp->SetTextScaling(fScaleH);
  pdp->SetTextAspect(1.0f);

  pdp->Fill(C_BLACK | 225);

  INDEX iLastFrame = 12;
  CTextureObject *ptoFrame = &atoIcons[0];

  if (pphi != NULL)
  {
    INDEX iFrame = pphi->phi_fCompleted * iLastFrame;
    ptoFrame = &atoIcons[iFrame];
  }

  pdp->PutTexture(ptoFrame, PIXaabbox2D(PIX2D(pixI, pixJ), PIX2D(pixI + pixLogoSizeI, pixJ + pixLogoSizeJ)));

  pixI = pixPaddingI;
  pixJ = pixScreenSizeJ - pixCharSizeJ - pixPaddingJ;

  pdp->PutText(TRANS("Loading"), pixI, pixJ, colText);

  // free textures used in map rendering
  ReleaseMapData();
}
