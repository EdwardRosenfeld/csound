/*
    widgets.cpp:

    Copyright (C) 2002 Gabriel Maldonado

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <cstdio>
#include <cstdlib>
#include <cmath>

#if defined(WIN32)
#  include <windows.h>
#endif /* defined(WIN32) */

#if defined(LINUX)
#  include <pthread.h>
#  include <sched.h>
#  include <sys/time.h>
#endif /* defined(LINUX) */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Adjuster.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Value_Output.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Positioner.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Valuator.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Value_Slider.H>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;

#include "csdl.h"
#include "winFLTK.h"

#define CSOUND_WIDGETS_CPP 1
#include "widgets.h"

#define LIN_ (0)
#define EXP_ (-1)
#undef min
#undef max

typedef struct rtEvt_s {
    struct rtEvt_s  *nxt;
    EVTBLK          evt;
} rtEvt_t;

#ifndef NO_FLTK_THREADS
typedef struct widgetsGlobals_s {
    rtEvt_t     *eventQueue;
    void        *mutex_;
    int         exit_now;       /* set by GUI when all windows are closed   */
    int         end_of_perf;    /* set by main thread at end of performance */
    void        *threadHandle;
    int         fltkFlags;
} widgetsGlobals_t;
#endif

#ifndef NO_FLTK_THREADS
extern "C" {
  /* called by sensevents() once in every control period */
  static void evt_callback(CSOUND *csound, widgetsGlobals_t *p)
  {
    /* flush any pending real time events */
    if (p->eventQueue != NULL) {
      csound->LockMutex(p->mutex_);
      while (p->eventQueue != NULL) {
        rtEvt_t *ep = p->eventQueue;
        p->eventQueue = ep->nxt;
        csound->UnlockMutex(p->mutex_);
        csound->insert_score_event(csound, &(ep->evt), csound->curTime);
        free(ep);
        csound->LockMutex(p->mutex_);
      }
      csound->UnlockMutex(p->mutex_);
    }
    /* if all windows have been closed, terminate performance */
    if (p->exit_now) {
      EVTBLK  e;
      memset(&e, 0, sizeof(EVTBLK));
      e.opcod = 'e';
      csound->insert_score_event(csound, &e, csound->curTime);
    }
  }
}       // extern "C"
#endif  // NO_FLTK_THREADS

static char hack_o_rama1;       // IV - Aug 23 2002
static char hack_o_rama2;

// ---- IV - Aug 23 2002 ---- included file: Fl_linux_stubs.cpp

/*
 * this is a file used in the linux distribution to be able to use
 * the DirectCsound 5.1 widget code by Gabriel Maldonado on linux
 * systems.
 *
 * This code has been written by Nicola Bernardini (nicb@centrotemporeale.it)
 * mostly based on ideas by Dave Phillips (dlphip@bright.net)
 */

int FLkeyboard_sensing(void)
{
  /*
   * since we still don't know what is FLkeyboard_sensing(void)
   * should be returning, but we know that 0 will avoid troubles (?)
   * we return this for now... [nicb@axnet.it]
   */
  return 0;
}

Fl_Window *FLkeyboard_init(void)
{
  /*
   * here too - we don't know what is going on...
   * we fake it through and hope for the best...
   */
  Fl_Window *result = new Fl_Window(0, 0);

  return result;
}

