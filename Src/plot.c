/*
 * stab_snd.c
 * Copyright (c) 2004 Rice University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Rice
 * University not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Rice University makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.  
 *
 * RICE UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RICE UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Vinay Ribeiro, vinay@rice.edu.  */

/* plot.c is based on the gomoku software of George Lebl available at
   http://www-106.ibm.com/developerworks/library/l-gnomenclature/
*/

/* GNOME Piskvorky (gomoku),
   A 19x19 board where you have to connect 5 in a row to win
   (the board size is arbitrary)
   No artificial intelligence, you play both sides :) */
/* the main gnome include */
#include "stab_rcv.h"

#ifdef HAVE_GNOME_H
#include "gnome.h"

/* the board size */
#define BOARD_SIZE 19
/* border around the board itself */
#define BOARD_BORDER 15
/* cell size on the board */
#define CELL_SIZE 22
/* cell padding */
#define CELL_PAD 6
/* the height of the status area on the bottom of the canvas,
   it is the bottom border */
#define BOARD_STATUS (CELL_SIZE+5)
/* the thickness of lines */
#define THICKNESS 3
#define AXES_THICKNESS 1
/* the distance of the drop shadow */
#define DROP_SHADOW 2

#define NUM_TICKS 5


static double axis_x1 = (double)(BOARD_BORDER+CELL_SIZE+BOARD_STATUS);
static double axis_y1 = (double)(BOARD_BORDER +CELL_SIZE+ BOARD_SIZE*CELL_SIZE-2.1*BOARD_STATUS);
static double axis_x2 = (double)(BOARD_BORDER + CELL_SIZE+BOARD_SIZE*CELL_SIZE-2.1*BOARD_STATUS);
static double axis_y2 = (double)(BOARD_BORDER+CELL_SIZE);
static double x_shift = (double)(BOARD_BORDER + CELL_SIZE+BOARD_SIZE*CELL_SIZE);


/* the main window pointer */
static GnomeApp *app;
/* the canvas */
static GnomeCanvas *canvas;

/* the canvas group of the marks on the board */
static GnomeCanvasGroup *boardgroup1 = NULL;

/* double to string */
static void double2str(double num,char *str)
{
    sprintf(str,"%.1f",num);
}

/*  integer to string */
static void int2str(int num,char *str)
{  
  sprintf(str,"%d",num);
}



/* a utility function to draw a line */
static void
draw_a_line(GnomeCanvasGroup *group,
	    double x1, double y1, double x2, double y2, char *color,double thickness)
{
	GnomeCanvasPoints *points;

	/* allocate a new points array */
	points = gnome_canvas_points_new (2);

	/* fill out the points */
	points->coords[0] = x1;
	points->coords[1] = y1;
	points->coords[2] = x2;
	points->coords[3] = y2;
	/* draw the line */
	gnome_canvas_item_new(GNOME_CANVAS_GROUP(group),
			      gnome_canvas_line_get_type(),
			      "points", points,
			      "fill_color", color,
			      "width_units", thickness,
			      NULL);

	/* free the points array */
	gnome_canvas_points_free(points);
}


/* finds max value of array of double*/
static double find_max(double *val,int size)
{
  int i;
  double max;

  if(size<1)
    {
      fprintf(stderr,"size too small %d\n",size);
      exit(0);
    }
  max=val[0];
  for(i=0;i<size;i++)
    {
      if(max<val[i])
	max=val[i];
    }
  return(max);
}

/* finds max value of array of double*/
static double find_min(double *val,int size)
{
  int i;
  double min;

  if(size<1)
    {
      fprintf(stderr,"size too small %d\n",size);
      exit(0);
    }
  min=val[0];
  for(i=0;i<size;i++)
    {
      if(min>val[i])
	min=val[i];
    }
  return(min);
}

/* values to plot coordinates, yaxis always starts at zero */
static void convert_vals_to_plot_vals(double *xvals_in,double *yvals_in,double *xvals_out,double *yvals_out,int size,double xmax,double xmin,double ymax)
{
  int i;
  
  if(size<1)
    {
      fprintf(stderr,"size too small %d\n",size);
      exit(0);
    }

  if(xmin<0)
    {
      fprintf(stderr,"xmin too small %f\n",xmin);
      exit(0);
    }
  if(xmax<0)
    {
      fprintf(stderr,"xmax too small %f\n",xmax);
      exit(0);
    }
  if(ymax<0)
    {
      fprintf(stderr,"ymax too small %f\n",ymax);
      exit(0);
    }
  
  for (i=0;i<size;i++)
    {
      if (xmax>xmin)
	xvals_out[i]=(xvals_in[i]-xmin)*((axis_x2-axis_x1)/(xmax-xmin))+axis_x1;
      else
	xvals_out[i]=(axis_x2+axis_x1)/2;
      
      if (ymax>0)
	  yvals_out[i]=(yvals_in[i]/ymax)*(axis_y2-axis_y1)+axis_y1;
      else
	  yvals_out[i]=axis_y1;
    }
  
  return;  
}

