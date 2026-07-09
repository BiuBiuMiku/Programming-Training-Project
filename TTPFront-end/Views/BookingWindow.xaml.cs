using System.Collections.Generic;
using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using TTPFrontEnd.Models;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class BookingWindow : Window
{
    private readonly BookingViewModel _viewModel;

    /// <summary>直达购票</summary>
    public BookingWindow(HttpClient httpClient, string trainNo,
        string fromStation, string toStation, string date,
        List<SeatTypeInfo> seatTypes, List<SavedPassenger> savedPassengers,
        int totalDistanceKm)
    {
        InitializeComponent();

        _viewModel = new BookingViewModel(httpClient, trainNo,
            fromStation, toStation, date, seatTypes, savedPassengers,
            totalDistanceKm);
        DataContext = _viewModel;
    }

    /// <summary>中转购票</summary>
    public BookingWindow(HttpClient httpClient, TransferRoute route,
        string date, List<SavedPassenger> savedPassengers)
    {
        InitializeComponent();

        _viewModel = new BookingViewModel(httpClient, route, date, savedPassengers);
        DataContext = _viewModel;
    }

    private void PassengerCheck_Click(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement fe && fe.Tag is SelectablePassenger sp)
        {
            _viewModel.TogglePassenger(sp);
        }
    }

    private void SeatType_Click(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement fe && fe.Tag is SeatTypeInfo st)
            _viewModel.SelectSeatTypeCommand.Execute(st);
    }

    private void TransferSeatType_Click(object sender, RoutedEventArgs e)
    {
        if (sender is not FrameworkElement fe || fe.Tag is not SeatTypeInfo st) return;
        var opt = FindParentWithDataContext<TransferLegOption>(fe);
        if (opt != null) _viewModel.SetLegSeatType(opt, st);
    }

    private static T? FindParentWithDataContext<T>(DependencyObject child) where T : class
    {
        var dep = VisualTreeHelper.GetParent(child);
        while (dep != null)
        {
            if (dep is FrameworkElement pfe && pfe.DataContext is T t) return t;
            dep = VisualTreeHelper.GetParent(dep);
        }
        return null;
    }

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