extern "C" {
  void ButtonSched(CSOUND *csound, MYFLT *args[], int numargs)
  { /* based on code by rasmus */
#ifndef NO_FLTK_THREADS
    widgetsGlobals_t  *p;
    /* this is still not fully thread safe... */
    /* hope that no global variable is created just while this fn is called */
    p = (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                        "_widgets_globals");
    if (p != NULL) {
      rtEvt_t   *evt;
      int       i;

      /* Create the new event */
      evt = (rtEvt_t*) malloc(sizeof(rtEvt_t));
      evt->nxt = NULL;
      evt->evt.strarg = NULL;
      evt->evt.opcod = (char) *args[0];
      if (evt->evt.opcod == '\0')
        evt->evt.opcod = 'i';
      evt->evt.pcnt = numargs - 1;
      evt->evt.p[1] = evt->evt.p[2] = evt->evt.p[3] = FL(0.0);
      for (i = 1; i < numargs; i++)
        evt->evt.p[i] = *args[i];
      if (evt->evt.p[2] < FL(0.0))
        evt->evt.p[2] = FL(0.0);
      /* queue event for insertion by main Csound thread */
      csound->LockMutex(p->mutex_);
      if (p->eventQueue == NULL)
        p->eventQueue = evt;
      else {
        rtEvt_t *ep = p->eventQueue;
        while (ep->nxt != NULL)
          ep = ep->nxt;
        ep->nxt = evt;
      }
      csound->UnlockMutex(p->mutex_);
    }
    else
#endif  // NO_FLTK_THREADS
    {
      EVTBLK  e;
      int     i;

      /* Create the new event */
      e.strarg = NULL;
      e.opcod = (char) *args[0];
      if (e.opcod == '\0')
        e.opcod = 'i';
      e.pcnt = numargs - 1;
      e.p[1] = e.p[2] = e.p[3] = FL(0.0);
      for (i = 1; i < numargs; i++)
        e.p[i] = *args[i];
      if (e.p[2] < FL(0.0))
        e.p[2] = FL(0.0);
      csound->insert_score_event(csound, &e, csound->curTime);
    }
  }
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Knob.cxx

// generated by Fast Light User Interface Designer (fluid) version 2.00

Fl_Knob::Fl_Knob(int xx,int yy,int ww,int hh,const char *l)
  : Fl_Valuator(xx,yy,ww,hh,l)
{
    a1 = 35;
    a2 = 325;
    _type = DOTLIN;
    _percent = 0.3;
    _scaleticks = 10;
}

Fl_Knob::~Fl_Knob(void)
{
}

void Fl_Knob::draw(void)
{
  int ox,oy,ww,hh,side;
  unsigned char rr,gg,bb;

  ox = x();
  oy = y();
  ww = w();
  hh = h();
  draw_label();
  fl_clip(ox,oy,ww,hh);
  if (ww > hh) {
    side = hh;
    ox = ox + (ww - side) / 2;
  }
  else {
    side = ww;
    oy = oy + (hh - side) / 2;
  }
  side = w() > h () ? hh:ww;
  int dam = damage();
  if (dam & FL_DAMAGE_ALL) {
    int col = parent()->color();
    fl_color(col);
    fl_rectf(ox,oy,side,side);
    Fl::get_color((Fl_Color)col,rr,gg,bb);
    shadow(-60,rr,gg,bb);
    fl_pie(ox+9,oy+9,side-12,side-12,0,360);
    draw_scale(ox,oy,side);
    col = color();
    Fl::get_color((Fl_Color)col,rr,gg,bb);
    shadow(15,rr,gg,bb);
    fl_pie(ox+6,oy+6,side-12,side-12,40,80);
    shadow(30,rr,gg,bb);
    fl_pie(ox+6,oy+6,side-12,side-12,80,220);
    shadow(-15,rr,gg,bb);
    fl_pie(ox+6,oy+6,side-12,side-12,220,260);
    shadow(-30,rr,gg,bb);
    fl_pie(ox+6,oy+6,side-12,side-12,260,400);
    fl_color(FL_BLACK);
    fl_arc(ox+6,oy+6,side-11,side-11,0,360);
    fl_color(col);
    fl_pie(ox+10,oy+10,side-20,side-20,0,360);
  }
  else {
    fl_color(color());
    fl_pie(ox+10,oy+10,side-20,side-20,0,360);
  }
  Fl::get_color((Fl_Color)color(),rr,gg,bb);
  shadow(10,rr,gg,bb);
  fl_pie(ox+10,oy+10,side-20,side-20,110,150);
  fl_pie(ox+10,oy+10,side-20,side-20,290,330);
  shadow(17,rr,gg,bb);
  fl_pie(ox+10,oy+10,side-20,side-20,120,140);
  fl_pie(ox+10,oy+10,side-20,side-20,300,320);
  shadow(25,rr,gg,bb);
  fl_pie(ox+10,oy+10,side-20,side-20,127,133);
  fl_pie(ox+10,oy+10,side-20,side-20,307,313);
  draw_cursor(ox,oy,side);
  fl_pop_clip();
}
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
int Fl_Knob::handle(int  event)
{
  int ox,oy,ww,hh;

  ox = x() + 10; oy = y() + 10;
  ww = w() - 20;
  hh = h() - 20;
  switch (event) {
  case FL_PUSH:
    handle_push();
  case FL_DRAG:
    {
      int mx = Fl::event_x()-ox-ww/2;
      int my = Fl::event_y()-oy-hh/2;
      if (!mx && !my) return 1;
      double angle = 270-atan2((float)-my, (float)mx)*180.0/M_PI;
      double oldangle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
      while (angle < oldangle-180) angle += 360.0;
      while (angle > oldangle+180) angle -= 360.0;
      double val;
      if ((a1<a2) ? (angle <= a1) : (angle >= a1)) {
        val = minimum();
      }
      else if ((a1<a2) ? (angle >= a2) : (angle <= a2)) {
        val = maximum();
      }
      else {
        val = minimum() + (maximum()-minimum())*(angle-a1)/(a2-a1);
      }
      handle_drag(clamp(round(val)));
    }
    return 1;
  case FL_RELEASE:
    handle_release();
    return 1;
  default:
    return 0;
  }
  return 0;
}

void Fl_Knob::type(int ty)
{
    _type = ty;
}

void Fl_Knob::shadow(const int offs,const uchar r,uchar g,uchar b)
{
    int rr,gg,bb;

    rr = r + offs;
    rr = rr > 255 ? 255 : rr;
    rr = rr < 0 ? 0:rr;
    gg = g + offs;
    gg = gg > 255 ? 255 : gg;
    gg = gg < 0 ? 0 :gg ;
    bb = b + offs;
    bb = bb > 255 ? 255 : bb;
    bb = bb < 0 ? 0 : bb;
    fl_color((uchar)rr,(uchar)gg,(uchar)bb);
}

void Fl_Knob::draw_scale(const int ox,const int oy,const int side)
{
  float x1,y1,x2,y2,rds,cx,cy,ca,sa;

  rds = side / 2;
  cx = ox + side / 2;
  cy = oy + side / 2;
  if (!(_type & DOTLOG_3)) {
    if (_scaleticks == 0) return;
    double a_step = (10.0*M_PI/6.0) / _scaleticks;
    double a_orig = -(M_PI/3.0);
    for (int a = 0; a <= _scaleticks; a++) {
      double na = a_orig + a * a_step;
      ca = cos(na);
      sa = sin(na);
      x1 = cx + rds * ca;
      y1 = cy - rds * sa;
      x2 = cx + (rds-6) * ca;
      y2 = cy - (rds-6) * sa;
      fl_color(FL_BLACK);
      fl_line((int)x1,(int)y1,(int)x2,(int)y2);
      fl_color(FL_WHITE);
      if (sa*ca >=0)
        fl_line((int)x1+1,(int)y1+1,(int)x2+1,(int)y2+1);
      else
        fl_line((int)x1+1,(int)y1-1,(int)x2+1,(int)y2-1);
    }
  }
  else {
    int nb_dec = (_type & DOTLOG_3);
    for (int k = 0; k < nb_dec; k++) {
      double a_step = (10.0*M_PI/6.0) / nb_dec;
      double a_orig = -(M_PI/3.0) + k * a_step;
      for (int a = (k) ? 2:1; a <= 10; ) {
        double na = a_orig + log10((double)a) * a_step;
        ca = cos(na);
        sa = sin(na);
        x1 = cx - rds * ca;
        y1 = cy - rds * sa;
        x2 = cx - (rds-6) * ca;
        y2 = cy - (rds-6) * sa;
        fl_color(FL_BLACK);
        fl_line((int)x1,(int)y1,(int)x2,(int)y2);
        fl_color(FL_WHITE);
        if (sa*ca <0)
          fl_line((int)x1+1,(int)y1+1,(int)x2+1,(int)y2+1);
        else
          fl_line((int)x1+1,(int)y1-1,(int)x2+1,(int)y2-1);
        if ((a == 1) || (nb_dec == 1))
          a += 1;
        else
          a += 2;
      }
    }
  }
}

void Fl_Knob::draw_cursor(const int ox,const int oy,const int side)
{
  float rds,cur,cx,cy;
  double angle;

  rds = (side - 20.0f) / 2.0f;
  cur = _percent * rds / 2.0f;
  cx = ox + side / 2.0f;
  cy = oy + side / 2.0f;
  angle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
  fl_push_matrix();
  fl_scale(1,1);
  fl_translate(cx,cy);
  fl_rotate(-angle);
  fl_translate(0,rds-cur-2.0f);
  if (_type<LINELIN) {
    fl_begin_polygon();
    fl_color(selection_color());
    fl_circle(0.0,0.0,cur);
    fl_end_polygon();
    fl_begin_loop();
    fl_color(FL_BLACK);
    fl_circle(0.0,0.0,cur);
    fl_end_loop();
  }
  else {
    fl_begin_polygon();
    fl_color(selection_color());
    fl_vertex(-1.5,-cur);
    fl_vertex(-1.5,cur);
    fl_vertex(1.5,cur);
    fl_vertex(1.5,-cur);
    fl_end_polygon();
    fl_begin_loop();
    fl_color(FL_BLACK);
    fl_vertex(-1.5,-cur);
    fl_vertex(-1.5,cur);
    fl_vertex(1.5,cur);
    fl_vertex(1.5,-cur);
    fl_end_loop();
  }
  fl_pop_matrix();
}

void Fl_Knob::cursor(const int pc)
{
  _percent = (float)pc/100.0f;

  if (_percent < 0.05f) _percent = 0.05f;
  if (_percent > 1.0f) _percent = 1.0f;
  if (visible()) damage(FL_DAMAGE_CHILD);
}

void Fl_Knob::scaleticks(const int tck)
{
  _scaleticks = tck;
  if (_scaleticks < 0) _scaleticks = 0;
  if (_scaleticks > 31) _scaleticks = 31;
  if (visible()) damage(FL_DAMAGE_ALL);
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Spin.cpp

void Fl_Spin::draw(void)
{
  int sxx = x(), syy = y(), sww = w(), shh = h();
  Fl_Boxtype box1 = (Fl_Boxtype)(box());
  int border_size=Fl::box_dx(box());

  if (damage()&~FL_DAMAGE_CHILD) clear_damage(FL_DAMAGE_ALL);

  if (!box1) box1 = (Fl_Boxtype)(box()&-2);

  if ((indrag || mouseobj) && deltadir!=0) {
    if (deltadir>0) {
      draw_box(fl_down(box1),sxx,syy,sww,shh/2,color());
      draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
    }
    else {
      draw_box(box1,sxx,syy,sww,shh/2,color());
      draw_box(fl_down(box1),sxx,syy+shh/2,sww,shh/2,color());
    }
  }
  else {
    draw_box(box1,sxx,syy,sww,shh/2,color());
    draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
  }
  sxx+=border_size;
  syy+=border_size;
  sww-=border_size*2;
  shh-=border_size*2;
  if (active_r()) {
    fl_color(selection_color());
  }
  else {
    fl_color(selection_color() | 8);
  }
  int w1 = (sww-1)|1; // use odd sizes only
  int X = sxx+w1/2;
  int W = w1/3;
  int h1 = shh/2-border_size-2;
  int Y= syy;
  fl_polygon(X, Y, X+W,Y+h1 , X-W, Y+h1);
  Y=syy+shh/2+border_size+1+h1;
  fl_polygon(X, Y, X-W, Y-h1, X+W, Y-h1);
  clear_damage();
}

void Fl_Spin::repeat_callback(void* v)
{
  Fl_Spin* b = (Fl_Spin*)v;
  if (b->mouseobj) {
    Fl::add_timeout(0.1, repeat_callback, b);
    b->increment_cb();
  }
}

void Fl_Spin::increment_cb(void)
{
  if (!mouseobj) return;
  delta += deltadir;
  double v;
  switch (drag) {
  case 3: v = increment(value(), deltadir*100); break;
  case 2: v = increment(value(), deltadir*10); break;
  default:v = increment(value(), deltadir); break;
  }
  v = round(v);
  handle_drag(soft()?softclamp(v):clamp(v));
}

int Fl_Spin::handle(int event)
{
  double v;
  int olddelta;
  int mx = Fl::event_x();
  int my = Fl::event_y();
  int sxx = x(), syy = y(), sww = w(), shh = h();

  switch (event) {
  case FL_PUSH:
    //    if (!step()) goto DEFAULT;
    iy = my;
    ix = mx;
    drag = Fl::event_button();
    handle_push();
    indrag=1;
    mouseobj=1;
    Fl::add_timeout(0.5, repeat_callback, this);
    delta=0;
    if (Fl::event_inside(sxx,syy,sww,shh/2)) {
      deltadir=1;
    }
    else if (Fl::event_inside(sxx,syy+shh/2,sww,shh/2)) {
      deltadir=-1;
    }
    else {
      deltadir=0;
    }
    increment_cb();
    redraw();
    return 1;
  case FL_DRAG:
    if (mouseobj) {
      mouseobj=0;
      Fl::remove_timeout(repeat_callback, this);
    }
    //    if (!step()) goto DEFAULT;
    olddelta=delta;
    delta = - (Fl::event_y()-iy);
    if ((delta>5) || (delta<-5)) {
      deltadir=((olddelta-delta)>0)?-1:(((olddelta-delta)<0)?1:0);
    }
    else {
      deltadir=0; delta = olddelta;
    }
    switch (drag) {
    case 3: v = increment(value(), deltadir*100); break;
    case 2: v = increment(value(), deltadir*10); break;
    default:v = increment(value(), deltadir); break;
    }
    v = round(v);
    handle_drag(soft()?softclamp(v):clamp(v));
    indrag=1;
    return 1;
  case FL_RELEASE:
    if (mouseobj) {
      Fl::remove_timeout(repeat_callback, this);
    }
    //    if (!step()) goto DEFAULT;
    indrag=0;
    delta=0;
    deltadir=0;
    mouseobj=0;
    handle_release();
    redraw();
    return 1;
  default:
    indrag=0;
    return Fl_Valuator::handle(event);
  }
}

Fl_Spin::~Fl_Spin(void)
{
  Fl::remove_timeout(repeat_callback, this);
}

Fl_Spin::Fl_Spin(int x, int y, int w, int h, const char* l)
  : Fl_Valuator(x,y,w,h,l)
{
  soft_ = 0;
  align(FL_ALIGN_LEFT);
  ix=x;
  iy=y;
  drag=0;
  indrag=0;
  mouseobj = 0;
  deltadir=0;
  delta=0;
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Value_Input_Spin.cpp

void Fl_Value_Input_Spin::input_cb(Fl_Widget*, void* v)
{
  Fl_Value_Input_Spin& t = *(Fl_Value_Input_Spin*)v;
  double nv;
  if (t.step()>=1.0) nv = strtol(t.input.value(), 0, 0);
  else nv = strtod(t.input.value(), 0);
  hack_o_rama1 = 1;
  t.handle_push();
  t.handle_drag(nv);
  t.handle_release();
  hack_o_rama1 = 0;
}

void Fl_Value_Input_Spin::draw(void)
{
  int sxx = x(), syy = y(), sww = w(), shh = h();
  sxx += sww - buttonssize(); sww = buttonssize();
  Fl_Boxtype box1 = (Fl_Boxtype)(box()&-2);
  int border_size=Fl::box_dx(box());

  if (damage()&~FL_DAMAGE_CHILD) input.clear_damage(FL_DAMAGE_ALL);
  input.box(box());
  input.color(FL_WHITE, selection_color());
  input.draw();
  input.clear_damage();
  sxx+=border_size;
  syy+=border_size;
  sww-=border_size*2;
  shh-=border_size*2;

  if (!box1) box1 = (Fl_Boxtype)(box()&-2);

  if ((indrag || mouseobj) && deltadir!=0) {
    if (deltadir>0) {
      draw_box(fl_down(box1),sxx,syy,sww,shh/2,color());
      draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
    }
    else {
      draw_box(box1,sxx,syy,sww,shh/2,color());
      draw_box(fl_down(box1),sxx,syy+shh/2,sww,shh/2,color());
    }
  }
  else {
    draw_box(box1,sxx,syy,sww,shh/2,color());
    draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
  }
  sxx+=border_size;
  syy+=border_size;
  sww-=border_size*2;
  shh-=border_size*2;
  if (active_r()) {
    fl_color(labelcolor());
  }
  else {
    fl_color(labelcolor() | 8);
  }
  int w1 = (sww-1)|1; // use odd sizes only
  int X = sxx+w1/2;
  int W = w1/3;
  int h1 = shh/2-border_size-2;
  int Y= syy;
  fl_polygon(X, Y, X+W,Y+h1 , X-W, Y+h1);
  Y=syy+shh/2+border_size+1+h1;
  fl_polygon(X, Y, X-W, Y-h1, X+W, Y-h1);
  clear_damage();
}

void Fl_Value_Input_Spin::resize(int X, int Y, int W, int H)
{
  input.resize(X,Y,W,H);
  Fl_Valuator::resize(X,Y,W,H);
}

void Fl_Value_Input_Spin::value_damage(void)
{
  if (hack_o_rama1) return;
  char buf[128];
  format(buf);
  input.value(buf);
  input.mark(input.position()); // turn off selection highlight
}

void Fl_Value_Input_Spin::repeat_callback(void* v)
{
  Fl_Value_Input_Spin* b = (Fl_Value_Input_Spin*)v;
  if (b->mouseobj) {
    Fl::add_timeout(.1, repeat_callback, b);
    b->increment_cb();
  }
}

void Fl_Value_Input_Spin::increment_cb(void)
{
  if (!mouseobj) return;
  delta+=deltadir;
  double v;
  switch (drag) {
  case 3: v = increment(value(), deltadir*100); break;
  case 2: v = increment(value(), deltadir*10); break;
  default:v = increment(value(), deltadir); break;
  }
  v = round(v);
  handle_drag(soft()?softclamp(v):clamp(v));
}

int Fl_Value_Input_Spin::handle(int event)
{
  double v;
  int olddelta;
  int mx = Fl::event_x();
  int my = Fl::event_y();
  int sxx = x(), syy = y(), sww = w(), shh = h();
  sxx += sww - buttonssize(); sww = buttonssize();

  if (!indrag && ( !sldrag || !((mx>=sxx && mx<=(sxx+sww)) &&
                                (my>=syy && my<=(syy+shh))))  ) {
    indrag=0;
    switch(event) {
    case FL_PUSH:
    case FL_DRAG:
      sldrag=1;
      break;
    case FL_FOCUS:
      input.take_focus();
      break;
    case FL_UNFOCUS:
      redraw();
      break;
    default:
      sldrag=0;
    }
    input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
    return input.handle(event);
  }

  switch (event) {
  case FL_PUSH:
    //    if (!step()) goto DEFAULT;
    iy = my;
    ix = mx;
    drag = Fl::event_button();
    handle_push();
    indrag=1;
    mouseobj=1;
    Fl::add_timeout(.5, repeat_callback, this);
    delta=0;
    if (Fl::event_inside(sxx,syy,sww,shh/2)) {
      deltadir=1;
    }
    else if (Fl::event_inside(sxx,syy+shh/2,sww,shh/2)) {
      deltadir=-1;
    }
    else {
      deltadir=0;
    }
    increment_cb();
    redraw();
    return 1;
  case FL_DRAG:
    if (mouseobj) {
      mouseobj=0;
      Fl::remove_timeout(repeat_callback, this);
    }
    //    if (!step()) goto DEFAULT;
    olddelta=delta;
    delta = - (Fl::event_y()-iy);
    if ((delta>5) || (delta<-5)  ) {
      deltadir=((olddelta-delta)>0)?-1:(((olddelta-delta)<0)?1:0);
    }
    else {
      deltadir=0; delta = olddelta;
    }
    switch (drag) {
    case 3: v = increment(value(), deltadir*100); break;
    case 2: v = increment(value(), deltadir*10); break;
    default:v = increment(value(), deltadir); break;
    }
    v = round(v);
    handle_drag(soft()?softclamp(v):clamp(v));
    indrag=1;
    return 1;
  case FL_RELEASE:
    if (mouseobj) {
      Fl::remove_timeout(repeat_callback, this);
    }
    //    if (!step()) goto DEFAULT;
    indrag=0;
    delta=0;
    deltadir=0;
    mouseobj=0;
    handle_release();
    redraw();
    return 1;
  case FL_FOCUS:
    indrag=0;
    return input.take_focus();
  default:
    indrag=0;
    input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
    return 1;
  }
}

Fl_Value_Input_Spin::~Fl_Value_Input_Spin(void)
{
  Fl::remove_timeout(repeat_callback, this);
}

Fl_Value_Input_Spin::Fl_Value_Input_Spin(int x, int y,
                                         int w, int h, const char* l)
  : Fl_Valuator(x,y,w,h,l), input(x, y, w, h, 0)
{
  soft_ = 0;
  //  if (input.parent())  // defeat automatic-add
  //    ((Fl_Group*)input.parent())->remove(input);
  //  input.parent(this); // kludge!
  this->parent()->add(input);
  input.callback(input_cb, this);
  input.when(FL_WHEN_CHANGED);
  selection_color(input.selection_color());
  align(FL_ALIGN_LEFT);
  box(input.box());
  value_damage();
  buttonssize(15);
  ix       = x;
  iy       = y;
  drag     = 0;
  indrag   = 0;
  sldrag   = 0;
  mouseobj = 0;
  deltadir = 0;
  delta    = 0;
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Value_Slider_Input.cpp

void Fl_Value_Slider_Input::input_cb(Fl_Widget*, void* v) {
  Fl_Value_Slider_Input& t = *(Fl_Value_Slider_Input*)v;
  double nv;
  if (t.step()>=1.0) nv = strtol(t.input.value(), 0, 0);
  else nv = strtod(t.input.value(), 0);
  hack_o_rama2 = 1;
  t.handle_push();
  t.handle_drag(nv);
  t.handle_release();
  hack_o_rama2 = 0;
}

void Fl_Value_Slider_Input::draw(void)
{
  int sxx = x(), syy = y(), sww = w(), shh = h();
  int bww = w();
  int X = x(), Y = y(), W = w(), H = h();

  int border_size=Fl::box_dx(box());

  if (horizontal()) {
    bww = textboxsize();  sxx += textboxsize(); sww -= textboxsize();
    input.resize(X,Y,W-sww,shh);
  }
  else {
    fl_font(input.textfont(), input.textsize());
    syy += fl_height()+(border_size+1)*2; shh -= fl_height()+(border_size+1)*2;
    input.resize(X,Y,W,H-shh);
  }
  if (damage()&~FL_DAMAGE_CHILD)  input.clear_damage(FL_DAMAGE_ALL);
  input.box(box());
  input.color(FL_WHITE, selection_color());
  input.draw();
  input.resize(X,Y,W,H);
  input.clear_damage();
  //  if (horizontal())   input.draw();
  clear_damage();
  draw_box(box(),sxx,syy,sww,shh,color());
  sxx+=border_size;
  syy+=border_size;
  sww-=border_size*2;
  shh-=border_size*2;
  if (border_size<2) {
    sxx++;
    syy++;
    sww--;
    shh--;
  }
  Fl_Slider::draw(sxx,syy,sww,shh);
}

void Fl_Value_Slider_Input::resize(int X, int Y, int W, int H)
{
  input.resize(X,Y,W,H);
  Fl_Value_Slider::resize(X,Y,W,H);
}

void Fl_Value_Slider_Input::value_damage()
{
  if (hack_o_rama2) return;
  char buf[128];
  format(buf);
  input.value(buf);
  input.mark(input.position()); // turn off selection highlight
}

int Fl_Value_Slider_Input::handle(int event)
{
  int mx = Fl::event_x();
  int my = Fl::event_y();
  static int ix, drag, indrag=0,sldrag=0;
  int sxx = x(), syy = y(), sww = w(), shh = h();
  int border_size=Fl::box_dx(box());
  if (horizontal()) {
    sxx += textboxsize(); sww -= textboxsize();
  }
  else {
    fl_font(input.textfont(), input.textsize());
    syy += fl_height()+(border_size+1)*2; shh -= fl_height()+(border_size+1)*2;
  }
  if ( !indrag && ( !sldrag || !((mx>=sxx && mx<=(sxx+sww)) &&
                                 (my>=syy && my<=(syy+shh))))  ) {
    indrag=0;
    switch(event) {
    case FL_PUSH:
    case FL_DRAG:
      sldrag=1;
      break;
    case FL_FOCUS:
      input.take_focus();
      break;
    case FL_UNFOCUS:
      redraw();
      break;
    default:
      sldrag=0;
    }
    input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
    return input.handle(event);
  }
  switch (event) {
  case FL_PUSH:
    ix = mx;
    drag = Fl::event_button();
    indrag=1;
    return Fl_Slider::handle(event,sxx,syy,sww,shh);
  case FL_DRAG:
    indrag=1;
    return Fl_Slider::handle(event,sxx,syy,sww,shh);
  case FL_RELEASE:
    //   if (!step()) goto DEFAULT;
    if (value() != previous_value() || !Fl::event_is_click())
      handle_release();
    else {
      input.handle(FL_PUSH);
      input.handle(FL_RELEASE);
    }
    indrag=0;
    return 1;
  case FL_FOCUS:
    indrag=0;
    input.take_focus();
    return Fl_Slider::handle(event,sxx,syy,sww,shh);
  default:
    indrag=0;
    input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
    input.handle(event);
    return Fl_Slider::handle(event,sxx,syy,sww,shh);
  }
}

Fl_Value_Slider_Input::Fl_Value_Slider_Input(int x, int y,
                                             int w, int h, const char* l)
  : Fl_Value_Slider(x,y,w,h,l),input(x, y, w, h, 0)
{
  soft_ = 0;
  // if (input.parent())  // defeat automatic-add
  // ((Fl_Group*)input.parent())->remove(input);
  // input.parent(this); // kludge!
  // input.callback(input_cb, this);
  parent()->add(input);
  //  input.when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY);
  input.when(FL_WHEN_CHANGED );
  selection_color(input.selection_color());
  input.align(FL_ALIGN_LEFT);
  align(FL_ALIGN_LEFT);
  textboxsize(35);
  value_damage();
}

// ---- IV - Aug 23 2002 ---- end of included files

struct PANELS {
  Fl_Window *panel;
  int     is_subwindow;
  PANELS(Fl_Window *new_panel, int flag) : panel(new_panel),
                                           is_subwindow(flag) { }
  PANELS() {panel = NULL; is_subwindow=0; }
};

struct ADDR {
  void *opcode;
  void *WidgAddress;
};

struct ADDR_STACK /*: ADDR*/{
  OPDS *h;
  void *WidgAddress;
  int     count;
  ADDR_STACK(OPDS *new_h,void *new_address,  int new_count)
    : h(new_h),WidgAddress(new_address), count(new_count) {}
  ADDR_STACK() { h=NULL; WidgAddress = NULL;      count=0; }
};

struct ADDR_SET_VALUE /*: ADDR*/{
  int exponential;
  MYFLT min,max;
  void  *WidgAddress,  *opcode;
  ADDR_SET_VALUE(int new_exponential,MYFLT new_min, MYFLT new_max,
                 void *new_WidgAddress, void *new_opcode) :
    exponential(new_exponential),min(new_min), max(new_max),
    WidgAddress(new_WidgAddress),opcode(new_opcode) {}
  ADDR_SET_VALUE() {
    exponential=LIN_; min=0; max=0;
    WidgAddress=NULL; opcode=NULL;
  }
};

struct VALUATOR_FIELD {
  MYFLT value, value2;
  MYFLT min,max, min2,max2;
  int     exp, exp2;
  string widg_name;
  string opcode_name;
  SLDBK_ELEMENT *sldbnk;
  MYFLT  *sldbnkValues;
  VALUATOR_FIELD() {
    value = 0; value2 =0; widg_name= ""; opcode_name ="";
    min = 0; max =1; min2=0; max2=1; exp=LIN_; exp2=LIN_;
    sldbnk=0; sldbnkValues=0; }
  //  ~VALUATOR_FIELD() { if (sldbnk != 0) delete sldbnk;
  //  if (sldbnkValues !=0) delete sldbnkValues; }
};

static char *GetString(CSOUND *csound, MYFLT *pname, int is_string);

struct SNAPSHOT {
  int is_empty;
  vector<VALUATOR_FIELD> fields;
  SNAPSHOT(vector<ADDR_SET_VALUE>& valuators);
  SNAPSHOT() { is_empty = 1; }
  int get(vector<ADDR_SET_VALUE>& valuators);
};

//---------------

static int stack_count       = 0;

static int FLcontrol_iheight = 15;
static int FLroller_iheight  = 18;
static int FLcontrol_iwidth  = 400;
static int FLroller_iwidth   = 150;
static int FLvalue_iwidth    = 100;

static int FLcolor           = -1;
static int FLcolor2          = -1;
static int FLtext_size       = 0;
static int FLtext_color      = -1;
static int FLtext_font       = -1;
static int FLtext_align      = 0;

static int FL_ix             = 10;
static int FL_iy             = 10;

static vector<PANELS> fl_windows; // all panels
//static vector<void*> AddrValue;
//        addresses of widgets that display current value of valuators
static vector<ADDR_STACK> AddrStack; //addresses of containers
static vector<ADDR_SET_VALUE> AddrSetValue; //addresses of valuators
static vector<char*> allocatedStrings;
static vector<SNAPSHOT> snapshots;

static void set_butbank_value(Fl_Group *o, MYFLT value)
{
  int       childr = o->children();
  Fl_Button *b;
  int       ival = (int) value, j, label;

  if (ival < 0 || ival >= childr || (MYFLT) ival != value)
    return;
  for (j = 0; j < childr; j++) {
    b = (Fl_Button *) o->child(j);
    label = atoi(b->label());
    if (label == ival) {
      b->value(1);
      b->do_callback();
    }
    else
      b->value(0);
  }
}

SNAPSHOT::SNAPSHOT (vector<ADDR_SET_VALUE>& valuators)
{ // the constructor captures current values of all widgets
  // by copying all current values from "valuators" vector (AddrSetValue)
  // to the "fields" vector
  is_empty =0;
  for (int i=0; i < (int) valuators.size(); i++) {
    string  opcode_name, widg_name;
    ADDR_SET_VALUE& v  = valuators[i];
    CSOUND *csound = (CSOUND*) (((OPDS *) (v.opcode))->insdshead->csound);
    if ((int) fields.size() < i+1)
      fields.resize(i+1);
    VALUATOR_FIELD& fld = fields[i];
    float val, min, max;
    opcode_name = fld.opcode_name = ((OPDS *) (v.opcode))->optext->t.opcod;
    if (opcode_name == "FLslider") {
      FLSLIDER *p = (FLSLIDER *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      val = *p->kout; min = *p->imin; max =*p->imax;
      if (val < min) val=min;
      else if (val>max) val=max;
      fld.value = val;
      fld.min = *p->imin; fld.max = *p->imax; fld.exp = (int) *p->iexp;
    }
    else if (opcode_name == "FLslidBnk") {
      FLSLIDERBANK *p = (FLSLIDERBANK *) (v.opcode);
      fld.widg_name = GetString(csound, p->names, p->XSTRCODE);
      int numsliders = (int) *p->inumsliders;
      fld.sldbnk = p->slider_data;
      fld.sldbnkValues = new MYFLT[numsliders];
      allocatedStrings.push_back((char *) fld.sldbnkValues);
      fld.exp = numsliders; // EXCEPTIONAL CASE! fld.exp contains the number
      // of sliders and not the exponential flag
      for (int j =0; j < numsliders; j++) {
        switch (fld.sldbnk[j].exp) {
        case LIN_: case EXP_:
          val = *fld.sldbnk[j].out;
          min = fld.sldbnk[j].min; max = fld.sldbnk[j].max;
          if (val < min) val = min;
          if (val > max) val = max;
          break;
        default:
          val = ((Fl_Valuator *) ((Fl_Group*)v.WidgAddress)->
                 child(j))->value();
        }
        fld.sldbnkValues[j]= val;
      }
    }
    else if (opcode_name == "FLknob") {
      FLKNOB *p = (FLKNOB *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      val = *p->kout; min = *p->imin; max =*p->imax;
      if (val < min) val=min;
      else if (val>max) val=max;
      fld.value = val;
      fld.min = *p->imin; fld.max = *p->imax; fld.exp = (int) *p->iexp;
    }
    else if (opcode_name == "FLroller") {
      FLROLLER *p = (FLROLLER *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      val = *p->kout; min = *p->imin; max =*p->imax;
      if (val < min) val=min;
      else if (val>max) val=max;
      fld.value = val;
      fld.min = *p->imin; fld.max = *p->imax; fld.exp = (int) *p->iexp;
    }
    else if (opcode_name == "FLtext") {
      FLTEXT *p = (FLTEXT *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      val = *p->kout; min = *p->imin; max =*p->imax;
      if (val < min) val=min;
      else if (val>max) val=max;
      fld.value = val;
      fld.min = *p->imin; fld.max = *p->imax; fld.exp = LIN_;
    }
    else if (opcode_name == "FLjoy") {
      FLJOYSTICK *p = (FLJOYSTICK *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      val = *p->koutx; min = *p->iminx; max =*p->imaxx;
      if (val < min) val=min;
      else if (val>max) val=max;
      fld.value = val;
      val = *p->kouty; min = *p->iminy; max =*p->imaxy;
      if (val < min) val=min;
      else if (val>max) val=max;
      fld.value2 =val;
      fld.min =  *p->iminx; fld.max  = *p->imaxx; fld.exp  = (int) *p->iexpx;
      fld.min2 = *p->iminy; fld.max2 = *p->imaxy; fld.exp2 = (int) *p->iexpy;
    }
    else if (opcode_name == "FLbutton") {
      FLBUTTON *p = (FLBUTTON *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      fld.value = (*p->kout == *p->ion ? 1 : 0);    // IV - Aug 27 2002
      fld.min = 0; fld.max = 1; fld.exp = LIN_;
    }
    else if (opcode_name == "FLbutBank") {
      FLBUTTONBANK *p = (FLBUTTONBANK *) (v.opcode);
      fld.widg_name = "No name for FLbutbank";
      //fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      fld.value = *p->kout;
      fld.min = 0; fld.max = 1; fld.exp = LIN_;
    }
    else if (opcode_name == "FLcount") {
      FLCOUNTER *p = (FLCOUNTER *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
      val = *p->kout; min = *p->imin; max =*p->imax;
      if (min != max) {
        if (val < min) val=min;
        else if (val>max) val=max;
      }
      fld.value = val;
      fld.min = *p->imin; fld.max = *p->imax; fld.exp = LIN_;
    }
    else if (opcode_name == "FLvalue") {
      FLVALUE *p = (FLVALUE *) (v.opcode);
      fld.widg_name = GetString(csound, p->name, p->XSTRCODE);
    }
    else if (opcode_name == "FLbox") {
      FL_BOX *p = (FL_BOX *) (v.opcode);
      fld.widg_name = GetString(csound, p->itext, p->XSTRCODE);
    }
  }
}

int SNAPSHOT::get(vector<ADDR_SET_VALUE>& valuators)
{
  if (is_empty) {
/*  FIXME: should have CSOUND* pointer here */
/*  return csound->InitError(csound, "empty snapshot"); */
    return -1;
  }
  for (int j =0; j< (int) valuators.size(); j++) {
    Fl_Widget* o = (Fl_Widget*) (valuators[j].WidgAddress);
    void *opcode = valuators[j].opcode;
    CSOUND *csound = (CSOUND*) (((OPDS*) opcode)->insdshead->csound);
    VALUATOR_FIELD& fld = fields[j];
    string opcode_name = fld.opcode_name;

    MYFLT val = fld.value, min=fld.min, max=fld.max, range,base;
    if (val < min) val =min;
    else if (val >max) val = max;

    if (opcode_name == "FLjoy") {
      switch(fld.exp) {
      case LIN_:
        ((Fl_Positioner*) o)->xvalue(val);
        break;
      case EXP_:
        range  = fld.max - fld.min;
        base = ::pow(fld.max / fld.min, 1/range);
        ((Fl_Positioner*) o)->xvalue(log(val/fld.min) / log(base)) ;
        break;
      default:
        if (csound->oparms->msglevel & WARNMSG)
          csound->Warning(csound, "(SNAPSHOT::get): "
                                  "not implemented yet; exp=%d", fld.exp);
        break;
      }
      val = fld.value2; min = fld.min2; max = fld.max2;
      if (val < min) val =min;
      else if (val >max) val = max;
      switch(fld.exp2) {
      case LIN_:
        ((Fl_Positioner*) o)->yvalue(val);
        break;
      case EXP_:
        range  = fld.max2 - fld.min2;
        base = ::pow(fld.max2 / fld.min2, 1/range);
        ((Fl_Positioner*) o)->yvalue(log(val/fld.min2) / log(base)) ;
        break;
      default:
        if (csound->oparms->msglevel & WARNMSG)
          csound->Warning(csound, "(SNAPSHOT::get): "
                                  "not implemented yet; exp2=%d", fld.exp2);
        break;
      }
      o->do_callback(o, opcode);
    }
    else if (opcode_name == "FLbutton") {
      FLBUTTON *p = (FLBUTTON*) (opcode);
      if (*p->itype < 10) {//  don't allow to retreive its value if >= 10
        ((Fl_Button*) o)->value((int)fld.value);
        o->do_callback(o, opcode);
      }
    }
    else if (opcode_name == "FLbutBank") {
      FLBUTTONBANK *p = (FLBUTTONBANK*) (opcode);
      if (*p->itype < 10) {//  don't allow to retreive its value if >= 10
        //((Fl_Group*) o)->value(fld.value);
        set_butbank_value((Fl_Group*) o, fld.value);
        //o->do_callback(o, opcode);
        *p->kout=fld.value;
      }
    }
    else if (opcode_name == "FLcount") {
      FLCOUNTER *p = (FLCOUNTER*) (opcode);
      if (*p->itype < 10) { //  don't allow to retreive its value if >= 10
        ((Fl_Counter*) o)->value(fld.value);
        o->do_callback(o, opcode);
      }
    }
    else if (opcode_name == "FLslidBnk") {
      FLSLIDERBANK *p = (FLSLIDERBANK*) (opcode);
      int numsliders = (int) *p->inumsliders;
      Fl_Group * grup = (Fl_Group *) o;
      for (int j =0; j < numsliders; j++) {
        MYFLT val = fld.sldbnkValues[j];
        switch (p->slider_data[j].exp) {
        case LIN_:
          ((Fl_Valuator *) grup->child(j))->value(val);
          break;
        case EXP_:
          range  = p->slider_data[j].max - p->slider_data[j].min;
          base = ::pow(p->slider_data[j].max / p->slider_data[j].min, 1/range);
          ((Fl_Valuator*) grup->child(j))->
            value(log(val/p->slider_data[j].min) / log(base)) ;
          break;
        default:
          ((Fl_Valuator *) grup->child(j))->value(val);
          /*
            if (csound->oparms->msglevel & WARNMSG)
            csound->Warning(csound, "not implemented yet (bogus)");
            break;
          */
        }
        grup->child(j)->do_callback( grup->child(j),
                                     (void *) &(p->slider_data[j]));
      }
    }
    else {
      switch(fld.exp) {
      case LIN_:
        if (opcode_name == "FLbox" || opcode_name == "FLvalue" ) continue;
        ((Fl_Valuator*) o)->value(val);
        break;
      case EXP_:
        range  = fld.max - fld.min;
        base = ::pow(fld.max / fld.min, 1/range);
        ((Fl_Valuator*) o)->value(log(val/fld.min) / log(base)) ;
        break;
      default:
        if (csound->oparms->msglevel & WARNMSG)
          csound->Warning(csound, "(SNAPSHOT::get): not implemented yet; "
                                  "exp=%d", fld.exp);
        break;
      }
      o->do_callback(o, opcode);
    }
  }
  return OK;
}

extern "C" {

static int set_snap(CSOUND *csound, FLSETSNAP *p)
{
  SNAPSHOT snap(AddrSetValue);
  int numfields = snap.fields.size();
  int index = (int) *p->index;
  *p->inum_snap = snapshots.size();
  *p->inum_val = numfields; // number of snapshots

  if (*p->ifn >= 1) { // if the table number is valid
    FUNC    *ftp;   // store the snapshot into the table
    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) {
      MYFLT *table = ftp->ftable;
      for (int j = 0; j < numfields; j++) {
        table[index*numfields+j] = snap.fields[j].value;
      }
    }
    else return csound->InitError(csound, "FLsetsnap: invalid table");
  }
  else { // else store it into snapshot bank
    if ((int) snapshots.size() < index+1)
      snapshots.resize(index+1);
    snapshots[index]=snap;
  }
  return OK;
}

static int get_snap(CSOUND *csound, FLGETSNAP *p)
{
  int index = (int) *p->index;
  if (!snapshots.empty()) {
    if (index > (int) snapshots.size()) index = snapshots.size();
    else if (index < 0) index=0;
    if (snapshots[index].get(AddrSetValue)!=OK) return NOTOK;
  }
  *p->inum_el = snapshots.size();
  return OK;
}

}       // extern "C"

#include <FL/fl_ask.H>

extern "C" {

static int save_snap(CSOUND *csound, FLSAVESNAPS *p)
{
  char    s[MAXNAME], *s2;
  string  filename;
  // put here some warning message!!
#ifdef WIN32
  int id = MessageBox(NULL, Str("Saving could overwrite the old file.\n"
                                "Are you sure to save ?"), "Warning",
                      MB_SYSTEMMODAL | MB_ICONWARNING | MB_OKCANCEL);
  if (id != IDOK)
    return OK;
#else
#  if !defined(NO_FLTK_THREADS)
  if (!((getFLTKFlags(csound) & 260) ^ 4))
#  endif
  {
    int   n;
    Fl_lock(csound);
    n = fl_ask(Str("Saving could overwrite the old file.\n"
                   "Are you sure you want to save ?"));
    Fl_unlock(csound);
    if (!n)
      return OK;
  }
#endif
  csound->strarg2name(csound, s, p->filename, "snap.", p->XSTRCODE);
  s2 = csound->FindOutputFile(csound, s, "SNAPDIR");
  if (s2 == NULL)
    return csound->InitError(csound, Str("FLsavesnap: cannot open file"));
  strcpy(s, s2);
  csound->Free(csound, s2);
  filename = s;

  fstream file(filename.c_str(), ios::out);
  for (int j =0; j < (int) snapshots.size(); j++) {
    file << "----------- "<< j << " -----------\n";
    for ( int k =0; k < (int) snapshots[j].fields.size(); k++) {
      VALUATOR_FIELD& f = snapshots[j].fields[k];
      if (f.opcode_name == "FLjoy") {
        file <<f.opcode_name<<" "<< f.value <<" "<< f.value2
             <<" "<< f.min <<" "<< f.max <<" "<< f.min2 <<" "
             << f.max2<<" "<<f.exp<<" "<<f.exp2<<" \"" <<f.widg_name<<"\"\n";
      }
      else if (f.opcode_name == "FLslidBnk") {
        // EXCEPTIONAL CASE! fld.exp contains the number of sliders and
        // not the exponential flag
        file << f.opcode_name << " " << f.exp << " ";
        for (int i=0; i < f.exp; i++) {
          file << f.sldbnkValues[i] << " ";
        }
        file << " \"" << f.widg_name << "\"\n";
      }
      else {
        file <<f.opcode_name<<" "<< f.value
             <<" "<< f.min <<" "<< f.max <<" "<<f.exp<<" \""
             <<f.widg_name<<"\"\n";
      }
    }
  }
  file << "---------------------------";
  file.close();
  return OK;
}

static int load_snap(CSOUND *csound, FLLOADSNAPS* p)
{
  char     s[MAXNAME], *s2;
  string   filename;

  csound->strarg2name(csound, s, p->filename, "snap.", p->XSTRCODE);
  s2 = csound->FindInputFile(csound, s, "SNAPDIR");
  if (s2 == NULL)
    return csound->InitError(csound, Str("FLloadsnap: cannot open file"));
  strcpy(s, s2);
  csound->Free(csound, s2);
  filename = s;

  fstream file(filename.c_str(), ios::in);

  int j=0,k=-1;
  while (!(file.eof())) {
    char buf[MAXNAME];
    file.getline(buf,MAXNAME);

    stringstream sbuf;
    sbuf << buf;

    string opc, opc_orig;
    getline(sbuf, opc, ' ');
    const char *ss = opc.c_str();
    if (*ss == '-') { // if it is a separation line
      k++; j=0;
      if ((int) snapshots.size() < k+1)
        snapshots.resize(k+1);
    }
    else if (*ss != '\0' && *ss != ' ' && *ss != '\n'){ //ignore blank lines
      ADDR_SET_VALUE& v = AddrSetValue[j];
      if ((int) snapshots[k].fields.size() < j+1)
        snapshots[k].fields.resize(j+1);
      snapshots[k].is_empty = 0;
      VALUATOR_FIELD& fld = snapshots[k].fields[j];
      opc_orig = ((OPDS *) (v.opcode))->optext->t.opcod;

      if (!(opc_orig == opc)) {
        return csound->InitError(csound,
                         "unmatched widget, probably due to a modified"
                         " orchestra. Modifying an orchestra makes it "
                         "incompatible with old snapshot files");
      }
      if (opc == "FLjoy") {
        fld.opcode_name = opc;
        string s;
        getline(sbuf,s, ' ');  fld.value = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.value2 = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.min = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.max = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.min2 = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.max2 = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.exp = atoi(s.c_str());
        getline(sbuf,s, ' ');  fld.exp2 = atoi(s.c_str());
        getline(sbuf,s, '\"');
        getline(sbuf,s, '\"');  fld.widg_name = s;
      }
      else if (opc == "FLslidBnk") {
        fld.opcode_name = opc;
        string s;
        getline(sbuf,s, ' ');
        // EXCEPTIONAL CASE! fld.exp contains the number of sliders
        // and not the exponential flag
        fld.exp = atoi(s.c_str());
        fld.sldbnkValues = new MYFLT[fld.exp];
        allocatedStrings.push_back((char *) fld.sldbnkValues);

        for (int k =0; k < fld.exp; k++) {
          getline(sbuf,s, ' ');  fld.sldbnkValues[k] = atof(s.c_str());

        }
        getline(sbuf,s, '\"');
        getline(sbuf,s, '\"');  fld.widg_name = s;
      }
      else {
        fld.opcode_name = opc;
        string s;
        getline(sbuf,s, ' ');  fld.value = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.min = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.max = atof(s.c_str());
        getline(sbuf,s, ' ');  fld.exp = atoi(s.c_str());
        getline(sbuf,s, '\"');
        getline(sbuf,s, '\"');  fld.widg_name = s;
      }
      j++;
    }
  }

  file.close();
  return OK;
}

}       // extern "C"

// -----------

static char *GetString(CSOUND *csound, MYFLT *pname, int is_string)
{
  char    *Name = new char[MAXNAME];
  allocatedStrings.push_back(Name);
  return csound->strarg2name(csound, Name, pname, "", is_string);
}

extern "C" {
  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    int   j;
#ifndef NO_FLTK_THREADS
    if ((getFLTKFlags(csound) & 260) ^ 4) {
      widgetsGlobals_t *p;

      p = (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                          "_widgets_globals");
      if (p == NULL)
        return 0;
      if (!(getFLTKFlags(csound) & 256)) {
        /* if window(s) still open: */
        if (!p->exit_now) {
          /* notify GUI thread... */
          p->end_of_perf = -1;
          Fl_lock(csound);
          Fl_awake(csound);
          Fl_unlock(csound);
          /* ...and wait for it to close */
          csound->JoinThread(p->threadHandle);
          p->threadHandle = NULL;
        }
      }
      /* clean up */
      csound->LockMutex(p->mutex_);
      while (p->eventQueue != NULL) {
        rtEvt_t *nxt = p->eventQueue->nxt;
        free(p->eventQueue);
        p->eventQueue = nxt;
      }
      csound->UnlockMutex(p->mutex_);
      csound->DestroyMutex(p->mutex_);
      csound->DestroyGlobalVariable(csound, "_widgets_globals");
    }
#endif  // NO_FLTK_THREADS
    for (j = allocatedStrings.size()-1; j >= 0; j--)  {
      delete[] allocatedStrings[j];
      allocatedStrings.pop_back();
    }
    j = fl_windows.size();
    if (j > 0) {
      // destroy all opened panels
      do {
        j--;
        if (fl_windows[j].is_subwindow == 0)
          delete fl_windows[j].panel;
        fl_windows.pop_back();
      } while (j);
      Fl_wait_locked(csound, 0.0);
    }
    //for (j = AddrValue.size()-1; j >=0; j--)  {
    //      AddrValue.pop_back();
    //}
    int ss = snapshots.size();
    for (j=0; j< ss; j++) {
      snapshots[j].fields.erase(snapshots[j].fields.begin(),
                                snapshots[j].fields.end());
      snapshots.resize(snapshots.size() + 1);
    }

    AddrSetValue.erase(AddrSetValue.begin(), AddrSetValue.end());

 // curr_x =   curr_y = 0;
    stack_count       = 0;

    FLcontrol_iheight = 15;
    FLroller_iheight  = 18;
    FLcontrol_iwidth  = 400;
    FLroller_iwidth   = 150;
    FLvalue_iwidth    = 100;

    FLcolor           = -1;
    FLcolor2          = -1;
    FLtext_size       = 0;
    FLtext_color      = -1;
    FLtext_font       = -1;
    FLtext_align      = 0;
    FL_ix             = 10;
    FL_iy             = 10;
    return 0;
  }
}       // extern "C"

//-----------

#ifndef NO_FLTK_THREADS

extern "C" {

static uintptr_t fltkRun(void *userdata)
{
  volatile widgetsGlobals_t *p;
  CSOUND    *csound = (CSOUND*) userdata;
  int       j;

  p = (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                      "_widgets_globals");
#ifdef LINUX
  {
    struct sched_param  sp;
    // IV - Aug 27 2002: widget thread is always run with normal priority
    memset(&sp, 0, sizeof(struct sched_param));
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);
  }
#endif

  if (!(p->fltkFlags & 8))
    Fl::lock();
  for (j = 0; j < (int) fl_windows.size(); j++) {
    fl_windows[j].panel->show();
  }
  if (!(p->fltkFlags & 16))
    Fl::awake();
  if (!(p->fltkFlags & 8))
    Fl::unlock();
  do {
    if (!(p->fltkFlags & 8))
      Fl::lock();
    Fl::wait(0.02);
    j = (Fl::first_window() != (Fl_Window*) 0);
    if (!(p->fltkFlags & 8))
      Fl::unlock();
  } while (j && !p->end_of_perf);
  csound->Message(csound, "end of widget thread\n");
  // IV - Jun 07 2005: exit if all windows are closed
  p->exit_now = -1;
  return (uintptr_t) 0;
}

}   // extern "C"

#endif  // NO_FLTK_THREADS

extern "C" int CsoundYield_FLTK(CSOUND *csound);

extern "C" int FL_run(CSOUND *csound, FLRUN *p)
{
  int     *fltkFlags;

  fltkFlags = getFLTKFlagsPtr(csound);
  (*fltkFlags) |= 32;
#ifndef NO_FLTK_THREADS
  if (((*fltkFlags) & 260) ^ 4) {
    widgetsGlobals_t  *pp;

    if (csound->QueryGlobalVariable(csound, "_widgets_globals") != NULL)
      return csound->InitError(csound, Str("FLrun was already called"));
    if (csound->CreateGlobalVariable(csound, "_widgets_globals",
                                             sizeof(widgetsGlobals_t)) != 0)
      csound->Die(csound, Str("FL_run: memory allocation failure"));
    pp = (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                         "_widgets_globals");
    pp->fltkFlags = *fltkFlags;
    /* create thread lock */
    pp->mutex_ = csound->Create_Mutex(0);
    /* register callback function to be called by sensevents() */
    csound->RegisterSenseEventCallback(csound, (void (*)(CSOUND *, void *))
                                                   evt_callback,
                                               (void*) pp);
    if (!((*fltkFlags) & 256)) {
      pp->threadHandle = csound->CreateThread(fltkRun, (void*) csound);
      return OK;
    }
  }
#endif  // NO_FLTK_THREADS
  {
    int j;

    Fl_lock(csound);
    for (j = 0; j < (int) fl_windows.size(); j++) {
      fl_windows[j].panel->show();
    }
    Fl_wait(csound, 0.0);
    Fl_unlock(csound);
    if (!((*fltkFlags) & 256))
      csound->SetYieldCallback(csound, CsoundYield_FLTK);
  }
  return OK;
}

extern "C" int fl_update(CSOUND *csound, FLRUN *p)
{
  Fl_lock(csound);
  for (int j=0; j< (int) AddrSetValue.size()-1; j++) {
    ADDR_SET_VALUE v = AddrSetValue[j];
    Fl_Valuator *o = (Fl_Valuator *) v.WidgAddress;
    o->do_callback(o, v.opcode);
  }
  Fl_unlock(csound);
  return OK;
}

//----------------------------------------------

static inline void displ(MYFLT val, MYFLT index)
{
  if (index >= 0) { //display current value of valuator
    char valString[MAXNAME];
    sprintf(valString, "%.5g", val);
    ((Fl_Output*) (AddrSetValue[(long) index]).WidgAddress)->
      value(valString);
  }
}

static void fl_callbackButton(Fl_Button* w, void *a)
{
  FLBUTTON *p = (FLBUTTON *) a;
  *((FLBUTTON*) a)->kout =  (w->value()) ? *p->ion : *p->ioff;
  if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                    p->args, p->INOCOUNT-8);
}

static void fl_callbackButtonBank(Fl_Button* w, void *a)
{
  FLBUTTONBANK *p = (FLBUTTONBANK *) a;
  *((FLBUTTONBANK*) a)->kout = (MYFLT) atoi(w->label());
  if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                    p->args, p->INOCOUNT-7);
}

static void fl_callbackCounter(Fl_Counter* w, void *a)
{
  FLCOUNTER *p = (FLCOUNTER *) a;
  *((FLCOUNTER*) a)->kout =  w->value();
  if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                    p->args, p->INOCOUNT-10);
}

static void fl_callbackLinearSlider(Fl_Valuator* w, void *a)
{
  FLSLIDER *p = ((FLSLIDER*) a);
  displ(*p->kout = w->value(), *p->idisp);
}

static void fl_callbackExponentialSlider(Fl_Valuator* w, void *a)
{
  FLSLIDER *p = ((FLSLIDER*) a);
  displ(*p->kout = p->min * ::pow (p->base, w->value()), *p->idisp);
}

static void fl_callbackInterpTableSlider(Fl_Valuator* w, void *a)
{
  FLSLIDER *p = ((FLSLIDER*) a);
  MYFLT ndx = w->value() * (p->tablen-1);
  int index = (long) ndx;
  MYFLT v1 = p->table[index];
  displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) *
                             (ndx - index)) * (*p->imax - p->min),
        *p->idisp);
}

static void fl_callbackTableSlider(Fl_Valuator* w, void *a)
{
  FLSLIDER *p = ((FLSLIDER*) a);
  displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
        (*p->imax - p->min),
        *p->idisp);
}

static void fl_callbackLinearSliderBank(Fl_Valuator* w, void *a)
{
  SLDBK_ELEMENT* p = (SLDBK_ELEMENT*) a;
  *p->out = w->value();
}

static void fl_callbackExponentialSliderBank(Fl_Valuator* w, void *a)
{
  SLDBK_ELEMENT* p = (SLDBK_ELEMENT*) a;
  *p->out = p->min * ::pow (p->base, w->value());
}

static void fl_callbackInterpTableSliderBank(Fl_Valuator* w, void *a)
{
  SLDBK_ELEMENT *p = ((SLDBK_ELEMENT*) a);

  MYFLT ndx = w->value() * (p->tablen-1);
  int index = (long) ndx;
  MYFLT v1 = p->table[index];
  *p->out = p->min + ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
    (p->max - p->min);
}

static void fl_callbackTableSliderBank(Fl_Valuator* w, void *a)
{
  SLDBK_ELEMENT *p = ((SLDBK_ELEMENT*) a);
  *p->out = p->min + p->table[(long)(w->value() * p->tablen)] *
    (p->max - p->min);
}

static void fl_callbackJoystick(Fl_Widget* w, void *a)
{
  FLJOYSTICK *p = (FLJOYSTICK*) a;
  Fl_Positioner *j = (Fl_Positioner*) w;
  MYFLT val;
  int iexpx = (int) *p->iexpx, iexpy = (int) *p->iexpy;
  switch (iexpx) {
  case LIN_:
    val = j->xvalue();
    break;
  case EXP_:
    val = *p->iminx * ::pow (p->basex, j->xvalue());
    break;
  default:
    if (iexpx > 0) { //interpolated
      MYFLT ndx = j->xvalue() * (p->tablenx-1);
      int index = (long) ndx;
      MYFLT v1 = p->tablex[index];
      val = *p->iminx + ( v1 + (p->tablex[index+1] - v1) *
                          (ndx - index)) * (*p->imaxx - *p->iminx);
    }
    else // non-interpolated
      val = *p->iminx+ p->tablex[(long) (j->xvalue() * p->tablenx)] *
        (*p->imaxx - *p->iminx);
  }
  displ(*p->koutx = val,*p->idispx);
  switch (iexpy) {
  case LIN_:
    val = j->yvalue();
    break;
  case EXP_:
    val = *p->iminy * ::pow (p->basey, j->yvalue());
    break;
  default:
    if (iexpy > 0) { //interpolated
      MYFLT ndx = j->yvalue() * (p->tableny-1);
      long index = (long) ndx;
      MYFLT v1 = p->tabley[index];
      val = *p->iminy + ( v1 + (p->tabley[index+1] - v1) * (ndx - index))
        * (*p->imaxy - *p->iminy);
    }
    else { // non-interpolated
      long index = (long) (j->yvalue()* p->tableny);
      val = *p->iminy+ p->tabley[index] * (*p->imaxy - *p->iminy);

    }
  }
  displ(*p->kouty = val, *p->idispy);
}

static void fl_callbackLinearRoller(Fl_Valuator* w, void *a)
{
  FLROLLER *p = ((FLROLLER*) a);
  displ(*p->kout =  w->value(),*p->idisp);
}

static void fl_callbackExponentialRoller(Fl_Valuator* w, void *a)
{
  FLROLLER *p = ((FLROLLER*) a);
  displ(*p->kout = ((FLROLLER*) a)->min * ::pow (p->base, w->value()),
        *p->idisp);
}

static void fl_callbackInterpTableRoller(Fl_Valuator* w, void *a)
{
  FLROLLER *p = ((FLROLLER*) a);
  MYFLT ndx = w->value() * (p->tablen-1);
  int index = (long) ndx;
  MYFLT v1 = p->table[index];
  displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
        (*p->imax - p->min), *p->idisp);
}

