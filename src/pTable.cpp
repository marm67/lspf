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
#define MOD_NAME pTable
#define LOGOUT   aplog


void Table::loadRow( int& RC,
		     vector<string>& m_flds )
{
	RC = 0 ;

	if ( table.size() > 65535 )
	{
		RC = 20 ;
		log( "E", "Max size of 65,535 rows exceeded." <<endl ) ;
		return ;
	}
	m_flds.insert( m_flds.begin(), d2ds( ++maxURID ) ) ;
	table.push_back( m_flds ) ;
	changed = true ;
}


void Table::reserveSpace( int tot_rows )
{
	table.reserve( tot_rows ) ;
}


void Table::saveTable( int& RC,
		       const string& m_name,
		       const string& m_path )
{
	// Save table to disk.
	// Version 2 file format adds extension variable support and record/file end markers, 0xFF.

	string s ;
	int i    ;
	int j    ;
	int k    ;
	int size ;
	int evar ;
	std::ofstream otable ;

	RC = 0 ;

	if ( !changed )
	{
		RC = 4  ;
		log( "I", "No changes to the table have been made.  Ignoring save" <<endl ) ;
		return  ;
	}

	m_path != "" ? s = m_path : s = tab_path ;
	if ( s.back() != '/' ) { s = s + "/" ; }

	if ( exists( s ) )
	{
		if ( !is_directory( s ) )
		{
			log( "E", "Directory "+ s +" is not a regular directory " <<endl ) ;
			RC = 20 ;
			return  ;
		}
	}
	else
	{
		log( "E", "Directory "+ s +" does not exist for table save " <<endl ) ;
		RC = 16 ;
		return  ;
	}

	s += m_name ;
	if ( exists( s ) )
	{
		if ( !is_regular_file( s ) )
		{
			log( "E", "File "+ s +" is not a regular file " <<endl ) ;
			RC = 20 ;
			return  ;
		}
	}

	debug1( "Saving table "+ m_name +" in "+ s <<endl ) ;

	size = table.size() ;
	otable.open( s.c_str(), ios::binary | ios::out ) ;
	otable << (char)00  ;  //
	otable << (char)133 ;  // x085 denotes a table
	otable << (char)2   ;  // Table file format.  Version 2 (extension variable support)
	otable << (char)44  ;  // Header length
	otable << "HDR                                         " ;
	otable << (char)01  ;  // Number of fields following the header record (only the Sort Information Record for now)
	otable << (char)sort_ir.size() ;
	otable << sort_ir              ;
	otable << (char)( size >> 8 )  ;
	otable << (char)( size )       ;
	otable << (char)num_keys       ;
	otable << (char)num_flds       ;
	for ( j = 1 ; j <= num_all ; j ++ )
	{
		otable << (char)word( tab_all, j ).size() ;
		otable << word( tab_all, j ) ;
	}
	for ( i = 0 ; i < size ; i++ )
	{
		for ( j = 1 ; j <= num_all ; j++ )
		{
			k = table.at( i ).at( j ).size() ;
			otable << (char)( k >> 8 )        ;
			otable << (char)( k  )            ;
			otable << table.at( i ).at( j )   ;
		}
		evar = table.at( i ).size() - num_all - 1 ;
		otable << (char)( evar >> 8 ) ;
		otable << (char)( evar  )     ;
		for ( ; j < table.at( i ).size() ; j++ )
		{
			k = table.at( i ).at( j ).size() ;
			otable << (char)( k >> 8 )       ;
			otable << (char)( k  )           ;
			otable << table.at( i ).at( j )  ;
		}
		otable << (char)0xFF ;
	}
	otable << (char)0xFF ;
	otable.close() ;
	resetChanged() ;
}


void Table::tbadd( int& RC,
		   fPOOL& funcPOOL,
		   string tb_namelst,
		   string tb_order,
		   int tb_num_of_rows )
{
	// Add a row to a table after the CRP (if not sorted) or in the sort position if ORDER has been specified and there has been a previous tbsort()

	// RC = 0  Okay
	// RC = 8  Keyed tables only.  Row already exists, CRP set to TOP
	// RC = 16 Numeric conversion error
	// RC = 20 Severe error

	// Add extension variables as varname list + values to the flds vector after defined keys and fields (start posn @ 1+all_flds)

	// If ORDER specified on a sorted table, sort the table again after adding the record and reset CRP

	int    i    ;
	int    ws   ;

	string key  ;
	string val  ;
	string URID ;

	vector<string> keys ;
	vector<string> flds ;
	vector<vector<string>>::iterator it ;

	RC = 0 ;
	iupper( tb_namelst ) ;
	iupper( tb_order )   ;

	if ( table.size() > 65535 )
	{
		RC = 20 ;
		log( "E", "Max size of 65,535 rows exceeded." <<endl ) ;
		return ;
	}

	if ( tb_num_of_rows > 0 ) { table.reserve( tb_num_of_rows ) ; }

	if      ( tb_order == "" ) { sort_ir  = "" ; }
	else if ( sort_ir  == "" ) { tb_order = "" ; }

	if ( num_keys > 0 )
	{
		keys.clear()  ;
		keys.reserve( num_keys ) ;
		for ( i = 1 ; i <= num_keys ; i++ )
		{
			key = funcPOOL.get( RC, 8, word( tab_keys, i ) ) ;
			if ( RC == 8 )
			{
				funcPOOL.put( RC, 0, word( tab_keys, i ), "" ) ;
				key = "" ;
			}
			else if ( RC > 8 ) { RC = 20 ; return ; }
			keys.push_back( key ) ;
		}
		for ( it = table.begin() ; it != table.end() ; it++ )
		{
			for ( i = 1 ; i <= num_keys ; i++ )
			{
				if ( it->at( i ) != keys.at( i-1 ) )  { break ; }
			}
			if ( i > num_keys ) { RC = 8 ; return ; }
		}
	}

	flds.clear() ;
	flds.push_back( d2ds( ++maxURID ) ) ;
	for ( i = 1 ; i <= num_all ; i++ )
	{
		val = funcPOOL.get( RC, 8, word( tab_all, i ) ) ;
		if ( RC == 8 )
		{
			funcPOOL.put( RC, 0, word( tab_all, i ), "" ) ;
			val = "" ;
		}
		else if ( RC > 8 ) { RC = 20 ; return ; }
		flds.push_back( val ) ;
	}
	if ( tb_namelst != "" )
	{
		flds.push_back( space( tb_namelst ) ) ;
		for ( ws = words( tb_namelst ), i = 1 ; i <= ws ; i++ )
		{
			val = funcPOOL.get( RC, 8, word( tb_namelst, i ) ) ;
			if ( RC == 8 )
			{
				funcPOOL.put( RC, 0, word( tb_namelst, i ), "" ) ;
				val = "" ;
			}
			else if ( RC > 8 ) { RC = 20 ; return ; }
			flds.push_back( val ) ;
		}
	}

	if ( CRP == 0 )
	{
		table.insert( table.begin(), flds ) ;
		CRP = 1 ;
	}
	else
	{
		it = table.begin() ;
		advance( it, CRP ) ;
		table.insert( it, flds ) ;
		CRP++ ;
	}

	if ( tb_order == "ORDER" && sort_ir != "" )
	{
		URID = table[ CRP-1 ].at( 0 ) ;
		tbsort( RC, sort_ir ) ;
		for ( it = table.begin() ; it != table.end() ; it++ )
		{
			CRP++ ;
			if ( URID == it->at( 0 ) ) { break ; }
		}
	}
	changed = true ;
}


