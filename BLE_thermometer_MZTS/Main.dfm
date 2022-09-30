object frmMain: TfrmMain
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsDialog
  Caption = 'Bluetooth LE Terminal'
  ClientHeight = 346
  ClientWidth = 194
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Position = poScreenCenter
  OnCreate = FormCreate
  OnShow = FormShow
  TextHeight = 13
  object lblState: TLabel
    Left = 0
    Top = 0
    Width = 194
    Height = 16
    Align = alTop
    Alignment = taCenter
    Caption = 'Select device'
    Color = 8270101
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWhite
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentColor = False
    ParentFont = False
    ShowAccelChar = False
    Transparent = False
    ExplicitWidth = 75
  end
  object btgroupDevices: TButtonGroup
    Left = 0
    Top = 16
    Width = 194
    Height = 295
    Align = alClient
    BevelInner = bvNone
    BevelOuter = bvNone
    ButtonHeight = 70
    ButtonWidth = 200
    ButtonOptions = [gboFullSize, gboShowCaptions]
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    Items = <>
    TabOrder = 0
    OnButtonClicked = btgroupDevicesButtonClicked
  end
  object Panel1: TPanel
    Left = 0
    Top = 311
    Width = 194
    Height = 35
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 1
    object btnDiscover: TSpeedButton
      Left = 0
      Top = 0
      Width = 49
      Height = 35
      Align = alLeft
      AllowAllUp = True
      Caption = 'Discover'
      OnClick = btnDiscoverClick
    end
    object btnStop: TSpeedButton
      Left = 49
      Top = 0
      Width = 49
      Height = 35
      Align = alLeft
      AllowAllUp = True
      Caption = 'Stop'
      Visible = False
      OnClick = btnStopClick
      ExplicitLeft = 0
    end
    object Panel2: TPanel
      Left = 98
      Top = 0
      Width = 96
      Height = 35
      Align = alClient
      AutoSize = True
      TabOrder = 0
      object ProgressBar: TProgressBar
        Left = 1
        Top = 16
        Width = 94
        Height = 18
        Align = alBottom
        DoubleBuffered = True
        ParentDoubleBuffered = False
        Position = 50
        Step = 1
        TabOrder = 0
        Visible = False
      end
    end
  end
  object DiscoverTimer: TTimer
    OnTimer = DiscoverTimerTimer
    Left = 112
    Top = 88
  end
  object OpenTimer: TTimer
    Enabled = False
    OnTimer = OpenTimerTimer
    Left = 112
    Top = 144
  end
end
