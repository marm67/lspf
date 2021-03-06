/*  Compile with ::                                                                                          */
/* g++ -shared -fPIC -std=c++11 -Wl,-soname,libPBRO01A.so -lmagic -lboost_regex -o libPBRO01A.so PBRO01A.cpp */

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


/******************************************************************************************/
/*  ZRC/ZRSN codes                                                                        */
/*   4/4     File empty                                                                   */
/*   8/4     Cannot open file                                                             */
/*   8/8     Not a regular file                                                           */
/*   8/12    File does not exists                                                         */
/*   8/16    Permission error                                                             */
/*   8/20    Unknown error                                                                */

/* Commands:                                                                              */
/*  BIN/BINARY                                                                            */
/*      Open file as binary                                                               */
/*  HEX ON | OFF                                                                          */
/*      Display data as hex                                                               */
/*  COLS ON  | OFF                                                                        */
/*      Display columns on top line of data                                               */
/*  F/FIND                                                                                */
/*      Find string                                                                       */
/*  RES/RESET                                                                             */
/*      Reset options ( COLS OFF )                                                        */
/*  TEXT                                                                                  */
/*      Open file as text (0A will be interpreted as newline)                             */
/******************************************************************************************/

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <vector>

#include "../lspf.h"
#include "../utilities.h"
#include "../classes.h"
#include "../pWidgets.h"
#include "../pVPOOL.h"
#include "../pTable.h"
#include "../pPanel.h"
#include "../pApplication.h"
#include <magic.h>
#include "eHilight.cpp"
#include "PBRO01A.h"

using namespace boost ;
using namespace std   ;
using namespace boost::filesystem ;

#undef LOGOUT
#undef MOD_NAME

#define LOGOUT aplog
#define MOD_NAME PBRO01A

b_find PBRO01A::Global_bfind_parms ;

