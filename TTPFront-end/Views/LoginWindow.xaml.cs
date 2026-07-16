using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class LoginWindow : Window
{
    private readonly HttpClient _httpClient;
    private readonly LoginViewModel _viewModel;

    public LoginWindow()
    {
        InitializeComponent();

        _httpClient = new HttpClient
        {
            BaseAddress = new System.Uri("http://localhost:8080"),
            Timeout = System.TimeSpan.FromSeconds(10)
        };

        _viewModel = new LoginViewModel(_httpClient);
        DataContext = _viewModel;

        Loaded += OnLoaded;
    }

    // ============ 密码框 → ViewModel 同步 ============

    private void PasswordBox_PasswordChanged(object sender, RoutedEventArgs e)
    {
        if (DataContext is LoginViewModel vm)
            vm.Password = PasswordBox.Password;
    }

    // ============ 密码明文/密文切换 ============

    private void TogglePassword_Click(object sender, RoutedEventArgs e)
    {
        if (PasswordBox.Visibility == Visibility.Visible)
        {
            // 切换到明文：把 PasswordBox 的值拷到 TextBox，然后切换显示
            PasswordTextBox.Text = PasswordBox.Password;
            PasswordBox.Visibility = Visibility.Collapsed;
            PasswordTextBox.Visibility = Visibility.Visible;
            TogglePasswordBtn.Content = "🔒";
            PasswordTextBox.Focus();
            PasswordTextBox.CaretIndex = PasswordTextBox.Text.Length;
        }
        else
        {
            // 切换到密文：把 TextBox 的值拷回 PasswordBox
            PasswordBox.Password = PasswordTextBox.Text;
            PasswordTextBox.Visibility = Visibility.Collapsed;
            PasswordBox.Visibility = Visibility.Visible;
            TogglePasswordBtn.Content = "👁";
            PasswordBox.Focus();
        }
    }

    // ============ 键盘快捷键 ============

    private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter && _viewModel.LoginCommand.CanExecute(null))
        {
            e.Handled = true;
            _viewModel.LoginCommand.Execute(null);
        }
    }

    // ============ 窗口初始化 ============

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        try
        {
            var savedToken = TokenStorage.LoadToken();
            if (savedToken != null)
            {
                _httpClient.DefaultRequestHeaders.Authorization =
                    new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", savedToken);
            }
        }
        catch (System.Exception ex) { Utils.Logger.Warn($"LoginWindow 加载 Token 异常: {ex.Message}"); }

        UsernameBox.Focus();
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e)
    {
        Application.Current.Shutdown();
    }

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
