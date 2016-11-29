# varlisp
a lisp syntax like mini interpreter

## based no these Library:

- [cpp-linenoise](https://github.com/sarrow104/cpp-linenoise)
- boost::variant
- boost::asio  (http-get url)
- gumbo-parser
- gumbo-query  (gumbo-query content query-string)
- posix regex
- sarrow104::ss1x(not upload yet)
- sarrow104::sss(not upload yet)

## extra builin-function list:

### arithmetic
  - (+ ...) -> number
  - (- arg ...) -> number
  - (* ...) -> number
  - (/ arg ...) -> number
      ...

### bitop
  - (& int1 int2 ...) -> int
  - (| int1 int2 ...) -> int
  - (>> int shift) -> int
  - (<< int shift) -> int
  - (~ int) -> int
  - (^ int1 int2 ...) -> int

### logic
  - (= obj1 obj2) -> #t | #f
  - (> obj1 obj2) -> #t | #f
    ...
  - (not expr) -> !#t | !#f
  - (equal '(list1) '(list2)) -> #t | #f

### defer
  - (defer (expr)) -> result-ignored

### encoding
  - (uchardet "content") -> "utf8"
      ...
  - (ensure-utf8 "content") -> "utf8-content"

### errno
  - (errno) -> int
  - (strerr) -> string

### eval
  - (eval '(...)) -> ...

### file
  - (read-all "path/to/file") -> string
  - (write content "path/to/file")
  - (write-append content "path/to/file")
  - (open "path" flag) -> file_descriptor | nil
  - (getfdflag fd) -> flag | nil
  - (close file_descriptor) -> errno
  - (read-line file_descriptor) -> string | nill
  - (read-char file_descriptor) -> int | nill
  - (write-char file_descriptor int) -> int | nill
  - (write-string file_descriptor string) -> int | nill

### string
  - (split "string-to-be-split")
  - (split "string-to-be-split" "sepeator-char")
  - (join (content list))
  - (join (content list) "sepeator-string")
  - (substr "target-string" offset)
  - (substr "target-string" offset length)
  - (split-char "target-string")
      -> '(int-char1 int-char2 ...)
  - (join-char '(int-char1 int-char2 ...))
      -> "string"

### http 
  - (http-get "url-string")
  - (http-get "url-string" "proxy-url" port-number)

### html-gumbo
  - (gumbo "\<html\>") -> gumboNode
  - (gumbo "\<html\>" "query-string") -> '(gumboNode)
  - (gumbo-query gumboNode "selector-string") -> '(gumboNodes)
  - (gqnode-attr gumboNode "attrib-name") -> "attrib-value" | nil 
  - (gqnode-hasAttr gumboNode "attrib-name") -> #t | #f
      ...
  - (gumbo-query "html-file-content" "gumbo-query-string")

### interpreter
  - (quit)    ->  #t
  - (it-debug #t|#f) -> nil

### list
  - (car (list item1 item2 ...)) -> item1
  - (cdr '(list item1 item2 ...)) -> '(item2 item3 ...)
  ...
  - (cons 1 (cons 2 '())) -> '(1 2)
  - (append '(list1) '(list2)) -> '(list1 list2)

### load
  - (load "path/to/lisp") -> nil

### regex
  - (regex "regex-string")
  - (regex-match reg-obj "target-string")
  - (regex-search reg-obj "target-string")
  - (regex-search reg-obj "target-string" offset-in-number)
  - (regex-replace reg-obj "target-string" "format-string")

  - (regex-split sep-reg "target-string")
  - (regex-collect reg "target-string")
  - (regex-collect reg "target-string" "fmt-string")

### path
  - (fnamemodify "path/string" "path modifier") -> "modified-fname"
  - (glob "paht/to/explorer") -> '("fname1", "fname2", ...)
  - (glob-recurse "paht/to/explorer" "fname-filter" depth)
         -> '("fname1", "fname2", ...)

### fmt
  - (format stream-fd "fmt-str" arg1 arg2 ... argn) -> ...
    ...

### shell
  - (shell "" arg1 arg2 arg3) -> '(stdout, stderr)
  - (shell-cd "path/to/go") -> "new-work-dir"
      ...

### sort
  - (sort func '(list)) -> '(sorted-list)

### time
  - (time expr) -> result-of-expr

### type
  - (typeid type) -> id
  - (number? type) -> #t | #f
    ...
  - (null? nil) -> #t

### help 
  - (help symbol) -> nil
  - (get-help symbol) -> string

### variable
  - (undef symbol) -> boolean
  - (ifdef symbol) -> boolean
  - (var-list) -> int

### map,reduce,filter
  - (map func list-1 list-2 ... list-n)
      -> '(func(l1[1] l2[1] ... ln[1])
           func(l1[2] l2[2] ... ln[2])
           ...
           func(l1[n] l2[n] ... ln[n]))

  - (reduce func list) 
      -> func(func(func(l[1] l[2]) l[3]) ... l[n-1]) l[n])

  - (filter func list)
      -> (sigma list[i] where (func list[i]) == #t)

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

  4. 2016-11-25 http-get支持301,302自动跳转；支持通过proxy下载https资源。

  5. 2016-11-09 支持map,reduce,filter算法

  6. 2016-11-10 內建gumbo-node，将gumbo-query算法内置；

  7. 2016-11-29 內建帮助系统；命令(help symbol)即可显示內建函数，以及自定义函数的帮助信息。

## TODO

    ...
