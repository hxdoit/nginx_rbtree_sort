/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file ngx_http_rbtree_module.h
 * @author huangxuan01(com@baidu.com)
 * @date 2014/12/07 23:59:30
 * @brief 
 *  
 **/




#ifndef  __NGX_HTTP_RBTREE_MODULE_H_
#define  __NGX_HTTP_RBTREE_MODULE_H_

#include <stdio.h>

struct rb_node                                     
{
    struct rb_node *rb_parent;
    int rb_color;
    int child_num;
    int score;
#define RB_RED          0
#define RB_BLACK        1
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};



struct rb_node* create_node(void);



void log_out(char*);







#endif  //__NGX_HTTP_RBTREE_MODULE_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
