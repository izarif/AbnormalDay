/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#include "SeriousSam/StdH.h"
#include <Engine/Build.h>
#ifndef PLATFORM_FREEBSD
#include <sys/timeb.h>
#endif
#include <time.h>

#ifdef PLATFORM_WIN32
#include <io.h>
#endif

#include "MainWindow.h"
#include <Engine/CurrentVersion.h>
#include <Engine/Templates/Stock_CSoundData.h>
#include <GameMP/LCDDrawing.h>
#include "MenuPrinting.h"
#include "LevelInfo.h"
#include "VarList.h"
#include "Credits.h"

// macros for translating radio button text arrays
#define RADIOTRANS(str) ("ETRS" str)
#define TRANSLATERADIOARRAY(array) TranslateRadioTexts(array, ARRAYCOUNT(array))

extern CMenuGadget *_pmgLastActivatedGadget;
extern BOOL bMenuActive;
extern BOOL bMenuRendering;
extern CTextureObject *_ptoLogoCT;
extern CTextureObject *_ptoLogoODI;
extern CTextureObject *_ptoLogoEAX;

INDEX _iLocalPlayer = -1;
BOOL  _bPlayerMenuFromSinglePlayer = FALSE;


GameMode _gmMenuGameMode = GM_NONE;
GameMode _gmRunningGameMode = GM_NONE;
CListHead _lhServers;

static INDEX sam_old_bFullScreenActive;
static INDEX sam_old_bBorderLessActive;
static INDEX sam_old_iAspectSizeI;
static INDEX sam_old_iAspectSizeJ;
static INDEX sam_old_iScreenSizeI;
static INDEX sam_old_iScreenSizeJ;
static INDEX sam_old_iDisplayDepth;
static INDEX sam_old_iDisplayAdapter;
static INDEX sam_old_iGfxAPI;
static INDEX sam_old_iVideoSetup;  // 0==speed, 1==normal, 2==quality, 3==custom

ENGINE_API extern INDEX snd_iFormat;

extern BOOL IsCDInDrive(void);

void OnPlayerSelect(void);



ULONG SpawnFlagsForGameType(INDEX iGameType)
{
  if (iGameType==-1) return SPF_SINGLEPLAYER;

  // get function that will provide us the flags
  CShellSymbol *pss = _pShell->GetSymbol("GetSpawnFlagsForGameTypeSS", /*bDeclaredOnly=*/ TRUE);
  // if none
  if (pss==NULL) {
    // error
    ASSERT(FALSE);
    return 0;
  }

  ULONG (*pFunc)(INDEX) = (ULONG (*)(INDEX))pss->ss_pvValue;
  return pFunc(iGameType);
}

BOOL IsMenuEnabled(const CTString &strMenuName)
{
  // get function that will provide us the flags
  CShellSymbol *pss = _pShell->GetSymbol("IsMenuEnabledSS", /*bDeclaredOnly=*/ TRUE);
  // if none
  if (pss==NULL) {
    // error
    ASSERT(FALSE);
    return TRUE;
  }

  BOOL (*pFunc)(const CTString &) = (BOOL (*)(const CTString &))pss->ss_pvValue;
  return pFunc(strMenuName);
}

// last tick done
TIME _tmMenuLastTickDone = -1;
// all possible menu entities
CListHead lhMenuEntities;
// controls that are currently customized
CTFileName _fnmControlsToCustomize = CTString("");

static CTString _strLastPlayerAppearance;
extern CTString sam_strNetworkSettings;

// function to activate when level is chosen
void (*_pAfterLevelChosen)(void);

// functions to activate when user chose 'yes/no' on confirmation
void (*_pConfimedYes)(void) = NULL;
void (*_pConfimedNo)(void) = NULL;

void FixupBackButton(CGameMenu *pgm);

void ConfirmYes(void)
{
  if (_pConfimedYes!=NULL) {
    _pConfimedYes();
  }
  void MenuGoToParent(void);
  MenuGoToParent();
}
void ConfirmNo(void)
{
  if (_pConfimedNo!=NULL) {
    _pConfimedNo();
  }
  void MenuGoToParent(void);
  MenuGoToParent();
}


void ControlsMenuOn()
{
  _pGame->SavePlayersAndControls();
  try {
    _pGame->gm_ctrlControlsExtra->Load_t(_fnmControlsToCustomize);
  } catch (const char *strError) {
    WarningMessage(strError);
  }
}

void ControlsMenuOff()
{
  try {
    if (_pGame->gm_ctrlControlsExtra->ctrl_lhButtonActions.Count()>0) {
      _pGame->gm_ctrlControlsExtra->Save_t(_fnmControlsToCustomize);
    }
  } catch (const char *strError) {
    FatalError(strError);
  }

  _pGame->gm_ctrlControlsExtra->DeleteAllButtonActions();

  _pGame->LoadPlayersAndControls();
}

// mouse cursor position
__extern PIX _pixCursorPosI = 0;
__extern PIX _pixCursorPosJ = 0;
__extern PIX _pixCursorExternPosI = 0;
__extern PIX _pixCursorExternPosJ = 0;
__extern BOOL _bMouseUsedLast = FALSE;
__extern CMenuGadget *_pmgUnderCursor =  NULL;
__extern BOOL _bMouseRight = FALSE;

extern BOOL _bDefiningKey;
extern BOOL _bEditingString;

// thumbnail for showing in menu
CTextureObject _toThumbnail;
BOOL _bThumbnailOn = FALSE;

CFontData _fdBig;
CFontData _fdMedium;
CFontData _fdSmall;
CFontData _fdTitle;


CSoundData *_psdSelect = NULL;
CSoundData *_psdPress = NULL;
CSoundObject *_psoMenuSound = NULL;

static CTextureObject _toPointer;
static CTextureObject toMenuLogo;
static CTextureObject _toLogoMenuB;

// -------------- All possible menu entities
#define BIG_BUTTONS_CT 6
#define SAVELOAD_BUTTONS_CT 10

#define TRIGGER_MG(mg, y, up, down, text, astr) \
  mg.mg_pmgUp = &up;\
  mg.mg_pmgDown = &down;\
  mg.mg_boxOnScreen = BoxMediumRow(y);\
  gm_lhGadgets.AddTail( mg.mg_lnNode);\
  mg.mg_astrTexts = astr;\
  mg.mg_ctTexts = sizeof( astr)/sizeof( astr[0]);\
  mg.mg_iSelected = 0;\
  mg.mg_strLabel = text;\
  mg.mg_strValue = astr[0];

#define CHANGETRIGGERARRAY(ltbmg, astr) \
  ltbmg.mg_astrTexts = astr;\
  ltbmg.mg_ctTexts = sizeof( astr)/sizeof( astr[0]);\
  ltbmg.mg_iSelected = 0;\
  ltbmg.mg_strText = astr[ltbmg.mg_iSelected];

#define PLACEMENT(x,y,z) CPlacement3D( FLOAT3D( x, y, z), \
  ANGLE3D( AngleDeg(0.0f), AngleDeg(0.0f), AngleDeg(0.0f)))

CTString astrNoYes[] = {
  RADIOTRANS( "No"),
  RADIOTRANS( "Yes"),
};
CTString astrComputerInvoke[] = {
  RADIOTRANS( "Use"),
  RADIOTRANS( "Double-click use"),
};
CTString astrWeapon[] = {
  RADIOTRANS( "Only if new"),
  RADIOTRANS( "Never"),
  RADIOTRANS( "Always"),
  RADIOTRANS( "Only if stronger"),
};
CTString astrCrosshair[] = {
  "",
  "Textures\\Interface\\Crosshairs\\Crosshair1.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair2.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair3.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair4.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair5.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair6.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair7.tex",
};

CTString astrMaxPlayersRadioTexts[] = {
  RADIOTRANS( "2"),
  RADIOTRANS( "3"),
  RADIOTRANS( "4"),
  RADIOTRANS( "5"),
  RADIOTRANS( "6"),
  RADIOTRANS( "7"),
  RADIOTRANS( "8"),
  RADIOTRANS( "9"),
  RADIOTRANS( "10"),
  RADIOTRANS( "11"),
  RADIOTRANS( "12"),
  RADIOTRANS( "13"),
  RADIOTRANS( "14"),
  RADIOTRANS( "15"),
  RADIOTRANS( "16"),
};
// here, we just reserve space for up to 16 different game types
// actual names are added later
CTString astrGameTypeRadioTexts[] = {
  "", "", "", "", "", 
  "", "", "", "", "", 
  "", "", "", "", "", 
  "", "", "", "", "", 
};
INDEX ctGameTypeRadioTexts = 1;
CTString astrDifficultyRadioTexts[] = {
  RADIOTRANS("Tourist"),
  RADIOTRANS("Easy"),
  RADIOTRANS("Normal"),
  RADIOTRANS("Hard"),
  RADIOTRANS("Serious"),
  RADIOTRANS("Mental"),
};
CTString astrSplitScreenRadioTexts[] = {
  RADIOTRANS( "1"),
  RADIOTRANS( "2 - split screen"),
  RADIOTRANS( "3 - split screen"),
  RADIOTRANS( "4 - split screen"),
};
//CTString astrBorderLessText[] =
//{
//  RADIOTRANS("Normal Window"),
//  RADIOTRANS("BorderLess Window"),
//};
PIX apixAspectRatios[][2] =
{
	4, 3,
	5, 4,
	16, 9,
	16, 10,
	21, 9,
};
PIX apixWidths[][2] = {
   320, 0,
   400, 0,
   512, 0,
   640, 0,
   720, 0,
   800, 0,
   960, 0,
  1024, 0,
  1152, 0,
  1280, 0,
  1366, 0,
  1600, 0,
  1680, 0,
  1920, 0,
  2048, 0,
  2560, 0,
  3440, 0,
  3840, 0,
  5120, 0,
  5760, 0,
  6016, 0,
  7680, 0,
  8192, 0,
  8400, 0,
};

// ptr to current menu
CGameMenu *pgmCurrentMenu = NULL;

// global back button
CMGButton mgBack;

// -------- Confirm menu
CConfirmMenu gmConfirmMenu;
CMGLabel mgConfirmLabel;
CMGButton mgConfirmYes;
CMGButton mgConfirmNo;

// -------- Main menu
CMainMenu gmMainMenu;
CMGLabel mgMainVersion;
CMGButton mgMainNewGame;
CMGButton mgMainLoadGame;
CMGButton mgMainOptions;
CMGButton mgMainCredits;
CMGButton mgMainQuitGame;

// -------- InGame menu
CInGameMenu gmInGameMenu;
CMGTitle mgInGameTitle;
CMGButton mgInGameResumeGame;
CMGButton mgInGameLoadGame;
CMGButton mgInGameSaveGame;
CMGButton mgInGameOptions;
CMGButton mgInGameQuitGame;

// -------- Single player menu
CSinglePlayerMenu gmSinglePlayerMenu;
CMGTitle mgSingleTitle;
CMGButton mgSinglePlayerLabel;
CMGButton mgSingleNewGame;
CMGButton mgSingleCustom;
CMGButton mgSingleQuickLoad;
CMGButton mgSingleLoad;
CMGButton mgSingleTraining;
CMGButton mgSingleTechTest;
CMGButton mgSinglePlayersAndControls;
CMGButton mgSingleOptions;

// -------- New single player menu
CSinglePlayerNewMenu gmSinglePlayerNewMenu;
CMGTitle mgSingleNewTitle;
CMGButton mgSingleNewTourist;
CMGButton mgSingleNewEasy;
CMGButton mgSingleNewMedium;
CMGButton mgSingleNewHard;
CMGButton mgSingleNewSerious;
CMGButton mgSingleNewMental;
// -------- Disabled menu
CDisabledMenu gmDisabledFunction;
CMGTitle mgDisabledTitle;
CMGButton mgDisabledMenuButton;
// -------- Manual levels menu
CLevelsMenu gmLevelsMenu;
CMGTitle mgLevelsTitle;
CMGLevelButton mgManualLevel[ LEVELS_ON_SCREEN];
CMGArrow mgLevelsArrowUp;
CMGArrow mgLevelsArrowDn;

// -------- console variable adjustment menu
BOOL _bVarChanged = FALSE;
CVarMenu gmVarMenu;
CMGTitle mgVarTitle;
CMGVarButton mgVar[LEVELS_ON_SCREEN];
CMGButton mgVarApply;
CMGArrow mgVarArrowUp;
CMGArrow mgVarArrowDn;

// -------- Player profile menu
CPlayerProfileMenu gmPlayerProfile;
CMGTitle mgPlayerProfileTitle;
CMGButton mgPlayerNoLabel;
CMGButton mgPlayerNo[8];
CMGButton mgPlayerNameLabel;
CMGEdit mgPlayerName;
CMGButton mgPlayerTeamLabel;
CMGEdit mgPlayerTeam;
CMGButton mgPlayerCustomizeControls;
CMGTrigger mgPlayerCrosshair;
CMGTrigger mgPlayerWeaponSelect;
CMGTrigger mgPlayerWeaponHide;
CMGTrigger mgPlayer3rdPerson;
CMGTrigger mgPlayerQuotes;
CMGTrigger mgPlayerAutoSave;
CMGTrigger mgPlayerCompDoubleClick;
CMGTrigger mgPlayerViewBobbing;
CMGTrigger mgPlayerSharpTurning;
CMGModel mgPlayerModel;

// -------- Controls menu
CControlsMenu gmControls;
CMGTitle mgControlsTitle;
CMGButton mgControlsButtons;
CMGSlider mgControlsSensitivity;
CMGTrigger mgControlsInvertTrigger;
CMGTrigger mgControlsSmoothTrigger;
CMGTrigger mgControlsAccelTrigger;
CMGTrigger mgControlsIFeelTrigger;
CMGButton mgControlsPredefined;
CMGButton mgControlsAdvanced;
CMGButton mgControlsBack;

// -------- Load/Save menu
CLoadSaveMenu gmLoadSaveMenu;
CMGTitle mgLoadSaveTitle;
CMGFileButton amgLSButton[SAVELOAD_BUTTONS_CT];
CMGArrow mgLSArrowUp;
CMGArrow mgLSArrowDn;
CMGButton mgLSBack;

// -------- High-score menu
CHighScoreMenu gmHighScoreMenu;
CMGTitle mgHighScoreTitle;
CMGHighScore mgHScore;
// -------- Customize keyboard menu
CCustomizeKeyboardMenu gmCustomizeKeyboardMenu;
CMGTitle mgCustomizeKeyboardTitle;
CMGKeyDefinition mgKey[KEYS_ON_SCREEN];
CMGArrow mgCustomizeArrowUp;
CMGArrow mgCustomizeArrowDn;
CMGButton mgCustomizeBack;

// -------- Choose servers menu
CServersMenu gmServersMenu;
CMGTitle mgServersTitle;
CMGServerList mgServerList;
CMGButton mgServerColumn[7];
CMGEdit mgServerFilter[7];
CMGButton mgServerRefresh;

// -------- Customize axis menu
CCustomizeAxisMenu gmCustomizeAxisMenu;
CMGTitle mgCustomizeAxisTitle;
CMGTrigger mgAxisActionTrigger;
CMGTrigger mgAxisMountedTrigger;
CMGSlider mgAxisSensitivity;
CMGSlider mgAxisDeadzone;
CMGTrigger mgAxisInvertTrigger;
CMGTrigger mgAxisRelativeTrigger;
CMGTrigger mgAxisSmoothTrigger;
CMGButton mgAxisBack;

// -------- Options menu
COptionsMenu gmOptionsMenu;
CMGTitle mgOptionsTitle;
CMGButton mgOptionsGame;
CMGButton mgOptionsVideo;
CMGButton mgOptionsAudio;
CMGButton mgOptionsControls;
CMGButton mgOptionsBack;

// -------- Video options menu
CVideoOptionsMenu gmVideoOptionsMenu;
CMGTitle mgVideoOptionsTitle;
CMGTrigger mgDisplayAPITrigger;

CTString astrDisplayAPIRadioTexts[] = {
  RADIOTRANS( "OpenGL"),
#ifdef SE1_D3D
  RADIOTRANS( "Direct3D"),
#endif 

};

CMGTrigger mgDisplayAdaptersTrigger;
CMGTrigger mgFullScreenTrigger;
CMGTrigger mgAspectRatiosTrigger;
CMGTrigger mgBorderLessTrigger;
CMGTrigger mgResolutionsTrigger;
CMGTrigger mgDisplayPrefsTrigger;

CTString astrDisplayPrefsRadioTexts[] = {
  RADIOTRANS( "Speed"),
  RADIOTRANS( "Normal"),
  RADIOTRANS( "Quality"),
  RADIOTRANS( "Custom"),
};

INDEX _ctResolutions = 0;
CTString *_astrResolutionTexts = NULL;
CDisplayMode *_admResolutionModes  = NULL;

INDEX _ctAspectRatios = 0;
CTString *_astrAspectRatioTexts = NULL;
CDisplayMode *_admAspectRatioModes  = NULL;

INDEX _ctAdapters = 0;
CTString *_astrAdapterTexts = NULL;
CMGButton mgVideoRendering;
CMGTrigger mgBitsPerPixelTrigger;

CTString astrBitsPerPixelRadioTexts[] = {
  RADIOTRANS( "Desktop"),
  RADIOTRANS( "16 BPP"),
  RADIOTRANS( "32 BPP"),
};

CMGButton mgVideoOptionsApply;
CMGButton mgVideoOptionsBack;

// -------- Audio options menu
CAudioOptionsMenu gmAudioOptionsMenu;
CMGTitle mgAudioOptionsTitle;
CMGTrigger mgAudioAutoTrigger;
CMGTrigger mgAudioAPITrigger;
CMGTrigger mgFrequencyTrigger;

CTString astrFrequencyRadioTexts[] = {
  RADIOTRANS( "No sound"),
  RADIOTRANS( "11kHz"),
  RADIOTRANS( "22kHz"),
  RADIOTRANS( "44kHz"),
};

CTString astrSoundAPIRadioTexts[] = {
#ifdef PLATFORM_WIN32
  RADIOTRANS( "WaveOut"),
  RADIOTRANS( "DirectSound"),
  RADIOTRANS( "EAX"),
#else
  RADIOTRANS( "Simple Directmedia Layer" ),
#endif
};

CMGSlider mgWaveVolume;
CMGSlider mgMPEGVolume;
CMGButton mgAudioOptionsApply;
CMGButton mgAudioOptionsBack;

// -------- Network menu
CNetworkMenu gmNetworkMenu;
CMGTitle  mgNetworkTitle;
CMGButton mgNetworkJoin;
CMGButton mgNetworkStart;
CMGButton mgNetworkQuickLoad;
CMGButton mgNetworkLoad;

// -------- Network join menu
CNetworkJoinMenu gmNetworkJoinMenu;
CMGTitle  mgNetworkJoinTitle;
CMGButton mgNetworkJoinLAN;
CMGButton mgNetworkJoinNET;
CMGButton mgNetworkJoinOpen;

// -------- Network start menu
CNetworkStartMenu gmNetworkStartMenu;
CMGTitle mgNetworkStartTitle;
CMGEdit mgNetworkSessionName;
CMGTrigger mgNetworkGameType;
CMGTrigger mgNetworkDifficulty;
CMGButton mgNetworkLevel;
CMGTrigger mgNetworkMaxPlayers;
CMGTrigger mgNetworkWaitAllPlayers;
CMGTrigger mgNetworkVisible;
CMGButton mgNetworkGameOptions;
CMGButton mgNetworkStartStart;

// -------- Network open menu
CNetworkOpenMenu gmNetworkOpenMenu;
CMGTitle mgNetworkOpenTitle;
CMGButton mgNetworkOpenAddressLabel;
CMGEdit mgNetworkOpenAddress;
CMGButton mgNetworkOpenPortLabel;
CMGEdit mgNetworkOpenPort;
CMGButton mgNetworkOpenJoin;

// -------- Split screen menu
CSplitScreenMenu gmSplitScreenMenu;
CMGTitle mgSplitScreenTitle;
CMGButton mgSplitScreenStart;
CMGButton mgSplitScreenQuickLoad;
CMGButton mgSplitScreenLoad;

// -------- Split screen start menu
CSplitStartMenu gmSplitStartMenu;
CMGTitle mgSplitStartTitle;
CMGTrigger mgSplitGameType;
CMGTrigger mgSplitDifficulty;
CMGButton mgSplitLevel;
CMGButton mgSplitOptions;
CMGButton mgSplitStartStart;

// -------- Select players menu
CSelectPlayersMenu gmSelectPlayersMenu;
CMGTitle mgSelectPlayerTitle;

CMGTrigger mgDedicated;
CMGTrigger mgObserver;
CMGTrigger mgSplitScreenCfg;

CMGChangePlayer mgPlayer0Change;
CMGChangePlayer mgPlayer1Change;
CMGChangePlayer mgPlayer2Change;
CMGChangePlayer mgPlayer3Change;

CMGButton mgSelectPlayersNotes;

CMGButton mgSelectPlayersStart;

// -------- Game options menu
CGameOptionsMenu gmGameOptionsMenu;
CMGTitle mgGameOptionsTitle;
CMGTrigger mgGameOptions3rdPerson;
CMGTrigger mgGameOptionsAutoSave;
CMGTrigger mgGameOptionsSharpTurning;
CMGTrigger mgGameOptionsViewBobbing;
CMGTrigger mgGameOptionsBlood;
CMGButton mgGameOptionsBack;

CTString astrBloodTexts[] = {
  RADIOTRANS("None"),
  RADIOTRANS("Green"),
  RADIOTRANS("Red"),
  RADIOTRANS("Hippie"),
};

// -------- Rendering options menu
CRenderingOptionsMenu gmRenderingOptionsMenu;
CMGTitle mgRenderingOptionsTitle;
CMGTrigger mgRenderingOptionsTexturesSize;
CMGTrigger mgRenderingOptionsTexturesQuality;
CMGTrigger mgRenderingOptionsBumpMaps;
CMGTrigger mgRenderingOptionsShadowMapsSize;
CMGTrigger mgRenderingOptionsLensFlares;
CMGSlider mgRenderingOptionsGamma;
CMGTrigger mgRenderingOptionsVSync;
CMGButton mgRenderingOptionsApply;
CMGButton mgRenderingOptionsBack;

CTString astrTextureSizeTexts[] = {
  RADIOTRANS("Tiny"),
  RADIOTRANS("Small"),
  RADIOTRANS("Normal"),
  RADIOTRANS("Large"),
  RADIOTRANS("Huge"),
};

FLOAT fTextureSizes[] = {
  6.5f,
  7.0f,
  8.0f,
  8.5f,
  9.0f,
};

CTString astrTextureQualityTexts[] = {
  RADIOTRANS("Optimal"),
  RADIOTRANS("16-bit"),
  RADIOTRANS("32-bit"),
  RADIOTRANS("Compressed"),
};

CTString astrShadowMapSizeTexts[] = {
  RADIOTRANS("Tiny"),
  RADIOTRANS("Small"),
  RADIOTRANS("Normal"),
  RADIOTRANS("Large"),
  RADIOTRANS("Huge"),
};

FLOAT fShadowMapsSizes[] = {
  5.0f,
  6.0f,
  7.0f,
  7.5f,
  8.0f,
};

CTString astrLensFlaresTexts[] = {
  RADIOTRANS("Disabled"),
  RADIOTRANS("Simple"),
  RADIOTRANS("Standart"),
};

FLOAT fGammaValues[] = {
  0.2f,
  0.5f,
  0.8f,
  1.0f,
  1.1f,
  1.3f,
  1.5f,
};

// -------- Credits menu
CCreditsMenu gmCreditsMenu;
CMGButton mgCreditsBack;

extern void PlayMenuSound(CSoundData *psd)
{
  if (_psoMenuSound!=NULL && !_psoMenuSound->IsPlaying()) {
    _psoMenuSound->Play(psd, SOF_NONGAME);
  }
}

CModelObject *AddAttachment_t(CModelObject *pmoParent, INDEX iPosition,
   const CTFileName &fnmModel, INDEX iAnim,
   const CTFileName &fnmTexture,
   const CTFileName &fnmReflection,
   const CTFileName &fnmSpecular)
{
  CAttachmentModelObject *pamo = pmoParent->AddAttachmentModel(iPosition);
  ASSERT(pamo!=NULL);
  pamo->amo_moModelObject.SetData_t(fnmModel);
  pamo->amo_moModelObject.PlayAnim(iAnim, AOF_LOOPING);
  pamo->amo_moModelObject.mo_toTexture.SetData_t(fnmTexture);
  pamo->amo_moModelObject.mo_toReflection.SetData_t(fnmReflection);
  pamo->amo_moModelObject.mo_toSpecular.SetData_t(fnmSpecular);
  return &pamo->amo_moModelObject;
}

void SetPlayerModel(CModelObject *pmoPlayer)
{
/*  try {
    pmoPlayer->SetData_t( CTFILENAME( "Models\\Player\\SeriousSam\\Player.mdl"));
    pmoPlayer->mo_toTexture.SetData_t( CTFILENAME( "Models\\Player\\SeriousSam\\Player.tex"));
    pmoPlayer->PlayAnim(PLAYER_ANIM_WALK, AOF_LOOPING);
    CModelObject *pmoBody = AddAttachment_t(pmoPlayer, PLAYER_ATTACHMENT_TORSO,
      CTFILENAME("Models\\Player\\SeriousSam\\Body.mdl"), BODY_ANIM_MINIGUN_STAND,
      CTFILENAME("Models\\Player\\SeriousSam\\Body.tex"),
      CTFILENAME(""), 
      CTFILENAME(""));
    CModelObject *pmoHead = AddAttachment_t(pmoBody, BODY_ATTACHMENT_HEAD,
      CTFILENAME("Models\\Player\\SeriousSam\\Head.mdl"), 0,
      CTFILENAME("Models\\Player\\SeriousSam\\Head.tex"),
      CTFILENAME(""), 
      CTFILENAME(""));
    CModelObject *pmoMiniGun = AddAttachment_t(pmoBody, BODY_ATTACHMENT_MINIGUN,
      CTFILENAME("Models\\Weapons\\MiniGun\\MiniGunItem.mdl"), 0,
      CTFILENAME("Models\\Weapons\\MiniGun\\MiniGun.tex"),
      CTFILENAME(""), 
      CTFILENAME(""));
    AddAttachment_t(pmoMiniGun, MINIGUNITEM_ATTACHMENT_BARRELS,
      CTFILENAME("Models\\Weapons\\MiniGun\\Barrels.mdl"), 0,
      CTFILENAME("Models\\Weapons\\MiniGun\\MiniGun.tex"),
      CTFILENAME("Models\\ReflectionTextures\\LightBlueMetal01.tex"), 
      CTFILENAME("Models\\SpecularTextures\\Medium.tex"));
    AddAttachment_t(pmoMiniGun, MINIGUNITEM_ATTACHMENT_BODY,
      CTFILENAME("Models\\Weapons\\MiniGun\\Body.mdl"), 0,
      CTFILENAME("Models\\Weapons\\MiniGun\\MiniGun.tex"),
      CTFILENAME("Models\\ReflectionTextures\\LightBlueMetal01.tex"), 
      CTFILENAME("Models\\SpecularTextures\\Medium.tex"));
    AddAttachment_t(pmoMiniGun, MINIGUNITEM_ATTACHMENT_ENGINE,
      CTFILENAME("Models\\Weapons\\MiniGun\\Engine.mdl"), 0,
      CTFILENAME("Models\\Weapons\\MiniGun\\MiniGun.tex"),
      CTFILENAME("Models\\ReflectionTextures\\LightBlueMetal01.tex"), 
      CTFILENAME("Models\\SpecularTextures\\Medium.tex"));

  } catch (const char *strError) { 
    FatalError( strError);
  }     
  */
}

// translate all texts in array for one radio button
void TranslateRadioTexts(CTString astr[], INDEX ct)
{
  for (INDEX i=0; i<ct; i++) {
    astr[i] = TranslateConst(astr[i], 4);
  }
}

// make description for a given resolution
CTString GetResolutionDescription(CDisplayMode &dm)
{
  CTString str;
  // if dual head
  if (dm.IsDualHead()) {
    str.PrintF(TRANSV("%dx%d double"), dm.dm_pixSizeI/2, dm.dm_pixSizeJ);
  // if widescreen
  } else if (dm.IsWideScreen()) {
    str.PrintF(TRANSV("%dx%d wide"), dm.dm_pixSizeI, dm.dm_pixSizeJ);
  // otherwise it is normal
  } else {
    str.PrintF("%dx%d", dm.dm_pixSizeI, dm.dm_pixSizeJ);
  }
  return str;
}

// make description for a given resolution
void SetResolutionInList(INDEX iRes, PIX pixSizeI, PIX pixSizeJ)
{
  ASSERT(iRes>=0 && iRes<_ctResolutions);

  CTString &str    = _astrResolutionTexts[iRes];
  CDisplayMode &dm = _admResolutionModes[iRes];
  dm.dm_pixSizeI = pixSizeI;
  dm.dm_pixSizeJ = pixSizeJ;
  str = GetResolutionDescription(dm);
}

// make description for a given aspect ratio
void SetAspectRatioInList(INDEX iAsp, PIX aspSizeI, PIX aspSizeJ)
{
  CTString &str    = _astrAspectRatioTexts[iAsp];
  CDisplayMode &dm = _admAspectRatioModes[iAsp];
  dm.dm_pixSizeI = aspSizeI;
  dm.dm_pixSizeJ = aspSizeJ;
  str.PrintF("%d:%d", aspSizeI, aspSizeJ);
}

// set new thumbnail
void SetThumbnail(CTFileName fn)
{
  _bThumbnailOn = TRUE;
  try {
    _toThumbnail.SetData_t(fn.NoExt()+"Tbn.tex");
  } catch (const char *strError) {
    (void)strError;
    try {
      _toThumbnail.SetData_t(fn.NoExt()+".tbn");
    } catch (const char *strError) {
      (void)strError;
      _toThumbnail.SetData(NULL);
    }
  }
}

// remove thumbnail
void ClearThumbnail(void)
{
  _bThumbnailOn = FALSE;
  _toThumbnail.SetData(NULL);
  _pShell->Execute( "FreeUnusedStock();");
}

// start load/save menus depending on type of game running

void QuickSaveFromMenu()
{
  _pShell->SetINDEX("gam_bQuickSave", 2); // force save with reporting
  StopMenus(TRUE);
}

void StartCurrentLoadMenu()
{
  if (_gmRunningGameMode==GM_NETWORK) {
    void StartNetworkLoadMenu(void);
    StartNetworkLoadMenu();
  } else if (_gmRunningGameMode==GM_SPLIT_SCREEN) {
    void StartSplitScreenLoadMenu(void);
    StartSplitScreenLoadMenu();
  } else {
    void StartSinglePlayerLoadMenu(void);
    StartSinglePlayerLoadMenu();
  }
}
void StartCurrentSaveMenu()
{
  if (_gmRunningGameMode==GM_NETWORK) {
    void StartNetworkSaveMenu(void);
    StartNetworkSaveMenu();
  } else if (_gmRunningGameMode==GM_SPLIT_SCREEN) {
    void StartSplitScreenSaveMenu(void);
    StartSplitScreenSaveMenu();
  } else {
    void StartSinglePlayerSaveMenu(void);
    StartSinglePlayerSaveMenu();
  }
}
void StartCurrentQuickLoadMenu()
{
  if (_gmRunningGameMode==GM_NETWORK) {
    void StartNetworkQuickLoadMenu(void);
    StartNetworkQuickLoadMenu();
  } else if (_gmRunningGameMode==GM_SPLIT_SCREEN) {
    void StartSplitScreenQuickLoadMenu(void);
    StartSplitScreenQuickLoadMenu();
  } else {
    void StartSinglePlayerQuickLoadMenu(void);
    StartSinglePlayerQuickLoadMenu();
  }
}

void StartMenus(const char *str)
{
  _tmMenuLastTickDone=_pTimer->GetRealTimeTick();
  // disable printing of last lines
  CON_DiscardLastLineTimes();

  // stop all IFeel effects
  IFeel_StopEffect(NULL);
  if (pgmCurrentMenu==&gmMainMenu || pgmCurrentMenu==&gmInGameMenu) {
    if (_gmRunningGameMode==GM_NONE) {
      pgmCurrentMenu = &gmMainMenu;
    } else {
      pgmCurrentMenu = &gmInGameMenu;
    }
  }

  // start main menu, or last active one
  if (pgmCurrentMenu!=NULL) {
    ChangeToMenu(pgmCurrentMenu);
  } else {
    if (_gmRunningGameMode==GM_NONE) {
      ChangeToMenu(&gmMainMenu);
    } else {
      ChangeToMenu(&gmInGameMenu);
    }
  }
  if (CTString(str)=="load") {
    StartCurrentLoadMenu();
    gmLoadSaveMenu.gm_pgmParentMenu = NULL;
  }
  if (CTString(str)=="save") {
    StartCurrentSaveMenu();
    gmLoadSaveMenu.gm_pgmParentMenu = NULL;
  }
  if (CTString(str)=="controls") {
    void StartControlsMenuFromOptions(void);
    StartControlsMenuFromOptions();
    gmControls.gm_pgmParentMenu = NULL;
  }
  if (CTString(str)=="join") {
    void StartSelectPlayersMenuFromOpen(void);
    StartSelectPlayersMenuFromOpen();
    gmSelectPlayersMenu.gm_pgmParentMenu = &gmMainMenu;
  }
  if (CTString(str)=="hiscore") {
    ChangeToMenu( &gmHighScoreMenu);
    gmHighScoreMenu.gm_pgmParentMenu = &gmMainMenu;
  }
  bMenuActive = TRUE;
  bMenuRendering = TRUE;
}


void StopMenus( BOOL bGoToRoot /*=TRUE*/)
{
  ClearThumbnail();
  if (pgmCurrentMenu!=NULL && bMenuActive) {
    pgmCurrentMenu->EndMenu();
  }
  bMenuActive = FALSE;
  if (bGoToRoot) {
    if (_gmRunningGameMode==GM_NONE) {
      pgmCurrentMenu = &gmMainMenu;
    } else {
      pgmCurrentMenu = &gmInGameMenu;
    }
  }

#ifdef PLATFORM_UNIX
  // rcg02042003 hack for SDL vs. Win32.
  if (_pInput != NULL)
    _pInput->ClearRelativeMouseMotion();
#endif
}


BOOL IsMenusInRoot(void)
{
  return pgmCurrentMenu==NULL || pgmCurrentMenu==&gmMainMenu || pgmCurrentMenu==&gmInGameMenu;
}

// ---------------------- When activated functions 
void StartSinglePlayerMenu(void)
{
  ChangeToMenu( &gmSinglePlayerMenu);
}

void ExitGame(void)
{
  _bRunning = FALSE;
  _bQuitScreen = TRUE;
}

CTFileName _fnmModSelected;
CTString _strModURLSelected;
CTString _strModServerSelected;

void ExitAndSpawnExplorer(void)
{
  _bRunning = FALSE;
  _bQuitScreen = FALSE;
  extern CTString _strURLToVisit;
  _strURLToVisit = _strModURLSelected;
}

void ExitConfirm(void)
{
  _pConfimedYes = &ExitGame;
  _pConfimedNo = NULL;
  mgConfirmLabel.mg_strText = TRANS("ARE YOU SERIOUS?");
  gmConfirmMenu.gm_pgmParentMenu = pgmCurrentMenu;
  gmConfirmMenu.BeLarge();
  ChangeToMenu( &gmConfirmMenu);
}

void StopConfirm(void)
{
  extern void StopCurrentGame(void);
  _pConfimedYes = &StopCurrentGame;
  _pConfimedNo = NULL;
  mgConfirmLabel.mg_strText = TRANS("ARE YOU SURE?");
  gmConfirmMenu.gm_pgmParentMenu = pgmCurrentMenu;
  gmConfirmMenu.BeLarge();
  ChangeToMenu( &gmConfirmMenu);
}

void ModConnect(void)
{
  extern CTFileName _fnmModToLoad;
  extern CTString _strModServerJoin;
  _fnmModToLoad = _fnmModSelected;
  _strModServerJoin = _strModServerSelected;
}

void ModConnectConfirm(void)
{
  if (_fnmModSelected==" ") {
    _fnmModSelected = CTString("AbnormalDay");
  }
  CTFileName fnmModPath = "Mods\\"+_fnmModSelected+"\\";
  if (!FileExists(fnmModPath+"BaseWriteInclude.lst")
    &&!FileExists(fnmModPath+"BaseWriteExclude.lst")
    &&!FileExists(fnmModPath+"BaseBrowseInclude.lst")
    &&!FileExists(fnmModPath+"BaseBrowseExclude.lst")) {
    extern void ModNotInstalled(void);
    ModNotInstalled();
    return;
  }

  CPrintF(TRANSV("Server is running a different MOD (%s).\nYou need to reload to connect.\n"), (const char *) _fnmModSelected);
  _pConfimedYes = &ModConnect;
  _pConfimedNo = NULL;
  mgConfirmLabel.mg_strText = TRANS("CHANGE THE MOD?");
  gmConfirmMenu.gm_pgmParentMenu = pgmCurrentMenu;
  gmConfirmMenu.BeLarge();
  ChangeToMenu( &gmConfirmMenu);
}

void SaveConfirm(void)
{
  extern void OnFileSaveOK(void);
  _pConfimedYes = &OnFileSaveOK;
  _pConfimedNo = NULL;
  mgConfirmLabel.mg_strText = TRANS("OVERWRITE?");
  gmConfirmMenu.gm_pgmParentMenu = pgmCurrentMenu;
  gmConfirmMenu.BeLarge();
  ChangeToMenu( &gmConfirmMenu);
}


void ModLoadYes(void)
{
  extern CTFileName _fnmModToLoad;
  _fnmModToLoad = _fnmModSelected;
}

void ModConfirm(void)
{
  _pConfimedYes = &ModLoadYes;
  _pConfimedNo = NULL;
  mgConfirmLabel.mg_strText = TRANS("LOAD THIS MOD?");
  gmConfirmMenu.gm_pgmParentMenu = &gmLoadSaveMenu;
  gmConfirmMenu.BeLarge();
  ChangeToMenu( &gmConfirmMenu);
}

void VideoConfirm(void)
{
  // FIXUP: keyboard focus lost when going from full screen to window mode
  // due to WM_MOUSEMOVE being sent
  _bMouseUsedLast = FALSE;
  _pmgUnderCursor = gmConfirmMenu.gm_pmgSelectedByDefault;

  _pConfimedYes = NULL;
  void RevertVideoSettings(void);
  _pConfimedNo = RevertVideoSettings;

  mgConfirmLabel.mg_strText = TRANS("KEEP THIS SETTINGS?");
  gmConfirmMenu.gm_pgmParentMenu = pgmCurrentMenu;
  gmConfirmMenu.BeLarge();
  ChangeToMenu( &gmConfirmMenu);
}

void CDConfirm(void (*pOk)(void))
{
  _pConfimedYes = pOk;
  _pConfimedNo = NULL;
  mgConfirmLabel.mg_strText = TRANS("PLEASE INSERT GAME CD?");
  if (pgmCurrentMenu!=&gmConfirmMenu) {
    gmConfirmMenu.gm_pgmParentMenu = pgmCurrentMenu;
    gmConfirmMenu.BeLarge();
    ChangeToMenu( &gmConfirmMenu);
  }
}

