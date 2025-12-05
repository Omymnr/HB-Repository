/****************************************************************
*		 This client was re-writen by OhmyWeed & Widah.			*
*					Based on OhmyWeed work.						*
*																*
*			V3.51 compatibility by OhmyWeed						*
*			V3.51 dialogs by Widah								*
*			Effects, mobs, Apocalypse, Heldenian				*
*			& finalizing by Widah							    *
*			V3.82 Crafting & Angels by OhmyWeed & Widah			*
*****************************************************************/

/****************************************************************
*	Find here all compilation options							*
*	I removed useless ones and tryed to add some explanations	*
*	( Widah 29/11/2025 )										*
*****************************************************************/

/*** Put here global data for your server ***/
#define NAME_WORLDNAME1			"Apocalypse"
#define MSG_WORLDNAME1			"Apocalypse Server"
#define MSG_HOMEPAGE			"http://www.helbreath.com/"
#define DEF_UPPERVERSION		1
#define DEF_LOWERVERSION		0
#define DEF_VERSION_STRING		"1.0.4"  // Version mostrada en pantalla
#define DEF_SERVER_IP			"127.0.0.1"
#define DEF_SERVER_PORT			9907
#define DEF_GSERVER_PORT		9907

#define SCREENX					80
#define SCREENY					60


/*** Some more compilation options ***/
#define DEF_USING_WIN_IME		// Used for unicode characters (unchecked)
#define DEF_MAKE_ACCOUNT		// Enable acc creation. Missing sprites, fixed!
#define DEF_SELECTSERVER		// Show/Hides server selection screen.
#define DEF_USE_LOGIN
#define DEF_COMMA_GOLD			// Allows to show comma nbe as original HB (ie: 1,200,000)
// #define DEF_4XXP_PER_LVL		// limits xp niv 140, then 4 x XP per lvl (affects XP shown as % on status)


//#define  _DEBUG				// Multi-Client, Instant-logout, Version check, Badwords,
								// Mics debug msgs, CtrlQ for admins, Show status, Cursor position
#define DEF_WINDOWED_MODE		// Shows HB in a windowed mode (for debug purpose only, sprite will bug....)


/*** Embedded Server Config (for distribution to friends) ***/
// Uncomment this line to embed server IP in the executable (no login.cfg needed)
// The IP is XOR encrypted to prevent easy extraction from the binary
// #define DEF_EMBEDDED_SERVER_CONFIG

#ifdef DEF_EMBEDDED_SERVER_CONFIG
// XOR key for simple obfuscation (change this to your own random value)
#define DEF_XOR_KEY 0x5A
// Server ports
#define DEF_EMBEDDED_LOG_PORT   2500
#define DEF_EMBEDDED_GAME_PORT  9907
#endif


/*** Special conditions   ***/
#ifndef _DEBUG
	#undef DEF_WINDOWED_MODE
#endif
