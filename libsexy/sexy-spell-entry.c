/*
 * @file libsexy/sexy-icon-entry.c Entry widget
 *
 * @Copyright (C) 2004-2006 Christian Hammond.
 * Some of this code is from gtkspell, Copyright (C) 2002 Evan Martin.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <sys/types.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <enchant.h>

#include "sexy-spell-entry.h"
#include "gtkspell-iso-codes.h"

/**
 * SECTION: sexy-spell-entry
 * @short_description:  #GtkEntry with spell check
 * @title: SpellEntry
 * @see_also: #GtkEntry
 * @include: libsexy3/sexy-spell-entry.h
 *
 * Text entry widget with spell check.
 * Enchant is used for the backend.
 *
 */

typedef struct
{
	EnchantBroker *broker;
	PangoAttrList *attr_list;
	GdkRGBA *underline_color;
	gint mark_character;
	GHashTable *dict_hash;
	GSList *dict_list;
	gchar **words;
	gint *word_starts;
	gint *word_ends;
	gboolean checked;
	gint preedit_length;
} SexySpellEntryPrivate;

static void sexy_spell_entry_class_init(SexySpellEntryClass *klass);
static void sexy_spell_entry_editable_init (GtkEditableInterface *iface);
static void sexy_spell_entry_init (SexySpellEntry *entry);
static void sexy_spell_entry_finalize (GObject *obj);
static void sexy_spell_entry_dispose (GObject *obj);
static gint sexy_spell_entry_draw (GtkWidget *widget, cairo_t *cr);
static gint sexy_spell_entry_button_press(GtkWidget *widget, GdkEventButton *event);
static void sexy_spell_entry_set_property (GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void sexy_spell_entry_get_property (GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void sexy_spell_entry_style_updated (GtkWidget *widget);


/* GtkEditable handlers */
static void sexy_spell_entry_changed (GtkEditable *editable, gpointer data);

/* Other handlers */
static gboolean sexy_spell_entry_popup_menu (GtkWidget *widget, SexySpellEntry *entry);

/* Internal utility functions */
static gint sexy_spell_entry_find_position (SexySpellEntry *entry, gint x);
static gboolean word_misspelled (SexySpellEntry *entry, int start, int end);
static gboolean default_word_check (SexySpellEntry *entry, const gchar *word);
static gboolean sexy_spell_entry_activate_language_internal (SexySpellEntry *entry,
                                                            const gchar *lang, GError **error);
static gchar* get_lang_from_dict (EnchantDict *dict);
static void sexy_spell_entry_recheck_all (SexySpellEntry *entry);
static void entry_strsplit_utf8 (GtkEntry *entry, gchar ***set, gint **starts, gint **ends);

G_DEFINE_TYPE_WITH_CODE (SexySpellEntry, sexy_spell_entry, GTK_TYPE_ENTRY,
	G_IMPLEMENT_INTERFACE(GTK_TYPE_EDITABLE, sexy_spell_entry_editable_init)
	G_ADD_PRIVATE(SexySpellEntry))

#define SEXY_SPELL_ENTRY_GET_PRIVATE(obj) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((obj), SEXY_TYPE_SPELL_ENTRY, SexySpellEntryPriv))

static int codetable_ref = 0;

enum
{
	WORD_CHECK,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {0};

enum
{
	PROP_0,
	PROP_CHECKED,
	N_PROPERTIES
};

static gboolean
spell_accumulator(GSignalInvocationHint *hint, GValue *return_accu, const GValue *handler_return, gpointer data)
{
	gboolean ret = g_value_get_boolean (handler_return);
	/* Handlers return TRUE if the word is misspelled.  In this
	 * case, it means that we want to stop if the word is checked
	 * as correct */
	g_value_set_boolean (return_accu, ret);
	return ret;
}

static void
free_words (SexySpellEntryPrivate *priv)
{
	if (priv->words)
		g_strfreev (priv->words);
	if (priv->word_starts)
		g_free (priv->word_starts);
	if (priv->word_ends)
		g_free (priv->word_ends);
}

static void
sexy_spell_entry_class_init(SexySpellEntryClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);

	klass->word_check = default_word_check;

	object_class->set_property = sexy_spell_entry_set_property;
	object_class->get_property = sexy_spell_entry_get_property;
	object_class->finalize = sexy_spell_entry_finalize;
	object_class->dispose = sexy_spell_entry_dispose;

	widget_class->draw = sexy_spell_entry_draw;
	widget_class->button_press_event = sexy_spell_entry_button_press;
	widget_class->style_updated = sexy_spell_entry_style_updated;

	/**
	 * SexySpellEntry::word-check:
	 * @entry: The entry on which the signal is emitted.
	 * @word: The word to check.
	 *
	 * The ::word-check signal is emitted whenever the entry has to check
	 * a word.  This allows the application to mark words as correct even
	 * if none of the active dictionaries contain it, such as nicknames in
	 * a chat client.
	 *
	 * Returns: %FALSE to indicate that the word should be marked as
	 * correct.
	 */
	signals[WORD_CHECK] = g_signal_new("word_check",
					   G_TYPE_FROM_CLASS(object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET(SexySpellEntryClass, word_check),
					   (GSignalAccumulator) spell_accumulator, NULL,
					   NULL,
					   G_TYPE_BOOLEAN,
					   1, G_TYPE_STRING);

	/**
	 * SexySpellEntry:checked:
	 *
	 * If checking of spelling is enabled.
	 *
	 * Since: 1.0
	 */
	g_object_class_install_property (object_class, PROP_CHECKED,
							g_param_spec_boolean ("checked", "Checked",
										"If checking spelling is enabled",
										TRUE, G_PARAM_READWRITE));
	/**
	 * SexySpellEntry:underline-color:
	 *
	 * The underline color of misspelled words.
	 * Defaults to red.
	 *
	 * Since: 1.0
	 */
	gtk_widget_class_install_style_property (widget_class,
							g_param_spec_boxed ("underline-color", "Underline Color",
										 "Underline color of misspelled words.",
										 GDK_TYPE_RGBA, G_PARAM_READABLE));
}

static void
sexy_spell_entry_editable_init (GtkEditableInterface *iface)
{
}

static void
sexy_spell_entry_set_property (GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(obj);

	switch (prop_id)
	{
		case PROP_CHECKED:
			sexy_spell_entry_set_checked (entry, g_value_get_boolean(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
			break;
	}
}

static void
sexy_spell_entry_get_property (GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(obj);

	switch (prop_id)
	{
		case PROP_CHECKED:
			g_value_set_boolean (value, sexy_spell_entry_get_checked (entry));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
			break;
	}
}

static gint
sexy_spell_entry_find_position (SexySpellEntry *entry, gint x)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	PangoLayout *layout;
	PangoLayoutLine *line;
	const gchar *text;
	gint cursor_index;
	gint index;
	gint pos;
	gboolean trailing;

	x = x + GPOINTER_TO_INT(g_object_get_data (G_OBJECT(entry), "scroll-offset"));

	layout = gtk_entry_get_layout (GTK_ENTRY(entry));
	text = pango_layout_get_text (layout);
	cursor_index = GPOINTER_TO_INT(g_object_get_data (G_OBJECT(entry), "cursor-position"));

	line = pango_layout_get_line_readonly (layout, 0);
	pango_layout_line_x_to_index (line, x * PANGO_SCALE, &index, &trailing);

	if (index >= cursor_index && priv->preedit_length)
	{
		if (index >= cursor_index + priv->preedit_length)
		{
			index -= priv->preedit_length;
		}
		else
		{
			index = cursor_index;
			trailing = FALSE;
		}
	}

	pos = g_utf8_pointer_to_offset (text, text + index);
	pos += trailing;

	return pos;
}

static void
insert_underline(SexySpellEntry *entry, guint start, guint end)
{
	PangoAttribute *ucolor;
	PangoAttribute *unline = pango_attr_underline_new (PANGO_UNDERLINE_ERROR);
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	if (priv->underline_color)
	{
		GdkRGBA *uc = priv->underline_color;

		ucolor = pango_attr_underline_color_new (uc->red * 65535, uc->green * 65535, uc->blue * 65535);
	}
	else
		ucolor = pango_attr_underline_color_new (65535, 0, 0);

	ucolor->start_index = start;
	unline->start_index = start;

	ucolor->end_index = end;
	unline->end_index = end;

	pango_attr_list_insert (priv->attr_list, ucolor);
	pango_attr_list_insert (priv->attr_list, unline);
}

static void
get_word_extents_from_position(SexySpellEntry *entry, gint *start, gint *end, guint position)
{
	const gchar *text;
	gint i, bytes_pos;
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	*start = -1;
	*end = -1;

	if (priv->words == NULL)
		return;

	text = gtk_entry_get_text (GTK_ENTRY(entry));
	bytes_pos = (gint)(g_utf8_offset_to_pointer(text, position) - text);

	for (i = 0; priv->words[i]; i++)
	{
		if (bytes_pos >= priv->word_starts[i] &&
		    bytes_pos <= priv->word_ends[i])
		{
			*start = priv->word_starts[i];
			*end   = priv->word_ends[i];
			return;
		}
	}
}

static void
add_to_dictionary(GtkWidget *menuitem, SexySpellEntry *entry)
{
	char *word;
	gint start, end;
	EnchantDict *dict;
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	get_word_extents_from_position (entry, &start, &end, priv->mark_character);
	word = gtk_editable_get_chars (GTK_EDITABLE(entry), start, end);

	dict = (EnchantDict *) g_object_get_data(G_OBJECT(menuitem), "enchant-dict");
	if (dict)
		enchant_dict_add_to_personal(dict, word, -1);

	g_free(word);

	free_words (priv);
	entry_strsplit_utf8 (GTK_ENTRY(entry), &priv->words,
						&priv->word_starts, &priv->word_ends);
	sexy_spell_entry_recheck_all (entry);
}

static void
ignore_all(GtkWidget *menuitem, SexySpellEntry *entry)
{
	char *word;
	gint start, end;
	GSList *li;
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	get_word_extents_from_position (entry, &start, &end, priv->mark_character);
	word = gtk_editable_get_chars (GTK_EDITABLE(entry), start, end);

	for (li = priv->dict_list; li; li = g_slist_next (li))
	{
		EnchantDict *dict = (EnchantDict *) li->data;
		enchant_dict_add_to_session (dict, word, -1);
	}

	g_free (word);

	free_words (priv);
	entry_strsplit_utf8(GTK_ENTRY(entry), &priv->words,
						&priv->word_starts, &priv->word_ends);
	sexy_spell_entry_recheck_all(entry);
}

static void
replace_word(GtkWidget *menuitem, SexySpellEntry *entry)
{
	char *oldword;
	const char *newword;
	gint start, end;
	gint cursor;
	EnchantDict *dict;
  	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	get_word_extents_from_position (entry, &start, &end, priv->mark_character);
	oldword = gtk_editable_get_chars (GTK_EDITABLE(entry), start, end);
	newword = gtk_label_get_text (GTK_LABEL(gtk_bin_get_child (GTK_BIN(menuitem))));

	cursor = gtk_editable_get_position (GTK_EDITABLE(entry));
	/* is the cursor at the end? If so, restore it there */
	if (g_utf8_strlen(gtk_entry_get_text (GTK_ENTRY(entry)), -1) == cursor)
		cursor = -1;
	else if(cursor > start && cursor <= end)
		cursor = start;

	gtk_editable_delete_text (GTK_EDITABLE(entry), start, end);
	gtk_editable_set_position (GTK_EDITABLE(entry), start);
	gtk_editable_insert_text (GTK_EDITABLE(entry), newword, strlen(newword), &start);
	gtk_editable_set_position (GTK_EDITABLE(entry), cursor);

	dict = (EnchantDict*)g_object_get_data(G_OBJECT(menuitem), "enchant-dict");

	if (dict)
		enchant_dict_store_replacement(dict,
					       oldword, -1,
					       newword, -1);

	g_free(oldword);
}

static void
build_suggestion_menu(SexySpellEntry *entry, GtkWidget *menu, EnchantDict *dict, const gchar *word)
{
	GtkWidget *mi;
	gchar **suggestions;
	size_t n_suggestions, i;

	suggestions = enchant_dict_suggest (dict, word, -1, &n_suggestions);

	if (suggestions == NULL || n_suggestions == 0)
	{
		/* no suggestions.  put something in the menu anyway... */
		GtkWidget *label = gtk_label_new ("");
		gtk_label_set_markup (GTK_LABEL(label), _("<i>(no suggestions)</i>"));

		mi = gtk_separator_menu_item_new ();
		gtk_container_add (GTK_CONTAINER(mi), label);
		gtk_widget_show_all (mi);
		gtk_menu_shell_prepend (GTK_MENU_SHELL(menu), mi);
	}
	else
	{
		/* build a set of menus with suggestions */
		for (i = 0; i < n_suggestions; i++)
		{
			if ((i != 0) && (i % 10 == 0))
			{
				mi = gtk_separator_menu_item_new ();
				gtk_widget_show (mi);
				gtk_menu_shell_append (GTK_MENU_SHELL(menu), mi);

				mi = gtk_menu_item_new_with_label (_("More..."));
				gtk_widget_show (mi);
				gtk_menu_shell_append (GTK_MENU_SHELL(menu), mi);

				menu = gtk_menu_new ();
				gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), menu);
			}

			mi = gtk_menu_item_new_with_label (suggestions[i]);
			g_object_set_data (G_OBJECT(mi), "enchant-dict", dict);
			g_signal_connect (G_OBJECT(mi), "activate", G_CALLBACK(replace_word), entry);
			gtk_widget_show (mi);
			gtk_menu_shell_append (GTK_MENU_SHELL(menu), mi);
		}
	}

	enchant_dict_free_suggestions(dict, suggestions);
}

static GtkWidget *
build_spelling_menu(SexySpellEntry *entry, const gchar *word)
{
	EnchantDict *dict;
	GtkWidget *topmenu, *mi;
	gchar *label;
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	topmenu = gtk_menu_new ();

	if (priv->dict_list == NULL)
		return topmenu;

	/* Suggestions */
	if (g_slist_length(priv->dict_list) == 1)
	{
		dict = (EnchantDict*)priv->dict_list->data;
		build_suggestion_menu (entry, topmenu, dict, word);
	}
	else
	{
		GSList *li;
		GtkWidget *menu;
		gchar *lang, *lang_name;

		for (li = priv->dict_list; li; li = g_slist_next (li))
		{
			dict = (EnchantDict*)li->data;
			lang = get_lang_from_dict (dict);
			lang_name = sexy_spell_entry_get_language_name (entry, lang);
			if (lang_name)
			{
				mi = gtk_menu_item_new_with_label (lang_name);
				g_free (lang_name);
			}
			else
			{
				mi = gtk_menu_item_new_with_label (lang);
			}
			g_free (lang);

			gtk_widget_show (mi);
			gtk_menu_shell_append (GTK_MENU_SHELL(topmenu), mi);
			menu = gtk_menu_new ();
			gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), menu);
			build_suggestion_menu (entry, menu, dict, word);
		}
	}

	/* Separator */
	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL(topmenu), mi);

	/* + Add to Dictionary */
	label = g_strdup_printf (_("Add \"%s\" to Dictionary"), word);
	mi = gtk_menu_item_new_with_label (label);
	g_free (label);

	if (g_slist_length (priv->dict_list) == 1)
	{
		dict = (EnchantDict*)priv->dict_list->data;
		g_object_set_data (G_OBJECT(mi), "enchant-dict", dict);
		g_signal_connect (G_OBJECT(mi), "activate", G_CALLBACK(add_to_dictionary), entry);
	}
	else
	{
		GSList *li;
		GtkWidget *menu, *submi;
		gchar *lang, *lang_name;

		menu = gtk_menu_new ();
		gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), menu);

		for (li = priv->dict_list; li; li = g_slist_next (li))
		{
			dict = (EnchantDict*)li->data;
			lang = get_lang_from_dict (dict);
			lang_name = sexy_spell_entry_get_language_name (entry, lang);
			if (lang_name)
			{
				submi = gtk_menu_item_new_with_label (lang_name);
				g_free (lang_name);
			}
			else
			{
				submi = gtk_menu_item_new_with_label (lang);
			}
			g_free (lang);

			g_object_set_data (G_OBJECT(submi), "enchant-dict", dict);
			g_signal_connect (G_OBJECT(submi), "activate", G_CALLBACK(add_to_dictionary), entry);

			gtk_widget_show (submi);
			gtk_menu_shell_append (GTK_MENU_SHELL(menu), submi);
		}
	}

	gtk_widget_show_all (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL(topmenu), mi);

	/* - Ignore All */
	mi = gtk_menu_item_new_with_label (_("Ignore All"));
	g_signal_connect (G_OBJECT(mi), "activate", G_CALLBACK(ignore_all), entry);
	gtk_widget_show_all (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL(topmenu), mi);

	return topmenu;
}