void PBRO01A::application()
{
	llog( "I", "Application PBRO01A starting.  Parms are " << PARM << endl ) ;

	int i  ;
	int j  ;
	int offset ;
	int CURPOS ;

	string w2     ;
	string w3     ;
	string file   ;
	string panel  ;
	string ZDSN   ;
	string CURFLD ;
	string ZZSTR1 ;

	bool rebuildZAREA ;

	errblock err ;

	panel = "" ;
	MSG   = "" ;

	file = parseString( err, PARM, "FILE()" ) ;
	if ( err.error() || file == "" )
	{
		llog( "E", "Invalid parameter format passed to PBRO01A" << endl ; )
		abend() ;
		return  ;
	}

	panel = parseString( err, PARM, "PANEL()" ) ;
	if ( err.error() )
	{
		llog( "E", "Invalid parameter format passed to PBRO01A" << endl ; )
		abend() ;
		return  ;
	}

	ZDSN = file ;
	llog( "I", "Displaying file " << file << " using panel " << panel << endl ) ;

	rebuildZAREA = true  ;
	hexOn        = false ;
	colsOn       = false ;
	Asbin        = false ;
	binOn        = false ;
	textOn       = false ;
	hilightOn    = true  ;
	entLang      = ""    ;

	typList[ 'C' ] = "CHARS"  ;
	typList[ 'P' ] = "PREFIX" ;
	typList[ 'S' ] = "SUFFIX" ;
	typList[ 'W' ] = "WORD"   ;

	vdefine( "ZCMD  ZVERB   ZROW1", &ZCMD, &ZVERB, &ZROW1 ) ;
	vdefine( "ZAREA ZSHADOW ZAREAT ZDSN", &ZAREA, &ZSHADOW, &ZAREAT, &ZDSN ) ;
	vdefine( "ZSCROLLN ZAREAW ZAREAD", &ZSCROLLN, &ZAREAW, &ZAREAD ) ;
	vdefine( "ZSCROLLA ZCOL1 ZCOL2 TYPE STR", &ZSCROLLA, &ZCOL1, &ZCOL2, &TYPE, &STR ) ;
	vdefine( "OCC LINES CMD ZZSTR1", &OCC, &LINES, &CMD, &ZZSTR1 ) ;

	startCol  = 1 ;
	maxCol    = 1 ;

	pquery( "PBRO01A1", "ZAREA", "ZAREAT", "ZAREAW", "ZAREAD" ) ;

	RC = 0 ;
	read_file( file ) ;
	if ( RC > 0 )
	{
		ZZSTR1 = file ;
		if ( ZRC == 4 )
		{
			if ( ZRSN == 4 ) { setmsg( "PSYS011P" ) ; }
		}
		else if ( ZRC == 8 )
		{
			if       ( ZRSN == 4  ) { setmsg( "PSYS011Q" ) ; }
			else if  ( ZRSN == 8  ) { setmsg( "PSYS011R" ) ; }
			else if  ( ZRSN == 12 ) { setmsg( "PSYS011S" ) ; }
			else if  ( ZRSN == 16 ) { setmsg( "PSYS011T" ) ; }
			else if  ( ZRSN == 20 ) { setmsg( "PSYS011U" ) ; }
		}
		setmsg( "PSYS011U", COND ) ;
		cleanup() ; return ;
	}

	MSG    = ""     ;
	CURFLD = "ZCMD" ;
	CURPOS = 1      ;

	if ( Global_bfind_parms.f_fset )
	{
		find_parms = Global_bfind_parms ;
	}

	ZASIZE = ZAREAW*ZAREAD ;

	while ( true )
	{
		if ( rebuildZAREA ) { fill_dynamic_area() ; }
		else                { ZSHADOW = CSHADOW   ; }
		if ( CURFLD == "ZAREA" )
		{
			for ( i = CURPOS-1 ; i < ZASIZE ; i++ )
			{
				if ( ZAREA[ i ] == ' ' ) { break ; }
				ZSHADOW[ i ] = B_WHITE ;
			}
			for ( i = CURPOS-1 ; i > 0 ; i-- )
			{
				if ( ZAREA[ i ] == ' ' ) { break ; }
				ZSHADOW[ i ] = B_WHITE ;
			}
		}

		ZROW1 = right( d2ds( topLine ), 8, '0' )  ;
		ZCOL1 = right( d2ds( startCol ), 5, '0' ) ;
		ZCOL2 = right( d2ds( startCol+ZAREAW-1 ), 5, '0' ) ;
		if ( MSG == "" ) { ZCMD = "" ; }

		display( "PBRO01A1", MSG, CURFLD, CURPOS ) ;
		if ( RC == 8 ) { cleanup() ; break ; }

		MSG          = ""    ;
		rebuildZAREA = false ;

		vget( "ZVERB ZSCROLLA ZSCROLLN", SHARED ) ;
		CMD  = upper( word( ZCMD, 1 ) ) ;
		w2   = upper( word( ZCMD, 2 ) ) ;
		w3   = upper( word( ZCMD, 3 ) ) ;

		if ( ZCURFLD == "ZAREA" )
		{
			if ( (colsOn && ZCURPOS <= ZAREAW) ) { CURFLD = "ZCMD" ; CURPOS = 1 ; }
			else if ( topLine == 0 && ( ZCURPOS <= ZAREAW || (colsOn && ZCURPOS <= 2*ZAREAW))) { CURFLD = "ZCMD" ; CURPOS = 1 ; }
			else { CURFLD = "ZAREA" ; CURPOS = ZCURPOS ; }
		}
		else
		{
			CURFLD = "ZCMD" ;
			CURPOS = 1      ;
		}

		if ( CMD == "" ) {}
		else if ( CMD == "BIN" || CMD == "BINARY" )
		{
			if ( w2 != "" ) { MSG = "PBRO013" ; continue ; }
			Asbin        = true  ;
			binOn        = true  ;
			textOn       = false ;
			rebuildZAREA = true  ;
			read_file( file ) ;
			if ( RC > 0 ) { setmsg( "PSYS011E" ) ; cleanup() ; return ; }
		}
		else if ( CMD == "HEX" )
		{
			if      ( (w2 == "" ) || (w2 == "ON" && w3 == "") ) { hexOn = true  ; rebuildZAREA = true ; }
			else if ( (w2 == "OFF" && w3 == "") )               { hexOn = false ; rebuildZAREA = true ; }
			else    { MSG = "PBRO012" ; continue ; }
		}
		else if ( findword( CMD, "HILITE HILIGHT HI" ) )
		{
			if ( w3 == "" ) { w3 = "AUTO" ; }
			if ( w3 != "AUTO" && !addHilight( w3 ) ) { MSG = "PBRO011M" ; continue ; }
			entLang = w3 ;
			if      ( w3 == "AUTO"  ) { detLang = determineLang( file ) ; }
			else                      { detLang = w3                    ; }
			if      ( w2 == "ON" )
			{
				for_each( shadow.begin(), shadow.end(),
					  [](b_shadow& a) { a.bs_wShadow = false ; a.bs_vShadow = false ; } ) ;
				hilightOn = true  ; rebuildZAREA = true ;
			}
			else if ( w2 == "OFF" )
			{
				for_each( shadow.begin(), shadow.end(),
					  [](b_shadow& a) { a.bs_wShadow = false ; a.bs_vShadow = false ; } ) ;
				hilightOn = false ; rebuildZAREA = true ;
			}
		}
		else if ( CMD == "COLS" )
		{
			if      ( (w2 == "" ) || (w2 == "ON" && w3 == "") ) { colsOn = true  ; rebuildZAREA = true ; }
			else if ( (w2 == "OFF" && w3 == "") )               { colsOn = false ; rebuildZAREA = true ; }
			else    { MSG = "PBRO012" ; continue ; }
		}
		else if ( CMD == "F" || CMD == "FIND" )
		{
			if ( setFind() > 0 ) { continue ; }
			ZCMD = ""  ;
			if ( ZCURFLD == "ZAREA" ) { offset = ZCURPOS ; if ( ( offset % ZAREAW ) == 1  ) { offset++ ; } }
			else                      { offset = 0       ; }
			actionFind( topLine, offset ) ;
			if ( find_parms.f_line > 0 )
			{
				ZALINE = find_parms.f_line - topLine ;
				if ( ZALINE < 0 || ZALINE > ZAREAD-1 )
				{
					topLine = find_parms.f_line - 1 ;
				}
				CURFLD = "ZAREA" ;
				CURPOS = ( find_parms.f_line - topLine ) * ZAREAW + find_parms.f_offset + 1 ;
				if ( colsOn ) { CURPOS = CURPOS + ZAREAW ; }
				TYPE   = typList[ find_parms.f_mtch ] ;
				STR    = find_parms.f_estring ;
				OCC    = d2ds( find_parms.f_occurs ) ;
				LINES  = d2ds( find_parms.f_lines  ) ;
				if ( find_parms.f_dir == 'A' ) { MSG = "PBRO011G" ; find_parms.f_dir = 'N' ; }
				else                           { MSG = "PBRO011F" ;                          }
			}
			else
			{
				CURFLD = "ZCMD" ;
				CURPOS = 1 ;
				TYPE = typList[ find_parms.f_mtch ] ;
				STR = find_parms.f_estring ;
				MSG = "PBRO011E" ;
				continue         ;
			}
			rebuildZAREA = true ;
			continue            ;
		}
		else if ( CMD == "RES" || CMD == "RESET" )
		{
			if ( w2 != "" ) { MSG = "PBRO013" ; continue ; }
			colsOn       = false ;
			binOn        = false ;
			textOn       = false ;
			rebuildZAREA = true  ;
		}
		else if ( CMD == "TEXT" )
		{
			if ( w2 != "" ) { MSG = "PBRO013" ; continue ; }
			Asbin        = false ;
			binOn        = false ;
			textOn       = true  ;
			rebuildZAREA = true  ;
			read_file( file )    ;
			if ( RC > 0 ) { setmsg( "PSYS011E" ) ; cleanup() ; return ; }
		}
		else { MSG = "PBRO011" ; continue ; }

		if ( ZVERB == "" ) {}
		else if ( ZVERB == "DOWN" )
		{
			rebuildZAREA = true ;
			if ( ZSCROLLA == "MAX" )
			{
				topLine = maxLines - ZAREAD ;
			}
			else
			{
				topLine = topLine + ZSCROLLN  ;
				if ( topLine >= maxLines ) { topLine = maxLines - 1 ; }
			}
		}
		else if ( ZVERB == "UP" )
		{
			rebuildZAREA = true ;
			if ( ZSCROLLA == "MAX" )
			{
				topLine = 0 ;
			}
			else
			{
				topLine = topLine - ZSCROLLN  ;
			}
		}
		else if ( ZVERB == "LEFT" )
		{
			rebuildZAREA = true ;
			if ( ZSCROLLA == "MAX" )
			{
				startCol = 1 ;
			}
			else
			{
				startCol = startCol - ZSCROLLN ;
				if ( startCol < 1 ) { startCol = 1 ; }
			}
		}
		else if ( ZVERB == "RIGHT" )
		{
			rebuildZAREA = true ;
			if ( ZSCROLLA == "MAX" )
			{
				startCol = maxCol - ZAREAW ;
				if ( startCol < 1 ) { startCol = 1 ; }
			}
			else
			{
				startCol = startCol + ZSCROLLN ;
			}
		}
		else if ( ZVERB == "RFIND" )
		{
			if ( find_parms.f_fset )
			{
				if ( ZCURFLD == "ZAREA" ) { offset = ZCURPOS ; if ( ( offset % ZAREAW ) == 1  ) { offset++ ; } }
				else                      { offset = 0       ; }
				actionFind( topLine, offset ) ;
				if ( find_parms.f_line > 0 )
				{
					ZALINE = find_parms.f_line - topLine ;
					if ( ZALINE < 0 || ZALINE > ZAREAD-1 )
					{
						topLine = find_parms.f_line - 1 ;
					}
					CURFLD  = "ZAREA" ;
					CURPOS  = ( find_parms.f_line - topLine ) * ZAREAW + find_parms.f_offset + 1 ;
					if ( colsOn ) { CURPOS = CURPOS + ZAREAW ; }
					TYPE    = typList[ find_parms.f_mtch ] ;
					STR     = find_parms.f_estring ;
					MSG     = "PBRO011F" ;
				}
				else
				{
					if ( find_parms.f_dir == 'N' ||
					     find_parms.f_dir == 'F' ) { find_parms.f_dir = 'F' ; MSG = "PBRO016" ; }
					else                           { find_parms.f_dir = 'L' ; MSG = "PBRO017" ; }
					continue ;
				}
				rebuildZAREA = true ;
			}
			else { MSG = "PBRO011A" ; continue ; }
		}
		else  { MSG = "PBRO011" ; continue ; }

		if ( topLine < 0 ) topLine = 0 ;
	}
	vput( "ZSCROLL", PROFILE ) ;
	Global_bfind_parms = find_parms ;
	return ;
}


