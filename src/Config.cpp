/* AppKit */
#include <Application.h>
#include <Message.h>

/* Interface Kit */
#include <Alert.h>
#include <Window.h>
#include <ScrollView.h>
#include <ListView.h>
#include <ListItem.h>
#include <TextControl.h>

/* Storage Kit */
#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <Node.h>
#include <NodeInfo.h>

#include <string.h>
#include <stdio.h>

#define VERSION "1.0"

const char * const STARTUP_MSG =
"--- AardwolfConfig -- Ref to Arg Launcher Configuration ---\n\
VERSION " VERSION " ( " __DATE__ " " __TIME__ " )\n\
(c) 1998, Tim Vernum (zodsoft@kagi.com)\n\
-------------------------------------------------\n\n" ;


const uint32 kMSG_WindowAdd		= 'WAdd' ;
const uint32 kMSG_ButtonAdd		= 'BAdd' ;
const uint32 kMSG_ButtonDelete	= 'BDel' ;
const uint32 kMSG_ButtonOK		= 'Okay' ;
const uint32 kMSG_ButtonCancel	= 'Cncl' ;
const uint32 kMSG_ListInvoke	= 'Invk' ;

static const char * const AppSignature = "application/x-vnd.Zod-AardwolfConfig" ;  

class AardwolfConfig ;
class ConfigWindow ;
class AddWindow ;

class AardwolfConfig : public BApplication
{
	typedef BApplication inherited ;
	public:
		AardwolfConfig( void ) ;
						
		void MessageReceived( BMessage * ) ;
		bool QuitRequested( void ) ;

	protected:
		void RemoveItem( BListView * ) ;
		void BeginAddItem( void ) ;
		void BeginEditItem( BListView * ) ;
		void DoAddItem( BListView * , BMessage * ) ;
		
	private:
		BMessage fApplications ;
		ConfigWindow * fpWindow ;

} ;

class ConfigWindow : public BWindow
{
	typedef BWindow inherited ;
	public:
		ConfigWindow( void ) ;
		ConfigWindow( BMessage * ) ;

		void MessageReceived( BMessage * ) ;
		bool QuitRequested( void ) ;

		BListView * fListView ;

	private:

		void Setup( void ) ;
		
} ;

class AddWindow : public BWindow
{
	typedef BWindow inherited ;
	public:
		AddWindow( void ) ;

		void MessageReceived( BMessage * ) ;
		void Show( void ) ;

		void SetType( const char * ) ;
		void SetCommand( const char * ) ; 

	private:

		BTextControl	*	fTypeControl ,
						* 	fCommandControl ;
						
		BButton			*	fOKButton ,
						*	fCancelButton ;
		
} ;

// ========== MAIN ========= //

int main( void )
{
	AardwolfConfig app ;
	app.Run();

	return(0);
}

// =========== Settings ============ //

bool LoadSettings( const char * filename , BMessage * msg )
{
	status_t s ;
	BPath path ;
	s = find_directory( B_USER_SETTINGS_DIRECTORY , &path, true ) ;
	if( s != B_OK )
	{
		printf( "Error: Cannot find settings directory : %s\n",
			strerror( s ) ) ;
		return false ;
	}
	
	BDirectory dir( path.Path() ) ;
	s = dir.InitCheck() ;
	if( s != B_OK )
	{
		printf( "Error: Cannot open settings directory : %s\n",
			strerror( s ) ) ;
		return false ;
	}

	BFile file( &dir, filename , B_READ_ONLY ) ;
	s = file.InitCheck() ;
	if( s != B_OK )
	{
		printf( "Error: Cannot open settings file '%s' : %s\n",
			filename, strerror( s ) ) ;
		return false ;
	}
	
	s = msg->Unflatten( &file ) ;
	if( s != B_OK )
	{
		printf( "Error: Cannot restore settings file '%s' : %s\n",
			filename, strerror( s ) ) ;
		return false ;
	}
	
	return true ;
}
	
void SaveSettings( const char * filename , BMessage * msg )
{
	status_t s ;
	BPath path ;
	s = find_directory( B_USER_SETTINGS_DIRECTORY , &path, true ) ;
	if( s != B_OK )
	{
		printf( "Error: Cannot find settings directory : %s\n",
			strerror( s ) ) ;
		return ;
	}

	BDirectory dir( path.Path() ) ;
	s = dir.InitCheck() ;
	if( s != B_OK )
	{
		printf( "Error: Cannot open settings directory : %s\n",
			strerror( s ) ) ;
		return ;
	}

	BFile file( &dir, filename, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE ) ;
	s = file.InitCheck() ;
	if( s != B_OK )
	{
		printf( "Error: Cannot open settings file '%s' : %s\n",
			filename, strerror( s ) ) ;
		return ;
	}
	
	msg->what = B_ARCHIVED_OBJECT ;
	s = msg->Flatten( &file ) ;
	if( s != B_OK )
	{
		printf( "Error: Cannot save settings file '%s' : %s\n",
			filename, strerror( s ) ) ;
		return ;
	}
}


