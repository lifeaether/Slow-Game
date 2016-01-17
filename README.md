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

`./Slow-Server --player1 player1 --player2 player2`

* player1
    * player1 にはプレイヤーの実行ファイルを指定します.
* player2
    * player2 にはもう一方のプレイヤーの実行ファイルを指定します.

## サンプルプレイヤー
Slow-Player.c はプレイヤーのサンプルプログラムです.