void PBRO01A::read_file( string file )
{
	string inLine ;
	string line1  ;
	string ext    ;
	string w1     ;

	b_shadow t    ;

	int p1 ;
	int rc ;
	int i   ;
	int j   ;
	char x  ;

	topLine = 0 ;
	std::ifstream fin ;

	try
	{
		if ( !exists( file ) )
		{
			RC   = 16 ;
			ZRC  = 8  ;
			ZRSN = 12 ;
			ZRESULT = "File does not exists" ;
			return    ;
		}
	}
	catch ( const filesystem_error& ex )
	{
		RC   = 16 ;
		ZRC  = 8  ;
		ZRSN = 16 ;
		ZRESULT = "Permission Error" ;
		return    ;
	}

	if ( !is_regular_file( file.c_str() ) )
	{
		RC   = 16 ;
		ZRC  = 8  ;
		ZRSN = 8  ;
		ZRESULT = "Not a regular file" ;
		return    ;
	}

	fin.open( file.c_str() ) ;

	if ( !fin.is_open() )
	{
		RC   = 16 ;
		ZRC  = 8  ;
		ZRSN = 4  ;
		ZRESULT = "Open Error" ;
		return    ;
	}

	fileType = "text/plain" ;
	magic_t cookie = magic_open( MAGIC_CONTINUE | MAGIC_ERROR | MAGIC_MIME | MAGIC_SYMLINK ) ;
	rc = magic_load( cookie, NULL ) ;

	if ( rc == 0 )
	{
		fileType = magic_file( cookie, file.c_str() ) ;
		if ( fileType != "" )
		{
			if ( findword( "charset=binary", fileType ) )
			{
				Asbin = true ;
			}
			else
			{
				p1 = fileType.find( ';' ) ;
				if ( p1 != string::npos ) { fileType = fileType.substr( 0, p1 ) ; }
			}
		}
	}
	magic_close( cookie ) ;

	maxLines = 1 ;
	data.clear()   ;
	shadow.clear() ;
	data.push_back( centre( " Top of Data ", ZAREAW, '*' ) ) ;
	shadow.push_back( t ) ;

	if      ( binOn )  { Asbin = true ; }
	else if ( textOn ) { Asbin = false ; }

	if ( Asbin )
	{
		inLine = string( ZAREAW, ' ' ) ;
		maxCol = ZAREAW ;
		i      = 0      ;
		while ( true )
		{
			fin.get( x ) ;
			if ( fin.fail() != 0 ) { break ; } ;
			inLine[ i ] = x ;
			i++ ;
			if ( i == ZAREAW )
			{
				data.push_back( inLine )       ;
				inLine = string( ZAREAW, ' ' ) ;
				shadow.push_back( t ) ;
				maxLines++ ;
				i = 0      ;
			}
		}
		if ( i > 0 )
		{
			inLine.resize( i )       ;
			data.push_back( inLine ) ;
			shadow.push_back( t ) ;
			maxLines++ ;
		}
		fileType  = "application/octet-stream" ;
		hilightOn = false ;
	}
	else
	{
		while ( getline( fin, inLine ) )
		{
			if ( maxLines == 1 ) { line1 = inLine ; }
			p1 = inLine.find_first_of( '\t' ) ;
			while ( p1 != string::npos )
			{
				j = 8 - (p1 % 8 ) ;
				inLine.replace( p1, 1, j, ' ' ) ;
				p1 = inLine.find_first_of( '\t', p1 + 1 ) ;
			}
			if ( maxCol < inLine.size() ) maxCol = inLine.size() ;
			data.push_back( inLine ) ;
			shadow.push_back( t ) ;
			maxLines++ ;
		}
		if ( entLang == "" || entLang == "AUTO" )
		{
			detLang = determineLang( file ) ;
		}
		else
		{
			detLang = entLang ;
		}
		hilightOn = true  ;
	}
	if ( maxLines == 1 )
	{
		RC   = 4  ;
		ZRC  = 4  ;
		ZRSN = 4  ;
		ZRESULT = "File Empty" ;
		return    ;
	}
	maxCol++   ;
	maxLines++ ;
	data.push_back( centre( " Bottom of Data ", ZAREAW, '*' ) ) ;
	shadow.push_back( t ) ;
	fin.close() ;
}