static void fl_callbackTableRoller(Fl_Valuator* w, void *a)
{
  FLROLLER *p = ((FLROLLER*) a);
  displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
        (*p->imax - p->min), *p->idisp);
}

static void fl_callbackLinearKnob(Fl_Valuator* w, void *a)
{
  FLKNOB *p = ((FLKNOB*) a);
  displ( *p->kout = w->value(), *p->idisp);
}

static void fl_callbackExponentialKnob(Fl_Valuator* w, void *a)
{
  FLKNOB *p = ((FLKNOB*) a);
  displ(*p->kout = ((FLKNOB*) a)->min * ::pow (p->base, w->value()), *p->idisp);
}

static void fl_callbackInterpTableKnob(Fl_Valuator* w, void *a)
{
  FLKNOB *p = ((FLKNOB*) a);
  MYFLT ndx = w->value() * (p->tablen-1);
  int index = (long) ndx;
  MYFLT v1 = p->table[index];
  displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
        (*p->imax - p->min), *p->idisp);
}

static void fl_callbackTableKnob(Fl_Valuator* w, void *a)
{
  FLKNOB *p = ((FLKNOB*) a);
  displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
        (*p->imax - p->min), *p->idisp);
}

static void fl_callbackLinearValueInput(Fl_Valuator* w, void *a)
{
  *((FLTEXT*) a)->kout =  w->value();
}

