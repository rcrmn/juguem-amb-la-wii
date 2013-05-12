/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Roc Ramon 2008 <rcrmn16@gmail.com>
 * 
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Crida de les llibreries de codi */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <cwiid.h>

#define toggle_bit(bf,b)	\
	(bf) = ((bf) & b)		\
	       ? ((bf) & ~(b))	\
	       : ((bf) | (b))

/* Text del menú principal */
#define MENU \
	"1: encen el LED 1\n" \
	"5: encen tots els LED\n" \
	"a: activa l'accelerometre.\n" \
	"d: dades de l'accelerometre.\n" \
	"r: reporta l'estat.\n" \
	"f: crea un fitxer amb les dades.\n" \
	"l: llegir dades d'un fitxer.\n" \
	"x: exit\n"
	
/* Constant de la quantitat de reports de acc que es mostraran
	cada cop que es demani */
#define MAX_N_RPT 10

/* Temps d'espera en milisegons entre cada un dels reports d'acc */
#define WAIT_TIME_MS 500

/* Variable per mesurar el temps entre cada un dels reports */
clock_t t_inici = 0;


/* Funció assignada a l'event de quan el comandament envia informació a
	l'ordinador */
cwiid_mesg_callback_t cwiid_callback;

/* Variable per contar els reports restants */
int reports, envia = 0;

/* Estructura per a guardar les acc */
struct stc_acc_xyz {
	unsigned int x;
	unsigned int y;
	unsigned int z;
	clock_t time;
};
/* Punter on es guardaran les acc */
struct stc_acc_xyz *p_acc_xyz;
/* Contador del punter anterior */
int acc_xyz_count = 0;


/* Nom del fitxer per a lectura i punter de dades retornades d'aquest */
char nom_fitxer_a_llegir[1];
struct stc_acc_xyz *dades;


		/*********************************************
		****************** Funcions ******************
		**********************************************/

/* Encén i apaga els LED requerits especificats en led_state */
void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state);

/* Fixa el mode de report especificat en rpt_mode */
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode);

/* Activa la impressió de reports d'acc (per tal d'imprimir-ne
	una quantitat concreta) */
void print_report(cwiid_wiimote_t *wiimote);

/* Imprimeix l'estat del wiimote:
	- Report ACC: activat/desactivat
	-LEDs encesos
	-Estat de la bateria
	-ACC actual */
void print_state(struct cwiid_state *state);

/* Imprimeix les acceleracions de mesg i afegeix una entrada a 
	la matriu on es guarden les dades d'acc obtingudes */
void report_acc(struct cwiid_acc_mesg *mesg);

/* Imprimeix totes les entrades de la matriu d'acc */
void print_all_reports(struct stc_acc_xyz *xyz, int count);

/* Guarda totes les dades de la matriu d'acc en un fitxer al directori
	del projecte amb un nombre de nom */
int guardar_dades_en_fitxer(struct stc_acc_xyz xyz[], int count);

/* Transforma un nombre enter a una cadena de caràcters */
void enter_a_string(unsigned int nombre, char nchar[]);

/* Retorna el punter d'una matriu on hi ha les dades del fitxer d'acc especificat */
struct stc_acc_xyz *llegir_fitxer_acc(char *ruta, int *counter);

/* Funció que manté els reports del comandament desactivats durant un periode
	de temps definit per WAIT_TIME_MS, en milisegons */
/*int espera(clock_t inici, clock_t final);*/
void espera(cwiid_wiimote_t *wiimote);



