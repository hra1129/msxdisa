# msxdisa 使い方

msxdisa は、MSX用ROMイメージを読み込み、ZMA向けのアセンブリソースを出力するC++17製のコマンドラインツールです。フラットROMと、ASCII-16、ASCII-8、Konami、SCC形式のMegaROMを指定できます。出力は推定結果を含むため、元プログラムの構造を完全に復元するものではありません。

## ビルドとテスト

Visual Studio 2022のDeveloper PowerShellなど、MSVCとCMakeが利用できる環境で、リポジトリのルートから次を実行します。

```bat
cmake -S . -B build -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Windowsでは生成された実行ファイルは通常 `build\Debug\msxdisa.exe` にあります。Release構成でビルドした場合は `build\Release\msxdisa.exe` です。macOS/Linuxでは対応するC++17コンパイラとCMakeを用いて構成し、生成された `msxdisa` を実行してください。ZMAは逆アセンブル自体には不要です。

## コマンドライン

```text
msxdisa [--mapper none|ascii16|ascii8|konami|scc] [--origin アドレス] [--start アドレス] [-o 出力.asm] 入力.rom
```

| オプション | 動作 |
| --- | --- |
| `--mapper` | ROM形式を指定します。省略時は `none`（フラットROM）です。形式の自動判定はしません。 |
| `--origin` | ROM先頭のCPU上の配置アドレスを上書きします。MegaROMではバンク幅に整列した `0x4000`〜`0xBFFF` のアドレスを指定します。 |
| `--start` | 命令解析の開始アドレスを上書きします。 |
| `-o`, `--output` | 出力ファイルを指定します。省略時は標準出力に出します。 |
| `-h`, `--help` | 簡単なヘルプを表示します。 |

アドレスには `0x4000` のような16進表記か10進表記を使えます。パスに空白が含まれる場合は引用符で囲んでください。

例（リポジトリのルートから実行）:

```bat
build\Debug\msxdisa.exe --mapper none -o example.asm example.rom
build\Debug\msxdisa.exe --mapper ascii8 -o cartridge.asm cartridge.rom
build\Debug\msxdisa.exe --origin 0x8000 --start 0x8010 -o custom.asm custom.rom
```

公開テスト用の `test_rom\disassemble.bat` はフラットROMの実行例です。バッチは `build\Release`、`build\Debug`、`build` の順に実行ファイルを探します。

## 既定アドレスと解析

- 先頭に `AB` カートリッジヘッダーがある場合、先頭アドレスは通常 `0x4000` です。ヘッダーのINITが `0x8000` 以上なら `0x8000` と推定します。INITが解析開始点になります。
- ヘッダーのないフラットROMは `0x0100` から配置・解析します。ヘッダーのないMegaROMは `0x4000` を既定値とします。
- ヘッダーの最初の2バイトは `DB`、続く部分は原則 `DW` として分類します。INITがヘッダー内を指す場合、探索されたバイトは命令扱いに変わることがあります。
- JP、JR、CALLの直接分岐先を探索し、無条件JP/RETでその経路の探索を終えます。全く調査されていないバンクは、先頭から命令として探索します。探索されずに残った領域はデータとして出力します。

MegaROMはファイルの先頭から物理バンク `0, 1, 2, ...` と番号を付けます。ASCII-16は16 KiB単位で `0x4000` または `0x8000` の窓に配置し、ASCII-8、Konami、SCCは8 KiB単位で `0x4000`、`0x6000`、`0x8000`、`0xA000` の窓に配置します。各バンク内の対応済み16-bit operandが参照する窓を数え、最も多い窓をそのバンクの `ORG` に採用します。同数または参照がない場合は、バンク番号から順に割り当てた既定の窓を維持します。INITを含むバンクとKonamiの固定bank 0も元の窓を維持します。`ORG` の前には `BANK#` コメントが付きます。

**注意:** この窓配置は静的な推定です。mapperレジスタへの書き込みを追跡しておらず、実行時にどのバンクが選択されるかは再現しません。同じ窓に複数の候補バンクがある場合、直接分岐先の特定もできません。推定配置と実機の挙動が異なる可能性があります。

## 出力の読み方

- ニーモニックと疑似命令のoperandはタブで同じ桁に揃えます。ジャンプ先などには物理bank番号とCPUアドレスからなる `B00L4004` 形式のラベルを使います。
- byteデータは `DB`、wordデータはリトルエンディアンの `DW` で出力します。データとしてまとめて出力したDB行の末尾にはASCII表示を付けます。`0x21`〜`0x7E` 以外は `.` で表示します。
- 対応するBIOSルーチン、HOOK、ワークエリアの名前が分かる場合はoperandを名前へ置換し、使用した名前だけファイル冒頭で値定義します。すべてのMSXシンボルを網羅しているわけではありません。
- Z80/R800の解釈が異なる既知の命令は、元バイト列を保つため `DB` と注釈で表す場合があります。デコーダが未対応の命令列も `DB` で残る場合があります。

## 確認と非公開ROMの取り扱い

生成ソースの再アセンブルを確認する場合は、別途ZMAを使います。リポジトリ内の `assembler\zma.exe` は逆アセンブラの実行時には参照しません。ZMAのアセンブルが成功した場合でも、元ROMとのバイト一致を確認してください。対応していないoperandが多いROMでは、現状の出力をZMAが受け付けないことがあります。

市販ROMや、その逆アセンブル結果は公開しないでください。現在の `.gitignore` は `*.rom` と `test_rom_secret/` を対象外にしています。秘密ROMの入力、ASMの出力、ZMAの検証用成果物はいずれも秘密ディレクトリまたはリポジトリ外に置き、Gitへ追加する前に `git status` と `git check-ignore` で確認してください。ZMAは実行した作業ディレクトリにログやsymbolファイルを生成する場合があります。