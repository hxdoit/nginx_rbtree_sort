#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <sys/shm.h>
#include "ngx_http_rbtree_module.h" 
#include "rbtree.h" 
#include "errno.h" 


typedef struct {
    ngx_str_t output_words;
} ngx_http_rbtree_loc_conf_t;

// To process HelloWorld command arguments
static char* ngx_http_rbtree(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

// Allocate memory for HelloWorld command
static void* ngx_http_rbtree_create_loc_conf(ngx_conf_t* cf);

// Copy HelloWorld argument to another place
static char* ngx_http_rbtree_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);

// 获取文件行数
static unsigned int get_line_num(char *f)
{
	FILE*fr=fopen(f,"r");
	unsigned int num=0;
	while(!feof(fr))
		if(fgetc(fr)==10)
			++num;
	fclose(fr);
	return num;
}
//初始化时读文件插入
static void init_insert(char *f)
{
	FILE *fp;
	char text[100];
	fp = fopen(f,"r");
	while(fgets(text,100,fp)!=NULL)
	{
		rb_insert(atoi(text));
	}
	fclose(fp);
}

static int shmid;
static struct rb_node *cur_empty_node = NULL;
static struct rb_node *buffer_begin= NULL;
static int total_buffer_size = 0;
extern int errno;
static int root_loc_diff = 0;

struct rb_node* create_node()
{
	if ((unsigned long)cur_empty_node - (unsigned long)buffer_begin > total_buffer_size - sizeof(struct rb_node))
	{
		fprintf(stderr, "not enough buffer for a new node\n");
		return NULL;	
	}
	return cur_empty_node++; 
}


static ngx_int_t init_module(ngx_cycle_t *cycle) {
	int line_count = get_line_num("/tmp/scores.txt");
	fprintf(stderr, "line count:%d\n", line_count);
	total_buffer_size = (int)(1.1*line_count*sizeof(struct rb_node));
	fprintf(stderr, "total buffer size:%d\n", total_buffer_size);

	if ((shmid = shmget(IPC_PRIVATE, total_buffer_size, 0600)) < 0) {
		fprintf(stderr, "get shared memory error:%s\n", strerror(errno));
		return NGX_ERROR;
	}
	void *p;
	if ((p = shmat(shmid, 0, 0)) == (void*)-1) {
		fprintf(stderr, "master attach shared memory error\n");
		return NGX_ERROR;
	}
	cur_empty_node = buffer_begin = p;
	set_root(NULL);

	init_insert("/tmp/scores.txt");

	root_loc_diff = (unsigned long)get_root() - (unsigned long)buffer_begin;
	fprintf(stderr, "init insert done\n");

	return NGX_OK;
}
static ngx_int_t init_process(ngx_cycle_t *cycle) {
	void * p;
	if ((p = shmat(shmid, 0, 0)) == (void*)-1) {
		fprintf(stderr, "process attach shared memory error\n");
		return NGX_ERROR;
	}
	set_root((void *)((unsigned long)p+root_loc_diff));
	fprintf(stderr, "init process %p\n",p);
	return NGX_OK;
}



// Structure for the HelloWorld command
static ngx_command_t ngx_http_rbtree_commands[] = {
    {
        ngx_string("rbtree"), // The command name
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_http_rbtree, // The command handler
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_rbtree_loc_conf_t, output_words),
        NULL
    },
    ngx_null_command
};

// Structure for the HelloWorld context
static ngx_http_module_t ngx_http_rbtree_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_rbtree_create_loc_conf,
    ngx_http_rbtree_merge_loc_conf
};

// Structure for the HelloWorld module, the most important thing
ngx_module_t ngx_http_rbtree_module = {
    NGX_MODULE_V1,
    &ngx_http_rbtree_module_ctx,
    ngx_http_rbtree_commands,
    NGX_HTTP_MODULE,
    NULL,
    init_module,
    init_process,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};


static ngx_int_t ngx_http_rbtree_handler(ngx_http_request_t* r) {
    //获取GET参数中的score
    char args[20];
    ngx_snprintf(args, 19,"%V", &r->args);
    args[r->args.len]=0;
 
    char * p; 
    p = strtok (args,"&"); 
    while(p!=NULL) { 
	    if(strncmp(p, "score=", 6) ==0 ){
		    break;
	    }
	    p = strtok(NULL,"&");
    }
    //获取score对应的排名 
    int rank= -1;
    if (p)
	    rank = rb_get_rank(atoi(p+=6)); 
    
   
    ngx_int_t rc;
    ngx_buf_t* b;
    ngx_chain_t out[2];

    ngx_http_rbtree_loc_conf_t* hlcf;
    hlcf = ngx_http_get_module_loc_conf(r, ngx_http_rbtree_module);

    r->headers_out.content_type.len = sizeof("text/plain") - 1;
    r->headers_out.content_type.data = (u_char*)"text/plain";

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    out[0].buf = b;
    out[0].next = &out[1];

    b->pos = (u_char*)"rbtree, ";
    b->last = b->pos + sizeof("rbtree, ") - 1;
    b->memory = 1;

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    out[1].buf = b;
    out[1].next = NULL;

    char *rank_p = ngx_pcalloc(r->pool, 10);
    sprintf(rank_p,"%d", rank);

    b->pos = rank_p;
    b->last = rank_p + strlen(rank_p);
    b->memory = 1;
    b->last_buf = 1;

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = strlen(rank_p) + sizeof("rbtree, ") - 1;
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    return ngx_http_output_filter(r, &out[0]);
}

static void* ngx_http_rbtree_create_loc_conf(ngx_conf_t* cf) {
    ngx_http_rbtree_loc_conf_t* conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_rbtree_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->output_words.len = 0;
    conf->output_words.data = NULL;

    return conf;
}

static char* ngx_http_rbtree_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child) {
    ngx_http_rbtree_loc_conf_t* prev = parent;
    ngx_http_rbtree_loc_conf_t* conf = child;
    ngx_conf_merge_str_value(conf->output_words, prev->output_words, "Nginx");
    return NGX_CONF_OK;
}

static char* ngx_http_rbtree(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {
    ngx_http_core_loc_conf_t* clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_rbtree_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
    return NGX_CONF_OK;
}

