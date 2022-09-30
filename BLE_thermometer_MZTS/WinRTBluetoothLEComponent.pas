unit WinRTBluetoothLEComponent;

interface

uses
  System.Classes, System.SysUtils, System.Types, System.Bluetooth, WinRTBluetoothLE;


type

  TWinRTDiscoverLEDeviceEvent = procedure(const Sender: TObject; const ADevice: TWinRTBluetoothLEDevice; Rssi: Integer; const ScanResponse: TScanResponse) of object;


  TWinRTBluetoothLE = class(TComponent)
  private

    //FOnDescriptorReadRequest: TGattDescriptorReadEvent;
    //FOnDescriptorWriteRequest: TGattDescriptorWriteEvent;
    FEnabled: Boolean;
    FForceRefreshCachedDevices: Boolean;
    FOnCharacteristicRead: TGattCharacteristicEvent;
    FOnCharacteristicReadRequest: TGattCharacteristicReadEvent;
    FOnCharacteristicWrite: TGattCharacteristicEvent;
    FOnCharacteristicWriteRequest: TGattCharacteristicWriteEvent;
    FOnCharacteristicSubscribed: TGattCharacteristicSubscriptionEvent;
    FOnCharacteristicUnSubscribed: TGattCharacteristicSubscriptionEvent;
    FOnConnectedDevice: TConnectLEDeviceEvent;
    FOnDescriptorRead: TGattDescriptorEvent;
    FOnDescriptorWrite: TGattDescriptorEvent;
    FOnConnect: TNotifyEvent;
    FOnDisconnect: TNotifyEvent;
    FOnDisconnectDevice: TConnectLEDeviceEvent;
    FOnDiscoverLEDevice: TWinRTDiscoverLEDeviceEvent;
    FOnEndDiscoverDevices: TDiscoveryLEEndEvent;
    FOnEndDiscoverServices: TDiscoverServicesEvent;
    FOnServicesDiscovered: TDiscoverServicesEvent;
    FOnReadRSSI: TGattDeviceRSSIEvent;
    FOnReliableWriteCompleted: TGattOperationResultEvent;
    FOnServiceAdded: TGattServiceEvent;

  protected
    FManager: TBluetoothLEManager;
    FServer: TBluetoothGattServer;
    ///  <summary>
    ///  DoInternalDiscoverLEDevice is the procedure asocieted to OnDiscoverLeDevice event, internal job purpose
    ///  </summary>
    procedure DoInternalDiscoverLEDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice; Rssi: Integer; const ScanResponse: TScanResponse); overload;
    procedure DoInternalDiscoveryEnd(const Sender: TObject; const ADeviceList: TBluetoothLEDeviceList); overload;
    procedure Setup;
    ///  <summary>
    ///  This method set up the GattServer if supported by the device.
    ///  </summary>
    procedure SetupGattServer;
    ///  <summary> procedure for the OnConnect Event </summary>
    procedure DoConnect(Sender: TObject);
    ///  <summary> procedure for the OnDisconnect Event </summary>
    procedure DoDisconnect(Sender: TObject);
    procedure DoConnectedDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);
    procedure DoDisconnectDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);
    procedure DoOnCharacteristicRead(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
      AGattStatus: TBluetoothGattStatus);
    procedure DoOnCharacteristicReadRequest(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
      var AGattStatus: TBluetoothGattStatus);
    procedure DoOnCharacteristicWrite(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
      AGattStatus: TBluetoothGattStatus);
    procedure DoOnCharacteristicWriteRequest(const Sender: TObject; const ACharacteristic: TBluetoothGattCharacteristic;
      var AGattStatus: TBluetoothGattStatus; const AValue: TByteDynArray);
    ///  <summary> procedure for the OnCharacteristicSubscribed Event </summary>
    procedure DoOnCharacteristicSubscribed(const Sender: TObject; const AClientId: string;
      const ACharacteristic: TBluetoothGattCharacteristic);
    ///  <summary> procedure for the OnCharacteristicUnSubscribed Event </summary>
    procedure DoOnCharacteristicUnSubscribed(const Sender: TObject; const AClientId: string;
      const ACharacteristic: TBluetoothGattCharacteristic);
    procedure DoOnDescriptorRead(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
      AGattStatus: TBluetoothGattStatus);
    procedure DoOnDescriptorReadRequest(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
      var AGattStatus: TBluetoothGattStatus);
    procedure DoOnDescriptorWrite(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
      AGattStatus: TBluetoothGattStatus);
    procedure DoOnDescriptorWriteRequest(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
      var AGattStatus: TBluetoothGattStatus; const AValue: TBytes);
    procedure DoOnReadRSSI(const Sender: TObject; ARssiValue: Integer; AGattStatus: TBluetoothGattStatus);
    procedure DoOnReliableWriteCompleted(const Sender: TObject; AGattStatus: TBluetoothGattStatus);
    procedure DoServiceAdded(const Sender: TObject; const AService: TBluetoothGattService;
      const AGattStatus: TBluetoothGattStatus);
    ///  <summary>
    ///  DoOnDiscoverLEDevice is the procedure associated to OnDiscoverLEDevice
    ///  </summary>
    procedure DoOnDiscoverLEDevice(const Sender: TObject; const ADevice: TWinRTBluetoothLEDevice; Rssi: Integer; const ScanResponse: TScanResponse);
    ///  <summary>
    ///  DoOnServicesDiscovered is the procedure associated to OnServicesDiscovered
    ///  </summary>
    procedure DoOnServicesDiscovered(const Sender: TObject; const AServiceList: TBluetoothGattServiceList);
    procedure DoOnEndDiscoverDevices(const Sender: TObject; const ADeviceList: TBluetoothLEDeviceList);
    ///  <summary>  SetEnabled processes the Bluetooth request of Enabled  </summary>
    procedure SetEnabled(AEnable: Boolean);

    function GetSupportsGattClient: Boolean;
    function GetSupportsGattServer: Boolean;
    ///  <summary>  GetDiscoveredDevices process the property DiscoveredDevices  </summary>
    function GetDiscoveredDevices: TBluetoothLEDeviceList;
    ///  <summary>  SetForceRefreshCachedDevices sets the property ForceRefreshCachedDevices </summary>
    procedure SetForceRefreshCachedDevices(Value: Boolean);

  public

    //function AddDescriptor(const ACharacteristic: TBluetoothGattCharacteristic; const ADescriptor: TBluetoothGattDescriptor): Boolean;
    //function CreateDescriptor(const ACharacteristic: TBluetoothGattCharacteristic; const AUUID: TBluetoothUUID): TBluetoothGattDescriptor;
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    function AddCharacteristic(const AService: TBluetoothGattService;
      const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
    function BeginReliableWrite(const ADevice: TBluetoothLEDevice): Boolean;
    function CreateCharacteristic(const AService: TBluetoothGattService; const AUUID: TBluetoothUUID;
      APropertyFlags: TBluetoothPropertyFlags; const ADescription: string): TBluetoothGattCharacteristic;
    function CreateIncludedService(const AService: TBluetoothGattService; const AnUUID: TBluetoothUUID;
      AType: TBluetoothServiceType): TBluetoothGattService;
    function CreateService(const AnUUID: TBluetoothUUID; AType: TBluetoothServiceType): TBluetoothGattService;
    function DiscoverServices(const ADevice: TBluetoothLEDevice): Boolean;
    function ExecuteReliableWrite(const ADevice: TBluetoothLEDevice): Boolean;
    function GetCharacteristic(const AService: TBluetoothGattService; const AUUID: TBluetoothUUID)
      : TBluetoothGattCharacteristic;
    function GetCharacteristics(const AService: TBluetoothGattService): TBluetoothGattCharacteristicList;
    function GetConnectionState: TBluetoothConnectionState;
    function GetCurrentAdapter: TBluetoothLEAdapter;
    function GetCurrentManager: TBluetoothLEManager;
    function GetDescriptors(const ACharacteristic: TBluetoothGattCharacteristic): TBluetoothGattDescriptorList;
    function GetGattServer: TBluetoothGattServer;
    function GetService(const ADevice: TBluetoothLEDevice; const AServiceID: TBluetoothUUID): TBluetoothGattService;
    function GetServices(const ADevice: TBluetoothLEDevice): TBluetoothGattServiceList;
    function ReadCharacteristic(const ADevice: TBluetoothLEDevice;
      const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
    function ReadDescriptor(const ADevice: TBluetoothLEDevice; const ADescriptor: TBluetoothGattDescriptor): Boolean;
    function SubscribeToCharacteristic(const ADevice: TBluetoothLEDevice;
      const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
    function WriteCharacteristic(const ADevice: TBluetoothLEDevice;
      const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
    function WriteDescriptor(const ADevice: TBluetoothLEDevice; const ADescriptor: TBluetoothGattDescriptor): Boolean;
    procedure AbortReliableWrite(const ADevice: TBluetoothLEDevice);
    procedure CancelDiscovery;
    procedure ClearServices;
    function DiscoverDevices(ATimeout: Integer): Boolean; overload;
    function DiscoverDevices(ATimeout: Integer; const AList: TBluetoothUUIDsList): Boolean; overload;
    function DiscoverDevices(ATimeout: Integer; AList: array of TBluetoothUUID): Boolean; overload;
    /// <summary> Starts a scan for Bluetooth LE devices that advertise data. ABluetoothLEScanFilterList is a List of Filters to perform scan
    /// related operations for specific devices. </summary>
    function DiscoverDevices(ATimeout: Cardinal; const ABluetoothLEScanFilterList: TBluetoothLEScanFilterList): Boolean; overload;
    function UnSubscribeToCharacteristic(const ADevice: TBluetoothLEDevice;
      const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
    procedure UpdateCharacteristicValue(const ACharacteristic: TBluetoothGattCharacteristic);
    ///  <summary>
    ///  RssiToDistance takes as a in-parameters the RSSI (Received signal strength indication)dBm,
    ///  the TxPower (Signal Strength at 1m distance)dBm and a Signal Propagation const, and
    ///  returns an approximation of the distance to a device in meters
    ///  </summary>
    function RssiToDistance(ARssi, ATxPower: Integer; ASignalPropagationConst: Single): Double;
    ///  <summary>
    ///  ReadRemoteRSSI read the RSSI from a remote Device. This is an asynchronous operation,
    ///  once ReadRemoteRSSI is completed, the OnReadRSSI callback is triggered.
    ///  </summary>
    function ReadRemoteRSSI(const ADevice: TBluetoothLEDevice): Boolean;
    ///  <summary> AddService Add the previously created Services and characteristics </summary>
    function AddService(const AService: TBluetoothGattService): Boolean;
    property ConnectionState: TBluetoothConnectionState read GetConnectionState;
    property GattServer: TBluetoothGattServer read GetGattServer;
    property CurrentAdapter: TBluetoothLEAdapter read GetCurrentAdapter;
    property CurrentManager: TBluetoothLEManager read GetCurrentManager;
    ///  <summary> DiscoveredDevices returns the list of found devices  </summary>
    property DiscoveredDevices: TBluetoothLEDeviceList read GetDiscoveredDevices;
    property SupportsGattClient: Boolean read GetSupportsGattClient;
    property SupportsGattServer: Boolean read GetSupportsGattServer;
    /// <summary> Setting to True, discovery process attemps to retrieve services from the actual device
    /// instead of use the cached ones. This property only has effect in Android. Default is False </summary>
    property ForceRefreshCachedDevices: Boolean read FForceRefreshCachedDevices write SetForceRefreshCachedDevices;

  published

    // property OnDescriptorReadRequest: TGattDescriptorReadEvent read FOnDescriptorReadRequest write FOnDescriptorReadRequest;
    // property OnDescriptorWriteRequest: TGattDescriptorWriteEvent read FOnDescriptorWriteRequest write FOnDescriptorWriteRequest;
    ///  <summary>  Enable or Disable the BluetoothLE component </summary>
    property Enabled: Boolean read FEnabled write SetEnabled;
    property OnCharacteristicRead: TGattCharacteristicEvent read FOnCharacteristicRead write FOnCharacteristicRead;
    property OnCharacteristicReadRequest: TGattCharacteristicReadEvent read FOnCharacteristicReadRequest
      write FOnCharacteristicReadRequest;
    property OnCharacteristicWrite: TGattCharacteristicEvent read FOnCharacteristicWrite write FOnCharacteristicWrite;
    property OnCharacteristicWriteRequest: TGattCharacteristicWriteEvent read FOnCharacteristicWriteRequest
      write FOnCharacteristicWriteRequest;
    ///  <summary>
    ///  OnCharacteristicSubscribed is triggered when a remote device Subscribes to one of Gatt server Characteristic.
    ///  </summary>
    property OnCharacteristicSubscribed: TGattCharacteristicSubscriptionEvent read FOnCharacteristicSubscribed
      write FOnCharacteristicSubscribed;
    ///  <summary>
    ///  OnCharacteristicSubscribed is triggered when a remote device UnSubscribes to one of Gatt server Characteristic.
    ///  </summary>
    property OnCharacteristicUnSubscribed: TGattCharacteristicSubscriptionEvent read FOnCharacteristicUnSubscribed
      write FOnCharacteristicUnSubscribed;
    ///  <summary>
    ///  OnConnectDevice is triggered when a remote device connects to our Gatt server.
    ///  </summary>
    property OnConnectedDevice: TConnectLEDeviceEvent read FOnConnectedDevice write FOnConnectedDevice;
    property OnDescriptorRead: TGattDescriptorEvent read FOnDescriptorRead write FOnDescriptorRead;
    property OnDescriptorWrite: TGattDescriptorEvent read FOnDescriptorWrite write FOnDescriptorWrite;
    ///  <summary>
    ///  OnConnect is triggered when we connect to a remote Gatt server device.
    ///  </summary>
    property OnConnect: TNotifyEvent read FOnConnect write FOnConnect;
    ///  <summary>
    ///  OnDisconnect is triggered when we disconnect from a remote Gatt server device.
    ///  </summary>
    property OnDisconnect: TNotifyEvent read FOnDisconnect write FOnDisconnect;
    ///  <summary>
    ///  OnDisconnectDevice is triggered when a remote device disconnects from our Gatt server.
    ///  </summary>
    property OnDisconnectDevice: TConnectLEDeviceEvent read FOnDisconnectDevice write FOnDisconnectDevice;
    ///  <summary>
    ///  OnDiscoverLEDevice is fired each time a device is found
    ///  </summary>
    property OnDiscoverLEDevice: TWinRTDiscoverLEDeviceEvent read FOnDiscoverLEDevice write FOnDiscoverLEDevice;
    ///  <summary>
    ///  OnServicesDiscovered is fired each time a device services are found
    ///  </summary>
    property OnServicesDiscovered: TDiscoverServicesEvent read FOnServicesDiscovered write FOnServicesDiscovered;
    property OnEndDiscoverDevices: TDiscoveryLEEndEvent read FOnEndDiscoverDevices write FOnEndDiscoverDevices;
    ///  <summary>
    ///  OnEndDiscoverServices property deprecated, use OnServicesDiscovered;
    ///  </summary>
    property OnEndDiscoverServices: TDiscoverServicesEvent read FOnEndDiscoverServices write FOnEndDiscoverServices;

    property OnReadRSSI: TGattDeviceRSSIEvent read FOnReadRSSI write FOnReadRSSI;
    property OnReliableWriteCompleted: TGattOperationResultEvent read FOnReliableWriteCompleted
      write FOnReliableWriteCompleted;
    property OnServiceAdded: TGattServiceEvent read FOnServiceAdded write FOnServiceAdded;
  end;

implementation

uses
  System.Generics.Collections, System.NetConsts;



procedure TWinRTBluetoothLE.AbortReliableWrite(const ADevice: TBluetoothLEDevice);
begin
  if FEnabled then
    if (ADevice <> nil) then
      ADevice.AbortReliableWrite
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.AddCharacteristic(const AService: TBluetoothGattService;
  const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
begin
  if GetGattServer <> nil then
    Result := GetGattServer.AddCharacteristic(AService, ACharacteristic)
  else
    Result := False;
end;

//function TBluetoothLE.AddDescriptor(const ACharacteristic: TBluetoothGattCharacteristic;
//  const ADescriptor: TBluetoothGattDescriptor): Boolean;
//begin
//
//end;

function TWinRTBluetoothLE.BeginReliableWrite(const ADevice: TBluetoothLEDevice): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.BeginReliableWrite
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

procedure TWinRTBluetoothLE.CancelDiscovery;
begin
  if FManager <> nil then
    FManager.CancelDiscovery
end;

procedure TWinRTBluetoothLE.ClearServices;
begin
  if GetGattServer <> nil then
    GetGattServer.ClearServices;
end;

procedure TWinRTBluetoothLE.Setup;
begin
  FManager :=  TWinRTBluetoothLEManager.Create as TBluetoothLEManager; //TBluetoothLEManager.CreateInstance;
  FManager.ForceRefreshCachedDevices := FForceRefreshCachedDevices;
  FManager.EnableBluetooth;
  FManager.OnDiscoveryEnd := DoInternalDiscoveryEnd;
  FManager.OnDiscoverLeDevice := DoInternalDiscoverLEDevice;
end;

procedure TWinRTBluetoothLE.SetupGattServer;
begin
  if (FServer = nil) and SupportsGattServer then
  begin
    // May Gatt Server is not supported
    FServer := FManager.GetGattServer;
    FServer.OnConnectedDevice := DoConnectedDevice;
    FServer.OnDisconnectDevice := DoDisconnectDevice;
    FServer.OnCharacteristicRead := DoOnCharacteristicReadRequest;
    FServer.OnCharacteristicWrite := DoOnCharacteristicWriteRequest;
    FServer.OnClientSubscribed := DoOnCharacteristicSubscribed;
    FServer.OnClientUnSubscribed := DoOnCharacteristicUnSubscribed;
    //FServer.OnDescriptorRead := DoOnDescriptorReadRequest;
    //FServer.OnDescriptorWrite := DoOnDescriptorWriteRequest;
    FServer.OnServiceAdded := DoServiceAdded;
  end;
end;

constructor TWinRTBluetoothLE.Create(AOwner: TComponent);
begin
  FEnabled := False;
  FForceRefreshCachedDevices := False;
  inherited;
end;

function TWinRTBluetoothLE.CreateCharacteristic(const AService: TBluetoothGattService;
  const AUuid: TBluetoothUUID; APropertyFlags: TBluetoothPropertyFlags;
  const ADescription: string): TBluetoothGattCharacteristic;
begin
  if GetGattServer <> nil then
    Result := GetGattServer.CreateCharacteristic(AService, AUuid, APropertyFlags, ADescription)
  else
    Result := nil;
end;

//function TBluetoothLE.CreateDescriptor(const ACharacteristic: TBluetoothGattCharacteristic;
//  const AUUID: TBluetoothUUID): TBluetoothGattDescriptor;
//begin
//  Result := GetGattServer.CreateDescriptor(ACharacteristic, AUUID);
//end;

function TWinRTBluetoothLE.CreateIncludedService(const AService: TBluetoothGattService; const AnUUID: TBluetoothUUID;
  AType: TBluetoothServiceType): TBluetoothGattService;
begin
  if GetGattServer <> nil then
    Result := GetGattServer.CreateIncludedService(AService, AnUUID, AType)
  else
    Result := nil;
end;

function TWinRTBluetoothLE.CreateService(const AnUUID: TBluetoothUUID; AType: TBluetoothServiceType): TBluetoothGattService;
begin
  if GetGattServer <> nil then
    Result := GetGattServer.CreateService(AnUUID, AType)
  else
    Result := nil;
end;

destructor TWinRTBluetoothLE.Destroy;
begin
  FManager.Free;
  inherited;
end;

function TWinRTBluetoothLE.DiscoverDevices(ATimeout: Integer): Boolean;
begin
  if (FManager <> nil) and SupportsGattClient then
    Result := FManager.StartDiscovery(ATimeout)
  else
    Result := False;
end;

function TWinRTBluetoothLE.DiscoverDevices(ATimeout: Integer; const AList: TBluetoothUUIDsList): Boolean;
begin
  if (FManager <> nil) and SupportsGattClient then
    Result := FManager.StartDiscovery(ATimeout, AList)
  else
    Result := False;
end;

function TWinRTBluetoothLE.DiscoverDevices(ATimeout: Cardinal; const ABluetoothLEScanFilterList: TBluetoothLEScanFilterList): Boolean;
begin
  if (FManager <> nil) and SupportsGattClient then
    Result := FManager.StartDiscovery(ATimeout, ABluetoothLEScanFilterList)
  else
    Result := False;
end;

function TWinRTBluetoothLE.DiscoverDevices(ATimeout: Integer; AList: array of TBluetoothUUID): Boolean;
var
  LList: TBluetoothUUIDsList;
  Uuid: TBluetoothUUID;
begin
  LList := TBluetoothUUIDsList.Create;
  for Uuid in AList do
    LList.Add(Uuid);
  Result := DiscoverDevices(ATimeout, LList);
end;

function TWinRTBluetoothLE.DiscoverServices(const ADevice: TBluetoothLEDevice): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.DiscoverServices
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

procedure TWinRTBluetoothLE.DoOnCharacteristicReadRequest(const Sender: TObject;
  const ACharacteristic: TBluetoothGattCharacteristic; var AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnCharacteristicReadRequest) then
    FOnCharacteristicReadRequest(Sender, ACharacteristic, AGattStatus);
end;

procedure TWinRTBluetoothLE.DoOnCharacteristicRead(const Sender: TObject;
  const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnCharacteristicRead) then
    TThread.Synchronize(nil, procedure
    begin
      if FEnabled and Assigned(FOnCharacteristicRead) then
        FOnCharacteristicRead(Sender, ACharacteristic, AGattStatus);
    end);
end;

procedure TWinRTBluetoothLE.DoOnServicesDiscovered(const Sender: TObject; const AServiceList: TBluetoothGattServiceList);
begin
  if FEnabled and Assigned(FOnServicesDiscovered) then
    TThread.Synchronize(nil, procedure
    begin
      if FEnabled and  Assigned(FOnServicesDiscovered) then
        FOnServicesDiscovered(Sender, AServiceList);
    end);

  if FEnabled and Assigned(FOnEndDiscoverServices) then
    TThread.Synchronize(nil, procedure
    begin
      if FEnabled and Assigned(FOnEndDiscoverServices) then
        FOnEndDiscoverServices(Sender, AServiceList);
    end);
end;

procedure TWinRTBluetoothLE.DoOnCharacteristicWriteRequest(const Sender: TObject;
  const ACharacteristic: TBluetoothGattCharacteristic; var AGattStatus: TBluetoothGattStatus; const AValue: TByteDynArray);
begin
  if FEnabled and Assigned(FOnCharacteristicWriteRequest) then
    FOnCharacteristicWriteRequest(Sender, ACharacteristic, AGattStatus, AValue);
end;

procedure TWinRTBluetoothLE.DoOnCharacteristicSubscribed(const Sender: TObject; const AClientId: string;
    const ACharacteristic: TBluetoothGattCharacteristic);
begin
  if FEnabled and Assigned(FOnCharacteristicSubscribed) then
    FOnCharacteristicSubscribed(Sender, AClientId, ACharacteristic);
end;

procedure TWinRTBluetoothLE.DoOnCharacteristicUnSubscribed(const Sender: TObject; const AClientId: string;
    const ACharacteristic: TBluetoothGattCharacteristic);
begin
  if FEnabled and Assigned(FOnCharacteristicUnSubscribed) then
    FOnCharacteristicUnSubscribed(Sender, AClientId, ACharacteristic);
end;

procedure TWinRTBluetoothLE.DoOnCharacteristicWrite(const Sender: TObject;
  const ACharacteristic: TBluetoothGattCharacteristic; AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnCharacteristicWrite) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnCharacteristicWrite) then
        FOnCharacteristicWrite(Sender, ACharacteristic, AGattStatus);
    end);
end;

procedure TWinRTBluetoothLE.DoOnDescriptorReadRequest(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
  var AGattStatus: TBluetoothGattStatus);
begin
                                                                             
//  if FEnabled and Assigned(FOnDescriptorReadRequest) then
//    TThread.Synchronize(nil, procedure begin
//      if FEnabled and Assigned(FOnDescriptorReadRequest) then
//        FOnDescriptorReadRequest(Sender, ADescriptor, AGattStatus);
//    end);
end;

procedure TWinRTBluetoothLE.DoOnDescriptorRead(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
  AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnDescriptorRead) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnDescriptorRead) then
        FOnDescriptorRead(Sender, ADescriptor, AGattStatus);
    end);
end;

procedure TWinRTBluetoothLE.DoOnDescriptorWriteRequest(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
  var AGattStatus: TBluetoothGattStatus; const AValue: TBytes);
begin
                                                                             
//  if FEnabled and Assigned(FOnDescriptorWrite) then
//    TThread.Synchronize(nil, procedure begin
//      if FEnabled and Assigned(FOnDescriptorWrite) then
//        FOnDescriptorWrite(Sender, ADescriptor, AGattStatus, AValue);
//    end);
end;

procedure TWinRTBluetoothLE.DoOnDescriptorWrite(const Sender: TObject; const ADescriptor: TBluetoothGattDescriptor;
  AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnDescriptorWrite) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnDescriptorWrite) then
        FOnDescriptorWrite(Sender, ADescriptor, AGattStatus);
    end);
