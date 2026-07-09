using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.Models;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class RefundWindow : Window
{
    private readonly RefundViewModel _viewModel;

    public RefundWindow(HttpClient httpClient, OrderInfo order)
    {
        InitializeComponent();

        _viewModel = new RefundViewModel(httpClient, order);
        DataContext = _viewModel;

        Loaded += async (s, e) => { try { await _viewModel.LoadFeeAsync(); } catch (System.Exception ex) { Utils.Logger.Error($"RefundWindow Loaded 异常: {ex}"); } };
    }

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
