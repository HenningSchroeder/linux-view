/*************************************************************************
** interface.c for USBView - a USB device viewer
** Copyright (c) 1999, 2000, 2009 by Greg Kroah-Hartman, <greg@kroah.com>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; version 2 of the License.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
** (See the included file COPYING)
*************************************************************************/


#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "config.h"
#include "vikram.xpm"
#include "rambo.xpm"

GtkWidget *treeUSB;
GtkTreeStore *treeStore;
GtkTextBuffer *textDescriptionBuffer;
GtkWidget *textDescriptionView;
GtkWidget *windowMain;

int timer;
int currentbox = 0;

#define VERSION "0.3"
#define MAX_LINE_SIZE 1000

gchar devicesFile[1000];
const char *verifyMessage =     " Verify that you are running on linux\n";

static void FileError (void)
{
        GtkWidget *dialog;

        dialog = gtk_message_dialog_new (
                                    GTK_WINDOW (windowMain),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "Can not open the file %s\n\n%s",
                                    devicesFile, verifyMessage);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}

static void Init (void)
{
        GtkTextIter begin;
        GtkTextIter end;

	strcpy (devicesFile, "/proc/cpuinfo");

        /* blow away the tree if there is one */
//        if (rootDevice != NULL) {
  //              gtk_tree_store_clear (treeStore);
    //    }

        /* clean out the text box */
        gtk_text_buffer_get_start_iter(textDescriptionBuffer,&begin);
        gtk_text_buffer_get_end_iter(textDescriptionBuffer,&end);
        gtk_text_buffer_delete (textDescriptionBuffer, &begin, &end);

        return;
}

static void PopulateListBox (int deviceId, int refresh);

gboolean RefreshFunction(gpointer data)
{
	static int count;

	if (currentbox == 5) {
		//g_print("timer expire %d\n", count++);
		PopulateListBox(currentbox, 0);
		return TRUE;
	}

	count = 0;
	return FALSE;
}
static void PopulateListBox (int deviceId, int refresh)
{
        char    *string;
        char    *tempString;
        GtkTextIter begin;
        GtkTextIter end;
	char            *dataLine;
	char            *processor;
	FILE *fp;
	int first;

	currentbox = deviceId;
	        /* clear the textbox */
        gtk_text_buffer_get_start_iter(textDescriptionBuffer,&begin);
        gtk_text_buffer_get_end_iter(textDescriptionBuffer,&end);
        gtk_text_buffer_delete (textDescriptionBuffer, &begin, &end);

        /* freeze the display */
        /* this keeps the annoying scroll from happening */
        gtk_widget_freeze_child_notify(textDescriptionView);

        string = (char *)g_malloc (1000);

        /* add the name to the textbox if we have one*/
        if (deviceId == 0) {
                gtk_text_buffer_insert_at_cursor(textDescriptionBuffer,
				"Display Each Cpu property",
				strlen("Display Each Cpu property"));
		goto done;
        }


	//display /proc/cpuinfo
	fp = fopen("/proc/cpuinfo", "r");
	dataLine = (char *)g_malloc (MAX_LINE_SIZE);
	processor = (char *)g_malloc (MAX_LINE_SIZE);
	sprintf(processor, "processor\t: %d", deviceId - 1);

	first = 1;
	while (!feof(fp)) {
		fgets (dataLine, MAX_LINE_SIZE-1, fp);
		//if (dataLine[0] == 'p' && dataLine[1] == 'r') {
		//	g_print("LINE:%s\n", dataLine);
		//}

		if (first && (!strncmp(processor, dataLine, strlen(processor)))) {
			first = 0;
		} else if (first)
			continue;

		if (!strncmp("flags", dataLine, strlen("flags")))
			continue;

		if (!first)
			gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, dataLine,strlen(dataLine)); 

		if (!strncmp("power management:", dataLine, strlen("power management:"))) {
			break;
		}
	}
	fclose(fp);
	g_free(dataLine);
	g_free(processor);

	if (deviceId == 5) {
		//display /proc/meminfo lines
		fp = fopen("/proc/meminfo", "r");
		dataLine = (char *)g_malloc (MAX_LINE_SIZE);
		while (!feof(fp)) {
			fgets (dataLine, MAX_LINE_SIZE-1, fp);
			gtk_text_buffer_insert_at_cursor(textDescriptionBuffer, dataLine,strlen(dataLine)); 
		}
		fclose(fp);
		g_free(dataLine);
		if (refresh)
			timer = g_timeout_add_seconds(1, RefreshFunction, 0);
	}

done:
       /* thaw the display */
        gtk_widget_thaw_child_notify(textDescriptionView);

        /* clean up our string */
        g_free (string);

}