int main()
{

	/* Punter al wiimote*/
	cwiid_wiimote_t *wiimote;

	/* Estat del wiimote*/
	struct cwiid_state state; 

	/* Adreça del bluetooth*/
	bdaddr_t bdaddr;

	/*variable d'estat dels LED; variable de mode de report del comandament */
	unsigned char led_state = 0, rpt_mode = 0; 

	/* Variables de sortida dels bucles */
	int exit = 0;
	int sortir = 0;

	/* Variable que defineix si s'ha connectat un comandament */
	int connectat = 0; 

	/* Contador per a la matriu de dades d'acc */
	int dades_acc_counter = 0; 
	

	printf("c: Connecta el wiimote.\nl: Llegeix un fitxer de dades.\nx: Surt.\n");

	/* Imprimir el menú inicial, on es tria si es vol obrir un fitxer de dades d'acc o
		connectar amb un comandament */
	while(!sortir)
	{
		switch(getchar()){
			case 'c':
				/* Si es tria la opció c (connectar) es surt d'aquest bucle per a iniciar
					la connexió i més endevant imprimir el menú d'opcions del comandament */
				sortir=1;
				break;
				
			case 'l':
				/* Si es tria la opció l es demana el nom d'un fitxer per a llegir */
				printf("Escriu el nom del fitxer a llegir:\n");
				scanf("%s", nom_fitxer_a_llegir);
			
				/* S'obre el fitxer, es llegeix i s'imprimeixen les dades en pantalla */
				dades = llegir_fitxer_acc(nom_fitxer_a_llegir, &dades_acc_counter);
			
				print_all_reports(dades, dades_acc_counter);
			
				/* Es torna a imprimir el menú d'inici per a poder llegir un nou fitxer o
					connectar amb un comandament */
				printf("c: Connecta el wiimote.\nl: Llegeix un fitxer de dades.\nx: Surt.\n");
				
				break;
				
			case 'x':
				/* si es tria la opció x (sortir) es finalitza el programa sense fer res més */
				return 0;
				exit = 1;
				sortir = 1;
				break;
				
			case '\n':
				break;
				
			default:
				printf("Opció incorrecta.\n");
				
		}
		
	}
	

	bdaddr = *BDADDR_ANY;
	
	/* Connectar el wiimote */
	printf("Prem els botons 1+2 del wiimote... \n");

	/* Intentem obrir la connexió */
	if( !(wiimote = cwiid_connect(&bdaddr,0)) ) 
	{
		printf("Impossible connectar.\n");
		return -1;
	}
	
	/* Creem el gallet per als events del wiimote */
	if( cwiid_set_mesg_callback(wiimote, cwiid_callback) )
	{
		printf("No es poden assignar events.\n");
		return -1;
	}
	
	printf("Wiimote connectat. \n\n");
	connectat = 1;
	
	/* Encendre els acceleròmetres */
	toggle_bit(rpt_mode, CWIID_RPT_ACC);
	set_rpt_mode(wiimote, rpt_mode);
	
	/* Imprimir l'estat del wiimote */
	printf("**************************\n");
	cwiid_get_state(wiimote, &state);
	print_state(&state);
	printf("**************************\n");
	
	/* Imprimeix el menú d'opcions del wiimote */
	printf("%s",MENU);
	

	/* Realitza l'acció requerida en triar una de les opcions del menú */
	while(!exit)
	{
		switch(getchar()) {
			case '1': /* Encendre un led */
				toggle_bit(led_state,CWIID_LED1_ON);
				set_led_state(wiimote, led_state);
				break;
				
			case '5': /* Encendre tots els led */
				if(led_state) led_state = 0; else led_state = 15;
				set_led_state(wiimote, led_state);
				break;
				
			case 'a': /* Activar els acceleròmetres */
				toggle_bit(rpt_mode, CWIID_RPT_ACC);
				set_rpt_mode(wiimote, rpt_mode);
				break;
				
			case 'd': /* Reportar acceleròmetres durant x vegades */
				print_report(wiimote);
				break;
				
			case 'r': /* Imprimir l'estat */
				if(cwiid_get_state(wiimote, &state))
					printf("Error en obtenir l'estat.\n");
				   
				print_state(&state);
				break;
			
			case 'f': /* Crear un fitxer amb les dades de la matriu */
				if(acc_xyz_count > 0){
					if(guardar_dades_en_fitxer(p_acc_xyz, acc_xyz_count)!=-1)
					{
						printf("S'ha guardat correctament.\n");
					}
					else
					{
						printf("Hi ha hagut errors mentre es guardava.\n");
					}
				}
				else
				{
					printf("No hi ha dades per guardar.\n");
				}
			
				break;
			
			case 'l': /* Demanar nom de fitxer a llegir i imprimir les dades */
				
				printf("Escriu el nom del fitxer a llegir:\n");
				scanf("%s", nom_fitxer_a_llegir);
			
				dades = llegir_fitxer_acc(nom_fitxer_a_llegir, &dades_acc_counter);
			
				print_all_reports(dades, dades_acc_counter);
			
				break;
				
			case 'x': /* Sortir */
				exit = -1;
				break;
				
			case '\n':
				break;
				
			default:
				printf("Opció no vàlida.\n");
		}
	}
	
	/* Desconnexió */
	if(connectat)
	{
		if(cwiid_disconnect(wiimote)) {
			
			printf("Error al desconnectar.\n");
			return -1;
		}
	}
	return 0;
	
}


