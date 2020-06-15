Web Security HW3
===

## 建置環境與使用說明

* Ubuntu 20.04
* g++ 9.3.0
* boost 1.71
* fmt 6.1.2
* GNU Make 4.2.1
* CMake 3.16.3
* openssl 2.1.7
* libssl 1.1.1

```bash
$ sudo apt -y update
$ sudo apt install -y g++ libboost-all-dev libfmt-dev make cmake libssl-dev openssl
$ ./build.sh #build the project
$ ./build/bin/server 55688 # run the server on port 55688
```

* server key 位置為 .key/host.key
* server certificate 位置為 .key/host.crt
* CA certificate 位置為 .key/ca.crt

重要程式碼
---
和作業一大同小異，差別是改成https連線

```cpp
// src/server/server.cpp
WebServer::WebServer(asio::io_context& ioContext, short port)
    : acceptor_(ioContext, tcp::endpoint(tcp::v4(), port)),
      context_(asio::ssl::context::sslv23)
{
    context_.set_options(
        asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use);
    // server certificate
    context_.use_certificate_chain_file(".key/host.crt");
    // server key
    context_.use_private_key_file(".key/host.key", asio::ssl::context::pem);
    // no need to verify client
    context_.set_verify_mode(asio::ssl::context::verify_none);
    accept();
}
```

設計架構與功能
---

### 架構

![](https://i.imgur.com/PT4fZck.jpg)


* Client
    * 就是個browser
* Server
    * 採用**asynchronous socket**，提升效能
    * 解析request和setenv
    * 將stdout導向socket，fork並執行對應的cgi
* Cgi
    * 從env拿到資料，把要輸出的資料全部丟到stdout就對了

### 功能

使用者將CA certificate放入瀏覽器，透過<hostname>/view.cgi進入後，可以看到目前上傳的檔案，並可以點入觀看詳細資料，最下方有上傳選項，使用者可以選擇本機的檔案，按下Upload後，會送Post Request到Server，Server會呼叫insert.cgi把檔案寫入upload資料夾中，成功後5秒內會將畫面重新導回/view.cgi。


成果截圖
---

| ![](https://i.imgur.com/bOSQpkw.png) | 一開始的畫面 |
| -------- | -------- |
| ![uploaded](https://i.imgur.com/csoi3hI.png) | 上傳成功 |
| ![](https://i.imgur.com/KCrvdFm.png) | 清單新增了 |

## 困難與心得

這次作業是把前兩次作業整合，相較之下沒碰到什麼困難。

## SSL- MITM
這篇論文描述了如何 Forge Ssl Certificate，用 Man-in-the-middle 的方式取得使用者加密資訊。

假設有三個人，user A、server B、Man-in-the-Middle M。A送連線建立請求給B，而M透過socket攔截這個request。接著，M和B handshake來建立SSL連線。而M因此有了B的 certificate，並從中拿出OU domain，將自己self-signed的certificate OU domain改成一樣，只是前面加個空白，以欺騙user。

最後，M將偽造的certificate送給A，因為是self-signed，所以browser提示A找不到CA，但真的和假的名字只差了一個空白，如果A不小心選擇接受，這項攻擊就成功了。

沒有甚麼太複雜的手法，單單利用使用者的不小心來攻擊，高明。但論文最後有提到，如果再加入MAC驗證certificate完整性，就可以阻擋了。