end;

procedure TWinRTBluetoothLE.DoOnDiscoverLEDevice(const Sender: TObject; const ADevice: TWinRTBluetoothLEDevice; Rssi: Integer; const ScanResponse: TScanResponse);
begin
  if FEnabled and Assigned(FOnDiscoverLEDevice) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnDiscoverLEDevice) then
          FOnDiscoverLEDevice(Sender, ADevice, Rssi, ScanResponse);
    end);
end;

procedure TWinRTBluetoothLE.DoOnEndDiscoverDevices(const Sender: TObject; const ADeviceList: TBluetoothLEDeviceList);
begin
  if FEnabled and Assigned(FOnEndDiscoverDevices) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnEndDiscoverDevices) then
          FOnEndDiscoverDevices(Sender, ADeviceList);
    end);
end;

procedure TWinRTBluetoothLE.DoOnReadRSSI(const Sender: TObject; ARssiValue: Integer; AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnReadRSSI) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnReadRSSI) then
        FOnReadRSSI(Sender, ARssiValue, AGattStatus);
    end);
end;

procedure TWinRTBluetoothLE.DoOnReliableWriteCompleted(const Sender: TObject; AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnReliableWriteCompleted) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnReliableWriteCompleted) then
        FOnReliableWriteCompleted(Sender, AGattStatus);
    end);
