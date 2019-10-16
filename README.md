# M5Stack-MusicIF Synthesizer Demo

M5Stack + M5Stack-MusicIF のデモンストレーション

USB-MIDI キーボードをつなぐとヘッドフォン端子から
鍵盤で叩いた内容に応じてシンセサイズした音が出力されます。

- B ボタンで音色は二種類を切り替え
- A、C ボタンで音量調整

# フォルダ構成

- synth-demo/ : M5Stack-Core 用 arduino コード
- stm/ : MusicIF 側 STM 用ファームウェア
- host-controller/ : MusicIF 側本ストコントローラーファームウェア

# ビルド

arduino-cli 用の Makefile を利用するなら

```shell
make
```
