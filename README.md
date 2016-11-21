# varlisp
a lisp syntax like mini interpreter

based no these Library:

- [cpp-linenoise](https://github.com/sarrow104/cpp-linenoise)
- boost::variant
- boost::asio  (http-get url)
- gumbo-parser
- gumbo-query  (gumbo-query content query-string)
- posix regex
- sarrow104::ss1x(not upload yet)
- sarrow104::sss(not upload yet)

extra builin-function list:
  - (read "path/to/file")
  - (write content "path/to/file")
  - (write-append content "path/to/file")

  - (split "string-to-be-split")
  - (split "string-to-be-split" "sepeator-char")
  - (join (content list))
  - (join (content list) "sepeator-string")

  - (http-get "url-string")
  - (http-get "url-string" "proxy-url" port-number)
  - (gumbo-query "html-file-content" "gumbo-query-string")

  - (regex "regex-string")
  - (regex-match reg-obj "target-string")
  - (regex-search reg-obj "target-string")
  - (regex-search reg-obj "target-string" offset-in-number)
  - (regex-replace reg-obj "target-string" "format-string")

  - (regex-split sep-reg "target-string")
  - (regex-collect reg "target-string")
  - (regex-collect reg "target-string" "fmt-string")

  - (substr "target-string" offset)
  - (substr "target-string" offset length)

  - (lambda (var) (body))

----------------------------------------------------------------------

## sample output

```lisp
> (regex-collect (regex "[0-9]+") "123 abc 1645 798a8709801")
("123" "1645" "798" "8709801")

> (regex-split (regex "[a-zA-Z ]+") "123 abc 1645 798a8709801")
("123" "1645" "798" "8709801")
```
just a toy

----------------------------------------------------------------------

## new feature

  1. 2016-11-17 支持命令行启动程序；回显控制，启动脚步控制；

  2. 2016-11-10 内置 `gumbo-node` 类型，以支援更丰富的 `html` 信息抽取

  3. debug模式 用內建函数开关debug模式，以便查看解释器动作流程。

## TODO

1. 增加 (help 指令) 具体的帮助文档，放在哪里呢？外部文件还是内部？ 内部的话，需要随时重新编译；
   外部的话，与实际的定义又分开了。最好是，能够自动抽取，然后放在外部文件中。