//-----------

static int rand_31_i(CSOUND *csound, int maxVal)
{
    double  x = (double) (csound->Rand31(&(csound->randSeed2)) - 1);
    return (int) (x * (double) (maxVal + 1) / 2147483646.0);
}

static void widget_attributes(CSOUND *csound, Fl_Widget *o)
{
  if (FLtext_size == -2 ) {
    FLtext_size = -1;
    FLtext_color= -1;
    FLtext_font = -1;
    FLtext_align= -1;
    FLcolor = -1;
  }
  if (FLtext_size)
    o->labelsize(FLtext_size); // if > 0 assign it, else skip, leaving default
  switch ((int) FLtext_color) {
  case -2: // random color
    o->labelcolor(fl_rgb_color(rand_31_i(csound, 255), rand_31_i(csound, 255),
                               rand_31_i(csound, 255)));
    break;
  case -1:
    // if FLtext_color is == -1, color assignment is skipped,
    // leaving default color
    break;
  default:
    o->labelcolor(FLtext_color);
    break;
  }
  if (FLtext_font> 0) {
    Fl_Font font;
    switch (FLtext_font) {
    case 1: font  = FL_HELVETICA; break;
    case 2: font  = FL_HELVETICA_BOLD; break;
    case 3: font  = FL_HELVETICA_ITALIC; break;
    case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
    case 5: font  = FL_COURIER; break;
    case 6: font  = FL_COURIER_BOLD; break;
    case 7: font  = FL_COURIER_ITALIC; break;
    case 8: font  = FL_COURIER_BOLD_ITALIC; break;
    case 9: font  = FL_TIMES; break;
    case 10: font = FL_TIMES_BOLD; break;
    case 11: font = FL_TIMES_ITALIC; break;
    case 12: font = FL_TIMES_BOLD_ITALIC; break;
    case 13: font = FL_SYMBOL; break;
    case 14: font = FL_SCREEN; break;
    case 15: font = FL_SCREEN_BOLD; break;
    case 16: font = FL_ZAPF_DINGBATS; break;
    default: font = FL_HELVETICA; break;
    }
    o->labelfont(font);
  }
  if (FLtext_align > 0) {
    Fl_Align type;
    switch (FLtext_align) {
    case 1: type  = FL_ALIGN_CENTER; break;
    case 2: type  = FL_ALIGN_TOP; break;
    case 3: type  = FL_ALIGN_BOTTOM; break;
    case 4: type  = FL_ALIGN_LEFT; break;
    case 5: type  = FL_ALIGN_RIGHT; break;
    case 6: type  = FL_ALIGN_TOP_LEFT; break;
    case 7: type  = FL_ALIGN_TOP_RIGHT; break;
    case 8: type  = FL_ALIGN_BOTTOM_LEFT; break;
    case 9: type  = FL_ALIGN_BOTTOM_RIGHT; break;
    case -1:                // What type is this?
    default: type = FL_ALIGN_BOTTOM; break;
    }
    o->align(type);
  }
  switch ((int) FLcolor) {  // random color
  case -2:
    o->color(FL_GRAY,
             fl_rgb_color(rand_31_i(csound, 255), rand_31_i(csound, 255),
                          rand_31_i(csound, 255)));
    break;
  case -1:
    // if FLcolor is == -1, color assignment is skipped,
    // leaving widget default color
    break;
  default:
    o->color(FLcolor, FLcolor2);
    break;
  }
}

