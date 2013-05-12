/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Roc Ramon 2008 <rcrmn16@gmail.com>
 * 
 */


#define APP_NAME		"AccWii"
#define APP_VERSION		PACKAGE_VERSION
#define APP_COPYRIGHT	"Copyright (C) 2008 Roc Ramon " \
                        "<rcrmn16@gmail.com>"
#define APP_COMMENTS	"Wiimote Acc Tool"


#include <ctype.h>
#include <float.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/stat.h>


#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

#include <bluetooth/bluetooth.h>
#include <cwiid.h>

#include <config.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "callbacks.h"




#define toggle_bit(bf,b)	\
	(bf) = ((bf) & b)		\
	       ? ((bf) & ~(b))	\
	       : ((bf) | (b))

#define WII_DESCONNECTAT "Sense connexió."
#define WII_CONNECTAT "Connexió establerta."


// Opcions de dibuix del gràfic
#define TEMPS_ENTRE_INFORMES_ACC_MS 100 // Temps mínim entre cada cop que s'agafen dades
#define PIXELS_PER_SEC 50 // Distància en píxels per cada segon
#define MS_ENTRE_MARQUES_DE_TEMPS 2000 // Milisegons que es deixen passar abans de posar una nova marca de temps
		// Colors per a les línies del gràfic
#define COLOR_LINIA_G a_colors.lila
#define COLOR_X a_colors.vermell
#define COLOR_Y a_colors.taronja
#define COLOR_Z a_colors.blau




/* For testing propose use the local (not installed) glade file */
/* #define GLADE_FILE PACKAGE_DATA_DIR"/prova_ui_1/glade/prova_ui_1.glade" */
#define GLADE_FILE "prova_ui_1.glade"

/***************************************************/
/****************Variables Globals******************/
/***************************************************/
	/* Punter al wiimote */
cwiid_wiimote_t *wiimote = NULL;

	/* Adreça del bluetooth */
bdaddr_t bdaddr;

	/* Estat del wiimote */
struct cwiid_state wiimote_state;

	/* Si està connectat el wiimote */
int wiimote_connected = 0;

	/* Estat dels LED */
unsigned char led_state = 0;

	/* Estat de la vibració */
unsigned char rumble_state = 0;

	/* Calibració dels acceleròmetres del wiimote */
struct acc_cal wm_calibracio, nc_calibracio;

	/* Estructura per guardar les dades per a la superfície de 
												la força G */
struct 
{
	int x;
	int z;
} dades_drawing_g;

	/* Variables per a guardar els temps */
double temps_inici, temps_fi;

	/* Estructura per guardar les acceleracions per al gràfic */
struct st_acc
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
	double time;
	struct st_acc *next_ptr;
};
struct st_acc *primeres_dades_acc_grafic = NULL;
struct st_acc *ultimes_dades_acc_grafic = NULL;
int count_acc;

	/* Variable de commutació de creació de gràfic */
int dibuixant_grafic = 0;

	/* Colors per als dibuixos */
struct st_colors{
	GdkGC *vermell;
	GdkGC *groc;
	GdkGC *blau;
	GdkGC *verd;
	GdkGC *taronja;
	GdkGC *lila;
	GdkGC *blanc;
	GdkGC *negre;
};

/***************************************************/
/************* Fi Variables Globals ****************/
/***************************************************/


/***************************************************/
/******************* Callbacks *********************/
/***************************************************/
	/* Callback del botó Connectar */
void button1_on_click(GtkWidget *widget, gpointer data);

	/* Callback del botó Desconnectar */
void button2_on_click(GtkWidget *widget, gpointer data);

	/* Callback del botó Comença (gràfic) */
void button3_on_click(GtkWidget *widget, gpointer data);

	/* Callback del botó Para (gràfic) */
void button4_on_click(GtkWidget *widget, gpointer data);

	/* Callback del botó Guardar Fitxer (gràfic) */
void button5_on_click(GtkWidget *widget, gpointer data);

	/* Callbacks per a les funcions del menú de commutació de LED */
void menu_toggle_LED1(GtkWidget *widget, gpointer data);

void menu_toggle_LED2(GtkWidget *widget, gpointer data);

void menu_toggle_LED3(GtkWidget *widget, gpointer data);

void menu_toggle_LED4(GtkWidget *widget, gpointer data);

	/* Callback del menú de funcions per la Vibració */
void awii_set_rumble();

	/* Callback per als events del wiimote */
cwiid_mesg_callback_t awii_events;

	/* Callback per al About */
void menuAbout_activate(void);

	/* Callback que dibuixarà a l'àrea de la força G */
void drawing_g_expose_event(void);

	/* Callback per dibuixar el gràfic */
void drawing_graphics_expose_event(void);

