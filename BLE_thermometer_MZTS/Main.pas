unit Main;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls, Vcl.ButtonGroup, Vcl.ComCtrls, Vcl.ExtCtrls, Vcl.Buttons,
  WinRTBluetoothLE;

const
  DISCOVER_TIME_CONST = 5000; // Время в милисекундах на процедуру обнаружения устройств
  OPENING_TIME_CONST = 10000; // Время в милисекундах на процедуру открытия устройства
  PROGRESS_BAR_STEPS = 100;

const
  WM_NEXT_DEV_MESSAGES = WM_USER + 1;
  WM_START_DISCOVER_SERVICES_MESSAGES = WM_USER + 2;

type
  TfrmMain = class(TForm)
    btgroupDevices: TButtonGroup;
    lblState: TLabel;
    ProgressBar: TProgressBar;
    DiscoverTimer: TTimer;
    Panel1: TPanel;
    Panel2: TPanel;
    btnDiscover: TSpeedButton;
    OpenTimer: TTimer;
    btnStop: TSpeedButton;
    procedure FormShow(Sender: TObject);
    procedure DiscoverTimerTimer(Sender: TObject);
    procedure btnDiscoverClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure btgroupDevicesButtonClicked(Sender: TObject; Index: Integer);
    procedure OpenTimerTimer(Sender: TObject);
    procedure btnStopClick(Sender: TObject);
  private
    discover_timeout: Integer;
    procedure PrepareVisualsBeforeDiscover;
    procedure BeginDiscover;
    function OpenTerminal(Index: Integer): boolean;
    procedure PrepareVisualsBeforeOpening;
  public
    f_terminal_active: boolean;
    procedure RestoreVisualsAfterDiscover;
    procedure DeviceOpened(Index: Integer);

  end;

var
  frmMain: TfrmMain;

implementation

{$R *.dfm}

uses DataModule, DashBoard;

