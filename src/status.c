/*
*      status.c
*      Copyright © 2008, 2009 Martin Duquesnoy <xorg62@gmail.com>
*      All rights reserved.
*
*      Redistribution and use in source and binary forms, with or without
*      modification, are permitted provided that the following conditions are
*      met:
*
*      * Redistributions of source code must retain the above copyright
*        notice, this list of conditions and the following disclaimer.
*      * Redistributions in binary form must reproduce the above
*        copyright notice, this list of conditions and the following disclaimer
*        in the documentation and/or other materials provided with the
*        distribution.
*      * Neither the name of the  nor the names of its
*        contributors may be used to endorse or promote products derived from
*        this software without specific prior written permission.
*
*      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "wmfs.h"

/* Systray width */
int sw = 0;

/** Check rectangles blocks in str and draw it
  * --> \b[x;y;width;height;#color]\
  *\param ib Infobar pointer
  *\param str String
  */
static void
statustext_rectangle(InfoBar *ib, char *str)
{
     StatusRec r;
     char as;
     int i, j, k;

     for(i = j = 0; i < (int)strlen(str); ++i, ++j)
          if(sscanf(&str[i], "\\b[%d;%d;%d;%d;#%x]%c", &r.x, &r.y, &r.w, &r.h, &r.color, &as) == 6
                    && as == '\\')
          {
               draw_rectangle(ib->bar->dr, r.x - sw, r.y, r.w, r.h, r.color);

               for(++i, --j; str[i] != as || str[i - 1] != ']'; ++i);
          }
          else if(j != i)
               str[j] = str[i];

     for(k = j; k < i; str[k++] = '\0');

     return;
}

/** Check graphs blocks in str and draw it
  * --> \g[x;y;width;height;#color;data]\
  *\param ib Infobar pointer
  *\param str String
  */
static void
statustext_graph(InfoBar *ib, char *str)
{
     StatusGraph g;
     char as, c, *p;
     int i, j, k, m, w;

     for(i = j = 0; i < (int)strlen(str); ++i, ++j)
          if(sscanf(&str[i], "\\g[%d;%d;%d;%d;#%x;%512[^]]]%c",
                    &g.x, &g.y, &g.w, &g.h, &g.color, g.data, &as) == 7
                    && as == '\\')
          {
               /* data is a list of numbers separated by ';' */
               w = g.w;
               p = strtok(g.data, ";");
               m = 0;

               for(c = atoi(p); p && m < w; ++m)
               {
                    /* height limits */
                    if(c < 0)
                         c = 0;
                    if(c > (char)g.h)
                         c = g.h;
                    g.data[m] = c;
                    p = strtok(NULL, ";");
               }

               /* width limits */
               for(; m < w; g.data[m++] = 0);

               /* data is a array[w] of bytes now */
               draw_graph(ib->bar->dr, g.x - sw, g.y, g.w, g.h, g.color, g.data);

               for(++i, --j; str[i] != as || str[i - 1] != ']'; ++i);
          }
          else if(j != i)
               str[j] = str[i];

     for(k = j; k < i; str[k++] = 0);

     return;
}

/** Check text blocks in str and draw it
  * --> \s[x;y;#color;text]\
  *\param ib Infobar pointer
  *\param str String
  */
static void
statustext_text(InfoBar *ib, char *str)
{
     StatusText s;
     char as;
     int i, j, k;

     for(i = j = 0; i < (int)strlen(str); ++i, ++j)
          if(sscanf(&str[i], "\\s[%d;%d;%7[^;];%512[^]]]%c", &s.x, &s.y, s.color, s.text, &as) == 5
                    && as == '\\')
          {
               draw_text(ib->bar->dr, s.x - sw, s.y, s.color, s.text);

               for(++i, --j; str[i] != as || str[i - 1] != ']'; ++i);
          }
          else if(j != i)
               str[j] = str[i];

     for(k = j; k < i; str[k++] = 0);

     return;
}

/** Draw normal text and colored normal text
  * --> \#color\ text in color
  *\param sc Screen
  *\param str String
  */
static void
statustext_normal(int sc, char *str)
{
     char strwc[MAXSTATUS] = { 0 };
     char buf[MAXSTATUS] = { 0 };
     char col[8] = { 0 };
     int n, i, j, k;

     for(i = j = n = 0; i < (int)strlen(str); ++i, ++j)
          if(str[i] == '\\' && str[i + 1] == '#' && str[i + 8] == '\\')
          {
               ++n;
               i += 8;
               --j;
          }
          else
               strwc[j] = str[i];

     /* Draw normal text without any blocks */
     draw_text(infobar[sc].bar->dr, (sgeo[sc].width - SHADH) - (textw(strwc) + sw),
               FHINFOBAR, infobar[sc].bar->fg, strwc);

     if(n)
     {
          strncpy(buf, strwc, sizeof(buf));

          for(i = k = 0; i < (int)strlen(str); ++i, ++k)
               if(str[i] == '\\' && str[i + 1] == '#' && str[i + 8] == '\\')
               {
                    /* Store current color in col[] */
                    for(j = 0, ++i; str[i] != '\\'; col[j++] = str[i++]);

                    /* Draw a rectangle with the bar color to draw the text properly */
                    draw_rectangle(infobar[sc].bar->dr, (sgeo[sc].width - SHADH) - (textw(&buf[k]) + sw),
                                   0, INFOBARH - (sgeo[sc].width - SHADH) - textw(&buf[k]),
                                   INFOBARH, conf.colors.bar);

                    /* Draw text with its color */
                    draw_text(infobar[sc].bar->dr, (sgeo[sc].width - SHADH) - (textw(&buf[k]) + sw),
                              FHINFOBAR,  col, &buf[k]);

                    strncpy(buf, strwc, sizeof(buf));
                    ++i;
               }
     }

     return;
}

/** Handle statustext and draw all things in infobar of specified screen
  *\param sc Screen number
  *\param str String
  */
void
statustext_handle(int sc, char *str)
{
     char *lastst;
     int i;

     /* If the str == the current statustext, return (not needed) */
     if(!str)
          return;

     if(sc == conf.systray.screen)
          sw = systray_get_width();

     barwin_refresh_color(infobar[sc].bar);

     /* save last status text address (for free at the end) */
     lastst = infobar[sc].statustext;

     infobar[sc].statustext = xstrdup(str);

     /* Store rectangles, located text & images properties. */
     statustext_rectangle(&infobar[sc], str);
     statustext_graph(&infobar[sc], str);
     statustext_text(&infobar[sc], str);

     /* Draw normal text (and possibly colored with \#color\ blocks) */
     statustext_normal(sc, str);

     barwin_refresh(infobar[sc].bar);

     free(lastst);

     return;
}