/***************************************************/
/**************** Fi dels Callback *****************/
/***************************************************/


/***************************************************/
/*************** Funcions Generals *****************/
/***************************************************/

	/* Reinicia tots els widgets en desconnectar el comandament */
void reinicia_tot();

	/* Reinicia tots els botons */
void reinicia_botons();

	/* Cambia la informació de la barra d'estat */
void status(GtkWidget *statusbar, const gchar *text);

	/* Crear una finestra de missatge */
void message(GtkMessageType type, const gchar *message, GtkWindow *parent);

	/* Connectar el wiimote */
int awii_connect();

	/* Desconnectar el wiimote */
int awii_disconnect();

	/* Callback del menú de funcions per als LED */
void awii_set_leds(int led);

	/* Encendre i apagar LED */
void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state);

	/* Actualitzar les dades d'ACC */
void awii_acc(struct cwiid_acc_mesg *acc_mesg);

	/* Dibuixa les gràfiques d'Acc a la superfície */
void dibuixa_grafics(struct cwiid_acc_mesg *acc_mesg);

	/* Inicialitza l'estructura de colors */
void inicialitza_colors(struct st_colors *a_colors);

	/* Retorna un double amb els milisegons actuals */
double temps_milisegons();

	/* Crea una nova estructura de dades del gràfic i en torna el punter */
struct st_acc *new_st_acc();

	/* Elimina una llista de dades del gràfic a partir de la primera */
void clear_st_acc(struct st_acc *first);

	/* Afegeix una estructura a la llista */
struct st_acc *add_st_acc(struct st_acc *st_final);

/***************************************************/
/************** Fi Funcions Generals ***************/
/***************************************************/


/***************************************************/
/******************** Widgets **********************/
/***************************************************/
GladeXML *gxml;
	GtkWidget *window;
	GtkWidget *buttonConnect;
	GtkWidget *buttonDisconnect;
	GtkWidget *buttonStartGraphic;
	GtkWidget *buttonStopGraphic;
	GtkWidget *buttonSaveFile;
	GtkWidget *chkDibuixaX;
	GtkWidget *chkDibuixaY;
	GtkWidget *chkDibuixaZ;
	GtkWidget *acc_x;
	GtkWidget *acc_y;
	GtkWidget *acc_z;
	GtkWidget *drawing_g;
	GtkWidget *acc_g;
	GtkWidget *drawing_graphics;
	GtkWidget *menuConnect;
	GtkWidget *menuDisconnect;
	GtkWidget *menuCmpLED1;
	GtkWidget *menuCmpLED2;
	GtkWidget *menuCmpLED3;
	GtkWidget *menuCmpLED4;
	GtkWidget *menuCmpVib;
	GtkWidget *menuHelpAbout;

/***************************************************/
/**************** Fi dels Widgets ******************/
/***************************************************/


/***************************************************/
/***************** create_window *******************/
/***************************************************/
GtkWidget* create_window (void)
{
	
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	/* This is important */
	glade_xml_signal_autoconnect (gxml);
	
	/***************************/
		/* lookup Widgets */
	/***************************/
	window = glade_xml_get_widget (gxml, "window");
	buttonConnect = glade_xml_get_widget (gxml, "button1");
	buttonDisconnect = glade_xml_get_widget (gxml, "button2");
	buttonStartGraphic = glade_xml_get_widget (gxml, "button3");
	buttonStopGraphic = glade_xml_get_widget (gxml, "button4");
	buttonSaveFile = glade_xml_get_widget (gxml, "button5");
	chkDibuixaX = glade_xml_get_widget (gxml, "chkDibuixaX");
	chkDibuixaY = glade_xml_get_widget (gxml, "chkDibuixaY");
	chkDibuixaZ = glade_xml_get_widget (gxml, "chkDibuixaZ");
	acc_x = glade_xml_get_widget (gxml, "acc_x");
	acc_y = glade_xml_get_widget (gxml, "acc_y");
	acc_z = glade_xml_get_widget (gxml, "acc_z");
	drawing_g = glade_xml_get_widget (gxml, "drawing_g");
	acc_g = glade_xml_get_widget (gxml, "acc_g");
	drawing_graphics = glade_xml_get_widget (gxml, "drawing_graphics");

		/* Widgets dels menús */
	menuConnect = glade_xml_get_widget (gxml, "menuitem2");
	menuDisconnect = glade_xml_get_widget (gxml, "menuitem3");
	menuCmpLED1 = glade_xml_get_widget (gxml, "menuitem6");
	menuCmpLED2 = glade_xml_get_widget (gxml, "menuitem7");
	menuCmpLED3 = glade_xml_get_widget (gxml, "menuitem8");
	menuCmpLED4 = glade_xml_get_widget (gxml, "menuitem9");
	menuCmpVib = glade_xml_get_widget (gxml, "menuitem10");
	menuHelpAbout = glade_xml_get_widget (gxml, "menuitem12");
	
	/***************************/
	
		/* Callbacks generals */
	g_signal_connect(menuHelpAbout, "activate", G_CALLBACK(menuAbout_activate),
	                 NULL);
	g_signal_connect(drawing_g, "expose_event", G_CALLBACK(drawing_g_expose_event), NULL);
	g_signal_connect(drawing_graphics, "expose_event", G_CALLBACK(drawing_graphics_expose_event), NULL);
	
	/* Assignar propietats als widgets */
	gtk_widget_set_sensitive(menuDisconnect, FALSE);
	
	reinicia_tot();
	
	return window;
}

