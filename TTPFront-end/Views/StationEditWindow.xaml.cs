using System.Collections.Generic;
using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.Models;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class StationEditWindow : Window
{
    private readonly StationEditViewModel _viewModel;

    public StationEditWindow(HttpClient httpClient, List<StationInfo> existingStations,
        string? editingCode = null)
    {
        InitializeComponent();

        _viewModel = new StationEditViewModel(httpClient, existingStations, editingCode);
        DataContext = _viewModel;
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e) => Close();

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
