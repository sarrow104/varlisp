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

  - (split "string-to-be-split")
  - (split "string-to-be-split" "sepeator-char")
  - (join (content list))
  - (join (content list) "sepeator-string")

  - (http-get "url-string")
  - (gumbo-query "html-file-content" "gumbo-query-string")

  - (regex "regex-string")
  - (regex-match reg-obj "target-string")
  - (regex-search reg-obj "target-string")
  - (regex-search reg-obj "target-string" offset-in-number)
  - (regex-replace reg-obj "target-string" "format-string")

  - (regex-replace reg-obj "target-string" "format-string")
  - (regex-replace reg-obj "target-string" "format-string")

  - (regex-split sep-reg "target-string")
  - (regex-collect reg "target-string")
  - (regex-collect reg "target-string" "fmt-string")

  - (substr "target-string" offset)
  - (substr "target-string" offset length)

----------------------------------------------------------------------

## sample output

```lisp
\> (regex-collect (regex "[0-9]+") "123 abc 1645 798a8709801")
("123" "1645" "798" "8709801")

\> (regex-split (regex "[a-zA-Z ]+") "123 abc 1645 798a8709801")
("123" "1645" "798" "8709801")
```
just a toy