//-----------

extern "C" {

static int FLkeyb(CSOUND *csound, FLKEYB *p)
{
  (void) csound;
  (void) p;
  return OK;
}

//-----------

static int StartPanel(CSOUND *csound, FLPANEL *p)
{
  char *panelName = GetString(csound, p->name, p->XSTRCODE);

  *(getFLTKFlagsPtr(csound)) |= 32;
  int x = (int) *p->ix, y = (int) *p->iy,
    width = (int) *p->iwidth, height = (int) *p->iheight;
  if (width <0) width = 400; //default
  if (height <0) height = 300;

  int borderType;
  switch( (int) *p->border ) {
  case 0: borderType = FL_FLAT_BOX; break;
  case 1: borderType = FL_DOWN_BOX; break;
  case 2: borderType = FL_UP_BOX; break;
  case 3: borderType = FL_ENGRAVED_BOX; break;
  case 4: borderType = FL_EMBOSSED_BOX; break;
  case 5: borderType = FL_BORDER_BOX; break;
  case 6: borderType = FL_THIN_DOWN_BOX; break;
  case 7: borderType = FL_THIN_UP_BOX; break;
  default: borderType = FL_FLAT_BOX;
  }

  Fl_Window *o;
  if (x < 0) o = new Fl_Window(width,height, panelName);
  else    o = new Fl_Window(x,y,width,height, panelName);
  widget_attributes(csound, o);
  o->box((Fl_Boxtype) borderType);
  o->resizable(o);
  widget_attributes(csound, o);
  ADDR_STACK adrstk(&p->h, (void *) o, stack_count);
  AddrStack.push_back(adrstk);
  PANELS panel(o, (stack_count>0) ? 1 : 0);
  fl_windows.push_back(panel);
  stack_count++;
  return OK;
}

static int EndPanel(CSOUND *csound, FLPANELEND *p)
{
  stack_count--;
  ADDR_STACK adrstk = AddrStack.back();
  if (strcmp( adrstk.h->optext->t.opcod, "FLpanel"))
    return csound->InitError(csound, "FLpanel_end: invalid stack pointer: "
                                     "verify its placement");
  if (adrstk.count != stack_count)
    return csound->InitError(csound, "FLpanel_end: invalid stack count: "
                             "verify FLpanel/FLpanel_end count and placement");
  ((Fl_Window*) adrstk.WidgAddress)->end();
  AddrStack.pop_back();
  return OK;
}

//-----------
static int StartScroll(CSOUND *csound, FLSCROLL *p)
{
  Fl_Scroll *o = new Fl_Scroll ((int) *p->ix, (int) *p->iy,
                                (int) *p->iwidth, (int) *p->iheight);
  ADDR_STACK adrstk(&p->h,o,stack_count);
  AddrStack.push_back(adrstk);
  stack_count++;
  return OK;
}

static int EndScroll(CSOUND *csound, FLSCROLLEND *p)
{
  stack_count--;
  ADDR_STACK adrstk = AddrStack.back();
  if (strcmp( adrstk.h->optext->t.opcod, "FLscroll"))
    return
      csound->InitError(csound, "FLscroll_end: invalid stack pointer: "
                                "verify its placement");
  if (adrstk.count != stack_count)
    return csound->InitError(csound, "FLscroll_end: invalid stack count: "
                            "verify FLscroll/FLscroll_end count and placement");
  ((Fl_Scroll*) adrstk.WidgAddress)->end();

  AddrStack.pop_back();
  return OK;
}

//-----------
static int StartTabs(CSOUND *csound, FLTABS *p)
{
  Fl_Tabs *o = new Fl_Tabs ((int) *p->ix, (int) *p->iy,
                            (int) *p->iwidth, (int) *p->iheight);
  widget_attributes(csound, o);
  ADDR_STACK adrstk(&p->h,o,stack_count);
  AddrStack.push_back(adrstk);
  stack_count++;
  return OK;
}

static int EndTabs(CSOUND *csound, FLTABSEND *p)
{
  stack_count--;
  ADDR_STACK adrstk = AddrStack.back();
  if (strcmp( adrstk.h->optext->t.opcod, "FLtabs"))
    return
      csound->InitError(csound, "FLscroll_end: invalid stack pointer: "
                                "verify its placement");
  if (adrstk.count != stack_count)
    return csound->InitError(csound, "FLtabs_end: invalid stack count: "
                     "verify FLtabs/FLtabs_end count and placement");
  ((Fl_Scroll*) adrstk.WidgAddress)->end();

  AddrStack.pop_back();
  return OK;
}

//-----------
static int StartGroup(CSOUND *csound, FLGROUP *p)
{
  char *Name = GetString(csound, p->name, p->XSTRCODE);
  Fl_Group *o = new Fl_Group ((int) *p->ix, (int) *p->iy,
                              (int) *p->iwidth, (int) *p->iheight,Name);
  widget_attributes(csound, o);
  int borderType;
  switch((int)*p->border ) {
  case 0: borderType = FL_FLAT_BOX; break;
  case 1: borderType = FL_DOWN_BOX; break;
  case 2: borderType = FL_UP_BOX; break;
  case 3: borderType = FL_ENGRAVED_BOX; break;
  case 4: borderType = FL_EMBOSSED_BOX; break;
  case 5: borderType = FL_BORDER_BOX; break;
  case 6: borderType = FL_THIN_DOWN_BOX; break;
  case 7: borderType = FL_THIN_UP_BOX; break;
  default: borderType = FL_FLAT_BOX;
  }
  o->box((Fl_Boxtype) borderType);
  widget_attributes(csound, o);
  ADDR_STACK adrstk(&p->h,o,stack_count);
  AddrStack.push_back(adrstk);
  stack_count++;
  return OK;
}

static int EndGroup(CSOUND *csound, FLGROUPEND *p)
{
  stack_count--;
  ADDR_STACK adrstk = AddrStack.back();
  if (strcmp( adrstk.h->optext->t.opcod, "FLgroup"))
    return csound->InitError(csound, "FLgroup_end: invalid stack pointer: "
                                     "verify its placement");
  if (adrstk.count != stack_count)
    return csound->InitError(csound, "FLgroup_end: invalid stack count: "
                             "verify FLgroup/FLgroup_end count and placement");
  ((Fl_Scroll*) adrstk.WidgAddress)->end();

  AddrStack.pop_back();
  return OK;
}

//-----------

static int StartPack(CSOUND *csound, FLSCROLL *p)
{
  Fl_Pack *o = new Fl_Pack ((int) *p->ix, (int) *p->iy,
                            (int) *p->iwidth, (int) *p->iheight);
  //fl_window->resizable(o);
  o->box(FL_ENGRAVED_FRAME);
  o->spacing(15);

  ADDR_STACK adrstk(&p->h,o,stack_count);;
  AddrStack.push_back(adrstk);
  stack_count++;
  return OK;
}

static int EndPack(CSOUND *csound, FLSCROLLEND *p)
{
  stack_count--;
  ADDR_STACK adrstk = AddrStack.back();
  if (strcmp( adrstk.h->optext->t.opcod, "FLpack"))
    return csound->InitError(csound, "FLpack_end: invalid stack pointer: "
                                     "verify its placement");
  if (adrstk.count != stack_count)
    return csound->InitError(csound, "FLpack_end: invalid stack count: "
                             "verify FLpack/FLpack_end count and placement");
  ((Fl_Pack*) adrstk.WidgAddress)->end();

  AddrStack.pop_back();
  return OK;
}

//-----------

static int fl_widget_color(CSOUND *csound, FLWIDGCOL *p)
{
  if (*p->red1 < 0) { // reset colors to default
    FLcolor = (int) *p->red1; //when called without arguments
    FLcolor2 =(int) *p->red1;
  }
  else {
    FLcolor = fl_rgb_color((int) *p->red1,
                           (int) *p->green1,
                           (int) *p->blue1);
    FLcolor2 = fl_rgb_color((int) *p->red2,
                            (int) *p->green2,
                            (int) *p->blue2);
  }
  return OK;
}

static int fl_widget_color2(CSOUND *csound, FLWIDGCOL2 *p)
{
  if (*p->red < 0) { // reset colors to default
    FLcolor2 =(int) *p->red;
  }
  else {
    FLcolor2 = fl_rgb_color((int) *p->red,
                            (int) *p->green,
                            (int) *p->blue);
  }
  return OK;
}

static int fl_widget_label(CSOUND *csound, FLWIDGLABEL *p)
{
  if (*p->size <= 0) { // reset settings to default
    FLtext_size = 0; //when called without arguments
    FLtext_font = -1;
    FLtext_align = 0;
    FLtext_color = -1;
  }
  else {
    FLtext_size = (int) *p->size;

    if (*p->font > -1) FLtext_font = (int) *p->font;
    if (*p->align > 0)  FLtext_align =  (int) *p->align;
    if (*p->red > -1 && *p->green > -1 && *p->blue > -1) {
      FLtext_color = fl_rgb_color((int) *p->red,
                                  (int) *p->green,
                                  (int) *p->blue);
    }
  }
  return OK;
}

}       // extern "C"

// -----------

static int fl_getWidgetTypeFromOpcodeName(CSOUND *csound, void *p)
{
    const char  *opname = csound->GetOpcodeName(p);

    if (strcmp(opname, "FLbutton") == 0)
      return 1;
    if (strcmp(opname, "FLbutBank") == 0)
      return 2;
    if (strcmp(opname, "FLjoy") == 0)
      return 3;
    if (strcmp(opname, "FLbox") != 0)
      return 0;
    csound->Warning(csound, "System error: value() method called from "
                            "non-valuator object");
    return -1;
}

static void fl_setWidgetValue_(CSOUND *csound,
                               ADDR_SET_VALUE &v, int widgetType,
                               MYFLT val, MYFLT log_base)
{
    Fl_Widget   *o = (Fl_Widget *) v.WidgAddress;
    void        *p = v.opcode;
    bool        fltkLockingIsEnabled;

    if ((!widgetType || widgetType > 2) &&
        (v.exponential == LIN_ || v.exponential == EXP_)) {
      if (val < v.min)
        val = v.min;
      else if (val > v.max)
        val = v.max;
      if (v.exponential == EXP_)
        val = (MYFLT) (log(val / v.min) / log_base);
    }
    fltkLockingIsEnabled = ((getFLTKFlags(csound) & 8) == 0);
    if (fltkLockingIsEnabled)
      Fl_lock(csound);
    switch (widgetType) {
    case 0:                                     // valuator
      ((Fl_Valuator *) o)->value(val);
      break;
    case 1:                                     // FLbutton
      if (val == *(((FLBUTTON *) v.opcode)->ion))
        ((Fl_Button *) o)->value(1);
      else if (val == *(((FLBUTTON *) v.opcode)->ioff))
        ((Fl_Button *) o)->value(0);
      break;
    case 2:                                     // FLbutBank
      set_butbank_value((Fl_Group *) o, val);
      break;
    case 3:                                     // FLjoy
      {
        static int  flag = 0;
        // FLsetVal always requires two adjacent calls when setting FLjoy
        if (!flag) {
          ((Fl_Positioner *) o)->xvalue(val);
          flag = 1;
        }
        else {
          ((Fl_Positioner *) o)->yvalue(val);
          flag = 0;
        }
      }
      break;
    default:                                    // invalid
      if (fltkLockingIsEnabled)
        Fl_unlock(csound);
      return;
    }
    o->do_callback(o, p);
    if (fltkLockingIsEnabled)
      Fl_unlock(csound);
}

