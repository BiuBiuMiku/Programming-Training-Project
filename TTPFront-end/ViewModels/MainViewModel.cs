using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Linq;
using System.Net.Http;
using System.Windows;
using TTPFrontEnd.Models;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class MainViewModel : ObservableObject
{
    public HttpClient SharedHttpClient { get; }
    public TrainQueryViewModel TrainQuery { get; }
    public OrderListViewModel OrderList { get; }
    public AdminPanelViewModel AdminPanel { get; }
    public ProfileViewModel Profile { get; }

    public MainViewModel(HttpClient httpClient, string username, string role)
    {
        SharedHttpClient = httpClient;
        TrainQuery = new TrainQueryViewModel(httpClient);
        OrderList = new OrderListViewModel(httpClient);
        AdminPanel = new AdminPanelViewModel(httpClient);
        Profile = new ProfileViewModel(username, role);
        Profile.PassengersChanged += () => RefreshPassengers();
        Username = username;
        Role = role;
        WelcomeText = Role == "admin" ? $"管理员 {username}" : $"欢迎，{username}";
        IsAdmin = Role == "admin";
        CurrentPage = "车次查询";
    }

    [ObservableProperty] private string _currentPage = string.Empty;
    [ObservableProperty] private string _welcomeText = string.Empty;
    [ObservableProperty] private string _username = string.Empty;
    [ObservableProperty] private string _role = "user";
    [ObservableProperty] private bool _isAdmin = false;

    public void RefreshPassengers() { TrainQuery.SavedPassengers = Profile.Passengers.ToList(); }
    public async void OnLoaded() { RefreshPassengers(); await TrainQuery.LoadStationsAsync(); }

    [RelayCommand] private void Navigate(string page) { CurrentPage = page; }
    [RelayCommand]
    private void Logout()
    {
        TokenStorage.ClearToken();
        var w = new Views.LoginWindow(); w.Show();
        Application.Current.MainWindow?.Close();
    }
}