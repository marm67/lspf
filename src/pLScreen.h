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

class pLScreen
{
	public:
		 pLScreen() ;
		~pLScreen() ;

	static pLScreen * currScreen ;
	static WINDOW   * OIA    ;
	static int  screensTotal ;
	static int  maxScreenID  ;
	static int  maxrow ;
	static int  maxcol ;

	void  clear() ;

	int   get_row()                    { return row ; }
	int   get_col()                    { return col ; }

	void  set_row_col( int a, int b )  { row = a ; col = b ; }

	void  cursor_left()                { col == 0 ? col = maxcol-1 : --col ; }
	void  cursor_right()               { col == maxcol-1 ? col = 0 : ++col ; }
	void  cursor_up()                  { row == 0 ? row = maxrow-1 : --row ; }
	void  cursor_down()                { row == maxrow-1 ? row = 0 : ++row ; }

	void  application_add( pApplication * pApplication ) { pApplicationStack.push( pApplication ) ; }
	void  application_remove_current()                   { pApplicationStack.pop() ; } ;
	pApplication * application_get_current()             { return pApplicationStack.top()   ; }
	int   application_stack_size()                       { return pApplicationStack.size()  ; }
	bool  application_stack_empty()                      { return pApplicationStack.empty() ; }

	void  set_Insert( bool ins )       { Insert = ins ; Insert ? curs_set(2) : curs_set(1)  ; }

	void  OIA_setup()  ;
	void  OIA_update( const char* ) ;
	void  show_enter() ;
	void  show_busy()  ;
	void  show_wait()  ;
	void  clear_status() ;

	void  save_panel_stack()    ;
	void  restore_panel_stack() ;

	int   screenID ;

private:
	int row ;
	int col ;

	bool Insert ;

	stack<pApplication *> pApplicationStack ;
	stack<PANEL *> panelList ;
} ;

