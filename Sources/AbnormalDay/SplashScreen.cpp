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

#include "SplashScreen.h"

#define SPLASH_TITLE "Abnormal Day loading..."

static OS::Window _window = NULL;

#if SE1_PREFER_SDL

static SDL_Renderer *_pRenderer = NULL;
static SDL_Texture *_pTexture = NULL;

void ShowSplashScreen(HINSTANCE hInstance)
{
  // Get splash image
  SDL_Surface *surSplash = SDL_LoadBMP("Splash.bmp");
  if (surSplash == NULL) return;

  // Get mask image for it
  SDL_Surface *surMask = SDL_LoadBMP("SplashMask.bmp");

  if (surMask != NULL) {
    // Mismatching sizes
    if (surMask->w != surSplash->w || surMask->h != surSplash->h) {
      SDL_FreeSurface(surMask);
      SDL_FreeSurface(surSplash);
      return;
    }

    _window = SDL_CreateShapedWindow(SPLASH_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     surSplash->w, surSplash->h, SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR);

    if (_window != NULL) {
      SDL_WindowShapeMode mode;
      SDL_zero(mode);
      mode.mode = ShapeModeColorKey;

      // Apply shape mask to the window
      SDL_Color &col = mode.parameters.colorKey;
      SDL_GetRGBA(SDL_MapRGB(surMask->format, 0xFF, 0xFF, 0xFF), surMask->format, &col.r, &col.g, &col.b, &col.a);

      if (SDL_SetWindowShape(_window, surMask, &mode) != 0) {
        _window.Destroy();
      }
    }

    SDL_FreeSurface(surMask);

  } else {
    // Simple splash without a mask
    _window = SDL_CreateWindow(SPLASH_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               surSplash->w, surSplash->h, SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR);
  }

  BOOL bSuccess = FALSE;

  if (_window != NULL) {
    _pRenderer = SDL_CreateRenderer(_window, -1, 0);

    if (_pRenderer != NULL) {
      SDL_SetRenderDrawColor(_pRenderer, 0, 0, 0, 0);
      SDL_RenderClear(_pRenderer);
      SDL_RenderPresent(_pRenderer);

      _pTexture = SDL_CreateTextureFromSurface(_pRenderer, surSplash);

      bSuccess = (_pTexture != NULL);
    }
  }

  SDL_FreeSurface(surSplash);

  // Render the final splash screen or discard it
  if (bSuccess) {
    SDL_RenderCopy(_pRenderer, _pTexture, NULL, NULL);
    SDL_RenderPresent(_pRenderer);

  } else {
    HideSplashScreen();
  }
};

void HideSplashScreen(void)
{
  // Cleanup
  if (_pTexture != NULL) {
    SDL_DestroyTexture(_pTexture);
    _pTexture = NULL;
  }

  if (_pRenderer != NULL) {
    SDL_DestroyRenderer(_pRenderer);
    _pRenderer = NULL;
  }

  _window.Destroy();
};

#else

#include "resource.h"

#define NAME "Splash"

static HBITMAP _hbmSplash = NULL;
static BITMAP _bmSplash;
static HBITMAP _hbmSplashMask = NULL;
static BITMAP _bmSplashMask;

static LRESULT FAR PASCAL SplashWindowProc( HWND hWnd, UINT message, 
			    WPARAM wParam, LPARAM lParam )
{
  switch( message ) {
  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(hWnd, &ps); 

    HDC hdcMem = CreateCompatibleDC(ps.hdc); 
    SelectObject(hdcMem, _hbmSplashMask); 
    BitBlt(ps.hdc, 0, 0, _bmSplash.bmWidth, _bmSplash.bmHeight, hdcMem, 0, 0, 
      SRCAND); 
    SelectObject(hdcMem, _hbmSplash); 
    BitBlt(ps.hdc, 0, 0, _bmSplash.bmWidth, _bmSplash.bmHeight, hdcMem, 0, 0, 
      SRCPAINT); 

    DeleteDC(hdcMem); 
    EndPaint(hWnd, &ps); 
   
    return 0;
                 } break;
  case WM_ERASEBKGND: return 1; break;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

void ShowSplashScreen(HINSTANCE hInstance)
{
  _hbmSplash = LoadBitmapA(hInstance, (char*)IDB_SPLASH);
  if (_hbmSplash==NULL) {
    return;
  }
  _hbmSplashMask = LoadBitmapA(hInstance, (char*)IDB_SPLASHMASK);
  if (_hbmSplashMask==NULL) {
    return;
  }

  GetObject(_hbmSplash, sizeof(BITMAP), (LPSTR) &_bmSplash); 
  GetObject(_hbmSplashMask, sizeof(BITMAP), (LPSTR) &_bmSplashMask);
  if (_bmSplashMask.bmWidth  != _bmSplash.bmWidth
    ||_bmSplashMask.bmHeight != _bmSplash.bmHeight) {
    return;
  }

	int iScreenX = ::GetSystemMetrics(SM_CXSCREEN);	// screen size
	int iScreenY = ::GetSystemMetrics(SM_CYSCREEN);

  WNDCLASSA wc;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = SplashWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon( hInstance, (LPCTSTR)IDR_MAINFRAME );
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NAME;
  wc.lpszClassName = NAME;
  RegisterClassA(&wc);

  /*
   * create a window
   */
  _window = CreateWindowExA(
	  WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW,
	  NAME,
	  SPLASH_TITLE,   // title
    WS_POPUP,
	  iScreenX/2-_bmSplash.bmWidth/2,
	  iScreenY/2-_bmSplash.bmHeight/2,
	  _bmSplash.bmWidth,_bmSplash.bmHeight,  // window size
	  NULL,
	  NULL,
	  hInstance,
	  NULL);

  if (_window == NULL) {
	  return;
  }
 
  ShowWindow(_window, SW_SHOW);
  RECT rect;
  GetClientRect(_window, &rect); 
  InvalidateRect(_window, &rect, TRUE); 
  UpdateWindow(_window); 
}

void HideSplashScreen(void)
{
  if (_window == NULL) {
    return;
  }
  _window.Destroy();
  DeleteObject(_hbmSplash);
  DeleteObject(_hbmSplashMask);
}

#endif // SE1_PREFER_SDL
