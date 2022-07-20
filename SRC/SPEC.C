/*
 *	spec.c			Platform-specific routines
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifdef AMIGA

#include <stdio.h>
#include <stdlib.h>

extern void *OpenLibrary( unsigned char *libName, unsigned long version );
extern void CloseLibrary( void *library );

		/* ---	   Funktioiden prototyypit	--- */

/* vain main():n käytössä olevat funktiot */
void OpenLibs( void );
void CloseLibs( void );

		/* ---		  Muuttujat		--- */

void *			MathBase		= NULL;
void *			MathTransBase	= NULL;

		/* ---		   Funktiot		--- */


/*		void OpenLibs( void );
 *
 *	Avaa Amigan liukulukujen käsittelykirjastoja.
 */

void OpenLibs( void )
{
	if(( MathBase = OpenLibrary( "mathffp.library", 0L )) == NULL )
	{
		fputs( "Could not open `mathffp.library'.\n", stderr );
		exit( 5 );
	}

	if(( MathTransBase = OpenLibrary( "mathtrans.library", 0L )) == NULL )
	{
		fputs( "Could not open `mathtrans.library'.\n", stderr );
		exit( 5 );
	}
}

/*		void CloseLibs( void );
 *
 *	Sulkee auki olevat Amigan liukulukujen käsittelykirjastot.
 */

void CloseLibs( void )
{
	if( MathTransBase )
	{
		CloseLibrary( MathTransBase );
		MathTransBase = NULL;
	}
	if( MathBase )
	{
		CloseLibrary( MathBase );
		MathBase = NULL;
	}
	return( 0 );
}

#endif

		/* ---		    Loppu		--- */
