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

## TODO

1. 增加log输出——对于一些，没有显示的操作，可以用高亮形式，输出结果。比如(write 外部文件等等）

2. 增加 shell指令执行；常用指令，内置；比如(cd ...) , (mv ... ...), (rm ...) 等等。

3. 增加 (help 指令) 具体的帮助文档，放在哪里呢？外部文件还是内部？ 内部的话，需要随时重新编译；
   外部的话，与实际的定义又分开了。最好是，能够自动抽取，然后放在外部文件中。

4. gumbo-query 不是很好用——它原本的功能是，枚举出，符合检索条件的html-tag。但对于我来说，很
   多时候，只是要获取，比如innerText；
   这里，我直接定死，得到的就是转换后的可打印文本！

   有两种方案，一种是，让CNode 成为基本类型，并且，添加gumbo-query的基本函数。这好像跑得比较远，
   要是这样的话，就完全不是lisp了。——当然，这种方案，也面临一个问题，就是定义域问题。CNode本
   身是不保存数据的，都保存在，由Gumbo-parser得到的Document中。

   另外一个方案，则是再增加函数——就无法链式计算了。
