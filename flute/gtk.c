/* Hacked from FLUTE code by njl@ecs.soton.ac.uk */
/*
 *   MAD-FLUTE, implementation of FLUTE protocol
 *   Copyright (c) 2003-2004 TUT - Tampere University of Technology
 *   main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../alclib/inc.h"

#include <gtk/gtk.h>
#include <glib.h>

#include "flute.h"
#include "fdt.h"
#include "fdt_gen.h"
#include "uri.h"

/* Global variables */

struct {
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  arguments_t recv_arg;
  arguments_t send_arg;
  pthread_cond_t wakeup;
  int send_id, recv_id;
  file_info_t *queue;
  file_info_t *downloads;
  long long send_toi;
  GAsyncQueue *async;
} common;

typedef struct {
  enum { A_STATUS, A_NEW_FILE, A_PROGRESS, A_FILE_DONE } type;
  unsigned long long toi;
  char *data;
} amsg;

/**** Private functions ****/

void signal_handler(int sig);
void usage(void);
void validateargs(arguments_t *a, int mode);

enum {
  TARGET_URI_LIST = 1,
  TARGET_UTF8_STRING,
  TARGET_HTML,
  TARGET_DEFAULT
};

static GtkTargetEntry targets[] = {
  { target: "text/uri-list", flags: 0, info: TARGET_URI_LIST },
  { target: "UTF8_STRING", flags: 0, info: TARGET_UTF8_STRING },
  { target: "text/html", flags: 0, info: TARGET_HTML },
  { target: "application/octet-stream", flags: 0, info: TARGET_DEFAULT } };

#define TARGET_COUNT (sizeof(targets) / sizeof(GtkTargetEntry))

static void *flute_send_thread(void *parm);
static void *flute_recv_thread(void *parm);

static void status_message(char *message);
static void send_queue(void);

static GtkWidget *main_window, *send_vbox, *recv_vbox, *status_bar;
static GtkWidget *pref_window;
static GtkWidget *drop_msg_label;
static guint default_context;

void am_new_file(char *file, unsigned long long toi)
{
  amsg *message = malloc(sizeof(amsg));
  message->type = A_NEW_FILE;
  message->toi = toi;
  message->data = g_strdup(file);
  g_async_queue_push (common.async, message);
}

void am_file_done(char *file, unsigned long long toi)
{
  amsg *message = malloc(sizeof(amsg));
  message->type = A_FILE_DONE;
  message->toi = toi;
  message->data = g_strdup(file);
  g_async_queue_push (common.async, message);
}

void am_status(char *status)
{
  amsg *message = malloc(sizeof(amsg));
  message->type = A_STATUS;
  message->toi = 0;
  message->data = g_strdup(status);
  g_async_queue_push (common.async, message);
}

void am_progress(double percent, unsigned long long toi)
{
  char text[64];
  sprintf(text, "%.2f%%", percent);
  amsg *message = malloc(sizeof(amsg));
  message->type = A_PROGRESS;
  message->toi = toi;
  message->data = g_strdup(text);
  g_async_queue_push (common.async, message);
}

static void show_location(char *location)
{
#if defined(WIN32)
  ShellExecute(NULL, "Open", location, NULL, NULL, SW_SHOWNORMAL)
#elif defined(__APPLE__)
  char buffer[1024] = "open \""; /* FIXME unsafe */
  strncat(buffer, location, 1000);
  strcat(buffer, "\"");
  system(buffer);
#else
  char buffer[1024] = "nautilus \""; /* FIXME unsafe */
  strncat(buffer, location, 1000);
  strcat(buffer, "\"");
  system(buffer);
#endif
}

static gboolean on_file_send_unlink(GtkButton *button, gpointer userdata)
{
  file_info_t *info = (file_info_t *) userdata;
  GtkWidget *hbox = gtk_widget_get_parent(GTK_WIDGET(button));
  if (common.queue == info) {
    common.queue = info->next;
  }
  if (info->prev) {
    info->prev->next = info->next;
  }
  if (info->next) {
    info->next->prev = info->prev;
  }
  g_free(info->location);
  g_free(info);
  gtk_widget_destroy(hbox);
  send_queue();

  return FALSE;
}

static gboolean on_file_recv_clicked(GtkButton *button, gpointer userdata)
{
  file_info_t *info = (file_info_t *) userdata;

  show_location(info->location);
  return FALSE;
}

static void start_file(unsigned long long toi, char *path)
{
  GtkWidget *label, *hbox, *button;
  file_info_t *info;

  char *filename = strrchr(path, '/');

  for(info = common.downloads; info != NULL; info= info->next) {
    if (info->toi == toi) return; /* already seen this TOI */
  }

  if (filename && filename[1] != 0) {
    filename+= 1;
  } else {
    filename = path;
  }
  info = malloc(sizeof(file_info_t));
  info->location = g_strdup(path); /* FIXME memleak city here */
  info->toi = toi;
  info->data = label = gtk_label_new("0%");
  info->prev = NULL;
  info->next = common.downloads;

  hbox = gtk_hbox_new(FALSE, 0);
  button = gtk_button_new_with_label(filename);
  gtk_button_set_alignment(GTK_BUTTON(button), 0.0, 0.5);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  g_signal_connect ((gpointer) button, "clicked",
                    G_CALLBACK (on_file_recv_clicked),
                    info); /* FIXME leak */
  gtk_container_add(GTK_CONTAINER(hbox), button);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label), 5, 5);
  gtk_container_add(GTK_CONTAINER(hbox), label);
  gtk_box_pack_start(GTK_BOX(recv_vbox), hbox, FALSE, FALSE, 1);
  gtk_widget_show_all(hbox);

  if (common.downloads) {
    common.downloads->prev = info;
  }
  common.downloads = info;
}

static void finish_file(unsigned long long toi, char *path)
{
  file_info_t *info;

  start_file(toi, path); /* sometimes the file hasn't started yet */
  for(info = common.downloads; info != NULL; info= info->next) {
    if (info->toi == toi) {
      /* FIXME memleak */
      info->location = g_strdup(path);
      GtkWidget *label = (GtkWidget *) info->data;
      gtk_label_set_text(GTK_LABEL(label), "Finished");
    }
  }
}

static void update_file(unsigned long long toi, char *message)
{
  file_info_t *info;

  for(info = common.downloads; info != NULL; info= info->next) {
    if (info->toi == toi) {
      GtkWidget *label = (GtkWidget *) info->data;
      gtk_label_set_text(GTK_LABEL(label), message);
    }
  }
}

static gboolean run_queue(gpointer ignore)
{
  amsg *message;

  do {
    message = (amsg *) g_async_queue_try_pop (common.async);
    if (message) {
      switch (message->type) {
      case A_STATUS:
	      	status_message(message->data);
  	break;
  
      case A_NEW_FILE:
     		start_file(message->toi, message->data);
  	break;
      case A_PROGRESS:
	       	update_file(message->toi, message->data);
  	break;
      case A_FILE_DONE:
     		finish_file(message->toi, message->data);
  	break;
      }
	if (message->data)	g_free(message->data);
      free(message);
    }
  } while(message);

  return TRUE;
}