static void
sexy_spell_entry_preedit_changed (SexySpellEntry *entry, gchar *preedit, gpointer userdata)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	priv->preedit_length = strlen (preedit);
}

static void
sexy_spell_entry_populate_popup(SexySpellEntry *entry, GtkMenu *menu, gpointer data)
{
	GtkWidget *mi;
	gint start, end;
	gchar *word;
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	if (priv->checked == FALSE)
		return;

	if (g_slist_length (priv->dict_list) == 0)
		return;

	get_word_extents_from_position (entry, &start, &end, priv->mark_character);
	if (start == end)
		return;
	if (!word_misspelled (entry, start, end))
		return;

	/* separator */
	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_prepend (GTK_MENU_SHELL(menu), mi);

	/* Above the separator, show the suggestions menu */
	mi = gtk_menu_item_new_with_label (_("Spelling Suggestions"));

	word = gtk_editable_get_chars (GTK_EDITABLE(entry), start, end);
	g_assert (word != NULL);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), build_spelling_menu(entry, word));
	g_free (word);

	gtk_widget_show_all (mi);
	gtk_menu_shell_prepend (GTK_MENU_SHELL(menu), mi);
}

static void
sexy_spell_entry_init(SexySpellEntry *entry)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	priv->dict_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	sexy_spell_entry_activate_default_languages (entry);

	if (codetable_ref == 0)
		codetable_init ();
	codetable_ref++;

	priv->attr_list = pango_attr_list_new();

	priv->checked = TRUE;
	priv->preedit_length = 0;

	g_signal_connect (G_OBJECT(entry), "popup-menu",
					  G_CALLBACK(sexy_spell_entry_popup_menu), entry);
	g_signal_connect (G_OBJECT(entry), "populate-popup",
					  G_CALLBACK(sexy_spell_entry_populate_popup), NULL);
	g_signal_connect (G_OBJECT(entry), "changed",
					  G_CALLBACK(sexy_spell_entry_changed), NULL);
	g_signal_connect (G_OBJECT(entry), "preedit-changed",
					  G_CALLBACK(sexy_spell_entry_preedit_changed), NULL);
}

