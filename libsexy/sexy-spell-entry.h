/*
 * @file libsexy/sexy-spell-entry.h Entry widget
 *
 * @Copyright (C) 2004-2006 Christian Hammond.
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

#pragma once

#include <gtk/gtk.h>

#define SEXY_SPELL_ERROR (sexy_spell_error_quark())
#define SEXY_TYPE_SPELL_ENTRY (sexy_spell_entry_get_type())
G_DECLARE_DERIVABLE_TYPE(SexySpellEntry, sexy_spell_entry, SEXY, SPELL_ENTRY, GtkEntry)

/**
 * SexySpellError:
 * @SEXY_SPELL_ERROR_BACKEND: Error occured in enchant when calling
 *                            sexy_spell_entry_activate_language() or
 *                            sexy_spell_entry_set_active_languages().
 *
 * Type of error.
 */
typedef enum {
	SEXY_SPELL_ERROR_BACKEND
} SexySpellError;


struct _SexySpellEntryClass
{
	GtkEntryClass parent_class;

	/* Signals */
	gboolean (*word_check)(SexySpellEntry *entry, const gchar *word);

  	/*< private >*/
	void (*_gtk_reserved1) (void);
	void (*_gtk_reserved2) (void);
	void (*_gtk_reserved3) (void);
};

G_BEGIN_DECLS

GtkWidget* sexy_spell_entry_new(void);
GQuark sexy_spell_error_quark(void);

GSList* sexy_spell_entry_get_languages(const SexySpellEntry *entry);
gchar* sexy_spell_entry_get_language_name(const SexySpellEntry *entry, const gchar *lang);
gboolean sexy_spell_entry_language_is_active(const SexySpellEntry *entry, const gchar *lang);
gboolean sexy_spell_entry_activate_language(SexySpellEntry *entry, const gchar *lang, GError **error);
void sexy_spell_entry_deactivate_language(SexySpellEntry *entry, const gchar *lang);
gboolean sexy_spell_entry_set_active_languages(SexySpellEntry *entry, GSList *langs, GError **error);
GSList* sexy_spell_entry_get_active_languages(SexySpellEntry *entry);
#ifndef G_DISABLE_DEPRECATED
gboolean sexy_spell_entry_is_checked(SexySpellEntry *entry);
#endif
gboolean sexy_spell_entry_get_checked(SexySpellEntry *entry);
void sexy_spell_entry_set_checked(SexySpellEntry *entry, gboolean checked);
void sexy_spell_entry_activate_default_languages(SexySpellEntry *entry);

G_END_DECLS