/* Funció per encendre i apagar LED */
void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state)
{

	if( cwiid_command(wiimote, CWIID_CMD_LED, led_state) )
	   printf("Error amb els LED.\n");
	   
}


/* Funció per activar (i desactivar) el report de les acc. */
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode)
{

	if( cwiid_command(wiimote, CWIID_CMD_RPT_MODE, CWIID_RPT_ACC) )
		printf("Error en el mode de report.\n");
	
}


/* Funció per a imprimir l'estat del wiimote */
void print_state(struct cwiid_state *state)
{
	
	/* Imprimir l'estat del ACC */
	printf("Report de ACC: %s", ((state->rpt_mode & CWIID_RPT_ACC) ? "Activat\n" : "Desactivat\n"));
	
	/* Imprimir si hi ha algun LED actiu. */
	if(state->led) printf("Hi ha LEDs actius.\n"); else printf("LEDs desactivats.\n");
	
	/* Imprimir l'estat de la bateria */
	printf("Bateria: %d%%\n", (int)(100.0 * state->battery / CWIID_BATTERY_MAX) );
	
	/* Imprimir els valors dels acc */
	printf("x=%d  y=%d  z=%d\n", state->acc[CWIID_X], state->acc[CWIID_Y], state->acc[CWIID_Z]);
	
}


/* Funció per imprimir un nombre concret de acc seguit */
void print_report(cwiid_wiimote_t *wiimote)
{
	
	/* Activar l'enviament de dades d'acc i posar el comptador a 0 */
	cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC);
	
	reports = 0;
	
	envia = 1;
	
	/* cwiid_enable */
	
}


/* Funció assignada a l'event de quan el comandament envia informació a
	l'ordinador */
void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count, 
					union cwiid_mesg mesg[], struct timespec *timestamp)
{
	
	int i;
	
	/* Per cada missatge enviat des del comandament... */
	for(i=0; i < mesg_count; i++)
	{
		
		if(mesg[i].type == CWIID_MESG_ACC && envia == 1 && reports < MAX_N_RPT)
		{
			
			/* Imprimir les acceleracions en pantalla si la informació és d'acceleracions,
				estem reportant acceleracions i encara no hem arribat al màxim */

			
			report_acc(&mesg[i].acc_mesg);
			
				reports++;
				
				/* Posar el comandament en espera abans de tornar a reportar */
				espera(wiimote);
			
			
			if(reports >= MAX_N_RPT)
			{
				/* Si ja hem arribat al màxim de dades a imprimir */
				/* Desactiva l'enviament de dades continu */
				cwiid_disable(wiimote, CWIID_FLAG_MESG_IFC);
				
				t_inici = 0;
				
				/* imprimir el conjunt de reports des de l'array */
				print_all_reports(p_acc_xyz, acc_xyz_count);
				
				
				printf("%s", MENU);
	
			}
			/* else
			{
				
				espera(wiimote);
				
			} */
				
		}
		
	}
	
	
	
	/* cwiid_disable */
	
}


/* Funció que manté els reports del comandament desactivats durant un periode
	de temps definit per WAIT_TIME_MS, en milisegons */

/*int espera(clock_t inici, clock_t final)*/
void espera(cwiid_wiimote_t *wiimote)
{
	clock_t inici;
	
	int x, y;
	
	/* Inicialitza el temps */
	inici = clock();

	/* Desactiva la recepció de dades d'acc */
	envia = 0;
	cwiid_disable(wiimote, CWIID_FLAG_MESG_IFC); 
	
	/* Espera durant WAIT_TIME_MS mitjançant un bucle */
	while( (x = (int)(((clock() - inici)/(double)CLOCKS_PER_SEC)*1000.0)) < WAIT_TIME_MS ){ y++; }
	
	/* Activa la recepció de dades d'acc */
	envia = 1;
	cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC); 
	
}

/* Imprimeix les acceleracions de mesg i afegeix una entrada a 
	la matriu on es guarden les dades d'acc obtingudes */