void StopCurrentGame(void)
{
  _pGame->StopGame();
  _gmRunningGameMode=GM_NONE;
  StopMenus(TRUE);
  StartMenus("");
}
void StartSinglePlayerNewMenuCustom(void)
{
  gmSinglePlayerNewMenu.gm_pgmParentMenu = &gmLevelsMenu;
  ChangeToMenu( &gmSinglePlayerNewMenu);
}
void StartSinglePlayerNewMenu(void)
{
  gmSinglePlayerNewMenu.gm_pgmParentMenu = &gmSinglePlayerMenu;
  extern CTString sam_strFirstLevel;
  _pGame->gam_strCustomLevel = sam_strFirstLevel;
  ChangeToMenu( &gmSinglePlayerNewMenu);
}

void StartSinglePlayerGame(void)
{
/*  if (!IsCDInDrive()) {
    CDConfirm(StartSinglePlayerGame);
    return;
  }
  */

  _pGame->gm_StartSplitScreenCfg = CGame::SSC_PLAY1;

  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_iSinglePlayer;
  _pGame->gm_aiStartLocalPlayers[1] = -1;
  _pGame->gm_aiStartLocalPlayers[2] = -1;
  _pGame->gm_aiStartLocalPlayers[3] = -1;

  _pGame->gm_strNetworkProvider = "Local";
  CUniversalSessionProperties sp;
  _pGame->SetSinglePlayerSession(sp);

  if (_pGame->NewGame( _pGame->gam_strCustomLevel, _pGame->gam_strCustomLevel, sp))
  {
    StopMenus();
    _gmRunningGameMode = GM_SINGLE_PLAYER;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}

void StartSinglePlayerGame_Tourist(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_TOURIST);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
}
void StartSinglePlayerGame_Easy(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_EASY);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
}
void StartSinglePlayerGame_Normal(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_NORMAL);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
}
void StartSinglePlayerGame_Hard(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_HARD);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
}
void StartSinglePlayerGame_Serious(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_EXTREME);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
}
void StartSinglePlayerGame_Mental(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_EXTREME+1);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
}

void StartTraining(void)
{
  gmSinglePlayerNewMenu.gm_pgmParentMenu = &gmSinglePlayerMenu;
  extern CTString sam_strTrainingLevel;
  _pGame->gam_strCustomLevel = sam_strTrainingLevel;
  ChangeToMenu( &gmSinglePlayerNewMenu);
}
void StartTechTest(void)
{
  gmSinglePlayerNewMenu.gm_pgmParentMenu = &gmSinglePlayerMenu;
  extern CTString sam_strTechTestLevel; 
  _pGame->gam_strCustomLevel = sam_strTechTestLevel;
  StartSinglePlayerGame_Normal();
}

void StartChangePlayerMenuFromOptions(void)
{
  _bPlayerMenuFromSinglePlayer = FALSE;
  gmPlayerProfile.gm_piCurrentPlayer = &_pGame->gm_iSinglePlayer;
  gmPlayerProfile.gm_pgmParentMenu = &gmOptionsMenu;
  ChangeToMenu( &gmPlayerProfile);
}

void StartChangePlayerMenuFromSinglePlayer(void)
{
  _iLocalPlayer = -1;
  _bPlayerMenuFromSinglePlayer = TRUE;
  gmPlayerProfile.gm_piCurrentPlayer = &_pGame->gm_iSinglePlayer;
  gmPlayerProfile.gm_pgmParentMenu = &gmSinglePlayerMenu;
  ChangeToMenu( &gmPlayerProfile);
}

void StartControlsMenuFromPlayer(void)
{
  gmControls.gm_pgmParentMenu = &gmPlayerProfile;
  ChangeToMenu( &gmControls);
}
void StartControlsMenuFromOptions(void)
{
  gmControls.gm_pgmParentMenu = &gmOptionsMenu;
  ChangeToMenu( &gmControls);
}

void DisabledFunction(void)
{
  gmDisabledFunction.gm_pgmParentMenu = pgmCurrentMenu;
  mgDisabledMenuButton.mg_strText = TRANS("The feature is not available in this version!");
  mgDisabledTitle.mg_strText = TRANS("DISABLED");
  ChangeToMenu( &gmDisabledFunction);
}

void ModNotInstalled(void)
{
  _pConfimedYes = &ExitAndSpawnExplorer;
  _pConfimedNo = NULL;
  mgConfirmLabel.mg_strText.PrintF(
    TRANS("You don't have MOD '%s' installed.\nDo you want to visit its web site?"), (const char*)_fnmModSelected);
  gmConfirmMenu.gm_pgmParentMenu = pgmCurrentMenu;
  gmConfirmMenu.BeSmall();
  ChangeToMenu( &gmConfirmMenu);

/*
  gmDisabledFunction.gm_pgmParentMenu = pgmCurrentMenu;
  mgDisabledMenuButton.mg_strText.PrintF(
    TRANS("You don't have MOD '%s' installed.\nPlease visit Croteam website for updates."), _fnmModSelected);
  mgDisabledTitle.mg_strText = TRANS("MOD REQUIRED");
  _strModURLSelected
  ChangeToMenu( &gmDisabledFunction);
  */
}