static void status_message(char *message)
{
  gtk_statusbar_pop(GTK_STATUSBAR(status_bar), default_context);
  gtk_statusbar_push(GTK_STATUSBAR(status_bar), default_context, message);
}

static void send_queue(void)
{
  char fdt_template[] = "/tmp/6share-XXXXXX";
  char *old_filename;

  mktemp(fdt_template);
  generate_fdt_from_list(common.queue, &common.send_id, fdt_template);
  old_filename = g_strdup(common.send_arg.fdt_file);
  strcpy(common.send_arg.fdt_file, fdt_template);
  unlink(old_filename);
  g_free(old_filename);

  pthread_mutex_lock(&(common.mutex));
  pthread_cond_signal(&(common.wakeup));
  pthread_mutex_unlock(&(common.mutex));
}

static void empty_queue(void)
{
  file_info_t *info = common.queue;

  common.queue= NULL;
  while (info) {
    file_info_t *next = info->next;
    /* FIXME free associated strings */
    free(info);
    info = next;
  }
//  set_session_state(common.send_id, SExiting); /* FIXME */
}

static void add_one_uri(char *uri)
{
  GtkWidget *label, *button, *hbox;
  file_info_t *info;
  char *filename;

  size_t len = strlen(uri);
  uri_t *parsed_uri = parse_uri(uri, len);

  filename = strrchr(parsed_uri->path, '/');
  if (filename && filename[1] != 0) {
    filename+= 1;
  } else {
    filename = parsed_uri->path;
  }
  if (strcmp(parsed_uri->scheme, "file")) return; /* otherwise hard */
#if 0
  if (drop_msg_label) {
    gtk_widget_destroy(drop_msg_label);
    drop_msg_label = NULL;
  }
#endif

  info = malloc(sizeof(file_info_t));
  info->location = g_strdup(parsed_uri->path); /* FIXME memleak */
  info->toi = common.send_toi++;
  info->prev = NULL;
  info->next = common.queue;
  if (common.queue) {
    common.queue->prev = info;
  }
  common.queue = info;

  hbox = gtk_hbox_new(FALSE, 0);
  button = gtk_button_new_with_label("x");
  g_signal_connect ((gpointer) button, "clicked",
                    G_CALLBACK (on_file_send_unlink),
                    info);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 1);
  label = gtk_label_new(filename);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label), 5, 5);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 1);
  gtk_box_pack_start(GTK_BOX(send_vbox), hbox, FALSE, FALSE, 1);
  gtk_widget_show_all(hbox);

  free_uri(parsed_uri);
}

static void add_uris(char *uri_list)
{
  char *uri, *tmp;

  tmp = strdup(uri_list);
  uri = strtok(uri_list, "\r\n");
  while (uri != NULL) {
    if (*uri) add_one_uri(uri);
    uri = strtok(NULL, "\r\n");
  }
  free(tmp);
  send_queue();
}

#define SHORT_NAME 28

static void add_file(char *data, long length, char *extn)
{
  char *write, filename[SHORT_NAME] = "";
  char uri[1024] = "file:///tmp/cutting";
  FILE *file;
  
  int plain = 1, k;
  write = filename;
  for (k = 0; k < length && k < 256; ++k) {
    if (plain && isalnum(data[k])) {
      *(write++) = data[k];
    } else if (data[k] == '<') {
      plain = 0;
    } else if (data[k] == '>') {
      plain = 1;
    } else if (plain && isspace(data[k])) {
      if (write != filename) /* not as 1st character */
      *(write++) = ' ';
    }
    if (write >= filename + SHORT_NAME) break;
  }
  *write = 0; /* terminate string */

  if (write > filename + 4) {
    strcpy(uri + 12, filename);
  }

  strcat(uri, extn);
  file = fopen(uri + 7 , "wb");
  fwrite(data, 1, length, file);
  fclose(file);
  add_one_uri(uri);
  send_queue();
}

static void
on_drag_data_received        (GtkWidget *widget, GdkDragContext  *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time,
                                        gpointer         user_data)
{

  switch (info) {
  case TARGET_URI_LIST:
    add_uris(data->data);
    break;
  case TARGET_HTML:
    add_file(data->data, data->length, ".html");
    break;
  case TARGET_UTF8_STRING:
    add_file(data->data, data->length, ".txt");
    break;
  default:
    printf("identified, but not yet handled data type\n");
    break;
  }
}

static void on_menu_pref_activate (GtkWidget *widget, gpointer user_data)
{
  gtk_widget_show_all(pref_window);
}

static void on_menu_about_activate (GtkWidget *widget, gpointer user_data)
{
#if GTK_CHECK_VERSION(2,6,0)
  char *artists[] = { "Nick Lamb <njl@ecs.soton.ac.uk>", "<jani.peltotalo@tut.fi>", "<sami.peltotalo@tut.fi>", 0 };
  gtk_show_about_dialog(GTK_WINDOW(main_window),
                        "authors", artists,
                        "copyright", "2004-5",
                        "comments", "6share is a work in progress");
#elif GTK_CHECK_VERSION(2,4,0)
 GtkWidget *dialog;

 dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(main_window),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  "<big><b>6share</b> Copyright 2005</big>\n"
                                  "6share is a work in progress\n"
                                  "<i>njl@ecs.soton.ac.uk "
                                  "jani.peltotalo@tut.fi "
                                  "sami.peltotalo@tut.fi</i>");
 gtk_dialog_run (GTK_DIALOG (dialog));
 gtk_widget_destroy (dialog);
#endif
}

static void on_menu_send_activate (GtkWidget *widget, gpointer user_data)
{
GtkWidget *dialog;

dialog = gtk_file_chooser_dialog_new ("Send File",
				      GTK_WINDOW(main_window),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      "_Send", GTK_RESPONSE_ACCEPT,
				      NULL);
gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);

if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GSList *list = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER(dialog));
    GSList *item;
    for (item = list; item != NULL; item = item->next) {
      add_one_uri(item->data);
      g_free(item->data);
    }
    if (list != NULL) send_queue();
    g_slist_free (list);
  }

gtk_widget_destroy (dialog);
}

/*
 * This function is programs main function.
 *
 * Params:	int argc: Number of command line arguments,
 *			char **argv: Pointer to command line arguments.
 *
 * Return: int: Different values (0, -1, -2, -3) depending on how program ends.
 *
 */

