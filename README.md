# Slow-Game

## ファイル
* README.md
    * このファイル
* Slow-Server.c
    * ゲームサーバーの実装
* Slow-Player.c
    * プレイヤープログラムのサンプル

## コンパイル
Slow-Server.c 及び Slow-Player.c は POSIX 環境でコンパイラ clang でのコンパイルを推奨します.

`clang Slow-Server.c -o Slow-Server`

`clang Slow-Player.c -o Slow-Player`

Windows 環境では [Cygwin](http://cygwin.com/) 上のclang でのコンパイルを推奨します。
Cygwinをインストールするときに clang のパッケージを選択します。

## ゲームサーバーの起動
ゲームを行うには Slow-Server を起動します.

* 使い方
     * ./Slow-Server --player1 EXE1 --player2 EXE2 --number 100

* オプション
    * --player1 プレイヤー1の実行ファイルを指定.
    * --player2 プレイヤー2の実行ファイルを指定.
    * --arg1 プレイヤー1の実行ファイルに与える第N引数. ただしNは--arg1が引数に現れた数.
    * --arg2 プレイヤー2の実行ファイルに与える第N引数. ただしNは--arg2が引数に現れた数.
    * --number 対戦数.
    * --version バージョン情報表示.
    * --verbose 動作を出力.

例
`./Slow-Server --player1 player1 --arg1 arg1 --arg1 arg2 --player2 player2`

* player1
    * player1 はプレイヤーの実行ファイルを指定します.
* arg1 
    * player1 の実行ファイルに与える第1引数.
* arg2 
    * player1 の実行ファイルに与える第2引数.
* player2
    * player2 にはもう一方のプレイヤーの実行ファイルを指定します.

## サンプルプレイヤー
Slow-Player.c はプレイヤーのサンプルプログラムです.

