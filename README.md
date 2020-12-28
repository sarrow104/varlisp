# varlisp
a lisp syntax like mini interpreter

## based no these Library:

- [cpp-linenoise](https://github.com/sarrow104/cpp-linenoise)
- boost::variant
- boost::asio  (http-get url)
- gumbo-parser
- gumbo-query  (gumbo-query content query-string)
- posix regex
- sarrow104/ss1x
- sarrow104/sss

## extra builin-function list:

### arithmetic
  - `(+ ...) -> number`
  - `(- arg ...) -> number`
  - `(* ...) -> number`
  - `(/ arg ...) -> number`
  - `(% arg ...) -> number`
  - `(power arg1 arg2) -> number`

### bitop
  - `(& int1 int2 ...) -> int64_t`
  - `(| int1 int2 ...) -> int64_t`
  - `(>> int64_t shift) -> int64_t`
  - `(<< int64_t shift) -> int64_t`
  - `(~ int64_t) -> int64_t`
  - `(^ int1 int2 ...) -> int64_t`

### logic
  - `(= obj1 obj2) -> #t | #f`
  - `(!= obj1 obj2) -> #t | #f`
  - `(> obj1 obj2) -> #t | #f`
  - `(< obj1 obj2) -> #t | #f`
  - `(>= obj1 obj2) -> #t | #f`
  - `(<= obj1 obj2) -> #t | #f`
  - `(not expr) -> !#t | !#f`
  - `(equal '(list1) '(list2)) -> #t | #f`

### defer
  - `(defer (expr)) -> nil`

### encoding
  - `(uchardet "content") -> "utf8"`
  - `(pychardet "content") -> "utf8"`
  - `(ivchardet "encodings" "content") -> "utf8"`
  - `(iconv "enc-from" "enc-to" "content") -> "converted-out"`
  - `(ensure-utf8 "content") -> "utf8-content"`

### errno
  - `(errno) -> int64_t`
  - `(strerr) -> string`

### eval
  - `(eval '(...)) -> ...`

### file
  - `(read-all "path/to/file") -> string`
  - `(write content "path/to/file")`
  - `(write-append content "path/to/file")`
  - `(open "path" flag) -> file_descriptor | nil`
  - `(getfdflag fd) -> flag | nil`
  - `(setfdflag fd flag) -> errno`
  - `(close file_descriptor) -> errno`
  - `(read-line file_descriptor) -> string | nill`
  - `(read-char file_descriptor) -> int64_t | nill`
  - `(write-char file_descriptor int64_t) -> int64_t | nill`
  - `(write-string file_descriptor string) -> int64_t | nill`
  - `(list-opend-fd) -> [(fd name)...] | []`

### string
  - `(split "string-to-be-split")`
  - `(split "string-to-be-split" "sepeator-char")`
  - `(join (content list))`
  - `(join (content list) "sepeator-string")`
  - `(substr-byte "target-string" offset)`
  - `(substr-byte "target-string" offset length) -> sub-str`
  - `(substr "target-string" offset)`
  - `(substr "target-string" offset length)`
  - `(ltrim "target-string") -> "left-trimed-string"`
  - `(rtrim "target-string") -> "right-trimed-string"`
  - `(trim "target-string") -> "bothsides-trimed-string"`
  - `(strlen "target-string") -> length`
  - `(strlen-byte "target-string") -> length`
  - `(split-char "target-string") -> '(int64_t-char1 int64_t-char2 ...)`
  - `(join-char '(int64_t-char1 int64_t-char2 ...)) -> "string"`
  - `(byte-nth int64_nth "string" -> int64_t)`
  - `(char-nth int64_nth "string" -> int64_t)`
  - `(strstr "source" "needle") -> offset-int | nil`
  - `(is-begin-with "source" "needle") -> boolean`
  - `(is-end-with "source" "needle") -> boolean`

### http 
  - `(http-timeout) -> cur-timeout-in-seconds`
  - `(http-timeout new-timeout-in-seconds) -> old-timeout-in-seconds`
  - `(http-debug) -> cur-http-debug-status`
  - `(http-debug new-http-debug-status) -> old-http-debug-status`
  - `(http-get "url") -> [<html>, {response}]`
  - `(http-get "url" {request_header}) -> [<html>, {response}]`
  - `(http-get "url" "proxy-url" proxy-port-number) -> [<html>, {response}]`
  - `(http-get "url" "proxy-url" proxy-port-number {request_header}) -> [<html>, {response}]`
  - `(http-post "url" content) -> [<html>, {response}]`
  - `(http-post "url" content {request_header}) -> [<html>, {response}]`
  - `(http-post "url" content "proxy-url" proxy-port-number) -> [<html>, {response}]`
  - `(http-post "url" content "proxy-url" proxy-port-number {request_header}) -> [<html>, {response}]`

### html-gumbo
  - `(gumbo "\<html\>") -> gumboNode`
  - `(gumbo "\<html\>" "query-string") -> '(gumboNode)`
  - `(gumbo-query gumboNode "selector-string") -> '(gumboNodes)`
  - `(gumbo-query "html-file-content" "gumbo-query-string")`
  - `(gumbo-children gumboNode) -> '(gumboNodes)`
  - `(gqnode-indent) -> "current-indent"`
  - `(gqnode-indent "new-indent") -> "new-accept-indent"`
  - `(gqnode-attr gumboNode "attrib-name") -> "attrib-value" | nil`
  - `(gqnode-hasAttr gumboNode "attrib-name") -> #t | #f`
  - `(gqnode-valid gumboNode) -> boolean`
  - `(gqnode-isText gumboNode) -> boolean`
  - `(gqnode-text gumboNode) -> "text"`
  - `(gqnode-textNeat gumboNode) -> "text"`
  - `(gqnode-ownText gumboNode) -> "text"`
  - `(gqnode-tag gumboNode) -> "text"`
  - `(gqnode-innerHtml gumboNode) -> "text"`
  - `(gqnode-outerHtml gumboNode) -> "text"`
  - `(gumbo-query-text "<html>" "selector-string")`
  - `(gumbo-original-rewrite) -> boolean`
  - `(gumbo-original-rewrite boolean) -> boolean`
  - `(gumbo-rewrite int-fd '(gq-node) "") -> nil`
  - `(gumbo-rewrite int-fd {request_header} '(gq-node) "") -> nil`
  - `(gumbo-rewrite [int-fd proxy-domain proxy-port] '(gq-node) "") -> nil`
  - `(gumbo-rewrite [int-fd proxy-domain proxy-port] {request_header} '(gq-node) "") -> nil`

### interpreter
  - `(quit)    ->  #t`
  - `(it-debug #t|#f) -> nil`
  - `(colog-format CL_ELEMENT) -> current-format-mask`

### list
  - `(car (list item1 item2 ...)) -> item1`
  - `(cdr '(list item1 item2 ...)) -> '(item2 item3 ...)`
  ...
  - `(cons 1 (cons 2 '())) -> '(1 2)`
  - `(length '(list)) -> quote-list-length`
  - `(empty? '(list)) -> boolean`
  - `(append '(list1) '(list2)) -> '(list1 list2)`
  - `(flat '(list...)) -> '(list...)`
  - `(slice '(list) begin end)`
  - `(slice '(list) begin end step)`
  - `(range begin end)`
  - `(range begin end step)`
  - `(find item '(list)) -> index | nil`
  - `(find item '(list) operation) -> index | nil`

### load
  - `(load "path/to/lisp") -> nil`
  - `(save "path/to/lisp") -> item-count`
  - `(clear) -> item-count`

### regex
  - `(regex "regex-string") -> regex-obj`
  - `(regex-match reg-obj "target-string") -> boolean`
  - `(regex-search reg-obj "target-string") -> (list sub0, sub1 ...)`
  - `(regex-search reg-obj "target-string" offset-in-number) -> (list sub0, sub1 ...)`
  - `(regex-replace reg-obj "target-string") -> string`
  - `(regex-replace reg-obj "target-string" "format-string") -> string`
  - `(regex-replace reg-obj "target-string" functor) -> string`

  - `(regex-split sep-reg "target-string")`
  - `(regex-collect reg "target-string")`
  - `(regex-collect reg "target-string" "fmt-string")`

### path
  - `(path-fnamemodify "path/string" "path modifier") -> "modified-fname"`
  - `(path-append part1 part2) -> part1/part2`
  - `(glob "paht/to/explorer") -> '("fname1", "fname2", ...)`
  - `(glob-recurse "paht/to/explorer" "fname-filter" depth) -> '("fname1", "fname2", ...)`
  - `(file? "path/to/file") -> boolean`
  - `(directory? "path/to/directory") -> boolean`
  - `(expand "$var-path") -> "expand-path"`

### print fmt
  - `(io-print "fmt" ...)`
  - `(io-print-ln "fmt\n" ...)`
  - `(io-fmt "fmt-str" arg1 arg2 ... argn) -> "fmt-out"`
  - `(format stream-fd "fmt-str" arg1 arg2 ... argn) -> ...`
  - `(fmt-escape "normal-string-may-have-curly-bracket") -> "scaped-string"`

### shell
  - `(shell "") -> '(stdout, stderr)`
  - `(shell "" arg1 arg2 arg3) -> '(stdout, stderr)`
  - `(shell-cd "path/to/go") -> "new-work-dir"`
  - `(system "") -> return-code`
  - `(system "" arg1 arg2 arg3) -> return-code`
  - `(shell-mkdir "path/to/make") -> "full/path" | nil`
  - `(shell-pwd) -> "current-working-dir"`
  - `(shell-env) -> {(kev \"value\")...}`
  - `(shell-env key) -> "value-string" | nil`
      ...

### sort
  - `(sort func '(list)) -> '(sorted-list)`
  - `(sort! func '(list)) -> nil` # NOTE in-place sort

### time
  - `(time-elapsed expr) -> result-of-expr`
  - `(date) -> [year month day]`
  - `(date-time) -> [year month day HH MM SS]`
  - `(date-time seconds-since-Epoch) -> [year month day HH MM SS]`
  - `(date-time-nano) -> [year month day HH MM SS nano]`
  - `(time-format \"fmt\" '()) -> string`
  - `(time-strparse \"27 December 2016, at 13:17\" \"%d %h %Y, at %H:%M\") -> '(time-list)`
  - `(time expr) -> result-of-expr`

### type
  - `(typeid type) -> id`
  - `(number? type) -> #t | #f`
    ...
  - `(null? nil) -> #t`
  - `(cast lexical-value var) -> type-of-lexical-value`

### help 
  - `(help symbol) -> nil`
  - `(get-help symbol) -> string`

### variable
  - `(undef symbol) -> boolean`
  - `(ifdef symbol) -> boolean`
  - `(var-list) -> int64_t`
  - `(let ((symbol expr)...) (expr)...) -> result-of-last-expr`
  - `(letn ((symbol expr)...) (expr)...) -> result-of-last-expr`
  - `(set 'quote-symbal expr) -> value-of-expr`
  - `(setq symbol1 expr1 symbol2 expr2 ... ) -> value-of-last-expr`
  - `(setf "varname" expr) -> nil`
  - `(swap var1 var2) -> nil`
  - `(locate {}-or-[]-value '(string-or-int...)) -> sub-value`
  - `(symbols) -> [current-symbol-list]]`
  - `(symbols {}-name) -> [target-{}-symbol-list]`

### map,reduce,filter
  - `(map func list-1 list-2 ... list-n) -> '(func(l1[1] l2[1] ... ln[1]) func(l1[2] l2[2] ... ln[2]) ...  func(l1[n] l2[n] ... ln[n]))`
  - `(reduce func list) -> func(func(func(l[1] l[2]) l[3]) ... l[n-1]) l[n])`
  - `(filter func list) -> (sigma list[i] where (func list[i]) == #t)`
  - `(transform func list) -> '(func(car-nth i list) ... )`

### json
  - `(json-print obj boolean) -> nil`
  - `(json-string obj boolean) -> nil`
  - `(json-indent) -> "current-json-indent-setting"`
  - `(json-indent "string") -> "json-indent-setting"`
  - `(apply func '(args...))`

### lambda
  - `(call? symbol) -> boolean`
  - `(signature func-symbol) -> [min, max, "help_msg"] | nil`
  - `(curry func var...) -> (lambda ($1) (apply func '($1 var...)))`
  - `(partial func var... $1...) -> (lambda ($1...) (apply f '(var... $...)))`

### min,max
  - `(min [obj1 obj2...]) -> the-minimum-element`
  - `(min obj1 obj2...) -> the-minimum-element`
  - `(max [obj1 obj2...]) -> the-maximum-element`
  - `(max obj1 obj2...) -> the-maximum-element`

### encrypt
  - `(en-base64 "string") -> "enc"`
  - `(de-base64 "string") -> "decode"`
  - `(en-gzip "string") -> "enc"`
  - `(de-gzip "string") -> "dec"`
  - `(deflate "string") -> "dec"`
  - `(inflate "string") -> "dec"`

### digest
  - `(digest-sha1-file "path/to/file") -> "sha1-string" | nil`
  - `(digest-hex-string "bytes-content-to-hex") -> "hex-string"`

### exception
  - `(catch expr (value1)...) -> result-of-expr | exception-value`
  - `(catch expr (value1 on_except_type1)...) -> result-of-expr | result-of-on_value_handle-on-exception-value`
  - `(throw value) -> nil`
  - `(std-exception expr) -> result-of-expr`

### mime type
  - `(mime-file "path-to-file") -> mime-type-string`
  - `(mime-buffer "string-buffer-to-detect") -> mime-type-string`

### url util
  - `(url-split "url-string") -> '(protocal domain port path {parameters})`
  - `(url-join '(protocal domain port path {parameters})) -> "url-string"`
  - `(url-full target-string mapping-url) -> "full-url-string"`
  - `(url-encode "string") -> "encoded-string"`
  - `(url-decode "encoded-string") -> "string"`

### loop
 - for
   - `(for (var s-list) expr...) -> result-of-last-expr`
   - `(for (var ini-value end-value) expr...) ->result-of-last-expr`
   - `(for (var ini-value end-value step) expr...) -> result-of-last-expr`
   - `(for ((v1 a1 v2 a2...) condition (iterate-expr)) expr...) -> result-of-last-expr`
 - `(begin expr...) -> result-of-last-expr`
 - `(silent expr...) -> result-of-last-expr`
 - `(tie ('var-list...) [list...])`

### cookie
  - `(cookie-enable boolean) -> boolean`
  - `(cookie-enable?) -> boolean`
  - `(cookie-get-value "domain" "path") -> "cookie-value" | nil`
  - `(cookie-set-value "domain" "path" "cookie") -> boolean`

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
