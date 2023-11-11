/* Copyright (c) 2023 Dreamy Cecil
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

#ifndef SE_INCL_GFX_NULL_H
#define SE_INCL_GFX_NULL_H

#include <Engine/Graphics/GfxInterface.h>

class IGfxNull : public IGfxInterface
{
  public:
    // Unique graphical API type
    GfxAPIType GetType(void) const override {
      return GAT_NONE;
    };

  // Graphical API methods
  public:

    // Enable operations
    virtual void EnableDepthWrite(void) { NOTHING; };
    virtual void EnableDepthBias(void) { NOTHING; };
    virtual void EnableDepthTest(void) { NOTHING; };
    virtual void EnableAlphaTest(void) { NOTHING; };
    virtual void EnableBlend(void) { NOTHING; };
    virtual void EnableDither(void) { NOTHING; };
    virtual void EnableTexture(void) { NOTHING; };
    virtual void EnableClipping(void) { NOTHING; };
    virtual void EnableClipPlane(void) { NOTHING; };

    // Disable operations
    virtual void DisableDepthWrite(void) { NOTHING; };
    virtual void DisableDepthBias(void) { NOTHING; };
    virtual void DisableDepthTest(void) { NOTHING; };
    virtual void DisableAlphaTest(void) { NOTHING; };
    virtual void DisableBlend(void) { NOTHING; };
    virtual void DisableDither(void) { NOTHING; };
    virtual void DisableTexture(void) { NOTHING; };
    virtual void DisableClipping(void) { NOTHING; };
    virtual void DisableClipPlane(void) { NOTHING; };

    // Set blending operations
    virtual void BlendFunc(GfxBlend eSrc, GfxBlend eDst) { NOTHING; };

    // Set depth buffer compare mode
    virtual void DepthFunc(GfxComp eFunc) { NOTHING; };

    // Set depth buffer range
    virtual void DepthRange(FLOAT fMin, FLOAT fMax) { NOTHING; };

    // Color mask control (use CT_RMASK, CT_GMASK, CT_BMASK, CT_AMASK to enable specific channels)
    virtual void SetColorMask(ULONG ulColorMask) { NOTHING; };

  // Projections
  public:

    // Set face culling
    virtual void CullFace(GfxFace eFace) { NOTHING; };
    virtual void FrontFace(GfxFace eFace) { NOTHING; };

    // Set custom clip plane (if NULL, disable it)
    virtual void ClipPlane(const DOUBLE *pdPlane) { NOTHING; };

    // Set orthographic matrix
    virtual void SetOrtho(const FLOAT fLeft, const FLOAT fRight, const FLOAT fTop,  const FLOAT fBottom, const FLOAT fNear, const FLOAT fFar, const BOOL bSubPixelAdjust) { NOTHING; };

    // Set frustrum matrix
    virtual void SetFrustum(const FLOAT fLeft, const FLOAT fRight, const FLOAT fTop,  const FLOAT fBottom, const FLOAT fNear, const FLOAT fFar) { NOTHING; };

    // Set view matrix
    virtual void SetViewMatrix(const FLOAT *pfMatrix) { NOTHING; };

    // Set texture matrix
    virtual void SetTextureMatrix(const FLOAT *pfMatrix) { NOTHING; };

    // Polygon mode (point, line or fill)
    virtual void PolygonMode(GfxPolyMode ePolyMode) { NOTHING; };

  // Textures
  public:

    // Set texture wrapping mode
    virtual void SetTextureWrapping(enum GfxWrap eWrapU, enum GfxWrap eWrapV) { NOTHING; };

    // Set texture modulation mode (1X or 2X)
    virtual void SetTextureModulation(INDEX iScale) { NOTHING; };

    // Generate texture for API
    virtual void GenerateTexture(ULONG &ulTexObject) { NOTHING; };

    // Unbind texture from API
    virtual void DeleteTexture(ULONG &ulTexObject) { NOTHING; };

  // Vertex arrays
  public:

    // Prepare vertex array for API
    virtual void SetVertexArray(void *pvtx, INDEX ctVtx) { NOTHING; };

    // Prepare normal array for API
    virtual void SetNormalArray(GFXNormal *pnor) { NOTHING; };

    // Prepare UV array for API
    virtual void SetTexCoordArray(GFXTexCoord *ptex, BOOL b4) { NOTHING; }; // b4 = projective mapping (4 FLOATs)

    // Prepare color array for API
    virtual void SetColorArray(GFXColor *pcol) { NOTHING; };

    // Draw prepared arrays
    virtual void DrawElements(INDEX ctElem, INDEX *pidx) { NOTHING; };

    // Set constant color for subsequent rendering (until first SetColorArray() call!)
    virtual void SetConstantColor(COLOR col) { NOTHING; };

    // Color array usage control
    virtual void EnableColorArray(void) { NOTHING; };
    virtual void DisableColorArray(void) { NOTHING; };

  // Miscellaneous
  public:

    // Force finish of rendering queue
    virtual void Finish(void) { NOTHING; };

    // Compiled vertex array control
    virtual void LockArrays(void) { NOTHING; };

    // Toggle truform
    virtual void EnableTruform(void) { NOTHING; };
    virtual void DisableTruform(void) { NOTHING; };
};

#endif // include-once check