void SelectItem (GtkTreeSelection *selection, gpointer userData)
{
        GtkTreeIter iter;
        GtkTreeModel *model;
        gint deviceAddr;

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                gtk_tree_model_get (model, &iter,
                                DEVICE_ADDR_COLUMN, &deviceAddr,
                                -1);
                PopulateListBox (deviceAddr, 1);
		//g_print("selected: %d\n", deviceAddr);
        }
}

void parse_line (char * line)
{
	static GtkTreeIter     parent;
	GtkTreeIter     leaf;
	static int first = 1;
	static int counter;

	if (first) {
		gtk_tree_store_append (treeStore, &parent, NULL);

		gtk_tree_store_set (treeStore, &parent,
                            NAME_COLUMN, "CPU",
                            DEVICE_ADDR_COLUMN, counter++,
                            COLOR_COLUMN, "blue",
                            -1);
		first = 0;
	}

	if (!strncmp(line,"processor",9)) {
		gtk_tree_store_append (treeStore, &leaf, &parent);

		gtk_tree_store_set (treeStore, &leaf,
                            NAME_COLUMN, line,
                            DEVICE_ADDR_COLUMN, counter++,
                            COLOR_COLUMN, "blue",
                            -1);
	}

	if (!strncmp(line,"MemTotal:",9)) {
		gtk_tree_store_append (treeStore, &leaf, NULL);

		gtk_tree_store_set (treeStore, &leaf,
                            NAME_COLUMN, "MEM",
                            DEVICE_ADDR_COLUMN, counter++,
                            COLOR_COLUMN, "blue",
                            -1);
	}
}



void LoadTree( void )
{
        static gboolean signal_connected = FALSE;
        FILE            *cpuinfo_file;
        char            *dataLine;
        int             finished;
        int             i;
	static int first = 1;

	if (!first)
		return;

	Init();

again:
	cpuinfo_file = fopen(devicesFile, "r");
	if (cpuinfo_file == NULL ) {
		FileError();
		return;
	}

        dataLine = (char *)g_malloc (MAX_LINE_SIZE);
        finished = 0;
        while (!finished) {
                /* read the line in from the file */
                fgets (dataLine, MAX_LINE_SIZE-1, cpuinfo_file);

                if (dataLine[strlen(dataLine)-1] == '\n')
                        parse_line (dataLine);

                if (feof (cpuinfo_file))
                        finished = 1;
        }

        fclose (cpuinfo_file);
        g_free (dataLine);

	if (first) {
		strcpy(devicesFile, "/proc/meminfo");
		first = 0;
		goto again;
	}


       gtk_widget_show (treeUSB);
        gtk_tree_view_expand_all (GTK_TREE_VIEW (treeUSB));

        /* hook up our callback function to this tree if we haven't yet */
	{
        GtkTreeSelection *select;
		select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeUSB));
		g_signal_connect (G_OBJECT (select), "changed",
                                  G_CALLBACK (SelectItem), NULL);
	}

	return;
}



void on_buttonAbout_clicked (GtkButton *button, gpointer user_data)
{
	GdkPixbuf *logo;
	gchar *authors[] = { "Vikram Pandita <vikrampandita@gmail.com>",
				"Based on work by Gregkh for usbtree", NULL };

	logo = gdk_pixbuf_new_from_xpm_data ((const char **)dummy);
	gtk_show_about_dialog (GTK_WINDOW (windowMain),
		"logo", logo,
		"program-name", "linux-view",
		"version", VERSION,
		"comments", "Display Linux Info",
		"website-label", "vikrampandita@gmail.com",
		"website", "www.linkedin.com/pub/vikram-pandita/8/237/239/",
		"copyright", "Copyright © 2013",
		"authors", authors,
		NULL);
}
void on_buttonClose_clicked (GtkButton *button, gpointer user_data)
{
        gtk_main_quit();
}

gboolean on_window1_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_main_quit();
	return FALSE;
}

void on_buttonRefresh_clicked (GtkButton *button, gpointer user_data)
{
	//system("echo ----------------");
	//system("ls");
	//system("echo ----------------");
	LoadTree();
}