void PBRO01A::fill_dynamic_area()
{
	string w   ;
	string t1  ;
	string t2  ;
	string t3  ;
	string t4  ;
	string s1b ;
	string s1g ;
	string s1y ;
	string s1w ;
	string div ;
	string col ;

	int i  ;
	int l  ;
	int t  ;
	int wI ;
	int wL ;
	int ln ;

	s1b = string( ZAREAW, B_BLUE   ) ;
	s1g = string( ZAREAW, N_GREEN  ) ;
	s1y = string( ZAREAW, N_YELLOW ) ;
	s1w = string( ZAREAW, N_WHITE  ) ;
	div = string( ZAREAW, '-' )      ;

	if ( colsOn )
	{
		col = substr( "+----+----+----+---", startCol%10+1, 10-startCol%10 ) ;
		if ( startCol%10 == 0 ) { col = "" ; t = startCol / 10 ; }
		else t = startCol / 10 + 1 ;
		for ( i = 0 ; i < (ZAREAW/10+1) ; i++ )
		{
			col = col + d2ds( t%10 ) + "----+----" ;
			t++ ;
		}
		col = substr( col, 1, ZAREAW ) ;
		ZAREA   = col ;
		ZSHADOW = s1w ;
	}
	else
	{
		ZAREA   = "" ;
		ZSHADOW = "" ;
	}

	if ( hexOn )
	{
		for( int k = topLine ; k < (topLine + ZAREAD) ; k++ )
		{
			t3 = string( ZAREAW, ' ' ) ;
			t4 = string( ZAREAW, ' ' ) ;
			if ( k > 0 && k < data.size()-1 )
			{
				ln = data.at( k ).size() - startCol + 1 ;
				if ( ln > ZAREAW ) ln = ZAREAW       ;
				t1 = substr( data.at( k ), startCol, ZAREAW ) ;
				t2 = cs2xs( t1 ) ;
				if ( ln > 0 )
				{
					i = 0 ;
					for ( l = 0 ; l < (ln * 2) ; l++ )
					{
						t3[ i ] = t2[ l ] ;
						l++ ;
						t4[ i ] = t2[ l ] ;
						i++ ;
					}
				}
				ZAREA   += t1  + t3  + t4  + div ;
				ZSHADOW += s1y + s1b + s1b + s1w ;
			}
			else
			{
				ZAREA   += substr( data.at( k ), 1, ZAREAW ) ;
				ZSHADOW += s1b ;
			}
			if ( ZAREA.size() >= ZASIZE ) { break ; }
			if ( k >= data.size() - 1 )   { break ; }
		}
	}
	else
	{
		for( int k = topLine ; k < (topLine + ZAREAD) ; k++ )
		{
			if ( k >  0 && k < data.size()-1 ) { t1 = substr( data.at( k ), startCol, ZAREAW ) ; }
			else                               { t1 = substr( data.at( k ), 1, ZAREAW )        ; }
			ZAREA += t1 ;
			if ( ZAREA.size() >= ZASIZE ) { break ; }
			if ( k == data.size() - 1 )   { break ; }
		}

	}
	ZAREA.resize( ZASIZE, ' ' ) ;
	ZSHADOW.resize( ZASIZE, B_BLUE ) ;
	if ( hilightOn && !hlight.hl_abend )
	{
		fill_hilight_shadow() ;
	}
	CSHADOW = ZSHADOW ;
}


