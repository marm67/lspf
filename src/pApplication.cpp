/*
  Copyright (c) 2015 Daniel John Erdos

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#undef  MOD_NAME
#undef  LOGOUT

#define MOD_NAME pApplic
#define LOGOUT   aplog

void ispexeci( pApplication *, const string&, errblock& ) ;

map<int, void *>pApplication::ApplUserData ;

pApplication::pApplication()
{
	currApplication        = this   ;
	testMode               = false  ;
	propagateEnd           = false  ;
	jumpEntered            = false  ;
	resumeTime             = boost::posix_time::second_clock::universal_time() ;
	ControlDisplayLock     = false  ;
	ControlNonDispl        = false  ;
	ControlErrorsReturn    = false  ;
	ControlPassLRScroll    = false  ;
	ControlSplitEnable     = true   ;
	ControlRefUpdate       = true   ;
	errPanelissued         = false  ;
	abending               = false  ;
	addpop_active          = false  ;
	addpop_row             = 0      ;
	addpop_col             = 0      ;
	noTimeOut              = false  ;
	busyAppl               = false  ;
	terminateAppl          = false  ;
	abnormalEnd            = false  ;
	abnormalEndForced      = false  ;
	abnormalTimeout        = false  ;
	rawOutput              = false  ;
	reloadCUATables        = false  ;
	rexxName               = ""     ;
	NEWPOOL                = false  ;
	PASSLIB                = false  ;
	SUSPEND                = false  ;
	libdef_muser           = false  ;
	libdef_puser           = false  ;
	libdef_tuser           = false  ;
	SEL                    = false  ;
	selPanel               = false  ;
	setMSG                 = false  ;
	reffield               = ""     ;
	PANELID                = ""     ;
	PPANELID               = ""     ;
	ZHELP                  = ""     ;
	ZAHELP                 = ""     ;
	ZTDROWS                = 0      ;
	ZTDTOP                 = 0      ;
	ZCURFLD                = ""     ;
	ZCURPOS                = 1      ;
	ZRC                    = 0      ;
	ZRSN                   = 0      ;
	ZRESULT                = ""     ;
	ZERR1                  = ""     ;
	ZERR2                  = ""     ;
	ZERR3                  = ""     ;
	ZERR4                  = ""     ;
	ZERR5                  = ""     ;
	ZERR6                  = ""     ;
	ZERR7                  = ""     ;
	ZERR8                  = ""     ;
	vdefine( "ZCURFLD",  &ZCURFLD ) ;
	vdefine( "ZCURPOS",  &ZCURPOS ) ;
	vdefine( "ZTDMARK",  &ZTDMARK ) ;
	vdefine( "ZTDDEPTH", &ZTDDEPTH) ;
	vdefine( "ZTDROWS",  &ZTDROWS ) ;
	vdefine( "ZTDSELS",  &ZTDSELS ) ;
	vdefine( "ZTDTOP",   &ZTDTOP  ) ;
	vdefine( "ZTDVROWS", &ZTDVROWS) ;
	rmsgs.clear()                   ;
}


pApplication::~pApplication()
{
	map<string, pPanel *>::iterator it;
	for ( it = panelList.begin() ; it != panelList.end() ; it++ )
	{
		delete it->second ;
	}
	busyAppl = false ;
}


void pApplication::startSelect( selobj& s )
{
	ZAPPNAME = s.PGM     ;
	PASSLIB  = s.PASSLIB ;
	SUSPEND  = s.SUSPEND ;
	PARM     = s.PARM    ;
	selPanel = s.selPanel() ;
}


void pApplication::init()
{
	// Before being dispatched in its own thread, set the search paths, table display BOD mark and
	// addpop status if this has not been invoked with the SUSPEND option on the SELECT command

	ZPLIB    = p_poolMGR->get( errBlock, "ZPLIB", PROFILE ) ;
	ZTLIB    = p_poolMGR->get( errBlock, "ZTLIB", PROFILE ) ;
	ZMLIB    = p_poolMGR->get( errBlock, "ZMLIB", PROFILE ) ;
	ZORXPATH = p_poolMGR->get( errBlock, "ZORXPATH", PROFILE ) ;

	ZTDMARK  = centre( " Bottom of Data ", ds2d( p_poolMGR->get( errBlock, "ZSCRMAXW", SHARED ) ), '*' ) ;

	if ( !SUSPEND )
	{
		lscreen_num = ds2d( p_poolMGR->get( errBlock, "ZSCRNUM", SHARED ) ) ;
		addpop_row  = ds2d( p_poolMGR->get( errBlock, lscreen_num, "ZPROW" ) ) ;
		addpop_col  = ds2d( p_poolMGR->get( errBlock, lscreen_num, "ZPCOL" ) ) ;
		if ( addpop_row != 0 || addpop_col != 0 )
		{
			addpop_active = true ;
		}
	}

	llog( "I", "Initialisation complete" << endl ; )
}


void pApplication::wait_event()
{
	busyAppl = false ;

	while ( true )
	{
		if ( terminateAppl ) { RC = 20 ; llog( "E", "Application terminating.  Cancelling wait_event" << endl ) ; abend() ; }
		if ( !busyAppl )
		{
			boost::mutex::scoped_lock lk(mutex) ;
			cond_appl.wait(lk) ;
			lk.unlock() ;
		}
		if ( busyAppl ) { return ; }
	}
}


void pApplication::createPanel( const string& p_name )
{
	string paths ;

	errBlock.setRC( 0 ) ;

	if ( panelList.count( p_name ) > 0 ) { return ; }

	if ( !isvalidName( p_name ) )
	{
		errBlock.seterrid( "PSYE021A", p_name ) ;
		return ;
	}

	pPanel * p_panel    = new pPanel ;
	p_panel->p_poolMGR  = p_poolMGR  ;
	p_panel->p_funcPOOL = &funcPOOL  ;
	p_panel->LRScroll   = ControlPassLRScroll ;
	p_panel->REXX       = ( rexxName != "" )  ;
	p_panel->selPanel( selPanel ) ;
	p_panel->init( errBlock ) ;
	if ( errBlock.error() ) { return ; }

	paths = get_search_path( s_ZPLIB ) ;

	p_panel->loadPanel( errBlock, p_name, paths ) ;

	if ( errBlock.RC0() )
	{
		panelList[ p_name ] = p_panel ;
		load_keylist( p_panel )       ;

	}
	else
	{
		delete p_panel ;
	}
}


bool pApplication::isprimMenu()
{
	if ( panelList.size() == 0 ) { return false                  ; }
	else                         { return currPanel->primaryMenu ; }
}


void pApplication::get_home( uint& row, uint& col )
{
	if ( panelList.size() == 0 ) { row = 0 ; col = 0               ; }
	else                         { currPanel->get_home( row, col ) ; }
}


void pApplication::get_cursor( uint& row, uint& col )
{
	if ( panelList.size() == 0 ) { row = 0 ; col = 0                 ; }
	else                         { currPanel->get_cursor( row, col ) ; }
}


string pApplication::get_current_panelDescr()
{
	if ( panelList.size() == 0 ) { return ""                          ; }
	else                         { return currPanel->get_panelDescr() ; }
}


string pApplication::get_current_screenName()
{
	// This is called from lspf thread, so we need to set the correct shared pool for this
	// application thread when getting ZSCRNAME.  Must be set back after this call in lspf

	errblock err ;

	p_poolMGR->setShrdPool( err, shrdPool ) ;
	return p_poolMGR->get( err, "ZSCRNAME", SHARED ) ;
}


bool pApplication::inputInhibited()
{
	if ( panelList.size() > 0 ) { return currPanel->inputInhibited() ; }
	return false ;
}


bool pApplication::msgInhibited()
{
	if ( panelList.size() > 0 ) { return currPanel->msgInhibited() ; }
	return false ;
}


void pApplication::msgResponseOK()
{
	if ( panelList.size() > 0 ) { currPanel->msgResponseOK() ; }
}


void pApplication::store_scrname()
{
	errblock err ;

	ZSCRNAME = p_poolMGR->get( err, "ZSCRNAME", SHARED ) ;
}


bool pApplication::errorsReturn()
{
	return ControlErrorsReturn ;
}


void pApplication::setTestMode()
{
	testMode = true ;
	errBlock.setDebugMode() ;
}


void pApplication::restore_Zvars( int screenID )
{
	errblock err ;

	if ( panelList.size() > 0 ) { currPanel->update_keylist_vars() ; }

	if ( p_poolMGR->get( err, screenID, "ZSCRNAM2" ) == "PERM" )
	{
			ZSCRNAME = p_poolMGR->get( err, screenID, "ZSCRNAME" ) ;
	}
	p_poolMGR->put( err, "ZSCRNAME", ZSCRNAME, SHARED ) ;

	p_poolMGR->put( err, screenID, "ZPROW", d2ds( addpop_row ) ) ;
	p_poolMGR->put( err, screenID, "ZPCOL", d2ds( addpop_col ) ) ;
}


void pApplication::refresh_id()
{
	if ( panelList.size() > 0 ) { currPanel->display_id() ; }
}


void pApplication::set_msg( const string& msg_id )
{
	// Display a message on current panel using msg_id

	// This is called from mainline lspf, so set CONTROL ERRORS RETURN and restore
	// original CONTROL ERROR settings on exit.  This is so lspf does not hang on an error.

	bool errorStatus ;

	if ( panelList.size() == 0 ) { return ; }

	errorStatus = ControlErrorsReturn ;

	ControlErrorsReturn = true ;
	get_Message( msg_id ) ;
	if ( RC == 0 )
	{
		currPanel->clear_msg_loc() ;
		currPanel->set_panel_msg( MSG, MSGID ) ;
		currPanel->display_msg()   ;
	}
	ControlErrorsReturn = errorStatus  ;
}


void pApplication::set_msg1( const slmsg& t, string msgid, bool Immed )
{
	// Propogate setmsg() to the current panel using the short-long-message object.
	// If immed, display the message now rather than allow the application DISPLAY()/TBDISPL() service to it.

	// MSG1 and MSGID1 are used to store messages issued by the setmsg() service after variable substitution.

	MSG1   = t     ;
	MSGID1 = msgid ;
	setMSG = true  ;

	if ( Immed && panelList.size() > 0 )
	{
		currPanel->set_panel_msg( MSG1, MSGID1 ) ;
		currPanel->display_msg() ;
		setMSG = false ;
	}
}


bool pApplication::nretriev_on()
{
	if ( panelList.size() == 0 ) { return false                     ; }
	else                         { return currPanel->get_nretriev() ; }
}


string pApplication::get_nretfield()
{
	if ( panelList.size() == 0 ) { return ""                         ; }
	else                         { return currPanel->get_nretfield() ; }
}


void pApplication::display( string p_name, const string& p_msg, const string& p_cursor, int p_csrpos )
{
	string ZZVERB ;

	const string e1 = "Error during DISPLAY of panel " + p_name ;
	const string e2 = "Error processing )INIT section of panel "   ;
	const string e3 = "Error processing )REINIT section of panel " ;
	const string e4 = "Error processing )PROC section of panel "   ;
	const string e5 = "Error during update of panel " ;
	const string e6 = "Error updating field values of panel " ;

	bool doReinit ;

	RC       = 0     ;
	doReinit = false ;

	if ( p_name == "" )
	{
		if ( PANELID == "" )
		{
			errBlock.setcall( e1, "PSYE021C" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		p_name   = PANELID ;
		doReinit = true    ;
	}

	if ( p_cursor != "" && !isvalidName( p_cursor ) )
	{
		errBlock.setcall( e1, "PSYE021D", p_name, p_cursor ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	createPanel( p_name ) ;
	if ( !errBlock.RC0() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( propagateEnd )
	{
		if ( PPANELID == p_name ) { propagateEnd = false ;                }
		else                      { PPANELID = p_name ; RC = 8 ; return ; }
	}

	currPanel = panelList[ p_name ] ;

	if ( setMSG )
	{
		if ( !ControlNonDispl )
		{
			currPanel->set_panel_msg( MSG1, MSGID1 ) ;
			setMSG = false ;
		}
	}
	else if ( p_msg != "" )
	{
		get_Message( p_msg ) ;
		if ( RC > 0 ) { return ; }
		currPanel->set_panel_msg( MSG, MSGID ) ;
	}
	else
	{
		currPanel->clear_msg() ;
	}
	PANELID           = p_name   ;
	currPanel->CURFLD = p_cursor ;
	currPanel->CURPOS = p_csrpos ;

	if ( addpop_active ) { currPanel->set_popup( addpop_row, addpop_col ) ; }
	else                 { currPanel->remove_popup()                      ; }

	p_poolMGR->put( errBlock, "ZPANELID", p_name, SHARED, SYSTEM ) ;

	if ( doReinit )
	{
		currPanel->display_panel_reinit( errBlock ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e3 + p_name ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else
	{
		currPanel->display_panel_init( errBlock ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e2 + p_name ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		currPanel->update_field_values( errBlock ) ;
		RC = errBlock.RC ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e6 + p_name ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}

	if ( p_poolMGR->get( errBlock, "ZSCRNAM1", SHARED ) == "ON" &&
	     p_poolMGR->get( errBlock, lscreen_num, "ZSCRNAM2" ) == "PERM" )
	{
			p_poolMGR->put( errBlock, "ZSCRNAME", p_poolMGR->get( errBlock, lscreen_num, "ZSCRNAME" ), SHARED ) ;
	}

	while ( true )
	{
		currPanel->display_panel( errBlock ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e1 ) ;
			checkRCode( errBlock ) ;
			break ;
		}
		currPanel->cursor_to_field( RC ) ;
		if ( RC > 0 )
		{
			errBlock.setcall( e1, "PSYE022N", currPanel->CURFLD ) ;
			checkRCode( errBlock ) ;
			break ;
		}
		wait_event() ;
		ControlDisplayLock = false ;
		ControlNonDispl    = false ;
		currPanel->display_panel_update( errBlock ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e5 + p_name ) ;
			checkRCode( errBlock ) ;
			break ;
		}

		currPanel->display_panel_proc( errBlock, 0 ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e4 + p_name ) ;
			checkRCode( errBlock ) ;
			break ;
		}

		ZZVERB = p_poolMGR->get( errBlock, "ZVERB", SHARED ) ;
		if ( !errBlock.RC0() )
		{
			errBlock.setcall( e1, "PSYE015L", "GET", "ZVERB" ) ;
			checkRCode( errBlock ) ;
			break ;
		}
		if ( ZZVERB == "RETURN" ) { propagateEnd = true ; }
		if ( findword( ZZVERB, "END EXIT RETURN" ) ) { RC = 8 ; return ; }

		if ( currPanel->MSGID != "" )
		{
			get_Message( currPanel->MSGID ) ;
			if ( RC > 0 ) { break ; }
			currPanel->set_panel_msg( MSG, MSGID ) ;
			currPanel->display_panel_reinit( errBlock ) ;
			RC = errBlock.getRC() ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e3 + p_name ) ;
				checkRCode( errBlock ) ;
				break ;
			}
			continue ;
		}
		break ;
	}
	currPanel->clear_msg()  ;
	currPanel->resetAttrs() ;
}


void pApplication::libdef( const string& lib, const string& type, const string& id, const string& procopt )
{
	// libdef - Add/remove a list of paths to the search order for panels, messages and tables
	// Format: LIBDEF ZxLIB                    - remove LIBDEF for search
	//         LIBDEF ZxLIB FILE ID(path-list) - add path-list to the search path

	// To do:
	//         processing options

	// RC = 0   Normal completion
	// RC = 4   Removing a LIBDEF that was not in effect
	// RC = 16  No paths in the ID() parameter
	// RC = 20  Severe error

	const string e1 = "LIBDEF error" ;

	RC = 0 ;

	if ( !findword( lib, "ZMLIB ZPLIB ZTLIB" ) )
	{
		errBlock.setcall( e1, "PSYE022H", lib ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( !findword( procopt, "COND UNCOND STACK STKADD" ) )
	{
		errBlock.setcall( e1, "PSYE022I", procopt ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( type == "" )
	{
		if ( lib == "ZMLIB" )
		{
			if ( !libdef_muser )
			{
				RC = 4 ;
				return ;
			}
			libdef_muser = false ;
			zmuser.pop() ;
		}
		else if ( lib == "ZPLIB" )
		{
			if ( !libdef_puser )
			{
				RC = 4 ;
				return ;
			}
			libdef_puser = false ;
			zpuser.pop() ;
		}
		else
		{
			if ( !libdef_tuser )
			{
				RC = 4 ;
				return ;
			}
			libdef_tuser = false ;
			ztuser.pop() ;
		}
	}
	else if ( type == "FILE" )
	{
		if ( id == "" )
		{
			errBlock.setcall( e1, "PSYE022F", 16 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		if ( lib == "ZMLIB" )
		{
			if ( zmuser.empty() ) { zmuser.push( id ) ; }
			else                  { zmuser.top() = id ; }
			libdef_muser = true ;
		}
		else if ( lib == "ZPLIB" )
		{
			if ( zpuser.empty() ) { zpuser.push( id ) ; }
			else                  { zpuser.top() = id ; }
			libdef_puser = true ;
		}
		else if ( lib == "ZTLIB" )
		{
			if ( ztuser.empty() ) { ztuser.push( id ) ; }
			else                  { ztuser.top() = id ; }
			libdef_tuser = true ;
		}
	}
	else
	{
		errBlock.setcall( e1, "PSYE022G", type ) ;
		checkRCode( errBlock ) ;
	}
}


string pApplication::get_search_path( s_paths p )
{
	switch ( p )
	{
	case s_ZMLIB:
		if ( libdef_muser ) { return mergepaths( zmuser.top(), ZMLIB ) ; }
		else                { return ZMLIB                             ; }
		break ;
	case s_ZPLIB:
		if ( libdef_puser ) { return mergepaths( zpuser.top(), ZPLIB ) ; }
		else                { return ZPLIB                             ; }
		break ;
	case s_ZTLIB:
		if ( libdef_tuser ) { return mergepaths( ztuser.top(), ZTLIB ) ; }
		else                { return ZTLIB                             ; }
		break ;
	}
	return "" ;
}


string pApplication::get_select_cmd( const string& opt )
{
	// Get system variable ZSEL for the SELECT PANEL command.  If not set, use the panel command entry.

	string t ;

	vcopy( "ZSEL", t, MOVE ) ;
	if ( t == "" )
	{
		t = panelList[ PANELID ]->return_command( opt ) ;
	}
	vreplace( "ZSEL", "" ) ;
	return t ;
}


void pApplication::set_cursor( int row, int col )
{
	if ( panelList.size() > 0 ) { currPanel->set_cursor( row, col ) ; }
}


void pApplication::vdefine( const string& names, int * i_ad1, int * i_ad2, int * i_ad3, int * i_ad4, int * i_ad5, int * i_ad6, int * i_ad7, int * i_ad8 )
{
	// RC = 0  Normal completion
	// RC = 20 Severe Error
	// (funcPOOL.define returns 0 or 20)

	int w ;

	string name ;

	RC = 0 ;

	w = words( names ) ;
	if ( ( w > 8 ) || ( w < 1 ) )
	{
		errBlock.seterrid( "PSYE022D" ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( i_ad1 == NULL )
	{
		errBlock.seterrid( "PSYE022E" ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	name = word( names, 1 ) ;
	funcPOOL.define( errBlock, name, i_ad1 ) ;
	if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }

	if ( w > 1 )
	{
		if ( i_ad2 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 2 ) ;
		funcPOOL.define( errBlock, name, i_ad2 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 2 )
	{
		if ( i_ad3 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 3 ) ;
		funcPOOL.define( errBlock, name, i_ad3 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 3 )
	{
		if ( i_ad4 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 4 ) ;
		funcPOOL.define( errBlock, name, i_ad4 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 4 )
	{
		if ( i_ad5 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 5 ) ;
		funcPOOL.define( errBlock, name, i_ad5 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 5 )
	{
		if ( i_ad6 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 6 ) ;
		funcPOOL.define( errBlock, name, i_ad6 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}

	if ( w > 6 )
	{
		if ( i_ad7 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 7 ) ;
		funcPOOL.define( errBlock, name, i_ad7 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}

	if ( w > 7 )
	{
		if ( i_ad8 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 8 ) ;
		funcPOOL.define( errBlock, name, i_ad8 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
}



void pApplication::vdefine( const string& names, string * s_ad1, string * s_ad2, string * s_ad3, string * s_ad4, string * s_ad5, string * s_ad6, string * s_ad7, string * s_ad8 )
{
	// RC = 0  Normal completion
	// RC = 20 Severe Error
	// (funcPOOL.define returns 0 or 20)

	int w  ;

	string name ;

	RC = 0 ;

	w = words( names ) ;
	if ( ( w > 8 ) || ( w < 1 ) )
	{
		errBlock.seterrid( "PSYE022D" ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( s_ad1 == NULL )
	{
		errBlock.seterrid( "PSYE022E" ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	name = word( names, 1 ) ;
	funcPOOL.define( errBlock, name, s_ad1 ) ;
	if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }

	if ( w > 1 )
	{
		if ( s_ad2 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 2 ) ;
		funcPOOL.define( errBlock, name, s_ad2 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 2 )
	{
		if ( s_ad3 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 3 ) ;
		funcPOOL.define( errBlock, name, s_ad3 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 3 )
	{
		if ( s_ad4 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 4 ) ;
		funcPOOL.define( errBlock, name, s_ad4 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 4 )
	{
		if ( s_ad5 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 5 ) ;
		funcPOOL.define( errBlock, name, s_ad5 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 5 )
	{
		if ( s_ad6 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 6 ) ;
		funcPOOL.define( errBlock, name, s_ad6 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 6 )
	{
		if ( s_ad7 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 7 ) ;
		funcPOOL.define( errBlock, name, s_ad7 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
	if ( w > 7 )
	{
		if ( s_ad8 == NULL )
		{
			errBlock.seterrid( "PSYE022E" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		name = word( names, 8 ) ;
		funcPOOL.define( errBlock, name, s_ad8 ) ;
		if ( errBlock.error() ) { checkRCode( errBlock ) ; return ; }
	}
}


void pApplication::vdelete( const string& names )
{
	// RC = 0  Normal completion
	// RC = 8  At least one variable not found
	// RC = 20 Severe error
	// (funcPOOL.dlete returns 0, 8 or 20)

	int i  ;
	int ws ;

	errBlock.setmaxRC( 0 ) ;

	ws = words( names ) ;
	for ( i = 1 ; i <= ws ; i++ )
	{
		funcPOOL.dlete( errBlock, word( names, i ) ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( "VDELETE failed" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		errBlock.setmaxRC() ;
	}
	RC = errBlock.getmaxRC() ;
}


void pApplication::vmask( const string& name, const string& type, const string& mask )
{
	// Set a mask for a function pool variable (must be vdefined first)
	// Partial implementation as no VEDIT panel statement yet so this is never used

	// RC = 0  Normal completion
	// RC = 8  Variable not found
	// RC = 20 Severe error
	// (funcPOOL.setmask returns 0, 8 or 20)

	int i ;

	const string e1    = "VMASK error" ;
	const string fmask = "IDATE STDDATE ITIME STDTIME JDATE JSTD" ;

	if ( type == "FORMAT" )
	{
		if ( wordpos( mask, fmask ) == 0 )
		{
			errBlock.setcall( e1, "PSYE022P", mask, fmask ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( type == "USER" )
	{
		if ( mask.size() > 20 )
		{
			errBlock.setcall( e1, "PSYE022Q" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		else
		{
			for ( i = 0 ; i < mask.size() ; i++ )
			{
				if ( mask[i] != 'A' && mask[i] != 'B' && mask[i] != '9' &&
				     mask[i] != 'H' && mask[i] != 'N' && mask[i] != 'V' &&
				     mask[i] != 'S' && mask[i] != 'X' && mask[i] != '(' &&
				     mask[i] != ')' && mask[i] != '-' && mask[i] != '/' &&
				     mask[i] != ',' && mask[i] != '.' )
				{
					errBlock.setcall( e1, "PSYE022S", mask ) ;
					checkRCode( errBlock ) ;
					return ;
				}
			}
		}
	}
	else
	{
		errBlock.setcall( e1, "PSYE022R", type ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	funcPOOL.setmask( errBlock, name, mask ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::vreset()
{
	// RC = 0  Normal completion
	// RC = 20 Severe error
	// (funcPOOL.reset returns 0)

	funcPOOL.reset( errBlock ) ;
	RC = errBlock.getRC() ;
}


void pApplication::vreplace( const string& name, const string& s_val )
{
	// RC = 0  Normal completion
	// RC = 20 Severe error
	// (funcPOOL.put returns 0, 20)

	funcPOOL.put( errBlock, name, s_val ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "VREPLACE failed" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::vreplace( const string& name, int i_val )
{
	// RC = 0  Normal completion
	// RC = 20 Severe error
	// (funcPOOL.put returns 0 or 20)

	funcPOOL.put( errBlock, name, i_val ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "VREPLACE failed" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::vget( const string& names, poolType pType )
{
	// RC = 0  Normal completion
	// RC = 8  Variable not found
	// RC = 20 Severe error
	// (funcPOOL.put returns 0 or 20)
	// (funcPOOL.getType returns 0, 8 or 20.  For RC = 8 create implicit function pool variable)
	// (poolMGR.get return 0, 8 or 20)

	int ws      ;
	int i       ;
	string val  ;
	string name ;

	const string e1 = "VGET failed" ;

	dataType var_type ;

	errBlock.setmaxRC( 0 ) ;

	ws = words( names ) ;
	for ( i = 1 ; i <= ws ; i++ )
	{
		name = word( names, i ) ;
		val  = p_poolMGR->get( errBlock, name, pType ) ;
		if ( errBlock.RC0() )
		{
			var_type = funcPOOL.getType( errBlock, name ) ;
			if ( errBlock.RC0() )
			{
				switch ( var_type )
				{
				case INTEGER:
					funcPOOL.put( errBlock, name, ds2d( val ) ) ;
					break ;
				case STRING:
					funcPOOL.put( errBlock, name, val ) ;
				}
			}
			else if ( errBlock.RC8() )
			{
				funcPOOL.put( errBlock, name, val ) ;
			}
		}
		if ( errBlock.error() )
		{
			errBlock.setcall( e1 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		errBlock.setmaxRC() ;
	}
	RC = errBlock.getmaxRC() ;
}


void pApplication::vput( const string& names, poolType pType )
{
	// RC = 0  Normal completion
	// RC = 8  Variable not found in function pool
	// RC = 12 Read-only variable
	// RC = 16 Truncation occured
	// RC = 20 Severe error
	// (funcPOOL.get returns 0, 8 or 20)
	// (funcPOOL.getType returns 0, 8 or 20)
	// (poolMGR.put return 0, 12, 16 or 20)

	int i  ;
	int ws ;

	string s_val ;
	string name  ;

	const string e1 = "VPUT failed" ;

	dataType var_type ;

	errBlock.setmaxRC( 0 ) ;

	ws = words( names ) ;
	for ( i = 1 ; i <= ws ; i++ )
	{
		name     = word( names, i ) ;
		var_type = funcPOOL.getType( errBlock, name ) ;
		if ( errBlock.RC0() )
		{
			switch ( var_type )
			{
			case INTEGER:
				s_val = d2ds( funcPOOL.get( errBlock, 0, var_type, name ) ) ;
				break ;
			case STRING:
				s_val = funcPOOL.get( errBlock, 0, name ) ;
			}
			if ( errBlock.error() )
			{
				errBlock.setcall( e1 ) ;
				checkRCode( errBlock ) ;
				return ;
			}
			p_poolMGR->put( errBlock, name, s_val, pType ) ;
		}
		if ( errBlock.error() )
		{
			errBlock.setcall( e1 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		errBlock.setmaxRC() ;
	}
	RC = errBlock.getmaxRC() ;
}


void pApplication::vcopy( const string& var, string& val, vcMODE mode )
{
	// Retrieve a copy of a dialogue variable name in var and move to variable val
	// (normal dialogue variable search order)
	// This routine is only valid for MODE=MOVE

	// RC = 0  Normal completion
	// RC = 8  Variable not found
	// RC = 16 Truncation occured
	// RC = 20 Severe error
	// (funcPOOL.get returns 0, 8 or 20)
	// (funcPOOL.getType returns 0, 8 or 20)
	// (poolMGR.get return 0, 8 or 20)

	dataType var_type ;

	const string e1 = "VCOPY failed" ;

	switch ( mode )
	{
	case LOCATE:
		errBlock.setcall( e1, "PSYE022A" ) ;
		break ;

	case MOVE:
		var_type = funcPOOL.getType( errBlock, var ) ;
		if ( errBlock.RC0() )
		{
			switch ( var_type )
			{
			case INTEGER:
				val = d2ds( funcPOOL.get( errBlock, 0, var_type, var ) ) ;
				break ;
			case STRING:
				val = funcPOOL.get( errBlock, 0, var ) ;
			}
			if ( errBlock.error() )
			{
				errBlock.setcall( e1 ) ;
				checkRCode( errBlock ) ;
				break ;
			}
		}
		else if ( errBlock.RC8() )
		{
			val = p_poolMGR->get( errBlock, var, ASIS ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e1 ) ;
				checkRCode( errBlock ) ;
				return ;
			}
		}
		else
		{
			errBlock.setcall( e1 ) ;
			checkRCode( errBlock ) ;
		}
	}
	RC = errBlock.getRC() ;
}


void pApplication::vcopy( const string& var, string * & p_val, vcMODE mode )
{
	// Return the address of a dialogue variable name in var, in p_val
	// (normal dialogue variable search order)
	// This routine is only valid for MODE=LOCATE
	// MODE=LOCATE not valid for integer pointers as these may be in the variable pools as strings

	// RC = 0  Normal completion
	// RC = 8  Variable not found
	// RC = 16 Truncation occured
	// RC = 20 Severe error
	// (funcPOOL.get returns 0, 8 or 20)
	// (funcPOOL.getType returns 0, 8 or 20)
	// (funcPOOL.vlocate returns 0, 8 or 20)
	// (poolMGR.get return 0, 8 or 20)

	dataType var_type  ;

	const string e1 = "VCOPY failed" ;

	switch ( mode )
	{
	case LOCATE:
		var_type = funcPOOL.getType( errBlock, var ) ;
		if ( errBlock.RC0() )
		{
			switch ( var_type )
			{
			case INTEGER:
				errBlock.setcall( e1, "PSYE022C" ) ;
				break ;
			case STRING:
				p_val = funcPOOL.vlocate( errBlock, var, CHECK ) ;
			}
			if ( errBlock.error() )
			{
				errBlock.setcall( e1 ) ;
				checkRCode( errBlock ) ;
			}
		}
		else if ( errBlock.RC8() )
		{
			p_val = p_poolMGR->vlocate( errBlock, var, ASIS ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e1 ) ;
				checkRCode( errBlock ) ;
			}
		}
		else
		{
			errBlock.setcall( e1 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		break ;
	case MOVE:
		errBlock.setcall( e1, "PSYE022B" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::verase( const string& names, poolType pType )
{
	// RC = 0  Normal completion
	// RC = 8  Variable not found
	// RC = 12 Read-only variable
	// RC = 20 Severe error
	// (poolMGR.erase return 0, 8 12 or 20)


	int i  ;
	int ws ;

	string name ;

	errBlock.setmaxRC( 0 ) ;

	ws = words( names ) ;
	for ( i = 1 ; i <= ws ; i++ )
	{
		name = word( names, i ) ;
		p_poolMGR->erase( errBlock, name, pType ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( "VERASE failed" ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		errBlock.setmaxRC() ;
	}
	RC = errBlock.getmaxRC() ;
}


string pApplication::vlist( poolType pType, int lvl )
{
	return p_poolMGR->vlist( RC, pType, lvl ) ;
}


string pApplication::vilist( vdType defn )
{
	return funcPOOL.vilist( RC, defn ) ;
}


string pApplication::vslist( vdType defn )
{
	return funcPOOL.vslist( RC, defn ) ;
}


void pApplication::addpop( const string& a_fld, int a_row, int a_col )
{
	//  Create pop-up window and set row/col for the next panel display.  If addpop() is already active, store old values for next rempop()
	//  Position of addpop is relative to row=1, col=3 or the previous addpop() position for this logical screen.
	//  Defaults are 0,0 giving row=1, col=3 (or 2,4 when starting at 1,1)

	//  RC = 0  Normal completion
	//  RC = 12 No panel displayed before addpop() service when using field parameter
	//  RC = 20 Severe error

	const string e1 = "ADDPOP error" ;

	uint p_row = 0 ;
	uint p_col = 0 ;

	if ( a_fld != "" )
	{
		if ( panelList.size() == 0 )
		{
			errBlock.setcall( e1, "PSYE022L", 12 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		if ( !currPanel->field_get_row_col( a_fld, p_row, p_col ) )
		{
			errBlock.setcall( e1, "PSYE022M", a_fld,  PANELID, 20 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		a_row += p_row ;
		a_col += p_col - 4 ;
	}
	else
	{
		a_row += ds2d( p_poolMGR->get( errBlock, lscreen_num, "ZPROW" ) ) ;
		a_col += ds2d( p_poolMGR->get( errBlock, lscreen_num, "ZPCOL" ) ) ;
	}
	if ( addpop_active )
	{
		addpop_stk.push( addpop_row ) ;
		addpop_stk.push( addpop_col ) ;
	}

	addpop_active = true ;
	addpop_row = (a_row <  0 ) ? 1 : a_row + 2 ;
	addpop_col = (a_col < -1 ) ? 2 : a_col + 4 ;

	p_poolMGR->put( errBlock, lscreen_num, "ZPROW", d2ds( addpop_row ) ) ;
	p_poolMGR->put( errBlock, lscreen_num, "ZPCOL", d2ds( addpop_col ) ) ;
	RC = errBlock.getRC() ;
}


void pApplication::rempop( const string& r_all )
{
	//  Remove pop-up window.  Restore previous rempop() if there is one (push order row,col).

	//  RC = 0  Normal completion
	//  RC = 16 No pop-up window exists at this level
	//  RC = 20 Severe error

	const string e1 = "REMPOP error" ;

	RC = 0 ;

	if ( !addpop_active )
	{
		errBlock.setcall( e1, "PSYE022T", 16 ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( r_all == "" )
	{
		if ( !addpop_stk.empty() )
		{
			addpop_col = addpop_stk.top() ;
			addpop_stk.pop() ;
			addpop_row = addpop_stk.top() ;
			addpop_stk.pop() ;
		}
		else
		{
			addpop_active = false ;
			addpop_row    = 0 ;
			addpop_col    = 0 ;
		}
	}
	else if ( r_all == "ALL" )
	{
		while ( !addpop_stk.empty() )
		{
			addpop_stk.pop() ;
		}
		addpop_active = false ;
		addpop_row    = 0 ;
		addpop_col    = 0 ;
	}
	else
	{
		errBlock.setcall( e1, "PSYE022U", r_all ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	p_poolMGR->put( errBlock, lscreen_num, "ZPROW", d2ds( addpop_row ) ) ;
	p_poolMGR->put( errBlock, lscreen_num, "ZPCOL", d2ds( addpop_col ) ) ;
	RC = errBlock.getRC() ;
}


void pApplication::movepop()
{
	if ( addpop_active )
	{
		currPanel->set_popup( addpop_row, addpop_col ) ;
		currPanel->move_popup() ;
	}
}


void pApplication::control( const string& parm1, const string& parm2, const string& parm3 )
{
	// CONTROL ERRORS CANCEL - abend for RC >= 12
	// CONTROL ERRORS RETURN - return to application for any RC

	// CONTROL DISPLAY SAVE/RESTORE - SAVE/RESTORE status for a TBDISPL
	//         SAVE/RESTORE saves/restores the six ZTD* variables in the function pool as well
	//         as the currtbPanel pointer for retrieving other model sets via tbdispl with no panel specified
	//         Only necessary if a tbdispl invokes another tbdispl in the same task

	// CONTROL SPLIT DISABLE - RC=8 if screen already split

	// CONTROL PASSTHRU LRSCROLL  PASON | PASOFF | PASQUERY

	// CONTROL REFLIST UPDATE
	// CONTROL REFLIST NOUPDATE

	// lspf extensions:
	// CONTROL TIMEOUT  ENABLE  - Enable application timeouts after ZWAITMAX ms (default).
	// CONTROL TIMEOUT  DISABLE - Disable forced abend of applications if ZWAITMAX exceeded.
	// CONTROL ABENDRTN DEFAULT - Reset abend routine to the default, pApplication::cleanup_default
	// CONTROL RDISPLAY FLUSH   - Flush raw output to the screen

	map<string, pPanel *>::iterator it;

	const string e1 = "CONTROL error" ;

	errBlock.setRC( 0 ) ;

	if ( parm3 != "" && parm1 != "PASSTHRU" )
	{
		errBlock.setcall( e1, "PSYE022V", parm1 ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( parm1 == "DISPLAY" )
	{
		if ( parm2 == "LOCK" )
		{
			ControlDisplayLock = true ;
		}
		else if ( parm2 == "NONDISPL" )
		{
			ControlNonDispl = true ;
		}
		else if ( parm2 == "REFRESH" )
		{
			refresh() ;
		}
		else if ( parm2 == "SAVE" )
		{
			stk_str.push( ZTDMARK  ) ;
			stk_int.push( ZTDDEPTH ) ;
			stk_int.push( ZTDROWS  ) ;
			stk_int.push( ZTDSELS  ) ;
			stk_int.push( ZTDTOP   ) ;
			stk_int.push( ZTDVROWS ) ;
			SRpanelStack.push( currtbPanel ) ;
		}
		else if ( parm2 == "RESTORE" )
		{
			if ( SRpanelStack.empty() )
			{
				errBlock.setcall( e1, "PSYE022W" ) ;
				checkRCode( errBlock ) ;
				return ;
			}
			else
			{
				ZTDVROWS = stk_int.top() ;
				stk_int.pop() ;
				ZTDTOP   = stk_int.top() ;
				stk_int.pop() ;
				ZTDSELS  = stk_int.top() ;
				stk_int.pop() ;
				ZTDROWS  = stk_int.top() ;
				stk_int.pop() ;
				ZTDDEPTH = stk_int.top() ;
				stk_int.pop() ;
				ZTDMARK  = stk_str.top() ;
				stk_str.pop() ;
				currtbPanel = SRpanelStack.top() ;
				SRpanelStack.pop() ;
			}
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "DISPLAY", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( parm1 == "ERRORS" )
	{
		if ( parm2 == "RETURN" )
		{
			ControlErrorsReturn = true ;
		}
		else if ( parm2 == "CANCEL" )
		{
			ControlErrorsReturn = false ;
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "ERRORS", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( parm1 == "PASSTHRU" )
	{
		if ( parm2 == "LRSCROLL" )
		{
			if ( parm3 == "PASON" )
			{
				ControlPassLRScroll = true ;
				for ( it = panelList.begin() ; it != panelList.end() ; it++ )
				{
					it->second->LRScroll = true ;
				}
			}
			else if ( parm3 == "PASOFF" )
			{
				ControlPassLRScroll = false ;
				for ( it = panelList.begin() ; it != panelList.end() ; it++ )
				{
					it->second->LRScroll = false ;
				}
			}
			else if ( parm3 == "PASQUERY" )
			{
				errBlock.setRC( ControlPassLRScroll ? 1 : 0 ) ;
			}
			else
			{
				errBlock.setcall( e1, "PSYE022X", "PASSTHRU LRSCROLL", parm3 ) ;
				checkRCode( errBlock ) ;
				return ;
			}
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "PASSTHRU", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( parm1 == "RDISPLAY" )
	{
		if ( parm2 == "FLUSH" )
		{
			rawOutput = true  ;
			wait_event()      ;
			rawOutput = false ;
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "RDISPLAY", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( parm1 == "REFLIST" )
	{
		if ( parm2 == "UPDATE" )
		{
			ControlRefUpdate = true ;
		}
		else if ( parm2 == "NOUPDATE" )
		{
			ControlRefUpdate = false ;
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "REFLIST", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( parm1 == "SPLIT" )
	{
		if ( parm2 == "ENABLE" )
		{
			ControlSplitEnable = true ;
		}
		else if ( parm2 == "DISABLE" )
		{
			if ( p_poolMGR->get( errBlock, "ZSPLIT", SHARED ) == "YES" )
			{
				errBlock.setRC( 8 ) ;
			}
			else
			{
				ControlSplitEnable = false ;
			}
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "SPLIT", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( parm1 == "TIMEOUT" )
	{
		if ( parm2 == "ENABLE" )
		{
			noTimeOut = false ;
		}
		else if ( parm2 == "DISABLE" )
		{
			noTimeOut = true ;
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "TIMEOUT", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else if ( parm1 == "ABENDRTN" )
	{
		if ( parm2 == "DEFAULT" )
		{
			pcleanup = &pApplication::cleanup_default ;
		}
		else
		{
			errBlock.setcall( e1, "PSYE022X", "ABENDTRN", parm2 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else
	{
		errBlock.setcall( e1, "PSYE022Y", parm1 ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::control( const string& parm1, void (pApplication::*pFunc)() )
{
	// lspf extensions:
	// CONTROL ABENDRTN ptr_to_routine - Set the routine to get control during an abend

	const string e1 = "CONTROL error" ;

	errBlock.setRC( 0 ) ;

	if ( parm1 == "ABENDRTN" )
	{
		pcleanup = pFunc ;
	}
	else
	{
		errBlock.setcall( e1, "PSYE022Y", parm1 ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::log( const string& msgid )
{
	// RC = 0   Normal completion
	// RC = 12  Message not found or message syntax error
	// RC = 20  Severe error

	string t ;

	getmsg( msgid, "ZERRSM", "ZERRLM" ) ;

	if ( RC == 0 )
	{
		vcopy( "ZERRSM", t, MOVE ) ;
		llog( "L", t << endl )     ;
		vcopy( "ZERRLM", t, MOVE ) ;
		llog( "L", t << endl )     ;
	}
}


void pApplication::tbadd( const string& tb_name, const string& tb_namelst, const string& tb_order, int tb_num_of_rows )
{
	// Add a row to a table

	// RC = 0   Normal completion
	// RC = 8   For keyed tables only, row already exists
	// RC = 12  Table not open
	// RC = 20  Severe error

	const string e1 = "TBADD error" ;

	if ( tb_order != "" && tb_order != "ORDER" )
	{
		errBlock.setcall( e1, "PSYE023A", "TBADD", tb_order ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( tb_num_of_rows < 0 || tb_num_of_rows > 65535 )
	{
		errBlock.setcall( e1, "PSYE023B", "TBADD", d2ds( tb_num_of_rows ) ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( !isTableOpen( tb_name, "TBADD" ) ) { return ; }

	p_tableMGR->tbadd( errBlock, funcPOOL, tb_name, tb_namelst, tb_order, tb_num_of_rows ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbbottom( const string& tb_name, const string& tb_savenm, const string& tb_rowid_vn, const string& tb_noread, const string& tb_crp_name )
{
	// Move row pointer to the bottom

	// RC = 0   Normal completion
	// RC = 8   Table is empty.  CRP is set to 0
	// RC = 12  Table not open
	// RC = 20  Severe error

	if ( !isTableOpen( tb_name, "TBBOTTOM" ) ) { return ; }

	p_tableMGR->tbbottom( errBlock, funcPOOL, tb_name, tb_savenm, tb_rowid_vn, tb_noread, tb_crp_name  ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBBOTTOM error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbclose( const string& tb_name, const string& tb_newname, const string& tb_path )
{
	// Save and close the table (calls saveTable and destroyTable routines).
	// If table opened in NOWRITE mode, just remove table from storage.

	// RC = 0   Normal completion
	// RC = 12  Table not open
	// RC = 16  Path error
	// RC = 20  Severe error

	errBlock.setRC( 0 ) ;

	if ( !isTableOpen( tb_name, "TBCLOSE" ) ) { return ; }

	if ( p_tableMGR->writeableTable( errBlock, tb_name ) )
	{
		p_tableMGR->saveTable( errBlock, taskid(), tb_name, tb_newname, tb_path ) ;
	}
	if ( errBlock.RC0() )
	{
		p_tableMGR->destroyTable( errBlock, taskid(), tb_name ) ;
		if ( errBlock.RC0() )
		{
			tablesOpen.erase( tb_name ) ;
		}
	}
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBCLOSE error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbcreate( const string& tb_name, const string& keys, const string& names, tbWRITE m_WRITE, tbREP m_REP, string m_path, tbDISP m_DISP )
{
	// Create a new table.
	// For tables without a path specified and the WRITE option, default to the first path entry in ZTLIB

	// RC = 0   Normal completion
	// RC = 4   Normal completion - Table exists and REPLACE speified
	// RC = 8   Table exists and REPLACE not specified or REPLACE specified and opend in SHARE mode or REPLACE specified and opened in EXCLUSIVE mode but not the owning task
	// RC = 12  Table in use
	// RC = 16  ??
	// RC = 20  Severe error

	int ws ;
	int i  ;

	string w ;

	const string e1 = "TBCREATE error" ;

	RC = 0 ;

	if ( !isvalidName( tb_name ) )
	{
		errBlock.setcall( e1, "PSYE022J", "table", w ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( tablesOpen.count( tb_name ) > 0 && m_REP != REPLACE )
	{
		RC = 8 ;
		return ;
	}

	if ( m_WRITE == WRITE )
	{
		if ( m_path == "" ) { m_path = getpath( ZTLIB, 1 ) ; }
	}

	for ( ws = words( keys ), i = 1 ; i <= ws ; i++ )
	{
		w = word( keys, i ) ;
		if ( !isvalidName( w ) )
		{
			errBlock.setcall( e1, "PSYE022J", "key", w ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}

	for ( ws = words( names ), i = 1; i <= ws ; i++ )
	{
		w = word( names, i ) ;
		if ( !isvalidName( w ) )
		{
			errBlock.setcall( e1, "PSYE022J", "field", w ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}

	p_tableMGR->createTable( errBlock, taskid(), tb_name, keys, names, m_REP, m_WRITE, m_path, m_DISP ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	else
	{
		tablesOpen[ tb_name ] = true ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbdelete( const string& tb_name )
{
	// Delete a row in the table.  For keyed tables, the table is searched with the current key.  For non-keyed tables the current CRP is used.

	// RC = 0   Normal completion
	// RC = 8   Row does not exist for a keyed table or for non-keyed table, CRP was at TOP(zero)
	// RC = 12  Table not open
	// RC = 20  Severe error

	if ( !isTableOpen( tb_name, "TBDELETE" ) ) { return ; }

	p_tableMGR->tbdelete( errBlock, funcPOOL, tb_name ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBDELETE error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbdispl( const string& tb_name, string p_name, const string& p_msg, string p_cursor, int p_csrrow, int p_csrpos, string p_autosel, const string& p_crp_name, const string& p_rowid_nm )
{
	// tbdispl with panel, no message - clear previous pending lines, rebuild scrollable area and display panel
	// tbdispl with panel, message    - rebuild scrollable area and display panel and message
	// tbdispl no panel, no message   - retrieve next pending line.  If none, display panel
	// tbdispl no panel, message      - display panel with message.  No rebuilding of the scrollable area

	// Set CRP to first changed line, autoselected line, or tbtop if there are no selected lines

	// If .AUTOSEL and .CSRROW set in panel, override the parameters p_autosel and p_csrrow

	// Autoselect if the p_csrpos CRP is visible

	// Use separate pointer currtbPanel for tb displays so that a CONTROL DISPLAY SAVE/RESTORE is only necessary when a tbdispl issues another tbdispl and not
	// for a display of an ordinary panel

	// RC =  0  Normal completion
	// RC =  4  More than 1 row selected
	// RC =  8  End pressed
	// RC = 12  Panel, message or cursor field not found
	// RC = 20  Severe error

	int exitRC  ;
	int ws      ;
	int i       ;
	int ln      ;
	int csrvrow ;

	bool scan   ;

	string ZZVERB ;
	string URID   ;
	string s      ;
	string t      ;

	const string e1 = "Error during TBDISPL of panel "+ p_name ;
	const string e2 = "Error processing )INIT section of panel "   ;
	const string e3 = "Error processing )REINIT section of panel " ;
	const string e4 = "Error processing )PROC section of panel "   ;
	const string e5 = "Error during update of panel " ;
	const string e6 = "Error updating field values of panel " ;

	RC = 0 ;
	ln = 0 ;

	if ( propagateEnd )
	{
		if ( p_name == "" )       { RC = 8 ; return      ; }
		if ( PPANELID == p_name ) { propagateEnd = false ;                }
		else                      { PPANELID = p_name ; RC = 8 ; return ; }
	}

	if ( !isTableOpen( tb_name, "TBDISPL" ) ) { RC = 20 ; return ; }

	if ( p_autosel == "" ) { p_autosel = "YES" ; }

	if ( p_cursor != "" && !isvalidName( p_cursor ) )
	{
		errBlock.seterror( "Invalid CURSOR position" ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( p_autosel != "YES" &&  p_autosel != "NO" )
	{
		errBlock.seterror( "Invalid AUTOSEL parameter.  Must be YES or NO" ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( p_crp_name != "" && !isvalidName( p_crp_name ) )
	{
		errBlock.seterror( "Invalid CRP variable name" ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( p_rowid_nm != "" && !isvalidName( p_rowid_nm ) )
	{
		errBlock.seterror( "Invalid ROWID variable name" ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( p_name != "" )
	{
		createPanel( p_name ) ;
		if ( !errBlock.RC0() )
		{
			errBlock.setcall( e1 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
		currtbPanel = panelList[ p_name ] ;
		currPanel   = currtbPanel         ;
		PANELID     = p_name ;
		p_poolMGR->put( errBlock, "ZPANELID", p_name, SHARED, SYSTEM ) ;
		if ( p_msg == "" )
		{
			currtbPanel->tb_clear_linesChanged( errBlock ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e1 ) ;
				checkRCode( errBlock ) ;
				return ;
			}
		}
		scan = currtbPanel->get_tbscan() ;
		p_tableMGR->fillfVARs( errBlock, funcPOOL, tb_name, currtbPanel->get_tb_clear(), scan, currtbPanel->tb_depth ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e1 ) ;
			checkRCode( errBlock ) ;
			return ;
		}
	}
	else
	{
		p_poolMGR->put( errBlock, "ZVERB", "",  SHARED ) ;
		if ( p_msg == "" && currtbPanel->tb_lineChanged( ln, URID ) )
		{
			currtbPanel->tb_remove_lineChanged() ;
		}
		else
		{
			p_name    = currtbPanel->PANELID ;
			PANELID   = currtbPanel->PANELID ;
			currPanel = currtbPanel ;
			p_poolMGR->put( errBlock, "ZPANELID", p_name, SHARED, SYSTEM ) ;
		}
		scan = currtbPanel->get_tbscan() ;
	}

	if ( setMSG )
	{
		if ( !ControlNonDispl && p_name != "" )
		{
			currtbPanel->set_panel_msg( MSG1, MSGID1 ) ;
			setMSG = false ;
		}
	}
	else if ( p_msg != "" )
	{
		get_Message( p_msg ) ;
		if ( RC > 0 ) { RC = 12 ; return ; }
		currtbPanel->set_panel_msg( MSG, MSGID ) ;
	}
	else
	{
		currtbPanel->clear_msg() ;
	}

	if ( p_cursor == "" ) { p_cursor = currtbPanel->Home ; }
	if ( addpop_active )  { currtbPanel->set_popup( addpop_row, addpop_col ) ; }
	else                  { currtbPanel->remove_popup()                      ; }

	currtbPanel->display_panel_init( errBlock ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e2 + p_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	currPanel->update_field_values( errBlock ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e6 + p_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	t = funcPOOL.get( errBlock, 8, ".AUTOSEL", NOCHECK ) ;
	if ( errBlock.RC0() ) { p_autosel = t ; }
	t = funcPOOL.get( errBlock, 8, ".CSRROW", NOCHECK ) ;
	if ( errBlock.RC0() ) { p_csrrow = ds2d( t ) ; }

	if ( p_poolMGR->get( errBlock, "ZSCRNAM1", SHARED ) == "ON" &&
	     p_poolMGR->get( errBlock, lscreen_num, "ZSCRNAM2" ) == "PERM" )
	{
			p_poolMGR->put( errBlock, "ZSCRNAME", p_poolMGR->get( errBlock, lscreen_num, "ZSCRNAME" ), SHARED ) ;
	}

	while ( true )
	{
		if ( p_name != "" )
		{
			currtbPanel->display_panel( errBlock ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e1 ) ;
				checkRCode( errBlock ) ;
				exitRC = 20 ;
				break ;
			}
			if ( currtbPanel->CURFLD == "" )
			{
				if ( wordpos( p_cursor, currtbPanel->tb_fields ) > 0 )
				{
					csrvrow = p_csrrow - ZTDTOP + 1 ;
					if ( csrvrow > 0 && csrvrow <= ZTDVROWS ) { i = csrvrow - 1 ; }
					else if ( ln > 0 ) { i = ln ; }
					else               { i = 1  ; }
					currtbPanel->CURFLD = p_cursor + "." + d2ds( i ) ;
				}
				else { currtbPanel->CURFLD = p_cursor ; }
				currtbPanel->CURPOS = p_csrpos ;
			}
			currtbPanel->cursor_to_field( RC ) ;
			if ( RC > 0 )
			{
				errBlock.setcall( e1, "PSYE022N", currtbPanel->CURFLD ) ;
				checkRCode( errBlock ) ;
				exitRC = 20 ;
				break ;
			}
			wait_event() ;
			ControlDisplayLock = false ;
			ControlNonDispl    = false ;
			currtbPanel->clear_msg() ;
			currtbPanel->CURFLD = "" ;
			currtbPanel->display_panel_update( errBlock ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e5 ) ;
				checkRCode( errBlock ) ;
				exitRC = 20 ;
				break ;
			}
			currtbPanel->tb_set_linesChanged() ;
		}

		exitRC = 0  ;
		if ( currtbPanel->tb_lineChanged( ln, URID ) )
		{
			tbskip( tb_name, 0, "", p_rowid_nm, URID, "", p_crp_name ) ;
			ws = words( currtbPanel->tb_fields ) ;
			for ( i = 1 ; i <= ws ; i++ )
			{
				s = word( currtbPanel->tb_fields, i ) ;
				funcPOOL.put( errBlock, s, funcPOOL.get( errBlock, 0, s + "." + d2ds( ln ), NOCHECK ) ) ;
				if ( errBlock.error() )
				{
					checkRCode( errBlock ) ;
					exitRC = 20 ;
					break ;
				}
			}
			if ( ZTDSELS > 1 ) { exitRC = 4; }
		}

		currtbPanel->display_panel_proc( errBlock, ln ) ;
		if ( errBlock.error() )
		{
			errBlock.setcall( e4 + p_name ) ;
			checkRCode( errBlock ) ;
			return ;
		}

		ZZVERB = p_poolMGR->get( errBlock, "ZVERB" ) ;
		if ( !errBlock.RC0() )
		{
			errBlock.setcall( e1, "PSYE015L", "GET", "ZVERB" ) ;
			checkRCode( errBlock ) ;
			exitRC = 20 ;
			break ;
		}
		if ( ZZVERB == "RETURN" ) { propagateEnd = true ; }
		if ( findword( ZZVERB, "END EXIT RETURN" ) ) { RC = 8 ; return ; }

		t = funcPOOL.get( errBlock, 8, ".CSRROW", NOCHECK ) ;
		if ( errBlock.RC0() ) { p_csrrow = ds2d( t ) ; }
		if ( currtbPanel->MSGID != "" )
		{
			get_Message( currtbPanel->MSGID ) ;
			if ( RC > 0 ) { RC = 12 ; return ; }
			currtbPanel->set_panel_msg( MSG, MSGID ) ;
			if ( p_name == "" )
			{
				p_name    = currtbPanel->PANELID ;
				currPanel = currtbPanel ;
				PANELID   = p_name      ;
				p_poolMGR->put( errBlock, "ZPANELID", p_name, SHARED, SYSTEM ) ;
			}
			currtbPanel->display_panel_reinit( errBlock, ln ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e3 + p_name ) ;
				checkRCode( errBlock ) ;
				exitRC = 20 ;
				break ;
			}
			t = funcPOOL.get( errBlock, 8, ".AUTOSEL", NOCHECK ) ;
			if ( errBlock.RC0() ) { p_autosel = t ; }
			t = funcPOOL.get( errBlock, 8, ".CSRROW", NOCHECK ) ;
			if ( errBlock.RC0() ) { p_csrrow = ds2d( t ) ; }
			continue ;
		}
		if ( ZZVERB == "UP" || ZZVERB == "DOWN" )
		{
			if ( ZZVERB == "UP" )
			{
				ZSCROLLA = p_poolMGR->get( errBlock, "ZSCROLLA", SHARED ) ;
				if ( ZSCROLLA != "MAX" )
				{
					ZSCROLLN = ds2d( p_poolMGR->get( errBlock, "ZSCROLLN", SHARED ) ) ;
					ZTDTOP > ZSCROLLN ? ( ZTDTOP = ZTDTOP - ZSCROLLN ) : ZTDTOP = 1 ;
				}
				else { ZTDTOP = 1 ; }
				ZCMD = "" ;
			}
			else
			{
				ZSCROLLA = p_poolMGR->get( errBlock, "ZSCROLLA", SHARED ) ;
				if ( ZSCROLLA == "MAX" )
				{
					ZTDTOP = ZTDROWS + 1 ;
				}
				else
				{
					ZSCROLLN = ds2d( p_poolMGR->get( errBlock, "ZSCROLLN", SHARED ) ) ;
					ZSCROLLN + ZTDTOP > ZTDROWS ? ( ZTDTOP = ZTDROWS+1 ) : ZTDTOP = ZTDTOP + ZSCROLLN ;
				}
				ZCMD = "" ;
			}
			p_tableMGR->fillfVARs( errBlock, funcPOOL, tb_name, currtbPanel->get_tb_clear(), scan, currtbPanel->tb_depth, ZTDTOP ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e6 + p_name ) ;
				checkRCode( errBlock ) ;
				return ;
			}
			currPanel->update_field_values( errBlock ) ;
			if ( errBlock.error() )
			{
				errBlock.setcall( e6 + p_name ) ;
				checkRCode( errBlock ) ;
				return ;
			}
			continue ;
		}
		break ;
	}
	if ( URID == "" )
	{
		csrvrow = p_csrrow - ZTDTOP + 1 ;
		if ( p_autosel == "YES" && csrvrow > 0 && csrvrow <= ZTDVROWS )
		{
			tbtop( tb_name ) ;
			tbskip( tb_name, p_csrrow, "", p_rowid_nm, "", "", p_crp_name ) ;
			ws = words( currtbPanel->tb_fields ) ;
			for ( i = 1 ; i <= ws ; i++ )
			{
				s = word( currtbPanel->tb_fields, i ) ;
				funcPOOL.put( errBlock, s, funcPOOL.get( errBlock, 0, s + "." + d2ds( csrvrow-1 ), NOCHECK ) ) ;
			}
			ZTDSELS++ ;
		}
		else
		{
			tbtop( tb_name ) ;
			if ( p_crp_name != "" ) { funcPOOL.put( errBlock, p_crp_name, 0  ) ; }
			if ( p_rowid_nm != "" ) { funcPOOL.put( errBlock, p_rowid_nm, "" ) ; }
		}
	}
	currtbPanel->resetAttrs() ;
	RC = exitRC ;
}


void pApplication::tbend( const string& tb_name )
{
	// Close a table without saving (calls destroyTable routine).
	// If opened share, use count is reduced and table deleted when use count = 0.

	// RC = 0   Normal completion
	// RC = 12  Table not open
	// RC = 20  Severe error

	if ( !isTableOpen( tb_name, "TBEND" ) ) { return ; }

	p_tableMGR->destroyTable( errBlock, taskid(), tb_name ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBEND error" ) ;
		checkRCode( errBlock ) ;
	}
	else
	{
		tablesOpen.erase( tb_name ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tberase( const string& tb_name, string tb_path )
{
	// Erase a table file from path

	// RC = 0   Normal completion
	// RC = 8   Table does not exist
	// RC = 12  Table in use
	// RC = 16  Path does not exist
	// RC = 20  Severe error

	const string e1 = "TBERASE error" ;

	if ( !isvalidName( tb_name ) )
	{
		errBlock.setcall( e1, "PSYE022J", "table", tb_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( tb_path == "" ) { tb_path = ZTLIB ; }
	p_tableMGR->tberase( errBlock, tb_name, tb_path ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbexist( const string& tb_name )
{
	// Test for the existance of a row in a keyed table

	// RC = 0   Normal completion
	// RC = 8   Row does not exist or not a keyed table
	// RC = 12  Table not open
	// RC = 20  Severe error

	if ( !isTableOpen( tb_name, "TBEXIST" ) ) { return ; }

	p_tableMGR->tbexist( errBlock, funcPOOL, tb_name ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBEXISTS error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}



void pApplication::tbget( const string& tb_name, const string& tb_savenm, const string& tb_rowid_vn, const string& tb_noread, const string& tb_crp_name )
{
	if ( !isTableOpen( tb_name, "TBGET" ) ) { return ; }

	p_tableMGR->tbget( errBlock, funcPOOL, tb_name, tb_savenm, tb_rowid_vn, tb_noread, tb_crp_name  ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBGET error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbmod( const string& tb_name, const string& tb_namelst, const string& tb_order )
{
	// Update a row in a table

	const string e1 = "TBMOD error" ;

	if ( tb_order != "" && tb_order != "ORDER" )
	{
		errBlock.setcall( e1, "PSYE023A", "TBMOD", tb_order ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( !isTableOpen( tb_name, "TBMOD" ) ) { return ; }

	p_tableMGR->tbmod( errBlock, funcPOOL, tb_name, tb_namelst, tb_order ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbopen( const string& tb_name, tbWRITE m_WRITE, string m_paths, tbDISP m_DISP )
{
	// Open an existing table, reading it from a file.  If aleady opened in SHARE/NOWRITE, increment use count

	// RC = 0   Normal completion
	// RC = 8   Table does not exist in search path
	// RC = 12  Table already open by this or another task
	// RC = 16  path does not exist
	// RC = 20  Severe error

	const string e1 = "TBOPEN error" ;

	if ( !isvalidName( tb_name ) )
	{
		errBlock.setcall( e1, "PSYE022J", "table", tb_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( tablesOpen.count( tb_name ) > 0 )
	{
		errBlock.setcall( e1, "PSYE022K", "table", tb_name, 12 ) ;
		checkRCode( errBlock ) ;
		return  ;
	}

	if ( m_paths == "" )
	{
		m_paths = get_search_path( s_ZTLIB ) ;
	}

	p_tableMGR->loadTable( errBlock, taskid(), tb_name, m_WRITE, m_paths, m_DISP ) ;

	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	else if ( errBlock.RC0() )
	{
		tablesOpen[ tb_name ] = true ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbput( const string& tb_name, const string& tb_namelst, const string& tb_order )
{
	// Update the current row in a table

	// RC = 0   Normal completion
	// RC = 8   Keyed tables - key does not match current row.  CRP set to top (0)
	//          Non-keyed tables - CRP at top
	// RC = 12  Table not open
	// RC = 16  Numeric conversion error for sorted tables
	// RC = 20  Severe error

	const string e1 = "TBPUT error" ;

	RC = 0 ;

	if ( tb_order != "" && tb_order != "ORDER" )
	{
		errBlock.setcall( e1, "PSYE023A", "TBPUT", tb_order ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( !isTableOpen( tb_name, "TBPUT" ) ) { return ; }

	p_tableMGR->tbput( errBlock, funcPOOL, tb_name, tb_namelst, tb_order ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbquery( const string& tb_name, const string& tb_keyn, const string& tb_varn, const string& tb_rownn, const string& tb_keynn, const string& tb_namenn, const string& tb_crpn, const string& tb_sirn, const string& tb_lstn, const string& tb_condn, const string& tb_dirn )
{
	if ( !isTableOpen( tb_name, "TBQUERY" ) ) { return ; }

	p_tableMGR->tbquery( errBlock, funcPOOL, tb_name, tb_keyn, tb_varn, tb_rownn, tb_keynn, tb_namenn, tb_crpn, tb_sirn, tb_lstn, tb_condn, tb_dirn ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBQUERY error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbsarg( const string& tb_name, const string& tb_namelst, const string& tb_dir, const string& tb_cond_pairs )
{
	if ( !isTableOpen( tb_name, "TBSARG" ) ) { return ; }

	p_tableMGR->tbsarg( errBlock, funcPOOL, tb_name, tb_namelst, tb_dir, tb_cond_pairs ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBSARG error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbsave( const string& tb_name, const string& tb_newname, const string& path )
{
	// Save the table to disk (calls saveTable routine).  Table remains open for processing.
	// Table must be open in WRITE mode.

	// RC = 0   Normal completion
	// RC = 12  Table not open or not open WRITE
	// RC = 16  Alternate name save error
	// RC = 20  Severe error

	RC = 0 ;

	const string e1 = "TBSAVE error" ;

	if ( !isTableOpen( tb_name, "TBSAVE" ) ) { return ; }

	if ( !p_tableMGR->writeableTable( errBlock, tb_name ) )
	{
		errBlock.setcall( e1 ) ;
		if ( !errBlock.error() )
		{
			errBlock.seterrid( "PSYE014S", tb_name, 12 ) ;
		}
		checkRCode( errBlock ) ;
		return ;
	}

	p_tableMGR->saveTable( errBlock, taskid(), tb_name, tb_newname, path ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbscan( const string& tb_name, const string& tb_namelst, const string& tb_savenm, const string& tb_rowid_vn, const string& tb_dir, const string& tb_read, const string& tb_crp_name, const string& tb_condlst )
{
	if ( !isTableOpen( tb_name, "TBSCAN" ) ) { return ; }

	p_tableMGR->tbscan( errBlock, funcPOOL, tb_name, tb_namelst, tb_savenm, tb_rowid_vn, tb_dir, tb_read, tb_crp_name, tb_condlst ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBSCAN error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}



void pApplication::tbskip( const string& tb_name, int num, const string& tb_savenm, const string& tb_rowid_vn, const string& tb_rowid, const string& tb_noread, const string& tb_crp_name )
{
	if ( !isTableOpen( tb_name, "TBSKIP" ) ) { return ; }

	p_tableMGR->tbskip( errBlock, funcPOOL, tb_name, num, tb_savenm, tb_rowid_vn, tb_rowid, tb_noread, tb_crp_name ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBSKIP error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbsort( const string& tb_name, const string& tb_fields )
{
	if ( !isTableOpen( tb_name, "TBSORT" ) ) { return ; }

	p_tableMGR->tbsort( errBlock, tb_name, tb_fields ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBSORT error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}



void pApplication::tbtop( const string& tb_name )
{
	if ( !isTableOpen( tb_name, "TBTOP" ) ) { return ; }

	p_tableMGR->tbtop( errBlock, tb_name ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBTOP error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


void pApplication::tbvclear( const string& tb_name )
{
	if ( !isTableOpen( tb_name, "TBVCLEAR" ) ) { return ; }

	p_tableMGR->tbvclear( errBlock, funcPOOL, tb_name ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "TBVCLEAR error" ) ;
		checkRCode( errBlock ) ;
	}
	RC = errBlock.getRC() ;
}


bool pApplication::isTableOpen( const string& tb_name, const string& func )
{
	RC = 0 ;

	if ( !isvalidName( tb_name ) )
	{
		errBlock.seterrid( "PSYE014Q", tb_name, func ) ;
	}
	else if ( tablesOpen.count( tb_name ) == 0 )
	{
		errBlock.seterrid( "PSYE013G", func, tb_name, 12 ) ;
	}
	if ( errBlock.error() )
	{
		errBlock.setcall( func + " error" ) ;
		checkRCode( errBlock ) ;
		return false ;
	}
	return true ;
}


void pApplication::edit( const string& m_file, const string& m_panel, const string& m_macro, const string& m_profile )
{
	SELCT.clear() ;
	SELCT.PGM     = p_poolMGR->get( errBlock, "ZEDITPGM", PROFILE ) ;
	SELCT.PARM    = "FILE("+ m_file +") PANEL("+ m_panel +") MACRO("+ m_macro +") PROFILE("+ m_profile+ ")" ;
	SELCT.NEWAPPL = ""      ;
	SELCT.NEWPOOL = false   ;
	SELCT.PASSLIB = false   ;
	SELCT.SUSPEND = true    ;
	SELCT.SCRNAME = "EDIT"  ;
	actionSelect() ;
}


void pApplication::browse( const string& m_file, const string& m_panel )
{
	SELCT.clear() ;
	SELCT.PGM     = p_poolMGR->get( errBlock, "ZBRPGM", PROFILE ) ;
	SELCT.PARM    = "FILE("+ m_file +") PANEL("+ m_panel +")" ;
	SELCT.NEWAPPL = ""       ;
	SELCT.NEWPOOL = false    ;
	SELCT.PASSLIB = false    ;
	SELCT.SUSPEND = true     ;
	SELCT.SCRNAME = "BROWSE" ;
	actionSelect() ;
}


void pApplication::view( const string& m_file, const string& m_panel )
{
	SELCT.clear() ;
	SELCT.PGM     = p_poolMGR->get( errBlock, "ZVIEWPGM", PROFILE ) ;
	SELCT.PARM    = "FILE("+ m_file +") PANEL("+ m_panel +")" ;
	SELCT.NEWAPPL = ""      ;
	SELCT.NEWPOOL = false   ;
	SELCT.PASSLIB = false   ;
	SELCT.SUSPEND = true    ;
	SELCT.SCRNAME = "VIEW"  ;
	actionSelect() ;
}


void pApplication::select( const string& cmd )
{
	// SELECT a function or panel in keyword format for use in applications,
	// ie PGM(abc) CMD(oorexx) PANEL(xxx) PARM(zzz) NEWAPPL PASSLIB SCRNAME(abc) etc.

	// No variable substitution is done at this level.

	if ( !SELCT.parse( errBlock, cmd ) )
	{
		errBlock.setcall( "Error in SELECT command "+cmd ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	actionSelect() ;
}


void pApplication::select( selobj sel )
{
	// SELECT a function or panel using a SELECT object (internal use only)

	SELCT = sel    ;
	actionSelect() ;
}


void pApplication::actionSelect()
{
	// RC =  0  Normal completion of the selection panel or function.  END was entered.
	// RC =  4  Normal completion.  RETURN was entered or EXIT specified on the selection panel

	RC       = 0     ;
	SEL      = true  ;
	busyAppl = false ;

	wait_event() ;

	if ( RC == 4 ) { propagateEnd = true ; }

	SEL = false   ;
	SELCT.clear() ;
}


void pApplication::pquery( const string& p_name, const string& a_name, const string& t_name, const string& w_name, const string& d_name, const string& r_name, const string& c_name )
{
	const string e1 = "PQUERY error for panel "+p_name ;

	RC = 0 ;

	if ( !isvalidName( p_name ) )
	{
		errBlock.setcall( e1, "PSYE021A", p_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( a_name != "" && !isvalidName( a_name ) )
	{
		errBlock.setcall( e1, "PSYE023C", "area", a_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( t_name != "" && !isvalidName( t_name ) )
	{
		errBlock.setcall( e1, "PSYE023C", "area type", t_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( w_name != "" && !isvalidName( w_name ) )
	{
		errBlock.setcall( e1, "PSYE023C", "area width", w_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( d_name != "" && !isvalidName( d_name ) )
	{
		errBlock.setcall( e1, "PSYE023C", "area depth", d_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( r_name != "" && !isvalidName( r_name ) )
	{
		errBlock.setcall( e1, "PSYE023C", "area row number", r_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( c_name != "" && !isvalidName( c_name ) )
	{
		errBlock.setcall( e1, "PSYE023C", "area column number", c_name ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	createPanel( p_name ) ;
	if ( panelList.count( p_name ) == 0 )
	{
		errBlock.setcall( e1 ) ;
		checkRCode( errBlock ) ;
		return  ;
	}
	panelList[ p_name ]->get_panel_info( RC, a_name, t_name, w_name, d_name, r_name, c_name ) ;
}


void pApplication::attr( const string& field, const string& attrs )
{
	if ( PANELID == "" )
	{
		errBlock.setcall( "ATTR change error", "PSYE023D" ) ;
		checkRCode( errBlock ) ;
		return  ;
	}
	panelList[ PANELID ]->attr( RC, field, attrs ) ;
}


void pApplication::reload_keylist( pPanel * p )
{
	// Does an unconditional reload every time, but need to find a way to detect a change (TODO)
	// Alternatively, don't preload pfkeys into the panel object but pass back requested key from pApplication.

	load_keylist( p ) ;
}


void pApplication::load_keylist( pPanel * p )
{
	string tabName  ;
	string tabField ;

	string UPROF    ;

	bool   klfail   ;

	if ( p->KEYLISTN == "" ) { return ; }

	tabName = p->KEYAPPL + "KEYP" ;
	klfail  = ( p_poolMGR->get( errBlock, "ZKLFAIL", PROFILE ) == "Y" ) ;

	vcopy( "ZUPROF", UPROF, MOVE ) ;
	tbopen( tabName, NOWRITE, UPROF, SHARE ) ;
	if ( RC > 0 )
	{
		if ( !klfail )
		{
			RC = 0 ;
			llog( "W", "Open of keylist table '"+ tabName +"' failed" << endl ) ;
			return ;
		}
		errBlock.setcall( "KEYLIST error", "PSYE023E", tabName ) ;
		checkRCode( errBlock ) ;
	}

	tbvclear( tabName ) ;
	vreplace( "KEYLISTN", p->KEYLISTN ) ;
	tbget( tabName ) ;
	if ( RC > 0 )
	{
		tbend( tabName ) ;
		if ( !klfail )
		{
			RC = 0 ;
			llog( "W", "Keylist '"+ p->KEYLISTN +"' not found in keylist table "+ tabName << endl ) ;
			return ;
		}
		errBlock.setcall( "KEYLIST error", "PSYE023F", p->KEYLISTN, tabName ) ;
		checkRCode( errBlock ) ;
		return  ;
	}

	vcopy( "KEY1DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(1),  tabField ) ;
	vcopy( "KEY2DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(2),  tabField ) ;
	vcopy( "KEY3DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(3),  tabField ) ;
	vcopy( "KEY4DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(4),  tabField ) ;
	vcopy( "KEY5DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(5),  tabField ) ;
	vcopy( "KEY6DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(6),  tabField ) ;
	vcopy( "KEY7DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(7),  tabField ) ;
	vcopy( "KEY8DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(8),  tabField ) ;
	vcopy( "KEY9DEF",  tabField, MOVE ) ; p->put_keylist( KEY_F(9),  tabField ) ;
	vcopy( "KEY10DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(10), tabField ) ;
	vcopy( "KEY11DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(11), tabField ) ;
	vcopy( "KEY12DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(12), tabField ) ;
	vcopy( "KEY13DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(13), tabField ) ;
	vcopy( "KEY14DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(14), tabField ) ;
	vcopy( "KEY15DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(15), tabField ) ;
	vcopy( "KEY16DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(16), tabField ) ;
	vcopy( "KEY17DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(17), tabField ) ;
	vcopy( "KEY18DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(18), tabField ) ;
	vcopy( "KEY19DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(19), tabField ) ;
	vcopy( "KEY20DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(20), tabField ) ;
	vcopy( "KEY21DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(21), tabField ) ;
	vcopy( "KEY22DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(22), tabField ) ;
	vcopy( "KEY23DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(23), tabField ) ;
	vcopy( "KEY24DEF", tabField, MOVE ) ; p->put_keylist( KEY_F(24), tabField ) ;
	vcopy( "KEYHELPN", p->KEYHELPN, MOVE ) ;

	tbend( tabName ) ;
}


void pApplication::rdisplay( const string& msg, bool subVars )
{
	int i  ;
	int j  ;
	int l  ;
	int m  ;

	string t ;

	RC = 0 ;
	l  = ds2d( p_poolMGR->get( errBlock, "ZSCRMAXW", SHARED ) ) ;
	m  = ds2d( p_poolMGR->get( errBlock, "ZSCRMAXD", SHARED ) ) - 1 ;

	if ( subVars ) { t = sub_vars( msg ) ; }
	else           { t = msg             ; }

	while ( true )
	{
		if ( t.size() > l )
		{
			i = l ;
			j = t.find_last_of( ' ', i ) ;
			if ( j != string::npos ) { i = j + 1 ; }
		}
		else
		{
			i = t.size() ;
		}
		rmsgs.push_back( t.substr( 0, i ) ) ;
		t.erase( 0, i ) ;
		if ( rmsgs.size() == m )
		{
			rawOutput = true  ;
			wait_event()      ;
			rawOutput = false ;
		}
		if ( t.size() == 0 ) { break ; }
	}
}


void pApplication::setmsg( const string& msg, msgSET sType )
{
	// Retrieve message and store in MSG1 and MSGID1.

	RC = 0 ;

	if ( ( sType == COND ) && setMSG ) { return ; }

	get_Message( msg ) ;
	if ( RC > 0 ) { RC = 20 ; return ; }
	MSG1   = MSG   ;
	MSGID1 = msg   ;
	setMSG = true  ;
}


void pApplication::getmsg( const string& msg, const string& smsg, const string& lmsg, const string& alm, const string& hlp, const string& typ, const string& wndo )
{
	// Load message msg and substitute variables

	// RC = 0   Normal completion
	// RC = 12  Message not found or message syntax error
	// RC = 20  Severe error

	const string e1 = "GETMSG error" ;

	slmsg tmsg ;

	if ( smsg != "" && !isvalidName( smsg ) )
	{
		errBlock.setcall( e1, "PSYE022O", "SHORT MESSAGE", smsg ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( lmsg != "" && !isvalidName( lmsg ) )
	{
		errBlock.setcall( e1, "PSYE022O", "LONG MESSAGE", lmsg ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( alm != "" && !isvalidName( alm ) )
	{
		errBlock.setcall( e1, "PSYE022O", "ALARM", alm ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( hlp != "" && !isvalidName( hlp ) )
	{
		errBlock.setcall( e1, "PSYE022O", "HELP", hlp ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( typ != "" && !isvalidName( typ ) )
	{
		errBlock.setcall( e1, "PSYE022O", "TYPE", typ ) ;
		checkRCode( errBlock ) ;
		return ;
	}
	if ( wndo != "" && !isvalidName( wndo ) )
	{
		errBlock.setcall( e1, "PSYE022O", "WINDOW", wndo ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( !load_Message( msg ) ) { return ; }

	tmsg      = msgList[ msg ] ;
	tmsg.smsg = sub_vars( tmsg.smsg ) ;
	tmsg.lmsg = sub_vars( tmsg.lmsg ) ;

	if ( !sub_Message_vars( tmsg ) )
	{
		errBlock.seterror( "Invalid variable value" ) ;
		checkRCode( errBlock ) ;
		return ;
	}

	if ( smsg != "" ) { funcPOOL.put( errBlock, smsg, tmsg.smsg ) ; }
	if ( lmsg != "" ) { funcPOOL.put( errBlock, lmsg, tmsg.lmsg ) ; }
	if (  alm != "" )
	{
		funcPOOL.put( errBlock, alm, tmsg.alm ? "YES" : "NO" ) ;
	}
	if (  typ != "" )
	{
		switch ( tmsg.type )
		{
			case IMT: funcPOOL.put( errBlock, typ, "NOTIFY" )   ;
				  break ;
			case WMT: funcPOOL.put( errBlock, typ, "WARNING" )  ;
				  break ;
			case AMT: funcPOOL.put( errBlock, typ, "CRITICAL" ) ;
		}
	}
	if ( hlp  != "" ) { funcPOOL.put( errBlock, hlp, tmsg.hlp ) ; }
	if ( wndo != "" )
	{
		funcPOOL.put( errBlock, wndo, tmsg.resp ? "RESP" : "NORESP" ) ;
	}
	RC = errBlock.getRC() ;
}


string pApplication::get_help_member( int row, int col )
{
	string fld ;
	string paths ;

	RC = 0 ;

	fld = currPanel->field_getname( row, col ) ;

	paths = get_search_path( s_ZPLIB ) ;

	return "M("+ MSG.hlp+ ") " +
	       "F("+ currPanel->get_field_help( fld )+ ") " +
	       "P("+ currPanel->ZPHELP +") " +
	       "A("+ ZAHELP +") " +
	       "K("+ currPanel->KEYHELPN +") "+
	       "PATHS("+paths+")" ;
}


void pApplication::get_Message( const string& p_msg )
{
	// Load messages from message library and copy slmsg object to MSG for the message requested

	// Substitute any dialogue variables in the short and long messages
	// Substitute any dialogue varibles in .TYPE, .WINDOW, .HELP and .ALARM parameters
	// Set MSGID

	RC = 0 ;

	if ( !load_Message( p_msg ) ) { return ; }

	MSG      = msgList[ p_msg ]     ;
	MSG.smsg = sub_vars( MSG.smsg ) ;
	MSG.lmsg = sub_vars( MSG.lmsg ) ;

	if ( !sub_Message_vars( MSG ) ) { RC = 20 ; return ; }

	MSGID    = p_msg ;
}


bool pApplication::load_Message( const string& p_msg )
{
	// Message format: 1-5 alph char prefix
	//                 3 numeric chars
	//                 1 alph char suffix (optional and only if prefix is less than 5)

	// Read messages and store in msgList map (no variable substitution done at this point)
	// Return false if message not found in member or there is an error (but still store individual messages from member)
	// Error on duplicate message-id in the file member

	// The message file name is determined by truncating the message ID after the second digit of the number.
	// AB123A file AB12
	// G012 file G01

	// If in test mode, reload message each time it is requested
	// Routine sets the return code:
	// RC =  0  Normal completion
	// RC = 12  Message not found or syntax error
	// RC = 20  Severe error

	int i  ;
	int j  ;
	char c ;

	string p_msg_fn ;
	string filename ;
	string mline    ;
	string paths    ;
	string tmp      ;
	string msgid    ;

	bool lcontinue  ;

	map<string,bool>MMsgs ;

	slmsg t ;

	RC = 0  ;

	if ( !testMode && msgList.count( p_msg ) > 0 ) { return true ; }

	i = chk_Message_id( p_msg ) ;

	if ( i == 0 || ((p_msg.size() - i) > 3 && !isalpha( p_msg.back()) ) )
	{
		RC = 12 ;
		checkRCode( "Message-id format invalid (1): "+ p_msg ) ;
		return false ;
	}

	p_msg_fn = p_msg.substr( 0, i+2 ) ;

	paths = get_search_path( s_ZMLIB ) ;

	for ( i = getpaths( paths ), j = 1 ; j <= i ; j++ )
	{
		filename = getpath( paths, j ) + p_msg_fn ;
		if ( exists( filename ) )
		{
			if ( !is_regular_file( filename ) )
			{
				RC = 20 ;
				checkRCode( "Message file "+ filename +" is not a regular file" ) ;
				return false ;
			}
			else
			{
				break ;
			}
		}
	}
	if ( j > i )
	{
		RC = 12 ;
		checkRCode( "Message file "+ p_msg_fn +" not found in ZMLIB for message-id "+ p_msg ) ;
		return false ;
	}

	msgid  = ""    ;
	t.smsg = ""    ;
	t.lmsg = ""    ;
	t.cont = false ;

	std::ifstream messages( filename.c_str() ) ;
	if ( !messages.is_open() )
	{
		RC = 20 ;
		checkRCode( "Error opening message file "+ filename ) ;
		return false ;
	}
	while ( getline( messages, mline ) )
	{
		trim( mline ) ;
		if ( mline == "" || mline[ 0 ] == '*' ) { continue ; }
		if ( mline.compare( 0, 2, "/*" ) == 0 ) { continue ; }
		if ( mline.compare( 0, p_msg_fn.size(), p_msg_fn ) == 0 )
		{
			if ( msgid != "" )
			{
				if ( t.cont || !parse_Message( t ) )
				{
					RC = 12 ;
					messages.close() ;
					checkRCode( "Error (1) in message-id "+ msgid ) ;
					return false ;
				}
				if ( MMsgs.count( msgid ) > 0 )
				{
					RC = 20 ;
					messages.close() ;
					checkRCode( "Duplicate message-id found: "+ msgid ) ;
					return false ;
				}
				trim_right( t.smsg )    ;
				trim_right( t.lmsg )    ;
				msgList[ msgid ] = t    ;
				MMsgs[ msgid ]   = true ;
			}
			msgid  = word( mline, 1 )    ;
			t.smsg = subword( mline, 2 ) ;
			t.lmsg = "" ;
			i = chk_Message_id( msgid ) ;
			if ( i == 0 || ((msgid.size() - i) > 3 && !isalpha( msgid.back()) ) )
			{
				RC = 12 ;
				checkRCode( "Message-id format invalid (2): "+ msgid ) ;
				return false ;
			}
		}
		else
		{
			if ( msgid == "" || ( t.lmsg != "" && !t.cont ) )
			{
				RC = 12 ;
				messages.close() ;
				checkRCode( "Extraeneous data: "+ mline ) ;
				return false ;
			}
			lcontinue = false ;
			if ( mline.back() == '+' )
			{
				mline.pop_back() ;
				trim( mline )    ;
				lcontinue = true ;
			}
			c = mline.front() ;
			if ( c == '\'' || c == '"' )
			{
				if ( mline.back() != c )
				{
					RC = 12 ;
					messages.close() ;
					checkRCode( "Error (2) in message-id "+ msgid ) ;
					return false ;
				}
				tmp = mline.substr( 1, mline.size() - 2 ) ;
				dquote( c, tmp ) ;
			}
			else
			{
				if ( words( mline ) > 1 )
				{
					RC = 12 ;
					messages.close() ;
					checkRCode( "Error (3) in message-id "+ msgid ) ;
					return false ;
				}
				tmp = mline ;
			}
			t.cont ? t.lmsg = t.lmsg + " " + tmp : t.lmsg = tmp ;
			if ( t.lmsg.size() > 512 )
			{
				RC = 12 ;
				messages.close() ;
				checkRCode( "Long message size exceeds 512 bytes for message-id "+ msgid ) ;
				return false ;
			}
			t.cont = lcontinue ;
		}
	}
	if ( messages.bad() )
	{
		RC = 20 ;
		messages.close() ;
		checkRCode( "Error while reading message file "+ filename ) ;
		return false ;
	}
	messages.close() ;

	if ( t.smsg != "" )
	{
		if ( t.cont || !parse_Message( t ) )
		{
			RC = 12 ;
			messages.close() ;
			checkRCode( "Error (4) in message-id "+ msgid ) ;
			return false ;
		}
		if ( MMsgs.count( msgid ) > 0 )
		{
			RC = 20 ;
			messages.close() ;
			checkRCode( "Duplicate message-id found: "+ msgid ) ;
			return false ;
		}
		trim_right( t.smsg ) ;
		trim_right( t.lmsg ) ;
		msgList[ msgid ] = t ;
	}

	if ( msgList.count( p_msg ) == 0 )
	{
		RC = 12 ;
		checkRCode( "Message-id "+ p_msg +" not found in message file "+ p_msg_fn ) ;
		return false ;
	}
	return true ;
}


bool pApplication::parse_Message( slmsg& t )
{
	// Parse message and fill the slmsg object.  Long message already processed at this stage.

	// .TYPE overrides .WINDOW and .ALARM

	int p1 ;
	int p2 ;
	int ln ;

	char c ;

	string rest ;
	string tmp  ;

	t.hlp   = ""    ;
	t.type  = IMT   ;
	t.smwin = false ;
	t.lmwin = false ;
	t.resp  = false ;
	t.alm   = false ;

	c = t.smsg.front() ;
	if ( c == '\'' || c == '"' )
	{
		p1 = t.smsg.find_last_of( c ) ;
		if ( p1 == string::npos ) { return false ; }
		rest   = substr( t.smsg, p1+2 )   ;
		t.smsg = t.smsg.substr( 1, p1-1 ) ;
		dquote( c, t.smsg ) ;
	}
	else
	{
		if ( t.smsg[ 0 ] == '.' )
		{
			rest   = t.smsg ;
			t.smsg = ""     ;
		}
		else
		{
			rest   = subword( t.smsg, 2 ) ;
			t.smsg = word( t.smsg, 1 )    ;
		}
	}

	p1 = pos( ".HELP=", rest ) ;
	if ( p1 == 0 )
	{
		p1 = pos( ".H=", rest ) ;
		ln = 3 ;
	}
	else
	{
		ln = 6 ;
	}
	if ( p1 > 0 )
	{
		p2 = pos( " ", rest, p1 ) ;
		if ( p2 == 0 ) { t.hlp = substr( rest, p1+ln )           ; rest = delstr( rest, p1 )        ; }
		else           { t.hlp = substr( rest, p1+ln, p2-p1-ln ) ; rest = delstr( rest, p1, p2-p1 ) ; }
		if ( t.hlp.size() == 0 ) { return false ; }
		if ( t.hlp[ 0 ] == '&')
		{
			t.hlp.erase( 0, 1 ) ;
			if ( !isvalidName( t.hlp ) ) { return false ; }
			t.dvhlp = t.hlp ;
		}
	}

	p1 = pos( ".WINDOW=", rest ) ;
	if ( p1 == 0 )
	{
		p1 = pos( ".W=", rest ) ;
		ln = 3 ;
	}
	else
	{
		ln = 8 ;
	}
	if ( p1 > 0 )
	{
		t.lmwin = true ;
		p2 = pos( " ", rest, p1 ) ;
		if ( p2 == 0 ) { tmp = substr( rest, p1+ln )           ; rest = delstr( rest, p1 )        ; }
		else           { tmp = substr( rest, p1+ln, p2-p1-ln ) ; rest = delstr( rest, p1, p2-p1 ) ; }
		if ( tmp.size() == 0 ) { return false ; }
		if ( tmp[ 0 ] == '&')
		{
			tmp.erase( 0, 1 ) ;
			if ( !isvalidName( tmp ) ) { return false ; }
			t.dvwin = tmp ;
		}
		else if ( tmp == "RESP"    || tmp == "R"  ) { t.smwin = true  ; t.resp = true ; }
		else if ( tmp == "NORESP"  || tmp == "N"  ) { t.smwin = true  ; }
		else if ( tmp == "LRESP"   || tmp == "LR" ) { t.resp  = true  ; }
		else if ( tmp == "LNORESP" || tmp == "LN" ) {                   }
		else    { return false  ; }
	}

	p1 = pos( ".ALARM=", rest ) ;
	if ( p1 == 0 )
	{
		p1 = pos( ".A=", rest ) ;
		ln = 3 ;
	}
	else
	{
		ln = 7 ;
	}
	if ( p1 > 0 )
	{
		p2 = pos( " ", rest, p1 ) ;
		if ( p2 == 0 ) { tmp = substr( rest, p1+ln )           ; rest = delstr( rest, p1 )        ; }
		else           { tmp = substr( rest, p1+ln, p2-p1-ln ) ; rest = delstr( rest, p1, p2-p1 ) ; }
		if ( tmp.size() == 0 ) { return false ; }
		if ( tmp[ 0 ] == '&')
		{
			tmp.erase( 0, 1 ) ;
			if ( !isvalidName( tmp ) ) { return false ; }
			t.dvalm = tmp ;
		}
		else if ( tmp == "YES" ) { t.alm = true  ; }
		else if ( tmp == "NO"  ) { t.alm = false ; }
		else                     { return false  ; }
	}

	p1 = pos( ".TYPE=", rest ) ;
	if ( p1 == 0 )
	{
		p1 = pos( ".T=", rest ) ;
		ln = 3 ;
	}
	else
	{
		ln = 6 ;
	}
	if ( p1 > 0 )
	{
		p2 = pos( " ", rest, p1 ) ;
		if ( p2 == 0 ) { tmp = substr( rest, p1+ln )           ; rest = delstr( rest, p1 )        ; }
		else           { tmp = substr( rest, p1+ln, p2-p1-ln ) ; rest = delstr( rest, p1, p2-p1 ) ; }
		if ( tmp.size() == 0 ) { return false ; }
		if ( tmp[ 0 ] == '&')
		{
			tmp.erase( 0, 1 ) ;
			if ( !isvalidName( tmp ) ) { return false ; }
			t.dvtype = tmp ;
		}
		else if ( tmp == "N" ) { t.type = IMT ; t.alm = false ; }
		else if ( tmp == "W" ) { t.type = WMT ; t.alm = true  ; }
		else if ( tmp == "A" ) { t.type = AMT ; t.alm = true  ; }
		else if ( tmp == "C" ) { t.type = AMT ; t.alm = true  ; t.resp = true ; t.smwin = true ; t.lmwin = true ; }
		else                   { return false                 ; }
	}

	if ( trim( rest ) != "" ) { return false ; }

	if ( t.smwin && t.smsg != "" ) { t.lmsg = t.smsg +" - "+ t.lmsg ; }
	return true ;
}


bool pApplication::sub_Message_vars( slmsg& t )
{
	// Get the dialogue variable value specified in message .T, .A, .H and .W options

	// Error if the dialgue variable is blank
	// This routine does not set the return code.  Set in the calling routine.

	// Use defaults for invalid values

	// .TYPE overrides .WINDOW and .ALARM

	string val ;

	if ( t.dvwin != "" )
	{
		t.lmwin = true ;
		vcopy( t.dvwin, val, MOVE ) ;
		trim( val ) ;
		if      ( val == "RESP"    || val == "R"  ) { t.smwin = true  ; t.resp = true ; }
		else if ( val == "NORESP"  || val == "N"  ) { t.smwin = true  ; }
		else if ( val == "LRESP"   || val == "LR" ) { t.resp  = true  ; }
		else if ( val == "LNORESP" || val == "LN" ) {                   }
		else if ( val == "" ) { return false ; }
	}
	if ( t.dvalm != "" )
	{
		vcopy( t.dvalm, val, MOVE ) ;
		trim( val ) ;
		if      ( val == "YES" ) { t.alm = true  ; }
		else if ( val == "NO"  ) { t.alm = false ; }
		else if ( val == ""    ) { return false  ; }
	}
	if ( t.dvtype != "" )
	{
		vcopy( t.dvtype, val, MOVE ) ;
		trim( val ) ;
		if      ( val == "N" ) { t.type = IMT ; t.alm = false ; }
		else if ( val == "W" ) { t.type = WMT ; t.alm = true  ; }
		else if ( val == "A" ) { t.type = AMT ; t.alm = true  ; }
		else if ( val == "C" ) { t.type = AMT ; t.alm = true  ; t.resp = true ; t.smwin = true ; t.lmwin = true ; }
		else if ( val == ""  ) { return false ; }
	}
	if ( t.dvhlp != "" )
	{
		vcopy( t.dvhlp, t.hlp, MOVE ) ;
	}
	return true ;
}


int pApplication::chk_Message_id( const string& msgid )
{
	// Return 0 if message-id format is incorrect, else the offset to the first numeric triplet

	int i ;
	int l ;

	l = msgid.size() ;

	if ( l < 4 || !isvalidName( msgid ) )
	{
		return 0 ;
	}

	l = l - 2 ;
	for ( i = 1 ; i < l ; i++ )
	{
		if ( isdigit( msgid[ i ] ) && isdigit( msgid[ i + 1 ] ) && isdigit( msgid[ i + 2 ] ) )
		{
			return i ;
		}
	}
	return 0 ;
}


string pApplication::sub_vars( string s )
{
	// In string, s, substitute variables starting with '&' for their dialogue value
	// The variable is delimited by any non-valid variable characters.
	// Rules: A '.' at the end of a variable, concatinates the value with the string following the dot
	//        .. reduces to .
	//        && reduces to & with no variable substitution

	int p1 ;
	int p2 ;

	string var ;
	string val ;

	const string validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789#$@" ;
	p1 = 0 ;

	while ( true )
	{
		p1 = s.find( '&', p1 ) ;
		if ( p1 == string::npos || p1 == s.size() - 1 ) { break ; }
		p1++ ;
		if ( s[ p1 ] == '&' )
		{
			s.erase( p1, 1 ) ;
			p1 = s.find_first_not_of( '&', p1 ) ;
			continue ;
		}
		p2  = s.find_first_not_of( validChars, p1 ) ;
		if ( p2 == string::npos ) { p2 = s.size() ; }
		var = upper( s.substr( p1, p2-p1 ) ) ;
		if ( isvalidName( var ) )
		{
			val = "" ;
			vcopy( var, val, MOVE ) ;
			if ( RC <= 8 )
			{
				if ( p2 < s.size() && s[ p2 ] == '.' )
				{
					s.replace( p1-1, var.size()+2, val ) ;
				}
				else
				{
					s.replace( p1-1, var.size()+1, val ) ;
				}
				p1 = p1 + val.size() - 1 ;
			}
		}
	}
	RC = 0   ;
	return s ;
}


void pApplication::info()
{
	int i ;
	int p ;

	llog( "-", "*************************************************************************************************************" << endl ) ;
	llog( "-", "Application Information for "<< ZAPPNAME << endl ) ;
	llog( "-", "                   Task ID: "<< taskID << endl ) ;
	llog( "-", "          Shared Pool Name: "<< shrdPool << endl ) ;
	llog( "-", "         Profile Pool Name: "<< ZZAPPLID << endl ) ;
	llog( "-", " " << endl ) ;
	llog( "-", "Application Description . : "<< ZAPPDESC << endl ) ;
	llog( "-", "Last Panel Displayed. . . : "<< PANELID << endl ) ;
	llog( "-", "Last Message Displayed. . : "<< MSGID << endl )   ;
	llog( "-", "Number of Panels Loaded . : "<< panelList.size() << endl )  ;
	llog( "-", "Number of Open Tables . . : "<< tablesOpen.size() << endl ) ;
	if ( rexxName != "" )
	{
		llog( "-", "Application running REXX. : "<< rexxName << endl ) ;
	}
	llog( "-", " " << endl ) ;
	if ( testMode )
	{
		llog( "-", "Application running in test mode"<< endl ) ;
	}
	if ( noTimeOut )
	{
		llog( "-", "Application has disabled timeouts"<< endl ) ;
	}
	if ( PASSLIB )
	{
		llog( "-", "Application started with PASSLIB option"<< endl ) ;
	}
	if ( NEWPOOL )
	{
		llog( "-", "Application started with NEWPOOL option"<< endl ) ;
	}
	if ( libdef_muser )
	{
		llog( "-", "LIBDEF active for user message search"<< endl ) ;
		p = getpaths( zmuser.top() ) ;
		for ( i = 1 ; i <= p ; i++ )
		{
			llog( "-", "       Path. . . . . . . "<< getpath( zmuser.top(), i ) << endl ) ;
		}

	}
	if ( libdef_puser )
	{
		llog( "-", "LIBDEF active for user panel search"<< endl ) ;
		p = getpaths( zpuser.top() ) ;
		for ( i = 1 ; i <= p ; i++ )
		{
			llog( "-", "       Path. . . . . . . "<< getpath( zpuser.top(), i ) << endl ) ;
		}
	}
	if ( libdef_tuser )
	{
		llog( "-", "LIBDEF active for user table search"<< endl ) ;
		p = getpaths( ztuser.top() ) ;
		for ( i = 1 ; i <= p ; i++ )
		{
			llog( "-", "       Path. . . . . . . "<< getpath( ztuser.top(), i ) << endl ) ;
		}
	}
	llog( "-", "*************************************************************************************************************" << endl ) ;
}


void pApplication::closeTables()
{
	map<string, bool>::iterator it;

	for ( it = tablesOpen.begin() ; it != tablesOpen.end() ; it++ )
	{
		debug1( "Closing table " << it->first << endl ) ;
		p_tableMGR->destroyTable( errBlock, taskid(), it->first ) ;
	}
}


void pApplication::ispexec( const string& s )
{
	ispexeci( this, s, errBlock ) ;
	if ( errBlock.error() )
	{
		errBlock.setcall( "ISPEXEC interface error" ) ;
		checkRCode( errBlock ) ;
	}
	errBlock.clearsrc() ;
}


void pApplication::checkRCode( const string& s )
{
	// If the error panel is to be displayed, cancel CONTROL DISPLAY LOCK

	int RC2 ;

	RC2 = RC ;

	llog( "E", s << endl ) ;
	if ( ZERR2 != "" ) { llog( "E", ZERR2 << endl ) ; }

	if ( !ControlErrorsReturn && RC >= 12 )
	{
		llog( "E", "RC="<< RC <<" CONTROL ERRORS CANCEL is in effect.  Aborting" << endl ) ;
		vreplace( "ZAPPNAME", ZAPPNAME ) ;
		vreplace( "ZERR1",  s ) ;
		vreplace( "ZERR2",  ZERR2  ) ;
		vreplace( "ZERR3",  ZERR3  ) ;
		vreplace( "ZERR4",  ZERR4  ) ;
		vreplace( "ZERR5",  ZERR5  ) ;
		vreplace( "ZERR6",  ZERR6  ) ;
		vreplace( "ZERR7",  ZERR7  ) ;
		vreplace( "ZERR8",  ZERR8  ) ;
		vreplace( "ZERRRC", d2ds( RC2 ) ) ;
		ControlDisplayLock  = false ;
		ControlErrorsReturn = true  ;
		display( "PSYSER1" )  ;
		errPanelissued = true ;
		abend() ;
	}
}


void pApplication::checkRCode( errblock err )
{
	// If the error panel is to be displayed, cancel CONTROL DISPLAY LOCK

	// Format: msg1   header - call description resulting in the error
	//         short  msg
	//         longer description
	//         sline  source line error detected if entered, else dialogue statement.
	//         dline  dialogue statement if entered.

	// Set RC to the error code in the error block if we are returning to the program (CONTROL ERRORS RETURN)
	// and clear errBlock (will have been re-used for services run from this routine)

	// Terminate processing if this routing is called during error processing.

	string t ;

	if ( err.abending() )
	{
		llog( "E", "Errors have occured during error processing.  Terminating application." << endl ) ;
		llog( "E", "Error msg  : "<< err.msg1 << endl )  ;
		llog( "E", "Error id   : "<< err.msgid << endl ) ;
		llog( "E", "Error ZVAL1: "<< err.val1 << endl )  ;
		llog( "E", "Error ZVAL2: "<< err.val2 << endl )  ;
		llog( "E", "Error ZVAL3: "<< err.val3 << endl )  ;
		llog( "E", "Source     : "<< err.getsrc() << endl ) ;
		abend() ;
	}

	if ( ControlErrorsReturn )
	{
		RC = err.getRC() ;
		errBlock.clear() ;
		return ;
	}

	errBlock.setAbending() ;

	if ( err.val1 != "" ) { vreplace( "ZVAL1", err.val1 ) ; }
	if ( err.val2 != "" ) { vreplace( "ZVAL2", err.val2 ) ; }
	if ( err.val3 != "" ) { vreplace( "ZVAL3", err.val3 ) ; }

	getmsg( err.msgid, "ZERRSM", "ZERRLM" ) ;

	vreplace( "ZERRMSG", err.msgid )  ;
	vreplace( "ZERRRC", d2ds( err.getRC() ) ) ;

	llog( "E", err.msg1 << endl ) ;

	vcopy( "ZERRSM", t, MOVE ) ;
	if ( t != "" ) { llog( "E", t << endl ) ; }
	vcopy( "ZERRLM", t, MOVE ) ;
	if ( t != "" ) { llog( "E", t << endl ) ; }

	llog( "E", "RC="<< err.getRC() <<" CONTROL ERRORS CANCEL is in effect.  Aborting" << endl ) ;

	vreplace( "ZAPPNAME", ZAPPNAME ) ;
	vreplace( "ZERR1",  err.msg1  ) ;

	if ( err.sline == "" )
	{
		if ( err.dline != NULL )
		{
			vreplace( "ZERR2", "Current dialogue statement:" ) ;
			vreplace( "ZERR3", err.getsrc() ) ;
		}
		else
		{
			vreplace( "ZERR2", "" ) ;
			vreplace( "ZERR3", "" ) ;
		}
	}
	else
	{
		vreplace( "ZERR2",  "Line where error was detected:" ) ;
		vreplace( "ZERR3", err.getsrc() ) ;
	}
	ControlDisplayLock  = false ;
	ControlErrorsReturn = true  ;
	display( "PSYSER2" )  ;
	errPanelissued = true ;
	abend() ;
}


void pApplication::cleanup_default()
{
	// Dummy routine.  Override in the application so the customised one is called on an exception condition.
	// Use CONTROL ABENDRTN ptr_to_routine

	// Called on: abend()
	//            abendexc()
	//            set_forced_abend()
	//            set_timeout_abend()
}


void pApplication::cleanup()
{
	llog( "I", "Shutting down application: "+ ZAPPNAME +" Taskid: " << taskID << endl ) ;
	terminateAppl = true  ;
	busyAppl      = false ;
	llog( "I", "Returning to calling program." << endl ) ;
	return ;
}


void pApplication::abend()
{
	llog( "E", "Shutting down application: "+ ZAPPNAME +" Taskid: " << taskID << " due to an abnormal condition" << endl ) ;
	abnormalEnd   = true  ;
	terminateAppl = true  ;
	busyAppl      = false ;
	SEL           = false ;
	(this->*pcleanup)()   ;
	llog( "E", "Application entering wait state" << endl ) ;
	boost::this_thread::sleep_for(boost::chrono::seconds(31536000)) ;
}


void pApplication::uabend( const string& msgid, const string& e1, int callno )
{
	// Abend application with error screen.
	// Screen will show short and long messages from msgid, and the return code RC
	// Optional override e1 for the long mesage, and an optional call number

	string t ;

	t = "A user abend has occured in application "+ ZAPPNAME ;
	if ( callno != -1 ) { t += " at call number " + d2ds( callno ) ; }

	getmsg( msgid, "ZERRSM", "ZERRLM" ) ;
	if ( e1 != "" ) { vreplace( "ZERRLM",  e1 ) ; }

	llog( "E", "Shutting down application: "+ ZAPPNAME +" Taskid: " << taskID << " due to a user abend" << endl ) ;

	vreplace( "ZAPPNAME", ZAPPNAME ) ;
	vreplace( "ZERRMSG", msgid )  ;
	vreplace( "ZERRRC", d2ds( RC ) ) ;
	vreplace( "ZERR1",  t ) ;
	vreplace( "ZERR2",  "" ) ;
	vreplace( "ZERR3",  "" ) ;
	ControlDisplayLock  = false ;
	ControlErrorsReturn = true  ;
	display( "PSYSER2" ) ;
	if ( RC <= 8 ) { errPanelissued = true ; }
	abnormalEnd   = true  ;
	terminateAppl = true  ;
	busyAppl      = false ;
	SEL           = false ;
	(this->*pcleanup)()   ;
	llog( "E", "Application entering wait state" << endl ) ;
	boost::this_thread::sleep_for(boost::chrono::seconds(31536000)) ;
}


void pApplication::abendexc()
{
	llog( "E", "An unhandled exception has occured in application: "+ ZAPPNAME +" Taskid: " << taskID << endl ) ;
	if ( !abending )
	{
		(this->*pcleanup)() ;
		abending = true     ;
	}
	else
	{
		llog( "E", "An abend has occured during abend processing.  Cleanup will not be called" << endl ) ;
	}
	exception_ptr ptr = current_exception() ;
	llog( "E", "Exception: " << (ptr ? ptr.__cxa_exception_type()->name() : "Unknown" ) << endl ) ;
	llog( "E", "Shutting down application: " << ZAPPNAME << " Taskid: " << taskID << endl ) ;
	abnormalEnd   = true  ;
	terminateAppl = true  ;
	busyAppl      = false ;
	SEL           = false ;
	llog( "E", "Application entering wait state" << endl ) ;
	boost::this_thread::sleep_for(boost::chrono::seconds(31536000)) ;
}


void pApplication::set_forced_abend()
{
	llog( "E", "Shutting down application: "+ ZAPPNAME +" Taskid: " << taskID << " due to a forced condition" << endl ) ;
	abnormalEnd       = true  ;
	abnormalEndForced = true  ;
	terminateAppl     = true  ;
	busyAppl          = false ;
	SEL               = false ;
	(this->*pcleanup)()       ;
}


void pApplication::set_timeout_abend()
{
	llog( "E", "Shutting down application: "+ ZAPPNAME +" Taskid: " << taskID << " due to a timeout condition" << endl ) ;
	abnormalEnd       = true  ;
	abnormalEndForced = true  ;
	abnormalTimeout   = true  ;
	terminateAppl     = true  ;
	busyAppl          = false ;
	SEL               = false ;
	(this->*pcleanup)()       ;
}


void pApplication::isredit( const string& s)
{
}


void pApplication::closeLog()
{
	llog( "I", "Closing application log" << endl ) ;
	aplog.close() ;
}