/***************************************************/
/***************************************************/
/***************************************************/



/***************************************************/
/********************* MAIN ************************/
/***************************************************/

int main (int argc, char *argv[])
{
	
	gtk_set_locale ();
	gtk_init (&argc, &argv);

		
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
	gdk_threads_init();
	gdk_threads_enter();
	
	
	window = create_window ();
	
	g_signal_connect(G_OBJECT(window), "destroy", 
					 G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show (window);

	gtk_main ();
	return 0;
}

/***************************************************/
/******************** FI MAIN **********************/
/***************************************************/



/***************************************************/
/***************** Event Handlers ******************/
/***************************************************/

// Botó de connectar
void button1_on_click(GtkWidget *widget, gpointer data)
{
	if(!awii_connect())
	{
		gtk_widget_set_sensitive(buttonDisconnect, TRUE);
		gtk_widget_set_sensitive(menuDisconnect, TRUE);
		gtk_widget_set_sensitive(buttonConnect, FALSE);
		gtk_widget_set_sensitive(menuConnect, FALSE);
	}
}

		/******/

// Botó de desconnectar
void button2_on_click(GtkWidget *widget, gpointer data)
{
	if(!awii_disconnect())
	{
		gtk_widget_set_sensitive(buttonConnect, TRUE);
		gtk_widget_set_sensitive(menuConnect, TRUE);
		gtk_widget_set_sensitive(buttonDisconnect, FALSE);
		gtk_widget_set_sensitive(menuDisconnect, FALSE);
	}
}

		/******/

// Botó d'activar el gràfic
void button3_on_click(GtkWidget *widget, gpointer data)
{
	if(wiimote_connected == 1)
	{
		reinicia_tot();
		temps_inici = temps_milisegons();
		temps_fi = temps_inici;
		dibuixant_grafic = 1;
		
		primeres_dades_acc_grafic = new_st_acc();
		ultimes_dades_acc_grafic = primeres_dades_acc_grafic;
	}
	
	reinicia_botons();
	
}

		/******/

// Botó de parar el gràfic
void button4_on_click(GtkWidget *widget, gpointer data)
{
	if((wiimote_connected == 1) && (dibuixant_grafic == 1))
	{
		dibuixant_grafic = 0;
	}
	
	reinicia_botons();
	
}

		/******/


// Botó de guardar en fitxer
void button5_on_click(GtkWidget *widget, gpointer data)
{
	GtkWidget *filechooser;
	gint resposta;
	gchar *filename, final_filename[256];
	GtkFileFilter *filtre;
	FILE *fitxer;
	
	struct st_acc *st_actual;
	
	filtre = gtk_file_filter_new();
	gtk_file_filter_set_name(filtre, "Comma Separated Values");
	gtk_file_filter_add_pattern(filtre, "*.csv");
	
	filechooser = gtk_file_chooser_dialog_new("Desa...", 
											  GTK_WINDOW(window),
											  GTK_FILE_CHOOSER_ACTION_SAVE,
											  "Cancela", GTK_RESPONSE_CANCEL,
											  "Guarda", GTK_RESPONSE_ACCEPT,
											  NULL);
	
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(filechooser), TRUE);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(filechooser), filtre);
	
	resposta = gtk_dialog_run (GTK_DIALOG(filechooser));
	
	if(resposta == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(filechooser));
		
		if( (strstr( (&(filename[(strlen(filename)-5)])), ".csv")) == NULL )
		{
			snprintf(final_filename, 256, "%s.csv", filename);
		}
		else
		{
			snprintf(final_filename, 256, "%s", filename);
		}
		
		// Guardar el fitxer
		
		fitxer = fopen(final_filename, "w");
		
		st_actual = primeres_dades_acc_grafic;
		
		int x;
		for(x=1; x<count_acc; x++)
		{
			
			fprintf(fitxer, "\"%f\",%u,%u,%u\n", 
					st_actual -> time, st_actual -> x, st_actual -> y, st_actual -> z);
			
			if( (st_actual->next_ptr) != NULL )
			{
				st_actual = st_actual->next_ptr;
			}
			else
			{
				break;
			}
			
		}
		
		fclose(fitxer);
		
		message(GTK_MESSAGE_INFO, "Arxiu guardat correctament.", GTK_WINDOW(window));
		
	}
	
	gtk_widget_destroy(filechooser);
	
	
}

		/******/