void PBRO01A::fill_hilight_shadow()
{
	// Build il_shadow starting at the first invalid shadow line in bs_shadow,
	// (backing up to the line after the position where there are no open brackets/comments)
	// until bottom of ZAREA reached

	int i  ;
	int dl ;
	int ll ;
	int l  ;
	int w  ;

	string ZTEMP ;

	hlight.hl_language = detLang ;

	ll = data.size()-2 ;
	if ( topLine+ZAREAD < ll ) { ll = topLine+ZAREAD ; }

	for ( w = 0, dl = 1 ; dl <= ll ; dl++ )
	{
		if ( !shadow.at( dl ).bs_vShadow ) { break  ; }
		if (  shadow.at( dl ).bs_wShadow ) { w = dl ; }
	}

	if ( dl <= ll )
	{
		hlight.hl_oBrac1   = 0     ;
		hlight.hl_oBrac2   = 0     ;
		hlight.hl_oIf      = 0     ;
		hlight.hl_oDo      = 0     ;
		hlight.hl_ifLogic  = true  ;
		hlight.hl_doLogic  = true  ;
		hlight.hl_Paren    = true  ;
		hlight.hl_oComment = false ;
		for ( dl = w + 1 ; dl <= ll ; dl++ )
		{
			addHilight( hlight, data[ dl ], shadow[ dl ].bs_Shadow ) ;
			if ( hlight.hl_abend ) { return ; }
			shadow[ dl ].bs_vShadow = true ;
			shadow[ dl ].bs_wShadow = ( hlight.hl_oBrac1 == 0 &&
						    hlight.hl_oBrac2 == 0 &&
						    hlight.hl_oIf    == 0 &&
						    hlight.hl_oDo    == 0 &&
						   !hlight.hl_continue    &&
						   !hlight.hl_oQuote      &&
						   !hlight.hl_oComment ) ;
		}
	}
	for ( i = 0 ; i < ZAREAD ; i++ )
	{
		l = topLine + i ;
		if ( l > data.size() - 2 ) { break ; }
		ZTEMP = shadow.at( l ).bs_Shadow ;
		if ( startCol > 1 ) { ZTEMP.erase( 0, startCol-1 ) ; }
		ZTEMP.resize( ZAREAW, B_BLUE ) ;
		ZSHADOW.replace( ZAREAW*(i), ZAREAW, ZTEMP ) ;
	}
}