// =========== AardwolfConfig =========== //

AardwolfConfig :: AardwolfConfig( void )
:	BApplication( AppSignature )
{
	printf( STARTUP_MSG ) ;

	BMessage archive ;
	fpWindow = new ConfigWindow ;

	LoadSettings( "Aardwolf_settings", &fApplications ) ;
	int32 count = fApplications.CountNames( B_STRING_TYPE ) ;
	fprintf( stderr, "Found %ld recognised MIME-Type(s)\n", count ) ;
	while ( count -- )
	{
		char * name ;
		type_code type ;
		status_t s = fApplications.GetInfo( B_STRING_TYPE, count, &name, &type ) ;
		if( s == B_OK )
		{
			fprintf( stderr, "\tAdding MIME-Type %s\n", name ) ;
			fpWindow->fListView->AddItem( new BStringItem( name ) ) ;
		}
	}

	fpWindow->Show() ;
}

bool AardwolfConfig :: QuitRequested( void )
{
	SaveSettings( "Aardwolf_settings", &fApplications ) ;
	return inherited::QuitRequested() ;
}

void AardwolfConfig :: MessageReceived( BMessage * msg )
{
	switch( msg->what )
	{
		case kMSG_ButtonDelete:
			return RemoveItem( fpWindow->fListView ) ;
			break ;
			
		case kMSG_ListInvoke:
			return BeginEditItem( fpWindow->fListView ) ;
			break ;

		case kMSG_ButtonAdd:
			return BeginAddItem( ) ;
			break ;

		case kMSG_WindowAdd:
			return DoAddItem( fpWindow->fListView , msg ) ;
			break ;

		default:
			inherited::MessageReceived( msg ) ;
	}
} 

void AardwolfConfig :: RemoveItem( BListView * view )
{
	int32 sel = view->CurrentSelection() ;
	if( sel < 0 )
		return ;
	BListItem * li = view->ItemAt( sel ) ;
	BStringItem * si = dynamic_cast<BStringItem *>( li ) ;
	if( !si )
		return ;
	
	fApplications.RemoveName( si->Text() ) ;
	view->LockLooper() ;
	view->RemoveItem( si ) ;
	view->UnlockLooper() ;
}

void AardwolfConfig :: BeginAddItem( )
{
	AddWindow * window = new AddWindow ;
	window->Show() ;
}

void AardwolfConfig :: BeginEditItem( BListView * view )
{
	int32 sel = view->CurrentSelection() ;
	if( sel < 0 )
		return ;
	BListItem * li = view->ItemAt( sel ) ;
	BStringItem * si = dynamic_cast<BStringItem *>( li ) ;
	if( !si )
		return ;

	const char * cmd ;
	status_t s = fApplications.FindString( si->Text(), &cmd  ) ;
	if( s == B_OK )
	{
		AddWindow * window = new AddWindow ;
		window->SetType( si->Text() ) ;
		window->SetCommand( cmd ) ;		
		window->Show() ;
	}
}

void AardwolfConfig :: DoAddItem( BListView * view , BMessage * msg )
{
	const char * type , * cmd ;
	if( msg->FindString( "field:type", &type ) != B_OK
	 || msg->FindString( "field:command", &cmd) != B_OK )
	{
		fprintf( stderr, "Could not find fields in Message\n" ) ;
		return ;
	}
	
	status_t s = fApplications.ReplaceString( type, cmd ) ;
	if( s == B_NAME_NOT_FOUND )
	{
		view->LockLooper() ;
		s = fApplications.AddString( type, cmd ) ;
		view->AddItem( new BStringItem( type ) ) ;
		view->UnlockLooper() ;
	}
	
	if( s != B_OK )
		fprintf( stderr, "ERROR: Could not add %s => %s\n", type, cmd ) ;
}

// =========== ConfigWindow =========== //

ConfigWindow :: ConfigWindow( void )
:	BWindow( BRect( 50, 50, 250, 200 ), "Config", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	Setup() ;
}

ConfigWindow :: ConfigWindow( BMessage * msg )
:	BWindow( msg )
{
//	Setup() ;
}

