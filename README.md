# C web server
## 作業說明
請實作一個小型的web server，具有底下功能：
1. 可從一般的瀏覽器，例如Chrome，取得所設計之web server上的網頁。
2. 網頁中需至少有一張圖片，並正確在瀏覽器上顯示。
3. 可以從瀏覽器上上傳檔案，並正確儲存至web server上。

技術細節：
1. 此web server必須為concurrent server。
2. 必須不可留下zombie process。

對於沒有在作業中指定的功能或規格，同學可依自訂細節。
<br>

## 執行方法
- 開啟server
```
make
sudo ./server
```
- 開啟client

在瀏覽器輸入網址`127.0.0.1`即可上傳檔案。

**注意**：
1. 僅支援firefox瀏覽器。
2. 檔案僅可上傳文字檔 (`.txt`, `.csv`, `.html`, etc.)，不可上傳圖片檔 (`.png`, `.jpg`, `.jpeg`)。
