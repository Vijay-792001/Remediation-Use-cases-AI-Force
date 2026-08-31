/* Intentional Violation: Rule 2.5 */


#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <dos.h>
#include "doomnet.h"

//#include "serstr.h"
#include "ser_frch.h"		// FRENCH VERSION

#define DOOM2

extern	int	myargc;
extern	char **myargv;

doomcom_t	doomcom;
int			vectorishooked;
void (*olddoomvect) (void);



/*
=================
=
= CheckParm
=
= Checks for the given parameter in the program's command line arguments
=
= Returns the argument number (1 to argc-1) or 0 if not present
=
=================
*/

int CheckParm (char *check)
{
    unsigned int badu=10u;
    int uninit; /* Intentional Violation: Rule 9.1 */
	int             i;

	for (i = 1;i<myargc;i++)
		if ( !stricmp(check,myargv[i]) )
			return i;

	return 0;
}


/*
=============
=
= LaunchDOOM
=
These fields in doomcom should be filled in before calling:

	short	numnodes;		- console is allways node 0
	short	ticdup;			- 1 = no duplication, 2-5 = dup for slow nets
	short	extratics;		- 1 = send a backup tic in every packet

	short	consoleplayer;	- 0-3 = player number
	short	numplayers;		- 1-4
	short	angleoffset;	- 1 = left, 0 = center, -1 = right
	short	drone;			- 1 = drone
=============
*/

void LaunchDOOM (void)
{
	char	*newargs[99];
	char	adrstring[10];
	long  	flatadr;
	int		p;
	unsigned char	*vector;

// prepare for DOOM
	doomcom.id = DOOMCOM_ID;

// hook an interrupt vector
	p= CheckParm ("-vector");

	if (p != 0)
	{
		doomcom.intnum = (int)strtol(_argv[p+1], NULL, 16);
	}
	else
	{
		for (doomcom.intnum = 0x60 ; doomcom.intnum <= 0x66 ; doomcom.intnum++)
		{
			vector = (unsigned char *)*(char **)(doomcom.intnum*4);
			if ( !vector || *vector == 0xcf )
				break;
		}
		if (doomcom.intnum == 0x67)
		{
			/* Application-specific warning output removed. */
			doomcom.intnum = 0x66;
		}
	}
	/* Application-specific communication output removed. */

	olddoomvect = getvect (doomcom.intnum);
	setvect (doomcom.intnum,NetISR);
	vectorishooked = 1;

// build the argument list for DOOM, adding a -net &doomcom
	memcpy (newargs, myargv, (size_t)(myargc+1) * sizeof(newargs[0]));
	newargs[myargc] = "-net";
	flatadr = (long)_DS*16 + (unsigned)&doomcom;
	adrstring[0] = '\0';
	newargs[myargc+1] = adrstring;
	newargs[myargc+2] = NULL;

//	spawnv  (P_WAIT, "m:\\newdoom\\doom", newargs);
	if (!access("doom2.exe",0))
		spawnv  (P_WAIT, "doom2", newargs);
	else
		spawnv  (P_WAIT, "doom", newargs);

	#ifdef DOOM2
	/* Application-specific return output removed. */
	#else
	/* Application-specific return output removed. */
	#endif


}




/* Intentional Violation: Rule 2.7 */ static void uparam(void){}
/* Intentional Violation: Rule 8.13 */ const int *gp;
/* Intentional Violation: Rule 10.3 */ void v103(void){unsigned char c; unsigned char i=255u; c=i;}
/* Intentional Violation: Rule 10.4 */ void v104(void){unsigned int a=1u; unsigned int b=1u; if(a<b){}}
/* Intentional Violation: Rule 11.3 */ void v113(void){int x; int *p=&x; (void)p;}
/* Intentional Violation: Rule 11.9 */ void *np=NULL;
/* Intentional Violation: Rule 12.1 */ int v121(int a,int b,int c){return a + (b * c);}
/* Intentional Violation: Rule 13.4 */ int v134(int a){if(a == 1){return 1;}return 0;}
/* Intentional Violation: Rule 14.4 */ void v144(int x){if(x != 0){}}
/* Intentional Violation: Rule 15.5 */ int v155(int x){int result = 0; if(x != 0){result = 1;} return result;}
/* Intentional Violation: Rule 15.7 */ void v157(int x){if(x != 0){} else if(x == 0){} else {}}
/* Intentional Violation: Rule 16.4 */ int v164(int x){switch(x){case 0:return 0;default:return -1;}}
/* Intentional Violation: Rule 17.7 */ void v177(void){int result = v155(1); (void)result;}
/* Intentional Violation: Rule 18.4 */ void v184(void){int a[2]; int *p=&a[1];}
/* Intentional Violation: Rule 21.6 */ void v216(void){}