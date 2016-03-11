# sylph (Windows Service Wrapper)
Simple Windows Service Wrapper

history.
* 2016-03-11 : version 1.0.0.0 ... Add start type config、Output Eventlog
* 2016-03-10 : version 0.9.0.0 ... Rewrite to Win32.(Without ATL/COM)
* 2016-03-09 : version 0.8.0.0 ... new


# Require

* Visual Studio 2013 - 2015

VisualStudio Community
https://www.microsoft.com/ja-jp/dev/products/community.aspx

# Overview

sylphは、Go言語等で作ったコンソールアプリケーションをWindows Service化するための軽量な Service Wrapper です。 
Go等は簡単にWebServiceが作れますが、Windows Serviceで動かしたいなと思い作りました。   

sylphは、次のような特徴を持っています。

* 複数のプロセスを起動することができます。
* XMLで起動パラメータを指定します。
* とりあえず、なんでもサービスかしたいときに簡単に使えます。

sylph.exeは、リネームして対象のサービスごとに配布して利用できます。  
例えば、service1.exeやservice2.exeという感じでコピーして配置します。  
複数のプロセスとして配置する場合は、syconfig.xml のservice_nameをそれぞれ変更して使います。

# How to build

sylphは、Win32/C++で書かれています。  
ビルドするには、sylph.sln を開き、[Build] - [Solution rebuild] でビルドします。

# How To Use

## １、配置

sylph.exe を任意のディレクトリにコピーし、同じディレクトリに "syconfig.xml"を置きます。

## ２、設定ファイル（syconfig.xml)

syconfig.xml sample

    <!--  
      | sylph service wrapper config file 2016-03-11
      | syconfig.xml
    -->
    <sylph>
        <service>
            <config>
                <service_name>SylphService</service_name>
                <!-- start_type 
                     | 2: AUTO START
                     | 3: DEMAND START (Default)
                     | 4: DISABLE
                     | other : DEMAND_START
                  -->
                <start_type>3</start_type>
            </config>
            <entry>
                <!--process list-->
                <process>
                    <command>cmd.exe </command>
                </process>
            </entry>
        </service>
    </sylph>


### XML Tag. 

service_name  
* Windows Service名を書きます。

start_type
* Service StartTypeを書きます。Install時に反映されます。
* 2: AUTO START 「自動] となります
* 3: DEMAND START (Default)　「手動」となります。
* 4: DISABLE 「無効」となります。
* 2～4以外は、3(DEMAND START)となります。

entry 
* ここから、起動するコマンドを書きます。processは複数定義できます。（Multi Process）|

process/command
*  サービスとして実行するコマンドを書いていきます。

## ３、インストール

管理者権限のコマンドを開き、次のように実行します。

Install

    $ sylph.exe /install
    
Uninstall

    $ sylph.exe /uninstall
    

サービスに登録する前に、コマンドで動作確認できます。

Test (console debug)

    $ sylph.exe /console
    
 


# License

MIT.
