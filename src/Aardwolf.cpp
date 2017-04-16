/* AppKit */
#include <Application.h>
#include <Message.h>

/* Interface Kit */
#include <Alert.h>

/* Storage Kit */
#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <Node.h>
#include <NodeInfo.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define VERSION "1.0"

const char * const STARTUP_MSG =
"--- Aardwolf -- Ref to Arg Launcher Utility ---\n\
VERSION " VERSION " ( " __DATE__ " " __TIME__ " )\n\
(c) 1998, Tim Vernum (zodsoft@kagi.com)\n" ;


// =================== StrCount ================= //

uint32 StrCount( const char * s, char c )
{
	uint32 count = 0 ;
	while( *s )
	{
		if( *s == c )
			count++ ;
		s++ ;
	}
	return count ;
}

// ================= StrEnd ============== //
char * StrEnd( char * s )
{
	while( *s )
		s++ ;
	return s ;
}

// ================= msprintf ============== //

void msprintf( char * buffer, const char * fmt, const char * str )
{
	while( *fmt )
	{
		if( *fmt == '%' )
		{
			fmt++ ;
			if( *fmt == '%' )
				*buffer = '%' ;
			else if( *fmt == 's' )
			{
				*buffer = '\'' ;
				buffer++ ;
				strcpy( buffer, str ) ;
				buffer = StrEnd( buffer ) ;
				*buffer = '\'' ;
			}
			else // ERROR!
			{
				fprintf( stderr, "Error Badly formated string %%%c\n", *fmt ) ;
				*buffer = *fmt  ;
			}
		}
		else
			*buffer = *fmt  ;
	
		fmt++;
		buffer++ ;
	}
}

// ================= Application ================ //

static const char * const AppSignature = "application/x-vnd.Zod-Aardwolf" ;  

class Aardwolf : public BApplication
{
	typedef BApplication inherited ;
	public:
		 Aardwolf( void ) ;
				
		void MessageReceived( BMessage * ) ;
		void RefsReceived( BMessage * ) ;
		
	private:
		
		static int32 LaunchApp_( void * ) ;
		void LaunchApp( entry_ref * ) ;
	
		char * ParseArgs( const char *, BPath * ) ;
		BMessage fApplications ;
		
		int32 fThreadCount ;
} ;

Aardwolf :: Aardwolf( void )
:	BApplication( AppSignature ) ,
	fThreadCount( 0 )
{
	printf( STARTUP_MSG ) ;

	BPath path ;

	status_t s ;
	s = find_directory( B_USER_SETTINGS_DIRECTORY , &path, true ) ;
	if( s != B_OK )
	{
		printf( "Error: Cannot find settings directory: %s\n",
			strerror( s ) ) ;
		exit( -1 ) ;
	}

	BDirectory dir( path.Path() ) ;
	s = dir.InitCheck() ;
	if( s != B_OK )
	{
		printf( "Error: Cannot open settings directory: %s\n",
			strerror( s ) ) ;
		exit( -1 ) ;
	}

	BFile file( &dir, "Aardwolf_settings", B_READ_ONLY ) ;
	s = file.InitCheck() ;
	if( s != B_OK )
	{
		printf( "Error: Cannot open settings file: %s\n",
			strerror( s ) ) ;
		exit( -1 ) ;
	}
	
	s = fApplications.Unflatten( &file ) ;
	if( s != B_OK )
	{
		printf( "Error: Cannot restore settings file: %s\n",
			strerror( s ) ) ;
		exit( -1 ) ;
	}
}

void Aardwolf :: MessageReceived( BMessage * msg )
{
	switch( msg->what )
	{
		default:
			inherited::MessageReceived( msg ) ;
	}
} 

