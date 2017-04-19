step1:
in nginx.conf add:
location /rbtree {
     rbtree on;
}
step2:
add new file /tmp/scores.txt, its content like bellow:
19
18
17
16
15
14
13
12
11
10
9
8
7
6
5
4
3
2
1
every number represents a score

step3:
compile nginx,
./configure --prefix=/home/work/nginx_real --add-module=ngx_http_rbtree_module/ 

step4:
start nginx


step5:
visit: http://10.100.45.234:8156/rbtree?score=24
respone: rbtree, 25

