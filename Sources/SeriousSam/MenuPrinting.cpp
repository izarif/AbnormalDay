/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#include "SeriousSam/StdH.h"

#include "MenuPrinting.h"

static const FLOAT _fBigStartJ = 0.25f;
static const FLOAT _fBigSizeJ = 0.066f;
static const FLOAT _fMediumSizeJ = 0.04f;

static const FLOAT _fNoStartI = 0.25f;
static const FLOAT _fNoSizeI = 0.04f;
static const FLOAT _fNoSpaceI = 0.01f;
static const FLOAT _fNoUpStartJ = 0.24f;
static const FLOAT _fNoDownStartJ = 0.44f;
static const FLOAT _fNoSizeJ = 0.04f;

static const FLOAT fPadding = 0.01f;
static const FLOAT fTitleSizeJ = 0.082f;

FLOATaabbox2D BoxTitle(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.01f, _fBigStartJ + fRow * fTitleSizeJ),
    FLOAT2D(0.99f, _fBigStartJ + (fRow + 1) * fTitleSizeJ));
}

FLOATaabbox2D BoxNoUp(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI), _fNoUpStartJ),
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI)+_fNoSizeI, _fNoUpStartJ+_fNoSizeJ));
}
FLOATaabbox2D BoxNoDown(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI), _fNoDownStartJ),
    FLOAT2D(_fNoStartI+fRow*(_fNoSizeI+_fNoSpaceI)+_fNoSizeI, _fNoDownStartJ+_fNoSizeJ));
}
FLOATaabbox2D BoxBigRow(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.1f, _fBigStartJ+fRow*_fBigSizeJ),
    FLOAT2D(0.9f, _fBigStartJ+(fRow+1)*_fBigSizeJ));
}

FLOATaabbox2D BoxBigLeft(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(fPadding, _fBigStartJ + fRow * _fBigSizeJ),
    FLOAT2D(1 - fPadding, _fBigStartJ + (fRow + 1) * _fBigSizeJ));
}

FLOATaabbox2D BoxBigRight(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.55f, _fBigStartJ+fRow*_fBigSizeJ),
    FLOAT2D(0.9f, _fBigStartJ+(fRow+1)*_fBigSizeJ));
}

FLOATaabbox2D BoxSaveLoad(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(fPadding, _fBigStartJ + fRow * _fMediumSizeJ),
    FLOAT2D(1 - fPadding, _fBigStartJ + (fRow + 1) * _fMediumSizeJ));
}

FLOATaabbox2D BoxVersion(void)
{
  return FLOATaabbox2D(
    FLOAT2D(fPadding, _fBigStartJ + -5.9f * _fMediumSizeJ),
    FLOAT2D(1 - fPadding, _fBigStartJ + (-5.9f + 1) * _fMediumSizeJ));
}

FLOATaabbox2D BoxMediumRow(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.05f, _fBigStartJ+fRow*_fMediumSizeJ),
    FLOAT2D(0.95f, _fBigStartJ+(fRow+1)*_fMediumSizeJ));
}
FLOATaabbox2D BoxKeyRow(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.15f, _fBigStartJ+fRow*_fMediumSizeJ),
    FLOAT2D(0.85f, _fBigStartJ+(fRow+1)*_fMediumSizeJ));
}
FLOATaabbox2D BoxMediumLeft(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(fPadding, _fBigStartJ + fRow * _fMediumSizeJ),
    FLOAT2D(0.5f - fPadding, _fBigStartJ + (fRow + 1) * _fMediumSizeJ));
}

FLOATaabbox2D BoxPlayerSwitch(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.05f, _fBigStartJ+fRow*_fMediumSizeJ),
    FLOAT2D(0.65f, _fBigStartJ+(fRow+1)*_fMediumSizeJ));
}
FLOATaabbox2D BoxMediumMiddle(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI, _fBigStartJ+fRow*_fMediumSizeJ),
    FLOAT2D(0.95f, _fBigStartJ+(fRow+1)*_fMediumSizeJ));
}
FLOATaabbox2D BoxPlayerEdit(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(_fNoStartI, _fBigStartJ+fRow*_fMediumSizeJ),
    FLOAT2D(0.65f, _fBigStartJ+(fRow+1)*_fMediumSizeJ));
}
FLOATaabbox2D BoxMediumRight(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.55f, _fBigStartJ+fRow*_fMediumSizeJ),
    FLOAT2D(0.95f, _fBigStartJ+(fRow+1)*_fMediumSizeJ));
}

FLOATaabbox2D BoxPopup(void)
{
  return FLOATaabbox2D(FLOAT2D(fPadding, 0.838f - fPadding), FLOAT2D(0.6f + fPadding, 1 - fPadding));
}

FLOATaabbox2D BoxPopupLabel(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, 0.841f),
    FLOAT2D(0.599f, 0.841f + _fBigSizeJ));
}

FLOATaabbox2D BoxPopupYesLarge(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, 0.92f),
    FLOAT2D(0.20f, 0.92f + _fBigSizeJ));
}

FLOATaabbox2D BoxPopupNoLarge(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.21f, 0.92f),
    FLOAT2D(0.39f, 0.92f + _fBigSizeJ));
}

FLOATaabbox2D BoxPopupYesSmall(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, 0.92f),
    FLOAT2D(0.20f, 0.92f + _fMediumSizeJ));
}

FLOATaabbox2D BoxPopupNoSmall(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.21f, 0.92f),
    FLOAT2D(0.39f, 0.92f + _fMediumSizeJ));
}