extern "C" {

static int fl_setWidgetValuei(CSOUND *csound, FL_SET_WIDGET_VALUE_I *p)
{
    MYFLT           log_base = FL(1.0);
    ADDR_SET_VALUE  &v = AddrSetValue[(int) *p->ihandle];
    int             widgetType;

    widgetType = fl_getWidgetTypeFromOpcodeName(csound, v.opcode);
    if (widgetType < 0)
      return OK;
    if (!widgetType || widgetType > 2) {
      switch (v.exponential) {
      case LIN_:        // linear
        break;
      case EXP_:        // exponential
        log_base = (MYFLT) log(::pow(v.max / v.min, 1.0 / (v.max - v.min)));
        break;
      default:
        csound->Warning(csound, "(fl_setWidgetValuei): "
                                "not implemented yet; exp=%d", v.exponential);
      }
    }
    fl_setWidgetValue_(csound, v, widgetType, *(p->ivalue), log_base);
    return OK;
}

static int fl_setWidgetValue_set(CSOUND *csound, FL_SET_WIDGET_VALUE *p)
{
    p->handle = (int) *(p->ihandle);

    MYFLT           log_base = FL(1.0);
    ADDR_SET_VALUE  &v = AddrSetValue[p->handle];
    int             widgetType;

    widgetType = fl_getWidgetTypeFromOpcodeName(csound, v.opcode);
    if (widgetType < 0)
      return OK;
    if (!widgetType || widgetType > 2) {
      switch (v.exponential) {
      case LIN_:        // linear
        break;
      case EXP_:        // exponential
        log_base = (MYFLT) log(::pow(v.max / v.min, 1.0 / (v.max - v.min)));
        break;
      default:
        csound->Warning(csound, "(fl_setWidgetValue_set): "
                                "not implemented yet; exp=%d", v.exponential);
      }
    }
    p->widgetType = widgetType;
    p->log_base = log_base;

    return OK;
}

static int fl_setWidgetValue(CSOUND *csound, FL_SET_WIDGET_VALUE *p)
{
    if (*p->ktrig != FL(0.0))
      fl_setWidgetValue_(csound, AddrSetValue[p->handle], p->widgetType,
                         *(p->kvalue), p->log_base);
    return OK;
}

//-----------
//-----------

static int fl_setColor1(CSOUND *csound, FL_SET_COLOR *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  int color = fl_rgb_color((int) *p->red,
                           (int) *p->green,
                           (int) *p->blue);
  o->color(color);
  return OK;
}

static int fl_setColor2(CSOUND *csound, FL_SET_COLOR *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  int color = fl_rgb_color((int) *p->red, (int) *p->green, (int) *p->blue);
  o->selection_color(color);
  return OK;
}

static int fl_setTextColor(CSOUND *csound, FL_SET_COLOR *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  int color = fl_rgb_color((int) *p->red, (int) *p->green, (int) *p->blue);
  o->labelcolor(color);
  return OK;
}

static int fl_setTextSize(CSOUND *csound, FL_SET_TEXTSIZE *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  o->labelsize((uchar) *p->ivalue);
  return OK;
}

static int fl_setFont(CSOUND *csound, FL_SET_FONT *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  Fl_Font font;
  switch ((int) *p->itype) {
  case 1: font  = FL_HELVETICA; break;
  case 2: font  = FL_HELVETICA_BOLD; break;
  case 3: font  = FL_HELVETICA_ITALIC; break;
  case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
  case 5: font  = FL_COURIER; break;
  case 6: font  = FL_COURIER_BOLD; break;
  case 7: font  = FL_COURIER_ITALIC; break;
  case 8: font  = FL_COURIER_BOLD_ITALIC; break;
  case 9: font  = FL_TIMES; break;
  case 10: font = FL_TIMES_BOLD; break;
  case 11: font = FL_TIMES_ITALIC; break;
  case 12: font = FL_TIMES_BOLD_ITALIC; break;
  case 13: font = FL_SYMBOL; break;
  case 14: font = FL_SCREEN; break;
  case 15: font = FL_SCREEN_BOLD; break;
  case 16: font = FL_ZAPF_DINGBATS; break;
  default: font = FL_SCREEN;
  }
  o->labelfont(font);
  return OK;
}

static int fl_setTextType(CSOUND *csound, FL_SET_FONT *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  Fl_Labeltype type;
  switch ((int) *p->itype) {
  case 0: type  = FL_NORMAL_LABEL; break;
  case 1: type  = FL_NO_LABEL; break;
  case 2: type  = FL_SYMBOL_LABEL; break;
  case 3: type  = FL_SHADOW_LABEL; break;
  case 4: type  = FL_ENGRAVED_LABEL; break;
  case 5: type  = FL_EMBOSSED_LABEL; break;
    /*    case 6: type  = _FL_BITMAP_LABEL; break;
          case 7: type  = _FL_PIXMAP_LABEL; break;
          case 8: type  = _FL_IMAGE_LABEL; break;
          case 9: type  = _FL_MULTI_LABEL; break; */
  case 10: type = FL_FREE_LABELTYPE; break;
  default: type = FL_NORMAL_LABEL;
  }
  o->labeltype(type);
  o->window()->redraw();
  return OK;
}

static int fl_box(CSOUND *csound, FL_BOX *p)
{
  char *text = GetString(csound, p->itext, p->XSTRCODE);
  Fl_Box *o =  new Fl_Box((int)*p->ix, (int)*p->iy,
                          (int)*p->iwidth, (int)*p->iheight, text);
  widget_attributes(csound, o);
  Fl_Boxtype type;
  switch ((int) *p->itype) {
  case 1: type  = FL_FLAT_BOX; break;
  case 2: type  = FL_UP_BOX; break;
  case 3: type  = FL_DOWN_BOX; break;
  case 4: type  = FL_THIN_UP_BOX; break;
  case 5: type  = FL_THIN_DOWN_BOX; break;
  case 6: type  = FL_ENGRAVED_BOX; break;
  case 7: type  = FL_EMBOSSED_BOX; break;
  case 8: type  = FL_BORDER_BOX; break;
  case 9: type  = _FL_SHADOW_BOX; break;
  case 10: type = _FL_ROUNDED_BOX; break;
  case 11: type = _FL_RSHADOW_BOX; break;
  case 12: type = _FL_RFLAT_BOX; break;
  case 13: type = _FL_ROUND_UP_BOX; break;
  case 14: type = _FL_ROUND_DOWN_BOX; break;
  case 15: type = _FL_DIAMOND_UP_BOX; break;
  case 16: type = _FL_DIAMOND_DOWN_BOX; break;
  case 17: type = _FL_OVAL_BOX; break;
  case 18: type = _FL_OSHADOW_BOX; break;
  case 19: type = _FL_OFLAT_BOX; break;
  default: type = FL_FLAT_BOX;
  }
  o->box(type);
  Fl_Font font;
  switch ((int) *p->ifont) {
  case 1: font  = FL_HELVETICA; break;
  case 2: font  = FL_HELVETICA_BOLD; break;
  case 3: font  = FL_HELVETICA_ITALIC; break;
  case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
  case 5: font  = FL_COURIER; break;
  case 6: font  = FL_COURIER_BOLD; break;
  case 7: font  = FL_COURIER_ITALIC; break;
  case 8: font  = FL_COURIER_BOLD_ITALIC; break;
  case 9: font  = FL_TIMES; break;
  case 10: font = FL_TIMES_BOLD; break;
  case 11: font = FL_TIMES_ITALIC; break;
  case 12: font = FL_TIMES_BOLD_ITALIC; break;
  case 13: font = FL_SYMBOL; break;
  case 14: font = FL_SCREEN; break;
  case 15: font = FL_SCREEN_BOLD; break;
  case 16: font = FL_ZAPF_DINGBATS; break;
  default: font = FL_HELVETICA;
  }
  o->labelfont(font);
  o->labelsize((unsigned char)*p->isize);
  o->align(FL_ALIGN_WRAP);
  AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *)o, (void *)p));
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_setText(CSOUND *csound, FL_SET_TEXT *p)
{
  char *text = GetString(csound, p->itext, p->XSTRCODE);
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  o->label(text);
  return OK;
}

static int fl_setSize(CSOUND *csound, FL_SET_SIZE *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  o->size((short)  *p->iwidth, (short) *p->iheight);
  return OK;
}

static int fl_setPosition(CSOUND *csound, FL_SET_POSITION *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  o->position((short)  *p->ix, (short) *p->iy);
  return OK;
}

static int fl_hide(CSOUND *csound, FL_WIDHIDE *p)
{
  Fl_lock(csound);
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  o->hide();
  Fl_unlock(csound);
  return OK;
}

static int fl_show(CSOUND *csound, FL_WIDSHOW *p)
{
  Fl_lock(csound);
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  o->show();
  Fl_unlock(csound);
  return OK;
}

static int fl_setBox(CSOUND *csound, FL_SETBOX *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  Fl_Boxtype type;
  switch ((int) *p->itype) {
  case 1: type  = FL_FLAT_BOX; break;
  case 2: type  = FL_UP_BOX; break;
  case 3: type  = FL_DOWN_BOX; break;
  case 4: type  = FL_THIN_UP_BOX; break;
  case 5: type  = FL_THIN_DOWN_BOX; break;
  case 6: type  = FL_ENGRAVED_BOX; break;
  case 7: type  = FL_EMBOSSED_BOX; break;
  case 8: type  = FL_BORDER_BOX; break;
  case 9: type  = FL_SHADOW_BOX; break;
  case 10: type = FL_ROUNDED_BOX; break;
  case 11: type = FL_RSHADOW_BOX; break;
  case 12: type = FL_RFLAT_BOX; break;
  case 13: type = FL_ROUND_UP_BOX; break;
  case 14: type = FL_ROUND_DOWN_BOX; break;
  case 15: type = FL_DIAMOND_UP_BOX; break;
  case 16: type = FL_DIAMOND_DOWN_BOX; break;
  case 17: type = FL_OVAL_BOX; break;
  case 18: type = FL_OSHADOW_BOX; break;
  case 19: type = FL_OFLAT_BOX; break;
  default: type = FL_FLAT_BOX;
  }
  o->box(type);
  return OK;
}

static int fl_align(CSOUND *csound, FL_TALIGN *p)
{
  ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
  Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
  Fl_Align type;
  switch ((int) *p->itype) {
  case 1: type  = FL_ALIGN_CENTER; break;
  case 2: type  = FL_ALIGN_TOP; break;
  case 3: type  = FL_ALIGN_BOTTOM; break;
  case 4: type  = FL_ALIGN_LEFT; break;
  case 5: type  = FL_ALIGN_RIGHT; break;
  case 6: type  = FL_ALIGN_TOP_LEFT; break;
  case 7: type  = FL_ALIGN_TOP_RIGHT; break;
  case 8: type  = FL_ALIGN_BOTTOM_LEFT; break;
  case 9: type  = FL_ALIGN_BOTTOM_RIGHT; break;
  default: type = FL_ALIGN_BOTTOM;
  }
  o->align(type);
  return OK;
}

//-----------
//-----------

static int fl_value(CSOUND *csound, FLVALUE *p)
{
  char *controlName = GetString(csound, p->name, p->XSTRCODE);
  int ix, iy, iwidth, iheight;
  if (*p->ix<0) ix = FL_ix;       else  FL_ix = ix = (int) *p->ix;
  if (*p->iy<0) iy = FL_iy;       else  FL_iy = iy = (int) *p->iy;
  if (*p->iwidth<0) iwidth = FLvalue_iwidth;
  else FLvalue_iwidth = iwidth = (int) *p->iwidth;
  if (*p->iheight<0) iheight = FLroller_iheight;
  else FLroller_iheight = iheight = (int) *p->iheight;

  Fl_Output *o = new Fl_Output(ix, iy, iwidth, iheight,controlName);
  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
  if (FLcolor < 0 )
    o->color(FL_GRAY );
  else
    o->color(FLcolor, FLcolor2);
  widget_attributes(csound, o);
  //AddrValue.push_back((void *) o);
  AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p));
  //*p->ihandle = AddrValue.size()-1;
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

//-----------