void menuAbout_activate(void)
{
	gtk_show_about_dialog(GTK_WINDOW(window),
	                      "name", APP_NAME,
	                      "version", APP_VERSION,
	                      "copyright", APP_COPYRIGHT,
	                      "comments", APP_COMMENTS,
	                      NULL);
}

		/******/

void menu_toggle_LED1(GtkWidget *widget, gpointer data)
{
	awii_set_leds(1);
}

		/******/

void menu_toggle_LED2(GtkWidget *widget, gpointer data)
{
	awii_set_leds(2);
}

		/******/

void menu_toggle_LED3(GtkWidget *widget, gpointer data)
{
	awii_set_leds(3);
}

		/******/

void menu_toggle_LED4(GtkWidget *widget, gpointer data)
{
	awii_set_leds(4);
}

		/******/

void awii_set_rumble()
{
	
	toggle_bit(rumble_state, 1);
	
	if (cwiid_set_rumble(wiimote, rumble_state)) 
	{
		message(GTK_MESSAGE_ERROR, "Error commutant la vibració.", GTK_WINDOW(window));
	} 
	
}

		/******/

void awii_events(cwiid_wiimote_t *wiimote, int mesg_count,
                    union cwiid_mesg mesg_array[], struct timespec *timestamp)
{
	if(wiimote_connected==0)
	{
		return;
	}
	
	int i;	
	
	gdk_threads_enter();
	
	for(i=0; i<mesg_count; i++)
	{
		switch (mesg_array[i].type)
		{
				
			case CWIID_MESG_ACC:
				awii_acc(&mesg_array[i].acc_mesg);
				break;
				
			case CWIID_MESG_ERROR:
				awii_disconnect();
				message( GTK_MESSAGE_ERROR, "Error en rebre informes.", GTK_WINDOW(window) );
				break;
				
			default:
				break;
		}
		
	}
	
	
	gdk_flush();
	gdk_threads_leave();
	
}

		/******/

#define MIDA_QUADRAT_F_G 150
void drawing_g_expose_event(void)
{
	gint width = MIDA_QUADRAT_F_G, height = MIDA_QUADRAT_F_G;
	int x, y;
	struct st_colors a_colors;
	
	inicialitza_colors(&a_colors);
	x = (((double)(dades_drawing_g.x - (0xFF/2))/0xFF)*MIDA_QUADRAT_F_G/2)+MIDA_QUADRAT_F_G/2;
	y = (((double)(dades_drawing_g.z - (0xFF/2))/0xFF)*MIDA_QUADRAT_F_G/2)+MIDA_QUADRAT_F_G/2;
	
	gdk_draw_arc(drawing_g->window, 
				 drawing_g->style->fg_gc[GTK_WIDGET_STATE(drawing_g)],
				 FALSE,
				 0, 0, width -1, height-1, 0, 64*360);
	
	/* Dibuixar punt d'acc */
	if(wiimote_connected != 0)
	{
		gdk_draw_arc(drawing_g->window,
					 a_colors.vermell,
					 TRUE,
					 x, y, 6, 6, 0, 64*360);
	}
	
		/* Dibuixar creu central */	
	gdk_draw_line(drawing_g->window,
				 a_colors.taronja,
				 72, 75, 78, 75);
	gdk_draw_line(drawing_g->window,
				 a_colors.taronja,
				 75, 72, 75, 78);
	
}


#define WIDTH_GRAFIC_ACC 590
#define HEIGHT_GRAFIC_ACC 360
#define TOP_DISTANCE_GRAFIC 15
#define LEFT_DISTANCE_GRAFIC 15
#define RIGHT_DISTANCE_GRAFIC 45
#define GRAFIC drawing_graphics->window