end;

procedure TWinRTBluetoothLE.DoConnect(Sender: TObject);
begin
  if FEnabled and Assigned(FOnConnect) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnConnect) then
        FOnConnect(Sender);
    end);
end;

procedure TWinRTBluetoothLE.DoDisconnect(Sender: TObject);
begin
  if FEnabled and Assigned(FOnDisconnect) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnDisconnect) then
        FOnDisconnect(Sender);
    end);
end;

procedure TWinRTBluetoothLE.DoConnectedDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);
begin
  if FEnabled and Assigned(FOnConnectedDevice) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnConnectedDevice) then
        FOnConnectedDevice(Sender, ADevice);
    end);
end;

procedure TWinRTBluetoothLE.DoDisconnectDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice);
begin
  if FEnabled and Assigned(FOnDisconnectDevice) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnDisconnectDevice) then
        FOnDisconnectDevice(Sender, ADevice);
    end);
end;

procedure TWinRTBluetoothLE.DoInternalDiscoverLEDevice(const Sender: TObject; const ADevice: TBluetoothLEDevice;
  Rssi: Integer; const ScanResponse: TScanResponse);
begin
  if FEnabled then
  begin
    ADevice.OnCharacteristicRead := DoOnCharacteristicRead;
    ADevice.OnCharacteristicWrite := DoOnCharacteristicWrite;
    ADevice.OnDescriptorRead := DoOnDescriptorRead;
    ADevice.OnDescriptorWrite := DoOnDescriptorWrite;
    ADevice.OnReliableWriteCompleted := DoOnReliableWriteCompleted;
    ADevice.OnReadRSSI := DoOnReadRSSI;
    ADevice.OnServicesDiscovered := DoOnServicesDiscovered;
    ADevice.OnConnect := DoConnect;
    ADevice.OnDisconnect := DoDisconnect;

    DoOnDiscoverLEDevice(Sender, TWinRtBluetoothLEDevice(ADevice), Rssi,ScanResponse);
  end;
