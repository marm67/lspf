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

class pApplication
{
	public:
		pApplication()  ;
		virtual ~pApplication() ;

		virtual void application() = 0 ;
		virtual void isredit( const string& ) ;

		static pApplication * currApplication ;
		static map<int, void *>ApplUserData   ;

		poolMGR  * p_poolMGR  ;
		tableMGR * p_tableMGR ;

		int    taskID             ;
		int    RC                 ;
		string PARM               ;
		string PANELID            ;
		string PPANELID           ;
		bool   ControlDisplayLock ;
		bool   ControlNonDispl    ;
		bool   ControlSplitEnable ;
		bool   ControlRefUpdate   ;
		bool   errPanelissued     ;
		bool   testMode           ;
		bool   propagateEnd       ;
		bool   jumpEntered        ;
		bool   noTimeOut          ;
		bool   busyAppl           ;
		bool   terminateAppl      ;
		bool   abnormalEnd        ;
		bool   abnormalEndForced  ;
		bool   abnormalTimeout    ;
		bool   reloadCUATables    ;
		bool   rawOutput          ;
		bool   libdef_muser       ;
		bool   libdef_puser       ;
		bool   libdef_tuser       ;
		string shrdPool           ;
		string rexxName           ;
		selobj SELCT              ;
		bool   SEL                ;
		bool   NEWPOOL            ;
		bool   PASSLIB            ;
		bool   SUSPEND            ;
		bool   setMSG             ;
		string reffield           ;

		string ZAPPNAME           ;
		string ZAPPDESC           ;
		string ZMLIB              ;
		string ZPLIB              ;
		string ZTLIB              ;
		string ZZAPPLID           ;
		string ZCURFLD            ;
		int    ZCURPOS            ;
		string ZTDMARK            ;
		int    ZTDDEPTH           ;
		int    ZTDROWS            ;
		int    ZTDSELS            ;
		int    ZTDTOP             ;
		int    ZTDVROWS           ;
		int    ZCURINX            ;
		string ZCMD               ;
		string ZAPPLID            ;
		string ZEDLMSG            ;
		string ZEDSMSG            ;
		string ZHELP              ;
		string ZAHELP             ;
		string ZHOME              ;
		string ZORXPATH           ;
		string ZPFKEY             ;
		string ZSEL               ;
		string ZSELPARM           ;
		string ZSCREEN            ;
		string ZSCROLL            ;
		int    ZSCRMAXD           ;
		int    ZSCRMAXW           ;
		int    ZSCROLLN           ;
		string ZSCROLLA           ;
		string ZUSER              ;
		string ZVERB              ;
		int    ZRC                ;
		int    ZRSN               ;
		string ZRESULT            ;

		vector<string>rmsgs  ;
		boost::posix_time::ptime resumeTime ;
		boost::thread            * pThread  ;

		void (* lspfCallback)( lspfCommand & ) ;

		pPanel * currPanel   ;
		pPanel * currtbPanel ;

		int    taskid()      { return taskID ; }
		void   init() ;
		void   info() ;
		bool   isRawOutput() { return rawOutput ; }

		string get_search_path( s_paths ) ;
		string get_select_cmd( const string& )  ;
		selobj get_select_cmd() { return SELCT ; }
		string get_help_member( int, int ) ;
		string get_current_panelDescr() ;
		string get_current_screenName() ;

		void   control( const string&, const string&, const string& ="" ) ;
		void   control( const string&, void (pApplication::*)() ) ;
		void   libdef( const string&, const string& ="", const string& ="", const string& ="UNCOND" ) ;
		void   rdisplay( const string&, bool =true ) ;
		void   display( string p_name, const string& p_msg = "", const string& p_cursor = "", int p_curpos = 0 ) ;
		void   pquery( const string& p_name, const string& a_name, const string& t = "", const string& w = "", const string& d = "", const string& r = "", const string& c = "" ) ;
		void   select( const string& ) ;
		void   select( selobj ) ;
		void   attr( const string&, const string& ) ;

		void   vcopy( const string&, string&, vcMODE=MOVE )   ;
		void   vcopy( const string&, string * &, vcMODE=LOCATE ) ;
		void   vdefine( const string&, string *, string * =NULL, string * =NULL, string * =NULL, string * =NULL, string * =NULL, string * =NULL, string * =NULL ) ;
		void   vdefine( const string&, int *, int * =NULL, int * =NULL, int * =NULL, int * =NULL, int * =NULL, int * =NULL, int * =NULL ) ;
		void   vdelete( const string& ) ;
		void   verase( const string& var, poolType =ASIS ) ;
		void   vget( const string& var, poolType =ASIS )   ;
		void   vput( const string& var, poolType =ASIS )   ;
		string vlist( poolType pType, int lvl ) ;
		string vilist( vdType =ALL ) ;
		string vslist( vdType =ALL ) ;
		void   vmask( const string& var, const string& type, const string& mask ) ;
		void   vreplace( const string&, const string& ) ;
		void   vreplace( const string&, int )           ;
		void   vreset() ;

		void   log( const string& msg ) ;

