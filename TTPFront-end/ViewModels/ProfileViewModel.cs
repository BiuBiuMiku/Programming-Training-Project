using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using TTPFrontEnd.Models;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class ProfileViewModel : ObservableObject
{
    public const int MaxPassengers = 5;

    /// <summary>乘客变更后触发，供 MainViewModel 刷新购票窗口的乘客列表</summary>
    public event Action? PassengersChanged;

    public ProfileViewModel(string username, string role)
    {
        Username = username; Role = role; WelcomeTitle = role == "admin" ? $"管理员 {username}" : username;
        try
        {
            var saved = PassengerStorage.Load(username);
            foreach (var p in saved) Passengers.Add(p);
            if (saved.Count > 0) Logger.Info($"已加载 {username} 的 {saved.Count} 个常用乘车人");
        }
        catch (Exception ex) { Logger.Error($"加载乘车人失败 [{username}]: {ex}"); }
    }

    [ObservableProperty] private string _username; [ObservableProperty] private string _role; [ObservableProperty] private string _welcomeTitle;
    [ObservableProperty] private ObservableCollection<SavedPassenger> _passengers = new();
    [ObservableProperty] private SavedPassenger? _selectedPassenger;
    [ObservableProperty] private bool _isEditing = false; [ObservableProperty] private bool _isNewPassenger = false;
    [ObservableProperty] private string _editRealName = string.Empty; [ObservableProperty] private string _editIdCard = string.Empty;
    [ObservableProperty] private string _editPhone = string.Empty; [ObservableProperty] private string? _editingId;
    [ObservableProperty] private string _errorMessage = string.Empty; [ObservableProperty] private string _successMessage = string.Empty;

    public bool CanAdd => Passengers.Count < MaxPassengers && !IsEditing;
    public bool IsListEmpty => Passengers.Count == 0;

    private bool CanSave => !string.IsNullOrWhiteSpace(EditRealName) && !string.IsNullOrWhiteSpace(EditIdCard) && EditIdCard.Length == 18;
    partial void OnEditRealNameChanged(string value) => SavePassengerCommand.NotifyCanExecuteChanged();
    partial void OnEditIdCardChanged(string value) => SavePassengerCommand.NotifyCanExecuteChanged();

    [RelayCommand] private void StartAdd() { if (Passengers.Count >= MaxPassengers) return; EditRealName = string.Empty; EditIdCard = string.Empty; EditPhone = string.Empty; EditingId = null; IsNewPassenger = true; IsEditing = true; ErrorMessage = string.Empty; SuccessMessage = string.Empty; }
    [RelayCommand] private void StartEdit(SavedPassenger? p) { if (p == null) return; EditRealName = p.RealName; EditIdCard = p.IdCard; EditPhone = p.Phone; EditingId = p.Id; IsNewPassenger = false; IsEditing = true; ErrorMessage = string.Empty; SuccessMessage = string.Empty; }
    [RelayCommand] private void CancelEdit() { IsEditing = false; ErrorMessage = string.Empty; }

    [RelayCommand(CanExecute = nameof(CanSave))]
    private void SavePassenger()
    {
        if (Passengers.Any(p => p.IdCard == EditIdCard && p.Id != EditingId)) { ErrorMessage = "该身份证号已存在"; return; }
        if (IsNewPassenger) { Passengers.Add(new SavedPassenger { RealName = EditRealName, IdCard = EditIdCard, Phone = EditPhone }); SuccessMessage = $"已添加 {EditRealName}，还可添加 {MaxPassengers - Passengers.Count} 人"; }
        else { var ex = Passengers.FirstOrDefault(p => p.Id == EditingId); if (ex != null) { ex.RealName = EditRealName; ex.IdCard = EditIdCard; ex.Phone = EditPhone; var idx = Passengers.IndexOf(ex); Passengers[idx] = ex; } SuccessMessage = "身份信息已更新"; }
        IsEditing = false; Persist(); OnPropertyChanged(nameof(CanAdd)); OnPropertyChanged(nameof(IsListEmpty));
        PassengersChanged?.Invoke();
    }

    [RelayCommand] private void DeletePassenger(SavedPassenger? p) { if (p == null) return; Passengers.Remove(p); Persist(); OnPropertyChanged(nameof(CanAdd)); OnPropertyChanged(nameof(IsListEmpty)); SuccessMessage = "已删除"; PassengersChanged?.Invoke(); }
    private void Persist() { try { PassengerStorage.Save(Username, Passengers.ToList()); } catch (Exception ex) { Logger.Error($"保存乘车人失败 [{Username}]: {ex}"); } }
}