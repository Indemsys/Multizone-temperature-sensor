unit DashBoard;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics, System.UITypes,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs,
  System.Bluetooth, System.Bluetooth.Components, Vcl.ExtCtrls, Vcl.Menus,
  Vcl.ComCtrls, CodeSiteLogging, Vcl.StdCtrls,
  System.StrUtils, System.Types;

const
  Vuart_service_UUID: TBluetoothUUID = '{63745550-6388-46BC-BE7E-7F3919B259E4}';
  Vuart_charact_write_resp_UUID
    : TBluetoothUUID = '{63745551-6388-46BC-BE7E-7F3919B259E4}'; //
  Vuart_charact_write_UUID
    : TBluetoothUUID = '{63745552-6388-46BC-BE7E-7F3919B259E4}'; //
  Vuart_charact_read_UUID
    : TBluetoothUUID = '{63745553-6388-46BC-BE7E-7F3919B259E4}'; //

  Cmd_service_UUID: TBluetoothUUID = '{37FE7100-FB31-49E6-8389-72414CEE1597}';
  Cmd_charact_mode_set_UUID
    : TBluetoothUUID = '{37FE7101-FB31-49E6-8389-72414CEE1597}'; //

  TEST_CHAR_VAL_MAX_SZ = 20;

  MODE_VUART = 1;
  MODE_PERFT = 2;

type
  T_temperatures = array [0 .. 7] of single;

  TfrmDashBoard = class(TForm)
    MainMenu1: TMainMenu;
    StatusBar: TStatusBar;
    RefreshTimer: TTimer;
    Label1: TLabel;
    T1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    Label7: TLabel;
    Label8: TLabel;
    T2: TLabel;
    T3: TLabel;
    T4: TLabel;
    T5: TLabel;
    T6: TLabel;
    T7: TLabel;
    T8: TLabel;
    Label9: TLabel;
    T_averaged: TLabel;
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormKeyPress(Sender: TObject; var Key: Char);
    procedure FormKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure RefreshTimerTimer(Sender: TObject);
    procedure T_averagedClick(Sender: TObject);
  private
    recv_str: string;
    tx_buf: TBytes;
    f_term_unsubscribed: Boolean;
    temperature: T_temperatures;

    FS: TFormatSettings;
    samples_cnt: integer;

    procedure Callback_Terminal_CharacteristicWrite(const Sender: TObject;
      const ACharacteristic: TBluetoothGattCharacteristic;
      AGattStatus: TBluetoothGattStatus);
    procedure Callback_Terminal_CharacteristicRead(const Sender: TObject;
      const ACharacteristic: TBluetoothGattCharacteristic;
      AGattStatus: TBluetoothGattStatus);

  public
    f_term_service_activated: Boolean;
    BLEdev: TBluetoothLEDevice;
    BLE_vuart_serv: TBluetoothGattService;
    BLE_cmd_serv: TBluetoothGattService;
    BLERxChar: TBluetoothGattCharacteristic;
    BLETxChar: TBluetoothGattCharacteristic;
    BLEModeChar: TBluetoothGattCharacteristic;

    function PrepareService: Boolean;
    function Activate_vuart_service: Boolean;
  end;

var
  frmDashBoard: TfrmDashBoard;

implementation

{$R *.dfm}

uses DataModule, main;

