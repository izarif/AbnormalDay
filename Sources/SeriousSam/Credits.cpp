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

#include "SeriousSam/StdH.h"
#include <Engine/CurrentVersion.h>
#include "Credits.h"


static CStaticStackArray<CTString> _astrCredits;
static BOOL _bCreditsOn = FALSE;

static PIX pixScreenSizeI = 0;
static PIX pixScreenSizeJ = 0;
static PIX pixJ = 0;
static PIX pixLineHeight;

static CTString strEmpty;
static FLOAT _fSpeed = 2.0f;

static BOOL _bUseRealTime = FALSE;
static CTimerValue _tvStart;

FLOAT fCreditsStartTime;

static FLOAT fScaleW;
static FLOAT fScaleH;

FLOAT GetTime(void)
{
  if(!_bUseRealTime) {
    return  _pTimer->GetLerpedCurrentTick() - fCreditsStartTime;
  } else {
    return (_pTimer->GetHighPrecisionTimer()-_tvStart).GetSeconds();;
  }
}

void PrintOneLine(CDrawPort *pdp, const CTString &strText) 
{
  COLOR col = _pGame->LCDGetColor(C_WHITE, "credits line");

  pdp->SetTextScaling(fScaleH);
  pdp->SetTextAspect( 1.0f);
  pdp->PutText(strText, 6.0f * fScaleW, pixJ, col | 255);

  pixJ+=pixLineHeight;
}

static void LoadOneFile(const CTFileName &fnm)
{
  try {
    // open the file
    CTFileStream strm;
    strm.Open_t(fnm);

    // count number of lines
    INDEX ctLines = 0;
    while(!strm.AtEOF()) {
      CTString strLine;
      strm.GetLine_t(strLine);
      ctLines++;
    }
    strm.SetPos_t(0);

    // allocate that much
    CTString *astr = _astrCredits.Push(ctLines);
    // load all lines
    for(INDEX iLine = 0; iLine<ctLines && !strm.AtEOF(); iLine++) {
      strm.GetLine_t(astr[iLine]);
    }

    strm.Close();

    _bCreditsOn = TRUE;
  } catch (const char *strError) {
    CPrintF("%s\n", (const char *)strError);
  }
}

// turn credits on
void Credits_On(INDEX iType)
{
  if (_bCreditsOn) {
    Credits_Off();
  }

  _astrCredits.PopAll();
  
  if (iType==1) {
    _fSpeed = 1.0f;
    LoadOneFile(CTFILENAME("Data\\Intro.txt"));
  } else if (iType==2) {
    _fSpeed = 2.0f;
    LoadOneFile(CTFILENAME("Data\\Credits.txt"));
    LoadOneFile(CTFILENAME("Data\\Credits_End.txt"));
  } else {
    _fSpeed = 2.0f;
    LoadOneFile(CTFILENAME("Data\\Credits.txt"));
  }
  // if some file was loaded
  if (_bCreditsOn) {
    // remember start time
    if (iType==1 || iType==2) {
      _bUseRealTime = FALSE;
      fCreditsStartTime = _pTimer->GetLerpedCurrentTick();
    } else {
      _bUseRealTime = TRUE;
      _tvStart = _pTimer->GetHighPrecisionTimer();
    }
  }
}

// turn credits off
void Credits_Off(void)
{
  if (!_bCreditsOn) {
    return;
  }
  _bCreditsOn = FALSE;
  _astrCredits.Clear();
}

FLOAT fCreditsTime;

// render credits to given drawport
FLOAT Credits_Render(CDrawPort *pdp)
{
  if (!_bCreditsOn) {
    return 0;
  }

  CDrawPort dpCredits(pdp, TRUE);

  pdp->Unlock();
  dpCredits.Lock();

  fCreditsTime = GetTime();

  pixScreenSizeI = dpCredits.GetWidth();
  pixScreenSizeJ = dpCredits.GetHeight();

  fScaleW = pixScreenSizeI / 640.0f;
  fScaleH = pixScreenSizeJ / 480.0f;

  dpCredits.SetFont( _pfdDisplayFont);
  pixLineHeight = 20 * fScaleH;

  const FLOAT fLinesPerSecond = _fSpeed;
  FLOAT fOffset = fCreditsTime * fLinesPerSecond;
  INDEX ctLinesOnScreen = pixScreenSizeJ / pixLineHeight;
  INDEX iLine1 = (INDEX) fOffset;

  pixJ = (PIX) (iLine1*pixLineHeight-fOffset*pixLineHeight);
  iLine1-=ctLinesOnScreen;

  INDEX ctLines = _astrCredits.Count();
  BOOL bOver = TRUE;

  for (INDEX i = iLine1; i<iLine1+ctLinesOnScreen+1; i++) {
    CTString *pstr = &strEmpty;
    INDEX iLine = i;
    if (iLine>=0 && iLine<ctLines) {
      pstr = &_astrCredits[iLine];
      bOver = FALSE;
    }

    PrintOneLine(&dpCredits, *pstr);
  }

  PIX pixBarJ = 60 * fScaleH;
  dpCredits.Fill(0, pixScreenSizeJ - pixBarJ, pixScreenSizeI, pixBarJ, C_BLACK | 255);

  dpCredits.Unlock();
  pdp->Lock();

  if (bOver) {
    return 0;
  } else if (ctLines-iLine1<ctLinesOnScreen) {
    return FLOAT(ctLines-iLine1)/ctLinesOnScreen;
  } else {
    return 1;
  }
}