static int fl_slider(CSOUND *csound, FLSLIDER *p)
{
  char *controlName = GetString(csound, p->name, p->XSTRCODE);
  int ix,iy,iwidth, iheight,itype, iexp;

  if (*p->iy < 0) {
    iy = FL_iy;
    FL_iy += FLcontrol_iheight + 5;
  }
  else {
    iy = (int) *p->iy;
    FL_iy = iy + FLcontrol_iheight + 5;
  }
  if (*p->ix < 0)  ix = FL_ix; // omitted options: set default
  else  FL_ix = ix = (int) *p->ix;
  if (*p->iwidth < 0) iwidth = FLcontrol_iwidth;
  else FLcontrol_iwidth = iwidth = (int) *p->iwidth;
  if (*p->iheight < 0) iheight = FLcontrol_iheight;
  else FLcontrol_iheight = iheight = (int) *p->iheight;
  if (*p->itype < 1) itype = 1;
  else  itype = (int) *p->itype;

  //if (*p->iexp == LIN_) iexp = LIN_;
  //else  iexp = (int) *p->iexp;
  switch((int) *p->iexp) {
  case -1: iexp = EXP_; break;
  case 0: iexp = LIN_; break;
  default: iexp = (int) *p->iexp;
  }

  if (itype > 10 && iexp == EXP_) {
    if (csound->oparms->msglevel & WARNMSG)
      csound->Warning(csound, "FLslider exponential, using non-labeled slider");
    itype -= 10;
  }

  Fl_Slider *o;
  if (itype < 10) o = new Fl_Slider(ix, iy, iwidth, iheight, controlName);
  else {
    o = new Fl_Value_Slider_Input(ix, iy, iwidth, iheight, controlName);
    itype -=10;
    //o->labelsize(20);
    ((Fl_Value_Slider_Input*) o)->textboxsize(50);
    ((Fl_Value_Slider_Input*) o)->textsize(13);
    o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
  }
  switch (itype) {
  case 1:  o->type(FL_HOR_FILL_SLIDER); break;
  case 2:  o->type(FL_VERT_FILL_SLIDER); break;
  case 3:  o->type(FL_HOR_SLIDER); break;
  case 4:  o->type(FL_VERT_SLIDER); break;
  case 5:  o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
  case 6:  o->type(FL_VERT_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
  default: return csound->InitError(csound, "FLslider: invalid slider type");
  }
  widget_attributes(csound, o);
  MYFLT min = p->min = *p->imin, max = *p->imax, range;
  switch (iexp) {
  case LIN_: //linear
    o->range(min,max);
    o->callback((Fl_Callback*)fl_callbackLinearSlider,(void *) p);
    break;
  case EXP_ : //exponential
    if (min == 0 || max == 0)
      return csound->InitError(csound, "FLslider: zero is illegal "
                                       "in exponential operations");
    range = max - min;
    o->range(0,range);
    p->base = ::pow((max / min), 1/range);
    o->callback((Fl_Callback*)fl_callbackExponentialSlider,(void *) p);
    break;
  default:
    {
      FUNC *ftp;
      MYFLT fnum = abs(iexp);
      if ((ftp = csound->FTFind(csound, &fnum)) != NULL) {
        p->table = ftp->ftable;
        p->tablen = ftp->flen;
      }
      else return NOTOK;
      o->range(0,.99999999);
      if (iexp > 0) //interpolated
        o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
      else // non-interpolated
        o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
    }
  }
  AddrSetValue.push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                        (void *) o, (void *) p));
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_slider_bank(CSOUND *csound, FLSLIDERBANK *p)
{
  char s[MAXNAME];
  if (p->XSTRCODE)
    strcpy(s, (char*) p->names);
  else if ((long) *p->names <= csound->strsmax &&
           csound->strsets != NULL && csound->strsets[(long) *p->names]) {
    strcpy(s, csound->strsets[(long) *p->names]);
  }
  string tempname(s);
  stringstream sbuf;
  sbuf << tempname;

  int width = (int) *p->iwidth;
  if (width <=0) width = 100;

  Fl_Group* w = new Fl_Group((int)*p->ix, (int)*p->iy,
                             width, (int)*p->inumsliders*10);
  FUNC *ftp;
  MYFLT *minmaxtable = NULL, *typetable = NULL, *outable, *exptable = NULL;

  if (*p->ioutable  < 1) {
    if (csound->zkstart != NULL &&
        csound->zklast > (long) (*p->inumsliders + *p->ioutablestart_ndx))
      outable = csound->zkstart + (long) *p->ioutablestart_ndx;
    else {
      return csound->InitError(csound, "invalid ZAK space allocation");
    }
  }
  else {
    if ((ftp = csound->FTFind(csound, p->ioutable)) != NULL)
      outable = ftp->ftable + (long) *p->ioutablestart_ndx;
    else
      return NOTOK;
  }
  if ((int) *p->iminmaxtable > 0) {
    if ((ftp = csound->FTFind(csound, p->iminmaxtable)) != NULL)
      minmaxtable = ftp->ftable;
    else return NOTOK;
  }
  if ((int) *p->iexptable > 0) {
    if ((ftp = csound->FTFind(csound, p->iexptable)) != NULL)
      exptable = ftp->ftable;
    else return NOTOK;
  }
  if ((int) *p->itypetable > 0) {
    if ((ftp = csound->FTFind(csound, p->itypetable)) != NULL)
      typetable = ftp->ftable;
    else return NOTOK;
  }

  for (int j =0; j< *p->inumsliders; j++) {
    string stemp;
    if (tempname == " ") {
      char s[40];
      sprintf(s, "%d", j);
      stemp = s;
    }
    else
      getline(sbuf, stemp, '@');
    char *Name =  new char[stemp.size()+2];
    strcpy(Name,stemp.c_str());
    allocatedStrings.push_back(Name);

    int x = (int) *p->ix,  y = (int) *p->iy + j*10;
    Fl_Slider *o;
    int slider_type;
    if ((int) *p->itypetable <= 0) {    // no slider type table
      if (*p->itypetable >= -7)         //  all sliders are of the same type
        slider_type = -((int) *p->itypetable);
      else                              // random type
        slider_type = rand_31_i(csound, 7) | 1;
    }
    else
      slider_type = (int) typetable[j];

    if (slider_type < 10)
      o = new Fl_Slider(x, y, width, 10, Name);
    else {
      o = new Fl_Value_Slider_Input(x, y, width, 10, Name);
      slider_type -=10;
      ((Fl_Value_Slider_Input*) o)->textboxsize(50);
      ((Fl_Value_Slider_Input*) o)->textsize(13);
    }
    switch((int) slider_type) { //type
    case 1: o->type(FL_HOR_FILL_SLIDER); break;
    case 3: o->type(FL_HOR_SLIDER); break;
    case 5: o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
    case 7:  o->type(FL_HOR_NICE_SLIDER); o->box(FL_DOWN_BOX); break;
    default: o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
    }
    o->align(FL_ALIGN_LEFT);
    widget_attributes(csound, o);
    MYFLT min, max, range;
    if ((int) *p->iminmaxtable > 0) {
      min = minmaxtable[j*2];
      max = minmaxtable[j*2+1];
    }
    else {
      min = FL(0.0);
      max = FL(1.0);
    }
    int iexp;

    p->slider_data[j].min=min;
    p->slider_data[j].max=max;
    p->slider_data[j].out=&outable[j];

    if ((int) *p->iexptable <=0)
      // no table, all sliders have the same behaviour
      iexp = (int) *p->iexptable;
    else
      iexp = (int) exptable[j];
    switch (iexp) {
    case -1: iexp = EXP_; break;
    case 0: iexp = LIN_; break;
    }

    MYFLT val = 0;
    p->slider_data[j].exp = iexp;
    switch (iexp) {
    case LIN_: //linear
      o->range(min,max);
      o->callback((Fl_Callback*)fl_callbackLinearSliderBank,
                  (void *) &(p->slider_data[j]));
      val = outable[j];
      if (val > max) val = max;
      else if (val < min) val = min;
      break;
    case EXP_ : //exponential
      if (min == 0 || max == 0)
        return
          csound->InitError(csound, "FLsliderBank: zero is illegal "
                                    "in exponential operations");
      range = max - min;
      o->range(0,range);
      p->slider_data[j].base = ::pow((max / min), 1/range);
      o->callback((Fl_Callback*)fl_callbackExponentialSliderBank,
                  (void *) &(p->slider_data[j]));
      {
        val = outable[j];
        MYFLT range = max-min;
        MYFLT base = ::pow(max / min, 1/range);
        val = (log(val/min) / log(base)) ;
      }
      break;
    default:
      {
        FUNC *ftp;
        MYFLT fnum = abs(iexp);
        if ((ftp = csound->FTFind(csound, &fnum)) != NULL)
          p->slider_data[j].table = ftp->ftable;
        else return NOTOK;
        p->slider_data[j].tablen = ftp->flen;
        o->range(0,.99999999);
        if (iexp > 0) //interpolated
          o->callback((Fl_Callback*)fl_callbackInterpTableSliderBank,
                      (void *)  &(p->slider_data[j]));
        else // non-interpolated
          o->callback((Fl_Callback*)fl_callbackTableSliderBank,
                      (void *)  &(p->slider_data[j]));
      }
    }
    o->value(val);
  }
  w->resizable(w);
  if (*p->iwidth <=0 || *p->iheight <=0) {// default width and height
    int a,b;
    w->size( a= w->parent()->w() -50, b= w->parent()->h());
    w->position(50, 0);
  }
  else {
    w->size( (int)*p->iwidth, (int)*p->iheight);
    w->position((int)*p->ix, (int)*p->iy);
  }
  w->end();
  AddrSetValue.push_back(ADDR_SET_VALUE(LIN_, 0, 0, (void *) w, (void *) p));
  //*p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_joystick(CSOUND *csound, FLJOYSTICK *p)
{
  char *Name = GetString(csound, p->name, p->XSTRCODE);
  int ix,iy,iwidth, iheight, iexpx, iexpy;

  if (*p->ix < 0)  ix = 10; // omitted options: set default
  else  FL_ix = ix = (int) *p->ix;
  if (*p->iy < 0)  iy = 10; // omitted options: set default
  else  iy = (int) *p->iy;
  if (*p->iwidth < 0) iwidth = 130;
  else iwidth = (int) *p->iwidth;
  if (*p->iheight < 0) iheight = 130;
  else iheight = (int) *p->iheight;

  switch((int) *p->iexpx) {
  case -1: iexpx = EXP_; break;
  case 0: iexpx = LIN_; break;
  default: iexpx = (int) *p->iexpx;
  }
  switch((int) *p->iexpy) {
  case -1: iexpy = EXP_; break;
  case 0: iexpy = LIN_; break;
  default: iexpy = (int) *p->iexpy;
  }
  /*
    if (*p->iexpx == LIN_) iexpx = LIN_;
    else  iexpx = (int) *p->iexpx;
    if (*p->iexpy == LIN_) iexpy = LIN_;
    else  iexpy = (int) *p->iexpy;
  */

  Fl_Positioner *o = new Fl_Positioner(ix, iy, iwidth, iheight, Name);
  widget_attributes(csound, o);
  switch (iexpx) {
  case LIN_: //linear
    o->xbounds(*p->iminx,*p->imaxx); break;
  case EXP_: //exponential
    { if (*p->iminx == 0 || *p->imaxx == 0)
      return csound->InitError(csound, "FLjoy X axe: zero is illegal "
                                       "in exponential operations");
    MYFLT range = *p->imaxx - *p->iminx;
    o->xbounds(0,range);
    p->basex = ::pow((*p->imaxx / *p->iminx), 1/range);
    } break;
  default:
    {
      FUNC *ftp;
      MYFLT fnum = abs(iexpx);
      if ((ftp = csound->FTFind(csound, &fnum)) != NULL) {
        p->tablex = ftp->ftable;
        p->tablenx = ftp->flen;
      }
      else return NOTOK;
      o->xbounds(0,.99999999);
      /*
        if (iexp > 0) //interpolated
        o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
        else // non-interpolated
        o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
      */
    }
  }
  switch (iexpy) {
  case LIN_: //linear
    o->ybounds(*p->imaxy,*p->iminy); break;
  case EXP_ : //exponential
    { if (*p->iminy == 0 || *p->imaxy == 0)
      return csound->InitError(csound, "FLjoy X axe: zero is illegal "
                                       "in exponential operations");
    MYFLT range = *p->imaxy - *p->iminy;
    o->ybounds(range,0);
    p->basey = ::pow((*p->imaxy / *p->iminy), 1/range);
    } break;
  default:
    {
      FUNC *ftp;
      MYFLT fnum = abs(iexpy);
      if ((ftp = csound->FTFind(csound, &fnum)) != NULL) {
        p->tabley = ftp->ftable;
        p->tableny = ftp->flen;
      }
      else return NOTOK;
      o->ybounds(0,.99999999);
  /*  if (iexp > 0) //interpolated
        o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
      else // non-interpolated
        o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
   */
    }
  }
  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
  o->callback((Fl_Callback*)fl_callbackJoystick,(void *) p);
  AddrSetValue.push_back(ADDR_SET_VALUE(iexpx, *p->iminx, *p->imaxx,
                                        (void *) o, (void *) p));
  *p->ihandle1 = AddrSetValue.size()-1;
  AddrSetValue.push_back(ADDR_SET_VALUE(iexpy, *p->iminy, *p->imaxy,
                                        (void *) o, (void *) p));
  *p->ihandle2 = AddrSetValue.size()-1;
  return OK;
}

static int fl_knob(CSOUND *csound, FLKNOB *p)
{
  char    *controlName = GetString(csound, p->name, p->XSTRCODE);
  int     ix, iy, iwidth, itype, iexp;

  if (*p->iy < 0) iy = FL_iy;
  else  FL_iy = iy = (int) *p->iy;
  if (*p->ix < 0)  ix = FL_ix;
  else  FL_ix = ix = (int) *p->ix;
  if (*p->iwidth < 0) iwidth = FLcontrol_iwidth;
  else FLcontrol_iwidth = iwidth = (int) *p->iwidth;
  if (*p->itype < 1) itype = 1;
  else  itype = (int) *p->itype;
  /*
    if (*p->iexp < LIN_) iexp = LIN_;
    else  iexp = (int) *p->iexp;
  */
  switch((int) *p->iexp) {
  case -1: iexp = EXP_; break;
  case 0: iexp = LIN_; break;
  default: iexp = (int) *p->iexp;
  }

  Fl_Valuator* o;
  switch (itype) {
  case 1:
    o = new Fl_Knob(ix, iy, iwidth, iwidth, controlName);
    o->box(FL_NO_BOX);
    if (*p->icursorsize > FL(0.5))
      ((Fl_Knob*) o)->cursor((int) (*p->icursorsize + FL(0.5)));
    break;
  case 2:
    o = new Fl_Dial(ix, iy, iwidth, iwidth, controlName);
    o->type(FL_FILL_DIAL);
    o->box(_FL_OSHADOW_BOX);
    ((Fl_Dial*) o)->angles(20, 340);
    break;
  case 3:
    o = new Fl_Dial(ix, iy, iwidth, iwidth, controlName);
    o->type(FL_LINE_DIAL);
    o->box(_FL_OSHADOW_BOX);
    break;
  case 4:
    o = new Fl_Dial(ix, iy, iwidth, iwidth, controlName);
    o->type(FL_NORMAL_DIAL);
    o->box(_FL_OSHADOW_BOX);
    break;
  default:
    return csound->InitError(csound, "FLknob: invalid knob type");
  }
  widget_attributes(csound, o);
  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
  o->range(*p->imin, *p->imax);
  switch (iexp) {
  case LIN_: //linear
    o->range(*p->imin,*p->imax);
    o->callback((Fl_Callback*)fl_callbackLinearKnob,(void *) p);
    o->step(0.001);
    break;
  case EXP_ : //exponential
    {
      MYFLT min = p->min = *p->imin, max = *p->imax;
      if (min == 0 || max == 0)
        return csound->InitError(csound, "FLknob: zero is illegal "
                                         "in exponential operations");
      MYFLT range = max - min;
      o->range(0,range);
      p->base = ::pow((max / min), 1/range);
      o->callback((Fl_Callback*)fl_callbackExponentialKnob,(void *) p);
    } break;
  default:
    {
      FUNC *ftp;
      p->min = *p->imin;
      MYFLT fnum = abs(iexp);
      if ((ftp = csound->FTFind(csound, &fnum)) != NULL) {
        p->table = ftp->ftable;
        p->tablen = ftp->flen;
      }
      else return OK;
      o->range(0,.99999999);
      if (iexp > 0) //interpolated
        o->callback((Fl_Callback*)fl_callbackInterpTableKnob,(void *) p);
      else // non-interpolated
        o->callback((Fl_Callback*)fl_callbackTableKnob,(void *) p);
    }
  }
  AddrSetValue.push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                        (void *) o, (void *) p));
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_text(CSOUND *csound, FLTEXT *p)
{
  char *controlName = GetString(csound, p->name, p->XSTRCODE);
  int ix,iy,iwidth,iheight,itype;
  MYFLT   istep;

  if (*p->iy < 0) iy = FL_iy;
  else  FL_iy = iy = (int) *p->iy;
  if (*p->ix < 0)  ix = FL_ix;
  else  FL_ix = ix = (int) *p->ix;
  if (*p->iwidth < 0) iwidth = FLcontrol_iwidth;
  else FLcontrol_iwidth = iwidth = (int) *p->iwidth;
  if (*p->iheight < 0) iheight = FLcontrol_iheight;
  else FLcontrol_iheight = iheight = (int) *p->iheight;
  if (*p->itype < 1) itype = 1;
  else  itype = (int) *p->itype;
  if (*p->istep < 0) istep = FL(.1);
  else  istep = *p->istep;

  Fl_Valuator* o;
  switch(itype) {
  case 1:
    {
      o = new Fl_Value_Input(ix, iy, iwidth, iheight, controlName);
      ((Fl_Value_Output *) o)->step(istep);
      ((Fl_Value_Output *) o)->range(*p->imin,*p->imax);
    }
    break;
  case 2:
    {
      o = new Fl_Value_Input_Spin(ix, iy, iwidth, iheight, controlName);
      ((Fl_Value_Input *) o)->step(istep);
      ((Fl_Value_Input *) o)->range(*p->imin,*p->imax);
    }
    break;
  case 3:
    {
      o = new Fl_Value_Output(ix, iy, iwidth, iheight, controlName);
      ((Fl_Value_Output *) o)->step(istep);
      ((Fl_Value_Output *) o)->range(*p->imin,*p->imax);
    }
    break;
  default:
    return NOTOK;
  }
  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
  widget_attributes(csound, o);
  o->callback((Fl_Callback*)fl_callbackLinearValueInput,(void *) p);
  AddrSetValue.push_back(ADDR_SET_VALUE(1, *p->imin, *p->imax,
                                        (void *) o, (void *) p));
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_button(CSOUND *csound, FLBUTTON *p)
{
  char *Name = GetString(csound, p->name, p->XSTRCODE);
  int type = (int) *p->itype;
  if (type >9 ) { // ignored when getting snapshots
    if (csound->oparms->msglevel & WARNMSG)
      csound->Warning(csound,
                      "FLbutton \"%s\" ignoring snapshot capture retrieve",
                      Name);
    type = type-10;
  }
  Fl_Button *w;
  *p->kout = *p->ioff;        // IV - Aug 27 2002

  switch (type) {
  case 1:
    w = new Fl_Button((int)*p->ix, (int)*p->iy,
                      (int)*p->iwidth, (int)*p->iheight, Name);
    break;
  case 2:
    w = new Fl_Light_Button((int)*p->ix, (int)*p->iy,
                            (int)*p->iwidth, (int)*p->iheight, Name);
    break;
  case 3:
    w = new Fl_Check_Button((int)*p->ix, (int)*p->iy,
                            (int)*p->iwidth, (int)*p->iheight, Name);
    break;
  case 4:
    w = new Fl_Round_Button((int)*p->ix, (int)*p->iy,
                            (int)*p->iwidth, (int)*p->iheight, Name);
    break;
  default:
    return csound->InitError(csound, "FLbutton: invalid button type");
  }
  Fl_Button *o = w;
  o->align(FL_ALIGN_WRAP);
  widget_attributes(csound, o);
  o->callback((Fl_Callback*)fl_callbackButton,(void *) p);
  AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p));
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_button_bank(CSOUND *csound, FLBUTTONBANK *p)
{
  char *Name = "/0";
  int type = (int) *p->itype;
  if (type > 9) {       // ignored when getting snapshots
    csound->Warning(csound,
                    "FLbutton \"%s\" ignoring snapshot capture retrieve",
                    Name);
    type = type - 10;
  }
  Fl_Group* o = new Fl_Group((int)*p->ix, (int)*p->iy,
                             (int)*p->inumx * 10, (int)*p->inumy*10);
  int z = 0;
  for (int j = 0; j < *p->inumx; j++) {
    for (int k = 0; k < *p->inumy; k++) {
      int       x = (int) *p->ix + j*10, y = (int) *p->iy + k*10;
      Fl_Button *w;
      char      *btName = new char[30];
      allocatedStrings.push_back(btName);
      sprintf(btName, "%d", z);
      switch (type) {
      case 1: w = new Fl_Button(x, y, 10, 10, btName); break;
      case 2: w = new Fl_Light_Button(x, y, 10, 10, btName); break;
      case 3: w = new Fl_Check_Button(x, y, 10, 10, btName); break;
      case 4: w = new Fl_Round_Button(x, y, 10, 10, btName); break;
      default: return csound->InitError(csound, "FLbuttonBank: "
                                                "invalid button type");
      }
      widget_attributes(csound, w);
      w->type(FL_RADIO_BUTTON);
      w->callback((Fl_Callback*) fl_callbackButtonBank, (void *) p);
      if (!z)
        w->value(1);
      z++;
    }
  }
  o->resizable(o);
  //*p->ix, *p->iy,
  o->size( (int)*p->iwidth, (int)*p->iheight);
  o->position((int)*p->ix, (int)*p->iy);
  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
  o->end();

  AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p));
  *p->kout = FL(0.0);
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_counter(CSOUND *csound, FLCOUNTER *p)
{
  char *controlName = GetString(csound, p->name, p->XSTRCODE);
  //      int ix,iy,iwidth,iheight,itype;
  //      MYFLT   istep1, istep2;

  Fl_Counter* o = new Fl_Counter((int)*p->ix, (int)*p->iy,
                                 (int)*p->iwidth, (int)*p->iheight,
                                 controlName);
  widget_attributes(csound, o);
  int type = (int) *p->itype;
  if (type >9 ) { // ignored when getting snapshots
    if (csound->oparms->msglevel & WARNMSG)
      csound->Warning(csound,
                      "FLcount \"%s\" ignoring snapshot capture retrieve",
                      controlName);
    type = type-10;
  }
  switch(type) {
  case 1: o->type(FL_NORMAL_COUNTER);  break;
  case 2: o->type(FL_SIMPLE_COUNTER);  break;
  default:o->type(FL_NORMAL_COUNTER);  break;
  }

  o->step(*p->istep1);
  o->lstep(*p->istep2);
  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
  // range is accepted only if min and max are different
  if (*p->imin != *p->imax)
    o->range(*p->imin,*p->imax); //otherwise no-range
  widget_attributes(csound, o);
  o->callback((Fl_Callback*)fl_callbackCounter,(void *) p);
  AddrSetValue.push_back(ADDR_SET_VALUE(1, 0, 100000, (void *) o, (void *) p));
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int fl_roller(CSOUND *csound, FLROLLER *p)
{
  char *controlName = GetString(csound, p->name, p->XSTRCODE);
  int ix,iy,iwidth, iheight,itype, iexp ;
  double istep;
  if (*p->iy < 0) {
    iy = FL_iy;
    FL_iy += FLroller_iheight + 15;
  }
  else {
    iy = (int) *p->iy;
    FL_iy = iy + FLroller_iheight + 15;
  }
  // omitted options: set defaults
  if (*p->ix<0) ix = FL_ix;       else  FL_ix = ix = (int) *p->ix;
  if (*p->iy<0) iy = FL_iy;       else  FL_iy = iy = (int) *p->iy;
  if (*p->iwidth<0) iwidth = FLroller_iwidth;
  else FLroller_iwidth = iwidth = (int) *p->iwidth;
  if (*p->iheight<0) iheight = FLroller_iheight;
  else FLroller_iheight = iheight = (int) *p->iheight;
  if (*p->itype<1) itype = 1;
  else  itype = (int) *p->itype;
  //if (*p->iexp<LIN_) iexp = LIN_;
  //else  iexp = (int) *p->iexp;
  switch((int) *p->iexp) {
  case -1: iexp = EXP_; break;
  case 0: iexp = LIN_; break;
  default: iexp = (int) *p->iexp;
  }

  if (*p->istep<0) istep = 1;     else  istep =  *p->istep;
  p->min = *p->imin;
  Fl_Roller *o;
  switch (itype) {
  case 1:
    o = new Fl_Roller(ix, iy, iwidth, iheight, controlName);
    o->type(FL_HORIZONTAL);
    break;
  case 2:
    o = new Fl_Roller(ix, iy, iwidth, iheight, controlName);
    o->type(FL_VERTICAL);
    break;
  default:
    return csound->InitError(csound, "FLroller: invalid roller type");
  }
  widget_attributes(csound, o);
  o->step(istep);
  switch (iexp) {
  case LIN_: //linear
    o->range(*p->imin,*p->imax);
    o->callback((Fl_Callback*)fl_callbackLinearRoller,(void *) p);
    break;
  case EXP_ : //exponential
    {
      MYFLT min = p->min, max = *p->imax;
      if (min == 0 || max == 0)
        return csound->InitError(csound, "FLslider: zero is illegal "
                                         "in exponential operations");
      MYFLT range = max - min;
      o->range(0,range);
      p->base = ::pow((max / min), 1/range);
      o->callback((Fl_Callback*)fl_callbackExponentialRoller,(void *) p);
    }
    break;
  default:
    {
      FUNC *ftp;
      MYFLT fnum = abs(iexp);
      if ((ftp = csound->FTFind(csound, &fnum)) != NULL) {
        p->table = ftp->ftable;
        p->tablen = ftp->flen;
      }
      else return NOTOK;
      o->range(0,0.99999999);
      if (iexp > 0) //interpolated
        o->callback((Fl_Callback*)fl_callbackInterpTableRoller,(void *) p);
      else // non-interpolated
        o->callback((Fl_Callback*)fl_callbackTableRoller,(void *) p);
    }
  }
  AddrSetValue.push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                        (void *) o, (void *) p));
  *p->ihandle = AddrSetValue.size()-1;
  return OK;
}

