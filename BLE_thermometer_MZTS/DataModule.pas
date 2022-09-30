unit DataModule;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Classes,   VCL.Dialogs, VCL.AppEvnts,
  System.Types, VCL.Graphics, CodeSiteLogging,
  VCL.ButtonGroup,  System.Bluetooth,  WinRTBluetoothLE, WinRTBluetoothLEComponent;

const
  Vuart_service_UUID: TBluetoothUUID = '{63745550-6388-46BC-BE7E-7F3919B259E4}';
  Vuart_charact_write_resp_UUID: TBluetoothUUID = '{63745551-6388-46BC-BE7E-7F3919B259E4}'; //
  Vuart_charact_write_UUID: TBluetoothUUID = '{63745552-6388-46BC-BE7E-7F3919B259E4}'; //
  Vuart_charact_read_UUID: TBluetoothUUID = '{63745553-6388-46BC-BE7E-7F3919B259E4}'; //

  Cmd_service_UUID: TBluetoothUUID = '{37FE7100-FB31-49E6-8389-72414CEE1597}';
  Cmd_charact_mode_set_UUID: TBluetoothUUID = '{37FE7101-FB31-49E6-8389-72414CEE1597}'; //

  TEST_CHAR_VAL_MAX_SZ = 20;

  MODE_VUART = 1;
  MODE_PERFT = 2;

type
  T_BLE_dev = record
    id: integer;
    name: string;
    address: string;
    rssi: string;
    discovered: boolean;
    device: TWinRTBluetoothLEDevice;
    vuart_service: TBluetoothGattService;
    cmd_service: TBluetoothGattService;
    write_characteristic: TBluetoothGattCharacteristic;
    read_characteristic: TBluetoothGattCharacteristic;
    cmd_characteristic: TBluetoothGattCharacteristic;
  end;

type
  T_discovering_services_automat = record
    active: boolean;
    dev_idx: integer;

  end;

