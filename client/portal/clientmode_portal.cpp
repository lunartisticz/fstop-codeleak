//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "ivmodemanager.h"
#include "clientmode_hlnormal.h"
#include "panelmetaclassmgr.h"
#include "c_playerresource.h"
#include "c_portal_player.h"
#include "clientmode_portal.h"
#include "usermessages.h"
#include "prediction.h"

#ifdef PORTAL2
#include "c_basehlplayer.h"
#endif // PORTAL2

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// default FOV for HL2
ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "75", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 75.0, true, 90.0 );


// The current client mode. Always ClientModeNormal in HL.
static IClientMode *g_pClientMode[ MAX_SPLITSCREEN_PLAYERS ];
IClientMode *GetClientMode()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pClientMode[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}
//extern EHANDLE g_eKillTarget1;
//extern EHANDLE g_eKillTarget2;

vgui::HScheme g_hVGuiCombineScheme = 0;

#define SCREEN_FILE		"scripts/vgui_screens.txt"

//void MsgFunc_KillCam(bf_read &msg) 
//{
//	C_Portal_Player *pPlayer = C_Portal_Player::GetLocalPortalPlayer();
//
//	if ( !pPlayer )
//		return;
//
//	g_eKillTarget1 = 0;
//	g_eKillTarget2 = 0;
//	g_nKillCamTarget1 = 0;
//	g_nKillCamTarget1 = 0;
//
//	long iEncodedEHandle = msg.ReadLong();
//
//	if( iEncodedEHandle == INVALID_NETWORKED_EHANDLE_VALUE )
//		return;	
//
//	int iEntity = iEncodedEHandle & ((1 << MAX_EDICT_BITS) - 1);
//	int iSerialNum = iEncodedEHandle >> MAX_EDICT_BITS;
//
//	EHANDLE hEnt1( iEntity, iSerialNum );
//
//	iEncodedEHandle = msg.ReadLong();
//
//	if( iEncodedEHandle == INVALID_NETWORKED_EHANDLE_VALUE )
//		return;	
//
//	iEntity = iEncodedEHandle & ((1 << MAX_EDICT_BITS) - 1);
//	iSerialNum = iEncodedEHandle >> MAX_EDICT_BITS;
//
//	EHANDLE hEnt2( iEntity, iSerialNum );
//
//	g_eKillTarget1 = hEnt1;
//	g_eKillTarget2 = hEnt2;
//
//	if ( g_eKillTarget1.Get() )
//	{
//		g_nKillCamTarget1	= g_eKillTarget1.Get()->entindex();
//	}
//
//	if ( g_eKillTarget2.Get() )
//	{
//		g_nKillCamTarget2	= g_eKillTarget2.Get()->entindex();
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		GetHud().InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual IViewPortPanel *CreatePanelByName( const char *szPanelName );
};

IViewPortPanel* CHudViewport::CreatePanelByName( const char *szPanelName )
{
	/*IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName) == 0 )
	{
		newpanel = new CHL2MPClientScoreBoardDialog( this );
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_INFO, szPanelName) == 0 )
	{
		newpanel = new CHL2MPTextWindow( this );
		return newpanel;
	}*/

	return BaseClass::CreatePanelByName( szPanelName ); 
}


class CHLModeManager : public IVModeManager
{
public:
	CHLModeManager( void );
	virtual		~CHLModeManager( void );

	virtual void	Init( void );
	virtual void	SwitchMode( bool commander, bool force );
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual void	CreateMove( float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
};

CHLModeManager::CHLModeManager( void )
{
}

CHLModeManager::~CHLModeManager( void )
{
}

void CHLModeManager::Init( void )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		g_pClientMode[ i ] = GetClientModeNormal();
	}
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );
}

void CHLModeManager::SwitchMode( bool commander, bool force )
{
}

void CHLModeManager::OverrideView( CViewSetup *pSetup )
{
}

void CHLModeManager::CreateMove( float flInputSampleTime, CUserCmd *cmd )
{
}

void CHLModeManager::LevelInit( const char *newmap )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelInit( newmap );
	}

	if ( g_nKillCamMode > OBS_MODE_NONE )
	{
		g_bForceCLPredictOff = false;
	}

	g_nKillCamMode		= OBS_MODE_NONE;
	//g_nKillCamTarget1	= 0;
	//g_nKillCamTarget2	= 0;
}

void CHLModeManager::LevelShutdown( void )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelShutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModePortalNormal::ShouldDrawViewModel( void )
{
#ifdef PORTAL2
	// Don't draw the viewmodel while zoomed in
	C_BaseHLPlayer *pHLPlayer = (C_BaseHLPlayer *) C_BasePlayer::GetLocalPlayer();
	if ( pHLPlayer && pHLPlayer->m_HL2Local.m_bZooming )
		return false;
#endif // PORTAL2

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModePortalNormal::ClientModePortalNormal()
{
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModePortalNormal::~ClientModePortalNormal()
{
}

void ClientModePortalNormal::Init()
{
	BaseClass::Init();

	//usermessages->HookMessage( "KillCam", MsgFunc_KillCam );
}

void ClientModePortalNormal::InitViewport()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

ClientModePortalNormal g_ClientModeNormal[ MAX_SPLITSCREEN_PLAYERS ];

IClientMode *GetClientModeNormal()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}


ClientModePortalNormal* GetClientModePortalNormal()
{
	Assert( dynamic_cast< ClientModePortalNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModePortalNormal* >( GetClientModeNormal() );
}


static CHLModeManager g_HLModeManager;
IVModeManager *modemanager = &g_HLModeManager;

