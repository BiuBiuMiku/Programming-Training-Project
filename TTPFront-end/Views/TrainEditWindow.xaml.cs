using System.Collections.Generic;
using System.Collections.Specialized;
using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using TTPFrontEnd.Models;
using TTPFrontEnd.ViewModels;

namespace TTPFrontEnd.Views;

public partial class TrainEditWindow : Window
{
    private readonly TrainEditViewModel _viewModel;

    public TrainEditWindow(HttpClient httpClient, List<StationInfo> stations,
        string? existingTrainNo)
    {
        InitializeComponent();

        _viewModel = new TrainEditViewModel(httpClient, stations, existingTrainNo);
        DataContext = _viewModel;

        // 每当经停站列表变动（新增/删除），自动为新站绑定距离重算回调
        _viewModel.Stops.CollectionChanged += OnStopsCollectionChanged;
        foreach (var s in _viewModel.Stops)
            s.OnStationChangedExternal = () => _viewModel.OnStopStationChanged();

        Loaded += async (s, e) =>
        {
            try
            {
                if (!_viewModel.IsNewMode)
                    await _viewModel.LoadExistingAsync();
            }
            catch (System.Exception ex) { Utils.Logger.Error($"TrainEditWindow Loaded 异常: {ex}"); }
        };
    }

    private void OnStopsCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.NewItems != null)
        {
            foreach (EditableStop s in e.NewItems)
                s.OnStationChangedExternal = () => _viewModel.OnStopStationChanged();
        }
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e) => Close();

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        base.OnMouseLeftButtonDown(e);
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }
}
