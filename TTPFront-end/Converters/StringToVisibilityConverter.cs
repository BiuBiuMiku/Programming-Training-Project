using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace TTPFrontEnd.Converters;

/// 字符串非空时显示，为空时折叠
public class StringToVisibilityConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if(string.IsNullOrEmpty(value as string)) return Visibility.Collapsed;
        return Visibility.Visible;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotSupportedException();
    }
}
