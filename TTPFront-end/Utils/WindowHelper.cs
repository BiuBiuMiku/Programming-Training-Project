using System.Windows;

namespace TTPFrontEnd.Utils;

public static class WindowHelper
{
    public static void SafeSetOwner(Window w)
    {
        var owner = Application.Current.MainWindow;
        if (owner != null && !ReferenceEquals(owner, w) && owner.IsLoaded)
        {
            w.Owner = owner;
            w.WindowStartupLocation = WindowStartupLocation.CenterOwner;
        }
        else
        {
            w.WindowStartupLocation = WindowStartupLocation.CenterScreen;
        }
    }
}