		void   tbadd( const string& tb_name, const string& tb_namelst="", const string& tb_order="", int tb_num_of_rows=0 ) ;
		void   tbbottom( const string& tb_name, const string& tb_savenm="", const string& tb_rowid_vn="", const string& tb_noread="", const string& tb_crp_name="" ) ;
		void   tbclose( const string& tb_name, const string& new_name="", const string& path="" ) ;
		void   tbcreate( const string& tb_name, const string& keys, const string& names, tbWRITE =NOWRITE, tbREP =NOREPLACE, string path="", tbDISP =EXCLUSIVE ) ;
		void   tbdelete( const string& tb_name ) ;
		void   tbdispl( const string& tb_name, string p_name="", const string& p_msg="", string p_cursor="", int p_csrrow=0, int p_csrpos=1, string p_autosel="YES", const string& p_crp_name="", const string& p_rowid_nm="" ) ;
		void   tbend( const string& tb_name ) ;
		void   tberase( const string& tb_name, string tb_path="" ) ;
		void   tbexist( const string& tb_name ) ;
		void   tbget( const string& tb_name, const string& tb_savenm="", const string& tb_rowid_vn="", const string& tb_noread="", const string& tb_crp_name="" ) ;
		void   tbmod( const string& tb_name, const string& tb_namelst="", const string& tb_order="" ) ;
		void   tbopen( const string& tb_name, tbWRITE WRITE, string path="", tbDISP c=EXCLUSIVE ) ;
		void   tbput( const string& tb_name, const string& tb_namelst="", const string& tb_order="" ) ;
		void   tbquery( const string& tb_name, const string& tb_keyn="", const string& tb_varn="", const string& tb_rownn="", const string& tb_keynn="", const string& tb_namenn="",const string& tb_crpn="", const string& tb_sirn="", const string& tb_lstn="", const string& tb_condn="", const string& tb_dirn="" ) ;
		void   tbsarg( const string& tb_name, const string& tb_namelst="", const string& tb_dir="NEXT", const string& tb_cond_pairs="" ) ;
		void   tbsave( const string& tb_name, const string& new_name="", const string& path="") ;
		void   tbscan( const string& tb_name, const string& tb_namelst="", const string& tb_savenm="", const string& tb_rowid_vn="", const string& tb_dir="NEXT", const string& tb_read="", const string& tb_crp_name="", const string& tb_condlst="" ) ;
		void   tbskip( const string& tb_name, int num=1, const string& tb_savenm="", const string& tb_rowid_vn="", const string& tb_rowid="", const string& tb_noread="", const string& tb_crp_name=""  ) ;
		void   tbsort( const string& tb_name, const string& tb_fields ) ;
		void   tbtop( const string& tb_name ) ;
		void   tbvclear( const string& tb_name ) ;

		bool   isTableOpen( const string& tb_name, const string& func ) ;

		void   browse( const string& m_file, const string& m_panel="" ) ;
		void   edit( const string& m_file, const string& m_panel="", const string& m_macro ="", const string& m_profile="" ) ;
		void   view( const string& m_file, const string& m_panel=""   ) ;
		void   setmsg( const string& msg, msgSET sType=UNCOND  ) ;
		void   getmsg( const string&, const string&, const string& ="", const string& ="", const string& ="",const string& ="", const string& ="" ) ;

		void   addpop( const string& ="", int =0, int =0 ) ;
		void   rempop( const string& ="" ) ;
		void   movepop() ;

		void   set_cursor( int row, int col ) ;

		bool   inputInhibited() ;
		bool   msgInhibited()   ;
		bool   isprimMenu()     ;
		void   get_home( uint & row, uint & col ) ;
		void   get_cursor( uint & row, uint & col ) ;
		void   set_msg( const string& ) ;
		void   set_msg1( const slmsg &, string, bool =false ) ;
		slmsg  getmsg1()   { return MSG1   ; }
		string getmsgid1() { return MSGID1 ; }
		void   msgResponseOK() ;
		bool   nretriev_on()   ;
		void   refresh_id()    ;
		string get_nretfield() ;
		void   cleanup()       ;
		void   cleanup_default() ;
		void   (pApplication::*pcleanup)() = &pApplication::cleanup_default ;
		void   abend()       ;
		void   uabend( const string&, const string& = "", int = -1 ) ;
		void   abendexc()    ;
		void   set_forced_abend()  ;
		void   set_timeout_abend() ;
		void   closeLog()    ;
		void   closeTables() ;
		void   checkRCode( const string& ="" ) ;
		void   checkRCode( errblock ) ;
		void   store_scrname() ;
		void   restore_Zvars( int ) ;
		void   reload_keylist( pPanel * ) ;
		bool   errorsReturn() ;
		void   setTestMode()  ;
		bool   selectPanel()  { return selPanel ; }
		void   startSelect( selobj& ) ;

		string sub_vars( string ) ;

		void   ispexec( const string& ) ;


	private:
		boost::mutex mutex ;

		fPOOL funcPOOL  ;

		int  addpop_row  ;
		int  addpop_col  ;
		int  lscreen_num ;

		bool addpop_active       ;
		bool ControlErrorsReturn ;
		bool ControlPassLRScroll ;
		bool selPanel   ;
		bool abending   ;

		string MSGID    ;
		string MSGID1   ;
		slmsg  MSG      ;
		slmsg  MSG1     ;
		string ZSCRNAME ;
		string ZERR1    ;
		string ZERR2    ;
		string ZERR3    ;
		string ZERR4    ;
		string ZERR5    ;
		string ZERR6    ;
		string ZERR7    ;
		string ZERR8    ;

		void get_Message( const string& )  ;
		int  chk_Message_id( const string& ) ;
		bool load_Message( const string& ) ;
		bool parse_Message( slmsg & )    ;
		bool sub_Message_vars( slmsg & ) ;


		errblock errBlock ;

		map<string, slmsg> msgList ;
		map<string,  bool> tablesOpen    ;
		map<string,  pPanel *> panelList ;

		stack<pPanel *> SRpanelStack ;

		stack<string> stk_str ;
		stack<int> stk_int    ;
		stack<int> addpop_stk ;

		stack<string> zmuser ;
		stack<string> zpuser ;
		stack<string> ztuser ;

		void load_keylist( pPanel * )   ;
		void createPanel( const string& p_name ) ;
		void actionSelect()   ;

		void wait_event() ;
} ;
