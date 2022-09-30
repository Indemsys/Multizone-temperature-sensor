program MZTS;

uses
  System.UITypes,
  Vcl.Forms,
  Vcl.Dialogs,
  Windows,
  SysUtils,
  Main in 'Main.pas' {frmMain},
  DataModule in 'DataModule.pas' {Dm: TDataModule},
  DashBoard in 'DashBoard.pas' {frmDashBoard},
  CodeSiteLogging,
  WinRTBluetoothLE in 'WinRTBluetoothLE.pas',
  WinRTBluetoothLEComponent in 'WinRTBluetoothLEComponent.pas';

{$R *.res}

begin
  // Запускаем только один экземпляр приложения

  if CreateMutex(nil, True, 'ACB312CE-F9C2-4300-8DCF-E720790BE7CB') = 0 then
    RaiseLastOSError;
  if GetLastError = ERROR_ALREADY_EXISTS then
  begin
    MessageDlg('This App already run!', mtWarning, [mbOK], 0);
    Exit;

  end;

  {$IFDEF DEBUG}
  CodeSite.Enabled := true;
  {$ELSE}
  CodeSite.Enabled := false;
  {$ENDIF}

  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TDm, Dm);
  Application.CreateForm(TfrmMain, frmMain);
  Application.Run;
end.
