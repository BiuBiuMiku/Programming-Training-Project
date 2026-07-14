using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class RegisterWindow : Window
{
    private readonly RegisterViewModel _viewModel;

    public RegisterWindow()
    {
        InitializeComponent();

        var httpClient = new HttpClient
        {
            BaseAddress = new System.Uri("http://localhost:8080"),
            Timeout = System.TimeSpan.FromSeconds(10)
        };

        _viewModel = new RegisterViewModel(httpClient);
        DataContext = _viewModel;

        Loaded += (s, e) => PhoneBox.Focus();
        Closed += (s, e) => _viewModel.Dispose();
    }

    /// <summary>
    /// 密码框的值同步到 ViewModel.Password
    /// </summary>
    private void PasswordBox_PasswordChanged(object sender, RoutedEventArgs e)
    {
        _viewModel.Password = PwdBox.Password;
    }

    /// <summary>
    /// 确认密码框的值同步到 ViewModel.ConfirmPassword
    /// </summary>
    private void ConfirmPasswordBox_PasswordChanged(object sender, RoutedEventArgs e)
    {
        _viewModel.ConfirmPassword = ConfirmPwdBox.Password;
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e)
    {
        Application.Current.Shutdown();
    }

    /// <summary>
    /// 允许拖拽无边框窗口
    /// </summary>
    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