static void
sexy_spell_entry_finalize(GObject *obj)
{
	SexySpellEntry *entry;
	SexySpellEntryPrivate *priv;

	g_return_if_fail (obj != NULL);
	g_return_if_fail (SEXY_IS_SPELL_ENTRY(obj));

	entry = SEXY_SPELL_ENTRY(obj);
	priv = sexy_spell_entry_get_instance_private (entry);

	if (priv->attr_list)
		pango_attr_list_unref (priv->attr_list);
	if (priv->dict_hash)
		g_hash_table_destroy (priv->dict_hash);
	free_words(priv);

	if (priv->broker)
	{
		GSList *li;
		for (li = priv->dict_list; li; li = g_slist_next(li))
		{
			EnchantDict *dict = (EnchantDict*)li->data;
			enchant_broker_free_dict (priv->broker, dict);
		}
		g_slist_free (priv->dict_list);

		enchant_broker_free (priv->broker);
	}

	codetable_ref--;
	if (codetable_ref == 0)
		codetable_free ();

	G_OBJECT_CLASS(sexy_spell_entry_parent_class)->finalize(obj);
}

static void
sexy_spell_entry_dispose(GObject *obj)
{
	g_return_if_fail (SEXY_IS_SPELL_ENTRY(obj));

	G_OBJECT_CLASS(sexy_spell_entry_parent_class)->dispose(obj);
}