void Table::tbbottom( int& RC,
		      fPOOL& funcPOOL,
		      string tb_savenm,
		      string tb_rowid_vn,
		      string tb_noread,
		      string tb_crp_name )
{
	// RC = 0  Okay
	// RC = 8  Table Empty.  CRP set to top
	// RC = 20 Severe error

	// ROWID name and CRP name are for output only

	int    i      ;
	int    ws     ;
	string tbelst ;
	string var    ;
	string val    ;


	RC = 0 ;
	iupper( tb_savenm )   ;
	iupper( tb_rowid_vn ) ;
	iupper( tb_noread )   ;
	iupper( tb_crp_name ) ;

	if ( (tb_savenm   != "") && !isvalidName( tb_savenm   ) ) { RC = 20 ; return ; }
	if ( (tb_rowid_vn != "") && !isvalidName( tb_rowid_vn ) ) { RC = 20 ; return ; }
	if ( (tb_crp_name != "") && !isvalidName( tb_crp_name ) ) { RC = 20 ; return ; }

	if ( tb_noread != "" && tb_noread != "NOREAD" )
	{
		log( "E", "Invalid NOREAD parameter specified.  Invalid specification is "+ tb_noread <<endl ) ;
		RC = 20 ;
		return ;
	}

	if ( table.size() == 0 )
	{
		CRP = 0 ;
		RC  = 8 ;
		if ( tb_crp_name != "" ) { funcPOOL.put( RC, 0, tb_crp_name, CRP ) ; }
		return  ;
	}

	CRP = table.size() ;

	if ( tb_savenm != "" )
	{
		funcPOOL.put( RC, 0, tb_savenm, "" ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_noread != "NOREAD" )
	{
		for ( i = 1 ; i <= num_all ; i++ )
		{
			funcPOOL.put( RC, 0, word( tab_all, i ), table.at( CRP-1 ).at( i ) ) ;
			if ( RC > 0 ) { RC = 20 ; return ; }
		}
		if ( table.at( CRP-1 ).size() > num_all+1 )
		{
			tbelst = table.at( CRP-1 ).at( 1+num_all ) ;
			for ( ws = words( tbelst ), i = 1 ; i <= ws ; i++ )
			{
				var = word( tbelst, i ) ;
				val = table.at( CRP-1 ).at( 1+num_all+i ) ;
				funcPOOL.put( RC, 0, var, val ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
			if ( tb_savenm != "" )
			{
				funcPOOL.put( RC, 0, tb_savenm, "("+tbelst+")" ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
		}
	}
	if ( tb_rowid_vn != "" )
	{
		funcPOOL.put( RC, 0, tb_rowid_vn, table.at( CRP-1 ).at( 0 ) ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_crp_name != "" ) { funcPOOL.put( RC, 0, tb_crp_name, CRP ) ; }
}


void Table::tbdelete( int& RC,
		      fPOOL& funcPOOL )
{
	// Delete a row in the table.  For keyed tables, this is the row pointed to be the current contents of the key variable
	// For non-keyed tables, this is the row pointed to by the CRP

	// RC = 0  Okay
	// RC = 8  Keyed tables.  Row with key value does not exist.  CRP set to TOP
	//         Non-keyed tables.  CRP was at TOP (zero)
	// RC = 20 Severe error

	int i      ;
	int j      ;
	bool found ;
	string key ;

	vector<vector<string>>::iterator it ;
	vector<string> keys ;

	RC  = 0 ;
	if ( num_keys > 0 )
	{
		keys.clear() ;
		keys.reserve( num_keys ) ;
		for ( i = 1 ; i <= num_keys ; i++ )
		{
			key = funcPOOL.get( RC, 8, word( tab_keys, i ) ) ;
			if ( RC == 8 )
			{
				funcPOOL.put( RC, 0, word( tab_keys, i ), "" ) ;
				key = "" ;
			}
			else if ( RC > 8 ) { RC = 20 ; return ; }
			keys.push_back( key ) ;
		}
		found = false ;
		j     = 1     ;
		for ( it = table.begin() ; it != table.end() ; it++ )
		{
			for ( i = 1 ; i <= num_keys ; i++ )
			{
				if ( it->at( i ) != keys.at( i-1 ) ) { break  ; }
			}
			if ( i > num_keys ) { found = true ; break ; }
			j++ ;
		}
		if ( !found ) { CRP = 0 ; RC = 8 ; return ; }
		table.erase( it ) ;
		CRP = j - 1 ;
	}
	else
	{
		if ( CRP == 0 )
		{
			RC  = 8 ;
			return  ;
		}
		else
		{
			it = table.begin() ;
			advance( it, CRP-1 ) ;
			table.erase( it ) ;
			CRP-- ;
		}
	}
	changed = true ;
}


void Table::tbexist( int& RC,
		     fPOOL& funcPOOL )
{
	// Test for the existance of a row in a keyed table using the current value of the key variables.  CRP is positioned at this row if found else TOP

	// RC = 0  Okay
	// RC = 8  Keyed tables.  Row does not exist.  CRP is set to TOP (zero)
	//         Non-keyed tables.  Not valid.  CRP is set to TOP (zero)
	// RC = 20 Severe error

	bool   found ;
	int    i     ;
	int    j     ;
	string key   ;

	vector<vector<string>>::iterator it ;
	vector<string> keys ;

	RC  = 0 ;
	CRP = 0 ;

	if ( num_keys == 0 ) { RC = 8 ; return ; }

	keys.clear() ;
	keys.reserve( num_keys ) ;
	for ( i = 1 ; i <= num_keys ; i++ )
	{
		key = funcPOOL.get( RC, 8, word( tab_keys, i ) ) ;
		if ( RC == 8 )
		{
			funcPOOL.put( RC, 0, word( tab_keys, i ), "" ) ;
			key = "" ;
		}
		else if ( RC > 8 ) { RC = 20 ; return ; }
		keys.push_back( key ) ;
	}

	found = false ;
	j     = 1     ;
	for ( it = table.begin() ; it != table.end() ; it++ )
	{
		for ( i = 1 ; i <= num_keys ; i++ )
		{
			if ( it->at( i ) != keys.at( i-1 ) ) { break  ; }
		}
		if ( i > num_keys ) { found = true ; break ; }
		j++ ;
	}
	if ( !found ) { RC = 8 ; return ; }
	CRP = j ;
}


void Table::fillfVARs( int& RC,
		       fPOOL& funcPOOL,
		       int depth,
		       int posn )
{
	//  Fill the function pool variables ( of the form table_fieldname.line ) from the table for depth lines
	//  Also create function pool variable .ZURID.line to hold the URID of the table row corresponding to that screen line

	int j    ;
	int k    ;
	int size ;

	vector<vector<string>>::iterator it ;

	RC   = 0 ;
	size = table.size() ;

	funcPOOL.put( RC, 0, "ZTDTOP", posn )  ;
	funcPOOL.put( RC, 0, "ZTDROWS", size ) ;

	it = table.begin() ;
	advance( it, posn-1 ) ;

	for ( j = 0 ; j < depth ; j++ )
	{
		if ( j + posn > size ) { break ; }
		funcPOOL.put( RC, 0, ".ZURID." + d2ds( j ), it->at( 0 ), NOCHECK ) ;
		for ( k = 1 ; k <= num_all ; k++ )
		{
			funcPOOL.put( RC, 0, word( tab_all, k ) + "." + d2ds( j ), it->at( k ), NOCHECK ) ;
		}
		it++;
	}
	for ( ; j < depth ; j++ )
	{
		funcPOOL.put( RC, 0, ".ZURID." + d2ds( j ), "", NOCHECK ) ;
		for ( k = 1 ; k <= num_all ; k++ )
		{
			funcPOOL.put( RC, 0, word( tab_all, k ) + "." + d2ds( j ), "", NOCHECK ) ;
		}
		it++;
	}
}


void Table::tbget( int& RC,
		   fPOOL& funcPOOL,
		   string tb_savenm,
		   string tb_rowid_vn,
		   string tb_noread,
		   string tb_crp_name )
{
	// Access row in the table.  For table with keys, use the current value of the key in the dialogue variable.  For non-keyed tables, use the CRP

	// RC = 0  Okay
	// RC = 8  CRP was at TOP(zero) for non-keyed tables or key not found for keyed tables
	// RC = 20 Severe error

	// ROWID name and CRP name are for output only (not used for finding the record)

	int    i      ;
	int    ws     ;
	bool   found  ;
	string key    ;
	string tbelst ;
	string var    ;
	string val    ;

	vector<vector<string>>::iterator it ;
	vector<string> keys ;

	RC = 0 ;
	iupper( tb_savenm )   ;
	iupper( tb_rowid_vn ) ;
	iupper( tb_noread )   ;
	iupper( tb_crp_name ) ;

	if ( (tb_savenm   != "") && !isvalidName( tb_savenm   ) ) { RC = 20 ; return ; }
	if ( (tb_rowid_vn != "") && !isvalidName( tb_rowid_vn ) ) { RC = 20 ; return ; }
	if ( (tb_crp_name != "") && !isvalidName( tb_crp_name ) ) { RC = 20 ; return ; }

	if ( tb_noread != "" && tb_noread != "NOREAD" )
	{
		log( "E", "Invalid NOREAD parameter specified.  Invalid specification is "+ tb_noread <<endl ) ;
		RC = 20 ;
		return ;
	}

	if ( num_keys > 0 )
	{
		keys.clear() ;
		keys.reserve( num_keys ) ;
		for ( i = 1 ; i <= num_keys ; i++ )
		{
			key = funcPOOL.get( RC, 8, word( tab_keys, i ) ) ;
			if ( RC == 8 )
			{
				funcPOOL.put( RC, 0, word( tab_keys, i ), "" ) ;
				key = "" ;
			}
			else if ( RC > 8 ) { RC = 20 ; return ; }
			keys.push_back( key ) ;
		}
		found = false ;
		CRP   = 1     ;
		for ( it = table.begin() ; it != table.end() ; it++ )
		{
			for ( i = 1 ; i <= num_keys ; i++ )
			{
				if ( it->at( i ) != keys.at( i-1 ) )  { break ; }
			}
			if ( i > num_keys ) { found = true ; break ; }
			CRP++ ;
		}
		if ( !found ) { CRP = 0 ; }
	}
	if ( CRP == 0 )
	{
		if ( tb_crp_name != "" ) { funcPOOL.put( RC, 0, tb_crp_name, CRP ) ; }
		RC = 8 ;
		return ;
	}

	if ( tb_savenm != "" )
	{
		funcPOOL.put( RC, 0, tb_savenm, "" ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_noread != "NOREAD" )
	{
		for ( i = 1 ; i <= num_all ; i++ )
		{
			funcPOOL.put( RC, 0, word( tab_all, i ), table.at( CRP-1 ).at( i ) ) ;
			if ( RC > 0 ) { RC = 20 ; return ; }
		}
		if ( table.at( CRP-1 ).size() > num_all+1 )
		{
			tbelst = table.at( CRP-1 ).at( 1+num_all ) ;
			for ( ws = words( tbelst ), i = 1 ; i <= ws ; i++ )
			{
				var = word( tbelst, i ) ;
				val = table.at( CRP-1 ).at( 1+num_all+i ) ;
				funcPOOL.put( RC, 0, var, val ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
			if ( tb_savenm != "" )
			{
				funcPOOL.put( RC, 0, tb_savenm, "("+tbelst+")" ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
		}
	}
	if ( tb_rowid_vn != "" )
	{
		funcPOOL.put( RC, 0, tb_rowid_vn, table.at( CRP-1 ).at( 0 ) ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_crp_name != "" ) { funcPOOL.put( RC, 0, tb_crp_name, CRP ) ; }
}


void Table::tbmod( int& RC,
		   fPOOL& funcPOOL,
		   string tb_namelst,
		   string tb_order )
{
	// tbmod - update row if match found on key (keyed tables),  else perform tbadd to the bottom of the table
	//         Non-keyed tables same as a tbadd
	//         CRP always points to the row added/updated

	// RC = 0  Okay.  Keyed tables - row updated.  Non-keyed tables new row added
	// RC = 8  Row did not match - row added for keyed tables
	// RC = 16 Numeric conversion error
	// RC = 20 Severe error

	// Extension variables must be re-specified or they will be lost (tb_namelst)

	// If ORDER specified on a sorted table, sort again in case tbmod has changed the order and reset CRP

	int  i      ;
	int  ws     ;
	bool found  ;
	string key  ;
	string val  ;
	string URID ;

	vector<string> flds ;
	vector<string> keys ;
	vector<vector<string>>::iterator it ;

	RC = 0 ;

	if ( sort_ir == "" ) { tb_order = "" ; }

	iupper( tb_namelst ) ;
	iupper( tb_order )   ;

	if ( num_keys == 0 ) { tbadd( RC, funcPOOL, tb_namelst, tb_order, 0 ) ; return ; }

	CRP   = 1     ;
	found = false ;

	keys.clear()  ;
	keys.reserve( num_keys ) ;
	for ( i = 1 ; i <= num_keys ; i++ )
	{
		key = funcPOOL.get( RC, 8, word( tab_keys, i ) ) ;
		if ( RC == 8 )
		{
			funcPOOL.put( RC, 0, word( tab_keys, i ), "" ) ;
			key = "" ;
		}
		else if ( RC > 8 ) { RC = 20 ; return ; }
		keys.push_back( key ) ;
	}
	for ( it = table.begin() ; it != table.end() ; it++ )
	{
		for ( i = 1 ; i <= num_keys ; i++ )
		{
			if ( it->at( i ) != keys.at( i-1 ) ) { break ; }
		}
		if ( i > num_keys ) { found = true ; break ; }
		CRP++ ;
	}

	if ( !found )
	{
		CRP = table.size() ;
		tbadd( RC, funcPOOL, tb_namelst, tb_order, 0 ) ;
		if ( RC == 0 ) { RC = 8 ; }
	}
	else
	{
		for ( i = 1 ; i <= num_all ; i++ )
		{
			val = funcPOOL.get( RC, 8, word( tab_all, i ) ) ;
			if ( RC == 8 )
			{
				funcPOOL.put( RC, 0, word( tab_all, i ), "" ) ;
				val = "" ;
			}
			else if ( RC > 8 ) { RC = 20 ; return ; }
			flds.push_back( val ) ;
		}
		if ( tb_namelst != "" )
		{
			flds.push_back( space( tb_namelst ) ) ;
			for ( ws = words( tb_namelst ), i = 1 ; i <= ws ; i++ )
			{
				val = funcPOOL.get( RC, 8, word( tb_namelst, i ) ) ;
				if ( RC == 8 )
				{
					funcPOOL.put( RC, 0, word( tb_namelst, i ), "" ) ;
					val = "" ;
				}
				else if ( RC > 8 ) { RC = 20 ; return ; }
				flds.push_back( val ) ;
			}
		}
		URID = table[ CRP-1 ].at( 0 ) ;
		flds.insert( flds.begin(), URID ) ;
		table[ CRP-1 ] = flds ;
		if ( tb_order == "ORDER" && sort_ir != "" )
		{
			tbsort( RC, sort_ir ) ;
			for ( it = table.begin() ; it != table.end() ; it++ )
			{
				CRP++ ;
				if ( URID == it->at( 0 ) ) { break ; }
			}
		}
	}
	changed = true ;
}


void Table::tbput( int& RC,
		   fPOOL& funcPOOL,
		   string tb_namelst,
		   string tb_order )
{
	// Update the current row in a table using the CRP.  For keyed tables, key variables must match row at CRP

	// RC = 0   OK
	// RC = 8   Keyed tables - keys do not match current row.  CRP set to top (0)
	//          Non-keyed tables - CRP was at top
	// RC = 16  Numeric conversion error for sorted tables
	// RC = 20  Severe error

	// If ORDER specified on a sorted table, sort again in case tbput has changed the order and reset CRP

	int  i  ;
	int  ws ;

	string key  ;
	string val  ;
	string URID ;

	vector<string> flds ;
	vector<vector<string>>::iterator it ;

	RC = 0 ;

	if ( sort_ir == "" ) { tb_order = "" ; }

	iupper( tb_namelst ) ;
	iupper( tb_order )   ;

	if ( CRP == 0 ) { RC = 8 ; return ; }

	for ( i = 1 ; i <= num_keys ; i++ )
	{
		key = funcPOOL.get( RC, 8, word( tab_keys, i ) ) ;
		if ( RC == 8 )
		{
			funcPOOL.put( RC, 0, word( tab_keys, i ), "" ) ;
			key = "" ;
		}
		else if ( RC > 8 ) { RC = 20 ; return ; }
		if ( key != table[ CRP-1 ].at( i ) ) { CRP = 0 ; RC = 8 ; return ; }
		flds.push_back( key ) ;
	}
	for ( i = 1 ; i <= num_flds ; i++ )
	{
		val = funcPOOL.get( RC, 8, word( tab_flds, i ) ) ;
		if ( RC == 8 )
		{
			funcPOOL.put( RC, 0, word( tab_flds, i ), "" ) ;
			val = "" ;
		}
		else if ( RC > 8 ) { RC = 20 ; return ; }
		flds.push_back( val ) ;
	}
	if ( tb_namelst != "" )
	{
		flds.push_back( space( tb_namelst ) ) ;
		for ( ws = words( tb_namelst ), i = 1 ; i <= ws ; i++ )
		{
			val = funcPOOL.get( RC, 8, word( tb_namelst, i ) ) ;
			if ( RC == 8 )
			{
				funcPOOL.put( RC, 0, word( tb_namelst, i ), "" ) ;
				val = "" ;
			}
			else if ( RC > 8 ) { RC = 20 ; return ; }
			flds.push_back( val ) ;
		}
	}

	URID = table[ CRP-1 ].at( 0 ) ;
	flds.insert( flds.begin(), URID ) ;
	table[ CRP-1 ] = flds   ;

	if ( tb_order == "ORDER" && sort_ir != "" )
	{
		tbsort( RC, sort_ir ) ;
		for ( it = table.begin() ; it != table.end() ; it++ )
		{
			CRP++ ;
			if ( URID == it->at( 0 ) ) { break ; }
		}
	}
	changed = true ;
}


void Table::tbquery( int& RC,
		     fPOOL& funcPOOL,
		     string tb_keyn,
		     string tb_varn,
		     string tb_rownn,
		     string tb_keynn,
		     string tb_namenn,
		     string tb_crpn,
		     string tb_sirn,
		     string tb_lstn,
		     string tb_condn,
		     string tb_dirn )
{
	RC = 0 ;
	iupper( tb_keyn )   ;
	iupper( tb_varn )   ;
	iupper( tb_rownn )  ;
	iupper( tb_keynn )  ;
	iupper( tb_namenn ) ;
	iupper( tb_crpn )   ;
	iupper( tb_sirn )   ;
	iupper( tb_lstn )   ;
	iupper( tb_condn )  ;
	iupper( tb_dirn )   ;

	if ( tb_keyn != "" )  { funcPOOL.put( RC, 0, tb_keyn, tab_keys )  ; }
	if ( RC > 0 ) { return ; }
	if ( tb_varn != "" )  { funcPOOL.put( RC, 0, tb_varn, tab_flds )  ; }
	if ( RC > 0 ) { return ; }
	if ( tb_rownn != "" ) { funcPOOL.put( RC, 0, tb_rownn, table.size() ) ; }
	if ( RC > 0 ) { return ; }
	if ( tb_keynn != "" ) { funcPOOL.put( RC, 0, tb_keynn, num_keys ) ; }
	if ( RC > 0 ) { return ; }
	if ( tb_namenn != "" ) { funcPOOL.put( RC, 0, tb_namenn, num_flds ) ; }
	if ( RC > 0 ) { return ; }
	if ( tb_crpn != "" ) { funcPOOL.put( RC, 0, tb_crpn, CRP ) ; }
	if ( RC > 0 ) { return ; }
	if ( tb_sirn != "" ) { funcPOOL.put( RC, 0, tb_sirn, sort_ir ) ; }
	if ( RC > 0 ) { return ; }
	if ( tb_lstn != "" ) { funcPOOL.put( RC, 0, tb_lstn, sa_namelst ) ; }
	if ( RC > 0 ) { return ; }
	if ( tb_condn != "" ) { funcPOOL.put( RC, 0, tb_condn, sa_cond_pairs ) ; }
	if ( RC > 0 ) { return ; }
	if ( tb_dirn != "" ) { funcPOOL.put( RC, 0, tb_dirn, sa_dir ) ; }
	if ( RC > 0 ) { return ; }
}


void Table::tbsarg( int& RC,
		    fPOOL& funcPOOL,
		    string tb_namelst,
		    string tb_dir,
		    string tb_cond_pairs )
{
	// Notes:  Current value of variables are used if not blank.  Condition is EQ
	// ARGLIST(namelst) is for extension variables, the values of which will be used in the search

	int  p1 ;
	int  p2 ;
	int  i  ;
	int  ws ;
	bool loope  ;
	string var  ;
	string val  ;
	string cond ;

	tbsearch t  ;

	RC = 0 ;
	iupper( tb_namelst ) ;
	iupper( tb_dir )     ;
	iupper( trim( tb_cond_pairs ) ) ;

	if ( tb_cond_pairs != "" )
	{
		if ( tb_cond_pairs[ 0 ] != '(' || tb_cond_pairs.back() != ')' )
		{
			log( "E", "Invalid COND PAIRS specified. "+ tb_cond_pairs <<endl ) ;
			RC = 20 ;
			return ;
		}
		tb_cond_pairs = tb_cond_pairs.substr( 1, tb_cond_pairs.size()-2 ) ;
	}

	sarg.clear() ;

	if ( tb_dir == "" ) { tb_dir = "NEXT" ; }

	if ( (tb_dir != "NEXT") && (tb_dir != "PREVIOUS") )
	{
		log( "E", "Search direction on TBSARG must be NEXT or PREVIOUS.  Invalid specification is "+ tb_dir <<endl ) ;
		RC = 20 ;
		return  ;
	}

	for ( i = 1 ; i <= num_all ; i++ )
	{
		var = word( tab_all, i ) ;
		val = funcPOOL.get( RC, 8, var ) ;
		if ( RC > 8 ) { RC = 20 ; return ; }
		if ( val == "" ) { continue ; }
		if ( val.back() == '*' ) { t.tbs_gen = true  ; t.tbs_vsize = val.size()-1 ; t.tbs_val = val.substr( 0, t.tbs_vsize ) ; }
		else                     { t.tbs_gen = false ; t.tbs_val = val  ;                                                      }
		t.tbs_cond  = s_EQ  ;
		t.tbs_ext   = false ;
		sarg[ var ] = t     ;
	}

	for ( ws = words( tb_namelst ), i = 1 ; i <= ws ; i++ )
	{
		var = word( tb_namelst, i ) ;
		if ( wordpos( var, tab_all ) > 0 )
		{
			log( "E", "Extension variables cannot have the same name as key or field names" <<endl ) ;
			RC = 20 ;
			return  ;
		}
		val = funcPOOL.get( RC, 8, var ) ;
		if ( RC == 8 )
		{
			funcPOOL.put( RC, 0, var, "" ) ;
			val = "" ;
		}
		else if ( RC > 8 ) { RC = 20 ; return ; }
		if ( val.back() == '*' ) { t.tbs_gen = true  ; t.tbs_vsize = val.size()-1 ; t.tbs_val = val.substr( 0, t.tbs_vsize ) ; }
		else                     { t.tbs_gen = false ; t.tbs_val = val  ;                                                      }
		t.tbs_cond  = s_EQ ;
		t.tbs_ext   = true ;
		sarg[ var ] = t    ;
	}

	p2    = 0     ;
	loope = false ;
	while ( true )
	{
		if ( tb_cond_pairs == "" ) { break ; }
		p1 = tb_cond_pairs.find( ',', p2 ) ;
		if ( p1 == string::npos )
		{
			log( "E", "Search pairs must be of the form NAME,COND,NAME,COND...." <<endl ) ;
			RC = 20 ;
			return  ;
		}
		var = tb_cond_pairs.substr( p2, p1-p2 ) ;
		if ( wordpos( var, (tab_all + " " + tb_namelst ) ) == 0 )
		{
			log( "E", "Invalid NAME specifed on TBSARG request.  Must be a key, field or extension variable name.  Invalid specification is "+ var <<endl ) ;
			RC = 20 ;
			return  ;
		}
		p1++ ;
		p2 = tb_cond_pairs.find( ',', p1 ) ;
		if ( p2 == string::npos )
		{
			cond  = tb_cond_pairs.substr( p1 ) ;
			loope = true ;
		}
		else
		{
			cond = tb_cond_pairs.substr( p1, p2-p1 ) ;
		}
		p2++ ;
		if ( (sarg.find( var ) != sarg.end()) && !sarg[ var ].tbsSetcon( cond ) )
		{
			log( "E", "Invalid condition.  Has to be EQ, NE, LE, LT, GE or GT. Invalid specification is '"+ cond +"'" <<endl ) ;
			RC = 20 ;
			return  ;
		}
		if ( loope ) { break ; }
	}

	sa_dir        = tb_dir ;
	sa_namelst    = tb_namelst    ;
	sa_cond_pairs = tb_cond_pairs ;
}


void Table::tbscan( int& RC,
		    fPOOL& funcPOOL,
		    string tb_namelst,
		    string tb_savenm,
		    string tb_rowid_vn,
		    string tb_dir,
		    string tb_noread,
		    string tb_crp_name,
		    string tb_condlst )
{
	// Scan table from current CRP according to parameters tb_namelst/tb_condlst/tb_dir if specified or similar values from a previous tbsarg call
	// tb_namelst contains key, name and extenstion variables.
	// Scan on extension variables not currently supported.  Only one search argument/condition currently supported

	// RC = 0  Okay. Row found
	// RC = 8  Row not found

	// Current value of variables not in NAMELST make no difference to search
	// SAVENM is blank if there are no extension variables or if NOREAD is specified.  Not specifying var leaves it unchanged from before (obviously)
	// NULL values are not ignored in namelst.

	int  i   ;
	int p1   ;
	int ws   ;
	int size ;

	string val     ;
	string var     ;
	string tbelst  ;
	string s_dir   ;
	int    s_match ;
	bool   s_next  ;
	bool   found   ;
	bool   loope   ;

	tbsearch t     ;

	map< string, tbsearch > scan ;
	map< string, tbsearch >::iterator it ;

	RC = 0 ;
	iupper( tb_namelst )  ;
	iupper( tb_savenm )   ;
	iupper( tb_rowid_vn ) ;
	iupper( tb_dir )      ;
	iupper( tb_noread )   ;
	iupper( tb_crp_name ) ;

	if ( tb_dir == "" ) { tb_dir = "NEXT" ; }

	if ( (tb_dir != "NEXT") && (tb_dir != "PREVIOUS") )
	{
		log( "E", "Search direction must be NEXT (default) or PREVIOUS" <<endl ) ;
		RC = 20 ;
		return  ;
	}

	if ( tb_noread != "" && tb_noread != "NOREAD" )
	{
		log( "E", "Only NOREAD should be specifed if variables are not to be read.  Invalid specification is "+ tb_noread <<endl ) ;
		RC = 20 ;
		return  ;
	}

	if ( tb_savenm   != "" && !isvalidName( tb_savenm   ) ) { RC = 20 ; return ; }
	if ( tb_rowid_vn != "" && !isvalidName( tb_rowid_vn ) ) { RC = 20 ; return ; }
	if ( tb_crp_name != "" && !isvalidName( tb_crp_name ) ) { RC = 20 ; return ; }

	if ( tb_namelst == "" )
	{
		if ( sarg.size() == 0 )
		{
			log( "E", "TBSCAN performed with no name list, but no search arguments set by TBSARG" <<endl ) ;
			RC = 20 ;
			return  ;
		}
		scan  = sarg   ;
		s_dir = sa_dir ;
	}
	else
	{
		if ( words( tb_condlst ) > words( tb_namelst ) )
		{
			log( "E", "CONDLIST parameter has more conditions that names in the NAMELIST parameter (less defaults to EQ)" <<endl ) ;
			RC = 20 ;
			return  ;
		}
		for ( ws = words( tb_namelst ), i = 1 ; i <= ws ; i++ )
		{
			var = word( tb_namelst, i ) ;
			val = funcPOOL.get( RC, 8, var ) ;
			if ( RC == 8 )
			{
				funcPOOL.put( RC, 0, var, "" ) ;
				val = "" ;
			}
			else if ( RC > 8 ) { RC = 20 ; return ; }
			t.tbs_ext = false ;
			if ( val.back() == '*' ) { t.tbs_gen = true  ; t.tbs_vsize = val.size()-1 ; t.tbs_val = val.substr( 0, t.tbs_vsize ) ; }
			else                     { t.tbs_gen = false ; t.tbs_val = val  ;                                                      }
			scan[ var ] = t ;
			if ( !scan[ var ].tbsSetcon( word( tb_condlst, i )) )
			{
				log( "E", "Invalid condition.  Has to be EQ, NE, LE, LT, GE or GT" <<endl ) ;
				RC = 20 ;
				return  ;
			}
		}
		s_dir = tb_dir ;
	}

	found   = false ;
	s_next  = ( s_dir == "NEXT" ) ;
	size    = table.size()        ;

	while ( true )
	{
		if ( s_next ) { CRP++ ; if ( CRP > size ) break ; }
		else          { CRP-- ; if ( CRP < 1    ) break ; }
		s_match = 0     ;
		loope   = false ;
		for ( it = scan.begin() ; it != scan.end() ; it++ )
		{
			p1 = wordpos( it->first, tab_all ) ;
			if ( p1 == 0 )
			{
				if ( table.at( CRP-1 ).size() == num_all+1 ) { break ; }
				tbelst = table.at( CRP-1 ).at( 1+num_all ) ;
				p1 = wordpos( it->first, tbelst ) ;
				if ( p1 == 0 ) { break ; }
				p1 = 1 + num_all + p1 ;
			}
			if ( it->second.tbs_gen )
			{
				switch ( it->second.tbs_cond )
				{
				case s_EQ:
					if ( table.at( CRP-1 ).at( p1 ).substr( 0 , it->second.tbs_vsize ) != it->second.tbs_val ) { loope = true ; }
					break ;
				case s_NE:
					if ( table.at( CRP-1 ).at( p1 ).substr( 0 , it->second.tbs_vsize ) == it->second.tbs_val ) { loope = true ; }
					break ;
				case s_LE:
					if ( table.at( CRP-1 ).at( p1 ).substr( 0 , it->second.tbs_vsize )  > it->second.tbs_val ) { loope = true ; }
					break ;
				case s_LT:
					if ( table.at( CRP-1 ).at( p1 ).substr( 0 , it->second.tbs_vsize ) >= it->second.tbs_val ) { loope = true ; }
					break ;
				case s_GE:
					if ( table.at( CRP-1 ).at( p1 ).substr( 0 , it->second.tbs_vsize )  < it->second.tbs_val ) { loope = true ; }
					break ;
				case s_GT:
					if ( table.at( CRP-1 ).at( p1 ).substr( 0 , it->second.tbs_vsize ) <=  it->second.tbs_val ) { loope = true ; }
				}
			}
			else
			{
				switch ( it->second.tbs_cond )
				{
				case s_EQ:
					if ( table.at( CRP-1 ).at( p1 ) != it->second.tbs_val ) { loope = true ; }
					break ;
				case s_NE:
					if ( table.at( CRP-1 ).at( p1 ) == it->second.tbs_val ) { loope = true ; }
					break ;
				case s_LE:
					if ( table.at( CRP-1 ).at( p1 )  > it->second.tbs_val ) { loope = true ; }
					break ;
				case s_LT:
					if ( table.at( CRP-1 ).at( p1 ) >= it->second.tbs_val ) { loope = true ; }
					break ;
				case s_GE:
					if ( table.at( CRP-1 ).at( p1 )  < it->second.tbs_val ) { loope = true ; }
					break ;
				case s_GT:
					if ( table.at( CRP-1 ).at( p1 ) <= it->second.tbs_val ) { loope = true ; }
				}
			}
			if ( loope ) { break ; }
			s_match++ ;
		}
		if ( s_match == scan.size() ) { found = true ; break ; }
	}
	if ( !found )
	{
		CRP = 0 ;
		if ( tb_crp_name != "" ) { funcPOOL.put( RC, 0, tb_crp_name, CRP ) ; }
		if ( RC > 0 ) { RC = 20 ; return ; }
		RC  = 8 ;
		return  ;
	}

	if ( tb_savenm != "" )
	{
		funcPOOL.put( RC, 0, tb_savenm, "" ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_noread != "NOREAD" )
	{
		for ( i = 1 ; i <= num_all ; i++ )
		{
			funcPOOL.put( RC, 0, word( tab_all, i ), table.at( CRP-1 ).at( i ) ) ;
			if ( RC > 0 ) { RC = 20 ; return ; }
		}
		if ( table.at( CRP-1 ).size() > num_all+1 )
		{
			tbelst = table.at( CRP-1 ).at( 1+num_all ) ;
			for ( ws = words( tbelst ), i = 1 ; i <= ws ; i++ )
			{
				var = word( tbelst, i ) ;
				val = table.at( CRP-1 ).at( 1+num_all+i ) ;
				funcPOOL.put( RC, 0, var, val ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
			if ( tb_savenm != "" )
			{
				funcPOOL.put( RC, 0, tb_savenm, "("+tbelst+")" ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
		}
	}
	if ( tb_crp_name != "" )
	{
		funcPOOL.put( RC, 0, tb_crp_name, CRP ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_rowid_vn != "" )
	{
		funcPOOL.put( RC, 0, tb_rowid_vn, table.at( CRP-1 ).at( 0 ) ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
}


void Table::cmdsearch( int& RC,
		       fPOOL& funcPOOL,
		       const string& cmd )
{
	// cmdsearch is not part of the normal table services for application.
	// It's used for retrieving abbreviated commands from a command table.
	// Use tbsarg/tbscan for normal applications

	// RC = 0  Okay
	// RC = 8  Commmand not found
	// RC = 20 Severe error

	int  trunc ;
	bool found ;

	vector<vector<string>>::iterator it ;

	RC = 0 ;

	if ( !tab_cmds )
	{
		log( "E", "cmdsearch issued for a non-command table." <<endl ) ;
		RC = 20 ;
		return  ;
	}

	found = false ;
	for ( it = table.begin() ; it != table.end() ; it++ )
	{
		trunc = ds2d( it->at( 2 ) ) ;
		if ( trunc == 0 )
		{
			if ( it->at( 1 ) == cmd )
			{
				found = true ;
				break ;
			}
		}
		else
		{
			if ( abbrev( it->at( 1 ), cmd, trunc ) )
			{
				found = true ;
				break ;
			}
		}
	}

	if ( !found ) { RC = 8 ; return ; }

	funcPOOL.put( RC, 0, "ZCTVERB", it->at( 1 ) ) ;
	if ( RC > 0 ) { RC = 20 ; return ; }
	funcPOOL.put( RC, 0, "ZCTTRUNC", it->at( 2 ) ) ;
	if ( RC > 0 ) { RC = 20 ; return ; }
	funcPOOL.put( RC, 0, "ZCTACT", it->at( 3 ) ) ;
	if ( RC > 0 ) { RC = 20 ; return ; }
	funcPOOL.put( RC, 0, "ZCTDESC", it->at( 4 ) ) ;
	if ( RC > 0 ) { RC = 20 ; return ; }
}


void Table::tbskip( int& RC,
		    fPOOL& funcPOOL,
		    int num,
		    string tb_savenm,
		    string tb_rowid_vn,
		    string tb_rowid,
		    string tb_noread,
		    string tb_crp_name )
{
	// Move CRP to a position in the table and read the row into the dialogue variables
	// Position using tb_rowid (URID) if specified, else use num

	// RC = 0  Okay
	// RC = 8  CRP would be outside the table
	// RC = 12 Table not open
	// RC = 16 Truncation has occured
	// RC = 20 Severe error

	int i         ;
	int ws        ;
	string val    ;
	string var    ;
	string tbelst ;
	bool found    ;

	vector<vector<string>>::iterator it ;

	RC = 0 ;
	iupper( tb_savenm )   ;
	iupper( tb_rowid_vn ) ;
	iupper( tb_noread )   ;
	iupper( tb_crp_name ) ;

	if ( (tb_savenm   != "") && !isvalidName( tb_savenm   ) ) { RC = 20 ; return ; }
	if ( (tb_rowid_vn != "") && !isvalidName( tb_rowid_vn ) ) { RC = 20 ; return ; }
	if ( (tb_crp_name != "") && !isvalidName( tb_crp_name ) ) { RC = 20 ; return ; }

	if ( tb_noread != "" && tb_noread != "NOREAD" )
	{
		log( "E", "Invalid NOREAD parameter specified.  Invalid specification is "+ tb_noread <<endl ) ;
		RC = 20 ;
		return ;
	}

	if ( tb_rowid != "" )
	{
		CRP   = 1     ;
		found = false ;
		for ( it = table.begin() ; it != table.end() ; it++ )
		{
			if ( tb_rowid == it->at( 0 ) ) { found = true ; break ; }
			CRP++ ;
		}
		if ( !found ) { CRP = 0 ; RC  = 8 ; return  ; }
	}
	else
	{
		i = CRP + num  ;
		if ( ( i < 1 ) || ( i > table.size() ) ) { RC = 8 ; return ; }
		CRP = i ;
	}

	if ( tb_savenm != "" )
	{
		funcPOOL.put( RC, 0, tb_savenm, "" ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_noread != "NOREAD" )
	{
		for ( i = 1 ; i <= num_all ; i++ )
		{
			funcPOOL.put( RC, 0, word( tab_all, i ), table[ CRP-1 ].at( i ) ) ;
			if ( RC > 0 ) { RC = 20 ; return ; }
		}
		if ( table.at( CRP-1 ).size() > num_all+1 )
		{
			tbelst = table.at( CRP-1 ).at( 1+num_all ) ;
			for ( ws = words( tbelst ), i = 1 ; i <= ws ; i++ )
			{
				var = word( tbelst, i ) ;
				val = table.at( CRP-1 ).at( 1+num_all+i ) ;
				funcPOOL.put( RC, 0, var, val ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
			if ( tb_savenm != "" )
			{
				funcPOOL.put( RC, 0, tb_savenm, "("+tbelst+")" ) ;
				if ( RC > 0 ) { RC = 20 ; return ; }
			}
		}
	}
	if ( tb_rowid_vn != "" )
	{
		funcPOOL.put( RC, 0, tb_rowid_vn, table.at( CRP-1 ).at( 0 ) ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
	if ( tb_crp_name != "" ) { funcPOOL.put( RC, 0, tb_crp_name, CRP ) ; }
}


void Table::tbsort( int& RC,
		    string tb_fields )
{
	// FIELD
	// FIELD,C
	// FIELD,C,A,FIELD2
	// FIELD,C,A,FIELD2,N
	// FIELD,C,A,FIELD2,N,D

	int f1 ;
	int p1 ;

	string s_fields ;
	string s_temp   ;

	vector<string> s_parm ;
	vector<int> s_field   ;
	vector<bool> s_char   ;
	vector<bool> s_asc    ;

	int nsort ;

	string temp ;

	iupper( tb_fields ) ;
	temp      = tb_fields          ;

	while ( true )
	{
		p1 = tb_fields.find( ',' ) ;
		if ( p1 == string::npos )
		{
			s_parm.push_back( trim( tb_fields ) ) ;
			break ;
		}
		else
		{
			s_parm.push_back( strip( tb_fields.substr( 0, p1 ) ) ) ;
			tb_fields.erase( 0, p1+1 ) ;
		}
	}

	s_fields = "" ;
	while ( true )
	{
		if ( s_parm.size() == 0 ) { break ; }
		s_temp   = s_parm[ 0 ] ;
		s_fields = s_temp + " " + s_fields ;
		f1  = wordpos( s_temp, tab_all ) ;
		if ( f1 == 0 ) { RC = 20 ; return ; }
		s_field.push_back( f1 ) ;
		if ( s_parm.size() == 1 ) { break ; }
		s_parm.erase( s_parm.begin() ) ;

		s_temp = s_parm[ 0 ] ;
		if ( s_temp != "C" && s_temp != "N" ) { RC = 20 ; return ; }
		s_char.push_back( (s_temp == "C") ) ;
		if ( s_parm.size() == 1 ) { break ; }
		s_parm.erase( s_parm.begin() ) ;

		s_temp = s_parm[ 0 ] ;
		if ( s_temp != "A" && s_temp != "D" ) { RC = 20 ; return ; }
		s_asc.push_back( (s_temp == "A") ) ;
		s_parm.erase( s_parm.begin() ) ;
	}

	s_char.push_back( true ) ;
	s_asc.push_back( true )  ;

	nsort   = s_field.size() ;
	sort_ir = temp ;

	sort( table.begin(), table.end(),
		[ &s_field, &s_char, &s_asc, nsort ]( const vector<string>& a, const vector<string>& b )
		{
			for ( int i = 0 ; i < nsort ; i++ )
			{
				int j = s_field[ i ] ;
				if ( s_char[ i ] )
				{
					 if ( a[ j ] ==  b[ j ] ) { continue ; }
					 if ( s_asc[ i ] )
					 {
						 return a[ j ] < b[ j ] ;
					 }
					 else
					 {
						 return a[ j ] > b[ j ] ;
					 }
				}
				else
				{
					 int ia = ds2d( a[ j ] ) ;
					 int ib = ds2d( b[ j ] ) ;
					 if ( ia == ib ) { continue ; }
					 if ( s_asc[ i ] )
					 {
						 return ia < ib ;
					 }
					 else
					 {
						 return ia > ib ;
					 }
				}
			}
			return false ;
		} ) ;
	CRP = 0 ;
}


void Table::tbtop( int& RC )
{
	RC  = 0 ;
	CRP = 0 ;
}


void Table::tbvclear( int& RC,
		      fPOOL& funcPOOL )
{
	int i ;

	RC = 0 ;
	for ( i = 1 ; i <= num_all ; i++ )
	{
		funcPOOL.put( RC, 0, word( tab_all, i ), "" ) ;
		if ( RC > 0 ) { RC = 20 ; return ; }
	}
}


// *******************************************************************************************************************************
// *************************************************** TABLE MANAGER SECTION *****************************************************
// *******************************************************************************************************************************

tableMGR::tableMGR()
{
	debug1( "Constructor for table manager called " <<endl ) ;
}


void tableMGR::createTable( int& RC,
			    int m_task,
			    const string& tb_name,
			    string keys, string flds,
			    bool m_temporary,
			    tbREP m_REP,
			    const string& m_path,
			    tbDISP m_DISP )
{
	Table t ;

	iupper( keys ) ;
	iupper( flds ) ;

	debug1( "Creating table >>"+ tb_name +"<<" <<endl ) ;
	debug1( "     with keys >>"+ keys    +"<<" <<endl ) ;
	debug1( "    and fields >>"+ flds    +"<<" <<endl ) ;
	debug1( "          path >>"+ m_path  +"<<" <<endl ) ;

	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it != tables.end() )
	{
		if ( m_REP == REPLACE )
		{
			if ( m_DISP == SHARE )
			{
				log( "E", "Table "+ tb_name +" already exists in SHARE mode.  REPLACE not allowed" <<endl ) ;
				RC = 8 ;
				return ;
			}
			else
			{
				if ( it->second.ownerTask != m_task )
				{
					log( "E", "Table "+ tb_name +" already exists in EXCLUSIVE mode but owned by another task.  REPLACE not allowed" <<endl ) ;
					RC = 8 ;
					return ;
				}
				else
				{
					log( "I", "Table "+ tb_name +" already exists and REPLACE specified" <<endl ) ;
					tables.erase( tb_name ) ;
					RC = 4 ;
				}
			}
		}
		else
		{
			log( "E", "Table "+ tb_name +" already exists and REPLACE not specified" <<endl ) ;
			RC = 8 ;
			return ;
		}
		if ( m_temporary != it->second.tab_temporary )
		{
			log( "E", "Table created with inconsistent WRITE/NOWRITE attributes for open table "+ tb_name <<endl ) ;
			RC = 20 ;
			return  ;
		}
	}
	if ( getpaths( m_path ) > 0 ) { t.tab_path = getpath( m_path, 1 ) ; }
	t.ownerTask     = m_task ;
	t.tab_DISP      = m_DISP ;
	t.tab_temporary = m_temporary ;
	t.changed       = true   ;
	t.tab_keys      = space( keys ) ;
	t.num_keys      = words( keys ) ;
	t.tab_flds      = space( flds ) ;
	t.num_flds      = words( flds ) ;
	t.tab_all       = t.tab_keys + " " + t.tab_flds ;
	t.num_all       = t.num_keys + t.num_flds       ;
	t.tab_cmds      = ( t.tab_all == "ZCTVERB ZCTTRUNC ZCTACT ZCTDESC" ) ;
	tables[ tb_name ] = t ;
}


void tableMGR::loadTable( int& RC,
			  int task,
			  const string& tb_name,
			  tbDISP m_DISP,
			  const string& m_path )
{
	// If table already loaded, EXCLUSIVE can be changed to SHARE by the same task.  Any other combination is not valid

	// Routine to load V1 and V2 format tables

	char x        ;
	int  i        ;
	int  j        ;
	int  k        ;
	int  l        ;
	int  ver      ;
	int  num_rows ;
	int  num_keys ;
	int  num_flds ;
	int  all_flds ;
	int  n1, n2   ;

	bool   found  ;

	char buf1[ 256 ] ;
	char * buf2      ;
	char z[ 2 ]      ;

	string filename ;
	string path  ;
	string s     ;
	string hdr   ;
	string sir   ;
	string val   ;
	string keys  ;
	string flds  ;
	string err1  ;

	size_t buf2Size = 1024 ;

	std::ifstream table   ;
	vector<string> m_flds ;

	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it != tables.end() )
	{
		if ( (it->second.tab_DISP == SHARE) && (m_DISP == EXCLUSIVE) )
		{
			RC = 20 ;
			log( "E", "Table "+ tb_name +" is already open in SHARE mode.  EXCLUSIVE requested" <<endl ) ;
			return ;
		}
		if ( (it->second.tab_DISP == EXCLUSIVE) && it->second.ownerTask != task )
		{
			RC = 20 ;
			log( "E", "Table "+ tb_name +" is already open in EXCLUSIVE mode by another task" <<endl ) ;
			return ;
		}
		it->second.refCount++ ;
		it->second.tab_DISP = m_DISP ;
		return ;
	}

	found = false ;
	i = getpaths( m_path ) ;
	for ( j = 1 ; j <= i ; j++ )
	{
		path     = getpath( m_path, j ) ;
		filename = path + tb_name       ;
		if ( exists( filename ) )
		{
			if ( !is_regular_file( filename ) )
			{
				RC = 20 ;
				log( "E", "Table load file name "+ filename +" is not a regular file" <<endl ) ;
				return  ;
			}
			else
			{
				found = true ;
				break ;
			}
		}
	}
	if ( !found )
	{
		log( "E", "Table file not found in path search for "+ tb_name <<endl ) ;
		RC = 8 ;
		return ;
	}
	debug1( "Loading table "+ tb_name +" from "+ filename <<endl ) ;

	table.open( filename.c_str() , ios::binary | ios::in ) ;
	if ( !table.is_open() )
	{
		RC = 20 ;
		log( "E", "Table open for "+ tb_name +" gave an error.  Filename "+ filename <<endl ) ;
		return ;
	}

	table.read( buf1, 2);
	if ( memcmp( buf1, "\x00\x85", 2 ) )
	{
		RC = 20 ;
		log( "E", "Not a valid table file '"+ tb_name +"'.  Appears corrupt.  Filename "+ filename <<endl ) ;
		table.close() ;
		return ;
	}

	table.get( x ) ;
	ver = static_cast< int >( x ) ;
	if ( ver > 2 )
	{
		RC = 20 ;
		log( "E", "Invalid table prefix.  Version "<< ver <<" not supported or table is corrupt.  Filename "+ filename <<endl ) ;
		table.close() ;
		return ;
	}

	table.get( x ) ;
	i = static_cast< int >( x ) ;
	if ( i < 0 ) { i = 256 + i ; }
	table.read( buf1, i ) ;
	hdr.assign( buf1, i ) ;
	table.get( x ) ;
	i = static_cast< int >( x ) ;
	if ( i < 0 ) { i = 256 + i ; }
	for ( j = 0 ; j < i ; j++ )
	{
		table.get( x ) ;
		k = static_cast< int >( x ) ;
		if ( k < 0 ) { k = 256 + k ; }
		table.read( buf1, k ) ;
		switch ( j )
		{
		case 0: sir.assign( buf1, k ) ;
			break ;
		default:
			RC = 20 ;
			log( "E", "Invalid number of table information fields found.  Only 1 expected.  Found " << j+1 <<endl ) ;
			table.close() ;
			return ;
		}
	}
	table.read( z, 2 ) ;
	n1 = static_cast< int >( z[ 0 ] ) ;
	n2 = static_cast< int >( z[ 1 ] ) ;
	if ( n1 < 0 ) { n1 = 256 + n1 ; }
	if ( n2 < 0 ) { n2 = 256 + n2 ; }
	num_rows = n1 * 256 + n2 ;

	table.get( x ) ;
	n1 = static_cast< int >( x ) ;
	if ( n1 < 0 ) { n1 = 256 + n1 ; }
	num_keys = n1  ;
	table.get( x ) ;
	n1 = static_cast< int >( x ) ;
	if ( n1 < 0 ) { n1 = 256 + n1 ; }
	num_flds = n1 ;
	all_flds = num_keys + num_flds     ;

	keys = "" ;
	flds = "" ;

	for ( j = 0 ; j < num_keys ; j++ )
	{
		table.get( x ) ;
		if ( table.fail() != 0 )
		{
			RC = 20 ;
			log( "E", "Unexpected EOF reading keys.  Table "+ tb_name +" appears to be corrupt. Filename "+ filename <<endl ) ;
			table.close() ;
			return ;
		}
		i = static_cast< int >( x ) ;
		if ( i < 0 ) { i = 256 + i ; }
		table.read( buf1, i ) ;
		keys = keys + s.assign( buf1, i ) + " " ;
	}
	for ( j = 0 ; j < num_flds ; j++ )
	{
		table.get( x ) ;
		if ( table.fail() != 0 )
		{
			RC = 20 ;
			log( "E", "Unexpected EOF reading fields.  Table "+ tb_name +" appears to be corrupt.  Filename "+ filename <<endl ) ;
			table.close() ;
			return ;
		}
		i = static_cast< int >( x ) ;
		if ( i < 0 ) { i = 256 + i ; }
		table.read( buf1, i ) ;
		flds = flds + s.assign( buf1, i ) + " " ;
	}

	createTable( RC, task, tb_name, keys, flds, false, NOREPLACE, path, m_DISP ) ;
	if ( RC > 0 )
	{
		log( "E", "TBCREATE failed during loadTable for "+ tb_name <<endl ) ;
		table.close() ;
		return ;
	}

	debug1( "Table created RC = " << RC <<endl ) ;
	debug1( "Number of rows found in table " << num_rows <<endl ) ;

	it->second.reserveSpace( num_rows ) ;
	if ( sir != "" )
	{
		tables[ tb_name ].tbsort( RC, sir ) ;
		if ( RC > 0 )
		{
			RC = 20 ;
			log( "E", "Bad return code from tbsort.  Sort Information Record found is "+ sir <<endl ) ;
			return  ;
		}
	}
	buf2 = new char[ buf2Size ] ;
	err1 = "Unexpected EOF reading table values.  Table "+ tb_name +" appears to be corrupt.  Filename "+ filename ;
	for ( l = 0 ; l < num_rows ; l++ )
	{
		m_flds.clear() ;
		for ( j = 0 ; j < all_flds ; j++ )
		{
			table.read( z, 2 ) ;
			if ( table.fail() != 0 )
			{
				RC = 20 ;
				log( "E", err1 ) ;
				table.close() ;
				delete[] buf2 ;
				return  ;
			}
			n1 = static_cast< int >( z[ 0 ] ) ;
			n2 = static_cast< int >( z[ 1 ] ) ;
			if ( n1 < 0 ) { n1 = 256 + n1 ; }
			if ( n2 < 0 ) { n2 = 256 + n2 ; }
			i = n1 * 256 + n2 ;
			if ( i > buf2Size )
			{
				delete[] buf2 ;
				buf2Size = i  ;
				buf2     = new char[ buf2Size ] ;
			}
			table.read( buf2, i ) ;
			if ( table.fail() != 0 )
			{
				RC = 20 ;
				log( "E", err1 ) ;
				table.close() ;
				delete[] buf2 ;
				return ;
			}
			val.assign( buf2, i ) ;
			debug2( "Value read for row "<< l <<" position "<< j <<" '"+ val +"'" <<endl ) ;
			m_flds.push_back ( val ) ;
		}
		if ( ver > 1 )
		{
			table.read( z, 2 ) ;
			if ( table.fail() != 0 )
			{
				RC = 20 ;
				log( "E", err1 ) ;
				table.close() ;
				delete[] buf2 ;
				return  ;
			}
			n1 = static_cast< int >( z[ 0 ] ) ;
			n2 = static_cast< int >( z[ 1 ] ) ;
			if ( n1 < 0 ) { n1 = 256 + n1 ; }
			if ( n2 < 0 ) { n2 = 256 + n2 ; }
			i = n1 * 256 + n2 ;
			for ( j = 0 ; j < i ; j++ )
			{
				table.read( z, 2 ) ;
				if ( table.fail() != 0 )
				{
					RC = 20 ;
					log( "E", err1 ) ;
					table.close() ;
					delete[] buf2 ;
					return  ;
				}
				n1 = static_cast< int >( z[ 0 ] ) ;
				n2 = static_cast< int >( z[ 1 ] ) ;
				if ( n1 < 0 ) { n1 = 256 + n1 ; }
				if ( n2 < 0 ) { n2 = 256 + n2 ; }
				k = n1 * 256 + n2 ;
				if ( k > buf2Size )
				{
					delete[] buf2 ;
					buf2Size = i  ;
					buf2     = new char[ buf2Size ] ;
				}
				table.read( buf2, k ) ;
				if ( table.fail() != 0 )
				{
					RC = 20 ;
					log( "E", err1 ) ;
					table.close() ;
					delete[] buf2 ;
					return ;
				}
				val.assign( buf2, k ) ;
				m_flds.push_back ( val ) ;
			}
			table.read( z, 1 ) ;
			if ( table.fail() != 0 )
			{
				RC = 20 ;
				log( "E", err1 ) ;
				table.close() ;
				delete[] buf2 ;
				return  ;
			}
			if ( z[ 0 ] != char(0xFF) )
			{
				RC = 20 ;
				log( "E", "End of record marker not found.  File corrupt: "+ filename <<endl ; )
				table.close() ;
				delete[] buf2 ;
				return  ;
			}
		}
		tables[ tb_name ].loadRow( RC, m_flds ) ;
	}
	if ( ver > 1 )
	{
		table.read( z, 1 ) ;
		if ( table.fail() != 0 )
		{
			RC = 20 ;
			log( "E", err1 ) ;
			table.close() ;
			delete[] buf2 ;
			return  ;
		}
		if ( z[ 0 ] != char(0xFF) )
		{
			RC = 20 ;
			log( "E", "End of file marker not found.  File corrupt: "+ filename <<endl ; )
			table.close() ;
			delete[] buf2 ;
			return  ;
		}
	}
	tables[ tb_name ].resetChanged() ;
	debug1( "Number of rows loaded from table " << l <<endl ) ;
	if ( l != num_rows )
	{
		log( "E",  "Inconsistent number of records found with saved total.  Found=" << l << " Saved Total=" << num_rows <<endl ; )
		RC = 20 ;
	}
	table.close() ;
	delete[] buf2 ;
}


void tableMGR::saveTable( int& RC,
			  int task,
			  const string& tb_name,
			  const string& m_newname,
			  const string& m_path,
			  bool m_err )
{
	// This can be called by tbclose() or tbsave().  For temporary tables, only report error for tbsave() with no m_newname RC=12

	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for save" <<endl ) ;
		return ;
	}
	if ( m_err && m_newname == "" && it->second.tab_temporary )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" is a temporary table.  Cannot be saved" <<endl ) ;
		return ;
	}
	it->second.saveTable( RC, ( m_newname == "" ? tb_name : m_newname ), m_path ) ;
}


void tableMGR::destroyTable( int& RC,
			     int task,
			     const string& tb_name )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" does not exist" <<endl ) ;
		return ;
	}

	if ( it->second.refCount == 1 )
	{
		tables.erase( it ) ;
	}
	else
	{
		it->second.refCount-- ;
	}
}


bool tableMGR::isloaded( const string& tb_name )
{
	return tables.find( tb_name ) != tables.end() ;
}


bool tableMGR::tablexists( const string& tb_name,
			   const string& tb_path )
{
	// Check if a table file exists in path tb_path.  Don't check to see if it is a valid table, just a valid file.

	int  i ;
	int  j ;

	bool found ;

	string filename ;

	i     = getpaths( tb_path ) ;
	found = false ;
	for ( j = 1 ; j <= i ; j++ )
	{
		filename = getpath( tb_path, j ) + tb_name ;
		if ( exists( filename ) )
		{
			if ( !is_regular_file( filename ) )
			{
				log( "I", "Table file name "+ filename +" is not a regular file" <<endl ) ;
				return  false ;
			}
			else
			{
				found = true ;
				break ;
			}
		}
	}
	if ( !found ) { return  false ; }
	return true ;
}


void tableMGR::statistics()
{
	map<string, Table>::iterator it     ;
	map<string, tbsearch>::iterator its ;

	log( "-", "TABLE STATISTICS:" <<endl ) ;
	log( "-", "         Number of tables loaded . . . " << tables.size() <<endl ) ;
	log( "-", "          Table details:" <<endl ) ;
	for ( it = tables.begin() ; it != tables.end() ; it++ )
	{
		log( "-", "" <<endl ) ;
		log( "-", "                  Table: "+ it->first <<endl ) ;
		if ( it->second.tab_temporary )
		{
			log( "-", "                 Status: Temporary table" <<endl ) ;
		}
		else if ( it->second.changed )
		{
			log( "-", "                 Status: Modified since load or last save" <<endl ) ;
		}
		log( "-", "            Owning Task: " << it->second.ownerTask <<endl ) ;
		if ( it->second.num_keys > 0 )
		{
			log( "-", "                   Keys: " << setw(3) << it->second.tab_keys <<endl ) ;
		}
		log( "-", "                 Fields: " << it->second.tab_flds <<endl ) ;
		log( "-", "         Number of rows: " << it->second.table.size() <<endl ) ;
		log( "-", "                   Path: " << it->second.tab_path <<endl ) ;
		log( "-", "   Current Row Position: " << it->second.CRP <<endl ) ;
		if ( it->second.sarg.size() > 0 )
		{
			log( "-", "Current Search Argument: " <<endl ) ;
			log( "-", "        Condition Pairs: " << it->second.sa_cond_pairs <<endl ) ;
			log( "-", "       Search Direction: " << it->second.sa_dir <<endl ) ;
			log( "-", "    Extension Variables: " << it->second.sa_namelst <<endl ) ;
			for ( its = it->second.sarg.begin() ; its != it->second.sarg.end() ; its++ )
			{
				log( "-", "             Field Name: "+ its->first <<endl ) ;
				if ( its->second.tbs_gen )
					log( "-", "            Field Value: "+ its->second.tbs_val +" (generic search)" <<endl ) ;
				else
					log( "-", "            Field Value: "+ its->second.tbs_val <<endl ) ;
				log( "-", "        Field Condition: "+ its->second.tbs_scond <<endl ) ;
			}
		}
		if ( it->second.sort_ir != "" )
		{
			log( "-", "Sort Information Record: "+ it->second.sort_ir <<endl ) ;
		}
	}
	log( "-", "*************************************************************************************************************" <<endl ) ;
}


int tableMGR::getCRP( int& RC,
		      const string& tb_name )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for get CRP" <<endl ) ;
		return 0 ;
	}
	return it->second.getCRP() ;
}


void tableMGR::fillfVARs( int& RC,
			  fPOOL& funcPOOL,
			  const string& tb_name,
			  int depth,
			  int posn )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for fill function pool" <<endl ) ;
		return ;
	}
	it->second.fillfVARs( RC, funcPOOL, depth, posn ) ;
}


void tableMGR::tbget( int& RC,
		      fPOOL& funcPOOL,
		      const string& tb_name,
		      const string& tb_savenm,
		      const string& tb_rowid_vn,
		      const string& tb_noread,
		      const string& tb_crp_name  )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbget" <<endl ) ;
		return ;
	}
	it->second.tbget( RC, funcPOOL, tb_savenm, tb_rowid_vn, tb_noread, tb_crp_name ) ;
}


void tableMGR::tbmod( int& RC,
		      fPOOL& funcPOOL,
		      const string& tb_name,
		      const string& tb_namelst,
		      const string& tb_order )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for modify" <<endl ) ;
		return ;
	}
	it->second.tbmod( RC, funcPOOL, tb_namelst, tb_order ) ;
}


void tableMGR::tbput( int& RC,
		      fPOOL& funcPOOL,
		      const string& tb_name,
		      const string& tb_namelst,
		      const string& tb_order )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for modify" <<endl ) ;
		return ;
	}
	it->second.tbput( RC, funcPOOL, tb_namelst, tb_order ) ;
}


void tableMGR::tbadd( int& RC,
		      fPOOL& funcPOOL,
		      const string& tb_name,
		      const string& tb_namelst,
		      const string& tb_order,
		      int tb_num_of_rows )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbadd" <<endl ) ;
		return ;
	}
	it->second.tbadd( RC, funcPOOL, tb_namelst, tb_order, tb_num_of_rows ) ;
}


void tableMGR::tbbottom( int& RC,
			 fPOOL& funcPOOL,
			 const string& tb_name,
			 const string& tb_savenm,
			 const string& tb_rowid_vn,
			 const string& tb_noread,
			 const string& tb_crp_name  )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbbottom" <<endl ) ;
		return ;
	}
	it->second.tbbottom( RC, funcPOOL, tb_savenm, tb_rowid_vn, tb_noread, tb_crp_name ) ;
}


void tableMGR::tbdelete( int& RC,
			 fPOOL& funcPOOL,
			 const string& tb_name )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbdelete" <<endl ) ;
		return ;
	}
	it->second.tbdelete( RC, funcPOOL ) ;
}


void tableMGR::tberase( int& RC,
			const string& tb_name,
			const string& tb_path )
{
	int  i ;
	int  j ;

	bool found ;

	string filename ;
	map<string, Table>::iterator it ;

	RC = 0 ;

	it = tables.find( tb_name ) ;
	if ( it != tables.end() )
	{
		RC = 12 ;
		log( "E", "Table '"+ tb_name +"' in use. Cannot be erased unless closed" <<endl ) ;
		return ;
	}

	i = getpaths( tb_path ) ;
	found = false ;
	for ( j = 1 ; j <= i ; j++ )
	{
		filename = getpath( tb_path, j ) + tb_name ;
		if ( exists( filename ) )
		{
			if ( !is_regular_file( filename ) )
			{
				RC = 16 ;
				log( "W", "Table load file name "+ filename +" is not a regular file" <<endl ) ;
				return  ;
			}
			else
			{
				found = true ;
				break ;
			}
		}
	}
	if ( !found )
	{
		RC = 8 ;
		log( "W", "Table file for '"+ tb_name +"' not found in path search"  <<endl ) ;
		return ;
	}
	remove( filename ) ;
	log( "I", "Table file '"+ filename +"' deleted"  <<endl ) ;
}


void tableMGR::tbexist( int& RC,
			fPOOL& funcPOOL,
			const string& tb_name )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbexist" <<endl ) ;
		return ;
	}
	it->second.tbexist( RC, funcPOOL ) ;
}