void drawing_graphics_expose_event(void)
{
	gint width = WIDTH_GRAFIC_ACC, height = HEIGHT_GRAFIC_ACC;
	
	gdk_drawable_get_size(GRAFIC, &width, &height);
	
	struct st_colors a_colors;
	struct st_acc *acc_actual;
	double g_one;
	int y_g_one_p, y_g_one_n;
	
	inicialitza_colors(&a_colors);
	
	// Dibuixar els eixos de coordenades
	gdk_draw_line(GRAFIC,
				  a_colors.negre,
				  LEFT_DISTANCE_GRAFIC, TOP_DISTANCE_GRAFIC, LEFT_DISTANCE_GRAFIC, (height-TOP_DISTANCE_GRAFIC));
	gdk_draw_line(GRAFIC,
				  a_colors.negre,
				  0, (height/2), (width-20), (height/2));
	gdk_draw_layout(GRAFIC,
					a_colors.negre,
					0, ((height/2)-15),
					gtk_widget_create_pango_layout(drawing_graphics, "0"));
	
	// Dibuixar punts de G=1, G=-1
	g_one = ((double)143 - 119)/(0xFF -119);
	y_g_one_p = (height/2) - (((height/2) - TOP_DISTANCE_GRAFIC)*g_one);
	y_g_one_n = (height/2) + (((height/2) - TOP_DISTANCE_GRAFIC)*g_one);
	
	gdk_draw_line(GRAFIC,
				  COLOR_LINIA_G,
				  10,y_g_one_p,(width-20),y_g_one_p);
	gdk_draw_line(GRAFIC,
				  COLOR_LINIA_G,
				  10,y_g_one_n,(width-20),y_g_one_n);
	gdk_draw_layout(GRAFIC,
					COLOR_LINIA_G,
					0, y_g_one_p-15,
					gtk_widget_create_pango_layout(drawing_graphics, "G"));
	gdk_draw_layout(GRAFIC,
					COLOR_LINIA_G,
					0, y_g_one_n-15,
					gtk_widget_create_pango_layout(drawing_graphics, "-G"));
	
	// Dibuixar els gràfics
	if(dibuixant_grafic == 1)
	{
		int y_total=16, dy;
		int x1, x2, y1, y2, z1, z2;
		double pos1, pos2;
		
		acc_actual = primeres_dades_acc_grafic;
		
		int x;
		for(x=0; x<count_acc-1; x++)
		{
			
			// Dibuixar linies d'acceleracions
			if ( (int)((count_acc - x) * TEMPS_ENTRE_INFORMES_ACC_MS/1000 * PIXELS_PER_SEC) <= (width - RIGHT_DISTANCE_GRAFIC)) //(LEFT_DISTANCE_GRAFIC+1+20)))
			{
				dy = 0;
				
				x1 = acc_actual -> x;
				y1 = acc_actual -> y;
				z1 = acc_actual -> z;
				
				x2 = x1;
				y2 = y1;
				z2 = z1;
				
				if((count_acc > 1) && ((acc_actual -> next_ptr) != NULL))
				{
					dy = PIXELS_PER_SEC * (((acc_actual -> next_ptr) -> time) - (acc_actual -> time)) / 1000;
					
					x2 = ((acc_actual -> next_ptr) -> x);
					y2 = ((acc_actual -> next_ptr) -> y);
					z2 = ((acc_actual -> next_ptr) -> z);
				}
				
				// Dibuixar marques de temps de l'eix x
				if( ((int)(acc_actual -> time) % MS_ENTRE_MARQUES_DE_TEMPS) == 0 )
				{
					gdk_draw_line(GRAFIC,
								  a_colors.negre,
								  y_total, (height/2), y_total, ((height/2)+5));
					
					char numero_segons[8];
					snprintf(numero_segons, 7, "%d", (int)((acc_actual->time)/MS_ENTRE_MARQUES_DE_TEMPS) );
					gdk_draw_layout(GRAFIC,
									a_colors.negre,
									(y_total+2), (height/2),
									gtk_widget_create_pango_layout(drawing_graphics, numero_segons));
				}
				
				// Dibuixar linies de gràfic
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkDibuixaX)))
				{
					pos1 = ((double)x1 - 119)/(0xFF -119);
					pos2 = ((double)x2 - 119)/(0xFF -119);
					x1 = (height/2) - (((height/2) - TOP_DISTANCE_GRAFIC)*pos1);
					x2 = (height/2) - (((height/2) - TOP_DISTANCE_GRAFIC)*pos2);
					
					gdk_draw_line(GRAFIC,
								  COLOR_X,
								  y_total, x1, (y_total+dy), x2);
				}
				
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkDibuixaY)))
				{
					pos1 = ((double)y1 - 119)/(0xFF -119);
					pos2 = ((double)y2 - 119)/(0xFF -119);
					y1 = (height/2) - (((height/2) - TOP_DISTANCE_GRAFIC)*pos1);
					y2 = (height/2) - (((height/2) - TOP_DISTANCE_GRAFIC)*pos2);
					
					gdk_draw_line(GRAFIC,
								  COLOR_Y,
								  y_total, y1, (y_total+dy), y2);
				}
				
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkDibuixaZ)))
				{
					pos1 = ((double)z1 - 119)/(0xFF -119);
					pos2 = ((double)z2 - 119)/(0xFF -119);
					z1 = (height/2) - (((height/2) - TOP_DISTANCE_GRAFIC)*pos1);
					z2 = (height/2) - (((height/2) - TOP_DISTANCE_GRAFIC)*pos2);
					
					gdk_draw_line(GRAFIC,
								  COLOR_Z,
								  y_total, z1, (y_total+dy), z2);
				}
				
				
				y_total = y_total + dy;
				
			}
			
			if((acc_actual -> next_ptr) != NULL)
			{
				acc_actual = (acc_actual -> next_ptr);
			}
			else
			{
				return;
			}
			
		}
	}
	else
	{
		// Dibuixar les marques de temps en descans
		int n;
		
		for(n=0; n< (width-RIGHT_DISTANCE_GRAFIC); n++)
		{
			if( ((n%PIXELS_PER_SEC) == 0) && ((int)(n/PIXELS_PER_SEC)%(MS_ENTRE_MARQUES_DE_TEMPS/1000)) == 0 )
			{
				
				gdk_draw_line(GRAFIC,
								a_colors.negre,
								n+LEFT_DISTANCE_GRAFIC+1, (height/2), n+LEFT_DISTANCE_GRAFIC+1, ((height/2)+5));
				char nombre_segons[8];
					snprintf(nombre_segons, 7, "%d", (int)((n/PIXELS_PER_SEC)/(MS_ENTRE_MARQUES_DE_TEMPS/1000)) );
					gdk_draw_layout(GRAFIC,
									a_colors.negre,
									(n+LEFT_DISTANCE_GRAFIC+4), (height/2),
									gtk_widget_create_pango_layout(drawing_graphics, nombre_segons));
			}
		}
	}
	
}
/***************************************************/
/*************** End Event Handlers ****************/
/***************************************************/



