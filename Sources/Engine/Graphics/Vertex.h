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

#ifndef SE_INCL_VERTEX_H
#define SE_INCL_VERTEX_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif


struct GFXVertex3
{
  FLOAT x,y,z;
};


struct GFXNormal3
{
  FLOAT nx,ny,nz;
};


struct GFXTexCoord
{
  union {
    struct { FLOAT u,v; };
    struct { FLOAT s,t; };
  };
};


struct GFXTexCoord4
{
  FLOAT s,t,r,q;
};


struct GFXColor
{
  union {
    struct { UBYTE r,g,b,a; };
    struct { ULONG abgr;    };  // reverse order - use ByteSwap32()!
  };

  GFXColor() : abgr(0) {};

  GFXColor( COLOR col) {
    abgr = ByteSwap32(col);
  }

  __forceinline void Set( COLOR col) {
    abgr = ByteSwap32(col);
  }

  void MultiplyRGBA( const GFXColor &col1, const GFXColor &col2) {
    r = UBYTE((ULONG(col1.r) * col2.r) >> 8);
    g = UBYTE((ULONG(col1.g) * col2.g) >> 8);
    b = UBYTE((ULONG(col1.b) * col2.b) >> 8);
    a = UBYTE((ULONG(col1.a) * col2.a) >> 8);
  }

  void MultiplyRGB( const GFXColor &col1, const GFXColor &col2) {
    r = UBYTE((ULONG(col1.r) * col2.r) >> 8);
    g = UBYTE((ULONG(col1.g) * col2.g) >> 8);
    b = UBYTE((ULONG(col1.b) * col2.b) >> 8);
  }

  void MultiplyRGBCopyA1( const GFXColor &col1, const GFXColor &col2) {
    r = UBYTE((ULONG(col1.r) * col2.r) >> 8);
    g = UBYTE((ULONG(col1.g) * col2.g) >> 8);
    b = UBYTE((ULONG(col1.b) * col2.b) >> 8);
    a = col1.a;
  }

  void AttenuateRGB( ULONG ulA) {
    r = UBYTE((ULONG(r) * ulA) >> 8);
    g = UBYTE((ULONG(g) * ulA) >> 8);
    b = UBYTE((ULONG(b) * ulA) >> 8);
  }

  void AttenuateA( ULONG ulA) {
    a = UBYTE((ULONG(a) * ulA) >> 8);
  }
};


#define GFXVertex GFXVertex4
struct GFXVertex4
{
  GFXVertex4()
  {
  }
  FLOAT x,y,z;

  union {
  #if SE1_WIN
    struct { GFXColor col; };
  #else
    GFXColor col;
  #endif

    SLONG shade;
  };
};


#define GFXNormal GFXNormal4
struct GFXNormal4
{
  FLOAT nx,ny,nz;
  ULONG ul;
};



#endif  /* include-once check. */