{ -----------------------------------------------------------------------------
  Procedure: FormCreate

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmDashBoard.FormCreate(Sender: TObject);
begin
  recv_str := '';
  FS := TFormatSettings.Create;
  FS.DecimalSeparator := '.';
  samples_cnt := 0;
  // Перехватываем события приема и передачи
  with frmMain do
  begin
    Dm.BLE.OnCharacteristicWrite := Callback_Terminal_CharacteristicWrite;
    Dm.BLE.OnCharacteristicRead := Callback_Terminal_CharacteristicRead;
  end;

  SetLength(tx_buf, 1);
  frmMain.f_terminal_active := True;
end;

procedure TfrmDashBoard.FormKeyPress(Sender: TObject; var Key: Char);
begin
  //
end;

procedure TfrmDashBoard.FormKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
  //
end;

procedure TfrmDashBoard.T_averagedClick(Sender: TObject);
begin

end;

{ -----------------------------------------------------------------------------
  Procedure: PrepareService

  Arguments: None
  Result:    bolean
  ----------------------------------------------------------------------------- }
function TfrmDashBoard.PrepareService: Boolean;
begin
  CodeSite.Send('PrepareService');
  if Dm.BLE.SubscribeToCharacteristic(BLEdev, BLERxChar) then
  // Подписывемся на прием данных от сервиса
  begin
    CodeSite.Send('SubscribeToCharacteristic Ok');
  end
  else
  begin
    CodeSite.Send('SubscribeToCharacteristic Error');
  end;

  // Отправляем в аттрибут режима команду перехода в режим виртуального UART-а
  BLEModeChar.SetValueAsInt32(MODE_VUART);
  Dm.BLE.WriteCharacteristic(BLEdev, BLEModeChar);

  f_term_service_activated := True;
  RefreshTimer.Enabled := True;

  PrepareService := True;
end;

{ -----------------------------------------------------------------------------
  Procedure: RefreshTimerTimer



  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmDashBoard.RefreshTimerTimer(Sender: TObject);
begin
  if BLEdev.IsConnected then
  begin
    StatusBar.Panels[0].Text := 'Connected to ' + BLEdev.DeviceName;
    StatusBar.Color := clGreen;
  end
  else
  begin
    StatusBar.Panels[0].Text := 'Disconnected';
    StatusBar.Color := clRed;
  end;
  StatusBar.Panels[1].Text := 'Received ' + IntToStr(samples_cnt) + ' packets';
end;

{ -----------------------------------------------------------------------------
  Procedure: Activate_vuart_service

  Arguments: None
  Result:    boolean
  ----------------------------------------------------------------------------- }
function TfrmDashBoard.Activate_vuart_service: Boolean;
begin
  result := True;
  try
    PrepareService;

  except
    CodeSite.Send('Activate_vuart_service error.');
    f_term_service_activated := False;
    result := False;
    ShowMessage('Unable to connect. Try again.');
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: Callback_Terminal_CharacteristicRead

  Перехватчик чтения данных из компонета BLE в главной форме


  Arguments: const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmDashBoard.Callback_Terminal_CharacteristicRead
  (const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
  AGattStatus: TBluetoothGattStatus);
var
  data: TBytes;
  i: integer;
  k: integer;
  ch: Char;
  darr: TStringDynArray;
  aver: single;
  cnt: integer;
begin
  if ACharacteristic.uuid = BLERxChar.uuid then
  begin
    data := ACharacteristic.GetValue;

    for i := 0 to Length(data) - 1 do
    begin
      ch := Chr(data[i]);
      if ch = Chr($0A) then
      begin
        aver := 0;
        cnt := 0;
        darr := SplitString(recv_str, ',');
        try
          for k := Low(darr) to High(darr) do
          begin
            if k > 7 then
              break;
            darr[k] := Trim(darr[k]);
            temperature[k] := StrToFloat(darr[k]);
            aver := aver + temperature[k];
            cnt := cnt + 1;
          end;

        except
          on E: Exception do
            ShowMessage(darr[k] + ' on ' + IntToStr(k));
        end;
        T1.Caption := darr[0] + ' C';
        T2.Caption := darr[1] + ' C';
        T3.Caption := darr[2] + ' C';
        T4.Caption := darr[3] + ' C';
        T5.Caption := darr[4] + ' C';
        T6.Caption := darr[5] + ' C';
        T7.Caption := darr[6] + ' C';
        T8.Caption := darr[7] + ' C';
        aver := aver / cnt;
        T_averaged.Caption := FormatFloat('#.## C', aver);
        samples_cnt := samples_cnt + 1;
        recv_str := '';
      end
      else
      begin
        recv_str := recv_str + ch;
      end;

    end;

  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: Callback_Terminal_CharacteristicWrite

  Перехватчик записи данных из компонета BLE в главной форме


  Arguments: const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmDashBoard.Callback_Terminal_CharacteristicWrite
  (const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
  AGattStatus: TBluetoothGattStatus);
begin

end;

{ -----------------------------------------------------------------------------
  Procedure: FormClose

  Arguments: Sender: TObject; var Action: TCloseAction
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TfrmDashBoard.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  frmMain.f_terminal_active := False;

  RefreshTimer.Enabled := False;

  with frmMain do
  begin
    Dm.BLE.OnCharacteristicWrite := Nil;
    Dm.BLE.OnCharacteristicRead := Nil;
  end;

  if f_term_service_activated then
  begin
    // Отправляем в аттрибут режима команду перехода в режим теста производительности
    BLEModeChar.SetValueAsInt32(MODE_PERFT);
    try
      if Dm.BLE.WriteCharacteristic(BLEdev, BLEModeChar) then
      begin
        CodeSite.Send('WriteCharacteristic Ok');
      end
      else
      begin
        CodeSite.Send('WriteCharacteristic Error');
      end;
    except
    end;
    f_term_service_activated := False;
  end;

  Action := caFree;
  frmMain.Visible := True;

end;

end.