char * Aardwolf :: ParseArgs( const char * s, BPath * path )
{
	char * buffer ;

	uint32 count = StrCount( s, '%' ) ;
	if( count == 0 )
	{
		buffer = new char[ strlen( s ) + strlen(path->Path()) + 4 ] ;
		strcpy( buffer, s ) ;
		strcat( buffer, " '" ) ;
		strcat( buffer, path->Path() ) ;
		strcat( buffer, "'" ) ;
	}
	else
	{
		buffer = new char[ strlen( s ) + ( count * (strlen(path->Path())+2) ) + 1 ] ;
		msprintf( buffer, s, path->Path() ) ;
	}

	return buffer ;
}

void Aardwolf :: RefsReceived( BMessage * msg )
{
	uint32 type; 
	int32 count; 

	msg->GetInfo( "refs", &type, &count); 
	if ( type != B_REF_TYPE ) 
 		return ; 

	while( count -- )
	{
		entry_ref * ref = new entry_ref ;		
		if ( msg->FindRef("refs", count, ref) == B_OK )
		{
			char thread_name[B_OS_NAME_LENGTH] ;
			thread_name[0] = 'l' ;
			thread_name[1] = '>' ;
			strncpy( thread_name+2, ref->name, B_OS_NAME_LENGTH-2 ) ;
			thread_name[B_OS_NAME_LENGTH-1] = 0 ;			
			
			thread_id tid = spawn_thread( LaunchApp_ , thread_name ,
											B_LOW_PRIORITY , ref ) ;
			atomic_add( &fThreadCount , 1 ) ;
			resume_thread( tid ) ;
		}
		else
			delete ref ;
	}
}

void Aardwolf :: LaunchApp( entry_ref * ref )
{
	BNode n( ref ) ;
	BNodeInfo ni( &n ) ;
	
	status_t s = ni.InitCheck() ;
	if ( s != B_OK )
	{
		printf( "Error: Cannot get type for file %s: %s\n",
			ref->name, strerror( s ) ) ;
		return ;
	}

	char type[256] ;
	ni.GetType( type ) ;			

	const char * c ;
	s = fApplications.FindString( type, &c ) ;
	if( s != B_OK )
	{
		BAlert * alt ;
		char str[512] ;
		sprintf( str,
			"Aardwolf has no application to launch the file %s of type %s\n",
			ref->name, type ) ;				
		alt = new BAlert( "Launch Failed", str, "Aye Cap'n", NULL, NULL,
			B_WIDTH_FROM_WIDEST, B_STOP_ALERT) ;
		alt->Go() ;
		return ;
	}

	BEntry entry( ref ) ;
	BPath path ;
	entry.GetPath( &path ) ;
	char * command = ParseArgs( c, &path ) ;
	s = system( command ) ; ;
	if( s )
	{
		char str[512] ;
		
		if( s >> 8 )
			sprintf( str,
				"Aardwolf encountered a problem launching the file '%s'\nCommand '%s' returned %ld",
				ref->name, command, ( s >> 8 ) ) ;				
		else
			sprintf( str,
				"Aardwolf failed launch the file '%s'\nCommand '%s' could not be run ( %s )",
				ref->name, command, strerror(s) ) ;
							
		BAlert * alt = new BAlert( "Launch Failed", str, "Aye Cap'n", NULL, NULL,
			B_WIDTH_FROM_WIDEST, B_WARNING_ALERT) ;
		alt->Go() ;
	}
	
	delete[] command ;
}

int32 Aardwolf :: LaunchApp_ ( void * data )
{
	entry_ref * ref = reinterpret_cast<entry_ref*>( data ) ;
	((Aardwolf *)be_app)->LaunchApp( ref ) ;
	delete ref ;

	//	atomic_add returns THE PREVIOUS VALUE of fThreadCount
	if( atomic_add( &( ((Aardwolf *)be_app)->fThreadCount ), -1 ) <= 1 )
	{
		be_app->Quit( ) ;
	}
	return 0 ;
}

int main( void )
{
	Aardwolf app ;
	app.Run();

	return(0);
}