int main(int argc, char **argv) {
  pthread_t send_tid, recv_tid;

  GtkWidget *main_vbox, *paned, *frame, *scrolled_window;
  GtkWidget *menu_bar, *item, *menu, *sub_item;

  g_thread_init(NULL);
  gtk_init (&argc, &argv);

  pref_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (pref_window, "pref_window");
  gtk_window_set_title (GTK_WINDOW (pref_window), "6share preferences");
  g_signal_connect ((gpointer) pref_window, "delete_event",
                    G_CALLBACK (gtk_widget_hide_on_delete),
                    NULL);
  main_vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(pref_window), main_vbox);

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (main_window, "main_window");
  gtk_window_set_title (GTK_WINDOW (main_window), "6Share");
  g_signal_connect ((gpointer) main_window, "delete_event",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  main_vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(main_window), main_vbox);

  status_bar = gtk_statusbar_new();
  default_context = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar),
                                                 "default");
  gtk_statusbar_push(GTK_STATUSBAR(status_bar), default_context, "Ready");
  gtk_box_pack_end(GTK_BOX(main_vbox), status_bar, FALSE, FALSE, 1);
  
  menu_bar = gtk_menu_bar_new();
  item = gtk_menu_item_new_with_mnemonic ("_File");
  gtk_menu_bar_append(menu_bar, item);
  menu = gtk_menu_new();
  sub_item = gtk_menu_item_new_with_mnemonic ("_Send...");
  gtk_menu_attach(GTK_MENU(menu), sub_item, 0, 1, 0, 1);
  g_signal_connect ((gpointer) sub_item, "activate",
                    G_CALLBACK (on_menu_send_activate),
                    NULL);
  GtkAccelGroup *accel_group = gtk_accel_group_new();
  sub_item = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
  gtk_menu_attach(GTK_MENU(menu), sub_item, 0, 1, 1, 2);
  g_signal_connect ((gpointer) sub_item, "activate",
                    G_CALLBACK (gtk_main_quit),
                    NULL);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  item = gtk_menu_item_new_with_mnemonic ("_Edit");
  gtk_menu_bar_append(menu_bar, item);
  menu = gtk_menu_new();
  sub_item = gtk_menu_item_new_with_mnemonic ("_Preferences...");
  gtk_menu_attach(GTK_MENU(menu), sub_item, 0, 1, 0, 1);
  g_signal_connect ((gpointer) sub_item, "activate",
                    G_CALLBACK (on_menu_pref_activate),
                    NULL);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  item = gtk_menu_item_new_with_mnemonic ("_Help");
  gtk_menu_bar_append(menu_bar, item);
  menu = gtk_menu_new();
  sub_item = gtk_menu_item_new_with_mnemonic ("_About");
  gtk_menu_attach(GTK_MENU(menu), sub_item, 0, 1, 0, 1);
  g_signal_connect ((gpointer) sub_item, "activate",
                    G_CALLBACK (on_menu_about_activate),
                    NULL);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, FALSE, 1);

  paned = gtk_vpaned_new();
  gtk_box_pack_start(GTK_BOX(main_vbox), paned, TRUE, TRUE, 1);
  recv_vbox = gtk_vbox_new(FALSE, 0);
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), recv_vbox);
  gtk_widget_set_size_request(scrolled_window, -1, 200);
//  frame = gtk_frame_new(NULL);
//  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
//  gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
  gtk_paned_pack1(GTK_PANED(paned), scrolled_window, TRUE, FALSE);
  send_vbox = gtk_vbox_new(FALSE, 0);
  drop_msg_label= gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(drop_msg_label), "<i>Drop things here to send them</i>");
  gtk_widget_set_size_request(drop_msg_label, 250,-1);
  gtk_box_pack_start(GTK_BOX(send_vbox), drop_msg_label, TRUE, TRUE, 1);
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(frame), send_vbox);
  gtk_paned_pack2(GTK_PANED(paned), frame, FALSE, FALSE);

  GtkDestDefaults flags = GTK_DEST_DEFAULT_ALL;
  gtk_drag_dest_set(main_window, flags, targets,
                    TARGET_COUNT, GDK_ACTION_COPY | GDK_ACTION_PRIVATE);
  g_signal_connect ((gpointer) main_window, "drag_data_received",
                    G_CALLBACK (on_drag_data_received),
                    NULL);

  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
  gtk_widget_show_all (main_window);

/* create Q */
common.async = g_async_queue_new();

pthread_cond_init(&(common.cond), NULL);
pthread_cond_init(&(common.wakeup), NULL);
pthread_mutex_init(&(common.mutex), NULL);
common.queue = NULL;
common.downloads = NULL;
#ifdef LINUX
srand((unsigned int) argv);
common.send_toi = rand();
#else
common.send_toi = 1;
#endif

validateargs(&common.send_arg, SENDER);
memset(common.send_arg.fdt_file, 0, MAX_LENGTH);
strcpy(common.send_arg.base_dir, "/");
pthread_create(&send_tid, NULL, flute_send_thread, NULL);

//validateargs(&common.recv_arg, RECEIVER);
if (argc > 1) {
  common.recv_arg.srcaddr = argv[1];
} else {
  common.recv_arg.srcaddr = "2002:5244:29c4::1";
}
pthread_create(&recv_tid, NULL, flute_recv_thread, NULL);

  gtk_timeout_add(200, run_queue, NULL);

  gtk_main ();

printf("Waiting for FLUTE thead shutdown...\n");
set_session_state(common.recv_id, SExiting);
pthread_join(recv_tid, NULL);
set_session_state(common.send_id, SExiting);
pthread_mutex_lock(&(common.mutex));
pthread_cond_signal(&(common.wakeup));
pthread_mutex_unlock(&(common.mutex));
pthread_join(send_tid, NULL);
  return 0;
}

int flute_recv_main(void *parm) {

	int retval = 0;
	int retcode = 0;
	
#ifdef WIN32
	WSADATA wsd;
#endif

	int ch_id[MAX_CHANNELS_IN_SESSION];           /* Channel identifiers */


	/* Set signal handler */

//	signal(SIGINT, signal_handler);
#ifdef WIN32
//	signal(SIGBREAK, signal_handler);
#endif

	common.recv_id = -1; 

#ifdef WIN32

	if(WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		printf("WSAStartup() failed\n");

		if(common.recv_arg.logfd != -1) {
			close(common.recv_arg.logfd);
		}

		return -1;
	}
#endif

	memset(ch_id, -1, MAX_CHANNELS_IN_SESSION);

	retval = flute_receiver_prepare(&common.recv_arg, &common.recv_id, &ch_id[0]);

	while (retval == 0) {
	        retval = receiver_in_fdt_based_mode(&common.recv_id, &common.recv_arg);
#if 0
		if (retval == -3) {
			set_session_state(common.recv_id, SActive); /* FIXME */
			usleep(1000);
			retval = 0;
		}
#endif
	}

	if(retval == -3) {
		am_status("Session ending...");
//		printf("Sender closed the session\n");
//		fflush(stdout);
	}

	if(retval == -4) { /* bind failed */

#ifdef USE_SDPLIB
		if(strcmp(common.recv_arg.sdp_file, "") != 0) {
			sf_free(common.recv_arg.src_filt);
			free(common.recv_arg.src_filt);
			sdp_free(common.recv_arg.sdp);
			free(common.recv_arg.sdp);
		}
#endif
		if(common.recv_arg.logfd != -1) {
			close(common.recv_arg.logfd);
		}

#ifdef WIN32
		WSACleanup();
#endif			
		return -4;
	}

	if(common.recv_id == -1) {

#ifdef USE_SDPLIB
		if(strcmp(common.recv_arg.sdp_file, "") != 0) {
			sf_free(common.recv_arg.src_filt);
			free(common.recv_arg.src_filt);
			sdp_free(common.recv_arg.sdp);
			free(common.recv_arg.sdp);
		}
#endif
		if(common.recv_arg.logfd != -1) {
			close(common.recv_arg.logfd);
		}

#ifdef WIN32
		WSACleanup();
#endif
		return -1;
	}

#ifdef USE_SDPLIB
	if(strcmp(common.recv_arg.sdp_file, "") != 0) {
		sf_free(common.recv_arg.src_filt);
		free(common.recv_arg.src_filt);
		sdp_free(common.recv_arg.sdp);
		free(common.recv_arg.sdp);
	}
#endif

	/* To avoid recvfrom() failed, because of socket closing. */
	set_session_state(common.recv_id, SExiting);
		
	if(common.recv_arg.cc_id == Null) {
			
		while(common.recv_arg.nb_channel) {
			retcode = remove_alc_channel(common.recv_id, ch_id[common.recv_arg.nb_channel - 1]);
			common.recv_arg.nb_channel--;
		}	
	}
#ifdef USE_RLC
	else if(common.recv_arg.cc_id == RLC) {
			
		retcode = remove_alc_channel(common.recv_id, ch_id[0]);
		printf("\n%i packets lost in session.\n", get_alc_session(common.recv_id)->lost_packets);
		fflush(stdout);
	}
#endif

	close_alc_session(common.recv_id);

	if(common.recv_arg.logfd != -1) {
		close(common.recv_arg.logfd);
	}

#ifdef WIN32
	WSACleanup();
#endif

	return retval;
}