FLOATaabbox2D BoxChangePlayer(INDEX iTable, INDEX iButton)
{
  return FLOATaabbox2D(
    FLOAT2D(0.5f+0.15f*(iButton-1), _fBigStartJ+_fMediumSizeJ*2.0f*iTable),
    FLOAT2D(0.5f+0.15f*(iButton+0), _fBigStartJ+_fMediumSizeJ*2.0f*(iTable+1)));
}

FLOATaabbox2D BoxInfoTable(INDEX iTable)
{
  switch(iTable) {
  case 0:
  case 1:
  case 2:
  case 3:
    return FLOATaabbox2D(
      FLOAT2D(0.1f, _fBigStartJ+_fMediumSizeJ*2.0f*iTable),
      FLOAT2D(0.5f, _fBigStartJ+_fMediumSizeJ*2.0f*(iTable+1)));
  default:
    ASSERT(FALSE);
  case -1:  // single player table
  return FLOATaabbox2D(
    FLOAT2D(0.1f, 1-0.2f-_fMediumSizeJ*2.0f),
    FLOAT2D(0.5f, 1-0.2f));
  }
}

FLOATaabbox2D BoxArrow(enum ArrowDir ad)
{
  switch(ad) {
  default:
    ASSERT(FALSE);
  case AD_UP:
    return FLOATaabbox2D(
      FLOAT2D(fPadding, _fBigStartJ + 3.7f * _fMediumSizeJ),
      FLOAT2D(1 - fPadding, _fBigStartJ + (3.7f + 1) * _fMediumSizeJ));
  case AD_DOWN:
    return FLOATaabbox2D(
      FLOAT2D(fPadding, _fBigStartJ + 14.7f * _fMediumSizeJ),
      FLOAT2D(1 - fPadding, _fBigStartJ + (14.7f + 1) * _fMediumSizeJ));
  }
}

FLOATaabbox2D BoxBack(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, 0.95f),
    FLOAT2D(0.15f, 1.0f));
}

FLOATaabbox2D BoxNext(void)
{
  return FLOATaabbox2D(
    FLOAT2D(0.85f, 0.95f),
    FLOAT2D(0.98f, 1.0f));
}

FLOATaabbox2D BoxLeftColumn(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(0.02f, _fBigStartJ+fRow*_fMediumSizeJ),
    FLOAT2D(0.15f, _fBigStartJ+(fRow+1)*_fMediumSizeJ));
}
FLOATaabbox2D BoxPlayerModel(void)
{
  extern INDEX sam_bWideScreen;
  if (!sam_bWideScreen) {
    return FLOATaabbox2D(FLOAT2D(0.68f, 0.235f), FLOAT2D(0.965f, 0.78f));
  } else {
    return FLOATaabbox2D(FLOAT2D(0.68f, 0.235f), FLOAT2D(0.68f+(0.965f-0.68f)*9.0f/12.0f, 0.78f));
  }
}
FLOATaabbox2D BoxPlayerModelName(void)
{
  return FLOATaabbox2D(FLOAT2D(0.68f, 0.78f), FLOAT2D(0.965f, 0.82f));
}
PIXaabbox2D FloatBoxToPixBox(const CDrawPort *pdp, const FLOATaabbox2D &boxF)
{
  PIX pixW = pdp->GetWidth();
  PIX pixH = pdp->GetHeight();
  return PIXaabbox2D(
    PIX2D(boxF.Min()(1)*pixW, boxF.Min()(2)*pixH),
    PIX2D(boxF.Max()(1)*pixW, boxF.Max()(2)*pixH));
}

FLOATaabbox2D PixBoxToFloatBox(const CDrawPort *pdp, const PIXaabbox2D &boxP)
{
  FLOAT fpixW = pdp->GetWidth();
  FLOAT fpixH = pdp->GetHeight();
  return FLOATaabbox2D(
    FLOAT2D(boxP.Min()(1)/fpixW, boxP.Min()(2)/fpixH),
    FLOAT2D(boxP.Max()(1)/fpixW, boxP.Max()(2)/fpixH));
}

extern CFontData _fdTitle;
void SetFontTitle(CDrawPort *pdp)
{
  pdp->SetFont( &_fdTitle);
  pdp->SetTextScaling(1.25f * pdp->GetHeight() / 480.0f);
  pdp->SetTextAspect(1.0f);
}
extern CFontData _fdBig;
void SetFontBig(CDrawPort *pdp)
{
  pdp->SetFont( &_fdBig);
  pdp->SetTextScaling(1.0f * pdp->GetHeight() / 480.0f);
  pdp->SetTextAspect(1.0f);
}
extern CFontData _fdMedium;
void SetFontMedium(CDrawPort *pdp)
{
  pdp->SetFont( &_fdMedium);
  pdp->SetTextScaling(1.0f * pdp->GetHeight() / 480.0f);
  pdp->SetTextAspect(0.75f);
}
void SetFontSmall(CDrawPort *pdp)
{
  pdp->SetFont( _pfdConsoleFont);
  pdp->SetTextScaling(1.0f * pdp->GetHeight() / 480.0f);
  pdp->SetTextAspect(1.0f);
}

FLOATaabbox2D BoxKeyLeft(FLOAT fRow)
{
  return FLOATaabbox2D(
    FLOAT2D(fPadding, _fBigStartJ + fRow * _fMediumSizeJ),
    FLOAT2D(1 - fPadding, _fBigStartJ + (fRow + 1) * _fMediumSizeJ));
}