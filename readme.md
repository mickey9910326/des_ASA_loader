# cmd_ASA_loader

- - -
## Usage  
` des_ASA_loader [--port <com>] [--hex <file_name>]  `  

| Options    |  Description    |
| :------------- | :------------- |
| `--port <com>`      | Use desinated port com |
| `-p <com> `         | Same as `--port <com>` |
| `--hex <file_name>` | Load file file_name |
| `-h <file_name>`    | Same as `--hex <file_name>` |
| `--help`            | Show help messege |
| `-?`                | Same as `--help` |
| `--verbose`         | Show external messege for debug usage |
| `--color`           | Print messege with color (only on Windows) |

- - -
## 中文說明
指令： ` des_ASA_loader [--port <com>] [--hex <file_name>]  `  

| Options    |  Description    |
| :------------- | :------------- |
| `--port <com>`      | 使用指定的com來連接 |
| `-p <com> `         | 如同 `--port <com>` |
| `--hex <file_name>` | 載入指定的燒錄檔 |
| `-h <file_name>`    | 如同 `--hex <file_name>` |
| `--help`            | 顯示說明文件 |
| `-?`                | 如同  `--help` |
| `--verbose`         | 顯示額外資訊，供除錯使用 |
| `--color`           | 使顯示資訊附加色彩 (only on Windows) |

- - -
## example  
`des_ASA_loader --port 8 --hex Test.hex`

連結到 USB COM 8，燒錄Test.hex到ASA_M128

- - -  
## TODO list
 - 增加Timeout機制
 - 顯示目前可使用的serialport(可能為新專案)