int PBRO01A::setFind()
{
	int i        ;
	int j        ;
	int p1       ;
	int p2       ;
	int ws       ;
	char c1      ;
	char c2      ;
	string delim ;
	string cmd   ;
	string ucmd  ;
	string w1    ;
	string pic   ;

	vector<string>::iterator it ;

	const char quote('\"') ;
	const char apost('\'') ;

	b_find t ;

	MSG = ""  ;
	cmd = " " + subword( ZCMD, 2 ) + " " ;

	p1 = cmd.find( quote ) ;
	p2 = cmd.find( apost ) ;

	if      ( p1 == string::npos ) { delim = string( 1, apost ) ; }
	else if ( p2 == string::npos ) { delim = string( 1, quote ) ; }
	else if ( p1 < p2 )            { delim = string( 1, quote ) ; }
	else                           { delim = string( 1, apost ) ; }

	if ( p1 = pos( delim, cmd  ) )
	{
		p2  = pos( delim, cmd,  p1+1 ) ;
		if ( p2 == 0 ) { MSG = "PBRO011H" ; return 20 ; }
		c1 = toupper( cmd[ p1-2 ] ) ;
		c2 = toupper( cmd[ p2   ] ) ;
		if ( c1 == ' ' && c2 == ' ' ) { t.f_text = true ; }
		else if ( c1 == 'T' && c2 == ' ' ) { t.f_text   = true ; }
		else if ( c1 == 'C' && c2 == ' ' ) { t.f_asis   = true ; }
		else if ( c1 == 'X' && c2 == ' ' ) { t.f_hex    = true ; }
		else if ( c1 == 'P' && c2 == ' ' ) { t.f_pic    = true ; }
		else if ( c1 == 'R' && c2 == ' ' ) { t.f_rreg   = true ; }
		else if ( c2 == 'T' && c1 == ' ' ) { t.f_text   = true ; }
		else if ( c2 == 'C' && c1 == ' ' ) { t.f_asis   = true ; }
		else if ( c2 == 'X' && c1 == ' ' ) { t.f_hex    = true ; }
		else if ( c2 == 'P' && c1 == ' ' ) { t.f_pic    = true ; }
		else if ( c2 == 'R' && c1 == ' ' ) { t.f_rreg   = true ; }
		else                               { MSG = "PBRO011I" ; return 20 ; }
		if ( t.f_text )
		{
			t.f_estring = substr( cmd, (p1+1), (p2-p1-1) ) ;
			t.f_string  = upper( t.f_estring )             ;
		}
		else
		{
			t.f_estring = substr( cmd, (p1+1), (p2-p1-1) ) ;
			t.f_string  = t.f_estring ;
			t.f_asis    = true        ;
		}
		cmd = delstr( cmd, (p1-1), (p2-p1+3) ) ;
	}

	if ( t.f_rreg ) { t.f_regreq = true ; }

	ucmd = upper( cmd ) ;
	if ( p1 = wordpos( "NEXT", ucmd ) )
	{
		t.f_dir = 'N' ;
		p1   = wordindex( ucmd, p1 ) ;
		ucmd = delstr( ucmd, p1, 4 ) ;
	}
	else if ( p1 = wordpos( "PREV", ucmd ) )
	{
		t.f_dir = 'P' ;
		p1   = wordindex( ucmd, p1 ) ;
		ucmd = delstr( ucmd, p1, 4 ) ;
	}
	else if ( p1 = wordpos( "FIRST", ucmd ) )
	{
		t.f_dir = 'F' ;
		p1   = wordindex( ucmd, p1 ) ;
		ucmd = delstr( ucmd, p1, 5 ) ;
	}
	else if ( p1 = wordpos( "LAST", ucmd ) )
	{
		t.f_dir = 'L' ;
		p1   = wordindex( ucmd, p1 ) ;
		ucmd = delstr( ucmd, p1, 4 ) ;
	}
	else if ( p1 = wordpos( "ALL", ucmd ) )
	{
		t.f_dir = 'A' ;
		p1   = wordindex( ucmd, p1 ) ;
		ucmd = delstr( ucmd, p1, 3 ) ;
	}

	if ( !t.f_rreg )
	{
		if ( p1 = wordpos( "CHARS", ucmd ) )
		{
			t.f_mtch  = 'C'  ;
			p1   = wordindex( ucmd, p1 ) ;
			ucmd = delstr( ucmd, p1, 5 ) ;
		}
		else if ( p1 = wordpos( "PRE", ucmd ) )
		{
			t.f_mtch   = 'P'  ;
			t.f_regreq = true ;
			p1   = wordindex( ucmd, p1 ) ;
			ucmd = delstr( ucmd, p1, 3 ) ;
		}
		else if ( p1 = wordpos( "PREFIX", ucmd ) )
		{
			t.f_mtch   = 'P'  ;
			t.f_regreq = true ;
			p1   = wordindex( ucmd, p1 ) ;
			ucmd = delstr( ucmd, p1, 6 ) ;
		}
		else if ( p1 = wordpos( "SUF", ucmd ) )
		{
			t.f_mtch   = 'S'  ;
			t.f_regreq = true ;
			p1   = wordindex( ucmd, p1 ) ;
			ucmd = delstr( ucmd, p1, 3 ) ;
		}
		else if ( p1 = wordpos( "SUFFIX", ucmd ) )
		{
			t.f_mtch   = 'S'  ;
			t.f_regreq = true ;
			p1   = wordindex( ucmd, p1 ) ;
			ucmd = delstr( ucmd, p1, 6 ) ;
		}
		else if ( p1 = wordpos( "WORD", ucmd ) )
		{
			t.f_mtch   = 'W'  ;
			t.f_regreq = true ;
			p1   = wordindex( ucmd, p1 ) ;
			ucmd = delstr( ucmd, p1, 4 ) ;
		}
	}

	ws = words( ucmd ) ;
	for ( i = 1 ; i <= ws ; i++ )
	{
		w1 = word( ucmd, i ) ;
		if ( datatype( w1, 'W' ) )
		{
			if ( t.f_scol != 0 && t.f_ecol != 0 ) { MSG = "PBRO019" ; return 20 ; }
			j = ds2d( w1 ) ;
			if ( j < 1 || j > 65535 ) { MSG = "PBRO011J" ; return 20 ; }
			if ( t.f_scol == 0 ) { t.f_scol = j ; }
			else                 { t.f_ecol = j ; }
		}
		else
		{
			if ( w1 == "*" )
			{
				if ( find_parms.f_string == "" ) { MSG = "PBRO011C" ; return 20 ; }
				else
				{
					if ( t.f_string != "" )  { MSG = "PBRO015"  ; return 20 ; }
					w1 = find_parms.f_string ;
				}
			}
			if ( t.f_string != "" ) { STR = w1 ; MSG = "PBRO018" ; return 20 ; }
			t.f_text    = true ;
			t.f_estring = w1   ;
			t.f_string  = upper( w1 ) ;
		}
	}

	if ( t.f_scol>0 && t.f_ecol>0 && (t.f_scol > t.f_ecol) )
	{
		i        = t.f_scol ;
		t.f_scol = t.f_ecol ;
		t.f_ecol = i ;
	}

	if ( t.f_scol>0 && t.f_ecol>0 && ((t.f_ecol - t.f_scol) < t.f_estring.size()) ) { MSG = "PBRO014" ; return 20 ; }

	if ( t.f_scol>0 && t.f_ecol==0 ) { t.f_oncol = true ; }

	if ( t.f_hex )
	{
		if ( !ishex( t.f_string ) )  { MSG = "PBRO011K" ; return 20 ; }
		t.f_string = xs2cs( t.f_string ) ;
		t.f_asis   = true                ;
	}

	if ( t.f_pic )
	{
		pic = "" ;
		j   = 0  ;
		// =  any character                   .  invalid characters
		// @  alphabetic characters           -  non-numeric characters
		// #  numeric characters              <  lower case alphabetics
		// $  special characters              >  upper case alphabetics
		// ?  non-blank characters            *  any number of non-blank characters
		for ( i = 0 ; i < t.f_string.size() ; i++ )
		{
			switch ( t.f_string[ i ] )
			{
				case '*':
					pic = pic + "[^[:blank:]]*" ;
					break ;
				case '=':
					pic = pic + "." ;
					break ;
				case '@':
					pic = pic + "[[:alpha:]]" ;
					break ;
				case '#':
					pic = pic + "[[:digit:]]" ;
					break ;
				case '$':
					pic = pic + "[^[:blank:]^[:alpha:]^[:digit:]]" ;
					break ;
				case '?':
					pic = pic + "[^[:blank:]]" ;
					break ;
				case '.':
					pic = pic + "[^[:print:]]" ;
					break ;
				case '-':
					pic = pic + "[^[:digit:]]" ;
					break ;
				case '<':
					pic = pic + "[a-z]" ;
					break ;
				case '>':
					pic = pic + "[A-Z]" ;
					break ;
				default:
					pic = pic + t.f_string[ i ] ;
					break ;
			}
		}
		t.f_string = pic  ;
		t.f_regreq = true ;
	}

	if ( t.f_regreq && !t.f_rreg )
	{
		switch ( t.f_mtch )
		{
			case 'P':
				t.f_string = "\\b" + t.f_string + "\\w" ;
				break ;
			case 'S':
				t.f_string = "\\w" + t.f_string + "\\b" ;
				break ;
			case 'W':
				t.f_string = "\\b" + t.f_string + "\\b" ;
				break ;
		}
	}

	t.f_fset = true ;
	find_parms = t  ;
	return 0 ;
}