void ConfigWindow :: Setup( void )
{
	BRect frame = Frame() ;
	frame.OffsetTo( 0,0 ) ;

	frame.right -= B_V_SCROLL_BAR_WIDTH ;
	frame.bottom -= 30 ;
	fListView = new BListView( frame, "list" ) ;
	AddChild( new BScrollView( "scroll_list", fListView, 
            B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true ) );
	fListView->SetInvocationMessage( new BMessage( kMSG_ListInvoke ) ) ;
	fListView->SetTarget( be_app ) ;

	frame.bottom += 25 ;
	frame.top = frame.bottom - 20 ;
	frame.left += 5 ;
	frame.right = frame.left + 65 ;
	BButton * button = new BButton( frame, "ButtonAdd", "Add", new BMessage( kMSG_ButtonAdd ) );
	AddChild( button ) ;
	button->SetTarget( be_app ) ;

	frame.left = frame.right + 10 ;
	frame.right = frame.left + 65 ;
	button = new BButton( frame, "ButtonDelete", "Delete", new BMessage( kMSG_ButtonDelete ) ) ;
	AddChild( button ) ;
	button->SetTarget( be_app ) ;
}

bool ConfigWindow :: QuitRequested( void )
{
/*
	BMessage archive ;
	Archive( &archive , true ) ;
	SaveSettings( "AardwolfConfig_settings", &archive ) ;
*/

	be_app->PostMessage( new BMessage( B_QUIT_REQUESTED ) ) ;
	return inherited::QuitRequested() ;
}

void ConfigWindow :: MessageReceived( BMessage * msg )
{
	switch( msg->what )
	{			
		default:
			inherited::MessageReceived( msg ) ;
	}
}

// =========== AddWindow =========== //


AddWindow :: AddWindow( void )
:	BWindow( BRect( 25, 75, 275, 180 ) , "Add" ,
		B_MODAL_WINDOW_LOOK , B_NORMAL_WINDOW_FEEL ,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE )
{
	BRect frame = Frame() ;
	frame.OffsetTo( 0,0 ) ;
	BRect r = frame ;
	BView * view = new BView( r, "bg", B_FOLLOW_ALL_SIDES , B_WILL_DRAW ) ;
	rgb_color colGrey = { 200, 200, 200 } ;
	view->SetViewColor( colGrey ) ;
	AddChild( view ) ;

	r.bottom -= 30 ;
	r.top += 5 ;
	r.bottom -= 5;
	r.bottom = r.top + r.Height()/2 ;
	r.right -= 5 ;
	r.left += 5 ;
	fTypeControl = new BTextControl( r, "type", "Type:", "", NULL ) ;
	view->AddChild( fTypeControl );

	float h = r.Height() ;
	r.top = r.bottom + 5;
	r.bottom = r.top + h ;
	fCommandControl = new BTextControl( r, "command", "Command:", "", NULL ) ;
	view->AddChild( fCommandControl );

	float width = view->StringWidth( "Command: " ) ;
	fTypeControl->SetDivider( width ) ;
	fCommandControl->SetDivider( width ) ;

	r.bottom = frame.bottom - 15 ;
	r.top = r.bottom - 20 ;
	r.right = frame.right - 15 ;
	r.left = r.right - 50 ;
	fOKButton = new BButton( r, "ButtonOk", "OK", new BMessage( kMSG_ButtonOK ) ) ;
	view->AddChild( fOKButton ) ;

	r.right = r.left - 15 ;
	r.left = r.right - 50 ;
	fCancelButton = new BButton( r, "ButtonCancel", "Cancel", new BMessage( kMSG_ButtonCancel ) ) ;
	view->AddChild( fCancelButton ) ;

	fOKButton->SetTarget( this ) ;
	fCancelButton->SetTarget( this ) ;
}

void AddWindow :: Show( void )
{
	inherited::Show() ;
}

void AddWindow :: MessageReceived( BMessage * msg )
{
	switch( msg->what )
	{
		case kMSG_ButtonOK:
		{
			BMessage send( kMSG_WindowAdd ) ;
			send.AddString( "field:type", fTypeControl->Text() ) ;
	 		send.AddString( "field:command", fCommandControl->Text() ) ;
			be_app->PostMessage( &send ) ;
			// Fall Thru' 
		}			
		case kMSG_ButtonCancel:
			Close() ;
			break ;

		default:
			inherited::MessageReceived( msg ) ;
	}
}

void AddWindow :: SetType( const char * t )
{
	fTypeControl->SetText( t ) ;
}

void AddWindow :: SetCommand( const char * c )
{
	fCommandControl->SetText( c ) ;
}