/**
 * sexy_spell_entry_new
 *
 * Creates a new SexySpellEntry widget.
 *
 * Returns: (transfer full): a new #SexySpellEntry.
 */
GtkWidget *
sexy_spell_entry_new(void)
{
	return GTK_WIDGET(g_object_new (SEXY_TYPE_SPELL_ENTRY, NULL));
}

GQuark
sexy_spell_error_quark(void)
{
	static GQuark q = 0;
	if (q == 0)
		q = g_quark_from_static_string ("sexy-spell-error-quark");
	return q;
}

static gboolean
default_word_check(SexySpellEntry *entry, const gchar *word)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	gboolean result = TRUE;
	GSList *li;

	if (g_unichar_isalpha (*word) == FALSE)
	{
		/* We only want to check words */
		return FALSE;
	}
	for (li = priv->dict_list; li; li = g_slist_next (li))
	{
		EnchantDict *dict = (EnchantDict*)li->data;
		if (enchant_dict_check (dict, word, strlen(word)) == 0)
		{
			result = FALSE;
			break;
		}
	}
	return result;
}

static gboolean
word_misspelled(SexySpellEntry *entry, int start, int end)
{
	const gchar *text;
	gchar *word;
	gboolean ret;

	if (start == end)
		return FALSE;
	text = gtk_entry_get_text (GTK_ENTRY(entry));
	word = g_new0 (gchar, end - start + 2);

	g_strlcpy (word, text + start, end - start + 1);

	g_signal_emit (entry, signals[WORD_CHECK], 0, word, &ret);

	g_free (word);
	return ret;
}