void PBRO01A::actionFind( int spos, int offset )
{
	int dl ;
	int c1 ;
	int c2 ;
	int i  ;
	int j  ;
	int p1 ;
	int oX ;
	int oY ;

	bool found  ;
	bool found1 ;
	bool skip   ;

	vector<string>::iterator it  ;
	vector<string>::iterator itt ;
	vector<string>::iterator its ;
	vector<string>::iterator ite ;

	string::const_iterator itss ;
	string::const_iterator itse ;

	regex regexp ;
	smatch results ;

	j = 0 ;

	find_parms.f_occurs = 0 ;
	find_parms.f_line   = 0 ;
	find_parms.f_lines  = 0 ;
	find_parms.f_offset = 0 ;

	if ( find_parms.f_dir == 'F' || find_parms.f_dir == 'A' || find_parms.f_dir == 'L' ) { offset = 0 ; }

	if ( offset > ZAREAW && colsOn ) { offset = offset - ZAREAW ; }
	if ( offset > 0 ) { oX = (offset % ZAREAW)-1 ; oY = offset / ZAREAW ; }
	else              { oX = 0        ; oY = 0 ; }
	if ( oX == -1 )   { oX = ZAREAW-1 ; oY--   ; }

	try
	{
		if ( find_parms.f_regreq )
		{
			if ( find_parms.f_asis )
			{
				regexp.assign( find_parms.f_string )   ;
			}
			else
			{
				regexp.assign( find_parms.f_string, boost::regex::ECMAScript|boost::regex::icase ) ;
			}
		}
	}
	catch  (boost::regex_error& e)
	{
		if ( find_parms.f_regreq )
		{
			MSG = "PBRO011N" ;
			return           ;
		}
	}

	if      ( find_parms.f_dir == 'F' ) { dl = 1             ; }
	else if ( find_parms.f_dir == 'A' ) { dl = 1             ; }
	else if ( find_parms.f_dir == 'L' ) { dl = data.size()-2 ; }
	else                                { dl = spos + oY     ; }

	if ( dl > data.size() - 2 ) { dl = spos ; }
	if ( dl == 0 ) { dl = 1 ; }

	while ( true )
	{

		skip = false ;
		c1   = 0 ;
		c2   = data.at( dl ).size() -1 ;

		if ( find_parms.f_scol > 0 ) { c1 = find_parms.f_scol - 1 ; }
		if ( find_parms.f_ecol > 0 && c2 > find_parms.f_ecol-1 ) { c2 = find_parms.f_ecol - 1 ; }

		if ( oX > 0 )
		{
			if ( oX > data.at( dl ).size()-1 ) { oX = data.at( dl ).size()-1 ; }
			if ( find_parms.f_dir == 'P' || find_parms.f_dir == 'L' )
			{
				if ( oX < c1 )        { skip = true ; }
				else if ( oX <= c2 )  { c2   = oX-1 ; }
			}
			else
			{
				if ( oX > c2 )       { skip = true ; }
				else if ( oX >= c1 ) { c1   = oX+1 ; }
			}
		}
		oX = 0 ;
		if ( skip || c1 >= c2 )
		{
			if ( find_parms.f_dir == 'L' || find_parms.f_dir == 'P' ) { dl-- ; }
			else { dl++ ; }
			if ( dl < 1 || dl > data.size()-2 ) { break ; }
			continue ;
		}

		if ( find_parms.f_regreq )
		{
			found1 = true  ;
			found  = false ;
			itss   = data.at( dl ).begin() ;
			advance( itss, c1 ) ;
			itse   = itss    ;
			advance( itse, c2-c1+1 ) ;
			if ( find_parms.f_oncol )
			{
				if ( regex_search( itss, itse, results, regexp ) )
				{
					if ( itss == results[0].first ) { found = true ; p1 = find_parms.f_scol-1 ; }
				}
			}
			else
			{
				if ( find_parms.f_dir == 'P' || find_parms.f_dir == 'L' || find_parms.f_dir == 'A' )
				{
					while ( regex_search( itss, itse, results, regexp ) )
					{
						found = true ;
						for ( p1 = c1 ; itss != results[0].first ; itss++ ) { p1++ ; }
						c1 = p1 + 1 ;
						itss  = results[0].first ;
						itss++ ;
						if ( found1 ) { found1 = false ; find_parms.f_lines++ ; }
						find_parms.f_occurs++ ;
						if ( find_parms.f_dir == 'A' && find_parms.f_line == 0 ) { find_parms.f_line = dl ; find_parms.f_offset = p1 ; }
					}
				}
				else
				{
					if ( regex_search( itss, itse, results, regexp ) )
					{
						found = true ;
						for ( p1 = c1 ; itss != results[0].first ; itss++ ) { p1++ ; }
					}
				}
			}
			if ( found && find_parms.f_line == 0 ) { find_parms.f_line = dl ; find_parms.f_offset = p1 ; }
		}
		else
		{
			found1 = true ;
			while ( true )
			{
				found  = false ;
				if ( find_parms.f_dir == 'P' || find_parms.f_dir == 'L' )
				{
					if ( find_parms.f_asis ) { p1 = data.at( dl ).rfind( find_parms.f_string, c2 )       ; }
					else                     { p1 = upper( data.at( dl ) ).rfind( find_parms.f_string, c2 ) ; }
					c2 = p1 - 1 ;
				}
				else
				{
					if ( find_parms.f_asis ) { p1 = data.at( dl ).find( find_parms.f_string, c1 )          ; }
					else                     { p1 = upper( data.at( dl ) ).find( find_parms.f_string, c1 ) ; }
					c1 = p1 + 1 ;
				}
				if ( p1 != string::npos )
				{
					if ( find_parms.f_oncol )
					{
						if ( p1 == find_parms.f_scol-1 ) { found = true ; }
					}
					else if ( find_parms.f_dir == 'P' || find_parms.f_dir == 'L' )
					{
						if ( p1 >= c1 ) { found = true ; }
					}
					else
					{
						if ( p1 <= c2 ) { found = true ; }
					}
				}
				if ( found && find_parms.f_line == 0 )   { find_parms.f_line = dl ; find_parms.f_offset = p1 ; }
				if ( find_parms.f_dir != 'A' || !found ) { break ; }
				find_parms.f_occurs++ ;
				if ( found1 ) { found1 = false ; find_parms.f_lines++ ; }
			}
		}
		if      ( find_parms.f_dir == 'A' ) { dl++ ; }
		else if ( find_parms.f_dir == 'F' ) { if ( found ) { find_parms.f_dir = 'N' ; break ; } ; dl++ ; }
		else if ( find_parms.f_dir == 'L' ) { if ( found ) { find_parms.f_dir = 'P' ; break ; } ; dl-- ; }
		else if ( find_parms.f_dir == 'N' ) { if ( found ) { break ; } ; dl++ ; }
		else if ( find_parms.f_dir == 'P' ) { if ( found ) { break ; } ; dl-- ; }
		if ( dl < 1 || dl > data.size()-2 ) { break ; }
	}
}

