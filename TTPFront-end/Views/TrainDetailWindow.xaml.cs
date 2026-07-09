using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class TrainDetailWindow : Window
{
    private readonly TrainDetailViewModel _viewModel;

    public TrainDetailWindow(HttpClient httpClient, string trainNo, string date)
    {
        InitializeComponent();

        _viewModel = new TrainDetailViewModel(httpClient, trainNo, date);
        DataContext = _viewModel;

        Loaded += async (s, e) =>
        {
            try { await _viewModel.LoadDetailAsync(); }
            catch (System.Exception ex) { Utils.Logger.Error($"TrainDetail Loaded 异常: {ex}"); }
        };
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e)
    {
        Close();
    }

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