static void
check_word(SexySpellEntry *entry, int start, int end)
{
  	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	PangoAttrIterator *it;

	/* Check to see if we've got any attributes at this position.
	 * If so, free them, since we'll readd it if the word is misspelled */
	it = pango_attr_list_get_iterator (priv->attr_list);
	if (it == NULL)
		return;
	do {
		gint s, e;
		pango_attr_iterator_range(it, &s, &e);
		if (s == start)
		{
			GSList *attrs = pango_attr_iterator_get_attrs (it);
			g_slist_foreach (attrs, (GFunc)pango_attribute_destroy, NULL);
			g_slist_free (attrs);
		}
	} while (pango_attr_iterator_next(it));
	pango_attr_iterator_destroy (it);

	if (word_misspelled (entry, start, end))
		insert_underline (entry, start, end);
}

static void
sexy_spell_entry_recheck_all(SexySpellEntry *entry)
{
  	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GdkRectangle rect;
	GtkWidget *widget = GTK_WIDGET(entry);
	PangoLayout *layout;
	int i;

	if (priv->checked == FALSE)
		return;

	if (g_slist_length (priv->dict_list) == 0)
		return;

	/* Remove all existing pango attributes.  These will get readded as we check */
	pango_attr_list_unref (priv->attr_list);
	priv->attr_list = pango_attr_list_new ();

	/* Loop through words */
	for (i = 0; priv->words[i]; i++)
	{
		if (!strlen (priv->words[i]))
			continue;
		check_word (entry, priv->word_starts[i], priv->word_ends[i]);
	}

	layout = gtk_entry_get_layout (GTK_ENTRY(entry));
	pango_layout_set_attributes (layout, priv->attr_list);

	if (gtk_widget_get_realized (GTK_WIDGET(entry)))
	{
		rect.x = 0; rect.y = 0;
		rect.width  = gtk_widget_get_allocated_width (widget);
		rect.height = gtk_widget_get_allocated_height (widget);
		gdk_window_invalidate_rect (gtk_widget_get_window(widget), &rect, TRUE);
	}
}

static gint
sexy_spell_entry_draw(GtkWidget *widget, cairo_t *cr)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(widget);
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GtkEntry *gtk_entry = GTK_ENTRY(widget);
	PangoLayout *layout;

	if (priv->checked)
	{
		layout = gtk_entry_get_layout (gtk_entry);
		pango_layout_set_attributes (layout, priv->attr_list);
	}

	return GTK_WIDGET_CLASS(sexy_spell_entry_parent_class)->draw (widget, cr);
}

static void
sexy_spell_entry_style_updated (GtkWidget *widget)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(widget);
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GdkRGBA *underline_color = NULL;

	GTK_WIDGET_CLASS (sexy_spell_entry_parent_class)->style_updated (widget);

	gtk_widget_style_get (widget, "underline-color", &underline_color, NULL);

	if (priv->underline_color && priv->underline_color != underline_color)
		gdk_rgba_free (priv->underline_color);

	priv->underline_color = underline_color;

	if (priv->words != NULL)
		sexy_spell_entry_recheck_all (entry);
}

static gint
sexy_spell_entry_button_press (GtkWidget *widget, GdkEventButton *event)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(widget);
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	gint pos;

	pos = sexy_spell_entry_find_position (entry, event->x);
	priv->mark_character = pos;

	return GTK_WIDGET_CLASS(sexy_spell_entry_parent_class)->button_press_event (widget, event);
}

static gboolean
sexy_spell_entry_popup_menu (GtkWidget *widget, SexySpellEntry *entry)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	/* Menu popped up from a keybinding (menu key or <shift>+F10). Use
	 * the cursor position as the mark position */
	priv->mark_character = gtk_editable_get_position (GTK_EDITABLE (entry));
	return FALSE;
}