void report_acc(struct cwiid_acc_mesg *mesg)
{
	/* Imprimeix les acc per pantalla */
	printf("x=%d \ty=%d \tz=%d\n", mesg->acc[CWIID_X], 
		   mesg->acc[CWIID_Y], mesg->acc[CWIID_Z]);
	
	/* Augmenta el tamany de la matriu de dades */
	p_acc_xyz = realloc(p_acc_xyz, (sizeof(struct stc_acc_xyz)*(acc_xyz_count+1)));
	acc_xyz_count++;
	
	/* Afegeix les dades d'acceleració juntament amb la marca de temps */
	(p_acc_xyz+acc_xyz_count-1) -> x = mesg -> acc[CWIID_X];
	(p_acc_xyz+acc_xyz_count-1) -> y = mesg -> acc[CWIID_Y];
	(p_acc_xyz+acc_xyz_count-1) -> z = mesg -> acc[CWIID_Z];
	(p_acc_xyz+acc_xyz_count-1) -> time = (int)((clock()/(double)CLOCKS_PER_SEC)*1000);
	
}

/* Imprimeix totes les entrades de la matriu d'acc */
void print_all_reports(struct stc_acc_xyz *xyz, int count)
{
	int x;
	
	/* Per cada entrada... */
	for(x=0; x<count; x++)
	{
		
		/* Imprimir la informació per pantalla */
		printf("%d ==> x: %d   y: %d   z: %d \n", (int)(xyz+x)->time, (xyz+x)->x, 
			   (xyz+x)->y, (xyz+x)->z);
		
	}

}

/* Guarda totes les dades de la matriu d'acc en un fitxer al directori
	del projecte amb un nombre de nom */
int guardar_dades_en_fitxer(struct stc_acc_xyz xyz[], int count)
{
	/* Obtenim el valor de temps actual per a donar el nom al fitxer */
	unsigned int ara = (unsigned int)time(0);

	char nom[11];
	int retorn;

	/* Transformem el nombre a una cadena per a utilitzar-la de nom */
	enter_a_string(ara, nom);

	FILE *fitxer;
	
	/* Creem el fitxer */
	fitxer = fopen(nom, "wb");
	
	/* Guardem les dades al fitxer */
	retorn = fwrite(xyz, sizeof(struct stc_acc_xyz), count, fitxer);
	
	/* Tanquem el fitxer */
	fclose(fitxer);
	
	/* Comprobem que s'hagi guardat tot */
	if(retorn < count) retorn = -1;
	
	return retorn;
	
}

/* Transforma un nombre enter a una cadena de caràcters */
void enter_a_string(unsigned int nombre, char nchar[])
{
	int x=0;

	/* Per cada una de les xifres del nombre */
	while((nombre > 0)&&(x<11))
	{
		/* Obtenir el valor de la xifra */
		nchar[x] = (nombre % 10) + '0';
		
		/* passar al següent nombre */
		nombre /= 10;

		x++;
	}
	
	/* Afegir caràcter de final de cadena */
	nchar[x] = 0;
		
}

/* Retorna el punter d'una matriu on hi ha les dades del fitxer d'acc especificat */
struct stc_acc_xyz *llegir_fitxer_acc(char *ruta, int *counter)
{

	/* Matriu on es guardaràn les dades */
	struct stc_acc_xyz *dades;
	
	/* Variable per a contar la quantitat de dades total */
	int contador=0;
	
	FILE *fitxer;
	
	/* Inicialitzem la matriu de dades */
	dades = malloc(sizeof(struct stc_acc_xyz));
	
	/* Obrim el fitxer especificat */
	fitxer = fopen(ruta, "rb");
	
	/* Mentre no arribem al final del fitxer anem llegint el que hi hagi */
	while(!feof(fitxer))
	{
		
		/* Augmentem la capacitat de la matriu de dades i incrementem el contador */
		dades = realloc(dades, (sizeof(struct stc_acc_xyz)*(contador+1)));
		contador++;
		
		/* Llegim una entrada del fitxer i la afegim a la matriu */
		fread((dades+contador-1), sizeof(struct stc_acc_xyz), 1, fitxer);
		
	}
	
	/* Tanquem el fitxer */
	fclose(fitxer);
	
	/* Retornem el valor del contador a la variable externa utilitzada de contador */
	*counter = contador-1;
	
	/* Retornem la matriu de dades */
	return dades;	
	
}