void * flute_recv_thread(void *parm) {
	int retval = 0;

	while (retval == 0 || retval == -3) {
		validateargs(&common.recv_arg, RECEIVER);
		retval = flute_recv_main(parm);
	}
	return NULL;
}

/* FIXME */

void * flute_send_thread(void *parm) {

	int i;
	int retval = 0;
	int retcode = 0;
	
#ifdef WIN32
	WSADATA wsd;
#endif

	int ch_id[MAX_CHANNELS_IN_SESSION];           /* Channel identifiers */


	/* Set signal handler */

//	signal(SIGINT, signal_handler);
#ifdef WIN32
//	signal(SIGBREAK, signal_handler);
#endif

	common.send_id = -1; 

#ifdef WIN32

	if(WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WSAStartup() failed\n");

		if(common.send_arg.logfd != -1) {
			close(common.send_arg.logfd);
		}

        return -1;
    }
#endif

	memset(ch_id, -1, MAX_CHANNELS_IN_SESSION);

	retval = flute_sender_prepare(&common.send_arg, &common.send_id, &ch_id[0]);

	while (retval == 0 || retval == 866181) {
		pthread_mutex_lock(&(common.mutex));
		pthread_cond_wait(&(common.wakeup), &(common.mutex));
		pthread_mutex_unlock(&(common.mutex));
		
  		am_status("Sending...");
		retval = sender_in_fdt_based_mode(&common.send_id, &ch_id[0], &common.send_arg);
  		am_status("Ready");
	}

	if(common.send_id == -1) {

		if(common.send_arg.logfd != -1) {
			close(common.send_arg.logfd);
		}

#ifdef WIN32
		WSACleanup();
#endif
			return;
	}

	if(retval == -4) { /* bind failed */

		if(common.send_arg.logfd != -1) {
			close(common.send_arg.logfd);
		}

#ifdef WIN32
		WSACleanup();
#endif
		return;	
	}
	
	if(retval != -1) {
		printf("\nSending session close packets\n");
		fflush(stdout);
	}

	while(common.send_arg.nb_channel) {

		if(retval != -1) {

			if(common.send_arg.cc_id == Null) {

				for(i = 0; i < 3; i++) {
					retcode = send_session_close_packet(common.send_id, ch_id[common.send_arg.nb_channel - 1]);
						
					if(retcode == -1) {
						break;
					}
				}
#ifdef WIN32
				Sleep(200);
#else
				usleep(200000);
#endif
			}

#ifdef USE_RLC
			else if(common.send_arg.cc_id == RLC) {

				if(common.send_arg.nb_channel - 1 == 0) {
				
					for(i = 0; i < 3; i++) {
						retcode = send_session_close_packet(common.send_id, ch_id[common.send_arg.nb_channel - 1]);

						if(retcode == -1) {
							break;
						}
					}
#ifdef WIN32
					Sleep(200);
#else
					usleep(200000);
#endif
				}
			}
#endif
		}

		retcode = remove_alc_channel(common.send_id, ch_id[common.send_arg.nb_channel - 1]);
		common.send_arg.nb_channel--;
	}
		
	close_alc_session(common.send_id);

	if(common.send_arg.logfd != -1) {
		close(common.send_arg.logfd);
	}

#ifdef WIN32
	WSACleanup();
#endif

printf("FLUTE send thread ends...\n");

	return;
}

/*
 * This function receives and handles signals.
 *
 * Params:	int sig: Must be, not used for anything.
 *
 * Return:	void
 *
 */

void signal_handler(int sig){

	int session_state = get_session_state(common.send_id);

	if(session_state != -1) {

		printf("\nExiting...\n\n");
		fflush(stdout);

		set_session_state(common.send_id, SExiting);
	}
}


/*
 * This function print programs usage information.
 *
 * Params:	void
 *
 * Return:	void
 *
 */