end;

procedure TWinRTBluetoothLE.DoInternalDiscoveryEnd(const Sender: TObject; const ADeviceList: TBluetoothLEDeviceList);
begin
  if FEnabled then
    DoOnEndDiscoverDevices(Sender, ADeviceList);
end;

procedure TWinRTBluetoothLE.SetEnabled(AEnable: Boolean);
begin
if not (csDesigning in ComponentState) then
  TThread.Synchronize(nil, procedure
  begin
    FEnabled := AEnable;
    if FEnabled then
    begin
      if FManager = nil then
        Setup;
    end
    else
    begin
      if FManager <> nil then
      begin
        CancelDiscovery;
        FManager.Free;
        FManager := nil;
      end;
      if FServer <> nil then
      begin
        FServer.Free;
        FServer := nil;
      end;
    end;
  end)
  else
    FEnabled := AEnable;
end;

procedure TWinRTBluetoothLE.SetForceRefreshCachedDevices(Value: Boolean);
begin
  if Value <> FForceRefreshCachedDevices then
  begin
    FForceRefreshCachedDevices := Value;
    if FManager <> nil then
      FManager.ForceRefreshCachedDevices := FForceRefreshCachedDevices;
  end;
end;

procedure TWinRTBluetoothLE.DoServiceAdded(const Sender: TObject; const AService: TBluetoothGattService;
  const AGattStatus: TBluetoothGattStatus);