string PBRO01A::determineLang( string ZFILE )
{
	// Try to determine the language, first from the extension, then the directory containing the source,
	// then the contents of the file.
	// Limit the scan to the first 100 lines of code

	// Returned language must exist in eHilight (hiRoutine function map) or an exception will occur

	int p ;
	int i ;

	string s ;
	string t ;
	string w ;

	p = ZFILE.find_last_of( '.' ) ;
	if ( p != string::npos )
	{
		s = ZFILE.substr( p+1 ) ;
		if ( findword( s, "c cpp h hpp" ) ) { return "CPP"  ; }
		if ( findword( s, "rex rexx rx" ) ) { return "REXX" ; }
	}

	if ( ZFILE.find( "/rexx/" ) != string::npos ) { return "REXX" ; }
	if ( ZFILE.find( "/tmp/"  ) != string::npos ) { return "NONE" ; }

	for ( i = 1 ; i < data.size() && i < 100 ; i++ )
	{
		t = data.at( i ) ;
		if ( t.size() == 0 ) { continue       ; }
		if ( t[ 0 ] == '*' ) { return "ASM"   ; }
		if ( t[ 0 ] == ')' ) { return "PANEL" ; }
		w = word( t, 1 ) ;
		if ( findword( w, "TITLE CSECT DSECT MACRO START COPY" ) )
		{
			return "ASM"  ;
		}
		if ( i == 0 && t.find( "rexx" ) != string::npos )
		{
			return "REXX" ;
		}
		if ( w == "/*"    )  { return "CPP" ; }
	}
	return "DEFAULT" ;
}
// ============================================================================================ //

extern "C" { pApplication *maker() { return new PBRO01A ; } }
extern "C" { void destroy(pApplication *p) { delete p ; } }