static void
entry_strsplit_utf8 (GtkEntry *entry, gchar ***set, gint **starts, gint **ends)
{
	PangoLayout *layout;
	const PangoLogAttr *log_attrs;
	const gchar *text;
	gint n_attrs = 0, n_strings, i, j;
	PangoLogAttr a;

	layout = gtk_entry_get_layout (GTK_ENTRY(entry));
	text = gtk_entry_get_text (GTK_ENTRY(entry));
	log_attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

	/* Find how many words we have */
	for (i = 0, n_strings = 0; i < n_attrs; i++)
	{
		a = log_attrs[i];
		if (a.is_word_start && a.is_word_boundary)
			n_strings++;
	}

	*set = g_new0 (gchar *, n_strings + 1);
	*starts = g_new0 (gint, n_strings);
	*ends = g_new0 (gint, n_strings);

	/* Copy out strings */
	for (i = 0, j = 0; i < n_attrs; i++)
	{
		a = log_attrs[i];
		if (a.is_word_start && a.is_word_boundary)
		{
			gint cend, bytes;
			gchar *start;

			/* Find the end of this string */
			for (cend = i; cend < n_attrs; cend++)
			{
				a = log_attrs[cend];
				if (a.is_word_end && a.is_word_boundary)
					break;
			}

			/* Copy sub-string */
			start = g_utf8_offset_to_pointer(text, i);
			bytes = (gint) (g_utf8_offset_to_pointer(text, cend) - start);
			(*set)[j] = g_new0(gchar, bytes + 1);
			(*starts)[j] = (gint) (start - text);
			(*ends)[j] = (gint) (start - text + bytes);
			g_utf8_strncpy((*set)[j], start, cend - i);

			/* Move on to the next word */
			j++;
		}
	}
}

static void
sexy_spell_entry_changed(GtkEditable *editable, gpointer data)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(editable);
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	if (priv->checked == FALSE)
		return;
	if (g_slist_length(priv->dict_list) == 0)
		return;

	free_words(priv);
	entry_strsplit_utf8(GTK_ENTRY(entry), &priv->words,
						&priv->word_starts, &priv->word_ends);
	sexy_spell_entry_recheck_all(entry);
}

static gboolean
enchant_has_lang(const gchar *lang, GSList *langs)
{
	GSList *i;
	for (i = langs; i; i = g_slist_next(i))
	{
		if (strcmp(lang, i->data) == 0)
			return TRUE;
	}
	return FALSE;
}


static GSList *
get_default_spell_languages (void)
{
	const gchar* const *langs = g_get_language_names ();
	GSList *lang_list = NULL;
	char *last = NULL;
	char *p;
	int i;

	if (langs != NULL)
	{
		for (i = 0; langs[i]; i++)
		{
			if (g_ascii_strncasecmp (langs[i], "C", 1) != 0 && strlen (langs[i]) >= 2)
			{
				/* Avoid duplicates, "en" and "en_US" for example. */
				if (!last || !g_str_has_prefix (langs[i], last))
				{
					if (last != NULL)
						g_free(last);

					/* ignore .utf8 */
					if ((p = strchr (langs[i], '.')))
						*p='\0';

					last = g_strndup (langs[i], 2);

					lang_list = g_slist_append (lang_list, (char*)langs[i]);
				}
			}
		}
		if (last != NULL)
			g_free(last);
	}

	return lang_list;
}

/**
 * sexy_spell_entry_activate_default_languages:
 * @entry: A #SexySpellEntry.
 *
 * Activate spell checking for languages specified in the $LANG
 * or $LANGUAGE environment variables. If none is found it defaults to "en".
 * These languages are activated by default, so this function need only
 * be called if they were previously deactivated.
 */
void
sexy_spell_entry_activate_default_languages(SexySpellEntry *entry)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GSList *enchant_langs, *langs, *lang_item;
	char *lang;

	if (!priv->broker)
		priv->broker = enchant_broker_init();

	enchant_langs = sexy_spell_entry_get_languages(entry);
	langs = get_default_spell_languages();
	if (langs)
	{
		lang_item = langs;
		while (lang_item != NULL)
		{
			lang = (char*)lang_item->data;
			if (enchant_has_lang (lang, enchant_langs))
			{
				sexy_spell_entry_activate_language_internal (entry, lang, NULL);
			}
			lang_item = g_slist_next(lang_item);
		}
		g_slist_free (langs);
	}
	g_slist_free_full (enchant_langs, g_free);

	/* If we don't have any languages activated, use "en" */
	if (priv->dict_list == NULL)
		sexy_spell_entry_activate_language_internal(entry, "en", NULL);

	sexy_spell_entry_recheck_all (entry);
}

static void
get_lang_from_dict_cb(const char * const lang_tag,
		      const char * const provider_name,
		      const char * const provider_desc,
		      const char * const provider_file,
		      void * user_data)
{
	gchar **lang = (gchar **)user_data;
	*lang = g_strdup(lang_tag);
}

static gchar *
get_lang_from_dict(EnchantDict *dict)
{
	gchar *lang;

	enchant_dict_describe(dict, get_lang_from_dict_cb, &lang);
	return lang;
}

