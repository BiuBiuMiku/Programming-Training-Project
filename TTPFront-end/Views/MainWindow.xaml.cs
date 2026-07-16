using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.Models;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class MainWindow : Window
{
    private readonly MainViewModel _viewModel;

    public MainWindow(string username, string role)
    {
        InitializeComponent();

        var httpClient = new HttpClient
        {
            BaseAddress = new System.Uri("http://localhost:8080"),
            Timeout = System.TimeSpan.FromSeconds(10)
        };

        var savedToken = TokenStorage.LoadToken();
        if (savedToken != null)
        {
            httpClient.DefaultRequestHeaders.Authorization =
                new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", savedToken);
        }

        _viewModel = new MainViewModel(httpClient, username, role);
        DataContext = _viewModel;

        // 页面切换时自动加载数据
        _viewModel.PropertyChanged += (s, e) =>
        {
            if (e.PropertyName != nameof(MainViewModel.CurrentPage)) return;

            try
            {
                if (_viewModel.CurrentPage == "后台管理")
                    _ = _viewModel.AdminPanel.LoadAsync();
                else if (_viewModel.CurrentPage == "我的订单")
                    _ = _viewModel.OrderList.LoadAsync();
            }
            catch (Exception ex) { Utils.Logger.Error($"MainWindow 页面切换异常: {ex}"); }
        };

        Loaded += (s, e) => { try { _viewModel.OnLoaded(); } catch (Exception ex) { Utils.Logger.Error($"MainWindow Loaded 异常: {ex}"); } };
    }

    private void TrainList_MouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (_viewModel.TrainQuery.SelectedRoute != null &&
            !_viewModel.TrainQuery.SelectedRoute.IsTransfer)
        {
            _viewModel.TrainQuery.ViewDetailCommand.Execute(
                _viewModel.TrainQuery.SelectedRoute);
        }
    }

    // ============ 后台管理 ============

    private void AdminRefresh_Click(object sender, RoutedEventArgs e)
    {
        try { _ = _viewModel.AdminPanel.LoadAsync(); }
        catch (Exception ex) { Utils.Logger.Error($"AdminRefresh 异常: {ex}"); }
    }

    private void AdminAddTrain_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var stations = _viewModel.AdminPanel.Stations?.ToList()
                ?? new List<StationInfo>();

            var editWindow = new TrainEditWindow(
                _viewModel.SharedHttpClient, stations, null);
            editWindow.Owner = this;
            editWindow.WindowStartupLocation = WindowStartupLocation.CenterOwner;
            editWindow.ShowDialog();
        }
        catch (System.Exception ex)
        {
            MessageBox.Show($"打开编辑窗口失败:\n{ex}", "错误",
                MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private void AdminAddStation_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var stations = _viewModel.AdminPanel.Stations?.ToList()
                ?? new List<StationInfo>();

            var window = new StationEditWindow(
                _viewModel.SharedHttpClient, stations);
            window.Owner = this;
            window.WindowStartupLocation = WindowStartupLocation.CenterOwner;
            var result = window.ShowDialog();

            if (result == true)
                _ = _viewModel.AdminPanel.LoadAsync();
        }
        catch (System.Exception ex)
        {
            MessageBox.Show($"打开窗口失败:\n{ex}", "错误",
                MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    // ============ 我的订单 ============

    private void OrderRefresh_Click(object sender, RoutedEventArgs e)
    {
        try { _ = _viewModel.OrderList.LoadAsync(); }
        catch (Exception ex) { Utils.Logger.Error($"OrderRefresh 异常: {ex}"); }
    }
}
