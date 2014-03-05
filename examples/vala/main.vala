using Gtk;
using Sexy;

int main (string[] args)
{
    Gtk.init (ref args);

    var window = new Window ();
    window.title = "SexySpellEntryTest";
    window.window_position = WindowPosition.CENTER;
    window.destroy.connect (Gtk.main_quit);

    var entry = new SpellEntry();
    entry.set_text("egnlish");

    window.add (entry);
    window.show_all ();

    Gtk.main ();
    return 0;
}