/***************************************************/
/*************** Funcions Generals *****************/
/***************************************************/

void reinicia_tot()
{
	// Reinicia els botons
	reinicia_botons();
	
	// Reinicia les barres
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_x), 
								  0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_y), 
								  0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_z), 
								  0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_g), 
								  0.0);
	// Reinicia el dibuix de la F G
	dades_drawing_g.x = 0;
	dades_drawing_g.z = 0;
	gtk_widget_queue_draw(drawing_g);
	
	// Reinicia les dades dels punters de memoria del gràfic
	ultimes_dades_acc_grafic = NULL;
	dibuixant_grafic = 0;
	count_acc = 0;
	gtk_widget_queue_draw(drawing_graphics);
	
	/* Alliberar tots els punters de dades fets servir pel gràfic */
	if(primeres_dades_acc_grafic != NULL)
	{
		clear_st_acc(primeres_dades_acc_grafic);
		primeres_dades_acc_grafic = NULL;
	}
	
}

void reinicia_botons()
{
	if(wiimote_connected == 1)
	{
		
		gtk_widget_set_sensitive(buttonDisconnect, TRUE);
		gtk_widget_set_sensitive(buttonConnect, FALSE);
		if(dibuixant_grafic == 1)
		{
			gtk_widget_set_sensitive(buttonStartGraphic, TRUE);
			gtk_widget_set_sensitive(buttonStopGraphic, TRUE);
			gtk_widget_set_sensitive(chkDibuixaX, FALSE);
			gtk_widget_set_sensitive(chkDibuixaY, FALSE);
			gtk_widget_set_sensitive(chkDibuixaZ, FALSE);
		}
		else if (dibuixant_grafic == 0)
		{
			gtk_widget_set_sensitive(buttonStartGraphic, TRUE);
			gtk_widget_set_sensitive(buttonStopGraphic, FALSE);
			gtk_widget_set_sensitive(chkDibuixaX, TRUE);
			gtk_widget_set_sensitive(chkDibuixaY, TRUE);
			gtk_widget_set_sensitive(chkDibuixaZ, TRUE);
		}
		
		if (primeres_dades_acc_grafic == NULL)
		{
			gtk_widget_set_sensitive(buttonSaveFile, FALSE);
		}
		else 
		{
			gtk_widget_set_sensitive(buttonSaveFile, TRUE);
		}
		
	}
	else if(wiimote_connected == 0)
	{
		gtk_widget_set_sensitive(buttonDisconnect, FALSE);
		gtk_widget_set_sensitive(buttonConnect, TRUE);
		gtk_widget_set_sensitive(buttonStartGraphic, FALSE);
		gtk_widget_set_sensitive(buttonStopGraphic, FALSE);
		gtk_widget_set_sensitive(buttonSaveFile, FALSE);
		gtk_widget_set_sensitive(chkDibuixaX, FALSE);
		gtk_widget_set_sensitive(chkDibuixaY, FALSE);
		gtk_widget_set_sensitive(chkDibuixaZ, FALSE);
	}
}

		/******/

void status(GtkWidget *statusbar, const gchar *text)
{
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, text);
}

		/******/