void usage(void) {

	char c;

	printf("\nFLUTE Version 1.0, June 21th, 2004\n\n");
	printf("  Copyright (c) 2003-2004 TUT - Tampere University of Technology\n");
	printf("  main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi\n");
	printf("  web site: http://www.atm.tut.fi/mad\n\n");
	printf("  This is free software, and you are welcome to redistribute it\n");
	printf("  under certain conditions; See the GNU General Public License\n");
	printf("  as published by the Free Software Foundation, version 2 or later,\n");
	printf("  for more details.\n");

#ifdef USE_RLC
	printf("\n  * MAD-RCL library\n");
	printf("  * mad_rlc.c & mad_rlc.h -- Portions of code derived from MCL library by\n");
	printf("  * Vincent Roca et al. (http://www.inrialpes.fr/planete/people/roca/mcl/)\n");
	printf("  *\n");
	printf("  * Copyright (c) 1999-2004 INRIA - Universite Paris 6 - All rights reserved\n");
	printf("  * (main author: Julien Laboure - julien.laboure@inrialpes.fr\n");
	printf("  *               Vincent Roca - vincent.roca@inrialpes.fr)\n");
#endif

#ifdef USE_REED_SOLOMON
	printf("\n  * MAD-RS-FEC library\n");
	printf("  * fec.c & fec.h -- forward error correction based on Vandermonde matrices\n");
	printf("  * 980624\n");
	printf("  * (C) 1997-98 Luigi Rizzo (luigi@iet.unipi.it)\n");
	printf("  *\n");
	printf("  * Portions derived from code by Phil Karn (karn@ka9q.ampr.org),\n");
	printf("  * Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) and\n");
	printf("  * Hari Thirumoorthy (harit@spectra.eng.hawaii.edu), Aug 1995\n");
	printf("  *\n");
	printf("  * Redistribution and use in source and binary forms, with or without\n");
	printf("  * modification, are permitted provided that the following conditions\n");
	printf("  * are met:\n");
	printf("  *\n");
	printf("  * 1. Redistributions of source code must retain the above copyright\n");
	printf("  *    notice, this list of conditions and the following disclaimer.\n");
	printf("  * 2. Redistributions in binary form must reproduce the above\n");
	printf("  *    copyright notice, this list of conditions and the following\n");
	printf("  *    disclaimer in the documentation and/or other materials\n");
	printf("  *    provided with the distribution.\n");
	printf("  *\n");
	printf("  * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND\n");
	printf("  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,\n");
	printf("  * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A\n");
	printf("  * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS\n");
	printf("  * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,\n");
	printf("  * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n");
	printf("  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,\n");
	printf("  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
	printf("  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR\n");
	printf("  * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT\n");
	printf("  * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY\n");
	printf("  * OF SUCH DAMAGE.\n");
#endif

	printf("\nUsage: flute [-S] [-U] [-m:str] [-p:int] [-i:str] [-c:int] [-t:ull]\n"); 
	printf("             [-o:ull] [-e:int] [-F:str,str,...] [-a:str] [-b:int] [-h]\n");
	printf("             [-f:str] [-k:int] [-l:int] [-r:int] [-v:int] [[-n:int] | [-C]]\n");
	printf("             [-T:int] [-P:float,float] [-L:int] [-H] [-A] [-B:str] [-s:str]\n");
	printf("             [-D:ull] [-E:int] [-V:str]");

#ifdef USE_SDPLIB
	printf(" [-d:str]");
#endif

#if defined (USE_REED_SOLOMON) || defined (USE_SIMPLE_XOR)
	printf(" [-x:int]");
#endif

#ifdef USE_REED_SOLOMON
	printf(" [-X:int]");
#endif
	
#ifdef USE_RLC
	printf(" [-w:int]");
#endif

#ifdef USE_ZLIB
	printf("\n             [-z:int]");
#endif

#ifdef WIN32
	printf(" [-O]");
#endif
#ifdef SSM
	printf(" [-M]");
#endif
	printf("\n\n");
	printf("Common options:\n\n");
    
	printf("   -S               Act as sender, send data; otherwise receive data.\n");
	printf("   -U               Address type is unicast, default: multicast\n");
	printf("   -m:str           IP4 or IP6 address for base channel,\n");
	printf("                    default: %s or %s\n", DEF_MCAST_IPv4_ADDR, DEF_MCAST_IPv6_ADDR);
	printf("   -p:int           Port number for base channel, default: %s\n", DEF_MCAST_PORT);
	printf("   -i:str           Local IP address to bind to, default: INADDR_ANY\n");
	printf("   -I:str           Local interface for multicast join; otherwise OS default\n");
	printf("   -t:ull           TSI for this session, default: %i\n", DEF_TSI);

#ifdef USE_RLC
	printf("   -w:int           Congestion control scheme [0 = Null, 1 = RLC],\n");
	printf("                    default: %i\n", DEF_CC);
#endif

	printf("   -a:str           Address family (IP4 or IP6), default: IP4\n");
	printf("   -v:int           Verbosity level, [0 = low, 1 = high] default: 1\n");
	printf("   -V:str           Print logs to 'str' file, default: print to stdout\n");
	printf("   -h               Print this help.\n");
		
	printf("\nSender options:\n\n");
	
	printf("   -c:int           Number of used channels, default: %i\n", DEF_NB_CHANNEL);

#ifdef USE_ZLIB
	printf("   -z:int           Encode content [0 = no, 1 = FDT, 2 = FDT and files],\n");
	printf("                    default: %i\n", 0);
#endif	

	printf("   -D:ull           Duration of the session in seconds, default: %i\n", DEF_DURATION);

	printf("   -f:str           FDT file (send based on FDT), default: %s\n", DEF_FDT);
	printf("   -k:int           Number of files defined in one FDT Instance,\n");
	printf("                    default: %i\n", DEF_FILES_IN_FDT_INST);
	printf("   -l:int           Encoding symbol length in bytes, default: %i\n", DEF_SYMB_LENGTH);
	printf("   -r:int           Transmission rate in kbits/s, default: %i\n", DEF_TX_RATE);
	printf("   -T:int           Time To Live or Hop Limit for this session, default: %i\n", DEF_TTL);
	printf("   -e:int           Use EXT_FTI extensions header for file objects [1 = yes,\n");
	printf("                    0 = no], default: 1\n");
	printf("   -F:str           File or directory to be sent\n");
	printf("   -n:int           Number of transmissions, default: %i\n", DEF_TX_NB);
	printf("   -C               Continuous transmission, default: not used\n");
	printf("   -P[:float,float] Simulate packet losses, default: %.1f,%.1f\n", (float)P_LOSS_WHEN_OK,
		   (float)P_LOSS_WHEN_LOSS);

#if defined (USE_REED_SOLOMON) || defined (USE_SIMPLE_XOR)
	printf("   -x:int           FEC Encoding [0 = Null-FEC, 1 = Simple XOR,\n");
	printf("                    2 = Reed-Solomon], default: %i\n", DEF_FEC);
#endif

#ifdef USE_REED_SOLOMON
        printf("   -X:int           FEC ratio percent, default: %i\n", DEF_FEC_RATIO);
#endif


	printf("   -L:int           Maximum source block length in multiple of encoding\n");
	printf("                    symbols, default: %i\n", DEF_MAX_SB_LEN);
	printf("   -H               Use Half-word (if used TSI field could be 16, 32 or 48\n");
	printf("                    bits long and TOI field could be 16, 32, 48 or 64 bits\n");
	printf("                    long), default: not used\n");
	printf("   -B:str           Base directory for files to be sent,\n");
	printf("                    default: working directory\n");

	printf("\nReceiver options:\n\n");

	printf("   -A               Receive files automatically.\n");
	printf("   -F:str,str,...   File(s) to be received\n");
	printf("   -b:int           Receiver in big file mode [0 = no, 1 = yes], default: 1\n"); 
	printf("   -c:int           Maximum number of channels, default: %i\n", DEF_NB_CHANNEL);

#ifdef USE_SDPLIB
	printf("   -d:str           SDP file (join based on SDP file), default: no\n");
#endif

	printf("   -B:str           Base directory for downloaded files, default: %s\n", DEF_BASE_DIR);
	printf("   -s:str           Source IP4 or IP6 address of this session\n");
	printf("   -o:ull           TOI for the file to be received\n");

#ifdef WIN32
	printf("   -O               Open received file(s) automatically, default: no\n");
#endif

#ifdef SSM
	printf("   -M               Use Source Specific Multicast, default: no\n");
#endif

	printf("   -E:int           Accept Expired FDT Instances [0 = no, 1 = yes], default: 0\n"); 

	printf("\nPress Enter for more help");
	scanf("%c", &c);

	printf("\nExample use cases:\n\n");
	printf("1. Send file or directory n times\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -F:files/flute-draft.txt\n");
	printf("\t      -o:1 -n:2\n\n");
	printf("2. Send file or directory in a loop\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -F:files/flute-draft.txt\n");
	printf("\t      -o:1 -C\n\n");
	printf("3. Send files defined in an FDT file\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -f:fdt2.xml\n\n");
	printf("4. Send files defined in an FDT file in a loop\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -f:fdt2.xml -C\n\n");
	printf("5. Send using UDP\n\n\tflute -S -U -m:1.2.3.4 -p:4000 -t:2 -r:100 -f:fdt2.xml -C\n\n");
	printf("6. Receive one object\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n");
	printf("\t      -F:download/flute-draft.txt -o:1\n\n");
	printf("7. Receive file(s) with filename(s)\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n");
	printf("\t      -F:files/flute-man.txt,flute-draft.txt\n\n");
	printf("8. Receive file(s) defined by wild card option\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2 -F:*.jpg\n\n");
	printf("9. Receive file(s) with User Interface\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n\n");
	printf("10. Receive file(s) automatically from session\n\n\tflute -A -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n\n");
	printf("11. Receive using UDP\n\n\tflute -A -U -p:4000 -t:2 -s:2.2.2.2\n\n");

	exit(1);
}

/*
 * This function validates and parses command line arguments
 *
 * Params:	int argc: Number of command line arguments,
 *			char **argv: Pointer to command line arguments,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 *
 * Return:	void
 *
 */

void validateargs(arguments_t *a, int mode) {
	int i;
	unsigned int tmp_max_sblen = 0;
	unsigned int tmp_eslen = 0;
	char *tmp_addr = NULL;
	char *tmp_srcaddr = NULL;
	unsigned char fec_enc = DEF_FEC;
	char *tmp_base_dir = NULL;

#ifdef WIN32
	ULONGLONG tmp_tsi;
#else
	char *ep;
	unsigned long long tmp_tsi;
#endif
	
	int errno;
	div_t div_max_k;
	div_t div_max_n;

	char *loss_ratio2 = NULL;
	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

	time(&systime);
	curr_time = systime + 2208988800;

	/* Set default values for global parameters */

	a->cc_id = DEF_CC;									/* congestion control */

	a->mode = mode;									/* sender or receiver? */			
	
	tmp_tsi = DEF_TSI;									/* transport session identifier */
	a->toi = 0;											/* transport object identifier */
	
	a->port = DEF_MCAST_PORT;							/* local port number for base channel */
	a->nb_channel = DEF_NB_CHANNEL;						/* number of channels */
	a->intface = NULL;									/* interface */
	a->intface_name = NULL;									/* interface name for multicast join */

	a->addr_family = DEF_ADDR_FAMILY;
	a->addr_type = 0;
	a->eslen = DEF_SYMB_LENGTH;							/* encoding symbol length */
	a->max_sblen = DEF_MAX_SB_LEN;							/* source block length */
	a->txrate = DEF_TX_RATE;							/* transmission rate in kbit/s */
	a->ttl = DEF_TTL;									/* time to live  */
	a->nb_tx = DEF_TX_NB;								/* nb of time to send the file/directory */
	a->cont = 0;										/* continuous transmission */
	a->simul_losses = false;							/* simulate packet losses */
	a->loss_ratio1 = P_LOSS_WHEN_OK;
	a->loss_ratio2 = P_LOSS_WHEN_LOSS;
	a->fec_ratio = DEF_FEC_RATIO;						/* FEC ratio percent */	
	a->fec_enc_id = DEF_FEC_ENC_ID;						/* FEC Encoding */
	a->fec_inst_id = DEF_FEC_INST_ID; 

	a->files_in_fdt_inst = DEF_FILES_IN_FDT_INST;		/* number of files defined in FDT Instance */

	a->big_file_mode = 1;
	a->verbosity = 1;

	memset(a->file_path, 0, MAX_LENGTH);				/* file(s) or directory to send */
	
	memset(a->fdt_file, 0, MAX_LENGTH);
	strcpy(a->fdt_file, DEF_FDT);						/* fdt file (fdt based send) */
		
	a->rx_automatic = 0;								/* download files defined in IDT automatically */

#ifdef USE_SDPLIB
	memset(a->sdp_file, 0, MAX_LENGTH);
#endif

	a->file_list_mode = 1;

	a->use_fec_oti_ext_hdr = 1;							/* include fec oti extension header to flute header */

#ifdef USE_ZLIB
	a->encode_content = 0;								/* encode content using zlib library */
#endif

#ifdef WIN32
	a->openfile = 0;									/* open received file automatically */
#endif

#ifdef SSM
	a->use_ssm = false;										/* use source specific multicast */
#endif

	a->half_word = false;
	a->starttime = curr_time;
	a->stoptime = curr_time + DEF_DURATION;
	a->accept_expired_fdt_inst = 0;

	a->logfd = -1;

/* FIXME these are special cased */

a->eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130;
a->addr_family = PF_INET6;
a->addr = DEF_MCAST_IPv6_ADDR;
//a->addr = "ff12::6181";
a->verbosity = 0;

if (mode == RECEIVER) {
  a->rx_automatic = 1;
  a->file_list_mode = 0;
} else {
  a->txrate = 4000;
//  a->txrate = 500;

  a->cc_id = 1; /* RLC */
  a->fec_ratio = 50; /* 50% extra */
  fec_enc = 2; /* RS */

  a->toi = FDT_TOI;
  a->cont = 1; /* continuous */
//  a->nb_tx = 2; /* two loops */
}


 /* FIXME */
#if 0
    for(i = 1; i < argc ; i++) {

        if(argv[i][0] == '-') {
            
			switch(argv[i][1])
			{
				case 'A':  /* automatic download */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->rx_automatic = 1;
					a->file_list_mode = 0;
                    break;
				case 'B':  /* base directory for downloaded files */
                    if(strlen(argv[i]) > 3) {
                        tmp_base_dir = &argv[i][3];
					}
					else {
						usage();
					}
					break;
				case 'C':  /* Continuous transmission */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->cont = 1;
                    break;
				case 'e':  /* Use fec oti extension header */
                    if(strlen(argv[i]) > 3) {
                        a->use_fec_oti_ext_hdr = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
				case 'E':  /* Accept Expired FDT Instances */
                    if(strlen(argv[i]) > 3) {
                        a->accept_expired_fdt_inst = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
				case 'F':  /* file(s) or directory to send/receive */
                    if(strlen(argv[i]) > 3) {
                        memset(a->file_path, 0, MAX_LENGTH);
                        strcpy(a->file_path, &argv[i][3]);
                    }
					else {
						usage();
					}
					memset(a->fdt_file, 0, MAX_LENGTH);
					break;
				case 'H':  /* use half-word */
                    if(strlen(argv[i]) > 2) {
                            usage();
                    }
                    a->half_word = true;
                    break;
#ifdef WIN32
				case 'O':  /* automatic opening */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->openfile = 1;
                    break;
#endif

#ifdef SSM
				case 'M':  /* use source specific multicast */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->use_ssm = true;
                    break;
#endif
				
				case 'P':  /* simulate packet losses */
					if(strlen(argv[i]) >= 2) {
						a->simul_losses = true;
					}
					if(strlen(argv[i]) > 3) {
						a->loss_ratio1 = atof(strtok(&argv[i][3], ","));
						loss_ratio2 = strtok(NULL, "\0");
						if(loss_ratio2 == NULL) {
							usage();
						}
							a->loss_ratio2 = atof(loss_ratio2);
					}
					if(strlen(argv[i]) == 3) {
						usage();
					}
                    break;

#ifdef USE_REED_SOLOMON
                case 'X':  /* FEC ratio percent*/
                    if(strlen(argv[i]) > 3) {
                        a->fec_ratio = atoi(&argv[i][3]);
                    }
                    else {
                        usage();
                    }
                    break;
#endif

				case 'S':  /* sender */
					if(strlen(argv[i]) > 2) {
						usage();
					}
	                    a->mode = SENDER;
					a->toi = FDT_TOI;
                    break;
				case 'T':  /* ttl */
                    if(strlen(argv[i]) > 3) {
                       a->ttl = atoi(&argv[i][3]);
                    }
					else {
						usage();
					}
                    break;
				case 'U':  /* unicast address */                    
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->addr_type = 1;
                    break;
				case 'V': /* redirect stderr and stdout into given file */
					if (strlen(argv[i]) > 3) {
#ifdef WIN32
						a->logfd = _open(&argv[i][3], _O_RDWR | _O_CREAT | _O_TRUNC, 0666);
#else 
						a->logfd = open(&argv[i][3], O_RDWR | O_CREAT | O_TRUNC, 0666);
#endif

						if(a->logfd < 0) {
						
							printf("Log file: %s cannot be opened\n", &argv[i][3]);
							fflush(stdout);
							exit(-1);
						}

#ifdef WIN32
						_dup2(a->logfd, _fileno(stdout));
						_dup2(a->logfd, _fileno(stderr));
#else
						dup2(a->logfd, fileno(stdout));
						dup2(a->logfd, fileno(stderr));
#endif
					}
					break;
				case 'a':  /* address family */
					if(strlen(argv[i]) > 3) {
						if(!(strcmp(&argv[i][3], "IP4"))) {
							a->addr_family = PF_INET;
						}
						else if(!(strcmp(&argv[i][3], "IP6"))) {
							a->eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130;
							a->addr_family = PF_INET6;
						}
					}
					else {
						usage();
					}
                    break;
				case 'v':  /* verbosity level */
					if(strlen(argv[i]) > 3) {
							a->verbosity = atoi(&argv[i][3]);
					}
					else {
							usage();
					}
                    break;
				case 'b':  /* big file mode */
                    if(strlen(argv[i]) > 3) {
						a->big_file_mode = atoi(&argv[i][3]);
                    }
                    else {
						usage();
                    }
                    break;
				case 'c':  /* number of channels */
                    if(strlen(argv[i]) > 3) {
						a->nb_channel = atoi(&argv[i][3]);

						if(a->nb_channel > MAX_CHANNELS_IN_SESSION) {
							printf("Channel number set to maximum value: %i\n", MAX_CHANNELS_IN_SESSION);
							a->nb_channel = MAX_CHANNELS_IN_SESSION;
						}	
					}
					else {
						usage();
					}
                    break;

#ifdef USE_SDPLIB
				case 'd':  /* sdp file */
                    if(strlen(argv[i]) > 3) {
						memset(a->sdp_file, 0, MAX_LENGTH);
                        strcpy(a->sdp_file, &argv[i][3]);
					}
					else {
						usage();
					}
					break;
#endif

#ifdef USE_ZLIB
				case 'z':  /* encode content */
                    if(strlen(argv[i]) > 3) {
                        a->encode_content = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
#endif
				case 'f':  /* fdt based send */
                    if(strlen(argv[i]) > 3) {
                        memset(a->fdt_file, 0, MAX_LENGTH);
                        strcpy(a->fdt_file, &argv[i][3]);
                    }
					else {
						usage();
					}
					memset(a->file_path, 0, MAX_LENGTH);
					break;
 				case 'i':  /* local interface address to bind to */
                    if(strlen(argv[i]) > 3) {
                        a->intface = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 'I':  /* local interface name for multicast join */
                    if(strlen(argv[i]) > 3) {
                        a->ifname = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 'h':  /* help/usage */
					usage();
					break;
				case 'k':  /* number of files defined in FDT Instance */
                    if(strlen(argv[i]) > 3) {
                        a->files_in_fdt_inst = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
				case 'l':  /*  encoding symbol length */
                    if(strlen(argv[i]) > 3) {
						tmp_eslen = (unsigned short)atoi(&argv[i][3]);						
                    }
					else {
						usage();
					}
                    break;
				case 'm':  /* Multicast group for base channel */
                    if(strlen(argv[i]) > 3) {
						tmp_addr = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 'n':  /* tx loops */
                    if(strlen(argv[i]) > 3) {
                       a->nb_tx = atoi(&argv[i][3]);
                    }
					else {
						usage();
					}
                    break;
				case 'o':  /* toi */
                    if(strlen(argv[i]) > 3) {
#ifdef WIN32			  
						a->toi = _atoi64(&argv[i][3]);

						if(a->toi > 0xFFFFFFFFFFFFFFFF) {
							printf("TOI: %I64u too big (max=%I64u)\n", a->toi,  0xFFFFFFFFFFFFFFFF);
                            fflush(stdout);
                            exit(-1);
						}
#else
						/*a->toi = atoll(&argv[i][3]);*/
  						
						a->toi = strtoull(&argv[i][3], &ep, 10);

                        if(&argv[i][3] == '\0' || *ep != '\0') {
                                printf("TOI not a number\n");
                                fflush(stdout);
                                exit(-1);
                        }

                        if(errno == ERANGE && a->toi == 0xFFFFFFFFFFFFFFFF) {
                                printf("TOI too big for unsigned long long (64 bits)\n");
                                fflush(stdout);
                                exit(-1);
                        }
#endif	
						a->file_list_mode = 0;
                    }
					else {
						usage();
					}
                    break;
				case 'D':  /* session duration time */
                    if(strlen(argv[i]) > 3) {
#ifdef WIN32			  
						a->duration = _atoi64(&argv[i][3]);

						if(a->duration > 0xFFFFFFFFFFFFFFFF) {
							printf("Duration: %I64u too big (max=%I64u)\n", a->duration,  0xFFFFFFFFFFFFFFFF);
                            fflush(stdout);
                            exit(-1);
						}
#else
						/*a->duration = atoll(&argv[i][3]);*/
  						
						a->duration = strtoull(&argv[i][3], &ep, 10);

                        if(&argv[i][3] == '\0' || *ep != '\0') {
                            printf("Duration not a number\n");
                            fflush(stdout);
                            exit(-1);
                        }

                        if(errno == ERANGE && a->duration == 0xFFFFFFFFFFFFFFFF) {
                            printf("Duration too big for unsigned long long (64 bits)\n");
                            fflush(stdout);
                            exit(-1);
                        }
#endif	
                    }
					else {
						usage();
					}
                    break;
                case 'p':  /* Port number for base channel*/
                    if(strlen(argv[i]) > 3) {
						a->port = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 'r':  /* Transmission rate in kbits/s */
                    if(strlen(argv[i]) > 3) {      
						a->txrate = atoi(&argv[i][3]);
                    }
					else {
						usage();
					}
					break;
				case 's':  /* first alc session identifier */
                    if(strlen(argv[i]) > 3) {
						tmp_srcaddr = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 't':  /* second alc session identifier */
                    if(strlen(argv[i]) > 3) {
#ifdef WIN32			  
						tmp_tsi = _atoi64(&argv[i][3]);
#else
						/*tmp_tsi = atoll(&argv[i][3]);*/
				
           				tmp_tsi = strtoull(&argv[i][3], &ep, 10);
           					
						if(&argv[i][3] == '\0' || *ep != '\0') {                   
							printf("TSI not a number\n");
                            fflush(stdout);
                            exit(-1);
						}

           				if(errno == ERANGE && tmp_tsi == 0xFFFFFFFFFFFFFFFF) {   
							printf("TSI too big for unsigned long long (64 bits)\n");
                            fflush(stdout);
                            exit(-1);
						}
#endif 
						if(tmp_tsi > 0xFFFFFFFFFFFF) {
#ifdef WIN32
							printf("TSI: %I64u too big (max=%I64u)\n", tmp_tsi, 0xFFFFFFFFFFFF);
#else
							printf("TSI: %llu too big (max=%llu)\n", tmp_tsi, 0xFFFFFFFFFFFF);
#endif
                            fflush(stdout);
                            exit(-1);
						}
                    }
					else {
						usage();
					}
                    break;

#if defined (USE_REED_SOLOMON) || defined (USE_SIMPLE_XOR)
				case 'x':  /* fec encoding */
                    if(strlen(argv[i]) > 3) {
                        fec_enc = (unsigned char)atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
#endif

				case 'L':  /* sbl length */
                    if(strlen(argv[i]) > 3) {
                        tmp_max_sblen = (unsigned int)atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;

#ifdef USE_RLC
				case 'w':  /* congestion control */
                    if(strlen(argv[i]) > 3) {
						a->cc_id = (unsigned int)atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
#endif

                default:
					usage();
                    break;
            }
        }
		else {
			usage();
		}
    }

	if(tmp_addr == NULL) {
		if(a->addr_family == PF_INET) {
			tmp_addr = DEF_MCAST_IPv4_ADDR;
		}
		else if(a->addr_family == PF_INET6) {
			tmp_addr = DEF_MCAST_IPv6_ADDR;
		}
	}
	a->addr = tmp_addr;

	if(tmp_srcaddr == NULL) {
		if(a->addr_family == PF_INET) {
			tmp_srcaddr = DEF_SRC_IPv4_ADDR;
		}
		else if(a->addr_family == PF_INET6) {
			tmp_srcaddr = DEF_SRC_IPv6_ADDR;
		}
	}
	a->srcaddr = tmp_srcaddr;
#endif



#ifdef USE_SIMPLE_XOR
        if(fec_enc == 1) {
                a->fec_enc_id = SIMPLE_XOR_FEC_ENC_ID;

                if(a->addr_family == PF_INET) {
                        a->eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129;
                }
                else if(a->addr_family == PF_INET6) {
                        a->eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129;
                }
        }
#endif

#ifdef USE_REED_SOLOMON
	if(fec_enc == 2) {
		a->fec_enc_id = SB_SYS_FEC_ENC_ID;
		a->fec_inst_id = REED_SOL_FEC_INST_ID;
	
		if(a->addr_family == PF_INET) {
			a->eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129;
		}
		else if(a->addr_family == PF_INET6) {
			a->eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129;
		}
	}
#endif

	if(a->mode == RECEIVER) {
		a->tsi = tmp_tsi;

		memset(a->base_dir, 0, MAX_LENGTH);

		if(tmp_base_dir == NULL) {
			strcpy(a->base_dir, DEF_BASE_DIR);
		}
		else {
			strcpy(a->base_dir, tmp_base_dir);
		}
	}
	else {
		if(((a->half_word == false) && (tmp_tsi > 0xFFFFFFFF))) {
			printf("TSI too big for unsigned long (32 bits)\n");
			fflush(stdout);
			exit(-1);
		}
		else {
			a->tsi = tmp_tsi;
		}

		memset(a->base_dir, 0, MAX_LENGTH);

		if(tmp_base_dir != NULL) {
			strcpy(a->base_dir, tmp_base_dir);
		}
	}

	if(tmp_max_sblen != 0) {
		if(a->fec_enc_id == COM_NO_C_FEC_ENC_ID) {
#ifdef USE_NULL_FEC
			if(tmp_max_sblen > MAX_SB_LEN_NULL_FEC) {
				printf("Maximum source block length set to maximum value: %i\n", MAX_SB_LEN_NULL_FEC);
				fflush(stdout);
				tmp_max_sblen = MAX_SB_LEN_NULL_FEC;
			}
			a->max_sblen = tmp_max_sblen;
#endif
		}

#ifdef USE_SIMPLE_XOR
                else if(a->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
                        if(tmp_max_sblen > MAX_SB_LEN_SIMPLE_XOR_FEC) {
                                printf("Maximum source block length set to maximum value: %i\n", MAX_SB_LEN_SIMPLE_XOR_FEC);
                                fflush(stdout);
                                tmp_max_sblen = MAX_SB_LEN_SIMPLE_XOR_FEC;
                        }
                        a->max_sblen = tmp_max_sblen;
                }
#endif

#ifdef USE_REED_SOLOMON
		else if(((a->fec_enc_id == SB_SYS_FEC_ENC_ID) && (a->fec_inst_id == REED_SOL_FEC_INST_ID))) {
			
			div_max_n = div((tmp_max_sblen * (100 + a->fec_ratio)), 100);
			
			if(div_max_n.quot > MAX_N_REED_SOLOMON) {
				
				div_max_k = div((MAX_N_REED_SOLOMON * 100), (100 + a->fec_ratio));
				
				printf("Maximum source block length set to maximum value: %i\n", div_max_k.quot);
				fflush(stdout);
				tmp_max_sblen = div_max_k.quot;
			}
			a->max_sblen = tmp_max_sblen;
		}
#endif
	}

	if(tmp_eslen != 0) {
		if(a->addr_family == PF_INET) {
			if(((a->fec_enc_id == COM_NO_C_FEC_ENC_ID) || (a->fec_enc_id == COM_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130;
				}
			}
			else if(((a->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (a->fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (a->fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129;
				}	
			}
		}
		else if(a->addr_family == PF_INET6) {	
			if(((a->fec_enc_id == COM_NO_C_FEC_ENC_ID) || (a->fec_enc_id == COM_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130;
				}
			}
			else if(((a->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (a->fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (a->fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129;
				}	
			}
		}

		a->eslen = tmp_eslen;
	}
}
