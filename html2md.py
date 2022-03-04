'''
Author: your name
Date: 2021-12-03 12:25:05
LastEditTime: 2021-12-07 17:20:27
LastEditors: Please set LastEditors
Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
FilePath: /undefined/Users/sarrow/ubuntu-home-back/project/Lisp/varLisp/src/html2md.py
'''
#!/usr/bin/python3

import html2text
import io
import sys

content = sys.stdin.read()

#with open('Kotlin.htm', 'w') as snap:
#    snap.write(content)
html2text.config.BODY_WIDTH = 0

mkd = html2text.html2text(content)

#with open('kotlin.md', 'w') as snap:
#    snap.write(mkd)

print(mkd)

#with open('How can we use CoroutineScopes in Kotlin 2.html', 'r') as file:
#    content = file.read()

#> echo "<h1>hello</h1>" | python3 html2md.py
#> # hello

# (define url "https://medium.com/swlh/introduction-to-flow-channel-and-shared-stateflow-e1c28c5bc755")
# (define g-doc (pipe-run (http-get url) car gumbo))
# (define content (gqnode-innerHtml (car (gumbo-query g-doc "article"))))
# ; (write "out.htm" content)
# ; (car (shell-pipe (expand "python3 $root/html2md.py") content))
# (format 1 "{}" (car (shell-pipe (expand "python3 $root/html2md.py") content)))
# (write "out.md" (car (shell-pipe (expand "python3 $root/html2md.py") content)))
# $ cat out.htm | python3 html2md.py

# 需要调整gqnode.innerHtml() 方法，目的，不添加任何东西，在打印的时候；

## 源码层面的宏支持——即，ast下的替换；代码生成；

# (define url "https://medium.com/swlh/introduction-to-flow-channel-and-shared-stateflow-e1c28c5bc755") (define g-doc (pipe-run (http-get url) car gumbo)) (define content (gqnode-innerHtml (car (gumbo-query g-doc "article")))) (write "out.md" (car (shell-pipe (expand "python3 $root/html2md.py") content)))