void   tableMGR::tbquery( int& RC,
			  fPOOL& funcPOOL,
			  const string& tb_name,
			  const string& tb_keyn,
			  const string& tb_varn,
			  const string& tb_rownn,
			  const string& tb_keynn,
			  const string& tb_namenn,
			  const string& tb_crpn,
			  const string& tb_sirn,
			  const string& tb_lstn,
			  const string& tb_condn,
			  const string& tb_dirn )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		debug1( "Table "+ tb_name +" not found to set search argument" <<endl ) ;
		return ;
	}
	it->second.tbquery( RC, funcPOOL, tb_keyn, tb_varn, tb_rownn, tb_keynn, tb_namenn, tb_crpn, tb_sirn, tb_lstn, tb_condn, tb_dirn ) ;
}


void   tableMGR::tbsarg( int& RC,
			 fPOOL& funcPOOL,
			 const string& tb_name,
			 const string& tb_namelst,
			 const string& tb_dir,
			 const string& tb_cond_pairs )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		debug1( "Table "+ tb_name +" not found to set search argument" <<endl ) ;
		return ;
	}
	it->second.tbsarg( RC, funcPOOL, tb_namelst, tb_dir, tb_cond_pairs ) ;
}


void tableMGR::tbscan( int& RC,
		       fPOOL& funcPOOL,
		       const string& tb_name,
		       const string& tb_namelst,
		       const string& tb_savenm,
		       const string& tb_rowid_vn,
		       const string& tb_dir,
		       const string& tb_noread,
		       const string& tb_crp_name,
		       const string& tb_condlst )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		debug1( "Table "+ tb_name +" not found for tbscan" <<endl ) ;
		return ;
	}
	it->second.tbscan( RC, funcPOOL, tb_namelst, tb_savenm, tb_rowid_vn, tb_dir, tb_noread, tb_crp_name, tb_condlst ) ;
}