/*put text at location*/
static void
put_text_at(GnomeCanvasGroup *group,char *str, double x, double y,int anchor)
{
  
  GnomeCanvasItem *item;
  
  item = gnome_canvas_item_new(GNOME_CANVAS_GROUP(group),
			       gnome_canvas_text_get_type(),
			       "x",x,
			       "y",y,
			       "anchor",anchor,
			       "font","fixed",
			       "fill_color", "black",
			       "text","",
			       NULL);
  
  /* set the text */
  gnome_canvas_item_set(item,
			"text",str,
			NULL);
}

/*shift in x value for plot*/
static double get_shift(int plot_no)
{
  double shift=0.0;
  if (plot_no>0)
    shift=x_shift;
  return(shift);
}

static void
set_title(GnomeCanvasGroup *group,int plot_no)
{
  char *text;
  char day[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  char month[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  char date[31][3]={"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31"};
  
  time_t x_time;
  struct tm *tm;
  double shift;
  
  shift=get_shift(plot_no);
  
  text=(char *)calloc((int)(50),sizeof(char));

  time( &x_time );
  tm=localtime(&x_time);
  
  if (plot_no==0)
      text=g_strdup_printf("Available Bandwidth       %s ----> %s [%s %s %s %d %.2d:%.2d]",hostname,localhost, day[tm->tm_wday],date[tm->tm_mday-1],month[tm->tm_mon],1900+tm->tm_year,tm->tm_hour,tm->tm_min);
  else
  text=g_strdup_printf("Probability of being a thin link");

      put_text_at(GNOME_CANVAS_GROUP(group),text,(double)(BOARD_BORDER + BOARD_SIZE*CELL_SIZE)/3.2+shift,(double)( BOARD_BORDER),GTK_ANCHOR_WEST);

}


/* draw all the tick labels*/
static void draw_tick_labels(GnomeCanvasGroup *group,double xmax,double xmin,double ymax,int plot_no)
{
  int i,num_x_tick_labels,x_tick_inc=1;
  char str[30];

  double x[NUM_TICKS+1],y[NUM_TICKS+1];
  double xplot[NUM_TICKS+1],yplot[NUM_TICKS+1];
  double shift;

  shift=get_shift(plot_no);

  /* y-tick values */
  for (i=0;i<=NUM_TICKS;i++)
    {
      y[i]=i*ymax/(NUM_TICKS);
      x[i]=xmin;/*just initialization*/
    }
  
  num_x_tick_labels=(int)(xmax-xmin);
  if (num_x_tick_labels<0)
    num_x_tick_labels=0;

  while(num_x_tick_labels>NUM_TICKS)
    {
      num_x_tick_labels=num_x_tick_labels/2;
      x_tick_inc=x_tick_inc*2;
    }

  /*x-tick values*/
  for (i=0;i<=num_x_tick_labels;i++)
    {
      x[i]=xmin+i*x_tick_inc;
      
    }
  
  convert_vals_to_plot_vals(x,y,xplot,yplot,NUM_TICKS+1,xmax,xmin,ymax);
  
  for (i=0;i<=num_x_tick_labels;i++)
    {
      int2str((int)x[i],str);
      put_text_at(GNOME_CANVAS_GROUP(group),str, shift+xplot[i],axis_y1+3*CELL_PAD,GTK_ANCHOR_CENTER);
      draw_a_line(GNOME_CANVAS_GROUP(group),shift+xplot[i],axis_y1-CELL_PAD,shift+xplot[i],axis_y1+CELL_PAD,"black",AXES_THICKNESS);
    }

 for (i=0;i<=NUM_TICKS;i++)
    {
      
      double2str(y[i],str);
      put_text_at(GNOME_CANVAS_GROUP(group),str, shift+axis_x1-CELL_PAD,yplot[i],GTK_ANCHOR_EAST);
    }
  
}

/*refresh plot*/
void refresh_plot()
{
  while (gtk_events_pending())
    {
      gtk_main_iteration();
    }
  return;
}

/* draw a circle onto group*/
static void
draw_a_circle(GnomeCanvasGroup *group,double x,double y,char *color)
{
	gnome_canvas_item_new(group,
			      gnome_canvas_ellipse_get_type(),
			      "x1",(double)(x-CELL_PAD/2),
			      "y1",(double)(y-CELL_PAD/2),
			      "x2",(double)(x + CELL_PAD/2),
			      "y2",(double)(y + CELL_PAD/2),
			      "outline_color",color,
			      "width_units", (double)THICKNESS,
			      NULL);
}


void clean_plot()
{
 GnomeCanvasItem *item;

  /* make a new group for the plot lines and axes labels. This will be destroyed each time we need to make a new plot */
  if(boardgroup1)
    {
      gtk_object_destroy(GTK_OBJECT(boardgroup1));
    }
  
  item = gnome_canvas_item_new(gnome_canvas_root(canvas),
			       gnome_canvas_group_get_type(),
			       "x",(double)0,
			       "y",(double)0,
			       NULL);
  
  boardgroup1 = GNOME_CANVAS_GROUP(item);
  refresh_plot();
}


/* plot a line given x,y coords*/
/*  x= pointer to x coords, y= pointer to y coords, size = num of x/y coords */
void make_plot(double *x, double *y, double *conf_int,int size,int plot_no)
{
  int i;
   double *xplot,*yplot,*y_plus_ci,*y_minus_ci,*y_plus_ci_plot,*y_minus_ci_plot;
  double xmax,ymax,xmin;
  double shift;
  GnomeCanvasPoints *points;

  if(size<1)
    {
      fprintf(stderr,"size too small %d\n",size);
      exit(0);
    }

  shift=get_shift(plot_no);    
  
  xplot=(double *)calloc((int)(size),sizeof(double));
  yplot=(double *)calloc((int)(size),sizeof(double));

  y_plus_ci=(double *)calloc((int)(size),sizeof(double));
  y_minus_ci=(double *)calloc((int)(size),sizeof(double));
  y_plus_ci_plot=(double *)calloc((int)(size),sizeof(double));
  y_minus_ci_plot=(double *)calloc((int)(size),sizeof(double));
  
  for (i=0;i<size;i++)
    {
      y_plus_ci[i]=y[i]+conf_int[i];
      y_minus_ci[i]=y[i]-conf_int[i];
      
      /*make sure no negative values */
      if(y[i]<0) 
	{
	  y[i]=0;
	  fprintf(stderr,"negative y\n");
	}
      
      if(y_minus_ci[i]<0) 
	y_minus_ci[i]=0;

      if(y_plus_ci[i]<0) 
	y_plus_ci[i]=0;
    }

  xmin=find_min(x,size);
  xmax=find_max(x,size);
  ymax=find_max(y_plus_ci,size);

  /*tick labels*/
  draw_tick_labels(GNOME_CANVAS_GROUP(boardgroup1),xmax,xmin,ymax,plot_no);

  /*title*/  
  set_title(GNOME_CANVAS_GROUP(boardgroup1),plot_no);

  convert_vals_to_plot_vals(x,y,xplot,yplot,size,xmax,xmin,ymax);
  convert_vals_to_plot_vals(x,y_plus_ci,xplot,y_plus_ci_plot,size,xmax,xmin,ymax);
  convert_vals_to_plot_vals(x,y_minus_ci,xplot,y_minus_ci_plot,size,xmax,xmin,ymax);

  /*draw line if more than one point*/

  if(size>1)
    {
      points = gnome_canvas_points_new(size);
      
      for (i=0;i<size;i++)
	{
	  points->coords[2*i] = xplot[i]+shift;
	  points->coords[2*i+1] = yplot[i];
	}
      
      gnome_canvas_item_new(GNOME_CANVAS_GROUP(boardgroup1),
			    gnome_canvas_line_get_type(),
			    "points", points,
			    "fill_color", "blue",
			    "width_units", (double)THICKNESS,
			    NULL);
    }
   /* draw circles and confidence intervals */
   for (i=0;i<size;i++)
    {
      draw_a_circle(GNOME_CANVAS_GROUP(boardgroup1),xplot[i]+shift,yplot[i],"red");
      draw_a_line(GNOME_CANVAS_GROUP(boardgroup1),xplot[i]+shift,y_minus_ci_plot[i],xplot[i]+shift,y_plus_ci_plot[i],"blue",THICKNESS);
    }
      
   refresh_plot();  
   return;
}


/* plot a line given x,y coords*/
/*  x= pointer to x coords, y= pointer to y coords, size = num of x/y coords */
void stem_plot(double *x, double *y, int size,int plot_no)
{
  int i;
  double *xplot,*yplot;
  double xmax,ymax,xmin;
  double shift;
  
  if(size<1)
    {
      fprintf(stderr,"size too small %d\n",size);
      exit(0);
    }
  
  shift=get_shift(plot_no);    
  
  xplot=(double *)calloc((int)(size),sizeof(double));
  yplot=(double *)calloc((int)(size),sizeof(double));
  
  for (i=0;i<size;i++)
    {
      /*make sure no negative values */
      if(y[i]<0) 
	{
	  y[i]=0;
	  fprintf(stderr,"negative y\n");
	}
    }

  xmin=find_min(x,size);
  xmax=find_max(x,size);
  ymax=find_max(y,size);

  /*tick labels*/
  draw_tick_labels(GNOME_CANVAS_GROUP(boardgroup1),xmax,xmin,ymax,plot_no);

  /*title*/  
  set_title(GNOME_CANVAS_GROUP(boardgroup1),plot_no);

  convert_vals_to_plot_vals(x,y,xplot,yplot,size,xmax,xmin,ymax);

   /* draw circles and confidence intervals */
   for (i=0;i<size;i++)
    {
      draw_a_circle(GNOME_CANVAS_GROUP(boardgroup1),xplot[i]+shift,yplot[i],"red");
      draw_a_line(GNOME_CANVAS_GROUP(boardgroup1),xplot[i]+shift,axis_y1,xplot[i]+shift,yplot[i],"blue",THICKNESS);
    }
      
   refresh_plot();  
   return;
}


/* draw axes. The origin is at x1,y1 and top right is at x2,y2 */
static void
draw_axes(GnomeCanvasGroup *group)
{
  int i;


  draw_a_line(GNOME_CANVAS_GROUP(group),axis_x1,axis_y1,axis_x2,axis_y1,"black",AXES_THICKNESS);
  draw_a_line(GNOME_CANVAS_GROUP(group),axis_x1,axis_y1,axis_x1,axis_y2,"black",AXES_THICKNESS);

  /* y-ticks */
  for(i=1;i<=NUM_TICKS;i++)
    {
      /*      draw_a_line(GNOME_CANVAS_GROUP(group),axis_x1+i*(axis_x2-axis_x1)/NUM_TICKS,axis_y1-CELL_PAD,axis_x1+i*(axis_x2-axis_x1)/NUM_TICKS,axis_y1+CELL_PAD,"black",AXES_THICKNESS);*/
      draw_a_line(GNOME_CANVAS_GROUP(group),axis_x1-CELL_PAD,axis_y1+i*(axis_y2-axis_y1)/NUM_TICKS,axis_x1+CELL_PAD,axis_y1+i*(axis_y2-axis_y1)/NUM_TICKS,"black",AXES_THICKNESS);
    }

  /* second set of axes */
  draw_a_line(GNOME_CANVAS_GROUP(group),x_shift+axis_x1,axis_y1,x_shift+axis_x2,axis_y1,"black",AXES_THICKNESS);
  draw_a_line(GNOME_CANVAS_GROUP(group),x_shift+axis_x1,axis_y1,x_shift+axis_x1,axis_y2,"black",AXES_THICKNESS);

  /* y-ticks */
  for(i=1;i<=NUM_TICKS;i++)
    {
      /*      draw_a_line(GNOME_CANVAS_GROUP(group),x_shift+axis_x1+i*(axis_x2-axis_x1)/NUM_TICKS,axis_y1-CELL_PAD,x_shift+axis_x1+i*(axis_x2-axis_x1)/NUM_TICKS,axis_y1+CELL_PAD,"black",AXES_THICKNESS);*/
      draw_a_line(GNOME_CANVAS_GROUP(group),x_shift+axis_x1-CELL_PAD,axis_y1+i*(axis_y2-axis_y1)/NUM_TICKS,x_shift+axis_x1+CELL_PAD,axis_y1+i*(axis_y2-axis_y1)/NUM_TICKS,"black",AXES_THICKNESS);
    }

}


/* draw the background of the playing board */
static void
draw_background(void)
{
	/* white background */
	gnome_canvas_item_new(gnome_canvas_root(canvas),
			      gnome_canvas_rect_get_type(),
			      "x1",(double)0,
			      "y1",(double)0,
			      "x2",(double)(BOARD_BORDER +
					    2*BOARD_SIZE*CELL_SIZE+BOARD_STATUS),
			      "y2",(double)(BOARD_BORDER +
					    BOARD_SIZE*CELL_SIZE+BOARD_STATUS),
			      "fill_color", "white",
			      NULL);
	

	draw_axes(gnome_canvas_root(canvas));

}


/* create the main window */
static GnomeApp *
create_window(void)
{
	GtkWidget *app;
	GtkWidget *w;
	

	app = gnome_app_new("STAB",
			    "STAB: Spatio-Temporal Available Bandwidth");
	
	/* we bind the delete event directly to gtk_main_quit,
	   since this function takes no arguments, there will be
	   no conflict in argument lists */
	gtk_signal_connect(GTK_OBJECT(app), "delete_event",
			   GTK_SIGNAL_FUNC(gtk_main_quit),
			   NULL);

	/* create a new canvas */
	w = gnome_canvas_new();
	canvas = GNOME_CANVAS(w);

	/* set where can the canvas scroll (our usable area) */
	gnome_canvas_set_scroll_region(canvas, 0.0, 0.0,
				       2*BOARD_SIZE*CELL_SIZE + 2*BOARD_BORDER,
				       BOARD_SIZE*CELL_SIZE + BOARD_BORDER +
				       BOARD_STATUS);
	/* set the size of the widget */
	gtk_widget_set_usize(w,
			     2*BOARD_SIZE*CELL_SIZE + 2*BOARD_BORDER,
			     BOARD_SIZE*CELL_SIZE + BOARD_BORDER +
			     BOARD_STATUS);

	/* set the contents of the app window to the canvas */
	gnome_app_set_contents(GNOME_APP(app), w);
	
	return GNOME_APP(app);
}


/*xlabel, ylabel, and title*/

static void init_text_vars()
{
  char str[50];
  GnomeCanvasItem *item;
double  shift;

  if(boardgroup1)
    {
      gtk_object_destroy(GTK_OBJECT(boardgroup1));
    }
  
  item = gnome_canvas_item_new(gnome_canvas_root(canvas),
			       gnome_canvas_group_get_type(),
			       "x",(double)0,
			       "y",(double)0,
			       NULL);
  
  boardgroup1 = GNOME_CANVAS_GROUP(item);

  strcpy(str,"WAITING FOR DATA!! \n");
  put_text_at(GNOME_CANVAS_GROUP(boardgroup1),str,(double)(axis_x1+axis_x2)/2,(double)(axis_y1+axis_y2)/2,GTK_ANCHOR_WEST); 


  /* ylabel */
  strcpy(str,"Mbps");
  put_text_at(gnome_canvas_root(canvas),str,(double)BOARD_BORDER,(double)(BOARD_SIZE*CELL_SIZE)/2,GTK_ANCHOR_CENTER);
  
  /* xlabel*/
  strcpy(str,"sub-path length");
 put_text_at(gnome_canvas_root(canvas),str,(double)(BOARD_BORDER + BOARD_SIZE*CELL_SIZE)/2.5,(double)(BOARD_SIZE*CELL_SIZE +BOARD_BORDER +BOARD_STATUS/2),GTK_ANCHOR_WEST);

 shift=get_shift(1);
  /* xlabel*/
  strcpy(str,"link number");
 put_text_at(gnome_canvas_root(canvas),str,(double)(BOARD_BORDER + BOARD_SIZE*CELL_SIZE)/2.1+shift,(double)(BOARD_SIZE*CELL_SIZE +BOARD_BORDER +BOARD_STATUS/2),GTK_ANCHOR_WEST);



}


/* our main function */
void init_plot()
{

  char **tmp;
  
  tmp=(char **)calloc((int)(2),sizeof(char *));
  tmp[0]=(char *)calloc((int)(128),sizeof(char));
  tmp[1]=NULL;

  strcpy(tmp[0],"init");
   gnome_init("STAB",VERSION,1,tmp);
  
  app = create_window();
  
  gtk_widget_show_all(GTK_WIDGET(app));
  
  draw_background();
  
  init_text_vars();
  
  /*	gtk_main();*/
  
   refresh_plot();
}

#else

void make_plot(double *x, double *y, double *conf_int,int size,int plot_no)
{
  return;
}

void init_plot()
{
  return;
}

void refresh_plot()
{
  return;
}

void clean_plot()
{
  return;
}

void stem_plot(double *x, double *y, int size,int plot_no)
{
  return;
}

#endif