CTFileName _fnDemoToPlay;
void StartDemoPlay(void)
{
  _pGame->gm_StartSplitScreenCfg = CGame::SSC_OBSERVER;
  // play the demo
  _pGame->gm_strNetworkProvider = "Local";
  if( _pGame->StartDemoPlay( _fnDemoToPlay))
  {
    // exit menu and pull up the console
    StopMenus();
    if( _pGame->gm_csConsoleState!=CS_OFF) _pGame->gm_csConsoleState = CS_TURNINGOFF;
    _gmRunningGameMode = GM_DEMO;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}

void StartSelectLevelFromSingle(void)
{
  FilterLevels(SpawnFlagsForGameType(-1));
  _pAfterLevelChosen = StartSinglePlayerNewMenuCustom;
  ChangeToMenu( &gmLevelsMenu);
  gmLevelsMenu.gm_pgmParentMenu = &gmSinglePlayerMenu;
}

void StartNetworkGame(void)
{
//  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_aiMenuLocalPlayers[0];
  _pGame->gm_aiStartLocalPlayers[1] = _pGame->gm_aiMenuLocalPlayers[1];
  _pGame->gm_aiStartLocalPlayers[2] = _pGame->gm_aiMenuLocalPlayers[2];
  _pGame->gm_aiStartLocalPlayers[3] = _pGame->gm_aiMenuLocalPlayers[3];

  CTFileName fnWorld = _pGame->gam_strCustomLevel;

  _pGame->gm_strNetworkProvider = "TCP/IP Server";
  CUniversalSessionProperties sp;
  _pGame->SetMultiPlayerSession(sp);
  if (_pGame->NewGame( _pGame->gam_strSessionName, fnWorld, sp))
  {
    StopMenus();
    _gmRunningGameMode = GM_NETWORK;
    // if starting a dedicated server
    if (_pGame->gm_MenuSplitScreenCfg==CGame::SSC_DEDICATED) {
      // pull down the console
      extern INDEX sam_bToggleConsole;
      sam_bToggleConsole = TRUE;
    }
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}
void JoinNetworkGame(void)
{
//  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_aiMenuLocalPlayers[0];
  _pGame->gm_aiStartLocalPlayers[1] = _pGame->gm_aiMenuLocalPlayers[1];
  _pGame->gm_aiStartLocalPlayers[2] = _pGame->gm_aiMenuLocalPlayers[2];
  _pGame->gm_aiStartLocalPlayers[3] = _pGame->gm_aiMenuLocalPlayers[3];

  _pGame->gm_strNetworkProvider = "TCP/IP Client";
  if (_pGame->JoinGame( CNetworkSession( _pGame->gam_strJoinAddress)))
  {
    StopMenus();
    _gmRunningGameMode = GM_NETWORK;
  } else {
    if (_pNetwork->ga_strRequiredMod != "") {
      extern CTFileName _fnmModToLoad;
      extern CTString _strModServerJoin;
      char strModName[256] = {0};
      char strModURL[256] = {0};
      _pNetwork->ga_strRequiredMod.ScanF("%250[^\\]\\%s", &strModName, &strModURL);
      _fnmModSelected = CTString(strModName);
      _strModURLSelected = strModURL;
      if (_strModURLSelected=="") {
        _strModURLSelected = "http://www.croteam.com/mods/Old";
      }
      _strModServerSelected.PrintF("%s:%s", (const char *) _pGame->gam_strJoinAddress, (const char *) _pShell->GetValue("net_iPort"));
      ModConnectConfirm();
    }
    _gmRunningGameMode = GM_NONE;
  }
}
void StartHighScoreMenu(void)
{
  gmHighScoreMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmHighScoreMenu);
}
CTFileName _fnGameToLoad;
void StartNetworkLoadGame(void)
{

//  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_aiMenuLocalPlayers[0];
  _pGame->gm_aiStartLocalPlayers[1] = _pGame->gm_aiMenuLocalPlayers[1];
  _pGame->gm_aiStartLocalPlayers[2] = _pGame->gm_aiMenuLocalPlayers[2];
  _pGame->gm_aiStartLocalPlayers[3] = _pGame->gm_aiMenuLocalPlayers[3];

  _pGame->gm_strNetworkProvider = "TCP/IP Server";
  if (_pGame->LoadGame( _fnGameToLoad))
  {
    StopMenus();
    _gmRunningGameMode = GM_NETWORK;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}

void StartSplitScreenGame(void)
{
//  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_aiMenuLocalPlayers[0];
  _pGame->gm_aiStartLocalPlayers[1] = _pGame->gm_aiMenuLocalPlayers[1];
  _pGame->gm_aiStartLocalPlayers[2] = _pGame->gm_aiMenuLocalPlayers[2];
  _pGame->gm_aiStartLocalPlayers[3] = _pGame->gm_aiMenuLocalPlayers[3];

  CTFileName fnWorld = _pGame->gam_strCustomLevel;

  _pGame->gm_strNetworkProvider = "Local";
  CUniversalSessionProperties sp;
  _pGame->SetMultiPlayerSession(sp);
  if (_pGame->NewGame( fnWorld.FileName(), fnWorld, sp))
  {
    StopMenus();
    _gmRunningGameMode = GM_SPLIT_SCREEN;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}

void StartSplitScreenGameLoad(void)
{
//  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_aiMenuLocalPlayers[0];
  _pGame->gm_aiStartLocalPlayers[1] = _pGame->gm_aiMenuLocalPlayers[1];
  _pGame->gm_aiStartLocalPlayers[2] = _pGame->gm_aiMenuLocalPlayers[2];
  _pGame->gm_aiStartLocalPlayers[3] = _pGame->gm_aiMenuLocalPlayers[3];

  _pGame->gm_strNetworkProvider = "Local";
  if (_pGame->LoadGame( _fnGameToLoad))
  {
    StopMenus();
    _gmRunningGameMode = GM_SPLIT_SCREEN;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}

void StartSelectPlayersMenuFromSplit(void)
{
  gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
  gmSelectPlayersMenu.gm_bAllowObserving = FALSE;
  mgSelectPlayersStart.mg_pActivatedFunction = &StartSplitScreenGame;
  gmSelectPlayersMenu.gm_pgmParentMenu = &gmSplitStartMenu;
  ChangeToMenu( &gmSelectPlayersMenu);
}

void StartSelectPlayersMenuFromNetwork(void)
{
  gmSelectPlayersMenu.gm_bAllowDedicated = TRUE;
  gmSelectPlayersMenu.gm_bAllowObserving = TRUE;
  mgSelectPlayersStart.mg_pActivatedFunction = &StartNetworkGame;
  gmSelectPlayersMenu.gm_pgmParentMenu = &gmNetworkStartMenu;
  ChangeToMenu( &gmSelectPlayersMenu);
}

void StartSelectPlayersMenuFromOpen(void)
{
  gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
  gmSelectPlayersMenu.gm_bAllowObserving = TRUE;
  mgSelectPlayersStart.mg_pActivatedFunction = &JoinNetworkGame;
  gmSelectPlayersMenu.gm_pgmParentMenu = &gmNetworkOpenMenu;
  ChangeToMenu( &gmSelectPlayersMenu);

  /*if (sam_strNetworkSettings=="")*/ {
    void StartNetworkSettingsMenu(void);
    StartNetworkSettingsMenu();
    gmLoadSaveMenu.gm_bNoEscape = TRUE;
    gmLoadSaveMenu.gm_pgmParentMenu = &gmNetworkOpenMenu;
    gmLoadSaveMenu.gm_pgmNextMenu = &gmSelectPlayersMenu;
  }
}
void StartSelectPlayersMenuFromServers(void)
{
  gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
  gmSelectPlayersMenu.gm_bAllowObserving = TRUE;
  mgSelectPlayersStart.mg_pActivatedFunction = &JoinNetworkGame;
  gmSelectPlayersMenu.gm_pgmParentMenu = &gmServersMenu;
  ChangeToMenu( &gmSelectPlayersMenu);

  /*if (sam_strNetworkSettings=="")*/ {
    void StartNetworkSettingsMenu(void);
    StartNetworkSettingsMenu();
    gmLoadSaveMenu.gm_bNoEscape = TRUE;
    gmLoadSaveMenu.gm_pgmParentMenu = &gmServersMenu;
    gmLoadSaveMenu.gm_pgmNextMenu = &gmSelectPlayersMenu;
  }
}
void StartSelectServerLAN(void)
{
  gmServersMenu.m_bInternet = FALSE;
  ChangeToMenu( &gmServersMenu);
  gmServersMenu.gm_pgmParentMenu = &gmNetworkJoinMenu;
}
void StartSelectServerNET(void)
{
  gmServersMenu.m_bInternet = TRUE;
  ChangeToMenu( &gmServersMenu);
  gmServersMenu.gm_pgmParentMenu = &gmNetworkJoinMenu;
}

void StartSelectLevelFromSplit(void)
{
  FilterLevels(SpawnFlagsForGameType(mgSplitGameType.mg_iSelected));
  void StartSplitStartMenu(void);
  _pAfterLevelChosen = StartSplitStartMenu;
  ChangeToMenu( &gmLevelsMenu);
  gmLevelsMenu.gm_pgmParentMenu = &gmSplitStartMenu;
}
void StartSelectLevelFromNetwork(void)
{
  FilterLevels(SpawnFlagsForGameType(mgNetworkGameType.mg_iSelected));
  void StartNetworkStartMenu(void);
  _pAfterLevelChosen = StartNetworkStartMenu;
  ChangeToMenu( &gmLevelsMenu);
  gmLevelsMenu.gm_pgmParentMenu = &gmNetworkStartMenu;
}

void StartSelectPlayersMenuFromSplitScreen(void)
{
  gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
  gmSelectPlayersMenu.gm_bAllowObserving = FALSE;
//  mgSelectPlayersStart.mg_pActivatedFunction = &StartSplitScreenGame;
  gmSelectPlayersMenu.gm_pgmParentMenu = &gmSplitScreenMenu;
  ChangeToMenu( &gmSelectPlayersMenu);
}
void StartSelectPlayersMenuFromNetworkLoad(void)
{
  gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
  gmSelectPlayersMenu.gm_bAllowObserving = TRUE;
  mgSelectPlayersStart.mg_pActivatedFunction = &StartNetworkLoadGame;
  gmSelectPlayersMenu.gm_pgmParentMenu = &gmLoadSaveMenu;
  ChangeToMenu( &gmSelectPlayersMenu);
}

void StartSelectPlayersMenuFromSplitScreenLoad(void)
{
  gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
  gmSelectPlayersMenu.gm_bAllowObserving = FALSE;
  mgSelectPlayersStart.mg_pActivatedFunction = &StartSplitScreenGameLoad;
  gmSelectPlayersMenu.gm_pgmParentMenu = &gmLoadSaveMenu;
  ChangeToMenu( &gmSelectPlayersMenu);
}

BOOL LSLoadSinglePlayer(const CTFileName &fnm)
{
  _pGame->gm_StartSplitScreenCfg = CGame::SSC_PLAY1;

  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_iSinglePlayer;
  _pGame->gm_aiStartLocalPlayers[1] = -1;
  _pGame->gm_aiStartLocalPlayers[2] = -1;
  _pGame->gm_aiStartLocalPlayers[3] = -1;
  _pGame->gm_strNetworkProvider = "Local";
  if (_pGame->LoadGame(fnm)) {
    StopMenus();
    _gmRunningGameMode = GM_SINGLE_PLAYER;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
  return TRUE;
}
BOOL LSLoadNetwork(const CTFileName &fnm)
{
  // call local players menu
  _fnGameToLoad = fnm;
  StartSelectPlayersMenuFromNetworkLoad();
  return TRUE;
}
BOOL LSLoadSplitScreen(const CTFileName &fnm)
{
  // call local players menu
  _fnGameToLoad = fnm;
  StartSelectPlayersMenuFromSplitScreenLoad();
  return TRUE;
}
extern BOOL LSLoadDemo(const CTFileName &fnm)
{
  // call local players menu
  _fnDemoToPlay = fnm;
  StartDemoPlay();
  return TRUE;
}

BOOL LSLoadPlayerModel(const CTFileName &fnm)
{
  // get base filename
  CTString strBaseName = fnm.FileName();
  // set it for current player
  CPlayerCharacter &pc = _pGame->gm_apcPlayers[*gmPlayerProfile.gm_piCurrentPlayer];
  CPlayerSettings *pps = (CPlayerSettings *)pc.pc_aubAppearance;
  memset(pps->ps_achModelFile, 0, sizeof(pps->ps_achModelFile));
  strncpy(pps->ps_achModelFile, strBaseName, sizeof(pps->ps_achModelFile));

  void MenuGoToParent(void);
  MenuGoToParent();
  return TRUE;
}

BOOL LSLoadControls(const CTFileName &fnm)
{
  try {
    ControlsMenuOn();
    _pGame->gm_ctrlControlsExtra->Load_t(fnm);
    ControlsMenuOff();
  } catch (const char *strError) {
    CPrintF("%s", (const char *)strError);
  }

  void MenuGoToParent(void);
  MenuGoToParent();
  return TRUE;
}

BOOL LSLoadAddon(const CTFileName &fnm)
{
  extern INDEX _iAddonExecState;
  extern CTFileName _fnmAddonToExec;
  _iAddonExecState = 1;
  _fnmAddonToExec = fnm;
  return TRUE;
}

BOOL LSLoadMod(const CTFileName &fnm)
{
  _fnmModSelected = fnm;
  ModConfirm();
  return TRUE;
}

BOOL LSLoadCustom(const CTFileName &fnm)
{
  mgVarTitle.mg_strText = TRANS("ADVANCED OPTIONS");
//  LoadStringVar(fnm.NoExt()+".des", mgVarTitle.mg_strText);
//  mgVarTitle.mg_strText.OnlyFirstLine();
  gmVarMenu.gm_fnmMenuCFG = fnm;
  gmVarMenu.gm_pgmParentMenu = &gmLoadSaveMenu;
  ChangeToMenu( &gmVarMenu);
  return TRUE;
}

BOOL LSLoadNetSettings(const CTFileName &fnm)
{
  sam_strNetworkSettings = fnm;
  CTString strCmd;
  strCmd.PrintF("include \"%s\"", (const char*)sam_strNetworkSettings);
  _pShell->Execute(strCmd);

  void MenuGoToParent(void);
  MenuGoToParent();
  return TRUE;
}

// same function for saving in singleplay, network and splitscreen
BOOL LSSaveAnyGame(const CTFileName &fnm)
{
  if( _pGame->SaveGame( fnm)) {
    StopMenus();
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOL LSSaveDemo(const CTFileName &fnm)
{
  // save the demo
  if(_pGame->StartDemoRec(fnm)) {
    StopMenus();
    return TRUE;
  } else {
    return FALSE;
  }
}

// save/load menu calling functions
void StartPlayerModelLoadMenu(void)
{
  mgLoadSaveTitle.mg_strText = TRANS("CHOOSE MODEL");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEUP;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = FALSE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Models\\Player\\");
  gmLoadSaveMenu.gm_fnmSelected = _strLastPlayerAppearance;
  gmLoadSaveMenu.gm_fnmExt = CTString(".amc");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadPlayerModel;

  gmLoadSaveMenu.gm_pgmParentMenu = &gmPlayerProfile;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartControlsLoadMenu(void)
{
  mgLoadSaveTitle.mg_strText = TRANS("# LOAD CONTROLS");
  gmLoadSaveMenu.gm_bAllowThumbnails = FALSE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEUP;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = FALSE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Controls\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".ctl");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadControls;

  gmLoadSaveMenu.gm_pgmParentMenu = &gmControls;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartCustomLoadMenu(void)
{
  mgLoadSaveTitle.mg_strText = TRANS("ADVANCED OPTIONS");
  gmLoadSaveMenu.gm_bAllowThumbnails = FALSE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_NAMEUP;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = FALSE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Scripts\\CustomOptions\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".cfg");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadCustom;

  gmLoadSaveMenu.gm_pgmParentMenu = &gmOptionsMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartAddonsLoadMenu(void)
{
  mgLoadSaveTitle.mg_strText = TRANS("EXECUTE ADDON");
  gmLoadSaveMenu.gm_bAllowThumbnails = FALSE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_NAMEUP;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = FALSE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Scripts\\Addons\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".ini");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadAddon;

  gmLoadSaveMenu.gm_pgmParentMenu = &gmOptionsMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartModsLoadMenu(void)
{
  mgLoadSaveTitle.mg_strText = TRANS("CHOOSE MOD");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_NAMEUP;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = FALSE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Mods\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".des");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadMod;

  gmLoadSaveMenu.gm_pgmParentMenu = &gmMainMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartNetworkSettingsMenu(void)
{
  mgLoadSaveTitle.mg_strText = TRANS("CONNECTION SETTINGS");
  gmLoadSaveMenu.gm_bAllowThumbnails = FALSE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEUP;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = FALSE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Scripts\\NetSettings\\");
  gmLoadSaveMenu.gm_fnmSelected = sam_strNetworkSettings;
  gmLoadSaveMenu.gm_fnmExt = CTString(".ini");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadNetSettings;

  gmLoadSaveMenu.gm_pgmParentMenu = &gmOptionsMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartSinglePlayerQuickLoadMenu(void)
{
  _gmMenuGameMode = GM_SINGLE_PLAYER;

  mgLoadSaveTitle.mg_strText = TRANS("QUICK LOAD");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory.PrintF("SaveGame\\Player%d\\Quick\\", _pGame->gm_iSinglePlayer);
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadSinglePlayer;

  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartSinglePlayerLoadMenu(void)
{
  _gmMenuGameMode = GM_SINGLE_PLAYER;

  mgLoadSaveTitle.mg_strText = TRANS("# LOAD");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory.PrintF("SaveGame\\Player%d\\", _pGame->gm_iSinglePlayer);
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadSinglePlayer;

  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}
void StartSinglePlayerSaveMenu(void)
{
  if( _gmRunningGameMode != GM_SINGLE_PLAYER) return;
  // if no live players
  if (_pGame->GetPlayersCount()>0 && _pGame->GetLivePlayersCount()<=0) {
    // do nothing
    return;
  }
  _gmMenuGameMode = GM_SINGLE_PLAYER;
  mgLoadSaveTitle.mg_strText = TRANS("# SAVE");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = TRUE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory.PrintF("SaveGame\\Player%d\\", _pGame->gm_iSinglePlayer);
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmBaseName = CTString("SaveGame");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSSaveAnyGame;
  gmLoadSaveMenu.gm_strSaveDes = _pGame->GetDefaultGameDescription(TRUE);

  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}
void StartDemoLoadMenu(void)
{
  _gmMenuGameMode = GM_DEMO;

  mgLoadSaveTitle.mg_strText = TRANS("PLAY DEMO");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Demos\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".dem");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadDemo;

  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}
void StartDemoSaveMenu(void)
{
  if( _gmRunningGameMode == GM_NONE) return;
  _gmMenuGameMode = GM_DEMO;

  mgLoadSaveTitle.mg_strText = TRANS("RECORD DEMO");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEUP;
  gmLoadSaveMenu.gm_bSave = TRUE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("Demos\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmBaseName = CTString("Demo");
  gmLoadSaveMenu.gm_fnmExt = CTString(".dem");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSSaveDemo;
  gmLoadSaveMenu.gm_strSaveDes = _pGame->GetDefaultGameDescription(FALSE);

  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartNetworkQuickLoadMenu(void)
{
  _gmMenuGameMode = GM_NETWORK;

  mgLoadSaveTitle.mg_strText = TRANS("QUICK LOAD");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("SaveGame\\Network\\Quick\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadNetwork;
  
  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartNetworkLoadMenu(void)
{
  _gmMenuGameMode = GM_NETWORK;

  mgLoadSaveTitle.mg_strText = TRANS("LOAD");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("SaveGame\\Network\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadNetwork;
  
  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartNetworkSaveMenu(void)
{
  if( _gmRunningGameMode != GM_NETWORK) return;
  _gmMenuGameMode = GM_NETWORK;

  mgLoadSaveTitle.mg_strText = TRANS("SAVE");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = TRUE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("SaveGame\\Network\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmBaseName = CTString("SaveGame");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSSaveAnyGame;
  gmLoadSaveMenu.gm_strSaveDes = _pGame->GetDefaultGameDescription(TRUE);
  
  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartSplitScreenQuickLoadMenu(void)
{
  _gmMenuGameMode = GM_SPLIT_SCREEN;

  mgLoadSaveTitle.mg_strText = TRANS("QUICK LOAD");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("SaveGame\\SplitScreen\\Quick\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadSplitScreen;
  
  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

void StartSplitScreenLoadMenu(void)
{
  _gmMenuGameMode = GM_SPLIT_SCREEN;

  mgLoadSaveTitle.mg_strText = TRANS("LOAD");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = FALSE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("SaveGame\\SplitScreen\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSLoadSplitScreen;
  
  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}
void StartSplitScreenSaveMenu(void)
{
  if( _gmRunningGameMode != GM_SPLIT_SCREEN) return;
  _gmMenuGameMode = GM_SPLIT_SCREEN;

  mgLoadSaveTitle.mg_strText = TRANS("SAVE");
  gmLoadSaveMenu.gm_bAllowThumbnails = TRUE;
  gmLoadSaveMenu.gm_iSortType = LSSORT_FILEDN;
  gmLoadSaveMenu.gm_bSave = TRUE;
  gmLoadSaveMenu.gm_bManage = TRUE;
  gmLoadSaveMenu.gm_fnmDirectory = CTString("SaveGame\\SplitScreen\\");
  gmLoadSaveMenu.gm_fnmSelected = CTString("");
  gmLoadSaveMenu.gm_fnmBaseName = CTString("SaveGame");
  gmLoadSaveMenu.gm_fnmExt = CTString(".sav");
  gmLoadSaveMenu.gm_pAfterFileChosen = &LSSaveAnyGame;
  gmLoadSaveMenu.gm_strSaveDes = _pGame->GetDefaultGameDescription(TRUE);
  
  gmLoadSaveMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmLoadSaveMenu);
}

// game options var settings
void StartVarGameOptions(void)
{
  mgVarTitle.mg_strText = TRANS("GAME OPTIONS");
  gmVarMenu.gm_fnmMenuCFG = CTFILENAME("Scripts\\Menu\\GameOptions.cfg");
  ChangeToMenu( &gmVarMenu);
}

void StartSinglePlayerGameOptions(void)
{
  ChangeToMenu(&gmGameOptionsMenu);
}

void StartGameOptionsFromNetwork(void)
{
  StartVarGameOptions();
  gmVarMenu.gm_pgmParentMenu = &gmNetworkStartMenu;
}

void StartGameOptionsFromSplitScreen(void)
{
  StartVarGameOptions();
  gmVarMenu.gm_pgmParentMenu = &gmSplitStartMenu;
}

void StartRenderingOptionsMenu(void)
{
  ChangeToMenu(&gmRenderingOptionsMenu);
}

void StartCustomizeKeyboardMenu(void)
{
  ChangeToMenu( &gmCustomizeKeyboardMenu);
}
void StartCustomizeAxisMenu(void)
{
  ChangeToMenu( &gmCustomizeAxisMenu);
}
void StopRecordingDemo(void)
{
  _pNetwork->StopDemoRec();
}
void StartOptionsMenu(void)
{
  gmOptionsMenu.gm_pgmParentMenu = pgmCurrentMenu;
  ChangeToMenu( &gmOptionsMenu);
}

void StartCreditsMenu(void)
{
  ChangeToMenu(&gmCreditsMenu);
}

static void ResolutionToSize(INDEX iRes, PIX &pixSizeI, PIX &pixSizeJ)
{
    //ASSERT(iRes>=0 && iRes<_ctResolutions);
	if (iRes < 0 || iRes >= _ctResolutions) iRes=0;
    CDisplayMode &dm = _admResolutionModes[iRes];
    pixSizeI = dm.dm_pixSizeI;
    pixSizeJ = dm.dm_pixSizeJ;
}

static void AspectRatioToSize(INDEX iAsp, PIX &aspSizeI, PIX &aspSizeJ)
{
	if (iAsp < 0 || iAsp >= _ctAspectRatios) iAsp = 0;
	CDisplayMode &dm = _admAspectRatioModes[iAsp];
	aspSizeI = dm.dm_pixSizeI;
	aspSizeJ = dm.dm_pixSizeJ;
}

static void SizeToResolution(PIX pixSizeI, PIX pixSizeJ, PIX aspSizeI, PIX aspSizeJ, INDEX &iRes, INDEX &iAsp)
{
	long found=0;
	for(iAsp=0; iAsp<_ctAspectRatios; iAsp++)
	{
	    CDisplayMode &dm = _admAspectRatioModes[iAsp];
		if (dm.dm_pixSizeI==aspSizeI && dm.dm_pixSizeJ==aspSizeJ)
		{
			found=1;
	      	break;
	    }
	}
	if (!found)
	{
		iAsp=0;
	};

	for(iRes=0; iRes<_ctResolutions; iRes++)
	{
		CDisplayMode &dm = _admResolutionModes[iRes];
		if (dm.dm_pixSizeI==pixSizeI)
		{
      		return;
    	}
	}

  	// if still none found
  	ASSERT(FALSE);  // this should never happen
  	// return first one
  	iRes = 0;
}


static INDEX APIToSwitch(enum GfxAPIType gat)
{
  switch(gat) {
  case GAT_OGL: return 0;
#ifdef SE1_D3D
  case GAT_D3D: return 1;
#endif // SE1_D3D
  default: ASSERT(FALSE); return 0;
  }
}
static enum GfxAPIType SwitchToAPI(INDEX i)
{
  switch(i) {
  case 0: return GAT_OGL;
#ifdef SE1_D3D
  case 1: return GAT_D3D;
#endif // SE1_D3D
  default: ASSERT(FALSE); return GAT_OGL;
  }
}

static INDEX DepthToSwitch(enum DisplayDepth dd)
{
  switch(dd) {
  case DD_DEFAULT: return 0;
  case DD_16BIT  : return 1;
  case DD_32BIT  : return 2;
  default: ASSERT(FALSE); return 0;
  }
}
static enum DisplayDepth SwitchToDepth(INDEX i)
{
  switch(i) {
  case 0: return DD_DEFAULT;
  case 1: return DD_16BIT;
  case 2: return DD_32BIT;
  default: ASSERT(FALSE); return DD_DEFAULT;
  }
}

static void InitVideoOptionsButtons(void);
static void UpdateVideoOptionsButtons(INDEX i);

void RevertVideoSettings(void)
{
  // restore previous variables
  sam_bBorderLessActive = sam_old_bBorderLessActive;
  sam_bFullScreenActive = sam_old_bFullScreenActive;
  sam_iScreenSizeI      = sam_old_iScreenSizeI;
  sam_iScreenSizeJ      = sam_old_iScreenSizeJ;
  sam_iAspectSizeI      = sam_old_iAspectSizeI;
  sam_iAspectSizeJ      = sam_old_iAspectSizeJ;
  sam_iDisplayDepth     = sam_old_iDisplayDepth;
  sam_iDisplayAdapter   = sam_old_iDisplayAdapter;
  sam_iGfxAPI           = sam_old_iGfxAPI;
  sam_iVideoSetup       = sam_old_iVideoSetup;

  // update the video mode
  extern void ApplyVideoMode(void);
  ApplyVideoMode();

  // refresh buttons
  InitVideoOptionsButtons();
  UpdateVideoOptionsButtons(-1);
}

void ApplyVideoOptions(void)
{
  sam_old_bBorderLessActive = sam_bBorderLessActive;
  sam_old_bFullScreenActive = sam_bFullScreenActive;
  sam_old_iScreenSizeI      = sam_iScreenSizeI;
  sam_old_iScreenSizeJ      = sam_iScreenSizeJ;
  sam_old_iAspectSizeI      = sam_iAspectSizeI;
  sam_old_iAspectSizeJ      = sam_iAspectSizeJ;
  sam_old_iDisplayDepth     = sam_iDisplayDepth;
  sam_old_iDisplayAdapter   = sam_iDisplayAdapter;
  sam_old_iGfxAPI           = sam_iGfxAPI;
  sam_old_iVideoSetup       = sam_iVideoSetup;

  BOOL bBorderLessMode = mgBorderLessTrigger.mg_iSelected; // == 1;
  BOOL bFullScreenMode = mgFullScreenTrigger.mg_iSelected; // == 1;
  PIX pixWindowSizeI, pixWindowSizeJ;
  PIX aspWindowSizeI, aspWindowSizeJ;

  ResolutionToSize(mgResolutionsTrigger.mg_iSelected, pixWindowSizeI, pixWindowSizeJ);
  AspectRatioToSize(mgAspectRatiosTrigger.mg_iSelected, aspWindowSizeI, aspWindowSizeJ);
  enum GfxAPIType gat  = SwitchToAPI(mgDisplayAPITrigger.mg_iSelected);
  enum DisplayDepth dd = SwitchToDepth(mgBitsPerPixelTrigger.mg_iSelected);
  const INDEX iAdapter = mgDisplayAdaptersTrigger.mg_iSelected;

  // setup preferences
  extern INDEX _iLastPreferences;
  if( sam_iVideoSetup==3) _iLastPreferences = 3;
  sam_iVideoSetup = mgDisplayPrefsTrigger.mg_iSelected;

  // force fullscreen mode if needed
  CDisplayAdapter &da = _pGfx->gl_gaAPI[gat].ga_adaAdapter[iAdapter];
  if( da.da_ulFlags & DAF_FULLSCREENONLY) { bFullScreenMode = TRUE; bBorderLessMode = FALSE;}
  if( da.da_ulFlags & DAF_16BITONLY) dd = DD_16BIT;
  if( bBorderLessMode ) { bFullScreenMode = FALSE;}
  // force window to always be in default colors
  if( !bFullScreenMode) dd = DD_DEFAULT;

  //CPrintF(TRANS(" bFullScreenMode,  bFullScreenMode mode could not be set!\n"), bFullScreenMode,);

  // (try to) set mode
  sam_bBorderLessActive = bBorderLessMode;
  StartNewMode(gat, iAdapter, pixWindowSizeI, pixWindowSizeJ, aspWindowSizeI, aspWindowSizeJ, dd, bFullScreenMode, bBorderLessMode);

  //_fBigSizeJ = (2 - ((float)pixWindowSizeI / (float)pixWindowSizeJ))/10.0;

  // refresh buttons
  InitVideoOptionsButtons();
  UpdateVideoOptionsButtons(-1);
  
  // ask user to keep or restore
  if( bFullScreenMode) VideoConfirm();
}

#define VOLUME_STEPS  50

void RefreshSoundFormat( void)
{
  switch( _pSound->GetFormat())
  {
  case CSoundLibrary::SF_NONE:     {mgFrequencyTrigger.mg_iSelected = 0;break;}
  case CSoundLibrary::SF_11025_16: {mgFrequencyTrigger.mg_iSelected = 1;break;}
  case CSoundLibrary::SF_22050_16: {mgFrequencyTrigger.mg_iSelected = 2;break;}
  case CSoundLibrary::SF_44100_16: {mgFrequencyTrigger.mg_iSelected = 3;break;}
  default:                          mgFrequencyTrigger.mg_iSelected = 0;
  }

  mgAudioAutoTrigger.mg_iSelected = Clamp(sam_bAutoAdjustAudio, 0, 1);
  mgAudioAPITrigger.mg_iSelected = Clamp(_pShell->GetINDEX("snd_iInterface"), (INDEX)0, (INDEX)2);

  mgWaveVolume.mg_iMinPos = 0;
  mgWaveVolume.mg_iMaxPos = VOLUME_STEPS;
  mgWaveVolume.mg_iCurPos = (INDEX)(_pShell->GetFLOAT("snd_fSoundVolume")*VOLUME_STEPS +0.5f);
  mgWaveVolume.ApplyCurrentPosition();

  mgMPEGVolume.mg_iMinPos = 0;
  mgMPEGVolume.mg_iMaxPos = VOLUME_STEPS;
  mgMPEGVolume.mg_iCurPos = (INDEX)(_pShell->GetFLOAT("snd_fMusicVolume")*VOLUME_STEPS +0.5f);
  mgMPEGVolume.ApplyCurrentPosition();

  mgAudioAutoTrigger.ApplyCurrentSelection();
  mgAudioAPITrigger.ApplyCurrentSelection();
  mgFrequencyTrigger.ApplyCurrentSelection();
}

void ApplyAudioOptions(void)
{
  sam_bAutoAdjustAudio = mgAudioAutoTrigger.mg_iSelected;
  if (sam_bAutoAdjustAudio) {
    _pShell->Execute("include \"Scripts\\Addons\\SFX-AutoAdjust.ini\"");
  } else {
    _pShell->SetINDEX("snd_iInterface", mgAudioAPITrigger.mg_iSelected);

    switch( mgFrequencyTrigger.mg_iSelected)
    {
    case 0: {_pSound->SetFormat(CSoundLibrary::SF_NONE)    ;break;}
    case 1: {_pSound->SetFormat(CSoundLibrary::SF_11025_16);break;}
    case 2: {_pSound->SetFormat(CSoundLibrary::SF_22050_16);break;}
    case 3: {_pSound->SetFormat(CSoundLibrary::SF_44100_16);break;}
    default: _pSound->SetFormat(CSoundLibrary::SF_NONE);
    }
  }

  RefreshSoundFormat();
  snd_iFormat = _pSound->GetFormat();
}

void StartVideoOptionsMenu(void)
{
  ChangeToMenu( &gmVideoOptionsMenu);
}

void StartAudioOptionsMenu(void)
{
  ChangeToMenu( &gmAudioOptionsMenu);
}

void StartNetworkMenu(void)
{
  ChangeToMenu( &gmNetworkMenu);
}
void StartNetworkJoinMenu(void)
{
  ChangeToMenu( &gmNetworkJoinMenu);
}
void StartNetworkStartMenu(void)
{
  ChangeToMenu( &gmNetworkStartMenu);
}

void StartNetworkOpenMenu(void)
{
  ChangeToMenu( &gmNetworkOpenMenu);
}

void StartSplitScreenMenu(void)
{
  ChangeToMenu( &gmSplitScreenMenu);
}
void StartSplitStartMenu(void)
{
  ChangeToMenu( &gmSplitStartMenu);
}

// initialize game type strings table
void InitGameTypes(void)
{
  // get function that will provide us the info about gametype
  CShellSymbol *pss = _pShell->GetSymbol("GetGameTypeNameSS", /*bDeclaredOnly=*/ TRUE);
  // if none
  if (pss==NULL) {
    // error
    astrGameTypeRadioTexts[0] = "<\?\?\?>";
    ctGameTypeRadioTexts = 1;
    return;
  }

  // for each mode
  for(ctGameTypeRadioTexts=0; ctGameTypeRadioTexts < static_cast<INDEX>(ARRAYCOUNT(astrGameTypeRadioTexts)); ctGameTypeRadioTexts++) {
    // get the text
    CTString (*pFunc)(INDEX) = (CTString (*)(INDEX))pss->ss_pvValue;
    CTString strMode = pFunc(ctGameTypeRadioTexts);
    // if no mode modes
    if (strMode=="") {
      // stop
      break;
    }
    // add that mode
    astrGameTypeRadioTexts[ctGameTypeRadioTexts] = strMode;
  }
}

static CTextureObject toMainMenuBack;
static CTextureObject toInGameMenuBack;
static CTextureObject toCreditsMenuBack;
static CTextureObject toDefaultMenuBack;
static CTextureObject *ptoCurrentMenuBack;
static CTextureObject toPopupBack;

// ------------------------ Global menu function implementation
void InitializeMenus(void)
{
  try {
    // initialize and load corresponding fonts
    _fdSmall.Load_t(  CTFILENAME( "Fonts\\Display3-narrow.fnt"));
    _fdMedium.Load_t( CTFILENAME( "Fonts\\Display3-normal.fnt"));
    _fdBig.Load_t(    CTFILENAME( "Fonts\\Display3-caps.fnt"));
    _fdTitle.Load_t(  CTFILENAME( "Fonts\\Title2.fnt"));
    _fdSmall.SetCharSpacing(-1);
    _fdSmall.SetLineSpacing( 0);
    _fdSmall.SetSpaceWidth(0.4f);
    _fdMedium.SetCharSpacing(+1);
    _fdMedium.SetLineSpacing( 0);
    _fdMedium.SetSpaceWidth(0.4f);
    _fdBig.SetCharSpacing(+1);
    _fdBig.SetLineSpacing( 0);
    _fdTitle.SetCharSpacing(+1);
    _fdTitle.SetLineSpacing( 0);

    // load menu sounds
    _psdSelect = _pSoundStock->Obtain_t( CTFILENAME("Sounds\\Menu\\Select.wav"));
    _psdPress  = _pSoundStock->Obtain_t( CTFILENAME("Sounds\\Menu\\Press.wav"));
    _psoMenuSound = new CSoundObject;

    // initialize and load menu textures
    _toPointer.SetData_t( CTFILENAME( "Textures\\General\\Pointer.tex"));
    toMenuLogo.SetData_t(CTFILENAME("Textures\\Logo\\Menu.tex"));
    _toLogoMenuB.SetData_t(  CTFILENAME( "Textures\\Logo\\sam_menulogo256b.tex"));

    toMainMenuBack.SetData_t(CTFILENAME("Textures\\General\\MainMenuBack.tex"));
    toInGameMenuBack.SetData_t(CTFILENAME("Textures\\General\\InGameMenuBack.tex"));
    toCreditsMenuBack.SetData_t(CTFILENAME("Textures\\General\\CreditsMenuBack.tex"));
    toDefaultMenuBack.SetData_t(CTFILENAME("Textures\\General\\MenuBack.tex"));
    toPopupBack.SetData_t(CTFILENAME("Textures\\General\\PopupBack.tex"));
  }
  catch (const char *strError) {
    FatalError( strError);
  }
  // force logo textures to be of maximal size
  ((CTextureData*)toMenuLogo.GetData())->Force(TEX_CONSTANT);
  ((CTextureData*)_toLogoMenuB.GetData())->Force(TEX_CONSTANT);

  ((CTextureData*)toMainMenuBack.GetData())->Force(TEX_CONSTANT);
  ((CTextureData*)toInGameMenuBack.GetData())->Force(TEX_CONSTANT);
  ((CTextureData*)toDefaultMenuBack.GetData())->Force(TEX_CONSTANT);
  ((CTextureData*)toCreditsMenuBack.GetData())->Force(TEX_CONSTANT);
  ((CTextureData*)toPopupBack.GetData())->Force(TEX_CONSTANT);

  // menu's relative placement
  //CPlacement3D plRelative = CPlacement3D( FLOAT3D( 0.0f, 0.0f, -9.0f),
  //                          ANGLE3D( AngleDeg(0.0f), AngleDeg(0.0f), AngleDeg(0.0f)));
  try
  {
    TRANSLATERADIOARRAY(astrNoYes);
    TRANSLATERADIOARRAY(astrComputerInvoke);
    TRANSLATERADIOARRAY(astrDisplayAPIRadioTexts);
    TRANSLATERADIOARRAY(astrDisplayPrefsRadioTexts);
    TRANSLATERADIOARRAY(astrBitsPerPixelRadioTexts);
    TRANSLATERADIOARRAY(astrFrequencyRadioTexts);
    TRANSLATERADIOARRAY(astrSoundAPIRadioTexts);
    TRANSLATERADIOARRAY(astrDifficultyRadioTexts);
    TRANSLATERADIOARRAY(astrMaxPlayersRadioTexts);
    TRANSLATERADIOARRAY(astrWeapon);
    TRANSLATERADIOARRAY(astrSplitScreenRadioTexts);
    TRANSLATERADIOARRAY(astrBloodTexts);
    TRANSLATERADIOARRAY(astrTextureSizeTexts);
    TRANSLATERADIOARRAY(astrTextureQualityTexts);
    TRANSLATERADIOARRAY(astrShadowMapSizeTexts);
    TRANSLATERADIOARRAY(astrLensFlaresTexts);

    // initialize game type strings table
    InitGameTypes();

    // ------------------- Initialize menus
    gmConfirmMenu.Initialize_t();
    gmConfirmMenu.gm_strName="Confirm";
    gmConfirmMenu.gm_pmgSelectedByDefault = &mgConfirmYes;
    gmConfirmMenu.gm_pgmParentMenu = NULL;

    gmMainMenu.Initialize_t();
    gmMainMenu.gm_strName="Main";
    gmMainMenu.gm_pmgSelectedByDefault = &mgMainNewGame;
    gmMainMenu.gm_pgmParentMenu = NULL;

    gmInGameMenu.Initialize_t();
    gmInGameMenu.gm_strName="InGame";
    gmInGameMenu.gm_pmgSelectedByDefault = &mgInGameResumeGame;
    gmInGameMenu.gm_pgmParentMenu = NULL;

    gmSinglePlayerMenu.Initialize_t();
    gmSinglePlayerMenu.gm_strName="SinglePlayer";
    gmSinglePlayerMenu.gm_pmgSelectedByDefault = &mgSingleNewGame;
    gmSinglePlayerMenu.gm_pgmParentMenu = &gmMainMenu;

    gmSinglePlayerNewMenu.Initialize_t();
    gmSinglePlayerNewMenu.gm_strName="SinglePlayerNew";
    gmSinglePlayerNewMenu.gm_pmgSelectedByDefault = &mgSingleNewMedium;
    gmSinglePlayerNewMenu.gm_pgmParentMenu = &gmSinglePlayerMenu;

    gmDisabledFunction.Initialize_t();
    gmDisabledFunction.gm_strName="DisabledFunction";
    gmDisabledFunction.gm_pmgSelectedByDefault = &mgDisabledMenuButton;
    gmDisabledFunction.gm_pgmParentMenu = NULL;

    gmPlayerProfile.Initialize_t();
    gmPlayerProfile.gm_strName="PlayerProfile";
    gmPlayerProfile.gm_pmgSelectedByDefault = &mgPlayerName;

    gmControls.Initialize_t();
    gmControls.gm_strName="Controls";
    gmControls.gm_pmgSelectedByDefault = &mgControlsButtons;

    // warning! parent menu has to be set inside button activate function from where
    // Load/Save menu is called
    gmLoadSaveMenu.Initialize_t();
    gmLoadSaveMenu.gm_strName="LoadSave";
    gmLoadSaveMenu.gm_pmgSelectedByDefault = &amgLSButton[0];

    gmHighScoreMenu.Initialize_t();
    gmHighScoreMenu.gm_strName="HighScore";
    gmHighScoreMenu.gm_pmgSelectedByDefault = &mgBack;

    gmCustomizeKeyboardMenu.Initialize_t();
    gmCustomizeKeyboardMenu.gm_strName="CustomizeKeyboard";
    gmCustomizeKeyboardMenu.gm_pmgSelectedByDefault = &mgKey[0];
    gmCustomizeKeyboardMenu.gm_pgmParentMenu = &gmControls;

    gmCustomizeAxisMenu.Initialize_t();
    gmCustomizeAxisMenu.gm_strName="CustomizeAxis";
    gmCustomizeAxisMenu.gm_pmgSelectedByDefault = &mgAxisActionTrigger;
    gmCustomizeAxisMenu.gm_pgmParentMenu = &gmControls;

    gmOptionsMenu.Initialize_t();
    gmOptionsMenu.gm_strName="Options";
    gmOptionsMenu.gm_pmgSelectedByDefault = &mgOptionsGame;
    gmOptionsMenu.gm_pgmParentMenu = &gmMainMenu;

    gmVideoOptionsMenu.Initialize_t();
    gmVideoOptionsMenu.gm_strName="VideoOptions";
    gmVideoOptionsMenu.gm_pmgSelectedByDefault = &mgDisplayAPITrigger;
    gmVideoOptionsMenu.gm_pgmParentMenu = &gmOptionsMenu;

    gmAudioOptionsMenu.Initialize_t();
    gmAudioOptionsMenu.gm_strName="AudioOptions";
    gmAudioOptionsMenu.gm_pmgSelectedByDefault = &mgFrequencyTrigger;
    gmAudioOptionsMenu.gm_pgmParentMenu = &gmOptionsMenu;

    gmLevelsMenu.Initialize_t();
    gmLevelsMenu.gm_strName="Levels";
    gmLevelsMenu.gm_pmgSelectedByDefault = &mgManualLevel[0];
    gmLevelsMenu.gm_pgmParentMenu = &gmSinglePlayerMenu;

    gmVarMenu.Initialize_t();
    gmVarMenu.gm_strName="Var";
    gmVarMenu.gm_pmgSelectedByDefault = &mgVar[0];
    gmVarMenu.gm_pgmParentMenu = &gmNetworkStartMenu;

    gmServersMenu.Initialize_t();
    gmServersMenu.gm_strName="Servers";
    gmServersMenu.gm_pmgSelectedByDefault = &mgServerList;
    gmServersMenu.gm_pgmParentMenu = &gmNetworkOpenMenu;

    gmNetworkMenu.Initialize_t();
    gmNetworkMenu.gm_strName="Network";
    gmNetworkMenu.gm_pmgSelectedByDefault = &mgNetworkJoin;
    gmNetworkMenu.gm_pgmParentMenu = &gmMainMenu;

    gmNetworkStartMenu.Initialize_t();
    gmNetworkStartMenu.gm_strName="NetworkStart";
    gmNetworkStartMenu.gm_pmgSelectedByDefault = &mgNetworkStartStart;
    gmNetworkStartMenu.gm_pgmParentMenu = &gmNetworkMenu;
    
    gmNetworkJoinMenu.Initialize_t();
    gmNetworkJoinMenu.gm_strName="NetworkJoin";
    gmNetworkJoinMenu.gm_pmgSelectedByDefault = &mgNetworkJoinLAN;
    gmNetworkJoinMenu.gm_pgmParentMenu = &gmNetworkMenu;

    gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
    gmSelectPlayersMenu.gm_bAllowObserving = FALSE;
    gmSelectPlayersMenu.Initialize_t();
    gmSelectPlayersMenu.gm_strName="SelectPlayers";
    gmSelectPlayersMenu.gm_pmgSelectedByDefault = &mgSelectPlayersStart;

    gmNetworkOpenMenu.Initialize_t();
    gmNetworkOpenMenu.gm_strName="NetworkOpen";
    gmNetworkOpenMenu.gm_pmgSelectedByDefault = &mgNetworkOpenJoin;
    gmNetworkOpenMenu.gm_pgmParentMenu = &gmNetworkJoinMenu;

    gmSplitScreenMenu.Initialize_t();
    gmSplitScreenMenu.gm_strName="SplitScreen";
    gmSplitScreenMenu.gm_pmgSelectedByDefault = &mgSplitScreenStart;
    gmSplitScreenMenu.gm_pgmParentMenu = &gmMainMenu;

    gmSplitStartMenu.Initialize_t();
    gmSplitStartMenu.gm_strName="SplitStart";
    gmSplitStartMenu.gm_pmgSelectedByDefault = &mgSplitStartStart;
    gmSplitStartMenu.gm_pgmParentMenu = &gmSplitScreenMenu;

    gmGameOptionsMenu.Initialize_t();
    gmGameOptionsMenu.gm_strName = "GameOptions";
    gmGameOptionsMenu.gm_pmgSelectedByDefault = &mgGameOptionsBlood;
    gmGameOptionsMenu.gm_pgmParentMenu = &gmOptionsMenu;

    gmRenderingOptionsMenu.Initialize_t();
    gmRenderingOptionsMenu.gm_strName = "RenderingOptions";
    gmRenderingOptionsMenu.gm_pmgSelectedByDefault = &mgRenderingOptionsTexturesSize;
    gmRenderingOptionsMenu.gm_pgmParentMenu = &gmVideoOptionsMenu;

    gmCreditsMenu.Initialize_t();
    gmCreditsMenu.gm_strName = "Credits";
    gmCreditsMenu.gm_pmgSelectedByDefault = &mgCreditsBack;
    gmCreditsMenu.gm_pgmParentMenu = &gmMainMenu;
  }
  catch (const char *strError)
  {
    FatalError( strError);
  }
}

void DestroyMenus( void)
{
  gmMainMenu.Destroy();
  pgmCurrentMenu = NULL;
  _pSoundStock->Release(_psdSelect);
  _pSoundStock->Release(_psdPress);
  delete _psoMenuSound;
  _psdSelect = NULL;
  _psdPress = NULL;
  _psoMenuSound = NULL;
}

// go to parent menu if possible
void MenuGoToParent(void)
{
  // if there is no parent menu
  if( pgmCurrentMenu->gm_pgmParentMenu == NULL) {
    // if in game
    if (_gmRunningGameMode!=GM_NONE) {
      // exit menus
      StopMenus();
    // if no game is running
    } else {
      // go to main menu
      ChangeToMenu( &gmMainMenu);
    }
  // if there is some parent menu
  } else {
    // go to parent menu
    ChangeToMenu( pgmCurrentMenu->gm_pgmParentMenu);
  }
}

void MenuOnKeyDown( int iVKey)
{

  // check if mouse buttons used
  _bMouseUsedLast = (iVKey==VK_LBUTTON || iVKey==VK_RBUTTON || iVKey==VK_MBUTTON 
    || iVKey==10 || iVKey==11);

  // ignore mouse when editing
  if (_bEditingString && _bMouseUsedLast) {
    _bMouseUsedLast = FALSE;
    return;
  }

  // initially the message is not handled
  BOOL bHandled = FALSE;

  // if not a mouse button, or mouse is over some gadget
  if (!_bMouseUsedLast || _pmgUnderCursor!=NULL) {
    // ask current menu to handle the key
    bHandled = pgmCurrentMenu->OnKeyDown( iVKey);
  }

  // if not handled
  if(!bHandled) {
    // if escape or right mouse pressed
    if(iVKey==VK_ESCAPE || iVKey==VK_RBUTTON) {
      if (pgmCurrentMenu==&gmLoadSaveMenu && gmLoadSaveMenu.gm_bNoEscape) {
        return;
      }
      // go to parent menu if possible
      MenuGoToParent();
    }
  }
}

void MenuOnChar(MSG msg)
{
  // check if mouse buttons used
  _bMouseUsedLast = FALSE;

  // ask current menu to handle the key
  pgmCurrentMenu->OnChar(msg);
}

void MenuOnMouseMove(PIX pixI, PIX pixJ)
{
  static PIX pixLastI = 0;
  static PIX pixLastJ = 0;
  if (pixLastI==pixI && pixLastJ==pixJ) {
    return;
  }
  pixLastI = pixI;
  pixLastJ = pixJ;
  _bMouseUsedLast = !_bEditingString && !_bDefiningKey && !_pInput->IsInputEnabled();
}

void MenuUpdateMouseFocus(void)
{
  // get real cursor position
  POINT pt;
  GetCursorPos(&pt);
  ScreenToClient(_hwndMain, &pt);
  extern INDEX sam_bWideScreen;
  extern CDrawPort *pdp;
  if( sam_bWideScreen) {
    const PIX pixHeight = pdp->GetHeight();
    pt.y -= (LONG) ((pixHeight/0.75f-pixHeight)/2);
  }
  _pixCursorPosI += pt.x-_pixCursorExternPosI;
  _pixCursorPosJ  = _pixCursorExternPosJ;
  _pixCursorExternPosI = pt.x;
  _pixCursorExternPosJ = pt.y;

  // if mouse not used last
  if (!_bMouseUsedLast||_bDefiningKey||_bEditingString) {
    // do nothing
    return;
  }

  CMenuGadget *pmgActive = NULL;
  // for all gadgets in menu
  FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
    //CMenuGadget &mg = *itmg;
    // if focused
    if( itmg->mg_bFocused) {
      // remember it
      pmgActive = &itmg.Current();
    }
  }

  // if there is some under cursor
  if (_pmgUnderCursor!=NULL) {
    _pmgUnderCursor->OnMouseOver(_pixCursorPosI, _pixCursorPosJ);
    // if the one under cursor has no neighbours
    if (_pmgUnderCursor->mg_pmgLeft ==NULL 
      &&_pmgUnderCursor->mg_pmgRight==NULL 
      &&_pmgUnderCursor->mg_pmgUp   ==NULL 
      &&_pmgUnderCursor->mg_pmgDown ==NULL) {
      // it cannot be focused
      _pmgUnderCursor = NULL;
      return;
    }

    // if the one under cursor is not active and not disappearing
    if (pmgActive!=_pmgUnderCursor && _pmgUnderCursor->mg_bVisible) {
      // change focus
      if (pmgActive!=NULL) {
        pmgActive->OnKillFocus();
      }
      _pmgUnderCursor->OnSetFocus();
    }
  }
}

static CTimerValue _tvInitialization;
static TIME _tmInitializationTick = -1;
extern TIME _tmMenuLastTickDone;

void SetMenuLerping(void)
{
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  
  // if lerping was never set before
  if (_tmInitializationTick<0) {
    // initialize it
    _tvInitialization = tvNow;
    _tmInitializationTick = _tmMenuLastTickDone;
  }

  // get passed time from session state starting in precise time and in ticks
  FLOAT tmRealDelta = FLOAT((tvNow-_tvInitialization).GetSeconds());
  FLOAT tmTickDelta = _tmMenuLastTickDone-_tmInitializationTick;
  // calculate factor
  FLOAT fFactor = 1.0f-(tmTickDelta-tmRealDelta)/_pTimer->TickQuantum;

  // if the factor starts getting below zero
  if (fFactor<0) {
    // clamp it
    fFactor = 0.0f;
    // readjust timers so that it gets better
    _tvInitialization = tvNow;
    _tmInitializationTick = _tmMenuLastTickDone-_pTimer->TickQuantum;
  }
  if (fFactor>1) {
    // clamp it
    fFactor = 1.0f;
    // readjust timers so that it gets better
    _tvInitialization = tvNow;
    _tmInitializationTick = _tmMenuLastTickDone;
  }
  // set lerping factor and timer
  _pTimer->SetCurrentTick(_tmMenuLastTickDone);
  _pTimer->SetLerp(fFactor);
}


// render mouse cursor if needed
void RenderMouseCursor(CDrawPort *pdp)
{
  // if mouse not used last
  if (!_bMouseUsedLast|| _bDefiningKey || _bEditingString) {
    // don't render cursor
    return;
  }
  _pGame->LCDSetDrawport(pdp);
  _pGame->LCDDrawPointer(_pixCursorPosI, _pixCursorPosJ);
}

BOOL DoMenu( CDrawPort *pdp)
{
  pdp->Unlock();
  CDrawPort dpMenu(pdp, TRUE);
  dpMenu.Lock();

  MenuUpdateMouseFocus();

  // if in fullscreen
  CDisplayMode dmCurrent;
  _pGfx->GetCurrentDisplayMode(dmCurrent);
  if (dmCurrent.IsFullScreen()) {
    // clamp mouse pointer
    _pixCursorPosI = Clamp(_pixCursorPosI, (PIX)0, dpMenu.GetWidth());
    _pixCursorPosJ = Clamp(_pixCursorPosJ, (PIX)0, dpMenu.GetHeight());
  // if in window
  } else {
    // use same mouse pointer as windows
    _pixCursorPosI = _pixCursorExternPosI;
    _pixCursorPosJ = _pixCursorExternPosJ;
  }

  pgmCurrentMenu->Think();

  TIME tmTickNow = _pTimer->GetRealTimeTick();

  while( _tmMenuLastTickDone<tmTickNow)
  {
    _pTimer->SetCurrentTick(_tmMenuLastTickDone);
    // call think for all gadgets in menu
    FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
      itmg->Think();
    }
    _tmMenuLastTickDone+=_pTimer->TickQuantum;
  }

  SetMenuLerping();

  PIX pixW = dpMenu.GetWidth();
  PIX pixH = dpMenu.GetHeight();

  // blend background if menu is on
  if( bMenuActive)
  {
    // get current time
    //TIME  tmNow = _pTimer->GetLerpedCurrentTick();
    //UBYTE ubH1  = (INDEX)(tmNow*08.7f) & 255;
    //UBYTE ubH2  = (INDEX)(tmNow*27.6f) & 255;
    //UBYTE ubH3  = (INDEX)(tmNow*16.5f) & 255;
    //UBYTE ubH4  = (INDEX)(tmNow*35.4f) & 255;

    // clear screen with background texture
    _pGame->LCDPrepare(1.0f);
    _pGame->LCDSetDrawport(&dpMenu);
    // do not allow game to show through
    dpMenu.Fill(C_BLACK|255);

    FLOAT fScaleW = (FLOAT)pixW / 640.0f;
    FLOAT fScaleH = (FLOAT)pixH / 480.0f;
    PIX   pixI0, pixJ0, pixI1, pixJ1;

    if (pgmCurrentMenu == &gmMainMenu)
    {
      ptoCurrentMenuBack = &toMainMenuBack;
    }
    else if (pgmCurrentMenu == &gmInGameMenu)
    {
      ptoCurrentMenuBack = &toInGameMenuBack;
    }
    else if (pgmCurrentMenu == &gmCreditsMenu)
    {
      ptoCurrentMenuBack = &toCreditsMenuBack;
    }
    else
    {
      ptoCurrentMenuBack = &toDefaultMenuBack;
    }

    dpMenu.PutTexture(ptoCurrentMenuBack, PIXaabbox2D(PIX2D(0, 0), PIX2D(pixW, pixH)));

    // put logo(s) to main menu (if logos exist)
    if( pgmCurrentMenu==&gmMainMenu)
    {
      if( _ptoLogoODI!=NULL) {
        CTextureData &td = (CTextureData&)*_ptoLogoODI->GetData();
        #define LOGOSIZE 50
        const PIX pixLogoWidth  = (PIX) (LOGOSIZE * dpMenu.dp_fWideAdjustment);
        const PIX pixLogoHeight = (PIX) (LOGOSIZE* td.GetHeight() / td.GetWidth());
        pixI0 = (PIX) ((640-pixLogoWidth -16)*fScaleW);
        pixJ0 = (PIX) ((480-pixLogoHeight-16)*fScaleH);
        pixI1 = (PIX) (pixI0+ pixLogoWidth *fScaleW);
        pixJ1 = (PIX) (pixJ0+ pixLogoHeight*fScaleH);
        dpMenu.PutTexture( _ptoLogoODI, PIXaabbox2D( PIX2D( pixI0, pixJ0),PIX2D( pixI1, pixJ1)));
        #undef LOGOSIZE
      }  
      if( _ptoLogoCT!=NULL) {
        CTextureData &td = (CTextureData&)*_ptoLogoCT->GetData();
        #define LOGOSIZE 50
        const PIX pixLogoWidth  = (PIX) (LOGOSIZE * dpMenu.dp_fWideAdjustment);
        const PIX pixLogoHeight = (PIX) (LOGOSIZE* td.GetHeight() / td.GetWidth());
        pixI0 = (PIX) (12*fScaleW);
        pixJ0 = (PIX) ((480-pixLogoHeight-16)*fScaleH);
        pixI1 = (PIX) (pixI0+ pixLogoWidth *fScaleW);
        pixJ1 = (PIX) (pixJ0+ pixLogoHeight*fScaleH);
        dpMenu.PutTexture( _ptoLogoCT, PIXaabbox2D( PIX2D( pixI0, pixJ0),PIX2D( pixI1, pixJ1)));
        #undef LOGOSIZE
      } 
      
      {
        PIX pixLogoI = 6 * fScaleW;
        PIX pixLogoJ = 180 * fScaleH;
        PIX pixLogoSizeI = 256 * fScaleW;
        PIX pixLogoSizeJ = 128 * fScaleH;

        dpMenu.PutTexture(&toMenuLogo, PIXaabbox2D(
          PIX2D(pixLogoI, pixLogoJ), PIX2D(pixLogoI + pixLogoSizeI, pixLogoJ + pixLogoSizeJ)));
      }

    } else if (pgmCurrentMenu==&gmAudioOptionsMenu) {
      if( _ptoLogoEAX!=NULL) {
        CTextureData &td = (CTextureData&)*_ptoLogoEAX->GetData();
        const INDEX iSize = 95;
        const PIX pixLogoWidth  = (PIX) (iSize * dpMenu.dp_fWideAdjustment);
        const PIX pixLogoHeight = (PIX) (iSize * td.GetHeight() / td.GetWidth());
        pixI0 =  (PIX) ((640-pixLogoWidth - 35)*fScaleW);
        pixJ0 = (PIX) ((480-pixLogoHeight - 7)*fScaleH);
        pixI1 = (PIX) (pixI0+ pixLogoWidth *fScaleW);
        pixJ1 = (PIX) (pixJ0+ pixLogoHeight*fScaleH);
        dpMenu.PutTexture( _ptoLogoEAX, PIXaabbox2D( PIX2D( pixI0, pixJ0),PIX2D( pixI1, pixJ1)));
      }
    }

#define THUMBW 96
#define THUMBH 96
    // if there is a thumbnail
    if( _bThumbnailOn) {
      pixI0 = 243 * fScaleW;
      pixJ0 = 190 * fScaleH;
      pixI1 = pixI0 + THUMBW * fScaleW;
      pixJ1 = pixJ0 + THUMBH * fScaleH;
      if( _toThumbnail.GetData()!=NULL)
      { // show thumbnail with border
        dpMenu.PutTexture(&_toThumbnail, PIXaabbox2D(PIX2D(pixI0, pixJ0), PIX2D(pixI1, pixJ1)), C_WHITE | 255);
        dpMenu.DrawBorder(pixI0, pixJ0, THUMBW * fScaleW, THUMBH * fScaleH, _pGame->LCDGetColor(C_mdGREEN | 255, "thumbnail border"));
      } else {
        dpMenu.SetFont(_pfdDisplayFont);
        dpMenu.SetTextScaling(fScaleH);
        dpMenu.SetTextAspect(1.0f);
        dpMenu.PutTextCXY(TRANS("<no thumbnail>"), (pixI0 + pixI1) / 2, (pixJ0 + pixJ1) / 2, _pGame->LCDGetColor(C_GREEN | 255, "no thumbnail"));
      }
    }

    // assure we can listen to non-3d sounds
    _pSound->UpdateSounds();
  }

  // if this is popup menu
  if (pgmCurrentMenu->gm_bPopup) {

    // render parent menu first
    if (pgmCurrentMenu->gm_pgmParentMenu!=NULL) {
      _pGame->MenuPreRenderMenu(pgmCurrentMenu->gm_pgmParentMenu->gm_strName);
      FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_pgmParentMenu->gm_lhGadgets, itmg) {
        if( itmg->mg_bVisible) {
          itmg->Render( &dpMenu);
        }
      }
      _pGame->MenuPostRenderMenu(pgmCurrentMenu->gm_pgmParentMenu->gm_strName);
    }

    // gray it out
    dpMenu.Fill(C_BLACK|128);

    // clear popup box
    dpMenu.Unlock();

    PIXaabbox2D box = FloatBoxToPixBox(&dpMenu, BoxPopup());
    COLOR col = _pGame->LCDGetColor(C_GREEN | 255, "popup box");
    CDrawPort dpPopup(pdp, TRUE);

    dpPopup.Lock();
    dpPopup.PutTexture(&toPopupBack, box);
    dpPopup.DrawBorder(box.Min()(1), box.Min()(2), box.Size()(1), box.Size()(2), col | 255);
    dpPopup.Unlock();
    dpMenu.Lock();
  }

  // no entity is under cursor initially
  _pmgUnderCursor = NULL;

  BOOL bStillInMenus = FALSE;
  _pGame->MenuPreRenderMenu(pgmCurrentMenu->gm_strName);

  if (pgmCurrentMenu == &gmCreditsMenu) {
    Credits_Render(&dpMenu);
  }

  // for each menu gadget
  FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
    // if gadget is visible
    if( itmg->mg_bVisible) {
      bStillInMenus = TRUE;
      itmg->Render( &dpMenu);
      if (FloatBoxToPixBox(&dpMenu, itmg->mg_boxOnScreen)>=PIX2D(_pixCursorPosI, _pixCursorPosJ)) {
        _pmgUnderCursor = itmg;
      }
    }
  }
  _pGame->MenuPostRenderMenu(pgmCurrentMenu->gm_strName);

  // no currently active gadget initially
  CMenuGadget *pmgActive = NULL;
  // if mouse was not active last
  if (!_bMouseUsedLast) {
    // find focused gadget
    FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
      //CMenuGadget &mg = *itmg;
      // if focused
      if( itmg->mg_bFocused) {
        // it is active
        pmgActive = &itmg.Current();
        break;
      }
    }
  // if mouse was active last
  } else {
    // gadget under cursor is active
    pmgActive = _pmgUnderCursor;
  }

  // if editing
  if (_bEditingString && pmgActive!=NULL) {
    // dim the menu  bit
    dpMenu.Fill(C_BLACK|0x40);
    // render the edit gadget again
    pmgActive->Render(&dpMenu);
  }
  
  // if there is some active gadget and it has tips
  if (pmgActive!=NULL && (pmgActive->mg_strTip!="" || _bEditingString)) {
    CTString strTip = pmgActive->mg_strTip;
    if (_bEditingString) {
      strTip = TRANS("[Enter] - Ok, [Escape] - Cancel");
    }
    // print the tip
    SetFontMedium(&dpMenu);
    dpMenu.PutText(strTip,
      pixW * 0.01f, pixH * 0.962f, _pGame->LCDGetColor(C_WHITE | 255, "tool tip"));
  }

  _pGame->ConsolePrintLastLines(&dpMenu);

  RenderMouseCursor(&dpMenu);

  dpMenu.Unlock();
  pdp->Lock();

  return bStillInMenus;
}

void MenuBack(void)
{
  MenuGoToParent();
}

void FixupBackButton(CGameMenu *pgm)
{
  BOOL bResume = FALSE;

  if (mgBack.mg_lnNode.IsLinked()) {
    mgBack.mg_lnNode.Remove();
  }

  BOOL bHasBack = TRUE;

  if (pgm->gm_bPopup) {
    bHasBack = FALSE;
  }

  if (pgm->gm_pgmParentMenu==NULL) {
    if (_gmRunningGameMode==GM_NONE) {
      bHasBack = FALSE;
    } else {
      bResume = TRUE;
    }
  }
  if (!bHasBack) {
    mgBack.Disappear();
    return;
  }

  if (bResume) {
    mgBack.mg_strText = TRANS("RESUME");
    mgBack.mg_strTip = TRANS("return to game");
  } else {
    if (_bVarChanged) {
      mgBack.mg_strText = TRANS("CANCEL");
      mgBack.mg_strTip = TRANS("cancel changes");
    } else {
      mgBack.mg_strText = TRANS("BACK");
      mgBack.mg_strTip = TRANS("return to previous menu");
    }
  }

  mgBack.mg_iCenterI = -1;
  mgBack.mg_bfsFontSize = BFS_LARGE;
  mgBack.mg_boxOnScreen = BoxBack();
  //mgBack.mg_boxOnScreen = BoxLeftColumn(16.5f);
  pgm->gm_lhGadgets.AddTail( mgBack.mg_lnNode);

  mgBack.mg_pmgLeft = 
  mgBack.mg_pmgRight = 
  mgBack.mg_pmgUp = 
  mgBack.mg_pmgDown = pgm->gm_pmgSelectedByDefault;

  mgBack.mg_pActivatedFunction = &MenuBack;

  mgBack.Appear();
}

void ChangeToMenu( CGameMenu *pgmNewMenu)
{
  // auto-clear old thumbnail when going out of menu
  ClearThumbnail();

  if( pgmCurrentMenu != NULL) {
    if (!pgmNewMenu->gm_bPopup) {
      pgmCurrentMenu->EndMenu();
    } else {
      FOREACHINLIST(CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
        itmg->OnKillFocus();
      }
    }
  }
  pgmNewMenu->StartMenu();
  if (pgmNewMenu->gm_pmgSelectedByDefault) {
    if (mgBack.mg_bFocused) {
      mgBack.OnKillFocus();
    }
    pgmNewMenu->gm_pmgSelectedByDefault->OnSetFocus();
  }

  pgmCurrentMenu = pgmNewMenu;
}

// ------------------------ SGameMenu implementation
CGameMenu::CGameMenu( void)
{
  gm_pgmParentMenu = NULL;
  gm_pmgSelectedByDefault = NULL;
  gm_pmgArrowUp = NULL;
  gm_pmgArrowDn = NULL;
  gm_pmgListTop = NULL;
  gm_pmgListBottom = NULL;
  gm_iListOffset = 0;
  gm_ctListVisible = 0;
  gm_ctListTotal = 0;
  gm_bPopup = FALSE;
}

void CGameMenu::Initialize_t( void)
{
}

void CGameMenu::Destroy(void)
{
}
void CGameMenu::FillListItems(void)
{
  ASSERT(FALSE);  // must be implemented to scroll up/down
}

// +-1 -> hit top/bottom when pressing up/down on keyboard
// +-2 -> pressed pageup/pagedown on keyboard
// +-3 -> pressed arrow up/down  button in menu
// +-4 -> scrolling with mouse wheel
void CGameMenu::ScrollList(INDEX iDir)
{
  // if not valid for scrolling
  if (gm_ctListTotal<=0
    || gm_pmgArrowUp == NULL || gm_pmgArrowDn == NULL
    || gm_pmgListTop == NULL || gm_pmgListBottom == NULL) {
    // do nothing
    return;
  }

  INDEX iOldTopKey = gm_iListOffset;
  // change offset
  switch(iDir) {
    case -1:
      gm_iListOffset -= 1;
      break;
    case -4:
      gm_iListOffset -= 3;
      break;
    case -2:
    case -3:
      gm_iListOffset -= gm_ctListVisible;
      break;
    case +1:
      gm_iListOffset += 1;
      break;
    case +4:
      gm_iListOffset += 3;
      break;
    case +2:
    case +3:
      gm_iListOffset += gm_ctListVisible;
      break;
    default:
      ASSERT(FALSE);
      return;
  }
  if (gm_ctListTotal<=gm_ctListVisible) {
    gm_iListOffset = 0;
  } else {
    gm_iListOffset = Clamp(gm_iListOffset, INDEX(0), INDEX(gm_ctListTotal-gm_ctListVisible));
  }

  // set new names
  FillListItems();

  // if scroling with wheel
  if (iDir==+4 || iDir==-4) {
    // no focus changing
    return;
  }

  // delete all focuses
  FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
    itmg->OnKillFocus();
  }

  // set new focus
  //const INDEX iFirst = 0;
  //const INDEX iLast = gm_ctListVisible-1;
  switch(iDir) {
    case +1:
      gm_pmgListBottom->OnSetFocus();
      break;
    case +2:
      if (gm_iListOffset!=iOldTopKey) {
        gm_pmgListTop->OnSetFocus();
      } else {
        gm_pmgListBottom->OnSetFocus();
      }
    break;
    case +3:
      gm_pmgArrowDn->OnSetFocus();
      break;
    case -1:
      gm_pmgListTop->OnSetFocus();
      break;
    case -2:
      gm_pmgListTop->OnSetFocus();
      break;
    case -3:
      gm_pmgArrowUp->OnSetFocus();
      break;
  }
}

void CGameMenu::KillAllFocuses(void)
{
  // for each menu gadget in menu
  FOREACHINLIST( CMenuGadget, mg_lnNode, gm_lhGadgets, itmg) {
    itmg->mg_bFocused = FALSE;
  }
}

void CGameMenu::StartMenu(void)
{
  // for each menu gadget in menu
  FOREACHINLIST( CMenuGadget, mg_lnNode, gm_lhGadgets, itmg)
  {
    itmg->mg_bFocused = FALSE;
    // call appear
    itmg->Appear();
  }

  // if there is a list
  if (gm_pmgListTop!=NULL) {
    // scroll it so that the wanted tem is centered
    gm_iListOffset = gm_iListWantedItem-gm_ctListVisible/2;
    // clamp the scrolling
    gm_iListOffset = Clamp(gm_iListOffset, (INDEX)0, Max((INDEX)0, gm_ctListTotal-gm_ctListVisible));

    // fill the list
    FillListItems();

    // for each menu gadget in menu
    FOREACHINLIST( CMenuGadget, mg_lnNode, gm_lhGadgets, itmg) {
      // if in list, but disabled
      if (itmg->mg_iInList==-2) {
        // hide it
        itmg->mg_bVisible = FALSE;
      // if in list
      } else if (itmg->mg_iInList>=0) {
        // show it
        itmg->mg_bVisible = TRUE;
      }
      // if wanted
      if (itmg->mg_iInList==gm_iListWantedItem) {
        // focus it
        itmg->OnSetFocus();
        gm_pmgSelectedByDefault = itmg;
      }
    }
  }
}

void CGameMenu::EndMenu(void)
{
  // for each menu gadget in menu
  FOREACHINLIST( CMenuGadget, mg_lnNode, gm_lhGadgets, itmg)
  {
    // call disappear
    itmg->Disappear();
  }
}

// return TRUE if handled
BOOL CGameMenu::OnKeyDown( int iVKey)
{
  // find curently active gadget
  CMenuGadget *pmgActive = NULL;
  // for each menu gadget in menu
  FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
    // if focused
    if( itmg->mg_bFocused) {
      // remember as active
      pmgActive = &itmg.Current();
    }
  }

  // if none focused
  if( pmgActive == NULL) {
    // do nothing
    return FALSE;
  }

  // if active gadget handles it
  if( pmgActive->OnKeyDown( iVKey)) {
    // key is handled
    return TRUE;
  }

  // process normal in menu movement
  switch( iVKey) {
  case VK_PRIOR:
    ScrollList(-2);
    return TRUE;
  case VK_NEXT:
    ScrollList(+2);
    return TRUE;

  case 11:
    ScrollList(-4);
    return TRUE;
  case 10:
    ScrollList(+4);
    return TRUE;

  case VK_UP:
    // if this is top button in list
    if (pmgActive==gm_pmgListTop) {
      // scroll list up
      ScrollList(-1);
      // key is handled
      return TRUE;
    }
    // if we can go up
    if(pmgActive->mg_pmgUp != NULL && pmgActive->mg_pmgUp->mg_bVisible) {
      // call lose focus to still active gadget and
      pmgActive->OnKillFocus();
      // set focus to new one
      pmgActive = pmgActive->mg_pmgUp;
      pmgActive->OnSetFocus();
      // key is handled
      return TRUE;
    }
    break;
  case VK_DOWN:
    // if this is bottom button in list
    if (pmgActive==gm_pmgListBottom) {
      // scroll list down
      ScrollList(+1);
      // key is handled
      return TRUE;
    }
    // if we can go down
    if(pmgActive->mg_pmgDown != NULL && pmgActive->mg_pmgDown->mg_bVisible) {
      // call lose focus to still active gadget and
      pmgActive->OnKillFocus();
      // set focus to new one
      pmgActive = pmgActive->mg_pmgDown;
      pmgActive->OnSetFocus();
      // key is handled
      return TRUE;
    }
    break;
  case VK_LEFT:
    // if we can go left
    if(pmgActive->mg_pmgLeft != NULL) {
      // call lose focus to still active gadget and
      pmgActive->OnKillFocus();
      // set focus to new one
      if (!pmgActive->mg_pmgLeft->mg_bVisible && gm_pmgSelectedByDefault!=NULL) {
        pmgActive = gm_pmgSelectedByDefault;
      } else {
        pmgActive = pmgActive->mg_pmgLeft;
      }
      pmgActive->OnSetFocus();
      // key is handled
      return TRUE;
    }
    break;
  case VK_RIGHT:
    // if we can go right
    if(pmgActive->mg_pmgRight != NULL) {
      // call lose focus to still active gadget and
      pmgActive->OnKillFocus();
      // set focus to new one
      if (!pmgActive->mg_pmgRight->mg_bVisible && gm_pmgSelectedByDefault!=NULL) {
        pmgActive = gm_pmgSelectedByDefault;
      } else {
        pmgActive = pmgActive->mg_pmgRight;
      }
      pmgActive->OnSetFocus();
      // key is handled
      return TRUE;
    }
    break;
  }

  // key is not handled
  return FALSE;
}

void CGameMenu::Think(void)
{
}

BOOL CGameMenu::OnChar(MSG msg)
{
  // find curently active gadget
  CMenuGadget *pmgActive = NULL;
  // for each menu gadget in menu
  FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
    // if focused
    if( itmg->mg_bFocused) {
      // remember as active
      pmgActive = &itmg.Current();
    }
  }

  // if none focused
  if( pmgActive == NULL) {
    // do nothing
    return FALSE;
  }

  // if active gadget handles it
  if( pmgActive->OnChar(msg)) {
    // key is handled
    return TRUE;
  }

  // key is not handled
  return FALSE;
}

// ------------------------ CConfirmMenu implementation
void CConfirmMenu::Initialize_t(void)
{
  gm_bPopup = TRUE;

  mgConfirmLabel.mg_strText = "";
  gm_lhGadgets.AddTail(mgConfirmLabel.mg_lnNode);
  mgConfirmLabel.mg_boxOnScreen = BoxPopupLabel();
  mgConfirmLabel.mg_iCenterI = -1;
  mgConfirmLabel.mg_bfsFontSize = BFS_LARGE;
  
  mgConfirmYes.mg_strText = TRANS("YES");
  gm_lhGadgets.AddTail(mgConfirmYes.mg_lnNode);
  mgConfirmYes.mg_boxOnScreen = BoxPopupYesLarge();
  mgConfirmYes.mg_pActivatedFunction = &ConfirmYes;
  mgConfirmYes.mg_pmgLeft =
  mgConfirmYes.mg_pmgRight = &mgConfirmNo;
  mgConfirmYes.mg_iCenterI = -1;
  mgConfirmYes.mg_bfsFontSize = BFS_LARGE;

  mgConfirmNo.mg_strText = TRANS("NO");
  gm_lhGadgets.AddTail(mgConfirmNo.mg_lnNode);
  mgConfirmNo.mg_boxOnScreen = BoxPopupNoLarge();
  mgConfirmNo.mg_pActivatedFunction = &ConfirmNo;
  mgConfirmNo.mg_pmgLeft =
  mgConfirmNo.mg_pmgRight = &mgConfirmYes;
  mgConfirmNo.mg_iCenterI = -1;
  mgConfirmNo.mg_bfsFontSize = BFS_LARGE;
}

void CConfirmMenu::BeLarge(void)
{
  mgConfirmLabel.mg_bfsFontSize = BFS_LARGE;
  mgConfirmYes.mg_bfsFontSize = BFS_LARGE;
  mgConfirmNo.mg_bfsFontSize = BFS_LARGE;
  mgConfirmLabel.mg_iCenterI = -1;
  mgConfirmYes.mg_boxOnScreen = BoxPopupYesLarge();
  mgConfirmNo.mg_boxOnScreen = BoxPopupNoLarge();
}

void CConfirmMenu::BeSmall(void)
{
  mgConfirmLabel.mg_bfsFontSize = BFS_MEDIUM;
  mgConfirmYes.mg_bfsFontSize = BFS_MEDIUM;
  mgConfirmNo.mg_bfsFontSize = BFS_MEDIUM;
  mgConfirmLabel.mg_iCenterI = -1;
  mgConfirmYes.mg_boxOnScreen = BoxPopupYesSmall();
  mgConfirmNo.mg_boxOnScreen = BoxPopupNoSmall();
}

// return TRUE if handled
BOOL CConfirmMenu::OnKeyDown(int iVKey)
{
  if (iVKey==VK_ESCAPE || iVKey==VK_RBUTTON) {
    ConfirmNo();
    return TRUE;
  }
  return CGameMenu::OnKeyDown(iVKey);
}

// ------------------------ CMainMenu implementation
void CMainMenu::Initialize_t(void)
{
  // intialize main menu
  extern CTString sam_strVersion;
  mgMainVersion.mg_strText = sam_strVersion;
  mgMainVersion.mg_boxOnScreen = BoxVersion();
  mgMainVersion.mg_bfsFontSize = BFS_MEDIUM;
  mgMainVersion.mg_iCenterI = -1;
  gm_lhGadgets.AddTail(mgMainVersion.mg_lnNode);

  mgMainNewGame.mg_strText = TRANS("NEW GAME");
  mgMainNewGame.mg_bfsFontSize = BFS_LARGE;
  mgMainNewGame.mg_boxOnScreen = BoxBigLeft(5.51f);
  mgMainNewGame.mg_strTip = TRANS("Start new game");
  gm_lhGadgets.AddTail(mgMainNewGame.mg_lnNode);
  mgMainNewGame.mg_pmgUp = &mgMainQuitGame;
  mgMainNewGame.mg_pmgDown = &mgMainLoadGame;
  mgMainNewGame.mg_pActivatedFunction = &StartSinglePlayerGame_Normal;
  mgMainNewGame.mg_iCenterI = -1;

  mgMainLoadGame.mg_strText = TRANS("LOAD GAME");
  mgMainLoadGame.mg_bfsFontSize = BFS_LARGE;
  mgMainLoadGame.mg_boxOnScreen = BoxBigLeft(6.51f);
  mgMainLoadGame.mg_strTip = TRANS("Load a saved game");
  gm_lhGadgets.AddTail(mgMainLoadGame.mg_lnNode);
  mgMainLoadGame.mg_pmgUp = &mgMainNewGame;
  mgMainLoadGame.mg_pmgDown = &mgMainOptions;
  mgMainLoadGame.mg_pActivatedFunction = &StartSinglePlayerLoadMenu;
  mgMainLoadGame.mg_iCenterI = -1;

  mgMainOptions.mg_strText = TRANS("OPTIONS");
  mgMainOptions.mg_bfsFontSize = BFS_LARGE;
  mgMainOptions.mg_boxOnScreen = BoxBigLeft(7.51f);
  mgMainOptions.mg_strTip = TRANS("Change video, audio and input settings");
  gm_lhGadgets.AddTail(mgMainOptions.mg_lnNode);
  mgMainOptions.mg_pmgUp = &mgMainLoadGame;
  mgMainOptions.mg_pmgDown = &mgMainCredits;
  mgMainOptions.mg_pActivatedFunction = &StartOptionsMenu;
  mgMainOptions.mg_iCenterI = -1;

  mgMainCredits.mg_strText = TRANS("CREDITS");
  mgMainCredits.mg_bfsFontSize = BFS_LARGE;
  mgMainCredits.mg_boxOnScreen = BoxBigLeft(8.51f);
  mgMainCredits.mg_strTip = TRANS("Show the list of authors");
  gm_lhGadgets.AddTail(mgMainCredits.mg_lnNode);
  mgMainCredits.mg_pmgUp = &mgMainOptions;
  mgMainCredits.mg_pmgDown = &mgMainQuitGame;
  mgMainCredits.mg_pActivatedFunction = &StartCreditsMenu;
  mgMainCredits.mg_iCenterI = -1;

  mgMainQuitGame.mg_strText = TRANS("QUIT GAME");
  mgMainQuitGame.mg_bfsFontSize = BFS_LARGE;
  mgMainQuitGame.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgMainQuitGame.mg_strTip = TRANS("Quit game");
  gm_lhGadgets.AddTail(mgMainQuitGame.mg_lnNode);
  mgMainQuitGame.mg_pmgUp = &mgMainCredits;
  mgMainQuitGame.mg_pmgDown = &mgMainNewGame;
  mgMainQuitGame.mg_pActivatedFunction = &ExitGame;
  mgMainQuitGame.mg_iCenterI = -1;
}

// ------------------------ CMainMenu implementation
void CInGameMenu::Initialize_t(void)
{
  // intialize main menu
  mgInGameTitle.mg_strText = TRANS("# GAME");
  mgInGameTitle.mg_boxOnScreen = BoxTitle(3.44f);
  gm_lhGadgets.AddTail(mgInGameTitle.mg_lnNode);
  mgInGameTitle.mg_iCenterI = -1;

  mgInGameResumeGame.mg_strText = TRANS("RESUME GAME");
  mgInGameResumeGame.mg_bfsFontSize = BFS_LARGE;
  mgInGameResumeGame.mg_boxOnScreen = BoxBigLeft(5.51f);
  mgInGameResumeGame.mg_strTip = TRANS("Return to game");
  gm_lhGadgets.AddTail(mgInGameResumeGame.mg_lnNode);
  mgInGameResumeGame.mg_pmgUp = &mgInGameQuitGame;
  mgInGameResumeGame.mg_pmgDown = &mgInGameLoadGame;
  mgInGameResumeGame.mg_pActivatedFunction = &MenuBack;
  mgInGameResumeGame.mg_iCenterI = -1;

  mgInGameLoadGame.mg_strText = TRANS("LOAD GAME");
  mgInGameLoadGame.mg_bfsFontSize = BFS_LARGE;
  mgInGameLoadGame.mg_boxOnScreen = BoxBigLeft(6.51f);
  mgInGameLoadGame.mg_strTip = TRANS("Load a saved game");
  gm_lhGadgets.AddTail(mgInGameLoadGame.mg_lnNode);
  mgInGameLoadGame.mg_pmgUp = &mgInGameResumeGame;
  mgInGameLoadGame.mg_pmgDown = &mgInGameSaveGame;
  mgInGameLoadGame.mg_pActivatedFunction = &StartSinglePlayerLoadMenu;
  mgInGameLoadGame.mg_iCenterI = -1;

  mgInGameSaveGame.mg_strText = TRANS("SAVE GAME");
  mgInGameSaveGame.mg_bfsFontSize = BFS_LARGE;
  mgInGameSaveGame.mg_boxOnScreen = BoxBigLeft(7.51f);
  mgInGameSaveGame.mg_strTip = TRANS("Save current game");
  gm_lhGadgets.AddTail(mgInGameSaveGame.mg_lnNode);
  mgInGameSaveGame.mg_pmgUp = &mgInGameLoadGame;
  mgInGameSaveGame.mg_pmgDown = &mgInGameOptions;
  mgInGameSaveGame.mg_pActivatedFunction = &StartSinglePlayerSaveMenu;
  mgInGameSaveGame.mg_iCenterI = -1;

  mgInGameOptions.mg_strText = TRANS("OPTIONS");
  mgInGameOptions.mg_bfsFontSize = BFS_LARGE;
  mgInGameOptions.mg_boxOnScreen = BoxBigLeft(8.51f);
  mgInGameOptions.mg_strTip = TRANS("Change video, audio and input settings");
  gm_lhGadgets.AddTail(mgInGameOptions.mg_lnNode);
  mgInGameOptions.mg_pmgUp = &mgInGameSaveGame;
  mgInGameOptions.mg_pmgDown = &mgInGameQuitGame;
  mgInGameOptions.mg_pActivatedFunction = &StartOptionsMenu;
  mgInGameOptions.mg_iCenterI = -1;

  mgInGameQuitGame.mg_strText = TRANS("QUIT GAME");
  mgInGameQuitGame.mg_bfsFontSize = BFS_LARGE;
  mgInGameQuitGame.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgInGameQuitGame.mg_strTip = TRANS("Return to main menu");
  gm_lhGadgets.AddTail(mgInGameQuitGame.mg_lnNode);
  mgInGameQuitGame.mg_pmgUp = &mgInGameOptions;
  mgInGameQuitGame.mg_pmgDown = &mgInGameResumeGame;
  mgInGameQuitGame.mg_pActivatedFunction = &StopConfirm;
  mgInGameQuitGame.mg_iCenterI = -1;
}

void CInGameMenu::StartMenu(void)
{
  mgInGameLoadGame.mg_bEnabled = _pNetwork->IsServer();
  mgInGameSaveGame.mg_bEnabled = _pNetwork->IsServer();

  CGameMenu::StartMenu();
}

// ------------------------ CSinglePlayerMenu implementation

void CSinglePlayerMenu::Initialize_t(void)
{
  // intialize single player menu
  mgSingleTitle.mg_strText = TRANS("SINGLE PLAYER");
  mgSingleTitle.mg_boxOnScreen = BoxTitle(0.0f);
  gm_lhGadgets.AddTail( mgSingleTitle.mg_lnNode);

  mgSinglePlayerLabel.mg_boxOnScreen = BoxBigRow(0.0f);
  mgSinglePlayerLabel.mg_bfsFontSize = BFS_MEDIUM;
  mgSinglePlayerLabel.mg_iCenterI = -1;
  mgSinglePlayerLabel.mg_bEnabled = FALSE;
  mgSinglePlayerLabel.mg_bLabel = TRUE;
  gm_lhGadgets.AddTail(mgSinglePlayerLabel.mg_lnNode);

  mgSingleNewGame.mg_strText = TRANS("NEW GAME");
  mgSingleNewGame.mg_bfsFontSize = BFS_LARGE;
  mgSingleNewGame.mg_boxOnScreen = BoxBigRow(0.0f);
  mgSingleNewGame.mg_strTip = TRANS("start new game with current player");
  gm_lhGadgets.AddTail( mgSingleNewGame.mg_lnNode);
  mgSingleNewGame.mg_pmgUp = &mgSingleOptions;
  mgSingleNewGame.mg_pmgDown = &mgSingleCustom;
  mgSingleNewGame.mg_pActivatedFunction = &StartSinglePlayerNewMenu;

  mgSingleCustom.mg_strText = TRANS("CUSTOM LEVEL");
  mgSingleCustom.mg_bfsFontSize = BFS_LARGE;
  mgSingleCustom.mg_boxOnScreen = BoxBigRow(1.0f);
  mgSingleCustom.mg_strTip = TRANS("start new game on a custom level");
  gm_lhGadgets.AddTail( mgSingleCustom.mg_lnNode);
  mgSingleCustom.mg_pmgUp = &mgSingleNewGame;
  mgSingleCustom.mg_pmgDown = &mgSingleQuickLoad;
    mgSingleCustom.mg_pActivatedFunction = &StartSelectLevelFromSingle;

  mgSingleQuickLoad.mg_strText = TRANS("QUICK LOAD");
  mgSingleQuickLoad.mg_bfsFontSize = BFS_LARGE;
  mgSingleQuickLoad.mg_boxOnScreen = BoxBigRow(2.0f);
  mgSingleQuickLoad.mg_strTip = TRANS("load a quick-saved game (F9)");
  gm_lhGadgets.AddTail( mgSingleQuickLoad.mg_lnNode);
  mgSingleQuickLoad.mg_pmgUp = &mgSingleCustom;
  mgSingleQuickLoad.mg_pmgDown = &mgSingleLoad;
  mgSingleQuickLoad.mg_pActivatedFunction = &StartSinglePlayerQuickLoadMenu;

  mgSingleLoad.mg_strText = TRANS("LOAD");
  mgSingleLoad.mg_bfsFontSize = BFS_LARGE;
  mgSingleLoad.mg_boxOnScreen = BoxBigRow(3.0f);
  mgSingleLoad.mg_strTip = TRANS("load a saved game of current player");
  gm_lhGadgets.AddTail( mgSingleLoad.mg_lnNode);
  mgSingleLoad.mg_pmgUp = &mgSingleQuickLoad;
  mgSingleLoad.mg_pmgDown = &mgSingleTraining;
  mgSingleLoad.mg_pActivatedFunction = &StartSinglePlayerLoadMenu;

  mgSingleTraining.mg_strText = TRANS("TRAINING");
  mgSingleTraining.mg_bfsFontSize = BFS_LARGE;
  mgSingleTraining.mg_boxOnScreen = BoxBigRow(4.0f);
  mgSingleTraining.mg_strTip = TRANS("start training level - KarnakDemo");
  gm_lhGadgets.AddTail( mgSingleTraining.mg_lnNode);
  mgSingleTraining.mg_pmgUp = &mgSingleLoad;
  mgSingleTraining.mg_pmgDown = &mgSingleTechTest;
  mgSingleTraining.mg_pActivatedFunction = &StartTraining;

  mgSingleTechTest.mg_strText = TRANS("TECHNOLOGY TEST");
  mgSingleTechTest.mg_bfsFontSize = BFS_LARGE;
  mgSingleTechTest.mg_boxOnScreen = BoxBigRow(5.0f);
  mgSingleTechTest.mg_strTip = TRANS("start technology testing level");
  gm_lhGadgets.AddTail( mgSingleTechTest.mg_lnNode);
  mgSingleTechTest.mg_pmgUp = &mgSingleTraining;
  mgSingleTechTest.mg_pmgDown = &mgSinglePlayersAndControls;
  mgSingleTechTest.mg_pActivatedFunction = &StartTechTest;

  mgSinglePlayersAndControls.mg_bfsFontSize = BFS_LARGE;
  mgSinglePlayersAndControls.mg_boxOnScreen = BoxBigRow(6.0f);
  mgSinglePlayersAndControls.mg_pmgUp = &mgSingleTechTest;
  mgSinglePlayersAndControls.mg_pmgDown = &mgSingleOptions;
  mgSinglePlayersAndControls.mg_strText = TRANS("PLAYERS AND CONTROLS");
  mgSinglePlayersAndControls.mg_strTip = TRANS("change currently active player or adjust controls");
  gm_lhGadgets.AddTail( mgSinglePlayersAndControls.mg_lnNode);
  mgSinglePlayersAndControls.mg_pActivatedFunction = &StartChangePlayerMenuFromSinglePlayer;

  mgSingleOptions.mg_strText = TRANS("GAME OPTIONS");
  mgSingleOptions.mg_bfsFontSize = BFS_LARGE;
  mgSingleOptions.mg_boxOnScreen = BoxBigRow(7.0f);
  mgSingleOptions.mg_strTip = TRANS("adjust miscellaneous game options");
  gm_lhGadgets.AddTail( mgSingleOptions.mg_lnNode);
  mgSingleOptions.mg_pmgUp = &mgSinglePlayersAndControls;
  mgSingleOptions.mg_pmgDown = &mgSingleNewGame;
  mgSingleOptions.mg_pActivatedFunction = &StartSinglePlayerGameOptions;
}

void CSinglePlayerMenu::StartMenu(void)
{
  mgSingleTraining.mg_bEnabled = IsMenuEnabled("Training");
  mgSingleTechTest.mg_bEnabled = IsMenuEnabled("Technology Test");

  if (mgSingleTraining.mg_bEnabled) {
    if (!mgSingleTraining.mg_lnNode.IsLinked()) {
      gm_lhGadgets.AddTail( mgSingleTraining.mg_lnNode);
    }

    mgSingleLoad.mg_boxOnScreen 	= BoxBigRow(3.0f);
    mgSingleLoad.mg_pmgUp 			= &mgSingleQuickLoad;
    mgSingleLoad.mg_pmgDown 		= &mgSingleTraining;

    mgSingleTraining.mg_boxOnScreen = BoxBigRow(4.0f);
    mgSingleTraining.mg_pmgUp 		= &mgSingleLoad;
    mgSingleTraining.mg_pmgDown 	= &mgSingleTechTest;

    mgSingleTechTest.mg_boxOnScreen = BoxBigRow(5.0f);
    mgSingleTechTest.mg_pmgUp 		= &mgSingleTraining;
    mgSingleTechTest.mg_pmgDown 	= &mgSinglePlayersAndControls;

    mgSinglePlayersAndControls.mg_boxOnScreen = BoxBigRow(6.0f);
    mgSingleOptions.mg_boxOnScreen 	= BoxBigRow(7.0f);

  } else {
    if (mgSingleTraining.mg_lnNode.IsLinked()) {
      mgSingleTraining.mg_lnNode.Remove();
    }

    mgSingleLoad.mg_boxOnScreen 	= BoxBigRow(3.0f);
    mgSingleLoad.mg_pmgUp 			= &mgSingleQuickLoad;
    mgSingleLoad.mg_pmgDown 		= &mgSingleTechTest;

    mgSingleTechTest.mg_boxOnScreen = BoxBigRow(4.0f);
    mgSingleTechTest.mg_pmgUp 		= &mgSingleLoad;
    mgSingleTechTest.mg_pmgDown 	= &mgSinglePlayersAndControls;

    mgSinglePlayersAndControls.mg_boxOnScreen = BoxBigRow(5.0f);
    mgSingleOptions.mg_boxOnScreen 	= BoxBigRow(6.0f);
  }

  CGameMenu::StartMenu();

  CPlayerCharacter &pc = _pGame->gm_apcPlayers[ _pGame->gm_iSinglePlayer];
  mgSinglePlayerLabel.mg_strText.PrintF( TRANS("Player: %s\n"), (const char *) pc.GetNameForPrinting());
}

// ------------------------ CSinglePlayerNewMenu implementation
void CSinglePlayerNewMenu::Initialize_t(void)
{
  // intialize single player new menu
  mgSingleNewTitle.mg_strText = TRANS("NEW GAME");
  mgSingleNewTitle.mg_boxOnScreen = BoxTitle(0.0f);
  gm_lhGadgets.AddTail( mgSingleNewTitle.mg_lnNode);

  mgSingleNewTourist.mg_strText = TRANS("TOURIST");
  mgSingleNewTourist.mg_bfsFontSize = BFS_LARGE;
  mgSingleNewTourist.mg_boxOnScreen = BoxBigRow(0.0f);
  mgSingleNewTourist.mg_strTip = TRANS("for non-FPS players");
  gm_lhGadgets.AddTail( mgSingleNewTourist.mg_lnNode);
  mgSingleNewTourist.mg_pmgUp = &mgSingleNewSerious;
  mgSingleNewTourist.mg_pmgDown = &mgSingleNewEasy;
  mgSingleNewTourist.mg_pActivatedFunction = &StartSinglePlayerGame_Tourist;

  mgSingleNewEasy.mg_strText = TRANS("EASY");
  mgSingleNewEasy.mg_bfsFontSize = BFS_LARGE;
  mgSingleNewEasy.mg_boxOnScreen = BoxBigRow(1.0f);
  mgSingleNewEasy.mg_strTip = TRANS("for unexperienced FPS players");
  gm_lhGadgets.AddTail( mgSingleNewEasy.mg_lnNode);
  mgSingleNewEasy.mg_pmgUp = &mgSingleNewTourist;
  mgSingleNewEasy.mg_pmgDown = &mgSingleNewMedium;
  mgSingleNewEasy.mg_pActivatedFunction = &StartSinglePlayerGame_Easy;

  mgSingleNewMedium.mg_strText = TRANS("NORMAL");
  mgSingleNewMedium.mg_bfsFontSize = BFS_LARGE;
  mgSingleNewMedium.mg_boxOnScreen = BoxBigRow(2.0f);
  mgSingleNewMedium.mg_strTip = TRANS("for experienced FPS players");
  gm_lhGadgets.AddTail( mgSingleNewMedium.mg_lnNode);
  mgSingleNewMedium.mg_pmgUp = &mgSingleNewEasy;
  mgSingleNewMedium.mg_pmgDown = &mgSingleNewHard;
  mgSingleNewMedium.mg_pActivatedFunction = &StartSinglePlayerGame_Normal;

  mgSingleNewHard.mg_strText = TRANS("HARD");
  mgSingleNewHard.mg_bfsFontSize = BFS_LARGE;
  mgSingleNewHard.mg_boxOnScreen = BoxBigRow(3.0f);
  mgSingleNewHard.mg_strTip = TRANS("for experienced Serious Sam players");
  gm_lhGadgets.AddTail( mgSingleNewHard.mg_lnNode);
  mgSingleNewHard.mg_pmgUp = &mgSingleNewMedium;
  mgSingleNewHard.mg_pmgDown = &mgSingleNewSerious;
  mgSingleNewHard.mg_pActivatedFunction = &StartSinglePlayerGame_Hard;

  mgSingleNewSerious.mg_strText = TRANS("SERIOUS");
  mgSingleNewSerious.mg_bfsFontSize = BFS_LARGE;
  mgSingleNewSerious.mg_boxOnScreen = BoxBigRow(4.0f);
  mgSingleNewSerious.mg_strTip = TRANS("are you serious?");
  gm_lhGadgets.AddTail( mgSingleNewSerious.mg_lnNode);
  mgSingleNewSerious.mg_pmgUp = &mgSingleNewHard;
  mgSingleNewSerious.mg_pmgDown = &mgSingleNewTourist;
  mgSingleNewSerious.mg_pActivatedFunction = &StartSinglePlayerGame_Serious;

  mgSingleNewMental.mg_strText = TRANS("MENTAL");
  mgSingleNewMental.mg_bfsFontSize = BFS_LARGE;
  mgSingleNewMental.mg_boxOnScreen = BoxBigRow(5.0f);
  mgSingleNewMental.mg_strTip = TRANS("you are not serious!");
  gm_lhGadgets.AddTail( mgSingleNewMental.mg_lnNode);
  mgSingleNewMental.mg_pmgUp = &mgSingleNewSerious;
  mgSingleNewMental.mg_pmgDown = &mgSingleNewTourist;
  mgSingleNewMental.mg_pActivatedFunction = &StartSinglePlayerGame_Mental;
  mgSingleNewMental.mg_bMental = TRUE;
}
void CSinglePlayerNewMenu::StartMenu(void)
{
  CGameMenu::StartMenu();
  extern INDEX sam_bMentalActivated;
  if (sam_bMentalActivated) {
    mgSingleNewMental.Appear();
    mgSingleNewSerious.mg_pmgDown = &mgSingleNewMental;
    mgSingleNewTourist.mg_pmgUp = &mgSingleNewMental;
  } else {
    mgSingleNewMental.Disappear();
    mgSingleNewSerious.mg_pmgDown = &mgSingleNewTourist;
    mgSingleNewTourist.mg_pmgUp = &mgSingleNewSerious;
  }
}

void CDisabledMenu::Initialize_t(void)
{
  mgDisabledTitle.mg_boxOnScreen = BoxTitle(0.0f);
  gm_lhGadgets.AddTail( mgDisabledTitle.mg_lnNode);

  mgDisabledMenuButton.mg_bfsFontSize = BFS_MEDIUM;
  mgDisabledMenuButton.mg_boxOnScreen = BoxBigRow(0.0f);
  gm_lhGadgets.AddTail( mgDisabledMenuButton.mg_lnNode);
  mgDisabledMenuButton.mg_pActivatedFunction = NULL;
}

void ChangeCrosshair(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  pps->ps_iCrossHairType = iNew-1;
}
void ChangeWeaponSelect(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  pps->ps_iWeaponAutoSelect = iNew;
}
void ChangeWeaponHide(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  if (iNew) {
    pps->ps_ulFlags |= PSF_HIDEWEAPON;
  } else {
    pps->ps_ulFlags &= ~PSF_HIDEWEAPON;
  }
}
void Change3rdPerson(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  if (iNew) {
    pps->ps_ulFlags |= PSF_PREFER3RDPERSON;
  } else {
    pps->ps_ulFlags &= ~PSF_PREFER3RDPERSON;
  }
}
void ChangeQuotes(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  if (iNew) {
    pps->ps_ulFlags &= ~PSF_NOQUOTES;
  } else {
    pps->ps_ulFlags |= PSF_NOQUOTES;
  }
}
void ChangeAutoSave(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  if (iNew) {
    pps->ps_ulFlags |= PSF_AUTOSAVE;
  } else {
    pps->ps_ulFlags &= ~PSF_AUTOSAVE;
  }
}
void ChangeCompDoubleClick(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  if (iNew) {
    pps->ps_ulFlags &= ~PSF_COMPSINGLECLICK;
  } else {
    pps->ps_ulFlags |= PSF_COMPSINGLECLICK;
  }
}

void ChangeViewBobbing(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  if (iNew) {
    pps->ps_ulFlags &= ~PSF_NOBOBBING;
  } else {
    pps->ps_ulFlags |= PSF_NOBOBBING;
  }
}

void ChangeSharpTurning(INDEX iNew)
{
  INDEX iPlayer = *gmPlayerProfile.gm_piCurrentPlayer;
  CPlayerSettings *pps = (CPlayerSettings *)_pGame->gm_apcPlayers[iPlayer].pc_aubAppearance;
  if (iNew) {
    pps->ps_ulFlags |= PSF_SHARPTURNING;
  } else {
    pps->ps_ulFlags &= ~PSF_SHARPTURNING;
  }
}

// ------------------------ CPlayerProfileMenu implementation
void CPlayerProfileMenu::Initialize_t(void)
{
  // intialize player and controls menu
  _bPlayerMenuFromSinglePlayer = FALSE;
  mgPlayerProfileTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgPlayerProfileTitle.mg_strText = TRANS("PLAYER PROFILE");
  gm_lhGadgets.AddTail( mgPlayerProfileTitle.mg_lnNode);

  mgPlayerNoLabel.mg_strText = TRANS("PROFILE:");
  mgPlayerNoLabel.mg_boxOnScreen = BoxMediumLeft(0.0f);
  mgPlayerNoLabel.mg_bfsFontSize = BFS_MEDIUM;
  mgPlayerNoLabel.mg_iCenterI = -1;
  gm_lhGadgets.AddTail( mgPlayerNoLabel.mg_lnNode);

#ifdef PLATFORM_UNIX
#define ADD_SELECT_PLAYER_MG( index, mg, mgprev, mgnext, me)\
  mg.mg_iIndex = index;\
  mg.mg_bfsFontSize = BFS_MEDIUM;\
  mg.mg_boxOnScreen = BoxNoUp(index);\
  mg.mg_bRectangle = TRUE;\
  mg.mg_pmgLeft = &mgprev;\
  mg.mg_pmgRight = &mgnext;\
  mg.mg_pmgUp = &mgPlayerCustomizeControls;\
  mg.mg_pmgDown = &mgPlayerName;\
  mg.mg_pActivatedFunction = &OnPlayerSelect;\
  mg.mg_strText = #index;\
  mg.mg_strTip = TRANS("select new currently active player");\
  gm_lhGadgets.AddTail( mg.mg_lnNode);
#else
#define ADD_SELECT_PLAYER_MG( index, mg, mgprev, mgnext, me)\
  mg.mg_iIndex = index;\
  mg.mg_bfsFontSize = BFS_MEDIUM;\
  mg.mg_boxOnScreen = BoxNoUp(index);\
  mg.mg_bRectangle = TRUE;\
  mg.mg_pmgLeft = &mgprev;\
  mg.mg_pmgRight = &mgnext;\
  mg.mg_pmgUp = &mgPlayerCustomizeControls;\
  mg.mg_pmgDown = &mgPlayerName;\
  mg.mg_pActivatedFunction = &OnPlayerSelect;\
  mg.mg_strText = #index;\
  mg.mg_strTip = TRANS("select new currently active player");\
  gm_lhGadgets.AddTail( mg.mg_lnNode);
#endif

  ADD_SELECT_PLAYER_MG( 0, mgPlayerNo[0], mgPlayerNo[7], mgPlayerNo[1], mePlayerNo[0]);
  ADD_SELECT_PLAYER_MG( 1, mgPlayerNo[1], mgPlayerNo[0], mgPlayerNo[2], mePlayerNo[1]);
  ADD_SELECT_PLAYER_MG( 2, mgPlayerNo[2], mgPlayerNo[1], mgPlayerNo[3], mePlayerNo[2]);
  ADD_SELECT_PLAYER_MG( 3, mgPlayerNo[3], mgPlayerNo[2], mgPlayerNo[4], mePlayerNo[3]);
  ADD_SELECT_PLAYER_MG( 4, mgPlayerNo[4], mgPlayerNo[3], mgPlayerNo[5], mePlayerNo[4]);
  ADD_SELECT_PLAYER_MG( 5, mgPlayerNo[5], mgPlayerNo[4], mgPlayerNo[6], mePlayerNo[5]);
  ADD_SELECT_PLAYER_MG( 6, mgPlayerNo[6], mgPlayerNo[5], mgPlayerNo[7], mePlayerNo[6]);
  ADD_SELECT_PLAYER_MG( 7, mgPlayerNo[7], mgPlayerNo[6], mgPlayerNo[0], mePlayerNo[7]);
  mgPlayerNo[7].mg_pmgRight = &mgPlayerModel;

  mgPlayerNameLabel.mg_strText = TRANS("NAME:");
  mgPlayerNameLabel.mg_boxOnScreen = BoxMediumLeft(1.25f);
  mgPlayerNameLabel.mg_bfsFontSize = BFS_MEDIUM;
  mgPlayerNameLabel.mg_iCenterI = -1;
  gm_lhGadgets.AddTail( mgPlayerNameLabel.mg_lnNode);

  // setup of player name button is done on start menu
  mgPlayerName.mg_strText = "<***>";
  mgPlayerName.mg_ctMaxStringLen = 25;
  mgPlayerName.mg_boxOnScreen = BoxPlayerEdit(1.25);
  mgPlayerName.mg_bfsFontSize = BFS_MEDIUM;
  mgPlayerName.mg_iCenterI = -1;
  mgPlayerName.mg_pmgUp = &mgPlayerNo[0];
  mgPlayerName.mg_pmgDown = &mgPlayerTeam;
  mgPlayerName.mg_pmgRight = &mgPlayerModel;
  mgPlayerName.mg_strTip = TRANS("rename currently active player");
  gm_lhGadgets.AddTail( mgPlayerName.mg_lnNode);

  mgPlayerTeamLabel.mg_strText = TRANS("TEAM:");
  mgPlayerTeamLabel.mg_boxOnScreen = BoxMediumLeft(2.25f);
  mgPlayerTeamLabel.mg_bfsFontSize = BFS_MEDIUM;
  mgPlayerTeamLabel.mg_iCenterI = -1;
  gm_lhGadgets.AddTail( mgPlayerTeamLabel.mg_lnNode);

  // setup of player name button is done on start menu
  mgPlayerTeam.mg_strText = "<***>";
  mgPlayerName.mg_ctMaxStringLen = 25;
  mgPlayerTeam.mg_boxOnScreen = BoxPlayerEdit(2.25f);
  mgPlayerTeam.mg_bfsFontSize = BFS_MEDIUM;
  mgPlayerTeam.mg_iCenterI = -1;
  mgPlayerTeam.mg_pmgUp = &mgPlayerName;
  mgPlayerTeam.mg_pmgDown = &mgPlayerCrosshair;
  mgPlayerTeam.mg_pmgRight = &mgPlayerModel;
  //mgPlayerTeam.mg_strTip = TRANS("teamplay is disabled in this version");
  mgPlayerTeam.mg_strTip = TRANS("enter team name, if playing in team");
  gm_lhGadgets.AddTail( mgPlayerTeam.mg_lnNode);

  TRIGGER_MG(mgPlayerCrosshair, 4.0, mgPlayerTeam, mgPlayerWeaponSelect, TRANS("CROSSHAIR"), astrCrosshair);
  mgPlayerCrosshair.mg_bVisual = TRUE;
  mgPlayerCrosshair.mg_boxOnScreen = BoxPlayerSwitch(5.0f);
  mgPlayerCrosshair.mg_iCenterI = -1;
  mgPlayerCrosshair.mg_pOnTriggerChange = ChangeCrosshair;
  TRIGGER_MG(mgPlayerWeaponSelect, 4.0, mgPlayerCrosshair, mgPlayerWeaponHide, TRANS("AUTO SELECT WEAPON"), astrWeapon);
  mgPlayerWeaponSelect.mg_boxOnScreen = BoxPlayerSwitch(6.0f);
  mgPlayerWeaponSelect.mg_iCenterI = -1;
  mgPlayerWeaponSelect.mg_pOnTriggerChange = ChangeWeaponSelect;
  TRIGGER_MG(mgPlayerWeaponHide, 4.0, mgPlayerWeaponSelect, mgPlayer3rdPerson, TRANS("HIDE WEAPON MODEL"), astrNoYes);
  mgPlayerWeaponHide.mg_boxOnScreen = BoxPlayerSwitch(7.0f);
  mgPlayerWeaponHide.mg_iCenterI = -1;
  mgPlayerWeaponHide.mg_pOnTriggerChange = ChangeWeaponHide;
  TRIGGER_MG(mgPlayer3rdPerson, 4.0, mgPlayerWeaponHide, mgPlayerQuotes, TRANS("PREFER 3RD PERSON VIEW"), astrNoYes);
  mgPlayer3rdPerson.mg_boxOnScreen = BoxPlayerSwitch(8.0f);
  mgPlayer3rdPerson.mg_iCenterI = -1;
  mgPlayer3rdPerson.mg_pOnTriggerChange = Change3rdPerson;
  TRIGGER_MG(mgPlayerQuotes, 4.0, mgPlayer3rdPerson, mgPlayerAutoSave, TRANS("VOICE QUOTES"), astrNoYes);
  mgPlayerQuotes.mg_boxOnScreen = BoxPlayerSwitch(9.0f);
  mgPlayerQuotes.mg_iCenterI = -1;
  mgPlayerQuotes.mg_pOnTriggerChange = ChangeQuotes;
  TRIGGER_MG(mgPlayerAutoSave, 4.0, mgPlayerQuotes, mgPlayerCompDoubleClick, TRANS("AUTO SAVE"), astrNoYes);
  mgPlayerAutoSave.mg_boxOnScreen = BoxPlayerSwitch(10.0f);
  mgPlayerAutoSave.mg_iCenterI = -1;
  mgPlayerAutoSave.mg_pOnTriggerChange = ChangeAutoSave;
  TRIGGER_MG(mgPlayerCompDoubleClick, 4.0, mgPlayerAutoSave, mgPlayerSharpTurning, TRANS("INVOKE COMPUTER"), astrComputerInvoke);
  mgPlayerCompDoubleClick.mg_boxOnScreen = BoxPlayerSwitch(11.0f);
  mgPlayerCompDoubleClick.mg_iCenterI = -1;
  mgPlayerCompDoubleClick.mg_pOnTriggerChange = ChangeCompDoubleClick;
  TRIGGER_MG(mgPlayerSharpTurning, 4.0, mgPlayerCompDoubleClick, mgPlayerViewBobbing, TRANS("SHARP TURNING"), astrNoYes);
  mgPlayerSharpTurning.mg_boxOnScreen = BoxPlayerSwitch(12.0f);
  mgPlayerSharpTurning.mg_iCenterI = -1;
  mgPlayerSharpTurning.mg_pOnTriggerChange = ChangeSharpTurning;
  TRIGGER_MG(mgPlayerViewBobbing, 4.0, mgPlayerSharpTurning, mgPlayerCustomizeControls, TRANS("VIEW BOBBING"), astrNoYes);
  mgPlayerViewBobbing.mg_boxOnScreen = BoxPlayerSwitch(13.0f);
  mgPlayerViewBobbing.mg_iCenterI = -1;
  mgPlayerViewBobbing.mg_pOnTriggerChange = ChangeViewBobbing;

  mgPlayerCustomizeControls.mg_strText = TRANS("CUSTOMIZE CONTROLS");
  mgPlayerCustomizeControls.mg_boxOnScreen = BoxMediumLeft(14.5f);
  mgPlayerCustomizeControls.mg_bfsFontSize = BFS_MEDIUM;
  mgPlayerCustomizeControls.mg_iCenterI = -1;
  mgPlayerCustomizeControls.mg_pmgUp = &mgPlayerViewBobbing;
  mgPlayerCustomizeControls.mg_pActivatedFunction = &StartControlsMenuFromPlayer;
  mgPlayerCustomizeControls.mg_pmgDown = &mgPlayerNo[0];
  mgPlayerCustomizeControls.mg_pmgRight = &mgPlayerModel;
  mgPlayerCustomizeControls.mg_strTip = TRANS("customize controls for this player");
  gm_lhGadgets.AddTail( mgPlayerCustomizeControls.mg_lnNode);

  mgPlayerModel.mg_boxOnScreen = BoxPlayerModel();
  mgPlayerModel.mg_pmgLeft = &mgPlayerName;
  mgPlayerModel.mg_pActivatedFunction = &StartPlayerModelLoadMenu;
  mgPlayerModel.mg_pmgDown = &mgPlayerName;
  mgPlayerModel.mg_pmgLeft = &mgPlayerName;
  mgPlayerModel.mg_strTip = TRANS("change model for this player");
  gm_lhGadgets.AddTail( mgPlayerModel.mg_lnNode);
}

INDEX CPlayerProfileMenu::ComboFromPlayer(INDEX iPlayer)
{
  return iPlayer;
}

INDEX CPlayerProfileMenu::PlayerFromCombo(INDEX iCombo)
{
  return iCombo;
}

void CPlayerProfileMenu::SelectPlayer(INDEX iPlayer)
{
  CPlayerCharacter &pc = _pGame->gm_apcPlayers[iPlayer];

  for( INDEX iPl=0; iPl<8; iPl++)
  {
    mgPlayerNo[iPl].mg_bHighlighted = FALSE;
  }

  mgPlayerNo[iPlayer].mg_bHighlighted = TRUE;

  iPlayer = Clamp(iPlayer, INDEX(0), INDEX(7));

  if (_iLocalPlayer>=0 && _iLocalPlayer<4) {
    _pGame->gm_aiMenuLocalPlayers[_iLocalPlayer] = iPlayer;
  } else {
    _pGame->gm_iSinglePlayer = iPlayer;
  }
  mgPlayerName.mg_pstrToChange = &pc.pc_strName;
  mgPlayerName.SetText( *mgPlayerName.mg_pstrToChange);
  mgPlayerTeam.mg_pstrToChange = &pc.pc_strTeam;
  mgPlayerTeam.SetText( *mgPlayerTeam.mg_pstrToChange);

  CPlayerSettings *pps = (CPlayerSettings *)pc.pc_aubAppearance;

  mgPlayerCrosshair.mg_iSelected = pps->ps_iCrossHairType+1;
  mgPlayerCrosshair.ApplyCurrentSelection();
  mgPlayerWeaponSelect.mg_iSelected = pps->ps_iWeaponAutoSelect;
  mgPlayerWeaponSelect.ApplyCurrentSelection();
  mgPlayerWeaponHide.mg_iSelected = (pps->ps_ulFlags&PSF_HIDEWEAPON)?1:0;
  mgPlayerWeaponHide.ApplyCurrentSelection();
  mgPlayer3rdPerson.mg_iSelected = (pps->ps_ulFlags&PSF_PREFER3RDPERSON)?1:0;
  mgPlayer3rdPerson.ApplyCurrentSelection();
  mgPlayerQuotes.mg_iSelected = (pps->ps_ulFlags&PSF_NOQUOTES)?0:1;
  mgPlayerQuotes.ApplyCurrentSelection();
  mgPlayerAutoSave.mg_iSelected = (pps->ps_ulFlags&PSF_AUTOSAVE)?1:0;
  mgPlayerAutoSave.ApplyCurrentSelection();
  mgPlayerCompDoubleClick.mg_iSelected = (pps->ps_ulFlags&PSF_COMPSINGLECLICK)?0:1;
  mgPlayerCompDoubleClick.ApplyCurrentSelection();
  mgPlayerViewBobbing.mg_iSelected = (pps->ps_ulFlags&PSF_NOBOBBING)?0:1;
  mgPlayerViewBobbing.ApplyCurrentSelection();
  mgPlayerSharpTurning.mg_iSelected = (pps->ps_ulFlags&PSF_SHARPTURNING)?1:0;
  mgPlayerSharpTurning.ApplyCurrentSelection();

  // get function that will set player appearance
  CShellSymbol *pss = _pShell->GetSymbol("SetPlayerAppearance", /*bDeclaredOnly=*/ TRUE);
  // if none
  if (pss==NULL) {
    // no model
    mgPlayerModel.mg_moModel.SetData(NULL);
  // if there is some
  } else {
    // set the model
    BOOL (*pFunc)(CModelObject *, CPlayerCharacter *, CTString &, BOOL) =
      (BOOL (*)(CModelObject *, CPlayerCharacter *, CTString &, BOOL))pss->ss_pvValue;
    CTString strName;
    BOOL bSet;
    if( _gmRunningGameMode!=GM_SINGLE_PLAYER && !_bPlayerMenuFromSinglePlayer) {
      bSet = pFunc(&mgPlayerModel.mg_moModel, &pc, strName, TRUE);
      mgPlayerModel.mg_strTip = TRANS("change model for this player");
      mgPlayerModel.mg_bEnabled = TRUE;
    } else {
      // cannot change player appearance in single player mode
      bSet = pFunc(&mgPlayerModel.mg_moModel, NULL, strName, TRUE);
      mgPlayerModel.mg_strTip = TRANS("cannot change model for single-player game");
      mgPlayerModel.mg_bEnabled = FALSE;
    }
    (void)bSet;
    // ignore gender flags, if any
    strName.RemovePrefix("#female#"); 
    strName.RemovePrefix("#male#");
    mgPlayerModel.mg_plModel = CPlacement3D(FLOAT3D(0.1f,-1.0f,-3.5f), ANGLE3D(150,0,0));
    mgPlayerModel.mg_strText = strName;
    CPlayerSettings *pps = (CPlayerSettings *)pc.pc_aubAppearance;
    _strLastPlayerAppearance = pps->GetModelFilename();
    try {
      mgPlayerModel.mg_moFloor.SetData_t(CTFILENAME("Models\\Computer\\Floor.mdl"));
      mgPlayerModel.mg_moFloor.mo_toTexture.SetData_t(CTFILENAME("Models\\Computer\\Floor.tex"));
    } catch (const char *strError) {
      (void)strError;
    }
  }
}


void OnPlayerSelect(void)
{
  ASSERT( _pmgLastActivatedGadget != NULL);
  if (_pmgLastActivatedGadget->mg_bEnabled) {
    gmPlayerProfile.SelectPlayer( ((CMGButton *)_pmgLastActivatedGadget)->mg_iIndex);
  }
}

void CPlayerProfileMenu::StartMenu(void)
{
  gmPlayerProfile.gm_pmgSelectedByDefault = &mgPlayerName;

  if (_gmRunningGameMode==GM_NONE || _gmRunningGameMode==GM_DEMO) {
    for(INDEX i=0; i<8; i++) {
      mgPlayerNo[i].mg_bEnabled = TRUE;
    }
  } else {
    for(INDEX i=0; i<8; i++) {
      mgPlayerNo[i].mg_bEnabled = FALSE;
    }
    INDEX iFirstEnabled = 0;
    {for(INDEX ilp=0; ilp<4; ilp++) {
      CLocalPlayer &lp = _pGame->gm_lpLocalPlayers[ilp];
      if (lp.lp_bActive) {
        mgPlayerNo[lp.lp_iPlayer].mg_bEnabled = TRUE;
        if (iFirstEnabled==0) {
          iFirstEnabled = lp.lp_iPlayer;
        }
      }
    }}
    // backup to first player in case current player is disabled
    if( !mgPlayerNo[*gm_piCurrentPlayer].mg_bEnabled) *gm_piCurrentPlayer = iFirstEnabled;
  }
  // done
  SelectPlayer( *gm_piCurrentPlayer);
  CGameMenu::StartMenu();
}


void CPlayerProfileMenu::EndMenu(void)
{
  _pGame->SavePlayersAndControls();
  CGameMenu::EndMenu();
}

// ------------------------ CControlsMenu implementation
void CControlsMenu::Initialize_t(void)
{
  // intialize controls menu
  mgControlsTitle.mg_boxOnScreen = BoxTitle(2.74f);
  mgControlsTitle.mg_strText = TRANS("# CONTROLS");
  gm_lhGadgets.AddTail(mgControlsTitle.mg_lnNode);
  mgControlsTitle.mg_iCenterI = -1;

  mgControlsButtons.mg_strText = TRANS("CUSTOMIZE KEYS");
  mgControlsButtons.mg_boxOnScreen = BoxMediumLeft(7.71f);
  mgControlsButtons.mg_bfsFontSize = BFS_MEDIUM;
  mgControlsButtons.mg_iCenterI = -1;
  gm_lhGadgets.AddTail(mgControlsButtons.mg_lnNode);
  mgControlsButtons.mg_pmgUp = &mgControlsBack;
  mgControlsButtons.mg_pmgDown = &mgControlsAdvanced;
  mgControlsButtons.mg_pActivatedFunction = &StartCustomizeKeyboardMenu;
  mgControlsButtons.mg_strTip = TRANS("Customize key bindings");

  mgControlsAdvanced.mg_strText = TRANS("ADVANCED GAMEPAD SETUP");
  mgControlsAdvanced.mg_iCenterI = -1;
  mgControlsAdvanced.mg_boxOnScreen = BoxMediumLeft(8.71f);
  mgControlsAdvanced.mg_bfsFontSize = BFS_MEDIUM;
  gm_lhGadgets.AddTail(mgControlsAdvanced.mg_lnNode);
  mgControlsAdvanced.mg_pmgUp = &mgControlsButtons;
  mgControlsAdvanced.mg_pmgDown = &mgControlsSensitivity;
  mgControlsAdvanced.mg_pActivatedFunction = &StartCustomizeAxisMenu;
  mgControlsAdvanced.mg_strTip = TRANS("Change advanced settings for gamepad axis");

  mgControlsSensitivity.mg_boxOnScreen = BoxMediumLeft(9.71f);
  mgControlsSensitivity.mg_strText = TRANS("SENSITIVITY");
  mgControlsSensitivity.mg_pmgUp = &mgControlsAdvanced;
  mgControlsSensitivity.mg_pmgDown = &mgControlsInvertTrigger;
  mgControlsSensitivity.mg_strTip = TRANS("Sensitivity for all axis");
  gm_lhGadgets.AddTail( mgControlsSensitivity.mg_lnNode);
  mgControlsSensitivity.mg_iCenterI = -1;

  mgControlsInvertTrigger.mg_pmgUp = &mgControlsSensitivity;
  mgControlsInvertTrigger.mg_pmgDown = &mgControlsSmoothTrigger;
  mgControlsInvertTrigger.mg_boxOnScreen = BoxMediumLeft(10.71f);
  gm_lhGadgets.AddTail(mgControlsInvertTrigger.mg_lnNode);
  mgControlsInvertTrigger.mg_astrTexts = astrNoYes;
  mgControlsInvertTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgControlsInvertTrigger.mg_iSelected = 0;
  mgControlsInvertTrigger.mg_strLabel = TRANS("INVERT LOOK");
  mgControlsInvertTrigger.mg_strValue = astrNoYes[0];
  mgControlsInvertTrigger.mg_strTip = TRANS("Invert up / down looking");
  mgControlsInvertTrigger.mg_iCenterI = -1;

  mgControlsSmoothTrigger.mg_pmgUp = &mgControlsInvertTrigger;
  mgControlsSmoothTrigger.mg_pmgDown = &mgControlsAccelTrigger;
  mgControlsSmoothTrigger.mg_boxOnScreen = BoxMediumLeft(11.71f);
  gm_lhGadgets.AddTail(mgControlsSmoothTrigger.mg_lnNode);
  mgControlsSmoothTrigger.mg_astrTexts = astrNoYes;
  mgControlsSmoothTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgControlsSmoothTrigger.mg_iSelected = 0;
  mgControlsSmoothTrigger.mg_strLabel = TRANS("SMOOTH AXIS");
  mgControlsSmoothTrigger.mg_strValue = astrNoYes[0];
  mgControlsSmoothTrigger.mg_strTip = TRANS("Smooth mouse / gamepad movements");
  mgControlsSmoothTrigger.mg_iCenterI = -1;

  mgControlsAccelTrigger.mg_pmgUp = &mgControlsSmoothTrigger;
  mgControlsAccelTrigger.mg_pmgDown = &mgControlsIFeelTrigger;
  mgControlsAccelTrigger.mg_boxOnScreen = BoxMediumLeft(12.71f);
  gm_lhGadgets.AddTail(mgControlsAccelTrigger.mg_lnNode);
  mgControlsAccelTrigger.mg_astrTexts = astrNoYes;
  mgControlsAccelTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgControlsAccelTrigger.mg_iSelected = 0;
  mgControlsAccelTrigger.mg_strLabel = TRANS("MOUSE ACCELERATION");
  mgControlsAccelTrigger.mg_strValue = astrNoYes[0];
  mgControlsAccelTrigger.mg_strTip = TRANS("Allow mouse acceleration");
  mgControlsAccelTrigger.mg_iCenterI = -1;

  mgControlsIFeelTrigger.mg_pmgUp = &mgControlsAccelTrigger;
  mgControlsIFeelTrigger.mg_pmgDown = &mgControlsPredefined;
  mgControlsIFeelTrigger.mg_boxOnScreen = BoxMediumLeft(13.71f);
  gm_lhGadgets.AddTail(mgControlsIFeelTrigger.mg_lnNode);
  mgControlsIFeelTrigger.mg_astrTexts = astrNoYes;
  mgControlsIFeelTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgControlsIFeelTrigger.mg_iSelected = 0;
  mgControlsIFeelTrigger.mg_strLabel = TRANS("IFEEL SUPPORT");
  mgControlsIFeelTrigger.mg_strValue = astrNoYes[0];
  mgControlsIFeelTrigger.mg_strTip = TRANS("Enable iFeel tactile feedback mouse support");
  mgControlsIFeelTrigger.mg_iCenterI = -1;

  mgControlsPredefined.mg_strText = TRANS("LOAD PREDEFINED SETTINGS");
  mgControlsPredefined.mg_iCenterI = -1;
  mgControlsPredefined.mg_boxOnScreen = BoxMediumLeft(14.71f);
  mgControlsPredefined.mg_bfsFontSize = BFS_MEDIUM;
  gm_lhGadgets.AddTail(mgControlsPredefined.mg_lnNode);
  mgControlsPredefined.mg_pmgUp = &mgControlsIFeelTrigger;
  mgControlsPredefined.mg_pmgDown = &mgControlsBack;
  mgControlsPredefined.mg_pActivatedFunction = &StartControlsLoadMenu;
  mgControlsPredefined.mg_strTip = TRANS("Load one of predefined control settings");
  mgControlsPredefined.mg_iCenterI = -1;

  mgControlsBack.mg_bfsFontSize = BFS_LARGE;
  mgControlsBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgControlsBack.mg_pmgUp = &mgControlsPredefined;
  mgControlsBack.mg_pmgDown = &mgControlsButtons;
  mgControlsBack.mg_strText = TRANS("BACK");
  mgControlsBack.mg_strTip = TRANS("Return to options menu");
  gm_lhGadgets.AddTail(mgControlsBack.mg_lnNode);
  mgControlsBack.mg_pActivatedFunction = &MenuBack;
  mgControlsBack.mg_iCenterI = -1;
}

void CControlsMenu::StartMenu(void)
{
  gm_pmgSelectedByDefault = &mgControlsButtons;
  INDEX iPlayer = _pGame->gm_iSinglePlayer;
  if (_iLocalPlayer>=0 && _iLocalPlayer<4) {
    iPlayer = _pGame->gm_aiMenuLocalPlayers[_iLocalPlayer];
  }
  _fnmControlsToCustomize.PrintF("Controls\\Controls%d.ctl", iPlayer);

  ControlsMenuOn();
  ObtainActionSettings();
  CGameMenu::StartMenu();
}

void CControlsMenu::EndMenu(void)
{
  ApplyActionSettings();

  ControlsMenuOff();

  CGameMenu::EndMenu();
}

void CControlsMenu::ObtainActionSettings(void)
{
  CControls &ctrls = *_pGame->gm_ctrlControlsExtra;

  mgControlsSensitivity.mg_iMinPos = 0;
  mgControlsSensitivity.mg_iMaxPos = 50;
  mgControlsSensitivity.mg_iCurPos = (INDEX) (ctrls.ctrl_fSensitivity/2);
  mgControlsSensitivity.ApplyCurrentPosition();

  mgControlsInvertTrigger.mg_iSelected = ctrls.ctrl_bInvertLook ? 1 : 0;
  mgControlsSmoothTrigger.mg_iSelected = ctrls.ctrl_bSmoothAxes ? 1 : 0;
  mgControlsAccelTrigger .mg_iSelected = _pShell->GetINDEX("inp_bAllowMouseAcceleration") ? 1 : 0;
  mgControlsIFeelTrigger .mg_bEnabled = _pShell->GetINDEX("sys_bIFeelEnabled") ? 1 : 0;
  mgControlsIFeelTrigger .mg_iSelected = _pShell->GetFLOAT("inp_fIFeelGain")>0 ? 1 : 0;

  mgControlsInvertTrigger.ApplyCurrentSelection();
  mgControlsSmoothTrigger.ApplyCurrentSelection();
  mgControlsAccelTrigger .ApplyCurrentSelection();
  mgControlsIFeelTrigger .ApplyCurrentSelection();
}

void CControlsMenu::ApplyActionSettings(void)
{
  CControls &ctrls = *_pGame->gm_ctrlControlsExtra;

  FLOAT fSensitivity = 
    FLOAT(mgControlsSensitivity.mg_iCurPos-mgControlsSensitivity.mg_iMinPos) /
    FLOAT(mgControlsSensitivity.mg_iMaxPos-mgControlsSensitivity.mg_iMinPos)*100.0f;
  
  BOOL bInvert = mgControlsInvertTrigger.mg_iSelected != 0;
  BOOL bSmooth = mgControlsSmoothTrigger.mg_iSelected != 0;
  BOOL bAccel  = mgControlsAccelTrigger .mg_iSelected != 0;
  BOOL bIFeel  = mgControlsIFeelTrigger .mg_iSelected != 0;

  if (INDEX(ctrls.ctrl_fSensitivity)!=INDEX(fSensitivity)) {
    ctrls.ctrl_fSensitivity = fSensitivity;
  }
  ctrls.ctrl_bInvertLook = bInvert;
  ctrls.ctrl_bSmoothAxes = bSmooth;
  _pShell->SetINDEX("inp_bAllowMouseAcceleration", bAccel);
  _pShell->SetFLOAT("inp_fIFeelGain", bIFeel ? 1.0f : 0.0f);
  ctrls.CalculateInfluencesForAllAxis();
}

// ------------------------ CLoadSaveMenu implementation
void CLoadSaveMenu::Initialize_t(void)
{
  gm_pgmNextMenu = NULL;

  mgLoadSaveTitle.mg_boxOnScreen = BoxTitle(0.82f);
  gm_lhGadgets.AddTail(mgLoadSaveTitle.mg_lnNode);
  mgLoadSaveTitle.mg_iCenterI = -1;

  for (INDEX iLabel = 0; iLabel < SAVELOAD_BUTTONS_CT; iLabel++)
  {
    INDEX iPrev = (SAVELOAD_BUTTONS_CT + iLabel - 1) % SAVELOAD_BUTTONS_CT;
    INDEX iNext = (iLabel + 1) % SAVELOAD_BUTTONS_CT;
  
    // initialize label gadgets
    amgLSButton[iLabel].mg_pmgUp = &amgLSButton[iPrev];
    amgLSButton[iLabel].mg_pmgDown = &amgLSButton[iNext];
    amgLSButton[iLabel].mg_boxOnScreen = BoxSaveLoad(4.71f + iLabel);
    amgLSButton[iLabel].mg_pActivatedFunction = NULL; // never called!
    amgLSButton[iLabel].mg_iCenterI = -1;
    gm_lhGadgets.AddTail(amgLSButton[iLabel].mg_lnNode);
  }

  gm_lhGadgets.AddTail(mgLSArrowUp.mg_lnNode);
  gm_lhGadgets.AddTail(mgLSArrowDn.mg_lnNode);
  mgLSArrowUp.mg_adDirection = AD_UP;
  mgLSArrowDn.mg_adDirection = AD_DOWN;
  mgLSArrowUp.mg_boxOnScreen = BoxArrow(AD_UP);
  mgLSArrowDn.mg_boxOnScreen = BoxArrow(AD_DOWN);
  mgLSArrowUp.mg_pmgUp = &mgLSBack;
  mgLSArrowUp.mg_pmgDown = &amgLSButton[0];
  mgLSArrowDn.mg_pmgUp = &amgLSButton[SAVELOAD_BUTTONS_CT - 1];
  mgLSArrowDn.mg_pmgDown = &mgLSBack;

  gm_ctListVisible = SAVELOAD_BUTTONS_CT;
  gm_pmgArrowUp = &mgLSArrowUp;
  gm_pmgArrowDn = &mgLSArrowDn;
  gm_pmgListTop = &amgLSButton[0];
  gm_pmgListBottom = &amgLSButton[SAVELOAD_BUTTONS_CT - 1];

  mgLSBack.mg_bfsFontSize = BFS_LARGE;
  mgLSBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgLSBack.mg_pmgUp = &mgLSArrowDn;
  mgLSBack.mg_pmgDown = &mgLSArrowUp;
  mgLSBack.mg_strText = TRANS("BACK");
  mgLSBack.mg_strTip = TRANS("Return to main menu");
  gm_lhGadgets.AddTail(mgLSBack.mg_lnNode);
  mgLSBack.mg_pActivatedFunction = &MenuBack;
  mgLSBack.mg_iCenterI = -1;
}

int qsort_CompareFileInfos_NameUp(const void *elem1, const void *elem2 )
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return strcmp(fi1.fi_strName, fi2.fi_strName);
}

int qsort_CompareFileInfos_NameDn(const void *elem1, const void *elem2 )
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return -strcmp(fi1.fi_strName, fi2.fi_strName);
}

int qsort_CompareFileInfos_FileUp(const void *elem1, const void *elem2 )
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return strcmp(fi1.fi_fnFile, fi2.fi_fnFile);
}

int qsort_CompareFileInfos_FileDn(const void *elem1, const void *elem2 )
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return -strcmp(fi1.fi_fnFile, fi2.fi_fnFile);
}

void CLoadSaveMenu::StartMenu(void)
{
  gm_bNoEscape = FALSE;

  // delete all file infos
  FORDELETELIST( CFileInfo, fi_lnNode, gm_lhFileInfos, itfi) {
    delete &itfi.Current();
  }

  // list the directory
  CDynamicStackArray<CTFileName> afnmDir;
  MakeDirList(afnmDir, gm_fnmDirectory, CTString(""), 0);
  gm_iLastFile = -1;

  // for each file in the directory
  for (INDEX i=0; i<afnmDir.Count(); i++) {
    CTFileName fnm = afnmDir[i];
  
    // if it can be parsed
    CTString strName;
    if (ParseFile(fnm, strName)) {
      // create new info for that file
      CFileInfo *pfi = new CFileInfo;
      pfi->fi_fnFile = fnm;
      pfi->fi_strName = strName;
      // add it to list
      gm_lhFileInfos.AddTail(pfi->fi_lnNode);
    }
  }

  // sort if needed
  switch (gm_iSortType) {
  default: ASSERT(FALSE);
  case LSSORT_NONE: break;
  case LSSORT_NAMEUP:
    gm_lhFileInfos.Sort(qsort_CompareFileInfos_NameUp, _offsetof(CFileInfo, fi_lnNode));
    break;
  case LSSORT_NAMEDN:
    gm_lhFileInfos.Sort(qsort_CompareFileInfos_NameDn, _offsetof(CFileInfo, fi_lnNode));
    break;
  case LSSORT_FILEUP:
    gm_lhFileInfos.Sort(qsort_CompareFileInfos_FileUp, _offsetof(CFileInfo, fi_lnNode));
    break;
  case LSSORT_FILEDN:
    gm_lhFileInfos.Sort(qsort_CompareFileInfos_FileDn, _offsetof(CFileInfo, fi_lnNode));
    break;
  }

  // if saving
  if (gm_bSave) {
    // add one info as empty slot
    CFileInfo *pfi = new CFileInfo;
    CTString strNumber;
    strNumber.PrintF("%04d", gm_iLastFile+1);
    pfi->fi_fnFile = gm_fnmDirectory+gm_fnmBaseName+strNumber+gm_fnmExt;
    pfi->fi_strName = EMPTYSLOTSTRING;
    // add it to beginning
    gm_lhFileInfos.AddHead(pfi->fi_lnNode);
  }

  // set default parameters for the list
  gm_iListOffset = 0;
  gm_ctListTotal = gm_lhFileInfos.Count();

  // find which one should be selected
  gm_iListWantedItem = 0;
  if (gm_fnmSelected != "") {
    INDEX i = 0;
    FOREACHINLIST(CFileInfo, fi_lnNode, gm_lhFileInfos, itfi) {
      CFileInfo &fi = *itfi;
      if (fi.fi_fnFile==gm_fnmSelected) {
        gm_iListWantedItem = i;
        break;  
      }
      i++;
    }
  }

  CGameMenu::StartMenu();
}

void CLoadSaveMenu::EndMenu(void)
{
  // delete all file infos
  FORDELETELIST( CFileInfo, fi_lnNode, gm_lhFileInfos, itfi) {
    delete &itfi.Current();
  }
  gm_pgmNextMenu = NULL;
  CGameMenu::EndMenu();
}

void CLoadSaveMenu::FillListItems(void)
{
  // disable all items first
  for(INDEX i=0; i<SAVELOAD_BUTTONS_CT; i++) {
    amgLSButton[i].mg_bEnabled = FALSE;
    amgLSButton[i].mg_strText = TRANS("<empty>");
    amgLSButton[i].mg_strTip = "";
    amgLSButton[i].mg_iInList = -2;
  }

  BOOL bHasFirst = FALSE;
  BOOL bHasLast = FALSE;
  INDEX ctLabels = gm_lhFileInfos.Count();
  INDEX iLabel=0;
  FOREACHINLIST(CFileInfo, fi_lnNode, gm_lhFileInfos, itfi) {
    CFileInfo &fi = *itfi;
    INDEX iInMenu = iLabel-gm_iListOffset;
    if( (iLabel>=gm_iListOffset) && 
        (iLabel<(gm_iListOffset+SAVELOAD_BUTTONS_CT)) )
    {
      bHasFirst|=(iLabel==0);
      bHasLast |=(iLabel==ctLabels-1);
      amgLSButton[iInMenu].mg_iInList = iLabel;
      amgLSButton[iInMenu].mg_strDes  = fi.fi_strName;
      amgLSButton[iInMenu].mg_fnm  = fi.fi_fnFile;
      amgLSButton[iInMenu].mg_bEnabled = TRUE;
      amgLSButton[iInMenu].RefreshText();
      if (gm_bSave) {
        if (!FileExistsForWriting(amgLSButton[iInMenu].mg_fnm)) {
          amgLSButton[iInMenu].mg_strTip = TRANS("[Enter] - Save");
        } else {
          amgLSButton[iInMenu].mg_strTip = TRANS("[Enter] - Save, [F2] - Rename, [Del] - Delete");
        }
      } else if (gm_bManage) {
        amgLSButton[iInMenu].mg_strTip = TRANS("[Enter] - Load, [F2] - Rename, [Del] - Delete");
      } else {
        amgLSButton[iInMenu].mg_strTip = TRANS("[Enter] - Load");
      }
    }
    iLabel++;
  }

  // enable/disable up/down arrows
  mgLSArrowUp.mg_bEnabled = !bHasFirst && ctLabels>0;
  mgLSArrowDn.mg_bEnabled = !bHasLast  && ctLabels>0;
}

// called to get info of a file from directory, or to skip it
BOOL CLoadSaveMenu::ParseFile(const CTFileName &fnm, CTString &strName)
{
  if (fnm.FileExt()!=gm_fnmExt) {
    return FALSE;
  }
  CTFileName fnSaveGameDescription = fnm.NoExt()+".des";
  try {
    strName.Load_t( fnSaveGameDescription);
  } catch (const char *strError) {
    (void)strError;
    strName = fnm.FileName();

    if (fnm.FileExt()==".ctl") {
      INDEX iCtl = -1;
      strName.ScanF("Controls%d", &iCtl);
      if (iCtl>=0 && iCtl<=7) {
        strName.PrintF(TRANSV("From player: %s"), (const char *) (_pGame->gm_apcPlayers[iCtl].GetNameForPrinting()));
      }
    }
  }

  INDEX iFile = -1;
  fnm.FileName().ScanF((const char*)(gm_fnmBaseName+"%d"), &iFile);

  gm_iLastFile = Max(gm_iLastFile, iFile);

  return TRUE;
}

void CHighScoreMenu::Initialize_t(void)
{
  mgHScore.mg_boxOnScreen = FLOATaabbox2D(FLOAT2D(0,0), FLOAT2D(1,0.5));
  gm_lhGadgets.AddTail( mgHScore.mg_lnNode);

  mgHighScoreTitle.mg_strText = TRANS("HIGH SCORE TABLE");
  mgHighScoreTitle.mg_boxOnScreen = BoxTitle(0.0f);
  gm_lhGadgets.AddTail( mgHighScoreTitle.mg_lnNode);
}

void CHighScoreMenu::StartMenu(void)
{
  gm_pgmParentMenu = pgmCurrentMenu;
  CGameMenu::StartMenu();
}

// ------------------------ CCustomizeKeyboardMenu implementation
void CCustomizeKeyboardMenu::FillListItems(void)
{
  // disable all items first
  for(INDEX i=0; i<KEYS_ON_SCREEN; i++) {
    mgKey[i].mg_bEnabled = FALSE;
    mgKey[i].mg_iInList = -2;
  }

  BOOL bHasFirst = FALSE;
  BOOL bHasLast = FALSE;
  // set diks to key buttons
  INDEX iLabel=0;
  INDEX ctLabels = _pGame->gm_ctrlControlsExtra->ctrl_lhButtonActions.Count();
  FOREACHINLIST( CButtonAction, ba_lnNode, _pGame->gm_ctrlControlsExtra->ctrl_lhButtonActions, itAct)
  {
    INDEX iInMenu = iLabel-gm_iListOffset;
    if( (iLabel>=gm_iListOffset) && 
        (iLabel<(gm_iListOffset+gm_ctListVisible)) )
    {
      bHasFirst|=(iLabel==0);
      bHasLast |=(iLabel==ctLabels-1);
      mgKey[iInMenu].mg_strLabel = TranslateConst(itAct->ba_strName, 0);
      mgKey[iInMenu].mg_iControlNumber = iLabel;
      mgKey[iInMenu].SetBindingNames(FALSE);
      mgKey[iInMenu].mg_strTip = TRANS("[Enter] - Change, [Backspace] - Unbind");
      mgKey[iInMenu].mg_bEnabled = TRUE;
      mgKey[iInMenu].mg_iInList = iLabel;
    }
    iLabel++;
  }

  // enable/disable up/down arrows
  mgCustomizeArrowUp.mg_bEnabled = !bHasFirst && ctLabels>0;
  mgCustomizeArrowDn.mg_bEnabled = !bHasLast  && ctLabels>0;
}

void CCustomizeKeyboardMenu::Initialize_t(void)
{
  // intialize Audio options menu
  mgCustomizeKeyboardTitle.mg_strText = TRANS("# CUSTOMIZE KEYS");
  mgCustomizeKeyboardTitle.mg_boxOnScreen = BoxTitle(0.82f);
  gm_lhGadgets.AddTail(mgCustomizeKeyboardTitle.mg_lnNode);
  mgCustomizeKeyboardTitle.mg_iCenterI = -1;

#define KL_START 3.0f
#define KL_STEEP -1.45f

  for (INDEX iLabel = 0; iLabel < KEYS_ON_SCREEN; iLabel++)
  {
    INDEX iPrev = (gm_ctListVisible + iLabel - 1) % KEYS_ON_SCREEN;
    INDEX iNext = (iLabel + 1) % KEYS_ON_SCREEN;

    // initialize label entities
    mgKey[iLabel].mg_boxOnScreen = BoxKeyLeft(4.71f + iLabel);
    // initialize label gadgets
    mgKey[iLabel].mg_pmgUp = &mgKey[iPrev];
    mgKey[iLabel].mg_pmgDown = &mgKey[iNext];
    mgKey[iLabel].mg_bVisible = TRUE;
    gm_lhGadgets.AddTail(mgKey[iLabel].mg_lnNode);
    mgKey[iLabel].mg_iCenterI = -1;
  }

  // arrows just exist
  gm_lhGadgets.AddTail(mgCustomizeArrowDn.mg_lnNode);
  gm_lhGadgets.AddTail(mgCustomizeArrowUp.mg_lnNode);
  mgCustomizeArrowDn.mg_adDirection = AD_DOWN;
  mgCustomizeArrowUp.mg_adDirection = AD_UP;
  mgCustomizeArrowDn.mg_boxOnScreen = BoxArrow(AD_DOWN);
  mgCustomizeArrowUp.mg_boxOnScreen = BoxArrow(AD_UP);
  mgCustomizeArrowUp.mg_pmgUp = &mgCustomizeBack;
  mgCustomizeArrowUp.mg_pmgDown = &mgKey[0];
  mgCustomizeArrowDn.mg_pmgUp = &mgKey[KEYS_ON_SCREEN - 1];
  mgCustomizeArrowDn.mg_pmgDown = &mgCustomizeBack;

  mgCustomizeBack.mg_bfsFontSize = BFS_LARGE;
  mgCustomizeBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgCustomizeBack.mg_pmgUp = &mgCustomizeArrowDn;
  mgCustomizeBack.mg_pmgDown = &mgCustomizeArrowUp;
  mgCustomizeBack.mg_strText = TRANS("BACK");
  mgCustomizeBack.mg_strTip = TRANS("Return to controls menu");
  gm_lhGadgets.AddTail(mgCustomizeBack.mg_lnNode);
  mgCustomizeBack.mg_pActivatedFunction = &MenuBack;
  mgCustomizeBack.mg_iCenterI = -1;

  gm_ctListVisible = KEYS_ON_SCREEN;
  gm_pmgArrowUp = &mgCustomizeArrowUp;
  gm_pmgArrowDn = &mgCustomizeArrowDn;
  gm_pmgListTop = &mgKey[0];
  gm_pmgListBottom = &mgKey[KEYS_ON_SCREEN - 1];
}

void CCustomizeKeyboardMenu::StartMenu(void)
{
  ControlsMenuOn();
  gm_iListOffset = 0;
  gm_ctListTotal = _pGame->gm_ctrlControlsExtra->ctrl_lhButtonActions.Count();
  gm_iListWantedItem = 0;
  CGameMenu::StartMenu();
}

void CCustomizeKeyboardMenu::EndMenu(void)
{
  ControlsMenuOff();
  CGameMenu::EndMenu();
}

CCustomizeAxisMenu::~CCustomizeAxisMenu(void)
{
  delete[] mgAxisActionTrigger.mg_astrTexts;
  delete[] mgAxisMountedTrigger.mg_astrTexts;
}

void PreChangeAxis(INDEX iDummy)
{
  gmCustomizeAxisMenu.ApplyActionSettings();
}
void PostChangeAxis(INDEX iDummy)
{
  gmCustomizeAxisMenu.ObtainActionSettings();
}

void CCustomizeAxisMenu::Initialize_t(void)
{
  // intialize axis menu
  mgCustomizeAxisTitle.mg_strText = TRANS("# CUSTOMIZE AXIS");
  mgCustomizeAxisTitle.mg_boxOnScreen = BoxTitle(3.26f);
  gm_lhGadgets.AddTail(mgCustomizeAxisTitle.mg_lnNode);
  mgCustomizeAxisTitle.mg_iCenterI = -1;

  mgAxisActionTrigger.mg_pmgUp = &mgAxisSmoothTrigger;
  mgAxisActionTrigger.mg_pmgDown = &mgAxisMountedTrigger;
  mgAxisActionTrigger.mg_boxOnScreen = BoxMediumLeft(8.71f);
  gm_lhGadgets.AddTail(mgAxisActionTrigger.mg_lnNode);
  mgAxisActionTrigger.mg_astrTexts = astrNoYes;
  mgAxisActionTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgAxisActionTrigger.mg_iSelected = 0; 
  mgAxisActionTrigger.mg_strLabel = TRANS("ACTION");
  mgAxisActionTrigger.mg_strValue = astrNoYes[0];
  mgAxisActionTrigger.mg_strTip = TRANS("Select action to customize");
  mgAxisActionTrigger.mg_iCenterI = -1;

  mgAxisMountedTrigger.mg_pmgUp = &mgAxisActionTrigger;
  mgAxisMountedTrigger.mg_pmgDown = &mgAxisSensitivity;
  mgAxisMountedTrigger.mg_boxOnScreen = BoxMediumLeft(9.71f);
  gm_lhGadgets.AddTail(mgAxisMountedTrigger.mg_lnNode);
  mgAxisMountedTrigger.mg_astrTexts = astrNoYes;
  mgAxisMountedTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgAxisMountedTrigger.mg_iSelected = 0; 
  mgAxisMountedTrigger.mg_strLabel = TRANS("MOUNTED TO");
  mgAxisMountedTrigger.mg_strValue = astrNoYes[0];
  mgAxisMountedTrigger.mg_strTip = TRANS("Select axis that will perform the action");
  mgAxisMountedTrigger.mg_iCenterI = -1;
  
  mgAxisActionTrigger.mg_astrTexts = new CTString[AXIS_ACTIONS_CT];
  mgAxisActionTrigger.mg_ctTexts = AXIS_ACTIONS_CT;

  mgAxisActionTrigger.mg_pPreTriggerChange = PreChangeAxis;
  mgAxisActionTrigger.mg_pOnTriggerChange = PostChangeAxis;

  // for all available axis type controlers
  for (INDEX iControler = 0; iControler < AXIS_ACTIONS_CT; iControler++) {
    mgAxisActionTrigger.mg_astrTexts[iControler] = TranslateConst(CTString(_pGame->gm_astrAxisNames[iControler]), 0);
  }

  mgAxisActionTrigger.mg_iSelected = 3;
  
  INDEX ctAxis = _pInput->GetAvailableAxisCount();
  mgAxisMountedTrigger.mg_astrTexts = new CTString[ ctAxis];
  mgAxisMountedTrigger.mg_ctTexts = ctAxis;

  // for all axis actions that can be mounted
  for (INDEX iAxis = 0; iAxis < ctAxis; iAxis++) {
    mgAxisMountedTrigger.mg_astrTexts[iAxis] = _pInput->GetAxisTransName(iAxis);
  }
  
  mgAxisSensitivity.mg_boxOnScreen = BoxMediumLeft(10.71f);
  mgAxisSensitivity.mg_strText = TRANS("SENSITIVITY");
  mgAxisSensitivity.mg_pmgUp = &mgAxisBack;
  mgAxisSensitivity.mg_pmgDown = &mgAxisDeadzone;
  gm_lhGadgets.AddTail(mgAxisSensitivity.mg_lnNode);
  mgAxisSensitivity.mg_strTip = TRANS("Change sensitivity for this axis");
  mgAxisSensitivity.mg_iCenterI = -1;

  mgAxisDeadzone.mg_boxOnScreen = BoxMediumLeft(11.71f);
  mgAxisDeadzone.mg_strText = TRANS("DEAD ZONE");
  mgAxisDeadzone.mg_pmgUp = &mgAxisSensitivity;
  mgAxisDeadzone.mg_pmgDown = &mgAxisInvertTrigger;
  gm_lhGadgets.AddTail(mgAxisDeadzone.mg_lnNode);
  mgAxisDeadzone.mg_strTip = TRANS("Change dead zone for this axis");
  mgAxisDeadzone.mg_iCenterI = -1;

  mgAxisInvertTrigger.mg_pmgUp = &mgAxisDeadzone;
  mgAxisInvertTrigger.mg_pmgDown = &mgAxisRelativeTrigger;
  mgAxisInvertTrigger.mg_boxOnScreen = BoxMediumLeft(12.71f);
  gm_lhGadgets.AddTail(mgAxisInvertTrigger.mg_lnNode);
  mgAxisInvertTrigger.mg_astrTexts = astrNoYes;
  mgAxisInvertTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgAxisInvertTrigger.mg_iSelected = 0;
  mgAxisInvertTrigger.mg_strLabel = TRANS("INVERTED");
  mgAxisInvertTrigger.mg_strValue = astrNoYes[0];
  mgAxisInvertTrigger.mg_strTip = TRANS("Enable to invert this axis");
  mgAxisInvertTrigger.mg_iCenterI = -1;

  mgAxisRelativeTrigger.mg_pmgUp = &mgAxisInvertTrigger;
  mgAxisRelativeTrigger.mg_pmgDown = &mgAxisSmoothTrigger;
  mgAxisRelativeTrigger.mg_boxOnScreen = BoxMediumLeft(13.71f);
  gm_lhGadgets.AddTail(mgAxisRelativeTrigger.mg_lnNode);
  mgAxisRelativeTrigger.mg_astrTexts = astrNoYes;
  mgAxisRelativeTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgAxisRelativeTrigger.mg_iSelected = 0;
  mgAxisRelativeTrigger.mg_strLabel = TRANS("RELATIVE");
  mgAxisRelativeTrigger.mg_strValue = astrNoYes[0];
  mgAxisRelativeTrigger.mg_strTip = TRANS("Select relative or absolute axis reading");
  mgAxisRelativeTrigger.mg_iCenterI = -1;

  mgAxisSmoothTrigger.mg_pmgUp = &mgAxisRelativeTrigger;
  mgAxisSmoothTrigger.mg_pmgDown = &mgAxisActionTrigger;
  mgAxisSmoothTrigger.mg_boxOnScreen = BoxMediumLeft(14.71f);
  gm_lhGadgets.AddTail(mgAxisSmoothTrigger.mg_lnNode);
  mgAxisSmoothTrigger.mg_astrTexts = astrNoYes;
  mgAxisSmoothTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgAxisSmoothTrigger.mg_iSelected = 0;
  mgAxisSmoothTrigger.mg_strLabel = TRANS("SMOOTH");
  mgAxisSmoothTrigger.mg_strValue = astrNoYes[0];
  mgAxisSmoothTrigger.mg_strTip = TRANS("Enable to filter readings on this axis");
  mgAxisSmoothTrigger.mg_iCenterI = -1;

  mgAxisBack.mg_bfsFontSize = BFS_LARGE;
  mgAxisBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgAxisBack.mg_pmgUp = &mgAxisSmoothTrigger;
  mgAxisBack.mg_pmgDown = &mgAxisActionTrigger;
  mgAxisBack.mg_strText = TRANS("BACK");
  mgAxisBack.mg_strTip = TRANS("Return to controls menu");
  gm_lhGadgets.AddTail(mgAxisBack.mg_lnNode);
  mgAxisBack.mg_pActivatedFunction = &MenuBack;
  mgAxisBack.mg_iCenterI = -1;
}

void CCustomizeAxisMenu::ObtainActionSettings(void)
{
  ControlsMenuOn();
  CControls &ctrls = *_pGame->gm_ctrlControlsExtra;
  INDEX iSelectedAction = mgAxisActionTrigger.mg_iSelected;
  INDEX iMountedAxis = ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_iAxisAction;
  
  mgAxisMountedTrigger.mg_iSelected = iMountedAxis;

  mgAxisSensitivity.mg_iMinPos = 0;
  mgAxisSensitivity.mg_iMaxPos = 50;
  mgAxisSensitivity.mg_iCurPos = (INDEX) (ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_fSensitivity/2);
  mgAxisSensitivity.ApplyCurrentPosition();

  mgAxisDeadzone.mg_iMinPos = 0;
  mgAxisDeadzone.mg_iMaxPos = 50;
  mgAxisDeadzone.mg_iCurPos = (INDEX) (ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_fDeadZone/2);
  mgAxisDeadzone.ApplyCurrentPosition();

  mgAxisInvertTrigger.mg_iSelected = ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_bInvert ? 1 : 0;
  mgAxisRelativeTrigger.mg_iSelected = ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_bRelativeControler ? 1 : 0;
  mgAxisSmoothTrigger.mg_iSelected = ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_bSmooth ? 1 : 0;

  mgAxisActionTrigger.ApplyCurrentSelection();
  mgAxisMountedTrigger.ApplyCurrentSelection();
  mgAxisInvertTrigger.ApplyCurrentSelection();
  mgAxisRelativeTrigger.ApplyCurrentSelection();
  mgAxisSmoothTrigger.ApplyCurrentSelection();
}

void CCustomizeAxisMenu::ApplyActionSettings(void)
{
  CControls &ctrls = *_pGame->gm_ctrlControlsExtra;
  INDEX iSelectedAction = mgAxisActionTrigger.mg_iSelected;
  INDEX iMountedAxis = mgAxisMountedTrigger.mg_iSelected;
  FLOAT fSensitivity = 
    FLOAT(mgAxisSensitivity.mg_iCurPos-mgAxisSensitivity.mg_iMinPos) /
    FLOAT(mgAxisSensitivity.mg_iMaxPos-mgAxisSensitivity.mg_iMinPos)*100.0f;
  FLOAT fDeadZone = 
    FLOAT(mgAxisDeadzone.mg_iCurPos-mgAxisDeadzone.mg_iMinPos) /
    FLOAT(mgAxisDeadzone.mg_iMaxPos-mgAxisDeadzone.mg_iMinPos)*100.0f;
  
  BOOL bInvert = mgAxisInvertTrigger.mg_iSelected != 0;
  BOOL bRelative = mgAxisRelativeTrigger.mg_iSelected != 0;
  BOOL bSmooth = mgAxisSmoothTrigger.mg_iSelected != 0;

  ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_iAxisAction = iMountedAxis;
  if (INDEX(ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_fSensitivity)!=INDEX(fSensitivity)) {
    ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_fSensitivity = fSensitivity;
  }
  if (INDEX(ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_fDeadZone)!=INDEX(fDeadZone)) {
    ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_fDeadZone = fDeadZone;
  }
  ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_bInvert = bInvert;
  ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_bRelativeControler = bRelative;
  ctrls.ctrl_aaAxisActions[ iSelectedAction].aa_bSmooth = bSmooth;
  ctrls.CalculateInfluencesForAllAxis();
  ControlsMenuOff();
}

void CCustomizeAxisMenu::StartMenu(void)
{
  ObtainActionSettings();

  CGameMenu::StartMenu();
}

void CCustomizeAxisMenu::EndMenu(void)
{
  ApplyActionSettings();
  CGameMenu::EndMenu();
}

// ------------------------ COptionsMenu implementation
void COptionsMenu::Initialize_t(void)
{
  // intialize options menu
  mgOptionsTitle.mg_boxOnScreen = BoxTitle(3.44f);
  mgOptionsTitle.mg_strText = TRANS("# OPTIONS");
  gm_lhGadgets.AddTail(mgOptionsTitle.mg_lnNode);
  mgOptionsTitle.mg_iCenterI = -1;

  mgOptionsGame.mg_bfsFontSize = BFS_LARGE;
  mgOptionsGame.mg_boxOnScreen = BoxBigLeft(5.51f);
  mgOptionsGame.mg_pmgUp = &mgOptionsBack;
  mgOptionsGame.mg_pmgDown = &mgOptionsVideo;
  mgOptionsGame.mg_strText = TRANS("GAME OPTIONS");
  mgOptionsGame.mg_strTip = TRANS("Change gameplay options");
  gm_lhGadgets.AddTail(mgOptionsGame.mg_lnNode);
  mgOptionsGame.mg_pActivatedFunction = StartSinglePlayerGameOptions;
  mgOptionsGame.mg_iCenterI = -1;

  mgOptionsVideo.mg_bfsFontSize = BFS_LARGE;
  mgOptionsVideo.mg_boxOnScreen = BoxBigLeft(6.51f);
  mgOptionsVideo.mg_pmgUp = &mgOptionsGame;
  mgOptionsVideo.mg_pmgDown = &mgOptionsAudio;
  mgOptionsVideo.mg_strText = TRANS("VIDEO OPTIONS");
  mgOptionsVideo.mg_strTip = TRANS("Change video mode and driver");
  gm_lhGadgets.AddTail(mgOptionsVideo.mg_lnNode);
  mgOptionsVideo.mg_pActivatedFunction = &StartVideoOptionsMenu;
  mgOptionsVideo.mg_iCenterI = -1;

  mgOptionsAudio.mg_bfsFontSize = BFS_LARGE;
  mgOptionsAudio.mg_boxOnScreen = BoxBigLeft(7.51f);
  mgOptionsAudio.mg_pmgUp = &mgOptionsVideo;
  mgOptionsAudio.mg_pmgDown = &mgOptionsControls;
  mgOptionsAudio.mg_strText = TRANS("AUDIO OPTIONS");
  mgOptionsAudio.mg_strTip = TRANS("Change audio quality and volume");
  gm_lhGadgets.AddTail(mgOptionsAudio.mg_lnNode);
  mgOptionsAudio.mg_pActivatedFunction = &StartAudioOptionsMenu;
  mgOptionsAudio.mg_iCenterI = -1;

  mgOptionsControls.mg_bfsFontSize = BFS_LARGE;
  mgOptionsControls.mg_boxOnScreen = BoxBigLeft(8.51f);
  mgOptionsControls.mg_pmgUp = &mgOptionsAudio;
  mgOptionsControls.mg_pmgDown = &mgOptionsBack;
  mgOptionsControls.mg_strText = TRANS("CONTROLS");
  mgOptionsControls.mg_strTip = TRANS("Change controls");
  gm_lhGadgets.AddTail(mgOptionsControls.mg_lnNode);
  mgOptionsControls.mg_pActivatedFunction = &StartControlsMenuFromOptions;
  mgOptionsControls.mg_iCenterI = -1;

  mgOptionsBack.mg_bfsFontSize = BFS_LARGE;
  mgOptionsBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgOptionsBack.mg_pmgUp = &mgOptionsControls;
  mgOptionsBack.mg_pmgDown = &mgOptionsGame;
  mgOptionsBack.mg_strText = TRANS("BACK");
  mgOptionsBack.mg_strTip = TRANS("Return to main menu");
  gm_lhGadgets.AddTail(mgOptionsBack.mg_lnNode);
  mgOptionsBack.mg_pActivatedFunction = &MenuBack;
  mgOptionsBack.mg_iCenterI = -1;
}


// ------------------------ CVideoOptionsMenu implementation
static void FillAspectRatiosList(void)
{
	if (_astrAspectRatioTexts!=NULL)
	{
		delete [] _astrAspectRatioTexts;
	}
	if (_admAspectRatioModes!=NULL)
	{
		delete [] _admAspectRatioModes;
	}

	// if window
    long acount = ARRAYCOUNT(apixAspectRatios);
    _astrAspectRatioTexts = new CTString    [acount];
    _admAspectRatioModes  = new CDisplayMode[acount];
	if( mgFullScreenTrigger.mg_iSelected==0)
	{
	    for(_ctAspectRatios = 0 ; _ctAspectRatios < acount; _ctAspectRatios++)
	    {
			SetAspectRatioInList(_ctAspectRatios, apixAspectRatios[_ctAspectRatios][0], apixAspectRatios[_ctAspectRatios][1]);
	    }
	}else
	{
	    // get resolutions list from engine
	    INDEX rcount;
  		_ctAspectRatios = 0;
    	CDisplayMode *pdm = _pGfx->EnumDisplayModes(rcount, SwitchToAPI(mgDisplayAPITrigger.mg_iSelected), mgDisplayAdaptersTrigger.mg_iSelected);

	    _ctAspectRatios = acount;
        INDEX iAsp;
	    for( iAsp=0 ; iAsp<_ctAspectRatios; iAsp++)
	    {
			SetAspectRatioInList( iAsp, apixAspectRatios[iAsp][0], apixAspectRatios[iAsp][1]);
	    }
	    _ctAspectRatios = iAsp;
	}
	mgAspectRatiosTrigger.mg_astrTexts = _astrAspectRatioTexts;
	mgAspectRatiosTrigger.mg_ctTexts = _ctAspectRatios;

}

static void FillResolutionsList()
{
  // free resolutions
  if (_astrResolutionTexts!=NULL)
  {
    delete [] _astrResolutionTexts;
  }
  if (_admResolutionModes!=NULL)
  {
    delete [] _admResolutionModes;
  }
  _ctResolutions = 0;

  // if window
  	if(1==1 || mgFullScreenTrigger.mg_iSelected==0) {
    // always has fixed resolutions, but not greater than desktop

	PIX aspSizeI, aspSizeJ;
 	AspectRatioToSize(mgAspectRatiosTrigger.mg_iSelected, aspSizeI, aspSizeJ);
    float ratio=(float)aspSizeI / (float)aspSizeJ;

    _ctResolutions = ARRAYCOUNT(apixWidths);
    _astrResolutionTexts = new CTString    [_ctResolutions];
    _admResolutionModes  = new CDisplayMode[_ctResolutions];
    extern PIX _pixDesktopWidth;
    INDEX iRes=0;
    for( ; iRes<_ctResolutions; iRes++)
    {
      if( apixWidths[iRes][0]>_pixDesktopWidth) break;
      apixWidths[iRes][1] = (int)ceil( apixWidths[iRes][0] / ratio);
      if (apixWidths[iRes][1] == 1098) apixWidths[iRes][1] = 1080;
      if (apixWidths[iRes][1] == 769) apixWidths[iRes][1] = 768;
      SetResolutionInList( iRes, apixWidths[iRes][0], apixWidths[iRes][1]);
    }
    _ctResolutions = iRes;

  // if fullscreen
  } else
  {
	PIX aspSizeI, aspSizeJ;
 	AspectRatioToSize(mgAspectRatiosTrigger.mg_iSelected, aspSizeI, aspSizeJ);
    float ratio=(float)aspSizeI / (float)aspSizeJ;

    // get resolutions list from engine
    CDisplayMode *pdm = _pGfx->EnumDisplayModes(_ctResolutions, SwitchToAPI(mgDisplayAPITrigger.mg_iSelected), mgDisplayAdaptersTrigger.mg_iSelected);
    // allocate that much
    _astrResolutionTexts = new CTString    [_ctResolutions];
    _admResolutionModes  = new CDisplayMode[_ctResolutions];
    // for each resolution
    for( INDEX iRes=0; iRes<_ctResolutions; iRes++)
    {
      	// add it to list
   		//allow only resolutions that match selected aspact ratio
   		float sar = (float)pdm[iRes].dm_pixSizeI / (float)pdm[iRes].dm_pixSizeJ;

   		if (sar == ratio)
  		{
   			SetResolutionInList( iRes, pdm[iRes].dm_pixSizeI, pdm[iRes].dm_pixSizeJ);
   		}
    }
  }
  mgResolutionsTrigger.mg_astrTexts = _astrResolutionTexts;
  mgResolutionsTrigger.mg_ctTexts = _ctResolutions;
}


static void FillAdaptersList(void)
{
  if (_astrAdapterTexts!=NULL) {
    delete [] _astrAdapterTexts;
  }
  _ctAdapters = 0;

  INDEX iApi = SwitchToAPI(mgDisplayAPITrigger.mg_iSelected);
  _ctAdapters = _pGfx->gl_gaAPI[iApi].ga_ctAdapters;
  _astrAdapterTexts = new CTString[_ctAdapters];
  for(INDEX iAdapter = 0; iAdapter<_ctAdapters; iAdapter++) {
    _astrAdapterTexts[iAdapter] = _pGfx->gl_gaAPI[iApi].ga_adaAdapter[iAdapter].da_strRenderer;
  }
  mgDisplayAdaptersTrigger.mg_astrTexts = _astrAdapterTexts;
  mgDisplayAdaptersTrigger.mg_ctTexts = _ctAdapters;
}


static void UpdateVideoOptionsButtons(INDEX iSelected)
{
  const BOOL _bVideoOptionsChanged = (iSelected != -1);

  const BOOL bOGLEnabled = _pGfx->HasAPI(GAT_OGL);
#ifdef SE1_D3D
  const BOOL bD3DEnabled = _pGfx->HasAPI(GAT_D3D);
  ASSERT( bOGLEnabled || bD3DEnabled); 
#else //
  ASSERT( bOGLEnabled );
#endif // SE1_D3D
  CDisplayAdapter &da = _pGfx->gl_gaAPI[SwitchToAPI(mgDisplayAPITrigger.mg_iSelected)]
                                .ga_adaAdapter[mgDisplayAdaptersTrigger.mg_iSelected];

  // number of available preferences is higher if video setup is custom
  mgDisplayPrefsTrigger.mg_ctTexts = 3;
  if( sam_iVideoSetup==3) mgDisplayPrefsTrigger.mg_ctTexts++;

  // enumerate adapters
  FillAdaptersList();

  // show or hide buttons
#ifdef SE1_D3D
  mgDisplayAPITrigger.mg_bEnabled = bOGLEnabled && bD3DEnabled;
#else
  mgDisplayAPITrigger.mg_bEnabled = bOGLEnabled;
#endif
  mgDisplayAdaptersTrigger.mg_bEnabled = _ctAdapters>1;
  mgVideoOptionsApply.mg_bEnabled = _bVideoOptionsChanged;
  // determine which should be visible
  mgBorderLessTrigger.mg_bEnabled = TRUE;
  mgFullScreenTrigger.mg_bEnabled = TRUE;
  if( da.da_ulFlags&DAF_FULLSCREENONLY) {
    mgFullScreenTrigger.mg_bEnabled  = FALSE;
    mgBorderLessTrigger.mg_bEnabled  = FALSE;
    mgFullScreenTrigger.mg_iSelected = 1;
    mgBorderLessTrigger.mg_iSelected = 0;
    mgFullScreenTrigger.ApplyCurrentSelection();
    mgBorderLessTrigger.ApplyCurrentSelection();
  }

  if( mgFullScreenTrigger.mg_iSelected==1) {
    mgBorderLessTrigger.mg_iSelected = 0;
    mgBorderLessTrigger.ApplyCurrentSelection();
  }
  if( mgBorderLessTrigger.mg_iSelected==1) {
    mgFullScreenTrigger.mg_iSelected = 0;
    mgFullScreenTrigger.ApplyCurrentSelection();
  }
#ifdef PLATFORM_UNIX
  mgBitsPerPixelTrigger.mg_bEnabled = FALSE;
#else
  mgBitsPerPixelTrigger.mg_bEnabled = TRUE;
#endif
  if( mgFullScreenTrigger.mg_iSelected==0) {
    mgBitsPerPixelTrigger.mg_bEnabled = FALSE;
    mgBitsPerPixelTrigger.mg_iSelected = DepthToSwitch(DD_DEFAULT);
    mgBitsPerPixelTrigger.ApplyCurrentSelection();
  } else if( da.da_ulFlags&DAF_16BITONLY) {
    mgBitsPerPixelTrigger.mg_bEnabled = FALSE;
    mgBitsPerPixelTrigger.mg_iSelected = DepthToSwitch(DD_16BIT);
    mgBitsPerPixelTrigger.ApplyCurrentSelection();
  }

  // remember current selected resolution
  PIX pixSizeI, pixSizeJ;
  PIX aspSizeI, aspSizeJ;
  ResolutionToSize(mgResolutionsTrigger.mg_iSelected, pixSizeI, pixSizeJ);
  AspectRatioToSize(mgAspectRatiosTrigger.mg_iSelected, aspSizeI, aspSizeJ);

  // select same resolution again if possible
  FillAspectRatiosList();
  FillResolutionsList();
  SizeToResolution(pixSizeI, pixSizeJ, aspSizeI, aspSizeJ, mgResolutionsTrigger.mg_iSelected, mgAspectRatiosTrigger.mg_iSelected);

  // apply adapter and resolutions
  mgDisplayAdaptersTrigger.ApplyCurrentSelection();
  mgAspectRatiosTrigger.ApplyCurrentSelection();
  mgResolutionsTrigger.ApplyCurrentSelection();
}


static void InitVideoOptionsButtons(void)
{
  if( sam_bBorderLessActive) {
    sam_bFullScreenActive = FALSE;
  } else  if (sam_bFullScreenActive) {
      sam_bBorderLessActive = FALSE;
  }

  if( sam_bFullScreenActive) {
    mgFullScreenTrigger.mg_iSelected = 1;
    mgBorderLessTrigger.mg_iSelected = 0;
  } else if ( sam_bBorderLessActive ) {
    mgFullScreenTrigger.mg_iSelected = 0;
    mgBorderLessTrigger.mg_iSelected = 1;
  } else {
    mgFullScreenTrigger.mg_iSelected = 0;
    mgBorderLessTrigger.mg_iSelected = 0;
  }

  mgDisplayAPITrigger.mg_iSelected = APIToSwitch((GfxAPIType)(INDEX)sam_iGfxAPI);
  mgDisplayAdaptersTrigger.mg_iSelected = sam_iDisplayAdapter;
  mgBitsPerPixelTrigger.mg_iSelected = DepthToSwitch((enum DisplayDepth)(INDEX)sam_iDisplayDepth);

  FillAspectRatiosList();
  FillResolutionsList();
  SizeToResolution( sam_iScreenSizeI, sam_iScreenSizeJ, sam_iAspectSizeI, sam_iAspectSizeJ, mgResolutionsTrigger.mg_iSelected, mgAspectRatiosTrigger.mg_iSelected);
  mgDisplayPrefsTrigger.mg_iSelected = Clamp(int(sam_iVideoSetup), 0,3);

  mgFullScreenTrigger.ApplyCurrentSelection();
  mgBorderLessTrigger.ApplyCurrentSelection();
  mgDisplayPrefsTrigger.ApplyCurrentSelection();
  mgDisplayAPITrigger.ApplyCurrentSelection();
  mgDisplayAdaptersTrigger.ApplyCurrentSelection();
  mgAspectRatiosTrigger.ApplyCurrentSelection();
  mgResolutionsTrigger.ApplyCurrentSelection();
  mgBitsPerPixelTrigger.ApplyCurrentSelection();
}

void CVideoOptionsMenu::Initialize_t(void)
{
  // intialize video options menu
  mgVideoOptionsTitle.mg_boxOnScreen = BoxTitle(1.48f);
  mgVideoOptionsTitle.mg_strText = TRANS("# VIDEO");
  gm_lhGadgets.AddTail(mgVideoOptionsTitle.mg_lnNode);
  mgVideoOptionsTitle.mg_iCenterI = -1;

  mgDisplayAPITrigger.mg_pmgUp = &mgVideoOptionsBack;
  mgDisplayAPITrigger.mg_pmgDown = &mgDisplayAdaptersTrigger;
  mgDisplayAPITrigger.mg_boxOnScreen = BoxMediumLeft(5.11f);
  gm_lhGadgets.AddTail(mgDisplayAPITrigger.mg_lnNode);
  mgDisplayAPITrigger.mg_astrTexts = astrDisplayAPIRadioTexts;
  mgDisplayAPITrigger.mg_ctTexts = ARRAYCOUNT(astrDisplayAPIRadioTexts);
  mgDisplayAPITrigger.mg_iSelected = 0;
  mgDisplayAPITrigger.mg_strLabel = TRANS("GRAPHIC SYSTEM");
  mgDisplayAPITrigger.mg_strValue = astrDisplayAPIRadioTexts[0];
  mgDisplayAPITrigger.mg_strTip = TRANS("Select graphic system to use");
  mgDisplayAPITrigger.mg_iCenterI = -1;

  mgDisplayAdaptersTrigger.mg_pmgUp = &mgDisplayAPITrigger;
  mgDisplayAdaptersTrigger.mg_pmgDown = &mgDisplayPrefsTrigger;
  mgDisplayAdaptersTrigger.mg_boxOnScreen = BoxMediumLeft(6.11f);
  gm_lhGadgets.AddTail(mgDisplayAdaptersTrigger.mg_lnNode);
  mgDisplayAdaptersTrigger.mg_astrTexts = astrNoYes;
  mgDisplayAdaptersTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgDisplayAdaptersTrigger.mg_iSelected = 0;
  mgDisplayAdaptersTrigger.mg_strLabel = TRANS("DISPLAY ADAPTER");
  mgDisplayAdaptersTrigger.mg_strValue = astrNoYes[0];
  mgDisplayAdaptersTrigger.mg_strTip = TRANS("Select display adapter to use");
  mgDisplayAdaptersTrigger.mg_iCenterI = -1;

  mgDisplayPrefsTrigger.mg_pmgUp = &mgDisplayAdaptersTrigger;
  mgDisplayPrefsTrigger.mg_pmgDown = &mgAspectRatiosTrigger;
  mgDisplayPrefsTrigger.mg_boxOnScreen = BoxMediumLeft(7.11f);
  gm_lhGadgets.AddTail(mgDisplayPrefsTrigger.mg_lnNode);
  mgDisplayPrefsTrigger.mg_astrTexts = astrDisplayPrefsRadioTexts;
  mgDisplayPrefsTrigger.mg_ctTexts = ARRAYCOUNT(astrDisplayPrefsRadioTexts);
  mgDisplayPrefsTrigger.mg_iSelected = 0;
  mgDisplayPrefsTrigger.mg_strLabel = TRANS("PREFERENCES");
  mgDisplayPrefsTrigger.mg_strValue = astrDisplayPrefsRadioTexts[0];
  mgDisplayPrefsTrigger.mg_strTip = TRANS("Balance between speed and rendering quality");
  mgDisplayPrefsTrigger.mg_iCenterI = -1;

  mgAspectRatiosTrigger.mg_pmgUp = &mgDisplayPrefsTrigger;
  mgAspectRatiosTrigger.mg_pmgDown = &mgResolutionsTrigger;
  mgAspectRatiosTrigger.mg_boxOnScreen = BoxMediumLeft(8.11f);
  gm_lhGadgets.AddTail(mgAspectRatiosTrigger.mg_lnNode);
  mgAspectRatiosTrigger.mg_astrTexts = astrNoYes;
  mgAspectRatiosTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgAspectRatiosTrigger.mg_iSelected = 0;
  mgAspectRatiosTrigger.mg_strLabel = TRANS("ASPECT RATIO");
  mgAspectRatiosTrigger.mg_strValue = astrNoYes[0];
  mgAspectRatiosTrigger.mg_strTip = TRANS("Select video aspect ratio");
  mgAspectRatiosTrigger.mg_iCenterI = -1;

  mgResolutionsTrigger.mg_pmgUp = &mgAspectRatiosTrigger;
  mgResolutionsTrigger.mg_pmgDown = &mgFullScreenTrigger;
  mgResolutionsTrigger.mg_boxOnScreen = BoxMediumLeft(9.11f);
  gm_lhGadgets.AddTail(mgResolutionsTrigger.mg_lnNode);
  mgResolutionsTrigger.mg_astrTexts = astrNoYes;
  mgResolutionsTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgResolutionsTrigger.mg_iSelected = 0;
  mgResolutionsTrigger.mg_strLabel = TRANS("RESOLUTION");
  mgResolutionsTrigger.mg_strValue = astrNoYes[0];
  mgResolutionsTrigger.mg_strTip = TRANS("Select video resolution");
  mgResolutionsTrigger.mg_iCenterI = -1;

  mgFullScreenTrigger.mg_pmgUp = &mgResolutionsTrigger;
  mgFullScreenTrigger.mg_pmgDown = &mgBorderLessTrigger;
  mgFullScreenTrigger.mg_boxOnScreen = BoxMediumLeft(10.11f);
  gm_lhGadgets.AddTail(mgFullScreenTrigger.mg_lnNode);
  mgFullScreenTrigger.mg_astrTexts = astrNoYes;
  mgFullScreenTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgFullScreenTrigger.mg_iSelected = 0;
  mgFullScreenTrigger.mg_strLabel = TRANS("FULL SCREEN");
  mgFullScreenTrigger.mg_strValue = astrNoYes[0];
  mgFullScreenTrigger.mg_strTip = TRANS("Run game in a window or in full screen");
  mgFullScreenTrigger.mg_iCenterI = -1;

  mgBorderLessTrigger.mg_pmgUp = &mgFullScreenTrigger; mgBorderLessTrigger.mg_pmgDown = &mgBitsPerPixelTrigger;
  mgBorderLessTrigger.mg_boxOnScreen = BoxMediumLeft(11.11f);
  gm_lhGadgets.AddTail(mgBorderLessTrigger.mg_lnNode);
  mgBorderLessTrigger.mg_astrTexts = astrNoYes;
  mgBorderLessTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgBorderLessTrigger.mg_iSelected = 0;
  mgBorderLessTrigger.mg_strLabel = TRANS("BORDERLESS MODE");
  mgBorderLessTrigger.mg_strValue = astrNoYes[0];
  mgBorderLessTrigger.mg_strTip = TRANS("Run game in a borderless window");
  mgBorderLessTrigger.mg_iCenterI = -1;

  mgBitsPerPixelTrigger.mg_pmgUp = &mgBorderLessTrigger;
  mgBitsPerPixelTrigger.mg_pmgDown = &mgVideoRendering;
  mgBitsPerPixelTrigger.mg_boxOnScreen = BoxMediumLeft(12.11f);
  gm_lhGadgets.AddTail(mgBitsPerPixelTrigger.mg_lnNode);
  mgBitsPerPixelTrigger.mg_astrTexts = astrBitsPerPixelRadioTexts;
  mgBitsPerPixelTrigger.mg_ctTexts = ARRAYCOUNT(astrBitsPerPixelRadioTexts);
  mgBitsPerPixelTrigger.mg_iSelected = 0;
  mgBitsPerPixelTrigger.mg_strLabel = TRANS("BITS PER PIXEL");
  mgBitsPerPixelTrigger.mg_strValue = astrBitsPerPixelRadioTexts[0];
  mgBitsPerPixelTrigger.mg_strTip = TRANS("Select number of colors to use for display");
  mgBitsPerPixelTrigger.mg_iCenterI = -1;

  mgDisplayPrefsTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  mgDisplayAPITrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  mgDisplayAdaptersTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  mgFullScreenTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  mgBorderLessTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  mgAspectRatiosTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  mgResolutionsTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  mgBitsPerPixelTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  
  mgVideoRendering.mg_bfsFontSize = BFS_MEDIUM;
  mgVideoRendering.mg_boxOnScreen = BoxMediumLeft(13.1f);
  mgVideoRendering.mg_pmgUp = &mgBitsPerPixelTrigger;
  mgVideoRendering.mg_pmgDown = &mgVideoOptionsApply;
  mgVideoRendering.mg_strText = TRANS("RENDERING OPTIONS");
  mgVideoRendering.mg_strTip = TRANS("Change rendering settings");
  gm_lhGadgets.AddTail( mgVideoRendering.mg_lnNode);
  mgVideoRendering.mg_pActivatedFunction = &StartRenderingOptionsMenu;
  mgVideoRendering.mg_iCenterI = -1;

  mgVideoOptionsApply.mg_bfsFontSize = BFS_LARGE;
  mgVideoOptionsApply.mg_boxOnScreen = BoxBigLeft(8.51f);
  mgVideoOptionsApply.mg_pmgUp = &mgVideoRendering;
  mgVideoOptionsApply.mg_pmgDown = &mgVideoOptionsBack;
  mgVideoOptionsApply.mg_strText = TRANS("APPLY");
  mgVideoOptionsApply.mg_strTip = TRANS("Apply video settings");
  gm_lhGadgets.AddTail(mgVideoOptionsApply.mg_lnNode);
  mgVideoOptionsApply.mg_pActivatedFunction = &ApplyVideoOptions;
  mgVideoOptionsApply.mg_iCenterI = -1;

  mgVideoOptionsBack.mg_bfsFontSize = BFS_LARGE;
  mgVideoOptionsBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgVideoOptionsBack.mg_pmgUp = &mgVideoOptionsApply;
  mgVideoOptionsBack.mg_pmgDown = &mgDisplayAPITrigger;
  mgVideoOptionsBack.mg_strText = TRANS("BACK");
  mgVideoOptionsBack.mg_strTip = TRANS("Return to options menu");
  gm_lhGadgets.AddTail(mgVideoOptionsBack.mg_lnNode);
  mgVideoOptionsBack.mg_pActivatedFunction = &MenuBack;
  mgVideoOptionsBack.mg_iCenterI = -1;
}


void CVideoOptionsMenu::StartMenu(void)
{
  InitVideoOptionsButtons();

  CGameMenu::StartMenu();

  UpdateVideoOptionsButtons(-1);
}

// ------------------------ CAudioOptionsMenu implementation
static void OnWaveVolumeChange(INDEX iCurPos)
{
  _pShell->SetFLOAT("snd_fSoundVolume", iCurPos/FLOAT(VOLUME_STEPS));
}

void WaveSliderChange(void)
{
  if (_bMouseRight) {
    mgWaveVolume.mg_iCurPos+=5;
  } else {
    mgWaveVolume.mg_iCurPos-=5;
  }
  mgWaveVolume.ApplyCurrentPosition();
}

void FrequencyTriggerChange(INDEX iDummy)
{
  sam_bAutoAdjustAudio = 0;
  mgAudioAutoTrigger.mg_iSelected = 0;
  mgAudioAutoTrigger.ApplyCurrentSelection();
}

void MPEGSliderChange(void)
{
  if (_bMouseRight) {
    mgMPEGVolume.mg_iCurPos+=5;
  } else {
    mgMPEGVolume.mg_iCurPos-=5;
  }
  mgMPEGVolume.ApplyCurrentPosition();
}

static void OnMPEGVolumeChange(INDEX iCurPos)
{
  _pShell->SetFLOAT("snd_fMusicVolume", iCurPos/FLOAT(VOLUME_STEPS));
}

void CAudioOptionsMenu::Initialize_t(void)
{
  // intialize audio options menu
  mgAudioOptionsTitle.mg_boxOnScreen = BoxTitle(3.44f);
  mgAudioOptionsTitle.mg_strText = TRANS("# AUDIO");
  gm_lhGadgets.AddTail(mgAudioOptionsTitle.mg_lnNode);
  mgAudioOptionsTitle.mg_iCenterI = -1;

  mgAudioAutoTrigger.mg_pmgUp = &mgAudioOptionsBack;
  mgAudioAutoTrigger.mg_pmgDown = &mgFrequencyTrigger;
  mgAudioAutoTrigger.mg_boxOnScreen = BoxMediumLeft(9.11f);
  gm_lhGadgets.AddTail(mgAudioAutoTrigger.mg_lnNode);
  mgAudioAutoTrigger.mg_astrTexts = astrNoYes;
  mgAudioAutoTrigger.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgAudioAutoTrigger.mg_iSelected = 0;
  mgAudioAutoTrigger.mg_strLabel = TRANS("AUTO-ADJUST");
  mgAudioAutoTrigger.mg_strValue = astrNoYes[0];
  mgAudioAutoTrigger.mg_strTip = TRANS("Adjust quality to fit your system");
  mgAudioAutoTrigger.mg_iCenterI = -1;
  
  mgFrequencyTrigger.mg_pmgUp = &mgAudioAutoTrigger;
  mgFrequencyTrigger.mg_pmgDown = &mgAudioAPITrigger;
  mgFrequencyTrigger.mg_boxOnScreen = BoxMediumLeft(10.11f);
  gm_lhGadgets.AddTail(mgFrequencyTrigger.mg_lnNode);
  mgFrequencyTrigger.mg_astrTexts = astrFrequencyRadioTexts;
  mgFrequencyTrigger.mg_ctTexts = ARRAYCOUNT(astrFrequencyRadioTexts);
  mgFrequencyTrigger.mg_iSelected = 0;
  mgFrequencyTrigger.mg_strLabel = TRANS("FREQUENCY");
  mgFrequencyTrigger.mg_strValue = astrFrequencyRadioTexts[0];
  mgFrequencyTrigger.mg_strTip = TRANS("Select sound quality or turn sound off");
  mgFrequencyTrigger.mg_pOnTriggerChange = FrequencyTriggerChange;
  mgFrequencyTrigger.mg_iCenterI = -1;

  mgAudioAPITrigger.mg_pmgUp = &mgFrequencyTrigger;
  mgAudioAPITrigger.mg_pmgDown = &mgWaveVolume;
  mgAudioAPITrigger.mg_boxOnScreen = BoxMediumLeft(11.11f);
  gm_lhGadgets.AddTail(mgAudioAPITrigger.mg_lnNode);
  mgAudioAPITrigger.mg_astrTexts = astrSoundAPIRadioTexts;
  mgAudioAPITrigger.mg_ctTexts = ARRAYCOUNT(astrSoundAPIRadioTexts);
  mgAudioAPITrigger.mg_iSelected = 0;
  mgAudioAPITrigger.mg_strLabel = TRANS("SOUND SYSTEM");
  mgAudioAPITrigger.mg_strValue = astrSoundAPIRadioTexts[0];
  mgAudioAPITrigger.mg_strTip = TRANS("Select sound system to use");
  mgAudioAPITrigger.mg_pOnTriggerChange = FrequencyTriggerChange;
  mgAudioAPITrigger.mg_iCenterI = -1;

  mgWaveVolume.mg_boxOnScreen = BoxMediumLeft(12.11f);
  mgWaveVolume.mg_strText = TRANS("SOUND EFFECTS VOLUME");
  mgWaveVolume.mg_strTip = TRANS("Change volume of in-game sound effects");
  mgWaveVolume.mg_pmgUp = &mgAudioAPITrigger;
  mgWaveVolume.mg_pmgDown = &mgMPEGVolume;
  mgWaveVolume.mg_pOnSliderChange = &OnWaveVolumeChange;
  mgWaveVolume.mg_pActivatedFunction = WaveSliderChange;
  gm_lhGadgets.AddTail(mgWaveVolume.mg_lnNode);
  mgWaveVolume.mg_iCenterI = -1;

  mgMPEGVolume.mg_boxOnScreen = BoxMediumLeft(13.11f);
  mgMPEGVolume.mg_strText = TRANS("MUSIC VOLUME");
  mgMPEGVolume.mg_strTip = TRANS("Change volume of in-game music");
  mgMPEGVolume.mg_pmgUp = &mgWaveVolume;
  mgMPEGVolume.mg_pmgDown = &mgAudioOptionsApply;
  mgMPEGVolume.mg_pOnSliderChange = &OnMPEGVolumeChange;
  mgMPEGVolume.mg_pActivatedFunction = MPEGSliderChange;
  gm_lhGadgets.AddTail(mgMPEGVolume.mg_lnNode);
  mgMPEGVolume.mg_iCenterI = -1;

  mgAudioOptionsApply.mg_bfsFontSize = BFS_LARGE;
  mgAudioOptionsApply.mg_boxOnScreen = BoxBigLeft(8.51f);
  mgAudioOptionsApply.mg_strText = TRANS("APPLY");
  mgAudioOptionsApply.mg_strTip = TRANS("Apply audio settings");
  gm_lhGadgets.AddTail( mgAudioOptionsApply.mg_lnNode);
  mgAudioOptionsApply.mg_pmgUp = &mgMPEGVolume;
  mgAudioOptionsApply.mg_pmgDown = &mgAudioOptionsBack;
  mgAudioOptionsApply.mg_pActivatedFunction = &ApplyAudioOptions;
  mgAudioOptionsApply.mg_iCenterI = -1;

  mgAudioOptionsBack.mg_bfsFontSize = BFS_LARGE;
  mgAudioOptionsBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgAudioOptionsBack.mg_strText = TRANS("BACK");
  mgAudioOptionsBack.mg_strTip = TRANS("Return to options menu");
  gm_lhGadgets.AddTail(mgAudioOptionsBack.mg_lnNode);
  mgAudioOptionsBack.mg_pmgUp = &mgAudioOptionsApply;
  mgAudioOptionsBack.mg_pmgDown = &mgAudioAutoTrigger;
  mgAudioOptionsBack.mg_pActivatedFunction = &MenuBack;
  mgAudioOptionsBack.mg_iCenterI = -1;
}

void CAudioOptionsMenu::StartMenu(void)
{
  RefreshSoundFormat();
  CGameMenu::StartMenu();
}

// ------------------------ CLevelsMenu implementation
void CLevelsMenu::FillListItems(void)
{
  // disable all items first
  for(INDEX i=0; i<LEVELS_ON_SCREEN; i++) {
    mgManualLevel[i].mg_bEnabled = FALSE;
    mgManualLevel[i].mg_strText = TRANS("<empty>");
    mgManualLevel[i].mg_iInList = -2;
  }

  BOOL bHasFirst = FALSE;
  BOOL bHasLast = FALSE;
  INDEX ctLabels = _lhFilteredLevels.Count();
  INDEX iLabel=0;
  FOREACHINLIST(CLevelInfo, li_lnNode, _lhFilteredLevels, itli) {
    CLevelInfo &li = *itli;
    INDEX iInMenu = iLabel-gm_iListOffset;
    if( (iLabel>=gm_iListOffset) && 
        (iLabel<(gm_iListOffset+LEVELS_ON_SCREEN)) )
    {
      bHasFirst|=(iLabel==0);
      bHasLast |=(iLabel==ctLabels-1);
      mgManualLevel[iInMenu].mg_strText  = li.li_strName;
      mgManualLevel[iInMenu].mg_fnmLevel = li.li_fnLevel;
      mgManualLevel[iInMenu].mg_bEnabled = TRUE;
      mgManualLevel[iInMenu].mg_iInList = iLabel;
    }
    iLabel++;
  }

  // enable/disable up/down arrows
  mgLevelsArrowUp.mg_bEnabled = !bHasFirst && ctLabels>0;
  mgLevelsArrowDn.mg_bEnabled = !bHasLast  && ctLabels>0;
}

void CLevelsMenu::Initialize_t(void)
{
  mgLevelsTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgLevelsTitle.mg_strText = TRANS("CHOOSE LEVEL");
  gm_lhGadgets.AddTail( mgLevelsTitle.mg_lnNode);

  for( INDEX iLabel=0; iLabel<LEVELS_ON_SCREEN; iLabel++)
  {
    INDEX iPrev = (LEVELS_ON_SCREEN+iLabel-1)%LEVELS_ON_SCREEN;
    INDEX iNext = (iLabel+1)%LEVELS_ON_SCREEN;
    // initialize label gadgets
    mgManualLevel[iLabel].mg_pmgUp = &mgManualLevel[iPrev];
    mgManualLevel[iLabel].mg_pmgDown = &mgManualLevel[iNext];
    mgManualLevel[iLabel].mg_boxOnScreen = BoxMediumRow(iLabel);
    mgManualLevel[iLabel].mg_pActivatedFunction = NULL; // never called!
    gm_lhGadgets.AddTail( mgManualLevel[iLabel].mg_lnNode);
  }

  gm_lhGadgets.AddTail( mgLevelsArrowUp.mg_lnNode);
  gm_lhGadgets.AddTail( mgLevelsArrowDn.mg_lnNode);
  mgLevelsArrowUp.mg_adDirection = AD_UP;
  mgLevelsArrowDn.mg_adDirection = AD_DOWN;
  mgLevelsArrowUp.mg_boxOnScreen = BoxArrow(AD_UP);
  mgLevelsArrowDn.mg_boxOnScreen = BoxArrow(AD_DOWN);
  mgLevelsArrowUp.mg_pmgRight = mgLevelsArrowUp.mg_pmgDown = 
    &mgManualLevel[0];
  mgLevelsArrowDn.mg_pmgRight = mgLevelsArrowDn.mg_pmgUp = 
    &mgManualLevel[LEVELS_ON_SCREEN-1];

  gm_ctListVisible = LEVELS_ON_SCREEN;
  gm_pmgArrowUp = &mgLevelsArrowUp;
  gm_pmgArrowDn = &mgLevelsArrowDn;
  gm_pmgListTop = &mgManualLevel[0];
  gm_pmgListBottom = &mgManualLevel[LEVELS_ON_SCREEN-1];
}
void CLevelsMenu::StartMenu(void)
{
  // set default parameters for the list
  gm_iListOffset = 0;
  gm_ctListTotal = _lhFilteredLevels.Count();
  gm_iListWantedItem = 0;
  // for each level
  INDEX i=0;
  FOREACHINLIST(CLevelInfo, li_lnNode, _lhFilteredLevels, itlid) {
    CLevelInfo &lid = *itlid;
    // if it is the chosen one
    if (lid.li_fnLevel == _pGame->gam_strCustomLevel) {
      // demand focus on it
      gm_iListWantedItem = i;
      break;
    }
    i++;
  }
  CGameMenu::StartMenu();
}

void VarApply(void)
{
  FlushVarSettings(TRUE);
  gmVarMenu.EndMenu();
  gmVarMenu.StartMenu();
}

void CVarMenu::Initialize_t(void)
{
  mgVarTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgVarTitle.mg_strText = "";
  gm_lhGadgets.AddTail( mgVarTitle.mg_lnNode);

  for( INDEX iLabel=0; iLabel<VARS_ON_SCREEN; iLabel++)
  {
    INDEX iPrev = (VARS_ON_SCREEN+iLabel-1)%VARS_ON_SCREEN;
    INDEX iNext = (iLabel+1)%VARS_ON_SCREEN;
    // initialize label gadgets
    mgVar[iLabel].mg_pmgUp = &mgVar[iPrev];
    mgVar[iLabel].mg_pmgDown = &mgVar[iNext];
    mgVar[iLabel].mg_pmgLeft = &mgVarApply;
    mgVar[iLabel].mg_boxOnScreen = BoxMediumRow(iLabel);
    mgVar[iLabel].mg_pActivatedFunction = NULL; // never called!
    gm_lhGadgets.AddTail( mgVar[iLabel].mg_lnNode);
  }

  mgVarApply.mg_boxOnScreen = BoxMediumRow(16.5f);
  mgVarApply.mg_bfsFontSize = BFS_LARGE;
  mgVarApply.mg_iCenterI = 1;
  mgVarApply.mg_pmgLeft = 
  mgVarApply.mg_pmgRight = 
  mgVarApply.mg_pmgUp = 
  mgVarApply.mg_pmgDown = &mgVar[0];
  mgVarApply.mg_strText = TRANS("APPLY");
  mgVarApply.mg_strTip = TRANS("apply changes");
  gm_lhGadgets.AddTail( mgVarApply.mg_lnNode);
  mgVarApply.mg_pActivatedFunction = &VarApply;

  gm_lhGadgets.AddTail( mgVarArrowUp.mg_lnNode);
  gm_lhGadgets.AddTail( mgVarArrowDn.mg_lnNode);
  mgVarArrowUp.mg_adDirection = AD_UP;
  mgVarArrowDn.mg_adDirection = AD_DOWN;
  mgVarArrowUp.mg_boxOnScreen = BoxArrow(AD_UP);
  mgVarArrowDn.mg_boxOnScreen = BoxArrow(AD_DOWN);
  mgVarArrowUp.mg_pmgRight = mgVarArrowUp.mg_pmgDown = 
    &mgVar[0];
  mgVarArrowDn.mg_pmgRight = mgVarArrowDn.mg_pmgUp = 
    &mgVar[VARS_ON_SCREEN-1];

  gm_ctListVisible = VARS_ON_SCREEN;
  gm_pmgArrowUp = &mgVarArrowUp;
  gm_pmgArrowDn = &mgVarArrowDn;
  gm_pmgListTop = &mgVar[0];
  gm_pmgListBottom = &mgVar[VARS_ON_SCREEN-1];
}

void CVarMenu::FillListItems(void)
{
  // disable all items first
  for(INDEX i=0; i<VARS_ON_SCREEN; i++) {
    mgVar[i].mg_bEnabled = FALSE;
    mgVar[i].mg_pvsVar = NULL;
    mgVar[i].mg_iInList = -2;
  }
  BOOL bHasFirst = FALSE;
  BOOL bHasLast = FALSE;
  INDEX ctLabels = _lhVarSettings.Count();
  INDEX iLabel=0;

  FOREACHINLIST(CVarSetting, vs_lnNode, _lhVarSettings, itvs) {
    CVarSetting &vs = *itvs;
    INDEX iInMenu = iLabel-gm_iListOffset;
    if( (iLabel>=gm_iListOffset) && 
        (iLabel<(gm_iListOffset+VARS_ON_SCREEN)) )
    {
      bHasFirst|=(iLabel==0);
      bHasLast |=(iLabel==ctLabels-1);
      mgVar[iInMenu].mg_pvsVar = &vs;
      mgVar[iInMenu].mg_strTip = vs.vs_strTip;
      mgVar[iInMenu].mg_bEnabled = mgVar[iInMenu].IsEnabled();
      mgVar[iInMenu].mg_iInList = iLabel;
    }
    iLabel++;
  }
  // enable/disable up/down arrows
  mgVarArrowUp.mg_bEnabled = !bHasFirst && ctLabels>0;
  mgVarArrowDn.mg_bEnabled = !bHasLast  && ctLabels>0;
}

void CVarMenu::StartMenu(void)
{
  LoadVarSettings(gm_fnmMenuCFG);
  // set default parameters for the list
  gm_iListOffset = 0;
  gm_ctListTotal = _lhVarSettings.Count();
  gm_iListWantedItem = 0;
  CGameMenu::StartMenu();
}

void CVarMenu::EndMenu(void)
{
  // disable all items first
  for(INDEX i=0; i<VARS_ON_SCREEN; i++) {
    mgVar[i].mg_bEnabled = FALSE;
    mgVar[i].mg_pvsVar = NULL;
    mgVar[i].mg_iInList = -2;
  }
  FlushVarSettings(FALSE);
  _bVarChanged = FALSE;
}

void CVarMenu::Think(void)
{
  mgVarApply.mg_bEnabled = _bVarChanged;
}

// ------------------------ CServersMenu implementation
void RefreshServerList(void)
{
  _pNetwork->EnumSessions(gmServersMenu.m_bInternet);
}

void RefreshServerListManually(void)
{
  ChangeToMenu(&gmServersMenu); // this refreshes the list and sets focuses
}

void CServersMenu::Think(void)
{
  if (!_pNetwork->ga_bEnumerationChange) {
    return;
  }
  _pNetwork->ga_bEnumerationChange = FALSE;
}

CTString _strServerFilter[7];

void SortByColumn(int i)
{
  if (mgServerList.mg_iSort==i) {
    mgServerList.mg_bSortDown = !mgServerList.mg_bSortDown;
  } else {
    mgServerList.mg_bSortDown = FALSE;
  }
  mgServerList.mg_iSort=i;
}

void SortByServer(void) { SortByColumn(0); }
void SortByMap(void)    { SortByColumn(1); }
void SortByPing(void)   { SortByColumn(2); }
void SortByPlayers(void){ SortByColumn(3); }
void SortByGame(void)   { SortByColumn(4); }
void SortByMod(void)    { SortByColumn(5); }
void SortByVer(void)    { SortByColumn(6); }

void CServersMenu::Initialize_t(void)
{
  mgServersTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgServersTitle.mg_strText = TRANS("CHOOSE SERVER");
  gm_lhGadgets.AddTail( mgServersTitle.mg_lnNode);

  mgServerList.mg_boxOnScreen = FLOATaabbox2D(FLOAT2D(0,0), FLOAT2D(1,1));
  mgServerList.mg_pmgLeft = &mgServerList;  // make sure it can get focus
  mgServerList.mg_bEnabled = TRUE;
  gm_lhGadgets.AddTail(mgServerList.mg_lnNode);

  ASSERT(ARRAYCOUNT(mgServerColumn)==ARRAYCOUNT(mgServerFilter));
  for (INDEX i=0; i < static_cast<INDEX>(ARRAYCOUNT(mgServerFilter)); i++) {
    mgServerColumn[i].mg_strText = "";
    mgServerColumn[i].mg_boxOnScreen = BoxPlayerEdit(5.0);
    mgServerColumn[i].mg_bfsFontSize = BFS_SMALL;
    mgServerColumn[i].mg_iCenterI = -1;
    mgServerColumn[i].mg_pmgUp = &mgServerList;
    mgServerColumn[i].mg_pmgDown = &mgServerFilter[i];
    gm_lhGadgets.AddTail( mgServerColumn[i].mg_lnNode);

    mgServerFilter[i].mg_ctMaxStringLen = 25;
    mgServerFilter[i].mg_boxOnScreen = BoxPlayerEdit(5.0);
    mgServerFilter[i].mg_bfsFontSize = BFS_SMALL;
    mgServerFilter[i].mg_iCenterI = -1;
    mgServerFilter[i].mg_pmgUp = &mgServerColumn[i];
    mgServerFilter[i].mg_pmgDown = &mgServerList;
    gm_lhGadgets.AddTail( mgServerFilter[i].mg_lnNode);
    mgServerFilter[i].mg_pstrToChange = &_strServerFilter[i];
    mgServerFilter[i].SetText( *mgServerFilter[i].mg_pstrToChange);
  }

  mgServerRefresh.mg_strText = TRANS("REFRESH");
  mgServerRefresh.mg_boxOnScreen = BoxLeftColumn(15.0);
  mgServerRefresh.mg_bfsFontSize = BFS_SMALL;
  mgServerRefresh.mg_iCenterI = -1;
  mgServerRefresh.mg_pmgDown = &mgServerList;
  mgServerRefresh.mg_pActivatedFunction = &RefreshServerList;
  gm_lhGadgets.AddTail( mgServerRefresh.mg_lnNode);

  //CTString astrColumns[7];
  mgServerColumn[0].mg_strText = TRANS("Server") ;
  mgServerColumn[1].mg_strText = TRANS("Map")    ;
  mgServerColumn[2].mg_strText = TRANS("Ping")   ;
  mgServerColumn[3].mg_strText = TRANS("Players");
  mgServerColumn[4].mg_strText = TRANS("Game")   ;
  mgServerColumn[5].mg_strText = TRANS("Mod")    ;
  mgServerColumn[6].mg_strText = TRANS("Ver")    ;
  mgServerColumn[0].mg_pActivatedFunction = SortByServer;
  mgServerColumn[1].mg_pActivatedFunction = SortByMap;
  mgServerColumn[2].mg_pActivatedFunction = SortByPing;
  mgServerColumn[3].mg_pActivatedFunction = SortByPlayers;
  mgServerColumn[4].mg_pActivatedFunction = SortByGame;
  mgServerColumn[5].mg_pActivatedFunction = SortByMod;
  mgServerColumn[6].mg_pActivatedFunction = SortByVer;
  mgServerColumn[0].mg_strTip = TRANS("sort by server") ;
  mgServerColumn[1].mg_strTip = TRANS("sort by map")    ;
  mgServerColumn[2].mg_strTip = TRANS("sort by ping")   ;
  mgServerColumn[3].mg_strTip = TRANS("sort by players");
  mgServerColumn[4].mg_strTip = TRANS("sort by game")   ;
  mgServerColumn[5].mg_strTip = TRANS("sort by mod")    ;
  mgServerColumn[6].mg_strTip = TRANS("sort by version");
  mgServerFilter[0].mg_strTip = TRANS("filter by server");
  mgServerFilter[1].mg_strTip = TRANS("filter by map")    ;
  mgServerFilter[2].mg_strTip = TRANS("filter by ping (ie. <200)")   ;
  mgServerFilter[3].mg_strTip = TRANS("filter by players (ie. >=2)");
  mgServerFilter[4].mg_strTip = TRANS("filter by game (ie. coop)")   ;
  mgServerFilter[5].mg_strTip = TRANS("filter by mod")    ;
  mgServerFilter[6].mg_strTip = TRANS("filter by version");
}

void CServersMenu::StartMenu(void)
{
  RefreshServerList();

  CGameMenu::StartMenu();
}

// ------------------------ CNetworkMenu implementation
void CNetworkMenu::Initialize_t(void)
{
  // intialize network menu
  mgNetworkTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgNetworkTitle.mg_strText = TRANS("NETWORK");
  gm_lhGadgets.AddTail( mgNetworkTitle.mg_lnNode);

  mgNetworkJoin.mg_bfsFontSize = BFS_LARGE;
  mgNetworkJoin.mg_boxOnScreen = BoxBigRow(1.0f);
  mgNetworkJoin.mg_pmgUp = &mgNetworkLoad;
  mgNetworkJoin.mg_pmgDown = &mgNetworkStart;
  mgNetworkJoin.mg_strText = TRANS("JOIN GAME");
  mgNetworkJoin.mg_strTip = TRANS("join a network game");
  gm_lhGadgets.AddTail( mgNetworkJoin.mg_lnNode);
  mgNetworkJoin.mg_pActivatedFunction = &StartNetworkJoinMenu;

  mgNetworkStart.mg_bfsFontSize = BFS_LARGE;
  mgNetworkStart.mg_boxOnScreen = BoxBigRow(2.0f);
  mgNetworkStart.mg_pmgUp = &mgNetworkJoin;
  mgNetworkStart.mg_pmgDown = &mgNetworkQuickLoad;
  mgNetworkStart.mg_strText = TRANS("START SERVER");
  mgNetworkStart.mg_strTip = TRANS("start a network game server");
  gm_lhGadgets.AddTail( mgNetworkStart.mg_lnNode);
  mgNetworkStart.mg_pActivatedFunction = &StartNetworkStartMenu;

  mgNetworkQuickLoad.mg_bfsFontSize = BFS_LARGE;
  mgNetworkQuickLoad.mg_boxOnScreen = BoxBigRow(3.0f);
  mgNetworkQuickLoad.mg_pmgUp = &mgNetworkStart;
  mgNetworkQuickLoad.mg_pmgDown = &mgNetworkLoad;
  mgNetworkQuickLoad.mg_strText = TRANS("QUICK LOAD");
  mgNetworkQuickLoad.mg_strTip = TRANS("load a quick-saved game (F9)");
  gm_lhGadgets.AddTail( mgNetworkQuickLoad.mg_lnNode);
  mgNetworkQuickLoad.mg_pActivatedFunction = &StartNetworkQuickLoadMenu;

  mgNetworkLoad.mg_bfsFontSize = BFS_LARGE;
  mgNetworkLoad.mg_boxOnScreen = BoxBigRow(4.0f);
  mgNetworkLoad.mg_pmgUp = &mgNetworkQuickLoad;
  mgNetworkLoad.mg_pmgDown = &mgNetworkJoin;
  mgNetworkLoad.mg_strText = TRANS("LOAD");
  mgNetworkLoad.mg_strTip = TRANS("start server and load a network game (server only)");
  gm_lhGadgets.AddTail( mgNetworkLoad.mg_lnNode);
  mgNetworkLoad.mg_pActivatedFunction = &StartNetworkLoadMenu;

}
void CNetworkMenu::StartMenu(void)
{
  CGameMenu::StartMenu();
}

void UpdateNetworkLevel(INDEX iDummy)
{
  ValidateLevelForFlags(_pGame->gam_strCustomLevel, 
    SpawnFlagsForGameType(mgNetworkGameType.mg_iSelected));
  mgNetworkLevel.mg_strText = FindLevelByFileName(_pGame->gam_strCustomLevel).li_strName;
}

// ------------------------ CNetworkJoinMenu implementation
void CNetworkJoinMenu::Initialize_t(void)
{
  // title
  mgNetworkJoinTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgNetworkJoinTitle.mg_strText = TRANS("JOIN GAME");
  gm_lhGadgets.AddTail( mgNetworkJoinTitle.mg_lnNode);

  mgNetworkJoinLAN.mg_bfsFontSize = BFS_LARGE;
  mgNetworkJoinLAN.mg_boxOnScreen = BoxBigRow(1.0f);
  mgNetworkJoinLAN.mg_pmgUp = &mgNetworkJoinOpen;
  mgNetworkJoinLAN.mg_pmgDown = &mgNetworkJoinNET;
  mgNetworkJoinLAN.mg_strText = TRANS("SEARCH LAN");
  mgNetworkJoinLAN.mg_strTip = TRANS("search local network for servers");
  gm_lhGadgets.AddTail( mgNetworkJoinLAN.mg_lnNode);
  mgNetworkJoinLAN.mg_pActivatedFunction = &StartSelectServerLAN;

  mgNetworkJoinNET.mg_bfsFontSize = BFS_LARGE;
  mgNetworkJoinNET.mg_boxOnScreen = BoxBigRow(2.0f);
  mgNetworkJoinNET.mg_pmgUp = &mgNetworkJoinLAN;
  mgNetworkJoinNET.mg_pmgDown = &mgNetworkJoinOpen;
  mgNetworkJoinNET.mg_strText = TRANS("SEARCH INTERNET");
  mgNetworkJoinNET.mg_strTip = TRANS("search internet for servers");
  gm_lhGadgets.AddTail( mgNetworkJoinNET.mg_lnNode);
  mgNetworkJoinNET.mg_pActivatedFunction = &StartSelectServerNET;

  mgNetworkJoinOpen.mg_bfsFontSize = BFS_LARGE;
  mgNetworkJoinOpen.mg_boxOnScreen = BoxBigRow(3.0f);
  mgNetworkJoinOpen.mg_pmgUp = &mgNetworkJoinNET;
  mgNetworkJoinOpen.mg_pmgDown = &mgNetworkJoinLAN;
  mgNetworkJoinOpen.mg_strText = TRANS("SPECIFY SERVER");
  mgNetworkJoinOpen.mg_strTip = TRANS("type in server address to connect to");
  gm_lhGadgets.AddTail( mgNetworkJoinOpen.mg_lnNode);
  mgNetworkJoinOpen.mg_pActivatedFunction = &StartNetworkOpenMenu;
}

// ------------------------ CNetworkStartMenu implementation
void CNetworkStartMenu::Initialize_t(void)
{
  // title
  mgNetworkStartTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgNetworkStartTitle.mg_strText = TRANS("START SERVER");
  gm_lhGadgets.AddTail( mgNetworkStartTitle.mg_lnNode);

  // session name edit box
  mgNetworkSessionName.mg_strText = _pGame->gam_strSessionName;
  mgNetworkSessionName.mg_strLabel = TRANS("Session name:");
  mgNetworkSessionName.mg_ctMaxStringLen = 25;
  mgNetworkSessionName.mg_pstrToChange = &_pGame->gam_strSessionName;
  mgNetworkSessionName.mg_boxOnScreen = BoxMediumRow(1);
  mgNetworkSessionName.mg_bfsFontSize = BFS_MEDIUM;
  mgNetworkSessionName.mg_iCenterI = -1;
  mgNetworkSessionName.mg_pmgUp = &mgNetworkStartStart;
  mgNetworkSessionName.mg_pmgDown = &mgNetworkGameType;
  mgNetworkSessionName.mg_strTip = TRANS("name the session to start");
  gm_lhGadgets.AddTail( mgNetworkSessionName.mg_lnNode);

  // game type trigger
  TRIGGER_MG(mgNetworkGameType, 2,
       mgNetworkSessionName, mgNetworkDifficulty, TRANS("Game type:"), astrGameTypeRadioTexts);
  mgNetworkGameType.mg_ctTexts = ctGameTypeRadioTexts;
  mgNetworkGameType.mg_strTip = TRANS("choose type of multiplayer game");
  mgNetworkGameType.mg_pOnTriggerChange = &UpdateNetworkLevel;
  // difficulty trigger
  TRIGGER_MG(mgNetworkDifficulty, 3,
       mgNetworkGameType, mgNetworkLevel, TRANS("Difficulty:"), astrDifficultyRadioTexts);
  mgNetworkDifficulty.mg_strTip = TRANS("choose difficulty level");

  // level name
  mgNetworkLevel.mg_strText = "";
  mgNetworkLevel.mg_strLabel = TRANS("Level:");
  mgNetworkLevel.mg_boxOnScreen = BoxMediumRow(4);
  mgNetworkLevel.mg_bfsFontSize = BFS_MEDIUM;
  mgNetworkLevel.mg_iCenterI = -1;
  mgNetworkLevel.mg_pmgUp = &mgNetworkDifficulty;
  mgNetworkLevel.mg_pmgDown = &mgNetworkMaxPlayers;
  mgNetworkLevel.mg_strTip = TRANS("choose the level to start");
  mgNetworkLevel.mg_pActivatedFunction = &StartSelectLevelFromNetwork;
  gm_lhGadgets.AddTail( mgNetworkLevel.mg_lnNode);

  // max players trigger
  TRIGGER_MG(mgNetworkMaxPlayers, 5,
       mgNetworkLevel, mgNetworkWaitAllPlayers, TRANS("Max players:"), astrMaxPlayersRadioTexts);
  mgNetworkMaxPlayers.mg_strTip = TRANS("choose maximum allowed number of players");

  // wait all players trigger
  TRIGGER_MG(mgNetworkWaitAllPlayers, 6,
       mgNetworkMaxPlayers, mgNetworkVisible, TRANS("Wait for all players:"), astrNoYes);
  mgNetworkWaitAllPlayers.mg_strTip = TRANS("if on, game won't start until all players have joined");

  // server visible trigger
  TRIGGER_MG(mgNetworkVisible, 7,
       mgNetworkMaxPlayers, mgNetworkGameOptions, TRANS("Server visible:"), astrNoYes);
  mgNetworkVisible.mg_strTip = TRANS("invisible servers are not listed, cleints have to join manually");

  // options button
  mgNetworkGameOptions.mg_strText = TRANS("Game options");
  mgNetworkGameOptions.mg_boxOnScreen = BoxMediumRow(8);
  mgNetworkGameOptions.mg_bfsFontSize = BFS_MEDIUM;
  mgNetworkGameOptions.mg_iCenterI = 0;
  mgNetworkGameOptions.mg_pmgUp = &mgNetworkVisible;
  mgNetworkGameOptions.mg_pmgDown = &mgNetworkStartStart;
  mgNetworkGameOptions.mg_strTip = TRANS("adjust game rules");
  mgNetworkGameOptions.mg_pActivatedFunction = &StartGameOptionsFromNetwork;
  gm_lhGadgets.AddTail( mgNetworkGameOptions.mg_lnNode);

  // start button
  mgNetworkStartStart.mg_bfsFontSize = BFS_LARGE;
  mgNetworkStartStart.mg_boxOnScreen = BoxBigRow(7);
  mgNetworkStartStart.mg_pmgUp = &mgNetworkGameOptions;
  mgNetworkStartStart.mg_pmgDown = &mgNetworkSessionName;
  mgNetworkStartStart.mg_strText = TRANS("START");
  gm_lhGadgets.AddTail( mgNetworkStartStart.mg_lnNode);
  mgNetworkStartStart.mg_pActivatedFunction = &StartSelectPlayersMenuFromNetwork;
}

void CNetworkStartMenu::StartMenu(void)
{
  extern INDEX sam_bMentalActivated;
  mgNetworkDifficulty.mg_ctTexts = sam_bMentalActivated?6:5;

  mgNetworkGameType.mg_iSelected = Clamp(_pShell->GetINDEX("gam_iStartMode"), (INDEX)0, ctGameTypeRadioTexts-1);
  mgNetworkGameType.ApplyCurrentSelection();
  mgNetworkDifficulty.mg_iSelected = _pShell->GetINDEX("gam_iStartDifficulty")+1;
  mgNetworkDifficulty.ApplyCurrentSelection();

  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);

  INDEX ctMaxPlayers = _pShell->GetINDEX("gam_ctMaxPlayers");
  if (ctMaxPlayers<2 || ctMaxPlayers>16) {
    ctMaxPlayers = 2;
    _pShell->SetINDEX("gam_ctMaxPlayers", ctMaxPlayers);
  }

  mgNetworkMaxPlayers.mg_iSelected = ctMaxPlayers-2;
  mgNetworkMaxPlayers.ApplyCurrentSelection();

  mgNetworkWaitAllPlayers.mg_iSelected = Clamp(_pShell->GetINDEX("gam_bWaitAllPlayers"), (INDEX)0, (INDEX)1);
  mgNetworkWaitAllPlayers.ApplyCurrentSelection();

  mgNetworkVisible.mg_iSelected = _pShell->GetINDEX("ser_bEnumeration");
  mgNetworkVisible.ApplyCurrentSelection();

  UpdateNetworkLevel(0);

  CGameMenu::StartMenu();
}

void CNetworkStartMenu::EndMenu(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", mgNetworkDifficulty.mg_iSelected-1);
  _pShell->SetINDEX("gam_iStartMode", mgNetworkGameType.mg_iSelected);
  _pShell->SetINDEX("gam_bWaitAllPlayers", mgNetworkWaitAllPlayers.mg_iSelected);
  _pShell->SetINDEX("gam_ctMaxPlayers", mgNetworkMaxPlayers.mg_iSelected+2);
  _pShell->SetINDEX("ser_bEnumeration", mgNetworkVisible.mg_iSelected);

  CGameMenu::EndMenu();
}

#define ADD_GADGET( gd, box, up, dn, lf, rt, txt) \
  gd.mg_boxOnScreen = box;\
  gd.mg_pmgUp = up;\
  gd.mg_pmgDown = dn;\
  gd.mg_pmgLeft = lf;\
  gd.mg_pmgRight = rt;\
  gd.mg_strText = txt;\
  gm_lhGadgets.AddTail( gd.mg_lnNode);

#define SET_CHGPLR( gd, iplayer, bnone, bauto, pmgit) \
  gd.mg_pmgInfoTable = pmgit;\
  gd.mg_bResetToNone = bnone;\
  gd.mg_bAutomatic   = bauto;\
  gd.mg_iLocalPlayer = iplayer;

// ------------------------ CSelectPlayersMenu implementation
INDEX FindUnusedPlayer(void)
{
  INDEX *ai = _pGame->gm_aiMenuLocalPlayers;
  INDEX iPlayer;
  for(iPlayer=0; iPlayer<8; iPlayer++) {
    BOOL bUsed = FALSE;
    for (INDEX iLocal=0; iLocal<4; iLocal++) {
      if (ai[iLocal] == iPlayer) {
        bUsed = TRUE;
        break;
      }
    }
    if (!bUsed) {
      return iPlayer;
    }
  }
  ASSERT(FALSE);
  return iPlayer;
}
void SelectPlayersFillMenu(void)
{
  INDEX *ai = _pGame->gm_aiMenuLocalPlayers;

  mgPlayer0Change.mg_iLocalPlayer = 0;
  mgPlayer1Change.mg_iLocalPlayer = 1;
  mgPlayer2Change.mg_iLocalPlayer = 2;
  mgPlayer3Change.mg_iLocalPlayer = 3;

  if (gmSelectPlayersMenu.gm_bAllowDedicated && _pGame->gm_MenuSplitScreenCfg==CGame::SSC_DEDICATED) {
    mgDedicated.mg_iSelected = 1;
  } else {
    mgDedicated.mg_iSelected = 0;
  }
  mgDedicated.ApplyCurrentSelection();

  if (gmSelectPlayersMenu.gm_bAllowObserving && _pGame->gm_MenuSplitScreenCfg==CGame::SSC_OBSERVER) {
    mgObserver.mg_iSelected = 1;
  } else {
    mgObserver.mg_iSelected = 0;
  }
  mgObserver.ApplyCurrentSelection();

  if (_pGame->gm_MenuSplitScreenCfg>=CGame::SSC_PLAY1) {
    mgSplitScreenCfg.mg_iSelected = _pGame->gm_MenuSplitScreenCfg;
    mgSplitScreenCfg.ApplyCurrentSelection();
  }

  BOOL bHasDedicated = gmSelectPlayersMenu.gm_bAllowDedicated;
  BOOL bHasObserver = gmSelectPlayersMenu.gm_bAllowObserving;
  BOOL bHasPlayers = TRUE;

  if (bHasDedicated && mgDedicated.mg_iSelected) {
    bHasObserver = FALSE;
    bHasPlayers = FALSE;
  }

  if (bHasObserver && mgObserver.mg_iSelected) {
    bHasPlayers = FALSE;
  }

  CMenuGadget *apmg[8];
  memset(apmg, 0, sizeof(apmg));
  INDEX i=0;

  if (bHasDedicated) {
    mgDedicated.Appear();
    apmg[i++] = &mgDedicated;
  } else {
    mgDedicated.Disappear();
  }
  if (bHasObserver) {
    mgObserver.Appear();
    apmg[i++] = &mgObserver;
  } else {
    mgObserver.Disappear();
  }

  for (INDEX iLocal=0; iLocal<4; iLocal++) {
    if (ai[iLocal]<0 || ai[iLocal]>7) {
      ai[iLocal] = 0;
    }
    for (INDEX iCopy=0; iCopy<iLocal; iCopy++) {
      if (ai[iCopy]==ai[iLocal]) {
        ai[iLocal] = FindUnusedPlayer();
      }
    }
  }

  mgPlayer0Change.Disappear();
  mgPlayer1Change.Disappear();
  mgPlayer2Change.Disappear();
  mgPlayer3Change.Disappear();

  if (bHasPlayers) {
    mgSplitScreenCfg.Appear();
    apmg[i++] = &mgSplitScreenCfg;
    mgPlayer0Change.Appear();
    apmg[i++] = &mgPlayer0Change;
    if (mgSplitScreenCfg.mg_iSelected>=1) {
      mgPlayer1Change.Appear();
      apmg[i++] = &mgPlayer1Change;
    }
    if (mgSplitScreenCfg.mg_iSelected>=2) {
      mgPlayer2Change.Appear();
      apmg[i++] = &mgPlayer2Change;
    }
    if (mgSplitScreenCfg.mg_iSelected>=3) {
      mgPlayer3Change.Appear();
      apmg[i++] = &mgPlayer3Change;
    }
  } else {
    mgSplitScreenCfg.Disappear();
  }
  apmg[i++] = &mgSelectPlayersStart;

  // relink
  for (INDEX img=0; img<8; img++) {
    if (apmg[img]==NULL) {
      continue;
    }
    INDEX imgPred;
    for (imgPred=(img+8-1)%8; imgPred!=img; imgPred = (imgPred+8-1)%8) {
      if (apmg[imgPred]!=NULL) {
        break;
      }
    }
    INDEX imgSucc;
    for (imgSucc=(img+1)%8; imgSucc!=img; imgSucc = (imgSucc+1)%8) {
      if (apmg[imgSucc]!=NULL) {
        break;
      }
    }
    apmg[img]->mg_pmgUp   = apmg[imgPred];
    apmg[img]->mg_pmgDown = apmg[imgSucc];
  }

  mgPlayer0Change.SetPlayerText();
  mgPlayer1Change.SetPlayerText();
  mgPlayer2Change.SetPlayerText();
  mgPlayer3Change.SetPlayerText();

  if (bHasPlayers && mgSplitScreenCfg.mg_iSelected>=1) {
    mgSelectPlayersNotes.mg_strText = TRANS("Make sure you set different controls for each player!");
  } else {
    mgSelectPlayersNotes.mg_strText = "";
  }
}
void SelectPlayersApplyMenu(void)
{
  if (gmSelectPlayersMenu.gm_bAllowDedicated && mgDedicated.mg_iSelected) {
    _pGame->gm_MenuSplitScreenCfg = CGame::SSC_DEDICATED;
    return;
  }

  if (gmSelectPlayersMenu.gm_bAllowObserving && mgObserver.mg_iSelected) {
    _pGame->gm_MenuSplitScreenCfg = CGame::SSC_OBSERVER;
    return;
  }

  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
}

void UpdateSelectPlayers(INDEX i)
{
  SelectPlayersApplyMenu();
  SelectPlayersFillMenu();
}

void CSelectPlayersMenu::Initialize_t(void)
{
  // intialize split screen menu
  mgSelectPlayerTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgSelectPlayerTitle.mg_strText = TRANS("SELECT PLAYERS");
  gm_lhGadgets.AddTail( mgSelectPlayerTitle.mg_lnNode);

  TRIGGER_MG(mgDedicated, 0, mgSelectPlayersStart, mgObserver, TRANS("Dedicated:"), astrNoYes);
  mgDedicated.mg_strTip = TRANS("select to start dedicated server");
  mgDedicated.mg_pOnTriggerChange = UpdateSelectPlayers;

  TRIGGER_MG(mgObserver, 1, mgDedicated, mgSplitScreenCfg, TRANS("Observer:"), astrNoYes);
  mgObserver.mg_strTip = TRANS("select to join in for observing, not for playing");
  mgObserver.mg_pOnTriggerChange = UpdateSelectPlayers;

  // split screen config trigger
  TRIGGER_MG(mgSplitScreenCfg, 2, mgObserver, mgPlayer0Change, TRANS("Number of players:"), astrSplitScreenRadioTexts);
  mgSplitScreenCfg.mg_strTip = TRANS("choose more than one player to play in split screen");
  mgSplitScreenCfg.mg_pOnTriggerChange = UpdateSelectPlayers;

  mgPlayer0Change.mg_iCenterI = -1;
  mgPlayer1Change.mg_iCenterI = -1;
  mgPlayer2Change.mg_iCenterI = -1;
  mgPlayer3Change.mg_iCenterI = -1;
  mgPlayer0Change.mg_boxOnScreen = BoxMediumMiddle(4);
  mgPlayer1Change.mg_boxOnScreen = BoxMediumMiddle(5);
  mgPlayer2Change.mg_boxOnScreen = BoxMediumMiddle(6);
  mgPlayer3Change.mg_boxOnScreen = BoxMediumMiddle(7);
  mgPlayer0Change.mg_strTip =
  mgPlayer1Change.mg_strTip =
  mgPlayer2Change.mg_strTip =
  mgPlayer3Change.mg_strTip = TRANS("select profile for this player");
  gm_lhGadgets.AddTail( mgPlayer0Change.mg_lnNode);
  gm_lhGadgets.AddTail( mgPlayer1Change.mg_lnNode);
  gm_lhGadgets.AddTail( mgPlayer2Change.mg_lnNode);
  gm_lhGadgets.AddTail( mgPlayer3Change.mg_lnNode);

  mgSelectPlayersNotes.mg_boxOnScreen = BoxMediumRow(9.0);
  mgSelectPlayersNotes.mg_bfsFontSize = BFS_MEDIUM;
  mgSelectPlayersNotes.mg_iCenterI = -1;
  mgSelectPlayersNotes.mg_bEnabled = FALSE;
  mgSelectPlayersNotes.mg_bLabel = TRUE;
  gm_lhGadgets.AddTail( mgSelectPlayersNotes.mg_lnNode);
  mgSelectPlayersNotes.mg_strText = "";

  /*  // options button
  mgSplitOptions.mg_strText = TRANS("Game options");
  mgSplitOptions.mg_boxOnScreen = BoxMediumRow(3);
  mgSplitOptions.mg_bfsFontSize = BFS_MEDIUM;
  mgSplitOptions.mg_iCenterI = 0;
  mgSplitOptions.mg_pmgUp = &mgSplitLevel;
  mgSplitOptions.mg_pmgDown = &mgSplitStartStart;
  mgSplitOptions.mg_strTip = TRANS("adjust game rules");
  mgSplitOptions.mg_pActivatedFunction = &StartGameOptionsFromSplitScreen;
  gm_lhGadgets.AddTail( mgSplitOptions.mg_lnNode);*/

/*  // start button
  mgSplitStartStart.mg_bfsFontSize = BFS_LARGE;
  mgSplitStartStart.mg_boxOnScreen = BoxBigRow(4);
  mgSplitStartStart.mg_pmgUp = &mgSplitOptions;
  mgSplitStartStart.mg_pmgDown = &mgSplitGameType;
  mgSplitStartStart.mg_strText = TRANS("START");
  gm_lhGadgets.AddTail( mgSplitStartStart.mg_lnNode);
  mgSplitStartStart.mg_pActivatedFunction = &StartSelectPlayersMenuFromSplit;
*/

  ADD_GADGET( mgSelectPlayersStart, BoxMediumRow(11), &mgSplitScreenCfg, &mgPlayer0Change, NULL, NULL, TRANS("START"));
  mgSelectPlayersStart.mg_bfsFontSize = BFS_LARGE;
  mgSelectPlayersStart.mg_iCenterI = 0;
}

void CSelectPlayersMenu::StartMenu(void)
{
  CGameMenu::StartMenu();
  SelectPlayersFillMenu();
  SelectPlayersApplyMenu();
}

void CSelectPlayersMenu::EndMenu(void)
{
  SelectPlayersApplyMenu();
  CGameMenu::EndMenu();
}

CTString _strPort;
// ------------------------ CNetworkOpenMenu implementation
void CNetworkOpenMenu::Initialize_t(void)
{
  // intialize network join menu
  mgNetworkOpenTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgNetworkOpenTitle.mg_strText = TRANS("JOIN");
  gm_lhGadgets.AddTail( mgNetworkOpenTitle.mg_lnNode);

  mgNetworkOpenAddressLabel.mg_strText = TRANS("Address:");
  mgNetworkOpenAddressLabel.mg_boxOnScreen = BoxMediumLeft(1);
  mgNetworkOpenAddressLabel.mg_iCenterI = -1;
  gm_lhGadgets.AddTail( mgNetworkOpenAddressLabel.mg_lnNode);

  mgNetworkOpenAddress.mg_strText = _pGame->gam_strJoinAddress;
  mgNetworkOpenAddress.mg_ctMaxStringLen = 20;
  mgNetworkOpenAddress.mg_pstrToChange = &_pGame->gam_strJoinAddress;
  mgNetworkOpenAddress.mg_boxOnScreen = BoxMediumMiddle(1);
  mgNetworkOpenAddress.mg_bfsFontSize = BFS_MEDIUM;
  mgNetworkOpenAddress.mg_iCenterI = -1;
  mgNetworkOpenAddress.mg_pmgUp = &mgNetworkOpenJoin;
  mgNetworkOpenAddress.mg_pmgDown = &mgNetworkOpenPort;
  mgNetworkOpenAddress.mg_strTip = TRANS("specify server address");
  gm_lhGadgets.AddTail( mgNetworkOpenAddress.mg_lnNode);

  mgNetworkOpenPortLabel.mg_strText = TRANS("Port:");
  mgNetworkOpenPortLabel.mg_boxOnScreen = BoxMediumLeft(2);
  mgNetworkOpenPortLabel.mg_iCenterI = -1;
  gm_lhGadgets.AddTail( mgNetworkOpenPortLabel.mg_lnNode);

  mgNetworkOpenPort.mg_strText = "";
  mgNetworkOpenPort.mg_ctMaxStringLen = 10;
  mgNetworkOpenPort.mg_pstrToChange = &_strPort;
  mgNetworkOpenPort.mg_boxOnScreen = BoxMediumMiddle(2);
  mgNetworkOpenPort.mg_bfsFontSize = BFS_MEDIUM;
  mgNetworkOpenPort.mg_iCenterI = -1;
  mgNetworkOpenPort.mg_pmgUp = &mgNetworkOpenAddress;
  mgNetworkOpenPort.mg_pmgDown = &mgNetworkOpenJoin;
  mgNetworkOpenPort.mg_strTip = TRANS("specify server address");
  gm_lhGadgets.AddTail( mgNetworkOpenPort.mg_lnNode);

  mgNetworkOpenJoin.mg_boxOnScreen = BoxMediumMiddle(3);
  mgNetworkOpenJoin.mg_pmgUp = &mgNetworkOpenPort;
  mgNetworkOpenJoin.mg_pmgDown = &mgNetworkOpenAddress;
  mgNetworkOpenJoin.mg_strText = TRANS("Join");
  gm_lhGadgets.AddTail( mgNetworkOpenJoin.mg_lnNode);
  mgNetworkOpenJoin.mg_pActivatedFunction = &StartSelectPlayersMenuFromOpen;
}

void CNetworkOpenMenu::StartMenu(void)
{
  _strPort = _pShell->GetValue("net_iPort");
  mgNetworkOpenPort.mg_strText = _strPort;
}

void CNetworkOpenMenu::EndMenu(void)
{
  _pShell->SetValue("net_iPort", _strPort);
}

// ------------------------ CSplitScreenMenu implementation
void CSplitScreenMenu::Initialize_t(void)
{
  // intialize split screen menu
  mgSplitScreenTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgSplitScreenTitle.mg_strText = TRANS("SPLIT SCREEN");
  gm_lhGadgets.AddTail( mgSplitScreenTitle.mg_lnNode);

  mgSplitScreenStart.mg_bfsFontSize = BFS_LARGE;
  mgSplitScreenStart.mg_boxOnScreen = BoxBigRow(0);
  mgSplitScreenStart.mg_pmgUp = &mgSplitScreenLoad;
  mgSplitScreenStart.mg_pmgDown = &mgSplitScreenQuickLoad;
  mgSplitScreenStart.mg_strText = TRANS("NEW GAME");
  mgSplitScreenStart.mg_strTip = TRANS("start new split-screen game");
  gm_lhGadgets.AddTail( mgSplitScreenStart.mg_lnNode);
  mgSplitScreenStart.mg_pActivatedFunction = &StartSplitStartMenu;

  mgSplitScreenQuickLoad.mg_bfsFontSize = BFS_LARGE;
  mgSplitScreenQuickLoad.mg_boxOnScreen = BoxBigRow(1);
  mgSplitScreenQuickLoad.mg_pmgUp = &mgSplitScreenStart;
  mgSplitScreenQuickLoad.mg_pmgDown = &mgSplitScreenLoad;
  mgSplitScreenQuickLoad.mg_strText = TRANS("QUICK LOAD");
  mgSplitScreenQuickLoad.mg_strTip = TRANS("load a quick-saved game (F9)");
  gm_lhGadgets.AddTail( mgSplitScreenQuickLoad.mg_lnNode);
  mgSplitScreenQuickLoad.mg_pActivatedFunction = &StartSplitScreenQuickLoadMenu;

  mgSplitScreenLoad.mg_bfsFontSize = BFS_LARGE;
  mgSplitScreenLoad.mg_boxOnScreen = BoxBigRow(2);
  mgSplitScreenLoad.mg_pmgUp = &mgSplitScreenQuickLoad;
  mgSplitScreenLoad.mg_pmgDown = &mgSplitScreenStart;
  mgSplitScreenLoad.mg_strText = TRANS("LOAD");
  mgSplitScreenLoad.mg_strTip = TRANS("load a saved split-screen game");
  gm_lhGadgets.AddTail( mgSplitScreenLoad.mg_lnNode);
  mgSplitScreenLoad.mg_pActivatedFunction = &StartSplitScreenLoadMenu;
}


void CSplitScreenMenu::StartMenu(void)
{
  CGameMenu::StartMenu();
}

void UpdateSplitLevel(INDEX iDummy)
{
  ValidateLevelForFlags(_pGame->gam_strCustomLevel, 
    SpawnFlagsForGameType(mgSplitGameType.mg_iSelected));
  mgSplitLevel.mg_strText = FindLevelByFileName(_pGame->gam_strCustomLevel).li_strName;
}

// ------------------------ CSplitStartMenu implementation
void CSplitStartMenu::Initialize_t(void)
{
  // intialize split screen menu
  mgSplitStartTitle.mg_boxOnScreen = BoxTitle(0.0f);
  mgSplitStartTitle.mg_strText = TRANS("START SPLIT SCREEN");
  gm_lhGadgets.AddTail( mgSplitStartTitle.mg_lnNode);

  // game type trigger
  TRIGGER_MG(mgSplitGameType, 0,
       mgSplitStartStart, mgSplitDifficulty, TRANS("Game type:"), astrGameTypeRadioTexts);
  mgSplitGameType.mg_ctTexts = ctGameTypeRadioTexts;
  mgSplitGameType.mg_strTip = TRANS("choose type of multiplayer game");
  mgSplitGameType.mg_pOnTriggerChange = UpdateSplitLevel;
  // difficulty trigger
  TRIGGER_MG(mgSplitDifficulty, 1,
       mgSplitGameType, mgSplitLevel, TRANS("Difficulty:"), astrDifficultyRadioTexts);
  mgSplitDifficulty.mg_strTip = TRANS("choose difficulty level");

  // level name
  mgSplitLevel.mg_strText = "";
  mgSplitLevel.mg_strLabel = TRANS("Level:");
  mgSplitLevel.mg_boxOnScreen = BoxMediumRow(2);
  mgSplitLevel.mg_bfsFontSize = BFS_MEDIUM;
  mgSplitLevel.mg_iCenterI = -1;
  mgSplitLevel.mg_pmgUp = &mgSplitDifficulty;
  mgSplitLevel.mg_pmgDown = &mgSplitOptions;
  mgSplitLevel.mg_strTip = TRANS("choose the level to start");
  mgSplitLevel.mg_pActivatedFunction = &StartSelectLevelFromSplit;
  gm_lhGadgets.AddTail( mgSplitLevel.mg_lnNode);

  // options button
  mgSplitOptions.mg_strText = TRANS("Game options");
  mgSplitOptions.mg_boxOnScreen = BoxMediumRow(3);
  mgSplitOptions.mg_bfsFontSize = BFS_MEDIUM;
  mgSplitOptions.mg_iCenterI = 0;
  mgSplitOptions.mg_pmgUp = &mgSplitLevel;
  mgSplitOptions.mg_pmgDown = &mgSplitStartStart;
  mgSplitOptions.mg_strTip = TRANS("adjust game rules");
  mgSplitOptions.mg_pActivatedFunction = &StartGameOptionsFromSplitScreen;
  gm_lhGadgets.AddTail( mgSplitOptions.mg_lnNode);

  // start button
  mgSplitStartStart.mg_bfsFontSize = BFS_LARGE;
  mgSplitStartStart.mg_boxOnScreen = BoxBigRow(4);
  mgSplitStartStart.mg_pmgUp = &mgSplitOptions;
  mgSplitStartStart.mg_pmgDown = &mgSplitGameType;
  mgSplitStartStart.mg_strText = TRANS("START");
  gm_lhGadgets.AddTail( mgSplitStartStart.mg_lnNode);
  mgSplitStartStart.mg_pActivatedFunction = &StartSelectPlayersMenuFromSplit;
}

void CSplitStartMenu::StartMenu(void)
{
  extern INDEX sam_bMentalActivated;
  mgSplitDifficulty.mg_ctTexts = sam_bMentalActivated?6:5;

  mgSplitGameType.mg_iSelected = Clamp(_pShell->GetINDEX("gam_iStartMode"), (INDEX)0, ctGameTypeRadioTexts-1);
  mgSplitGameType.ApplyCurrentSelection();
  mgSplitDifficulty.mg_iSelected = _pShell->GetINDEX("gam_iStartDifficulty")+1;
  mgSplitDifficulty.ApplyCurrentSelection();

  // clamp maximum number of players to at least 4
  _pShell->SetINDEX("gam_ctMaxPlayers", ClampDn(_pShell->GetINDEX("gam_ctMaxPlayers"), (INDEX)4));

  UpdateSplitLevel(0);
  CGameMenu::StartMenu();
}
void CSplitStartMenu::EndMenu(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", mgSplitDifficulty.mg_iSelected-1);
  _pShell->SetINDEX("gam_iStartMode", mgSplitGameType.mg_iSelected);

  CGameMenu::EndMenu();
}

// ------------------------ CGameOptionsMenu implementation
void ChangeBloodColor(INDEX iNew)
{
  _pShell->SetINDEX("gam_iBlood", iNew);
}

void CGameOptionsMenu::Initialize_t(void)
{
  // intialize game options menu
  mgGameOptionsTitle.mg_boxOnScreen = BoxTitle(4.22f);
  mgGameOptionsTitle.mg_strText = TRANS("# GAME OPTIONS");
  gm_lhGadgets.AddTail(mgGameOptionsTitle.mg_lnNode);
  mgGameOptionsTitle.mg_iCenterI = -1;

  mgGameOptions3rdPerson.mg_pmgUp = &mgGameOptionsBack;
  mgGameOptions3rdPerson.mg_pmgDown = &mgGameOptionsAutoSave;
  mgGameOptions3rdPerson.mg_boxOnScreen = BoxMediumLeft(10.71f);
  gm_lhGadgets.AddTail(mgGameOptions3rdPerson.mg_lnNode);
  mgGameOptions3rdPerson.mg_astrTexts = astrNoYes;
  mgGameOptions3rdPerson.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgGameOptions3rdPerson.mg_iSelected = 0;
  mgGameOptions3rdPerson.mg_strLabel = TRANS("THIRD PERSON VIEW");
  mgGameOptions3rdPerson.mg_strValue = astrNoYes[0];
  mgGameOptions3rdPerson.mg_strTip = TRANS("Enable third person view");
  mgGameOptions3rdPerson.mg_iCenterI = -1;
  mgGameOptions3rdPerson.mg_pOnTriggerChange = Change3rdPerson;

  mgGameOptionsAutoSave.mg_pmgUp = &mgGameOptions3rdPerson;
  mgGameOptionsAutoSave.mg_pmgDown = &mgGameOptionsSharpTurning;
  mgGameOptionsAutoSave.mg_boxOnScreen = BoxMediumLeft(11.71f);
  gm_lhGadgets.AddTail(mgGameOptionsAutoSave.mg_lnNode);
  mgGameOptionsAutoSave.mg_astrTexts = astrNoYes;
  mgGameOptionsAutoSave.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgGameOptionsAutoSave.mg_iSelected = 0;
  mgGameOptionsAutoSave.mg_strLabel = TRANS("AUTO SAVE");
  mgGameOptionsAutoSave.mg_strValue = astrNoYes[0];
  mgGameOptionsAutoSave.mg_strTip = TRANS("Automatically save game every few minutes");
  mgGameOptionsAutoSave.mg_iCenterI = -1;
  mgGameOptionsAutoSave.mg_pOnTriggerChange = Change3rdPerson;

  mgGameOptionsSharpTurning.mg_pmgUp = &mgGameOptionsAutoSave;
  mgGameOptionsSharpTurning.mg_pmgDown = &mgGameOptionsViewBobbing;
  mgGameOptionsSharpTurning.mg_boxOnScreen = BoxMediumLeft(12.71f);
  gm_lhGadgets.AddTail(mgGameOptionsSharpTurning.mg_lnNode);
  mgGameOptionsSharpTurning.mg_astrTexts = astrNoYes;
  mgGameOptionsSharpTurning.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgGameOptionsSharpTurning.mg_iSelected = 0;
  mgGameOptionsSharpTurning.mg_strLabel = TRANS("SHARP TURNING");
  mgGameOptionsSharpTurning.mg_strValue = astrNoYes[0];
  mgGameOptionsSharpTurning.mg_strTip = TRANS("Makes in-game camera turning faster");
  mgGameOptionsSharpTurning.mg_iCenterI = -1;
  mgGameOptionsSharpTurning.mg_pOnTriggerChange = ChangeSharpTurning;

  mgGameOptionsViewBobbing.mg_pmgUp = &mgGameOptionsSharpTurning;
  mgGameOptionsViewBobbing.mg_pmgDown = &mgGameOptionsBlood;
  mgGameOptionsViewBobbing.mg_boxOnScreen = BoxMediumLeft(13.71f);
  gm_lhGadgets.AddTail(mgGameOptionsViewBobbing.mg_lnNode);
  mgGameOptionsViewBobbing.mg_astrTexts = astrNoYes;
  mgGameOptionsViewBobbing.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgGameOptionsViewBobbing.mg_iSelected = 0;
  mgGameOptionsViewBobbing.mg_strLabel = TRANS("VIEW BOBBING");
  mgGameOptionsViewBobbing.mg_strValue = astrNoYes[0];
  mgGameOptionsViewBobbing.mg_strTip = TRANS("Enable view bobbing");
  mgGameOptionsViewBobbing.mg_iCenterI = -1;
  mgGameOptionsViewBobbing.mg_pOnTriggerChange = ChangeViewBobbing;

  mgGameOptionsBlood.mg_pmgUp = &mgGameOptionsViewBobbing;
  mgGameOptionsBlood.mg_pmgDown = &mgGameOptionsBack;
  mgGameOptionsBlood.mg_boxOnScreen = BoxMediumLeft(14.71f);
  gm_lhGadgets.AddTail(mgGameOptionsBlood.mg_lnNode);
  mgGameOptionsBlood.mg_astrTexts = astrBloodTexts;
  mgGameOptionsBlood.mg_ctTexts = ARRAYCOUNT(astrBloodTexts);
  mgGameOptionsBlood.mg_iSelected = 0;
  mgGameOptionsBlood.mg_strLabel = TRANS("BLOOD AND GORE");
  mgGameOptionsBlood.mg_strValue = astrBloodTexts[0];
  mgGameOptionsBlood.mg_strTip = TRANS("Select blood and gore appearance");
  mgGameOptionsBlood.mg_iCenterI = -1;
  mgGameOptionsBlood.mg_pOnTriggerChange = ChangeBloodColor;

  mgGameOptionsBack.mg_bfsFontSize = BFS_LARGE;
  mgGameOptionsBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgGameOptionsBack.mg_pmgUp = &mgGameOptionsBlood;
  mgGameOptionsBack.mg_pmgDown = &mgGameOptions3rdPerson;
  mgGameOptionsBack.mg_strText = TRANS("BACK");
  mgGameOptionsBack.mg_strTip = TRANS("Return to options menu");
  gm_lhGadgets.AddTail(mgGameOptionsBack.mg_lnNode);
  mgGameOptionsBack.mg_pActivatedFunction = &MenuBack;
  mgGameOptionsBack.mg_iCenterI = -1;
}

void GetGameSettings(void)
{
  CPlayerCharacter& pc = _pGame->gm_apcPlayers[0];
  CPlayerSettings* pps = (CPlayerSettings*)pc.pc_aubAppearance;

  mgGameOptions3rdPerson.mg_iSelected = (pps->ps_ulFlags & PSF_PREFER3RDPERSON) ? 1 : 0;
  mgGameOptions3rdPerson.ApplyCurrentSelection();

  mgGameOptionsAutoSave.mg_iSelected = (pps->ps_ulFlags & PSF_AUTOSAVE) ? 1 : 0;
  mgGameOptionsAutoSave.ApplyCurrentSelection();

  mgGameOptionsViewBobbing.mg_iSelected = (pps->ps_ulFlags & PSF_NOBOBBING) ? 0 : 1;
  mgGameOptionsViewBobbing.ApplyCurrentSelection();

  mgGameOptionsSharpTurning.mg_iSelected = (pps->ps_ulFlags & PSF_SHARPTURNING) ? 1 : 0;
  mgGameOptionsSharpTurning.ApplyCurrentSelection();

  mgGameOptionsBlood.mg_iSelected = _pShell->GetINDEX("gam_iBlood");
  mgGameOptionsBlood.ApplyCurrentSelection();
}

void CGameOptionsMenu::StartMenu(void)
{
  GetGameSettings();

  mgGameOptionsBlood.mg_bEnabled = _gmRunningGameMode == GM_NONE;

  CGameMenu::StartMenu();
}

// ------------------------ CRenderingOptionsMenu implementation
void ApplyRenderingSettings(void)
{
  _pShell->SetINDEX("tex_iNormalSize", fTextureSizes[mgRenderingOptionsTexturesSize.mg_iSelected]);
  _pShell->SetINDEX("tex_iNormalQuality", 11 * (mgRenderingOptionsTexturesQuality.mg_iSelected + 1));
  _pShell->SetINDEX("wld_bTextureLayers", 110 + mgRenderingOptionsBumpMaps.mg_iSelected);
  _pShell->SetINDEX("shd_iStaticSize", fShadowMapsSizes[mgRenderingOptionsShadowMapsSize.mg_iSelected]);
  _pShell->SetINDEX("gfx_iLensFlareQuality", mgRenderingOptionsLensFlares.mg_iSelected);
  _pShell->SetFLOAT("gfx_fGamma", mgRenderingOptionsGamma.mg_iCurPos / 10.0f);
  _pShell->SetINDEX("gap_iSwapInterval", mgRenderingOptionsVSync.mg_iSelected);

  sam_iVideoSetup = 3;

  _pShell->Execute("ApplyVideoMode();");
  _pShell->Execute("RefreshTextures();");
  _pShell->Execute("RecacheShadows();");
}

void CRenderingOptionsMenu::Initialize_t(void)
{
  // intialize rendering options menu
  mgRenderingOptionsTitle.mg_boxOnScreen = BoxTitle(2.46f);
  mgRenderingOptionsTitle.mg_strText = TRANS("# RENDERING OPTIONS");
  gm_lhGadgets.AddTail(mgRenderingOptionsTitle.mg_lnNode);
  mgRenderingOptionsTitle.mg_iCenterI = -1;

  mgRenderingOptionsTexturesSize.mg_pmgUp = &mgRenderingOptionsBack;
  mgRenderingOptionsTexturesSize.mg_pmgDown = &mgRenderingOptionsTexturesQuality;
  mgRenderingOptionsTexturesSize.mg_boxOnScreen = BoxMediumLeft(7.11f);
  gm_lhGadgets.AddTail(mgRenderingOptionsTexturesSize.mg_lnNode);
  mgRenderingOptionsTexturesSize.mg_astrTexts = astrTextureSizeTexts;
  mgRenderingOptionsTexturesSize.mg_ctTexts = ARRAYCOUNT(astrTextureSizeTexts);
  mgRenderingOptionsTexturesSize.mg_iSelected = 0;
  mgRenderingOptionsTexturesSize.mg_strLabel = TRANS("TEXTURES SIZE");
  mgRenderingOptionsTexturesSize.mg_strValue = astrTextureSizeTexts[0];
  mgRenderingOptionsTexturesSize.mg_strTip = TRANS("Select textures size");
  mgRenderingOptionsTexturesSize.mg_iCenterI = -1;

  mgRenderingOptionsTexturesQuality.mg_pmgUp = &mgRenderingOptionsTexturesSize;
  mgRenderingOptionsTexturesQuality.mg_pmgDown = &mgRenderingOptionsBumpMaps;
  mgRenderingOptionsTexturesQuality.mg_boxOnScreen = BoxMediumLeft(8.11f);
  gm_lhGadgets.AddTail(mgRenderingOptionsTexturesQuality.mg_lnNode);
  mgRenderingOptionsTexturesQuality.mg_astrTexts = astrTextureQualityTexts;
  mgRenderingOptionsTexturesQuality.mg_ctTexts = ARRAYCOUNT(astrTextureQualityTexts);
  mgRenderingOptionsTexturesQuality.mg_iSelected = 0;
  mgRenderingOptionsTexturesQuality.mg_strLabel = TRANS("TEXTURES QUALITY");
  mgRenderingOptionsTexturesQuality.mg_strValue = astrTextureQualityTexts[0];
  mgRenderingOptionsTexturesQuality.mg_strTip = TRANS("Select textures quality");
  mgRenderingOptionsTexturesQuality.mg_iCenterI = -1;

  mgRenderingOptionsBumpMaps.mg_pmgUp = &mgRenderingOptionsTexturesQuality;
  mgRenderingOptionsBumpMaps.mg_pmgDown = &mgRenderingOptionsShadowMapsSize;
  mgRenderingOptionsBumpMaps.mg_boxOnScreen = BoxMediumLeft(9.11f);
  gm_lhGadgets.AddTail(mgRenderingOptionsBumpMaps.mg_lnNode);
  mgRenderingOptionsBumpMaps.mg_astrTexts = astrNoYes;
  mgRenderingOptionsBumpMaps.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgRenderingOptionsBumpMaps.mg_iSelected = 0;
  mgRenderingOptionsBumpMaps.mg_strLabel = TRANS("BUMP MAP TEXTURES");
  mgRenderingOptionsBumpMaps.mg_strValue = astrNoYes[0];
  mgRenderingOptionsBumpMaps.mg_strTip = TRANS("Enable bump map textures");
  mgRenderingOptionsBumpMaps.mg_iCenterI = -1;

  mgRenderingOptionsShadowMapsSize.mg_pmgUp = &mgRenderingOptionsBumpMaps;
  mgRenderingOptionsShadowMapsSize.mg_pmgDown = &mgRenderingOptionsLensFlares;
  mgRenderingOptionsShadowMapsSize.mg_boxOnScreen = BoxMediumLeft(10.11f);
  gm_lhGadgets.AddTail(mgRenderingOptionsShadowMapsSize.mg_lnNode);
  mgRenderingOptionsShadowMapsSize.mg_astrTexts = astrShadowMapSizeTexts;
  mgRenderingOptionsShadowMapsSize.mg_ctTexts = ARRAYCOUNT(astrShadowMapSizeTexts);
  mgRenderingOptionsShadowMapsSize.mg_iSelected = 0;
  mgRenderingOptionsShadowMapsSize.mg_strLabel = TRANS("SHADOW MAP TEXTURES SIZE");
  mgRenderingOptionsShadowMapsSize.mg_strValue = astrShadowMapSizeTexts[0];
  mgRenderingOptionsShadowMapsSize.mg_strTip = TRANS("Select shadow map textures size");
  mgRenderingOptionsShadowMapsSize.mg_iCenterI = -1;

  mgRenderingOptionsLensFlares.mg_pmgUp = &mgRenderingOptionsShadowMapsSize;
  mgRenderingOptionsLensFlares.mg_pmgDown = &mgRenderingOptionsGamma;
  mgRenderingOptionsLensFlares.mg_boxOnScreen = BoxMediumLeft(11.11f);
  gm_lhGadgets.AddTail(mgRenderingOptionsLensFlares.mg_lnNode);
  mgRenderingOptionsLensFlares.mg_astrTexts = astrLensFlaresTexts;
  mgRenderingOptionsLensFlares.mg_ctTexts = ARRAYCOUNT(astrLensFlaresTexts);
  mgRenderingOptionsLensFlares.mg_iSelected = 0;
  mgRenderingOptionsLensFlares.mg_strLabel = TRANS("LENS FLARES");
  mgRenderingOptionsLensFlares.mg_strValue = astrLensFlaresTexts[0];
  mgRenderingOptionsLensFlares.mg_strTip = TRANS("Select lens flares rendering type");
  mgRenderingOptionsLensFlares.mg_iCenterI = -1;

  mgRenderingOptionsGamma.mg_pmgUp = &mgRenderingOptionsLensFlares;
  mgRenderingOptionsGamma.mg_pmgDown = &mgRenderingOptionsVSync;
  mgRenderingOptionsGamma.mg_boxOnScreen = BoxMediumLeft(12.11f);
  gm_lhGadgets.AddTail(mgRenderingOptionsGamma.mg_lnNode);
  mgRenderingOptionsGamma.mg_iMinPos = 0;
  mgRenderingOptionsGamma.mg_iMaxPos = 15;
  mgRenderingOptionsGamma.mg_iCurPos = 0;
  mgRenderingOptionsGamma.mg_strText = TRANS("GAMMA");
  mgRenderingOptionsGamma.mg_strTip = TRANS("Select game gamma");
  mgRenderingOptionsGamma.mg_iCenterI = -1;

  mgRenderingOptionsVSync.mg_pmgUp = &mgRenderingOptionsGamma;
  mgRenderingOptionsVSync.mg_pmgDown = &mgRenderingOptionsApply;
  mgRenderingOptionsVSync.mg_boxOnScreen = BoxMediumLeft(13.11f);
  gm_lhGadgets.AddTail(mgRenderingOptionsVSync.mg_lnNode);
  mgRenderingOptionsVSync.mg_astrTexts = astrNoYes;
  mgRenderingOptionsVSync.mg_ctTexts = ARRAYCOUNT(astrNoYes);
  mgRenderingOptionsVSync.mg_iSelected = 0;
  mgRenderingOptionsVSync.mg_strLabel = TRANS("VERTICAL SYNCHRONIZATION");
  mgRenderingOptionsVSync.mg_strValue = astrNoYes[0];
  mgRenderingOptionsVSync.mg_strTip = TRANS("Synchronize frame rate with monitor refresh rate");
  mgRenderingOptionsVSync.mg_iCenterI = -1;

  mgRenderingOptionsApply.mg_bfsFontSize = BFS_LARGE;
  mgRenderingOptionsApply.mg_boxOnScreen = BoxBigLeft(8.51f);
  mgRenderingOptionsApply.mg_pmgUp = &mgRenderingOptionsVSync;
  mgRenderingOptionsApply.mg_pmgDown = &mgRenderingOptionsBack;
  mgRenderingOptionsApply.mg_strText = TRANS("APPLY");
  mgRenderingOptionsApply.mg_strTip = TRANS("Apply rendering settings");
  gm_lhGadgets.AddTail(mgRenderingOptionsApply.mg_lnNode);
  mgRenderingOptionsApply.mg_pActivatedFunction = &ApplyRenderingSettings;
  mgRenderingOptionsApply.mg_iCenterI = -1;

  mgRenderingOptionsBack.mg_bfsFontSize = BFS_LARGE;
  mgRenderingOptionsBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgRenderingOptionsBack.mg_pmgUp = &mgRenderingOptionsApply;
  mgRenderingOptionsBack.mg_pmgDown = &mgRenderingOptionsTexturesSize;
  mgRenderingOptionsBack.mg_strText = TRANS("BACK");
  mgRenderingOptionsBack.mg_strTip = TRANS("Return to video options menu");
  gm_lhGadgets.AddTail(mgRenderingOptionsBack.mg_lnNode);
  mgRenderingOptionsBack.mg_pActivatedFunction = &MenuBack;
  mgRenderingOptionsBack.mg_iCenterI = -1;
}

INDEX FindFloatInArray(FLOAT a[], INDEX iLen, FLOAT fVal)
{
  BOOL found = FALSE;
  INDEX i;

  for (i = 0; i < iLen; i++)
  {
    if (a[i] == fVal)
    {
        found = TRUE;
        break;
    }
  }

  return i;
}

void GetRenderingSettings(void)
{
  INDEX iNormalSize = _pShell->GetINDEX("tex_iNormalSize");

  mgRenderingOptionsTexturesSize.mg_iSelected = FindFloatInArray(fTextureSizes, ARRAYCOUNT(fTextureSizes), iNormalSize);
  mgRenderingOptionsTexturesSize.ApplyCurrentSelection();

  INDEX iNormalQuality = _pShell->GetINDEX("tex_iNormalQuality");

  mgRenderingOptionsTexturesQuality.mg_iSelected = (iNormalQuality / 11) - 1;
  mgRenderingOptionsTexturesQuality.ApplyCurrentSelection();

  INDEX iTextureLayers = _pShell->GetINDEX("wld_bTextureLayers");

  mgRenderingOptionsBumpMaps.mg_iSelected = iTextureLayers - 110;
  mgRenderingOptionsTexturesQuality.ApplyCurrentSelection();

  INDEX iStaticSize = _pShell->GetINDEX("shd_iStaticSize");

  mgRenderingOptionsShadowMapsSize.mg_iSelected = FindFloatInArray(fShadowMapsSizes, ARRAYCOUNT(fShadowMapsSizes), iStaticSize);
  mgRenderingOptionsShadowMapsSize.ApplyCurrentSelection();

  mgRenderingOptionsLensFlares.mg_iSelected = _pShell->GetINDEX("gfx_iLensFlareQuality");
  mgRenderingOptionsLensFlares.ApplyCurrentSelection();

  FLOAT fGamma = _pShell->GetFLOAT("gfx_fGamma");

  mgRenderingOptionsGamma.mg_iCurPos = fGamma * 10.0f;
  mgRenderingOptionsGamma.ApplyCurrentPosition();

  mgRenderingOptionsVSync.mg_iSelected = _pShell->GetINDEX("gap_iSwapInterval");
  mgRenderingOptionsVSync.ApplyCurrentSelection();
}

void CRenderingOptionsMenu::StartMenu(void)
{
  GetRenderingSettings();

  mgRenderingOptionsTexturesQuality.mg_bEnabled = _pShell->GetINDEX("sys_bHas32bitTextures");
  mgRenderingOptionsGamma.mg_bEnabled = _pShell->GetINDEX("sys_bHasAdjustableGamma");
  mgRenderingOptionsVSync.mg_bEnabled = _pShell->GetINDEX("sys_bHasSwapInterval");

  CGameMenu::StartMenu();
}

// ------------------------ CCreditsMenu implementation
void CCreditsMenu::Initialize_t(void)
{
  // intialize credits menu
  mgCreditsBack.mg_bfsFontSize = BFS_LARGE;
  mgCreditsBack.mg_boxOnScreen = BoxBigLeft(9.51f);
  mgCreditsBack.mg_pmgUp = &mgCreditsBack;
  mgCreditsBack.mg_pmgDown = &mgCreditsBack;
  mgCreditsBack.mg_strText = TRANS("BACK");
  mgCreditsBack.mg_strTip = TRANS("Return to main menu");
  gm_lhGadgets.AddTail(mgCreditsBack.mg_lnNode);
  mgCreditsBack.mg_pActivatedFunction = &MenuBack;
  mgCreditsBack.mg_iCenterI = -1;
}

extern FLOAT fCreditsStartTime;
extern FLOAT fCreditsTime;

void CCreditsMenu::StartMenu(void)
{
  try {
    soMusic.Play_t(CTFILENAME("Music\\Credits.mp3"), SOF_NONGAME | SOF_MUSIC | SOF_LOOP);
  }
  catch (const char *strError) {
    CPrintF("%s\n", (const char*)strError);
  }

  fCreditsStartTime = 0;
  fCreditsTime = 0;

  _pShell->SetINDEX("ad_iStartCredits", 3);
  CGameMenu::StartMenu();
}

void CCreditsMenu::Think(void)
{
   CGameMenu::Think();

  if (fCreditsTime > (fCreditsStartTime + 18)) {
    MenuBack();
  }
}

void CCreditsMenu::EndMenu(void)
{
  _pShell->SetINDEX("ad_iStartCredits", -1);
  soMusic.Stop();
  CGameMenu::EndMenu();
}
