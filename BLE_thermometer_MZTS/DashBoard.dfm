object frmDashBoard: TfrmDashBoard
  Left = 0
  Top = 0
  HorzScrollBar.Visible = False
  BorderStyle = bsSingle
  Caption = 'MZTS'
  ClientHeight = 294
  ClientWidth = 373
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = cl3DDkShadow
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  KeyPreview = True
  Menu = MainMenu1
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  OnKeyPress = FormKeyPress
  OnKeyUp = FormKeyUp
  TextHeight = 13
  object Label1: TLabel
    Left = 24
    Top = 26
    Width = 28
    Height = 23
    Caption = 'T1:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T1: TLabel
    Left = 72
    Top = 26
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label2: TLabel
    Left = 24
    Top = 55
    Width = 28
    Height = 23
    Caption = 'T2:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label3: TLabel
    Left = 24
    Top = 84
    Width = 28
    Height = 23
    Caption = 'T3:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label4: TLabel
    Left = 24
    Top = 113
    Width = 28
    Height = 23
    Caption = 'T4:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label5: TLabel
    Left = 24
    Top = 142
    Width = 28
    Height = 23
    Caption = 'T5:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label6: TLabel
    Left = 24
    Top = 171
    Width = 28
    Height = 23
    Caption = 'T6:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label7: TLabel
    Left = 24
    Top = 200
    Width = 28
    Height = 23
    Caption = 'T7:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label8: TLabel
    Left = 24
    Top = 229
    Width = 28
    Height = 23
    Caption = 'T8:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = cl3DDkShadow
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T2: TLabel
    Left = 72
    Top = 55
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T3: TLabel
    Left = 72
    Top = 84
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T4: TLabel
    Left = 72
    Top = 113
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T5: TLabel
    Left = 72
    Top = 142
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T6: TLabel
    Left = 72
    Top = 171
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T7: TLabel
    Left = 72
    Top = 200
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T8: TLabel
    Left = 72
    Top = 229
    Width = 53
    Height = 23
    Caption = '0.00 C'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clPurple
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label9: TLabel
    Left = 184
    Top = 26
    Width = 127
    Height = 23
    Caption = 'TEMPERATURE'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object T_averaged: TLabel
    Left = 184
    Top = 55
    Width = 142
    Height = 56
    Caption = '0.00 C'
    Font.Charset = ANSI_CHARSET
    Font.Color = clRed
    Font.Height = -48
    Font.Name = 'Arial'
    Font.Style = [fsBold]
    ParentFont = False
    OnClick = T_averagedClick
  end
  object StatusBar: TStatusBar
    Left = 0
    Top = 269
    Width = 373
    Height = 25
    BorderWidth = 2
    Panels = <
      item
        Text = '----'
        Width = 200
      end
      item
        Width = 50
      end>
  end
  object MainMenu1: TMainMenu
    Left = 568
    Top = 152
  end
  object RefreshTimer: TTimer
    Enabled = False
    Interval = 250
    OnTimer = RefreshTimerTimer
    Left = 488
    Top = 64
  end
end