void message(GtkMessageType type, const gchar *message, GtkWindow *parent)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(parent, 0, type, GTK_BUTTONS_OK, message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

		/******/

int awii_connect()
{
	if(wiimote_connected)
	{
		return -1;
	}
	
	char reset_bdaddr = 0;
	
	if (bacmp(&bdaddr, BDADDR_ANY) == 0) {
		reset_bdaddr = 1;
	}
	message(GTK_MESSAGE_INFO,
			"Prem els botons 1 + 2 del comandament per a connectar-lo i prem OK.",
			GTK_WINDOW(window));
	if ((wiimote = cwiid_connect(&bdaddr, (CWIID_FLAG_MESG_IFC | CWIID_FLAG_CONTINUOUS))) == NULL) {
		message(GTK_MESSAGE_ERROR, "Impossible connectar.", GTK_WINDOW(window));
		wiimote_connected = 0;
	}
	else if (cwiid_set_mesg_callback(wiimote, &awii_events)) {
		message(GTK_MESSAGE_ERROR, "Error marcant els events.",
				GTK_WINDOW(window));

		awii_disconnect();
		
		wiimote = NULL;
		wiimote_connected = 0;struct st_acc *add_st_acc(struct st_acc *st_final);
	}
	else {
		
		wiimote_connected = 1;
		if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NONE, &wm_calibracio)) 
		{
			message(GTK_MESSAGE_ERROR, "Impossible obtenir calibració dels acceleròmetres", 
					GTK_WINDOW(window));
			
			awii_disconnect();
			
			wiimote = NULL;
			wiimote_connected = 0;
		}

		if( cwiid_command(wiimote, CWIID_CMD_RPT_MODE, CWIID_RPT_ACC) )
		{
			message(GTK_MESSAGE_ERROR, "No s'ha pogut fixar el mode de connexió.", 
					GTK_WINDOW(window));

			awii_disconnect();
			
			wiimote = NULL;
			wiimote_connected = 0;
		}
	}
	if(wiimote_connected)
	{		
		//cwiid_enable(wiimote, CWIID_FLAG_CONTINUOUS);
		if( cwiid_command(wiimote, CWIID_CMD_STATUS, 0) )
		{
			message(GTK_MESSAGE_ERROR, "Error en obtenir l'estat.", GTK_WINDOW(window));
		}

		message(GTK_MESSAGE_INFO, "Wiimote connectat!", GTK_WINDOW(window));
	}
	
	if (reset_bdaddr) {
		bdaddr = *BDADDR_ANY;
	}
	
	reinicia_botons();
	
	// Reinicia els checkbox del gràfic
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDibuixaX), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDibuixaY), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDibuixaZ), TRUE);
	
	return !(wiimote_connected);
}

		/******/

int awii_disconnect()
{
	if (cwiid_close(wiimote)) {
		message(GTK_MESSAGE_ERROR, "Error en desconnectar.", GTK_WINDOW(window));
	}
	wiimote = NULL;
	wiimote_connected = 0;
	
	reinicia_tot();
	
	// Reinicia els checkbox del gràfic
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDibuixaX), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDibuixaY), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDibuixaZ), TRUE);
	
	return 0;
}

		/******/

void awii_set_leds(int led)
{
	
	switch (led)
	{
		case 1:
			toggle_bit(led_state, CWIID_LED1_ON);
			break;
		case 2:
			toggle_bit(led_state, CWIID_LED2_ON);
			break;
		case 3:
			toggle_bit(led_state, CWIID_LED3_ON);
			break;
		case 4:
			toggle_bit(led_state, CWIID_LED4_ON);
			break;
		default:
			return;
	}
	set_led_state (wiimote, led_state);
	
}

		/******/

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state)
{

	if( cwiid_command(wiimote, CWIID_CMD_LED, led_state) )
	{
	   message(GTK_MESSAGE_ERROR, "Error en commutar els LED.", GTK_WINDOW(window));
	}
	   
}

		/******/