GtkWidget*
create_windowMain ()
{
	GtkWidget *vbox1;
	GtkWidget *hpaned1;
	GtkWidget *scrolledwindow1;
	GtkWidget *hbuttonbox1;
	GtkWidget *buttonRefresh;
	GtkWidget *buttonConfigure;
	GtkWidget *buttonClose;
	GtkWidget *buttonAbout;
	GdkPixbuf *icon;
	GtkCellRenderer *treeRenderer;
	GtkTreeViewColumn *treeColumn;
	char version[500];

	windowMain = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (windowMain, "windowMain");
	sprintf(version,"Linux View v%s", VERSION);
	gtk_window_set_title (GTK_WINDOW (windowMain), version);
	gtk_window_set_default_size (GTK_WINDOW (windowMain), 600, 800);

	icon = gdk_pixbuf_new_from_xpm_data((const char **)rambo_xpm);
	gtk_window_set_icon(GTK_WINDOW(windowMain), icon);

	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name (vbox1, "vbox1");
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (windowMain), vbox1);

	hpaned1 = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_name (hpaned1, "hpaned1");
	gtk_widget_show (hpaned1);
	gtk_box_pack_start (GTK_BOX (vbox1), hpaned1, TRUE, TRUE, 0);

	treeStore = gtk_tree_store_new (N_COLUMNS,
				G_TYPE_STRING,	/* NAME_COLUMN */
				G_TYPE_INT,	/* DEVICE_ADDR_COLUMN */
				G_TYPE_STRING	/* COLOR_COLUMN */);
	treeUSB = gtk_tree_view_new_with_model (GTK_TREE_MODEL (treeStore));
	treeRenderer = gtk_cell_renderer_text_new ();
	treeColumn = gtk_tree_view_column_new_with_attributes (
					"CPU Devices",
					treeRenderer,
					"text", NAME_COLUMN,
					"foreground", COLOR_COLUMN,
					NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeUSB), treeColumn);
	gtk_widget_set_name (treeUSB, "treeUSB");
	gtk_widget_show (treeUSB);
	gtk_paned_pack1 (GTK_PANED (hpaned1), treeUSB, FALSE, FALSE);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_show (scrolledwindow1);
	gtk_paned_pack2 (GTK_PANED (hpaned1), scrolledwindow1, TRUE, FALSE);

	textDescriptionBuffer = gtk_text_buffer_new(NULL);
	textDescriptionView = gtk_text_view_new_with_buffer(textDescriptionBuffer);
	gtk_widget_set_name (textDescriptionView, "textDescription");
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textDescriptionView), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textDescriptionView), FALSE);
	gtk_widget_show (textDescriptionView);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), textDescriptionView);

	hbuttonbox1 = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
	gtk_widget_show (hbuttonbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, FALSE, 5);

	buttonRefresh = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_widget_set_name (buttonRefresh, "buttonRefresh");
	gtk_widget_show (buttonRefresh);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonRefresh);
	gtk_container_set_border_width (GTK_CONTAINER (buttonRefresh), 4);
	gtk_widget_set_can_default (buttonRefresh, TRUE);

	buttonConfigure = gtk_button_new_with_label ("Configure...");
	gtk_widget_set_name (buttonConfigure, "buttonConfigure");
	gtk_widget_show (buttonConfigure);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonConfigure);
	gtk_container_set_border_width (GTK_CONTAINER (buttonConfigure), 4);
	gtk_widget_set_can_default (buttonConfigure, TRUE);

	buttonAbout = gtk_button_new_from_stock(GTK_STOCK_ABOUT);
	gtk_widget_set_name (buttonAbout, "buttonAbout");
	gtk_widget_show (buttonAbout);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonAbout);
	gtk_container_set_border_width (GTK_CONTAINER (buttonAbout), 4);
	gtk_widget_set_can_default (buttonAbout, TRUE);

	buttonClose = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_widget_set_name (buttonClose, "buttonClose");
	gtk_widget_show (buttonClose);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonClose);
	gtk_container_set_border_width (GTK_CONTAINER (buttonClose), 4);
	gtk_widget_set_can_default (buttonClose, TRUE);

	g_signal_connect (G_OBJECT (windowMain), "delete_event",
			    G_CALLBACK (on_window1_delete_event),
			    NULL);
	g_signal_connect (G_OBJECT (buttonRefresh), "clicked",
			    G_CALLBACK (on_buttonRefresh_clicked),
			    NULL);
#if 0
	g_signal_connect (G_OBJECT (buttonConfigure), "clicked",
			    G_CALLBACK (on_buttonConfigure_clicked),
			    NULL);
#endif
	g_signal_connect (G_OBJECT (buttonAbout), "clicked",
			    G_CALLBACK (on_buttonAbout_clicked),
			    NULL);
	g_signal_connect (G_OBJECT (buttonClose), "clicked",
			    G_CALLBACK (on_buttonClose_clicked),
			    NULL);

	LoadTree();
	/* create our timer */
	//timer = gtk_timeout_add (2000, on_timer_timeout, 0);

	return windowMain;
}