static gboolean
sexy_spell_entry_activate_language_internal(SexySpellEntry *entry, const gchar *lang, GError **error)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	EnchantDict *dict;

	if (!priv->broker)
		priv->broker = enchant_broker_init();

	if (g_hash_table_lookup(priv->dict_hash, lang))
		return TRUE;

	dict = enchant_broker_request_dict(priv->broker, lang);

	if (!dict)
	{
		g_set_error(error, SEXY_SPELL_ERROR, SEXY_SPELL_ERROR_BACKEND, _("enchant error for language: %s"), lang);
		return FALSE;
	}

	priv->dict_list = g_slist_append(priv->dict_list, (gpointer) dict);
	g_hash_table_insert(priv->dict_hash, get_lang_from_dict(dict), (gpointer) dict);

	return TRUE;
}

static void
dict_describe_cb(const char * const lang_tag,
		 const char * const provider_name,
		 const char * const provider_desc,
		 const char * const provider_file,
		 void * user_data)
{
	GSList **langs = (GSList **)user_data;

	*langs = g_slist_append(*langs, (gpointer)g_strdup(lang_tag));
}

/**
 * sexy_spell_entry_get_languages:
 * @entry: A #SexySpellEntry.
 *
 * Retrieve a list of language codes for which dictionaries are available.
 *
 * Returns: (transfer full) (element-type utf8): a new #GList object, or %NULL on error.
 *          Should be freed with g_slist_free_full() and g_free().
 */
GSList *
sexy_spell_entry_get_languages(const SexySpellEntry *entry)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GSList *langs = NULL;

	g_return_val_if_fail(entry != NULL, NULL);
	g_return_val_if_fail(SEXY_IS_SPELL_ENTRY(entry), NULL);

	if (!priv->broker)
		return NULL;

	enchant_broker_list_dicts(priv->broker, dict_describe_cb, &langs);

	return langs;
}

/**
 * sexy_spell_entry_get_language_name:
 * @entry: A #SexySpellEntry.
 * @lang: The language code to lookup a friendly name for.
 *
 * Get a friendly name for a given locale.
 *
 * Returns: (transfer full): The name of the locale. Should be freed with g_free()
 */
gchar *
sexy_spell_entry_get_language_name(const SexySpellEntry *entry,
								   const gchar *lang)
{
	const gchar *lang_name = "";
	const gchar *country_name = "";

	if (codetable_ref == 0)
		codetable_init ();

	codetable_lookup (lang, &lang_name, &country_name);

	if (codetable_ref == 0)
		codetable_free ();

	if (strlen (country_name) != 0)
		return g_strdup_printf ("%s (%s)", lang_name, country_name);
	else
		return g_strdup_printf ("%s", lang_name);
}

/**
 * sexy_spell_entry_language_is_active:
 * @entry: A #SexySpellEntry.
 * @lang: The language to use, in a form enchant understands.
 *
 * Determine if a given language is currently active.
 *
 * Returns: %TRUE if the language is active.
 */
gboolean
sexy_spell_entry_language_is_active(const SexySpellEntry *entry,
									const gchar *lang)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	return (g_hash_table_lookup(priv->dict_hash, lang) != NULL);
}

/**
 * sexy_spell_entry_activate_language:
 * @entry: A #SexySpellEntry
 * @lang: The language to use in a form Enchant understands. Typically either
 *        a two letter language code or a locale code in the form xx_XX.
 * @error: Return location for error.
 *
 * Activate spell checking for the language specifed.
 *
 * Returns: %FALSE if there was an error.
 */
gboolean
sexy_spell_entry_activate_language(SexySpellEntry *entry, const gchar *lang, GError **error)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	gboolean ret;

	g_return_val_if_fail(entry != NULL, FALSE);
	g_return_val_if_fail(SEXY_IS_SPELL_ENTRY(entry), FALSE);
	g_return_val_if_fail(lang != NULL && lang != '\0', FALSE);

	if (error)
		g_return_val_if_fail(*error == NULL, FALSE);

	ret = sexy_spell_entry_activate_language_internal (entry, lang, error);

	if (ret)
	{
		free_words (priv);
		entry_strsplit_utf8 (GTK_ENTRY(entry), &priv->words,
							&priv->word_starts, &priv->word_ends);
		sexy_spell_entry_recheck_all (entry);
	}

	return ret;
}

/**
 * sexy_spell_entry_deactivate_language:
 * @entry: A #SexySpellEntry.
 * @lang: The language in a form Enchant understands. Typically either
 *        a two letter language code or a locale code in the form xx_XX.
 *
 * Deactivate spell checking for the language specifed.
 */