void tableMGR::cmdsearch( int& RC,
			  fPOOL& funcPOOL,
			  string tb_name,
			  const string& cmd,
			  const string& paths )
{
	// Search table for command 'cmd'.  Load table if not already in storage.

	// RC = 0  Okay
	// RC = 8  Command not found
	// RC = 12 Table not found
	// RC = 20 Severe error

	RC = 0 ;
	map<string, Table>::iterator it ;

	tb_name += "CMDS" ;
	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		if ( tablexists( tb_name, paths ) )
		{
			loadTable( RC, 0, tb_name, SHARE, paths ) ;
			if ( RC > 0 )
			{
				log( "E", "Command table "+ tb_name +" failed to load" <<endl ) ;
				RC = 20 ;
				return  ;
			}
			it = tables.find( tb_name ) ;
		}
		else
		{
			RC = 12 ;
			return  ;
		}
	}
	it->second.cmdsearch( RC, funcPOOL, cmd ) ;
}


void tableMGR::tbskip( int& RC,
		       fPOOL& funcPOOL,
		       const string& tb_name,
		       int num,
		       const string& tb_savenm,
		       const string& tb_rowid_vn,
		       const string& tb_rowid,
		       const string& tb_noread,
		       const string& tb_crp_name )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbskip" <<endl ) ;
		return ;
	}
	it->second.tbskip( RC, funcPOOL, num, tb_savenm, tb_rowid_vn, tb_rowid, tb_noread, tb_crp_name ) ;
}


void tableMGR::tbsort( int& RC,
		       const string& tb_name,
		       const string& tb_fields )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbsort" <<endl ) ;
		return ;
	}
	it->second.tbsort( RC, tb_fields ) ;
}


void tableMGR::tbtop( int& RC,
		      const string& tb_name )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbtop" <<endl ) ;
		return ;
	}
	it->second.tbtop( RC ) ;
}


void tableMGR::tbvclear( int& RC,
			 fPOOL& funcPOOL,
			 const string& tb_name )
{
	RC = 0 ;
	map<string, Table>::iterator it ;

	it = tables.find( tb_name ) ;
	if ( it == tables.end() )
	{
		RC = 12 ;
		log( "E", "Table "+ tb_name +" not found for tbvclear" <<endl ) ;
		return ;
	}
	it->second.tbvclear( RC, funcPOOL ) ;
}
