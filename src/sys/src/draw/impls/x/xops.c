
#include <stdio.h>
#include "ximpl.h"

#define XTRANS(win,xwin,x) \
   (int)(((xwin)->w)*((win)->port_xl + (((x - (win)->coor_xl)*\
                                   ((win)->port_xr - (win)->port_xl))/\
                                   ((win)->coor_xr - (win)->coor_xl))));
#define YTRANS(win,xwin,y) \
   (int)(((xwin)->h)*(1.0-(win)->port_yl - (((y - (win)->coor_yl)*\
                                       ((win)->port_yr - (win)->port_yl))/\
                                       ((win)->coor_yr - (win)->coor_yl))));

/*
    Defines the operations for the X Draw implementation.
*/

int XiDrawLine(DrawCtx Win, double xl, double yl, double xr, double yr,
                int cl, int cr)
{
  XiWindow* XiWin = (XiWindow*) Win->data;
  int       x1,y1,x2,y2, c = (cl + cr)/2;
  XiSetColor( XiWin, c );
  x1 = XTRANS(Win,XiWin,xl);   x2  = XTRANS(Win,XiWin,xr); 
  y1 = YTRANS(Win,XiWin,yl);   y2  = YTRANS(Win,XiWin,yr); 
  XDrawLine( XiWin->disp, XiDrawable(XiWin), XiWin->gc.set, x1, y1, x2, y2);
  return 0;
}

int XiDrawText(DrawCtx Win,double x,double  y,int c,char *chrs )
{
  int    xx,yy;
  XiWindow* XiWin = (XiWindow*) Win->data;
  xx = XTRANS(Win,XiWin,x);  yy = YTRANS(Win,XiWin,y);
  XiSetColor( XiWin, c );
  XDrawString( XiWin->disp, XiDrawable(XiWin), XiWin->gc.set,
               xx, yy - XiWin->font->font_descent, chrs, strlen(chrs) );
  return 0;
}

int XiFontFixed( XiWindow*,int, int,XiFont **);
int XiDrawTextSize(DrawCtx Win,double x,double  y)
{
  XiWindow* XiWin = (XiWindow*) Win->data;
  int       w,h;
  w = (int)((XiWin->w)*x*(Win->port_xr - Win->port_xl)/
                                   (Win->coor_xr - Win->coor_xl));
  h = (int)((XiWin->h)*y*(Win->port_yr - Win->port_yl)/
                                   (Win->coor_yr - Win->coor_yl));
  return XiFontFixed( XiWin,w, h, &XiWin->font);
}

int XiDrawTextVertical(DrawCtx Win,double x,double  y,int c,char *chrs )
{
  int       xx,yy,n = strlen(chrs),i;
  XiWindow* XiWin = (XiWindow*) Win->data;
  char      tmp[2];
  double    tw,th;
  
  tmp[1] = 0;
  XiSetColor( XiWin, c );
  XiDrawTextGetSize(Win,&tw,&th);
  xx = XTRANS(Win,XiWin,x);
  for ( i=0; i<n; i++ ) {
    tmp[0] = chrs[i];
    yy = YTRANS(Win,XiWin,y-th*i);
    XDrawString( XiWin->disp, XiDrawable(XiWin), XiWin->gc.set,
               xx, yy - XiWin->font->font_descent, tmp, 1 );
  }
  return 0;
}


int XiDrawTextGetSize(DrawCtx Win,double *x,double  *y)
{
  XiWindow* XiWin = (XiWindow*) Win->data;
  double    w,h;
  w = XiWin->font->font_w; h = XiWin->font->font_h;
  *x = w*(Win->coor_xr - Win->coor_xl)/
         (XiWin->w)*(Win->port_xr - Win->port_xl);
  *y = h*(Win->coor_yr - Win->coor_yl)/
         (XiWin->h)*(Win->port_yr - Win->port_yl);
  return 0;
}

int XiFlush(DrawCtx Win )
{
  XiWindow* XiWin = (XiWindow*) Win->data;
  if (XiWin->drw) {
    XCopyArea( XiWin->disp, XiWin->drw, XiWin->win, XiWin->gc.set, 0, 0, 
	       XiWin->w, XiWin->h, XiWin->x, XiWin->y );
  }
  XFlush( XiWin->disp );
  return 0;
}

int Xiviewport(DrawCtx Win,double xl,double yl,double xr,double yr)
{
  XiWindow*  XiWin = (XiWindow*) Win->data;
  XRectangle box;
  box.x = (int) (xl*XiWin->w);   box.y = (int) ((1.0-yr)*XiWin->h);
  box.width = (int) ((xr-xl)*XiWin->w);box.height = (int) ((yr-yl)*XiWin->h);
  XSetClipRectangles(XiWin->disp,XiWin->gc.set,0,0,&box,1,Unsorted);
  return 0;
}

int XiClearWindow(DrawCtx Win)
{
  XiWindow*  XiWin = (XiWindow*) Win->data;
  int        x,  y,  w,  h;
  x = (int) (Win->port_xl*XiWin->w);
  w = (int) ((Win->port_xr - Win->port_xl)*XiWin->w);
  y = (int) ((1.0-Win->port_yr)*XiWin->h);
  h = (int) ((Win->port_yr - Win->port_yl)*XiWin->h);
  XiSetPixVal(XiWin, XiWin->background );
  XFillRectangle(XiWin->disp,XiDrawable(XiWin),XiWin->gc.set, x, y, w, h);
  return 0;
}

extern int XiQuickWindow(XiWindow*,char*,char*,int,int,int,int,int);

static struct _DrawOps DvOps = { 0,XiFlush,XiDrawLine,0,0,0,
                                 XiDrawText,XiDrawTextVertical,
                                 XiDrawTextSize,XiDrawTextGetSize,
                                 Xiviewport,XiClearWindow};


/*@
    DrawOpenX - Opens an X window for use with the Draw routines.

  Input Parameters:
.   display - the X display to open on, or null for the local machine
.   title - the title to put in the title bar
.   x,y - the screen coordinates of the upper left corner of window
.   width, height - the screen width and height in pixels

  Output Parameters:
.   ctx - the drawing context.
@*/
int DrawOpenX(char* display,char *title,int x,int y,int w,int h,
              DrawCtx* inctx)
{
  DrawCtx  ctx;
  XiWindow *Xwin;
  int      ierr;

  *inctx = 0;
  CREATEHEADER(ctx,_DrawCtx);
  ctx->cookie  = DRAW_COOKIE;
  ctx->type    = XWINDOW;
  ctx->ops     = &DvOps;
  ctx->destroy = 0;
  ctx->view    = 0;
  ctx->coor_xl = 0.0;  ctx->coor_xr = 1.0;
  ctx->coor_yl = 0.0;  ctx->coor_yr = 1.0;
  ctx->port_xl = 0.0;  ctx->port_xr = 1.0;
  ctx->port_yl = 0.0;  ctx->port_yr = 1.0;

  /* actually create and open the window */
  Xwin         = (XiWindow *) MALLOC( sizeof(XiWindow) ); CHKPTR(Xwin);
  ierr         = XiQuickWindow(Xwin,display,title,x,y,w,h,256); CHKERR(ierr);

  ctx->data    = (void *) Xwin;
  *inctx       = ctx;
  return 0;
}
