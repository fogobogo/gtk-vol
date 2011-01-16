/*
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following disclaimer
  in the documentation and/or other materials provided with the
  distribution.
* Neither the name of the  nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/* gcc gtk-vol.c -o gtk-vol $(pkg-config --cflags --libs gtk+-2.0) */

#include <stdio.h>
#include <string.h>             /* strlen */
#include <sys/stat.h>           /* open() */
#include <fcntl.h>
#include <stropts.h>            /* ioctl() */
#include <linux/soundcard.h>
#include <errno.h>              /* errno */
#include <gtk/gtk.h>
#include <glib/gprintf.h>


#define DEVICE   "/dev/mixer"
#define MASTER  0
#define PCM     1

#define CONTROL MASTER

/* some icon sets have a panel-* version too. e.g. "panel-audio-volume-muted" */
#define ICON_MUTE   "audio-volume-muted"
#define ICON_LOW    "audio-volume-low"
#define ICON_MEDIUM "audio-volume-medium"
#define ICON_HIGH   "audio-volume-high"

#define VOLUME_STEP    1

struct volume {
    guint8 l;
    guint8 r;
};

int fd;
struct volume vol;
struct volume store;


void 
tray_icon_set_from_vol(GtkStatusIcon *icon)
{
    if(vol.l == 0
    || vol.r == 0) {
        gtk_status_icon_set_from_icon_name(icon, ICON_MUTE); 
    }

    if((vol.l > 0 && vol.l <= 33)
    || (vol.r > 0 && vol.r <= 33)) {
        gtk_status_icon_set_from_icon_name(icon, ICON_LOW);
    }

    if((vol.l > 33 && vol.l <= 66)
    || (vol.r > 33 && vol.r <= 66)) {
        gtk_status_icon_set_from_icon_name(icon, ICON_MEDIUM); 
    }

    if((vol.l > 66 && vol.l <= 100)
    || (vol.r > 66 && vol.r <= 100)) {
        gtk_status_icon_set_from_icon_name(icon, ICON_HIGH); 
    }

}


void
tray_icon_set_tooltip(GtkStatusIcon *icon) {
    gchar tooltip[64];
    
    if(vol.r == vol.l) {
        g_sprintf(tooltip, "%i%%", vol.r);
    }

    if(vol.l > vol.r) {
        g_sprintf(tooltip, "%i%%", vol.l);
    }

    if(vol.r > vol.l) {
        g_sprintf(tooltip, "%i%%", vol.r);
    }

    gtk_status_icon_set_tooltip(icon, tooltip);
}


void 
tray_icon_on_scroll(GtkStatusIcon *icon, GdkEventScroll *event, gpointer user_data)
{
    if(event->direction == GDK_SCROLL_UP) {
        vol.l += VOLUME_STEP;
        vol.r += VOLUME_STEP;
    }

    if(event->direction == GDK_SCROLL_DOWN) {
        /* special case: if we try to go below zero the volume jumps to 100. so this is to protect ears */
        if(vol.l == 0) {
            vol.l = 0;
        }

        if(vol.r == 0) {
            vol.r = 0;
        }

        else {
            vol.l -= VOLUME_STEP;
            vol.r -= VOLUME_STEP;
        }
    }

    if(ioctl(fd, MIXER_WRITE(CONTROL), &vol) == -1) {
        fprintf(stderr, "error: %s\n", strerror(errno));
    }

    tray_icon_set_from_vol(icon);
    tray_icon_set_tooltip(icon);
}


void 
tray_icon_on_click(GtkStatusIcon *icon, GdkEventButton event, gpointer user_data)
{

    /* store volume and mute */
    if(vol.l != 0 && vol.r != 0) {
        store.l = vol.l;
        store.r = vol.r;
        vol.l = 0;
        vol.r = 0;
        gtk_status_icon_set_tooltip(icon, "Muted");
    }

    /* restore volume */
    else if(vol.l == 0 && vol.r == 0) {
        vol.l = store.l;
        vol.r = store.r;
    }

    tray_icon_set_from_vol(icon);

    if(ioctl(fd, MIXER_WRITE(CONTROL), &vol) == -1) {
        fprintf(stderr, "error: %s\n", strerror(errno));
    }
}


void
tray_icon_check_for_update(GtkStatusIcon **icon) 
{
    struct volume now;

    if(ioctl(fd, MIXER_READ(CONTROL), &now) == -1) {
        fprintf(stderr, "error: %s\n", strerror(errno));
    }

    if(now.l != vol.l || now.r != vol.r) {
        vol.l = now.l;
        vol.r = now.r;

        tray_icon_set_from_vol((*icon));
        tray_icon_set_tooltip((*icon));
    }
}


GtkStatusIcon* 
create_tray_icon() 
{
    GtkStatusIcon *tray_icon;

    tray_icon = gtk_status_icon_new();

    g_signal_connect(G_OBJECT(tray_icon), "activate", 
            G_CALLBACK(tray_icon_on_click), 
            NULL);
    g_signal_connect(G_OBJECT(tray_icon),"scroll-event",
            G_CALLBACK(tray_icon_on_scroll), NULL);

    tray_icon_set_from_vol(tray_icon);
    tray_icon_set_tooltip(tray_icon);

    gtk_status_icon_set_visible(tray_icon, TRUE);

    return tray_icon;
}


int main(int argc, char **argv) {

    GtkStatusIcon *tray_icon;

    fd = open(DEVICE, O_RDWR, 0);

    if(fd < 0) {
        fprintf(stderr, "error opening device file. %s\n",strerror(errno));
    }

    /* read out current volume */
    if(ioctl(fd, MIXER_READ(CONTROL), &vol) == -1) {
        fprintf(stderr, "error: %s\n", strerror(errno));
    }

    gtk_init(&argc, &argv);
    tray_icon = create_tray_icon(&vol);
    /* in case a keycombo or another application is used to change the volume:  
    *  read out volume every second and apply the correct tooltip/icon */
	g_timeout_add_seconds(1,(GSourceFunc)tray_icon_check_for_update, &tray_icon);
    gtk_main();

    close(fd);

    return 0;
}