{ -----------------------------------------------------------------------------
  Procedure: FormCreate

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.FormCreate(Sender: TObject);
begin
  discover_timeout := DISCOVER_TIME_CONST;
  BeginDiscover;
end;

{ -----------------------------------------------------------------------------
  Procedure: FormShow

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.FormShow(Sender: TObject);
begin
  //BeginDiscover;
end;

{ -----------------------------------------------------------------------------
  Procedure: OpenTerminal

  Arguments: BLEDev: TBluetoothLEDevice; Index: Integer
  Result:    None
  ----------------------------------------------------------------------------- }
function TfrmMain.OpenTerminal(Index: Integer): boolean;
var
  BLEDev: TWinRTBluetoothLEDevice;
begin
  BLEDev := Dm.BLE_devs[Index].device;
  // В Win10 команда BLEDev.Connect ничего не делает,
  // устройство подключается только после последовательного выполнения функций BLE.CurrentManager.StartDiscovery и BLEDev.DiscoverServices
  // StartDiscovery отключит все устройства, а DiscoverServices подключит устройство для кторого выполняется  функция DiscoverServices
  // Сначало надо  убедится что устройство подключено
  frmDashBoard := TfrmDashBoard.Create(Self);
  Self.Visible := False;
  try
    frmDashBoard.BLEDev := BLEDev;
    frmDashBoard.BLE_vuart_serv := Dm.BLE_devs[Index].vuart_service;
    frmDashBoard.BLE_cmd_serv := Dm.BLE_devs[Index].cmd_service;
    frmDashBoard.BLERxChar := Dm.BLE_devs[Index].read_characteristic;
    frmDashBoard.BLETxChar := Dm.BLE_devs[Index].write_characteristic;
    frmDashBoard.BLEModeChar := Dm.BLE_devs[Index].cmd_characteristic;
    if frmDashBoard.Activate_vuart_service = False then
    begin
      frmDashBoard.Close;
      result := False;
    end
    else
    begin
      frmDashBoard.Show;
      // Выключаем все остальные устройства
      // Dm.DisconnectAllExceptSelected(Index);
      result := True;
    end;
  except
    frmDashBoard.Close;
    Self.Visible := True;
    result := False;
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: btgroupDevicesButtonClicked

  Вызывается по нажатию кнопки открытия дивайса

  Arguments: Sender: TObject; Index: Integer
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.btgroupDevicesButtonClicked(Sender: TObject; Index: Integer);
var
  dev_indx: Integer;
begin
  dev_indx := btgroupDevices.Items[Index].Hint.ToInteger;
  if Dm.BLE_devs[dev_indx].device.IsConnected = True then
  begin
    // Закрыть все остальные открытые устройсва
    // Dm.DisconnectAllExceptSelected(dev_indx);
    if OpenTerminal(dev_indx) = True then
      Exit;
  end;

  // dm.DisconnectDevice(dev_indx);
  Dm.StartDeviceOpening(dev_indx, OPENING_TIME_CONST);
  PrepareVisualsBeforeOpening;
end;

{ -----------------------------------------------------------------------------
  Procedure: DeviceOpened
  Вызывается если дивайс удалось открыть

  Arguments: Index: Integer
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.DeviceOpened(Index: Integer);
begin
  RestoreVisualsAfterDiscover;

  if Dm.BLE_devs[Index].device.IsConnected = False then
  begin
    ShowMessage('Divice "' + Dm.BLE_devs[Index].device.Address + '" disconnected. Unable to open terminal.');
    Exit;
  end;

  OpenTerminal(Index);
end;

{ -----------------------------------------------------------------------------
  Procedure: btnDiscoverClick

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.btnDiscoverClick(Sender: TObject);
begin
  BeginDiscover;
end;

{ -----------------------------------------------------------------------------
  Procedure: btnStopClick


  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.btnStopClick(Sender: TObject);
begin
  RestoreVisualsAfterDiscover;
  Dm.EndDiscoveringDevices;
  Dm.DiscoverDevicesServices;
end;

{ -----------------------------------------------------------------------------
  Procedure: BeginDiscover

  Arguments: None
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.BeginDiscover;
begin

  Dm.DiscoveringProcedure(discover_timeout);
  PrepareVisualsBeforeDiscover;
end;

{ -----------------------------------------------------------------------------
  Procedure: TimerTimer

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.DiscoverTimerTimer(Sender: TObject);
begin
  ProgressBar.Position := ProgressBar.Position + 1;
  if ProgressBar.Position >= ProgressBar.Max then
  begin
    RestoreVisualsAfterDiscover;
    Dm.EndDiscoveringDevices;
    Dm.DiscoverDevicesServices;
  end;

end;

{ -----------------------------------------------------------------------------
  Procedure: OpenTimerTimer

  Вызывается если открытие дивайса не произошло за заданное время

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.OpenTimerTimer(Sender: TObject);
begin
  ProgressBar.Position := ProgressBar.Position + 1;
  if ProgressBar.Position >= ProgressBar.Max then
  begin
    RestoreVisualsAfterDiscover;
    Dm.EndDiscoveringDevices;
    ShowMessage('Divice can''t be opened during desired time!');
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: StopDiscover

  Arguments: None
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.RestoreVisualsAfterDiscover;
begin
  lblState.Caption := 'Press to open terminal';
  ProgressBar.Visible := False;
  DiscoverTimer.Enabled := False;
  OpenTimer.Enabled := False;
  btnDiscover.Visible := True;
  btnStop.Visible := False;
  btgroupDevices.Enabled := True;
end;

{ -----------------------------------------------------------------------------
  Procedure: StartDiscover

  Arguments: None
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.PrepareVisualsBeforeOpening;
begin
  btgroupDevices.Enabled := False;
  btnDiscover.Visible := False;
  lblState.Caption := 'Opening...';
  ProgressBar.Position := 0;
  ProgressBar.Min := 0;
  ProgressBar.Max := PROGRESS_BAR_STEPS;
  ProgressBar.Visible := True;
  OpenTimer.Interval := Round(OPENING_TIME_CONST / PROGRESS_BAR_STEPS);
  OpenTimer.Enabled := True;
end;

{ -----------------------------------------------------------------------------
  Procedure: StartDiscover

  Arguments: None
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmMain.PrepareVisualsBeforeDiscover;
begin
  btgroupDevices.Enabled := False;
  btnDiscover.Visible := False;
  btnStop.Visible := True;
  lblState.Caption := 'Scaning...';
  ProgressBar.Position := 0;
  ProgressBar.Min := 0;
  ProgressBar.Max := PROGRESS_BAR_STEPS;
  ProgressBar.Visible := True;
  DiscoverTimer.Interval := Round(discover_timeout / PROGRESS_BAR_STEPS);
  DiscoverTimer.Enabled := True;
end;

end.