begin
  if FEnabled and Assigned(FOnServiceAdded) then
    TThread.Synchronize(nil, procedure begin
      if FEnabled and Assigned(FOnServiceAdded) then
        FOnServiceAdded(Sender, AService, AGattStatus);
    end);
end;

function TWinRTBluetoothLE.ExecuteReliableWrite(const ADevice: TBluetoothLEDevice): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.ExecuteReliableWrite
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.GetCharacteristic(const AService: TBluetoothGattService;
  const AUUID: TBluetoothUUID): TBluetoothGattCharacteristic;
begin
  Result := nil;
  if FEnabled then
    if (AService <> nil) then
      Result := AService.GetCharacteristic(AUUID)
    else
      raise EBluetoothLEServiceException.Create(SBluetoothLENoServiceAssigned);
end;

function TWinRTBluetoothLE.GetCharacteristics(const AService: TBluetoothGattService): TBluetoothGattCharacteristicList;
begin
  Result := nil;
  if FEnabled then
    if (AService <> nil) then
      Result := AService.Characteristics
    else
      raise EBluetoothLEServiceException.Create(SBluetoothLENoServiceAssigned);
end;

function TWinRTBluetoothLE.GetConnectionState: TBluetoothConnectionState;
begin
  if FManager <> nil then
    Result := FManager.ConnectionState
  else
    Result := TBluetoothConnectionState.Disconnected;
