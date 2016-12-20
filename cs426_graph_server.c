#include "mongoose.h"
#include "headers.h"

int head;
int tail;
char *ip_next;

typedef struct {
  Graph *graph;
} Data;


static void add_node(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;
  int status;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  if (!tail) {
    status = propogate(ADD_NODE, strtoull(tok->ptr, NULL, 10), 0);
    if (status == RPC_FAILED) {
      mg_printf(nc, "HTTP/1.1 500 RPC Failed\r\n");
      fprintf(stderr, "add_node: %.*s = %d\n", tok->len, tok->ptr, status);
      free(arr);
      return;
    }
  }

  status = graph->addNode(strtoull(tok->ptr, NULL, 10)); 

  //DEBUG
  fprintf(stderr, "add_node: %.*s = %d\n", tok->len, tok->ptr, status); 

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json); 
  } else if (status == EXISTS) {
    mg_printf(nc, "HTTP/1.1 204 OK\r\n");
    
  } else {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  }

  free(arr);
}

static void add_edge(struct mg_connection *nc, struct http_message *hm, void *user_data) {

  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;
  int status = 0;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_a_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  if (!tail) {
    fprintf(stderr, "Add_edge: not tail, about to propogate \n");
    status = propogate(ADD_EDGE, strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10));
    if (status == RPC_FAILED) {
      mg_printf(nc, "HTTP/1.1 500 RPC Failed\r\n");
      fprintf(stderr, "add_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);
      free(arr);
      return;
    }
  }

  status = graph->addEdge(strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10)); 

  fprintf(stderr, "Add_edge: added edge to graph\n");

  //DEBUG
  fprintf(stderr, "add_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status); 

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json);  
  } else if (status == EXISTS) {
    mg_printf(nc, "HTTP/1.1 204 OK\r\n");
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void remove_node(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;
  int status = 0;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  if (!tail) {
    fprintf(stderr, "Remove node: not tail, about to propogate\n");
    status = propogate(REMOVE_NODE, strtoull(tok->ptr, NULL, 10), 0);
    if (status == RPC_FAILED) {
      mg_printf(nc, "HTTP/1.1 500 RPC Failed\r\n");
      fprintf(stderr, "remove_node: %.*s = %d\n", tok->len, tok->ptr, status);
      free(arr);
      return;
    }
  }

  status = graph->removeNode(strtoull(tok->ptr, NULL, 10)); 

  //DEBUG
  fprintf(stderr, "remove_node: %.*s = %d\n", tok->len, tok->ptr, status);

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json); 
    
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
    
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void remove_edge(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;
  int status = 0;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_a_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  if (!tail) {
    fprintf(stderr, "Remove edge: not tail, about to propogate\n");
    status = propogate(REMOVE_EDGE, strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10));
    if (status == RPC_FAILED) {
      mg_printf(nc, "HTTP/1.1 500 RPC Failed\r\n");
      fprintf(stderr, "remove_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);
      free(arr);
      return;
    }
  }

  status = graph->removeEdge(strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10)); 

  //DEBUG
  fprintf(stderr, "remove_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json);  
    
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
    
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void get_node(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;

  std::pair<int, bool> result;
  int status;
  bool in_graph;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->getNode(strtoull(tok->ptr, NULL, 10));
  status = std::get<0>(result); 
  in_graph = std::get<1>(result);

  //DEBUG
  fprintf(stderr, "get_node: %.*s = %d\n", tok->len, tok->ptr, status);

  if (in_graph == true) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : T}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else if (in_graph == false) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : F}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else {
    mg_printf(nc, "Error creating JSON in_graph field \n");
    free(arr);
    return;
  }

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void get_edge(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;

  std::pair<int, bool> result;
  int status;
  bool in_graph;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_a_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->getEdge(strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10));
  status = std::get<0>(result); 
  in_graph = std::get<1>(result); 

  //DEBUG
  fprintf(stderr, "get_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);

  if (in_graph == true) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : T}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else if (in_graph == false) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : F}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else {
    mg_printf(nc, "Error creating JSON in_graph field \n");
    free(arr);
    return;
  }

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void get_neighbors(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;

  std::pair<int, std::string> result;
  int status;
  std::string neighbor_list;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->getNeighbors(strtoull(tok->ptr, NULL, 10));

  status = std::get<0>(result); 
  neighbor_list = std::get<1>(result);

  //DEBUG
  fprintf(stderr, "get_neighbors: %.*s = %d\n", tok->len, tok->ptr, status);

  json_buf_size = json_emit(buf, sizeof(buf), "{s : V, s : [S]}", 
                            "node_id", tok->ptr, tok->len, "neighbors", neighbor_list.c_str());
  assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void shortest_path(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;
  std::pair<int, uint64_t> result;
  int status;
  uint64_t distance;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  char distance_buf[22];

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->shortestPath(strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10));
  status = std::get<0>(result);
  distance = std::get<1>(result);

  //DEBUG
  fprintf(stderr, "shortest_path: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);

  snprintf(distance_buf, sizeof(distance_buf), "%lu", distance);

  json_buf_size = json_emit(buf, sizeof(buf), "{s : S}", "distance", distance_buf);
  assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else if (status == EXISTS) {
    mg_printf(nc, "HTTP/1.1 204 OK\r\n");
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

  if (ev == MG_EV_HTTP_REQUEST) {

    struct http_message *hm = (struct http_message *) ev_data;
    struct mg_str *uri = &(hm->uri);

    if (mg_vcmp(uri, "/api/v1/add_node") == 0) {
      add_node(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/add_edge") == 0) {
      add_edge(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/remove_node") == 0) {
      remove_node(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/remove_edge") == 0) {
      remove_edge(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/get_node") == 0) {
      get_node(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/get_edge") == 0) {
      get_edge(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/get_neighbors") == 0) {
      get_neighbors(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/shortest_path") == 0) {
      shortest_path(nc, hm, nc->mgr->user_data);
    } else {
      mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
    }

    nc->flags |= MG_F_SEND_AND_CLOSE;
  }
}

int main(int argc, char *argv[]) {

  if (argc != 8) {
    fprintf(stderr, 
      "Usage: ./cs426_graph_server <graph_server_port> -p <partnum> -l <partlist> \n");
    return 1;
  }

  // Parse arguments
  int part;
  char *port;
  char *part1_address;
  char *part2_address;
  char *part3_address;
  int c;

  while ((c = getopt(argc, argv, "p:l:")) != -1)
    switch (c)
      {
      case 'p':
        part = atoi(optarg);
        break;
      case 'l':
        part1_address = optarg;
        break;
      case '?':
        if (optopt == 'p')
          fprintf(stderr, "Option -%c requires an argument. \n", optopt);
        else if (isprint (optopt))
          fprintf(stderr, "Unknown option '-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        fprintf (stderr,"Aborting\n");
        abort();
      }

  if (argc - optind != 3) {
    fprintf(stderr, "Incorrect number of arguments\n");
    return 1;
  }
  port = argv[optind++];
  part2_address = argv[optind++];
  part3_address = argv[optind];
  fprintf(stderr, "Port: %s, part: %d, address1: %s, address2: %s, address3: %s\n", 
    port, part, part1_address, part2_address, part3_address);

  /*
  // Create new graph
  Graph *graph = new Graph();

  // RPC Server
  pthread_t rpc_thread;

  if (pthread_create(&rpc_thread, NULL, RunServer, graph)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  // HTTP Server
  Data *data = (Data *) malloc(sizeof(Data));
  data->graph = graph;

  struct mg_mgr mgr;
  struct mg_connection *nc;

  mg_mgr_init(&mgr, (void *) data);

  nc = mg_bind(&mgr, port, ev_handler);

  if (nc == NULL) {
    printf("Failed to create listener\n");
    return 1;
  }

  mg_set_protocol_http_websocket(nc);

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  // Free 
  free(data);
  */

  return 0;
}

