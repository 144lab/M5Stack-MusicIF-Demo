# M5Stack-MusicIF Synthesizer Demo

M5Stack + M5Stack-MusicIF のデモンストレーション

USB-MIDIキーボードをつなぐとヘッドフォン端子から
鍵盤で叩いた内容に応じてシンセサイズした音が出力されます。

- Bボタンで音色は二種類を切り替え
- A、Cボタンで音量調整

# フォルダ構成

- synth-demo/ : M5Stack-Core用arduinoコード
- stm/ : MusicIF側ファームウェア(予定)

# ビルド

arduino-cli用のMakefileを利用するなら

```shell
make
```