end;

function TWinRTBluetoothLE.GetCurrentAdapter: TBluetoothLEAdapter;
begin
  if FManager <> nil then
    Result := FManager.CurrentAdapter
  else
    Result := nil;
end;

function TWinRTBluetoothLE.GetCurrentManager: TBluetoothLEManager;
begin
  Result := FManager;
end;

function TWinRTBluetoothLE.GetDescriptors(const ACharacteristic: TBluetoothGattCharacteristic): TBluetoothGattDescriptorList;
begin
  Result := ACharacteristic.Descriptors;
end;

function TWinRTBluetoothLE.GetGattServer: TBluetoothGattServer;
begin
  SetupGattServer;
  Result := FServer;
end;

function TWinRTBluetoothLE.GetService(const ADevice: TBluetoothLEDevice;
  const AServiceID: TBluetoothUUID): TBluetoothGattService;
begin
  Result := nil;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.GetService(AServiceID)
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.GetServices(const ADevice: TBluetoothLEDevice): TBluetoothGattServiceList;
begin
  Result := nil;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.Services
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.ReadCharacteristic(const ADevice: TBluetoothLEDevice;
  const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.ReadCharacteristic(ACharacteristic)
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.ReadDescriptor(const ADevice: TBluetoothLEDevice;
  const ADescriptor: TBluetoothGattDescriptor): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.ReadDescriptor(ADescriptor)
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.SubscribeToCharacteristic(const ADevice: TBluetoothLEDevice;
  const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.SetCharacteristicNotification(ACharacteristic, True)
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.GetSupportsGattClient: Boolean;
begin
  if FManager <> nil then
    Result := FManager.SupportsGattClient
  else
    Result := False;
end;

function TWinRTBluetoothLE.GetSupportsGattServer: Boolean;
begin
  if FManager <> nil then
    Result := FManager.SupportsGattServer
  else
    Result := False;
end;

function TWinRTBluetoothLE.GetDiscoveredDevices: TBluetoothLEDeviceList;
begin
  if FManager <> nil then
    Result := FManager.LastDiscoveredDevices
  else
    Result := nil;
end;

function TWinRTBluetoothLE.UnSubscribeToCharacteristic( const ADevice: TBluetoothLEDevice;
  const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.SetCharacteristicNotification(ACharacteristic, False)
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

procedure TWinRTBluetoothLE.UpdateCharacteristicValue(const ACharacteristic: TBluetoothGattCharacteristic);
begin
  if (GetGattServer <> nil) then
    GetGattServer.UpdateCharacteristicValue(ACharacteristic);
end;

function TWinRTBluetoothLE.WriteCharacteristic(const ADevice: TBluetoothLEDevice;
  const ACharacteristic: TBluetoothGattCharacteristic): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.WriteCharacteristic(ACharacteristic)
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.WriteDescriptor(const ADevice: TBluetoothLEDevice;
  const ADescriptor: TBluetoothGattDescriptor): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.ReadDescriptor(ADescriptor)
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.RssiToDistance(ARssi, ATxPower: Integer; ASignalPropagationConst: Single): Double;
begin
  if FManager <> nil then
    Result := FManager.RssiToDistance(ARssi, ATxPower, ASignalPropagationConst)
  else
    Result := 0;
end;

function TWinRTBluetoothLE.ReadRemoteRSSI(const ADevice: TBluetoothLEDevice): Boolean;
begin
  Result := False;
  if FEnabled then
    if (ADevice <> nil) then
      Result := ADevice.ReadRemoteRSSI
    else
      raise EBluetoothLEDeviceException.Create(SBluetoothLENoDeviceAssigned);
end;

function TWinRTBluetoothLE.AddService(const AService: TBluetoothGattService): Boolean;
begin
  if (GetGattServer <> nil) and (AService <> nil) then
    Result := GetGattServer.AddService(AService)
  else
    Result := False;
end;

end.
