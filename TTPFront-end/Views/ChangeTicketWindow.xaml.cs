using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.Models;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class ChangeTicketWindow : Window
{
    private readonly ChangeTicketViewModel _viewModel;

    public ChangeTicketWindow(HttpClient httpClient, OrderInfo order)
    {
        InitializeComponent();

        _viewModel = new ChangeTicketViewModel(httpClient, order);
        DataContext = _viewModel;

        Loaded += async (s, e) => { try { await _viewModel.LoadOptionsAsync(); } catch (System.Exception ex) { Utils.Logger.Error($"ChangeTicketWindow Loaded 异常: {ex}"); } };
    }

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