static int FLprintkset(CSOUND *csound, FLPRINTK *p)
{
  if (*p->ptime < FL(1.0) / csound->global_ekr)
    p->ctime = FL(1.0) / csound->global_ekr;
  else        p->ctime = *p->ptime;

  p->initime = (MYFLT) csound->kcounter * csound->onedkr;
  p->cysofar = -1;
  return OK;
}

static int FLprintk(CSOUND *csound, FLPRINTK *p)
{
  MYFLT   timel;
  long    cycles;

  timel = ((MYFLT) csound->kcounter * csound->onedkr) - p->initime;
  cycles = (long)(timel / p->ctime);
  if (p->cysofar < cycles) {
    p->cysofar = cycles;
    char valString[MAXNAME];
    sprintf(valString,"%.5g", *p->val);
    ((Fl_Output*) (AddrSetValue[(long) *p->idisp]).WidgAddress)->
      value(valString );
  }
  return OK;
}

static int FLprintk2set(CSOUND *csound, FLPRINTK2 *p)   // IV - Aug 27 2002
{
  p->oldvalue = FL(-1.12123e35);        // hack to force printing first value
  return OK;
}

static int FLprintk2(CSOUND *csound, FLPRINTK2 *p)
{
  MYFLT   value = *p->val;
  if (p->oldvalue != value) {
    char valString[MAXNAME];
    sprintf(valString,"%.5g", *p->val);
    ((Fl_Output*) (AddrSetValue[(long) *p->idisp]).WidgAddress)->
      value(valString );
    p->oldvalue = value;
  }
  return OK;
}

#define S(x)    sizeof(x)

const OENTRY widgetOpcodes_[] = {
    { "FLslider",       S(FLSLIDER),            1,  "ki",   "Tiijjjjjjj",
        (SUBR) fl_slider,               (SUBR) NULL,              (SUBR) NULL },
    { "FLslidBnk",      S(FLSLIDERBANK),        1,  "",     "Tiooooooooo",
        (SUBR) fl_slider_bank,          (SUBR) NULL,              (SUBR) NULL },
    { "FLknob",         S(FLKNOB),              1,  "ki",   "Tiijjjjjjo",
        (SUBR) fl_knob,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLroller",       S(FLROLLER),            1,  "ki",   "Tiijjjjjjjj",
        (SUBR) fl_roller,               (SUBR) NULL,              (SUBR) NULL },
    { "FLtext",         S(FLTEXT),              1,  "ki",   "Tiijjjjjj",
        (SUBR) fl_text,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLjoy",          S(FLJOYSTICK),          1,  "kkii", "Tiiiijjjjjjjj",
        (SUBR) fl_joystick,             (SUBR) NULL,              (SUBR) NULL },
    { "FLcount",        S(FLCOUNTER),           1,  "ki",   "Tiiiiiiiiiz",
        (SUBR) fl_counter,              (SUBR) NULL,              (SUBR) NULL },
    { "FLbutton",       S(FLBUTTON),            1,  "ki",   "Tiiiiiiiz",
        (SUBR) fl_button,               (SUBR) NULL,              (SUBR) NULL },
    { "FLbutBank",      S(FLBUTTONBANK),        1,  "ki",   "iiiiiiiz",
        (SUBR) fl_button_bank,          (SUBR) NULL,              (SUBR) NULL },
    { "FLkeyb",         S(FLKEYB),              1,  "k",    "z",
        (SUBR) FLkeyb,                  (SUBR) NULL,              (SUBR) NULL },
    { "FLcolor",        S(FLWIDGCOL),           1,  "",     "jjjjjj",
        (SUBR) fl_widget_color,         (SUBR) NULL,              (SUBR) NULL },
    { "FLcolor2",       S(FLWIDGCOL2),          1,  "",     "jjj",
        (SUBR) fl_widget_color2,        (SUBR) NULL,              (SUBR) NULL },
    { "FLlabel",        S(FLWIDGLABEL),         1,  "",     "ojojjj",
        (SUBR) fl_widget_label,         (SUBR) NULL,              (SUBR) NULL },
    { "FLsetVal_i", S(FL_SET_WIDGET_VALUE_I),   1,  "",     "ii",
        (SUBR) fl_setWidgetValuei,      (SUBR) NULL,              (SUBR) NULL },
    { "FLsetVali",  S(FL_SET_WIDGET_VALUE_I),   1,  "",     "ii",
        (SUBR) fl_setWidgetValuei,      (SUBR) NULL,              (SUBR) NULL },
    { "FLsetVal",       S(FL_SET_WIDGET_VALUE), 3,  "",     "kki",
        (SUBR) fl_setWidgetValue_set,   (SUBR) fl_setWidgetValue, (SUBR) NULL },
    { "FLsetColor",     S(FL_SET_COLOR),        1,  "",     "iiii",
        (SUBR) fl_setColor1,            (SUBR) NULL,              (SUBR) NULL },
    { "FLsetColor2",    S(FL_SET_COLOR),        1,  "",     "iiii",
        (SUBR) fl_setColor2,            (SUBR) NULL,              (SUBR) NULL },
    { "FLsetTextSize",  S(FL_SET_TEXTSIZE),     1,  "",     "ii",
        (SUBR) fl_setTextSize,          (SUBR) NULL,              (SUBR) NULL },
    { "FLsetTextColor", S(FL_SET_COLOR),        1,  "",     "iiii",
        (SUBR) fl_setTextColor,         (SUBR) NULL,              (SUBR) NULL },
    { "FLsetFont",      S(FL_SET_FONT),         1,  "",     "ii",
        (SUBR) fl_setFont,              (SUBR) NULL,              (SUBR) NULL },
    { "FLsetTextType",  S(FL_SET_FONT),         1,  "",     "ii",
        (SUBR) fl_setTextType,          (SUBR) NULL,              (SUBR) NULL },
    { "FLsetText",      S(FL_SET_TEXT),         1,  "",     "Ti",
        (SUBR) fl_setText,              (SUBR) NULL,              (SUBR) NULL },
    { "FLsetSize",      S(FL_SET_SIZE),         1,  "",     "iii",
        (SUBR) fl_setSize,              (SUBR) NULL,              (SUBR) NULL },
    { "FLsetPosition",  S(FL_SET_POSITION),     1,  "",     "iii",
        (SUBR) fl_setPosition,          (SUBR) NULL,              (SUBR) NULL },
    { "FLhide",         S(FL_WIDHIDE),          1,  "",     "i",
        (SUBR) fl_hide,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLshow",         S(FL_WIDSHOW),          1,  "",     "i",
        (SUBR) fl_show,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLsetBox",       S(FL_SETBOX),           1,  "",     "ii",
        (SUBR) fl_setBox,               (SUBR) NULL,              (SUBR) NULL },
    { "FLsetAlign",     S(FL_TALIGN),           1,  "",     "ii",
        (SUBR) fl_align,                (SUBR) NULL,              (SUBR) NULL },
    { "FLbox",          S(FL_BOX),              1,  "i",    "Tiiiiiii",
        (SUBR) fl_box,                  (SUBR) NULL,              (SUBR) NULL },
    { "FLvalue",        S(FLVALUE),             1,  "i",    "Tjjjj",
        (SUBR) fl_value,                (SUBR) NULL,              (SUBR) NULL },
    { "FLpanel",        S(FLPANEL),             1,  "",     "Tjjooo",
        (SUBR) StartPanel,              (SUBR) NULL,              (SUBR) NULL },
    { "FLpanelEnd",     S(FLPANELEND),          1,  "",     "",
        (SUBR) EndPanel,                (SUBR) NULL,              (SUBR) NULL },
    { "FLpanel_end",    S(FLPANELEND),          1,  "",     "",
        (SUBR) EndPanel,                (SUBR) NULL,              (SUBR) NULL },
    { "FLscroll",       S(FLSCROLL),            1,  "",     "iiii",
        (SUBR) StartScroll,             (SUBR) NULL,              (SUBR) NULL },
    { "FLscrollEnd",    S(FLSCROLLEND),         1,  "",     "",
        (SUBR) EndScroll,               (SUBR) NULL,              (SUBR) NULL },
    { "FLscroll_end",   S(FLSCROLLEND),         1,  "",     "",
        (SUBR) EndScroll,               (SUBR) NULL,              (SUBR) NULL },
    { "FLpack",         S(FLPACK),              1,  "",     "iiii",
        (SUBR) StartPack,               (SUBR) NULL,              (SUBR) NULL },
    { "FLpackEnd",      S(FLPACKEND),           1,  "",     "",
        (SUBR) EndPack,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLpack_end",     S(FLPACKEND),           1,  "",     "",
        (SUBR) EndPack,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLtabs",         S(FLTABS),              1,  "",     "iiii",
        (SUBR) StartTabs,               (SUBR) NULL,              (SUBR) NULL },
    { "FLtabsEnd",      S(FLTABSEND),           1,  "",     "",
        (SUBR) EndTabs,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLtabs_end",     S(FLTABSEND),           1,  "",     "",
        (SUBR) EndTabs,                 (SUBR) NULL,              (SUBR) NULL },
    { "FLgroup",        S(FLGROUP),             1,  "",     "Tiiiij",
        (SUBR) StartGroup,              (SUBR) NULL,              (SUBR) NULL },
    { "FLgroupEnd",     S(FLGROUPEND),          1,  "",     "",
        (SUBR) EndGroup,                (SUBR) NULL,              (SUBR) NULL },
    { "FLgroup_end",    S(FLGROUPEND),          1,  "",     "",
        (SUBR) EndGroup,                (SUBR) NULL,              (SUBR) NULL },
    { "FLsetsnap",      S(FLSETSNAP),           1,  "ii",   "io",
        (SUBR) set_snap,                (SUBR) NULL,              (SUBR) NULL },
    { "FLgetsnap",      S(FLGETSNAP),           1,  "i",    "i",
        (SUBR) get_snap,                (SUBR) NULL,              (SUBR) NULL },
    { "FLsavesnap",     S(FLSAVESNAPS),         1,  "",     "T",
        (SUBR) save_snap,               (SUBR) NULL,              (SUBR) NULL },
    { "FLloadsnap",     S(FLLOADSNAPS),         1,  "",     "T",
        (SUBR) load_snap,               (SUBR) NULL,              (SUBR) NULL },
    { "FLrun",          S(FLRUN),               1,  "",     "",
        (SUBR) FL_run,                  (SUBR) NULL,              (SUBR) NULL },
    { "FLupdate",       S(FLRUN),               1,  "",     "",
        (SUBR) fl_update,               (SUBR) NULL,              (SUBR) NULL },
    { "FLprintk",       S(FLPRINTK),            3,  "",     "iki",
        (SUBR) FLprintkset,             (SUBR) FLprintk,          (SUBR) NULL },
    { "FLprintk2",      S(FLPRINTK2),           3,  "",     "ki",
        (SUBR) FLprintk2set,            (SUBR) FLprintk2,         (SUBR) NULL },
    { NULL,             0,                      0,  NULL,   NULL,
        (SUBR) NULL,                    (SUBR) NULL,              (SUBR) NULL }
};

}       // extern "C"

