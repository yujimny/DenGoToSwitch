# DenGoToSwitch

PlayStation1用の初代マスコン(2ハンドル)をNintendo Switchコントローラーとして動作させるArduinoスケッチです。特に「電車でGo!!」シリーズで使用することを想定しており、マスコンハンドルの5段階加速・8段階ブレーキ・非常ブレーキ（10-20段階）を実装しています。

## 機能

- PlayStation1用の初代マスコン(2ハンドル)の入力をNintendo Switchコントローラーとして出力
- マスコンハンドル操作の実装
  - 5段階の加速（切、1、2、3、4、5）
  - 8段階のブレーキ（1-8段階）
  - 非常ブレーキ（10-20段階）
- タクトスイッチによる左右方向入力
- ボタンマッピング機能

## 必要なハードウェア

- Arduino Leonardo（または互換ボード）
- PlayStation1用の初代マスコン(2ハンドル)
- タクトスイッチ 2個（左右方向入力用）

## 接続

### PlayStation1用の初代マスコン(2ハンドル)接続
- ATT: ピン11
- CMD: ピン9
- DAT: ピン8
- CLK: ピン10

### タクトスイッチ接続
- 左ボタン: ピン2（プルアップ有効）
- 右ボタン: ピン3（プルアップ有効）

## 必要なライブラリ

以下のライブラリをArduino IDEのライブラリマネージャーからインストールしてください：

1. **PsxControllerBitBang** - PlayStation1用の初代マスコン(2ハンドル)の読み取り用
2. **NintendoSwitchControlLibrary** - Switchコントローラーとしての出力用

## ボタンマッピング

### 基本ボタン
- PSX CROSS → Switch A
- PSX CIRCLE → Switch B
- PSX SQUARE → Switch X
- PSX SELECT → Switch MINUS
- PSX START → Switch PLUS

### マスコンハンドル操作

#### 加速（5段階）
- **切**: TRIANGLE release + PAD_UP + PAD_DOWN + PAD_LEFT + PAD_RIGHT
- **1段階**: TRIANGLE + PAD_UP + PAD_DOWN + PAD_RIGHT
- **2段階**: TRIANGLE release + PAD_UP + PAD_DOWN + PAD_RIGHT
- **3段階**: TRIANGLE + PAD_UP + PAD_DOWN + PAD_LEFT
- **4段階**: TRIANGLE release + PAD_UP + PAD_DOWN + PAD_LEFT
- **5段階**: TRIANGLE + PAD_UP + PAD_DOWN

#### ブレーキ（8段階）
L1、R1、L2、R2ボタンの組み合わせで制御：
- **解除**: L1 release + R1 + L2 + R2
- **1段階**: L1 + R1 + L2 release + R2
- **2段階**: L1 release + R1 + L2 release + R2
- **3段階**: L1 + R1 release + L2 + R2
- **4段階**: L1 release + R1 release + L2 + R2
- **5段階**: L1 + R1 release + L2 release + R2
- **6段階**: L1 release + R1 release + L2 release + R2
- **7段階**: L1 + R1 + L2 + R2 release
- **8段階**: L1 release + R1 + L2 + R2 release

#### 非常ブレーキ（10-20段階）
8段階から直接遷移可能。詳細な操作はコード内のコメントを参照してください。

### 方向入力
- 左タクトスイッチ: Switch 左方向
- 右タクトスイッチ: Switch 右方向

## インストール方法

1. このリポジトリをクローンまたはダウンロード
2. Arduino IDEを開く
3. 必要なライブラリをインストール
4. `DenGoToSwitch.ino`を開く
5. Arduino Leonardoに接続
6. ボードを選択（Arduino Leonardo）
7. スケッチをアップロード

## デバッグ

シリアルデバッグを有効にするには、コード内の以下の行のコメントを外してください：

```cpp
#define ENABLE_SERIAL_DEBUG
```

シリアルモニター（115200 baud）でコントローラーの状態を確認できます。

## ライセンス

Apache License 2.0

## 注意事項

- このスケッチはArduino Leonardo（または互換ボード）専用です
- PlayStation1用の初代マスコン(2ハンドル)の接続にはビットバンギング方式を使用しています
- タクトスイッチには50msのデバウンス処理が実装されています