type
  TDm = class(TDataModule)
    procedure DataModuleCreate(Sender: TObject);
    procedure DataModuleDestroy(Sender: TObject);

    procedure BLE_OnDiscoverLEDevice(const Sender: TObject; const ADevice: TWinRTBluetoothLEDevice; rssi: integer; const ScanResponse: TScanResponse);
    procedure BLE_OnEndDiscoverDevices(const Sender: TObject; const ADeviceList: TBluetoothLEDeviceList);
    procedure BLE_OnConnectedDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);
    procedure BLE_OnConnect(Sender: TObject);
    procedure BLE_OnReadRSSI(const Sender: TObject; ARssiValue: integer; AGattStatus: TBluetoothGattStatus);

    procedure BLE_OnServicesDiscovered(const Sender: TObject; const AServiceList: TBluetoothGattServiceList);
    procedure BLE_OnEndDiscoverServices(const Sender: TObject; const AServiceList: TBluetoothGattServiceList);

    procedure BLE_OnDescriptorRead(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor; AGattStatus: TBluetoothGattStatus);
    procedure BLE_OnDescriptorWrite(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor; AGattStatus: TBluetoothGattStatus);
    procedure BLE_OnCharacteristicSubscribed(const Sender: TObject; const AClientId: string; const ACharacteristic: TBluetoothGattCharacteristic);
    procedure BLE_OnCharacteristicUnSubscribed(const Sender: TObject; const AClientId: string; const ACharacteristic: TBluetoothGattCharacteristic);
    procedure BLE_OnCharacteristicWriteRequest(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
      var AGattStatus: TBluetoothGattStatus; const AValue: TByteDynArray);
    procedure BLE_OnCharacteristicWriteprocedure(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
    procedure BLE_OnCharacteristicReadRequest(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
      var AGattStatus: TBluetoothGattStatus);
    procedure BLE_OnCharacteristicRead(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
    procedure BLE_OnCharacteristicWrite(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
    procedure BLE_OnReliableWriteCompleted(const Sender: TObject; AGattStatus: TBluetoothGattStatus);

    procedure BLE_OnServiceAdded(const Sender: TObject; const AService: TBluetoothGattService; const AGattStatus: TBluetoothGattStatus);

    procedure BLE_OnDisconnect(Sender: TObject);
    procedure BLE_OnDisconnectDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);

  private
    // FBLEAdvertisementReceivedDelegate: TypedEventHandler_2__IBluetoothLEAdvertisementWatcher__IBluetoothLEAdvertisementReceivedEventArgs;

    LList: TBluetoothUUIDsList;
    f_device_opening: boolean; // Флаг процесса открытия выбранного устройства

    function SearchInDiscoveredDevices(const ADevice: TBluetoothLEDevice; rssi: integer): boolean;
    function FindCharacteristic(const uuid: TBluetoothUUID; BLEService: TBluetoothGattService; var BLEChar: TBluetoothGattCharacteristic): boolean;
    procedure ShowDiscoveredDivices;
    procedure AddButtontoForm(DeviceName: string; rssi: string; address: string; indx: integer);

  public
    BLE: TWinRTBluetoothLE;
    BLEman: TWinRTBluetoothLEManager;
    BLEAdapt: TWinRTBluetoothLEAdapter;
    BLE_devs: array of T_BLE_dev;
    target_dev_indx: integer;
    procedure StartDevicesDiscovering(discovering_time: integer);
    procedure StartDeviceOpening(indx: integer; discovering_time: integer);
    procedure DiscoveringProcedure(discovering_time: integer);
    procedure EndDiscoveringDevices;
    procedure DisconnectAllExceptSelected(indx: integer);
    function DiscoverDevicesServices: boolean;
    procedure DisconnectDevice(indx: integer);
    procedure DisconnectAll;
  end;

var
  Dm: TDm;

implementation

{ %CLASSGROUP 'Vcl.Controls.TControl' }

uses Main, DashBoard;

{$R *.dfm}
{ TDm }

{ -----------------------------------------------------------------------------
  Procedure: DataModuleCreate

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.DataModuleCreate(Sender: TObject);
begin
  CodeSite.Send('DataModuleCreate');
  BLE:= TWinRTBluetoothLE.Create(Nil);
  BLE.OnCharacteristicReadRequest := BLE_OnCharacteristicReadRequest;
  BLE.OnCharacteristicWriteRequest := BLE_OnCharacteristicWriteRequest;
  BLE.OnCharacteristicSubscribed := BLE_OnCharacteristicSubscribed;
  BLE.OnCharacteristicUnSubscribed := BLE_OnCharacteristicUnSubscribed;
  BLE.OnCharacteristicWrite := BLE_OnCharacteristicWrite; //
  BLE.OnCharacteristicRead := BLE_OnCharacteristicRead; //

  BLE.OnConnectedDevice := BLE_OnConnectedDevice;
  BLE.OnDescriptorRead := BLE_OnDescriptorRead;
  BLE.OnDescriptorWrite := BLE_OnDescriptorWrite;
  BLE.OnConnect := BLE_OnConnect;
  BLE.OnDisconnect := BLE_OnDisconnect;
  BLE.OnDisconnectDevice := BLE_OnDisconnectDevice;
  BLE.OnConnectedDevice := BLE_OnConnectedDevice; //
  BLE.OnDiscoverLEDevice := BLE_OnDiscoverLEDevice;
  BLE.OnServicesDiscovered := BLE_OnServicesDiscovered;
  BLE.OnEndDiscoverDevices := BLE_OnEndDiscoverDevices;
  BLE.OnEndDiscoverServices := BLE_OnEndDiscoverServices;
  BLE.OnReliableWriteCompleted := BLE_OnReliableWriteCompleted; //
  BLE.OnServiceAdded := BLE_OnServiceAdded; //
  BLE.OnReadRSSI := BLE_OnReadRSSI; //
  BLE.Enabled := True;

  BLEMan := BLE.GetCurrentManager as TWinRTBluetoothLEManager;
  BLEAdapt := BLE.GetCurrentAdapter as TWinRtBluetoothLEAdapter;



  CodeSite.Send('BLEman ClassName', BLEman.ClassName);
  CodeSite.Send('BLEman EnableBluetooth', BLEman.EnableBluetooth);
  CodeSite.Send('BLEAdapt ClassName', BLEAdapt.ClassName);
  CodeSite.Send('BLEAdapt Address', BLEAdapt.address);
  CodeSite.Send('BLEAdapt AdapterName', BLEAdapt.AdapterName);

  // FBLEAdvertisementReceivedDelegate := TBLEAdvertisementReceivedEventHandler.Create(Self);
end;

{ -----------------------------------------------------------------------------
  Procedure: StartDiscovering

  Arguments: discovering_time: integer
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.DiscoveringProcedure(discovering_time: integer);
var

  uuid: TBluetoothUUID;
  AList: array of TBluetoothUUID;
  i: integer;
begin
  if LList = Nil then
    LList := TBluetoothUUIDsList.Create;

  AList := [Vuart_service_UUID];
  for uuid in AList do
    LList.Add(uuid);

  // Перед началом обнаружения необходимо убедится что все дивайсы отключены.
  // Иначе обнаружение не выдаст никаких результатотв
  for i := Low(BLE_devs) to High(BLE_devs) do
  begin
    if BLE_devs[i].device.IsConnected then
      BLE_devs[i].device.Disconnect;
  end;

  f_device_opening := False;
  StartDevicesDiscovering(discovering_time);

  with frmMain.btgroupDevices.Items do
  begin
    Clear;
    SetLength(BLE_devs, 0);
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: BLEDiscoverLEDevice

  Функция вызывается каждый раз когда обнаружен очередное устройство
  Может вызываться многократно для одного и того же устройства

  Arguments:
  const Sender: TObject;
  const ADevice: TBluetoothLEDevice;
  Rssi: Integer; const ScanResponse: TScanResponse
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.BLE_OnDiscoverLEDevice(const Sender: TObject; const ADevice: TWinRTBluetoothLEDevice; rssi: integer; const ScanResponse: TScanResponse);
var
  k: integer;
begin
  CodeSite.Send('-- OnDiscoverLEDevice. Device:' + ADevice.DeviceName + '(' + ADevice.address + ')');

  if f_device_opening then
  begin
    // Здесь мы ждем обнаружения заданного устройства
    if ADevice.address = BLE_devs[target_dev_indx].device.address then
    begin
      // Прекращаем обнаружение устройств, поскольку обнаружилось искомое
      BLE.CancelDiscovery;
      // Запускаем обнаружение сервисов и одновременно подключение устройства
      if ADevice.DiscoverServices = True then
      begin
        BLE_devs[target_dev_indx].device := ADevice;
        frmMain.DeviceOpened(target_dev_indx);
      end;
    end;
  end
  else
  begin
    if SearchInDiscoveredDevices(ADevice, rssi) = False then
    begin
      // Добавляем дивайс
      k := High(BLE_devs) + 1;
      SetLength(BLE_devs, k + 1);
      BLE_devs[k].device := ADevice;
      BLE_devs[k].name := ADevice.DeviceName;
      BLE_devs[k].address := ADevice.address;
      BLE_devs[k].rssi := rssi.ToString;
      BLE_devs[k].id := k;
      BLE_devs[k].discovered := False;
      AddButtontoForm(ADevice.DeviceName, rssi.ToString, ADevice.address, k);
    end;
  end;

end;

{ -----------------------------------------------------------------------------
  Procedure: BLEEndDiscoverDevices

  Arguments: const Sender: TObject;   Sender is the TBluetoothLEManager
  const ADeviceList: TBluetoothLEDeviceList
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.BLE_OnEndDiscoverDevices(const Sender: TObject; const ADeviceList: TBluetoothLEDeviceList);
begin
  // Событие вызывается в ответ на команду BLE.CancelDiscovery в течении выполнения это команды, т.е. синхронно
  CodeSite.Send('-- OnEndDiscoverDevices');
end;

{ -----------------------------------------------------------------------------
  Procedure: EndDiscoveringDevices

  По таймауту времени поиска дивайсов или другому событию прекращаем поиск и начинаем обнаружение сервисов

  Arguments: None
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.EndDiscoveringDevices;
begin
  CodeSite.Send('BLE.CancelDiscovery. Begin');
  BLE.CancelDiscovery;
  CodeSite.Send('BLE.CancelDiscovery. End');
end;

{ -----------------------------------------------------------------------------
  Procedure: SelectDevAndStartDiscoverServices

  Arguments: indx: integer
  Result:    boolean
  ----------------------------------------------------------------------------- }
function TDm.DiscoverDevicesServices: boolean;
var
  i: integer;
  found_vuart_service: boolean;
  found_cmd_service: boolean;
  ble_char: TBluetoothGattCharacteristic;
  AServiceList: TBluetoothGattServiceList;
  j: integer;
begin
  CodeSite.Send('StartDiscoverServices.  Begin');
  result := False;

  for i := Low(BLE_devs) to High(BLE_devs) do
  begin
    try
      CodeSite.Send('StartDiscoverServices.  Discover on device:' + BLE_devs[i].device.DeviceName + '(' + BLE_devs[i].device.address + ')');
      if BLE_devs[i].device.DiscoverServices = True then
      begin
        AServiceList := BLE_devs[i].device.Services;
        found_vuart_service := False;
        found_cmd_service := False;
        // Ищем сервисы необходимые для работы виртуального UART
        for j := 0 to AServiceList.Count - 1 do
        begin
          if AServiceList.Items[j].uuid = Vuart_service_UUID then
          begin
            BLE_devs[i].vuart_service := AServiceList.Items[j];
            // Ищем необходимые характеристики
            if FindCharacteristic(Vuart_charact_write_UUID, AServiceList.Items[j], ble_char) = False then
              break;
            BLE_devs[i].write_characteristic := ble_char;
            if FindCharacteristic(Vuart_charact_read_UUID, AServiceList.Items[j], ble_char) = False then
              break;
            BLE_devs[i].read_characteristic := ble_char;
            found_vuart_service := True;
          end;

          if AServiceList.Items[j].uuid = Cmd_service_UUID then
          begin
            BLE_devs[i].cmd_service := AServiceList.Items[j];
            // Ищем необходимые характеристики
            if FindCharacteristic(Cmd_charact_mode_set_UUID, AServiceList.Items[j], ble_char) = False then
              break;
            BLE_devs[i].cmd_characteristic := ble_char;

            found_cmd_service := True;
          end;
        end;

        if found_vuart_service and found_cmd_service then
          BLE_devs[i].discovered := True;

        result := True;
      end;
    except
      // Здесь если вызов обнаружения сервисов вызвал ошибку
      BLE_devs[i].discovered := False;
      CodeSite.Send('StartDiscoverServices.  Exception on Discover. Device:' + BLE_devs[i].device.DeviceName + '(' + BLE_devs[i].device.address + ')');
    end;

  end;

  //DisconnectAll;
  ShowDiscoveredDivices;
  CodeSite.Send('StartDiscoverServices.  End');

end;

{ -----------------------------------------------------------------------------
  Procedure: StartDevicesDiscovering


  Arguments: discovering_time: Integer
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.StartDevicesDiscovering(discovering_time: integer);
begin
  // if BLE.CurrentManager.StartDiscovery(discovering_time, LList, False) = False then    // Не работает фильтрация
  if BLE.CurrentManager.StartDiscovery(discovering_time, nil, True) = False then
  begin
    ShowMessage('Can''t start discovering.');
    Abort;
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: StartDeviceOpening


  Arguments: None
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.StartDeviceOpening(indx: integer; discovering_time: integer);
begin
  f_device_opening := True;
  target_dev_indx := indx;
  Dm.StartDevicesDiscovering(discovering_time);
end;

{ -----------------------------------------------------------------------------
  Procedure: BLEServicesDiscovered

  Событие возникает в течении вызова функции  DiscoverServices после того как все сервисы обнаружены
  Но до события EndDiscoverServices
  Вызывается всегда из главного потока модуля ,даже если вызывающий это событие метод DiscoverServices вызывается из других потоков


  Arguments: const Sender: TObject;
  const AServiceList: TBluetoothGattServiceList
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.BLE_OnServicesDiscovered(const Sender: TObject; const AServiceList: TBluetoothGattServiceList);
begin
  CodeSite.Send('-- OnServicesDiscovered');
end;

{ -----------------------------------------------------------------------------
  Procedure: ShowDiscoveredDivices

  Каждый раз когда мы просмотрели сервисы очередного дивайса мы перерисовываем кнопки дивайсов по событию

  Arguments: None
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.ShowDiscoveredDivices;
var
  i: integer;
begin
  // Скрыть устройства без необходимых сервисов
  frmMain.btgroupDevices.Items.Clear;
  for i := Low(BLE_devs) to High(BLE_devs) do
  begin
    if BLE_devs[i].discovered = True then
      AddButtontoForm(BLE_devs[i].name, BLE_devs[i].rssi, BLE_devs[i].address, i);
  end;
  frmMain.RestoreVisualsAfterDiscover;
end;

{ -----------------------------------------------------------------------------
  Procedure: BLEEndDiscoverServices

  Событие возникает в течении вызова функции  DiscoverServices после того как все сервисы обнаружены
  Перед этим вызывается событие ServicesDiscovered

  Arguments: const Sender: TObject; const AServiceList: TBluetoothGattServiceList
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.BLE_OnEndDiscoverServices(const Sender: TObject; const AServiceList: TBluetoothGattServiceList);
begin
  CodeSite.Send('-- OnEndDiscoverServices');
end;

procedure TDm.BLE_OnConnectedDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);
begin
  CodeSite.Send('-- OnConnectedDevice');
end;

{ -----------------------------------------------------------------------------
  Procedure: BLEConnect

  Вызывается каждый раз когда какое либо устройство включается

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.BLE_OnConnect(Sender: TObject);
begin
  CodeSite.Send('-- OnConnect Device:' + TBluetoothLEDevice(Sender).DeviceName);
end;

procedure TDm.BLE_OnReadRSSI(const Sender: TObject; ARssiValue: integer; AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnReadRSSI');
end;

procedure TDm.BLE_OnDescriptorRead(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor; AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnDescriptorRead');
end;

procedure TDm.BLE_OnDescriptorWrite(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor; AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnDescriptorWrite');
end;

procedure TDm.BLE_OnCharacteristicSubscribed(const Sender: TObject; const AClientId: string; const ACharacteristic: TBluetoothGattCharacteristic);
begin
  CodeSite.Send('-- OnCharacteristicSubscribed');
end;

procedure TDm.BLE_OnCharacteristicUnSubscribed(const Sender: TObject; const AClientId: string; const ACharacteristic: TBluetoothGattCharacteristic);
begin
  CodeSite.Send('-- OnCharacteristicUnSubscribed');
end;

procedure TDm.BLE_OnCharacteristicWriteRequest(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
  var AGattStatus: TBluetoothGattStatus; const AValue: TByteDynArray);
begin
  CodeSite.Send('-- OnCharacteristicWriteRequest');
end;

procedure TDm.BLE_OnCharacteristicWriteprocedure(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnCharacteristicWriteprocedure');
end;

procedure TDm.BLE_OnCharacteristicReadRequest(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
  var AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnCharacteristicReadRequest');
end;

procedure TDm.BLE_OnCharacteristicRead(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnCharacteristicRead');
end;

procedure TDm.BLE_OnCharacteristicWrite(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnCharacteristicWrite');
end;

procedure TDm.BLE_OnReliableWriteCompleted(const Sender: TObject; AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnReliableWriteCompleted');
end;

procedure TDm.BLE_OnServiceAdded(const Sender: TObject; const AService: TBluetoothGattService; const AGattStatus: TBluetoothGattStatus);
begin
  CodeSite.Send('-- OnServiceAdded');
end;

{ -----------------------------------------------------------------------------
  Procedure: BLEDisconnect

  Вызывается каждый раз когда какое либо устройство отключается

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.BLE_OnDisconnect(Sender: TObject);
begin
  CodeSite.Send('-- OnDisconnect. Device:' + TBluetoothLEDevice(Sender).DeviceName);
end;

{ -----------------------------------------------------------------------------
  Procedure: BLEDisconnectDevice

  В режиме клиента не вызывается !!!

  Arguments: const Sender: TObject; const ADevice: TBluetoothLEDevice
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.BLE_OnDisconnectDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);
begin
  CodeSite.Send('-- OnDisconnectDevice');
end;

{ -----------------------------------------------------------------------------
  Procedure: AddButtontoForm

  Arguments: DeviceName: string; rssi: integer; address: string; indx: integer
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.AddButtontoForm(DeviceName: string; rssi: string; address: string; indx: integer);
var
  item: TGrpButtonItem;
begin
  with frmMain.btgroupDevices do
  begin
    item := frmMain.btgroupDevices.Items.Add;
    item.Caption := DeviceName + ' (' + rssi + ' dBm)' + #13 + address;
    item.Hint := indx.ToString;
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: DisconnectDevice


  Arguments: indx: integer
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.DisconnectDevice(indx: integer);
begin
  if BLE_devs[indx].device.IsConnected then
  begin
    try
      BLE_devs[indx].device.Disconnect;
    except
    end;
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: DisconnectAllExceptSelected

  Arguments: indx: integer
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.DisconnectAllExceptSelected(indx: integer);
var
  i: integer;
begin
  for i := Low(BLE_devs) to High(BLE_devs) do
  begin
    if i <> indx then
      BLE_devs[i].device.Disconnect;
  end;
end;

{-----------------------------------------------------------------------------
  Procedure: DisconnectAll
  
  
  Arguments: None
  Result:    None
-----------------------------------------------------------------------------}
procedure TDm.DisconnectAll;
var
  i: integer;
begin
  for i := Low(BLE_devs) to High(BLE_devs) do
  begin
      BLE_devs[i].device.Disconnect;
  end;
end;


{ -----------------------------------------------------------------------------
  Procedure: SearchInDiscoveredDevices

  Ищем устройство в массиве уже найденых устройств

  Arguments: const ADevice: TBluetoothLEDevice
  Result:    boolean
  ----------------------------------------------------------------------------- }
function TDm.SearchInDiscoveredDevices(const ADevice: TBluetoothLEDevice; rssi: integer): boolean;
var
  i: integer;
begin
  result := False;
  for i := Low(BLE_devs) to High(BLE_devs) do
  begin
    if (BLE_devs[i].name = ADevice.DeviceName) and (BLE_devs[i].address = ADevice.address) then
    begin
      with frmMain.btgroupDevices do
      begin
        with Items[i] do
        begin
          Caption := ADevice.DeviceName + ' (' + rssi.ToString + ' dBm)' + #13 + ADevice.address;
        end;
      end;
      result := True;
      exit;
    end;
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: FindCharacteristic

  Arguments: char_indx: Integer
  Result:    boolean
  ----------------------------------------------------------------------------- }
function TDm.FindCharacteristic(const uuid: TBluetoothUUID; BLEService: TBluetoothGattService; var BLEChar: TBluetoothGattCharacteristic): boolean;
var
  indx: integer;
begin
  result := False;
  for indx := 0 to BLEService.Characteristics.Count - 1 do
  begin
    if BLEService.Characteristics[indx].uuid = uuid then
    begin
      BLEChar := BLEService.Characteristics[indx];
      result := True;
      exit;
    end;
  end;
end;

{ -----------------------------------------------------------------------------
  Procedure: DataModuleDestroy

  Arguments: Sender: TObject
  Result:    None
  ----------------------------------------------------------------------------- }
procedure TDm.DataModuleDestroy(Sender: TObject);
begin
  SetLength(BLE_devs, 0);
  if LList <> Nil then
    LList.Free;
end;

end.
