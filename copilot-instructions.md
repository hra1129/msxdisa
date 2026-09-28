GitHub Copilot 向けのルール

# 参考資料
下記サイトは、特に確認なく自由に参照してよい

MSX テクハン Wiki
https://ngs.no.coocan.jp/doc/wiki.cgi/TechHan

MSX データパック Wiki
https://ngs.no.coocan.jp/doc/wiki.cgi/datapack

grauw's site
https://map.grauw.nl/resources/msx_io_ports.php

ZMA一式 (ローカル)
C:\Users\hra\Documents\github\HRA_product\zma

MSX資料（ローカル）
C:\Users\hra\Documents\github\HRA_product\msx_documents

# ビルド
VisualStudio 2022 がインストールされているので、それを使ってテストする。
ただし、リビルドすれば Windows 以外の MacOS, Linux 等でも動作することを期待する。

# ソースコード
インデントは揃える。
意味の分かるように過剰な省略形は避ける。
変数名や関数名は、その役割が明確になるように命名する。
コメントは必要に応じて適切に日本語で記述する。
ソースコードの冒頭には、MITライセンスであることを示す記述をつける。
関数や変数は、下層（呼ばれる側）が上に記述されているようにして、不要なプロトタイプ宣言は避ける。循環している場合など、必要な部分ではプロトタイプ宣言の利用は可。
グローバル変数の使用は最小限に抑える。必要な場合は、適切な命名規則を用いて他のコードと衝突しないようにする。
