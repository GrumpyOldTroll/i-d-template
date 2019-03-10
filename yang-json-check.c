/*
 * Copyright 2019, Akamai Technologies, Inc.
 * All rights reserved.
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Akamai. The name of 
 * Akamai may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

git clone https://github.com/CESNET/libyang
mkdir libyang/build
cd libyang/build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/local-installs ..
make && make install
cd ../..
gcc -o yang-json-check -I $HOME/local-installs/include yang-json-check.c -L $HOME/local-installs/lib -lyang
export LD_LIBRARY_PATH=$HOME/local-installs/lib
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libyang/libyang.h>

#define error_msg(msg, ...) fprintf(stderr, "%s:%u: " msg "\n", __FILE__, __LINE__, __VA_ARGS__)

#define ERR_INPUT_FAILURE -1
#define ERR_EXPECTATIONS_VIOLATED -2

struct ly_ctx* load_parser(const char* in_yang_file);
int read_model(struct ly_ctx* ctx, const char* yang_file);

struct lyd_node* read_data_file(struct ly_ctx* ctx, const char* json_file);

int examine_data(const struct lyd_node* root);

int main(int nargs, const char** args) {
  if (nargs != 3) {
    error_msg("usage: %s <input.yang> <input.json>\n", args[0]);
    return -1;
  }

  const char* model_path = args[1];
  struct ly_ctx* ctx = load_parser(model_path);
  if (!ctx) {
    return -1;
  }

  const char* data_path = args[2];
  struct lyd_node* in_data = read_data_file(ctx, data_path);
  if (!in_data) {
    ly_ctx_destroy(ctx, 0);
    return -2;
  }

  printf("read data file %s successfully\n", data_path);

  int rc = examine_data(in_data);
  if (rc != 0) {
    ly_ctx_destroy(ctx, 0);
    return -4;
  }

  lyd_free(in_data);
  ly_ctx_destroy(ctx, 0);
  return 0;
}

struct lyd_node* read_data_file(struct ly_ctx* ctx, const char* json_file) {
  LYD_FORMAT data_fmt = LYD_JSON;
  int options = LYD_OPT_CONFIG;
  struct lyd_node* in_data = lyd_parse_path(ctx, json_file, data_fmt, options);
  if (!in_data) {
    error_msg("failed lyd_parse_path, json_file=%s", json_file);
    return 0;
  }

  return in_data;
}

struct ly_ctx* load_parser(const char* in_yang_file) {
  const char* search_dir = "./modules";
  int options = LY_CTX_NOYANGLIBRARY | LY_CTX_ALLIMPLEMENTED;
  struct ly_ctx* ctx = ly_ctx_new(search_dir, options);
  if (!ctx) {
    error_msg("failed ly_ctx_new call (search_dir=%s, options=0x%x)",
              search_dir ? search_dir : "null", options);
    return 0;
  }

  if (0 != read_model(ctx, in_yang_file)) {
    ly_ctx_destroy(ctx, 0);
    return 0;
  }

  return ctx;
}

int read_model(struct ly_ctx* ctx, const char* yang_file) {
  LYS_INFORMAT lys_fmt = LYS_IN_YANG;
  const struct lys_module *mod = lys_parse_path(ctx, yang_file, lys_fmt);
  if (!mod) {
    const char* fmt_desc;
    switch (lys_fmt) {
      case LYS_IN_YANG:
        fmt_desc = "yang";
        break;
      case LYS_IN_YIN:
        fmt_desc = "yin";
        break;
      default:
        fmt_desc = "unknown";
    }
    error_msg("failed lys_parse_mem, lys_fmt=%d(%s), yang_file=\n%s",
              (int)lys_fmt, fmt_desc, yang_file);
    return ERR_INPUT_FAILURE;
  }

  return 0;
}

int examine_data(const struct lyd_node* root) {

#if 1
  char* xml_out = 0;
  LYD_FORMAT out_fmt = LYD_XML;
  int out_options = LYP_FORMAT | LYP_WD_TRIM;
  int rc = lyd_print_mem(&xml_out, root, out_fmt, out_options);

  if (rc) {
    error_msg("failed lyd_print_mem: %d", rc);
    if (xml_out) {
      free(xml_out);
    }
    return rc;
  }

  printf("dumping input as xml:\n%s", xml_out);
  free(xml_out);
#endif

  return 0;
}