void awii_acc(struct cwiid_acc_mesg *acc_mesg)
{
	if(wiimote_connected == 0)
	{
		reinicia_tot();
		return;
	}
	//double a_x, a_y, a_z;
	// double roll, pitch, a;
	double temps;	
	
	/* Barres d'Acc */
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_x), 
								  ((double)(acc_mesg->acc[CWIID_X])/0xFF));
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_y), 
								  ((double)(acc_mesg->acc[CWIID_Y])/0xFF));
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_z), 
								  ((double)(acc_mesg->acc[CWIID_Z])/0xFF));
	
	
	/* Dibuix Força G */
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(acc_g), 
								  ((double)(acc_mesg->acc[CWIID_Y])/0xFF));
	dades_drawing_g.x = (int)acc_mesg->acc[CWIID_X];
	dades_drawing_g.z = (int)acc_mesg->acc[CWIID_Z];
	gtk_widget_queue_draw(drawing_g);
	
	/* Gràfic acceleracions */
	temps = temps_milisegons();
	
	if ((dibuixant_grafic == 1) && (temps >= (temps_fi + TEMPS_ENTRE_INFORMES_ACC_MS)))
	{
		temps_fi = temps_fi + TEMPS_ENTRE_INFORMES_ACC_MS;
		// Afegir les dades de les acceleracions actuals
		ultimes_dades_acc_grafic -> x = (double)(acc_mesg->acc[CWIID_X]);
		ultimes_dades_acc_grafic -> y = (double)(acc_mesg->acc[CWIID_Y]);
		ultimes_dades_acc_grafic -> z = (double)(acc_mesg->acc[CWIID_Z]);
		ultimes_dades_acc_grafic -> time = temps_fi - temps_inici;
		
		count_acc++;
		
		// Afegir una nova entrada a la llista
		ultimes_dades_acc_grafic = add_st_acc(ultimes_dades_acc_grafic);
		
		// Redibuixar el gràfic
		gtk_widget_queue_draw(drawing_graphics);
		
	}

}

		/******/

void inicialitza_colors(struct st_colors *a_colors)
{
	GdkColor color;
	
	a_colors->vermell = gdk_gc_new(drawing_g->window);
	a_colors->groc = gdk_gc_new(drawing_g->window);
	a_colors->blau = gdk_gc_new(drawing_g->window);
	a_colors->verd = gdk_gc_new(drawing_g->window);
	a_colors->taronja = gdk_gc_new(drawing_g->window);
	a_colors->lila = gdk_gc_new(drawing_g->window);
	a_colors->blanc = gdk_gc_new(drawing_g->window);
	a_colors->negre = gdk_gc_new(drawing_g->window);
	
	//Vermell FF0000
	color.red = 65535;
	color.green = 0;
	color.blue = 0;
	gdk_gc_set_rgb_fg_color(a_colors->vermell, &color);

	//Blau 00FF00
	color.red = 0;
	color.green = 10000;
	color.blue = 65535;
	gdk_gc_set_rgb_fg_color(a_colors->blau, &color);
	
	//Verd FF0000
	color.red = 0;
	color.green = 65535;
	color.blue = 0;
	gdk_gc_set_rgb_fg_color(a_colors->verd, &color);
	
	//Groc FFFF00
	color.red = 65535;
	color.green = 65535;
	color.blue = 0;
	gdk_gc_set_rgb_fg_color(a_colors->groc, &color);
	
	//Taronja FF7F00
	color.red = 65535;
	color.green = 32767;
	color.blue = 0;
	gdk_gc_set_rgb_fg_color(a_colors->taronja, &color);
	
	//Lila 7F00FF
	color.red = 40000;
	color.green = 0;
	color.blue = 65535;
	gdk_gc_set_rgb_fg_color(a_colors->lila, &color);
	
	//Blanc FFFFFF
	color.red = 65535;
	color.green = 65535;
	color.blue = 65535;
	gdk_gc_set_rgb_fg_color(a_colors->blanc, &color);
	
	//Negre
	color.red = 0;
	color.green = 0;
	color.blue = 0;
	gdk_gc_set_rgb_fg_color(a_colors->negre, &color);
	
	
	
}

double temps_milisegons()
{
	struct timeval temps;
	double milisegons_davui, segons_davui;
	long int total;
	double segons_abans_davui;
		
	gettimeofday(&temps, NULL);
	
	segons_abans_davui = (double)temps.tv_sec / (3600 * 24);
		
	//return ((long double)ms);
	//return ((long double)temps.tv_usec);
	
	total = ((long int)segons_abans_davui) * 3600 * 24;
	
	segons_davui = temps.tv_sec - total;
	
	//return ((long double)segons_davui);
	
	milisegons_davui = (double)((segons_davui * 1000) + (int)(temps.tv_usec/1000));
	
	return milisegons_davui;
	
}


struct st_acc *new_st_acc()
{
	struct st_acc *nova_estructura;
	
	nova_estructura = ((struct st_acc *)(malloc(sizeof(struct st_acc))));
	
	nova_estructura->next_ptr = NULL;
	
	return nova_estructura;
	
}


struct st_acc *add_st_acc(struct st_acc *st_final)
{
	
	struct st_acc *new_st;
	
	new_st = new_st_acc();
	
	st_final -> next_ptr = new_st;
	
	return new_st;
}


void clear_st_acc(struct st_acc *first)
{
	struct st_acc *previous, *next;
	
	previous = first;
	
	while(previous->next_ptr != NULL)
	{
		next = previous -> next_ptr;
		free(previous);
		previous = next;
	}
	
}