void
sexy_spell_entry_deactivate_language(SexySpellEntry *entry, const gchar *lang)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);

	g_return_if_fail (entry != NULL);
	g_return_if_fail (SEXY_IS_SPELL_ENTRY(entry));

	if (!priv->dict_list)
		return;

	if (lang)
	{
		EnchantDict *dict;

		dict = g_hash_table_lookup (priv->dict_hash, lang);
		if (!dict)
			return;
		enchant_broker_free_dict(priv->broker, dict);
		priv->dict_list = g_slist_remove (priv->dict_list, dict);
		g_hash_table_remove (priv->dict_hash, lang);
	}
	else
	{
		/* deactivate all */
		GSList *li;
		EnchantDict *dict;

		for (li = priv->dict_list; li; li = g_slist_next (li))
		{
			dict = (EnchantDict*)li->data;
			enchant_broker_free_dict (priv->broker, dict);
		}

		g_slist_free (priv->dict_list);
		g_hash_table_destroy (priv->dict_hash);
		priv->dict_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
		priv->dict_list = NULL;
	}

	free_words (priv);
	entry_strsplit_utf8 (GTK_ENTRY(entry), &priv->words,
						&priv->word_starts, &priv->word_ends);
	sexy_spell_entry_recheck_all (entry);
}

/**
 * sexy_spell_entry_set_active_languages:
 * @entry: A #SexySpellEntry
 * @langs: (element-type utf8): A list of language codes to activate,
 *         in a form Enchant understands. Typically either a two letter
 *         language code or a locale code in the form xx_XX.
 * @error: Return location for error.
 *
 * Activate spell checking for only the languages specified.
 *
 * Returns: %FALSE if there was an error.
 */
gboolean
sexy_spell_entry_set_active_languages(SexySpellEntry *entry, GSList *langs, GError **error)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GSList *li;

	g_return_val_if_fail (entry != NULL, FALSE);
	g_return_val_if_fail (SEXY_IS_SPELL_ENTRY(entry), FALSE);
	g_return_val_if_fail (langs != NULL, FALSE);

	/* deactivate all languages first */
	sexy_spell_entry_deactivate_language (entry, NULL);

	for (li = langs; li; li = g_slist_next (li))
	{
		if (sexy_spell_entry_activate_language_internal (entry,
				(const gchar*)li->data, error) == FALSE)
			return FALSE;
	}
	free_words (priv);
	entry_strsplit_utf8 (GTK_ENTRY(entry), &priv->words,
						 &priv->word_starts, &priv->word_ends);
	sexy_spell_entry_recheck_all (entry);
	return TRUE;
}

/**
 * sexy_spell_entry_get_active_languages:
 * @entry: A #SexySpellEntry
 *
 * Retrieve a list of the currently active languages.
 *
 * Returns: (transfer full) (element-type utf8): A list of language codes ("en", "de_DE", etc).
 *          The list should be freed with g_slist_free_full () and g_free().
 */
GSList *
sexy_spell_entry_get_active_languages(SexySpellEntry *entry)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GSList *ret = NULL, *li;
	EnchantDict *dict;
	gchar *lang;

	g_return_val_if_fail (entry != NULL, NULL);
	g_return_val_if_fail (SEXY_IS_SPELL_ENTRY(entry), NULL);

	for (li = priv->dict_list; li; li = g_slist_next (li))
	{
		dict = (EnchantDict*)li->data;
		lang = get_lang_from_dict (dict);
		ret = g_slist_append (ret, lang);
	}
	return ret;
}

/**
 * sexy_spell_entry_get_checked:
 * @entry: A #SexySpellEntry.
 *
 * Queries a #SexySpellEntry and returns whether the entry has spell-checking enabled.
 *
 * Returns: %TRUE if the entry has spell-checking enabled.
 */
gboolean
sexy_spell_entry_get_checked(SexySpellEntry *entry)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	return priv->checked;
}

/**
 * sexy_spell_entry_is_checked:
 * @entry: A #SexySpellEntry.
 *
 * Queries a #SexySpellEntry and returns whether the entry has spell-checking enabled.
 * Use sexy_spell_entry_get_checked() instead.
 *
 * Deprecated: 1.0
 * Returns: %TRUE if the entry has spell-checking enabled.
 */
gboolean
sexy_spell_entry_is_checked(SexySpellEntry *entry)
{
	return sexy_spell_entry_get_checked (entry);
}

/**
 * sexy_spell_entry_set_checked:
 * @entry: A #SexySpellEntry.
 * @checked: Whether to enable spell-checking
 *
 * Sets whether the entry has spell-checking enabled.
 */
void
sexy_spell_entry_set_checked(SexySpellEntry *entry, gboolean checked)
{
	SexySpellEntryPrivate *priv = sexy_spell_entry_get_instance_private (entry);
	GtkWidget *widget;

	if (priv->checked == checked)
		return;

	priv->checked = checked;
	widget = GTK_WIDGET(entry);

	if (checked == FALSE && gtk_widget_get_realized (widget))
	{
		PangoLayout *layout;
		GdkRectangle rect;

		pango_attr_list_unref (priv->attr_list);
		priv->attr_list = pango_attr_list_new ();

		layout = gtk_entry_get_layout (GTK_ENTRY(entry));
		pango_layout_set_attributes (layout, priv->attr_list);

		rect.x = 0; rect.y = 0;
		rect.width  = gtk_widget_get_allocated_width (widget);
		rect.height = gtk_widget_get_allocated_height (widget);
		gdk_window_invalidate_rect (gtk_widget_get_window(widget), &rect, TRUE);
	}
	else
	{
		free_words (priv);
		entry_strsplit_utf8 (GTK_ENTRY(entry), &priv->words, &priv->word_starts, &priv->word_ends);
		sexy_spell_entry_recheck_all (entry);
	}
}
