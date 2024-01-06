#include "header.h"

#define port "8742"
#define port2 "8743"


static thread_data* global_thread_data = nullptr;  //because mg_wakeup doesnt seem to pass things well

#define REPLY(REPL_TYPE) \
  p->response = thread_data::ResponseType::REPL_TYPE;\
  global_thread_data = p;\
  mg_wakeup(p->mgr, p->conn_id, p,sizeof(thread_data));\
  break;

#define REPLY_OK \
  p->body = mg_strdup(mg_str("OK"));\
  global_thread_data = p;\
  REPLY(Text);

static void *thread_HTTP(void *param) {
  thread_data *p = (thread_data *) param;
  
    //printf("URL [%.*s]\n\n",hm->uri.len,hm->uri.ptr);
    struct mg_str match_captures[4];

    //breakable 1 iteration (breakable if)
    do{
      if(p->method=='P') //POST
      {
        if(p->url.len<=1){
          REPLY_OK;
        }
        else if (mg_match(p->url, mg_str("/upload/#"), match_captures))
        {
          p->url = mg_str_c(match_captures[0]) ;//(match_captures[0]);
          p->hm.uri = p->url;
          p->body = mg_strdup(p->hm.body);
          REPLY(Upload);
        }
        else if (mg_match(p->url, mg_str("/shell"), match_captures))
        {
          auto commands = p->hm.body;
          printf("\nshell:%.*s\n",(int)commands.len,commands.ptr);
          

          // run a process and create a streambuf that reads its stdout and stderr
          proccess proc((char*)commands.ptr, redi::pstreams::pstdout | redi::pstreams::pstderr);
          

          p->body = mg_strdup(mg_str( proc.ssOut.str().c_str() ));//(match_captures[0]);
          REPLY(Text);
        //mg_http_reply(c, 200, "",);

        }
        else if (mg_match(p->url, mg_str("/bash"), match_captures))
        {
          auto commands = p->hm.body;

          char tmpPath[256] = {};
          int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
          sprintf(tmpPath,"sh /tmp/%ld-bash.sh",timestamp);
          /// thread unsafe?? [TODO]
          mg_file_write(&mg_fs_posix,tmpPath+3,commands.ptr,commands.len);

          printf("\nshell:%.*s\n",(int)commands.len,commands.ptr);
          printf("%s\n",tmpPath);
      
          // run a process and create a streambuf that reads its stdout and stderr
          proccess proc(tmpPath, redi::pstreams::pstdout | redi::pstreams::pstderr);
              
          p->body = mg_strdup(mg_str( proc.ssOut.str().c_str() ));//(match_captures[0]);
          REPLY(Text);
        
        //  mg_http_reply(c, 200, "",proc.ssOut.str().c_str());
        }
        else if (mg_match(p->url, mg_str("/writefile#/#"), match_captures))
        {
          auto args = match_captures[0];
          int mode = 0; // 0 = overwrite, 1 = append
          if(args.len>0 && args.ptr[0]=='-'){
            if(args.ptr[1]=='a') mode=1;
          }
          p->url = mg_str_c(match_captures[1]);


          //MG_STR_C(match_captures[0],path_c);
          //struct mg_fd *fd = mg_fs_open(&mg_fs_posix, path_c, MG_FS_WRITE);
          
          p->body = p->hm.body;
          if(mode==0){
            REPLY(Writefile);
          }else{
            REPLY(WritefileAppend);  
          }
          //mg_file_write(&mg_fs_posix, path_c.ptr, hm->body.ptr, hm->body.len);

            
          //printf("WRITEFILE OPTS: %.*s\n",(int)path_c.len,path_c);
          REPLY_OK;
        }
      }
      else if(p->method=='G') //GET
      {
        if(p->url.len<=1){ //ping
          REPLY_OK;
        }
        else if (mg_match(p->url, mg_str("/shell/#"), match_captures))
        {
          auto commands =  mg_str_c(match_captures[0]);//mg_str_c_decode(match_captures[0]);
          printf("\nshell:%.*s\n",(int)commands.len,commands.ptr);
          
          // run a process and create a streambuf that reads its stdout and stderr
          proccess proc((char*)commands.ptr, redi::pstreams::pstdout | redi::pstreams::pstderr);
          

          p->body = mg_strdup(mg_str( proc.ssOut.str().c_str() ));//(match_captures[0]);
          printf("\nreply:%s\n",p->body.ptr);
          printf("Done\n");
          REPLY(Text);
          //mg_http_reply(c, 200, "",proc.ssOut.str().c_str());
    

        }
        else if (mg_match(p->url, mg_str("/ls/#"), match_captures))
        {
          auto path_c =  mg_str_c(match_captures[0]);//mg_str_c_decode(match_captures[0]);
          std::ostringstream ssOut;
          for (const auto & entry : fs::directory_iterator(path_c.ptr)) {
            ssOut << entry.path() << std::endl;
          }

          p->body = mg_strdup(mg_str( ssOut.str().c_str() ));//(match_captures[0]);
          REPLY(Text);
        }
        else if (mg_match(p->url, mg_str("/ls-#/#"), match_captures))
        {
          auto path_c = mg_str_c(match_captures[1]);//mg_str_c_decode(match_captures[1]);
          auto args =  mg_str_c(match_captures[0]);// mg_str_c_decode(match_captures[0]);
          std::ostringstream ssOut;
          bool dirsOnly = (args.ptr[0]=='d');
          bool filesOnly = !dirsOnly;//(args.ptr[0]=='f');
          for (const auto & entry : fs::directory_iterator(path_c.ptr)) {
            bool isDir = entry.is_directory();
            if((isDir&&dirsOnly) || (isDir==false && filesOnly))
              ssOut << entry.path() << std::endl;
          }
          
          p->body = mg_strdup(mg_str( ssOut.str().c_str() ));//(match_captures[0]);
          REPLY(Text);
        
          //mg_http_reply(c, 200, "",ssOut.str().c_str());
        }
        else if (mg_match(p->url, mg_str("/readfile/#"), match_captures))
        {
          
          p->body = mg_str_c(match_captures[0]) ;//(match_captures[0]);
          REPLY(Download);
          
            // Use mg_http_reply() API function to generate JSON response. It adds a
            // Content-Length header automatically. In the response, we show
            // the requested URI and HTTP body:
            /*
            mg_http_reply(c, 200, "", "{%m:%m,%m:%m,%m:%d}\n",  // See mg_snprintf doc
                            MG_ESC("uri"), mg_print_esc, hm->uri.len, hm->uri.ptr,
                            MG_ESC("body"), mg_print_esc, hm->body.len, hm->body.ptr,
                            MG_ESC("time"), std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()
                            );
            */
            //ne moramo da cistimo, thread se obrise..
            // delete[] path_c;
            // free ((void *)match_captures[0].ptr);
        }
        else if (mg_match(p->url, mg_str("/json/#"), match_captures))
        {
          // Use mg_http_reply() API function to generate JSON response. It adds a
          // Content-Length header automatically. In the response, we show
          // the requested URI and HTTP body:
          REPLY_OK;
          /*
          mg_http_reply(c, 200, "", "{%m:%m,%m:%m,%m:%d}\n",  // See mg_snprintf doc
                          MG_ESC("uri"), mg_print_esc, hm->uri.len, hm->uri.ptr,
                          MG_ESC("body"), mg_print_esc, hm->body.len, hm->body.ptr,
                          MG_ESC("time"), std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()
                          );
          */
        }
        /*
        else if (mg_http_match_uri(hm, "/api/hi"))
        {
            // Use mg_http_reply() API function to generate JSON response. It adds a
            // Content-Length header automatically. In the response, we show
            // the requested URI and HTTP body:
            mg_http_reply(c, 200, "", "{%m:%m,%m:%m,%m:%d}\n",  // See mg_snprintf doc
                            MG_ESC("uri"), mg_print_esc, hm->uri.len, hm->uri.ptr,
                            MG_ESC("body"), mg_print_esc, hm->body.len, hm->body.ptr,
                            MG_ESC("time"), std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count() );
        }
        else
        {
            // For all other URIs, serve static content from the current directory
            struct mg_http_serve_opts opts = {.root_dir = "."};
            mg_http_serve_dir(c, hm, &opts);
        }
        */
      }
    }while(0);

  

  //mg_wakeup(p->mgr, p->conn_id, "hi!", 3);  // Respond to parent
  //free((void *) p->body.ptr);            // Free all resources that were
  //free(p);                                  // passed to us
  return NULL;
}
static void *thread_WS(void *param) {
  struct thread_data *p = (struct thread_data *) param;
  

    struct mg_str match_captures[4];

do{
    //mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
    if (mg_match(p->url, mg_str("/watch/#"), match_captures))
    {
        auto path_c =  mg_str_c(match_captures[0]); //mg_str_c_decode(match_captures[0]);
     
      printf("WATCHING %s\n\n",path_c.ptr);
     //   mg_ws_upgrade(c, hm, NULL);

      #define EVENT_SIZE  ( sizeof (struct inotify_event) )
      #define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

      int length, i = 0;
      int fd;
      int wd;
      char buffer[BUF_LEN];

      fd = inotify_init();

      if ( fd < 0 ) perror( "inotify_init" );
      

      wd = inotify_add_watch( fd, path_c.ptr, 
                            IN_MODIFY | IN_CREATE | IN_DELETE );
      //Blocking read
      length = read( fd, buffer, BUF_LEN );  
      if ( length < 0 ) {
        perror( "read" );
      }  

      while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
        if ( event->len ) {
          p->body = {
            new char[1024],
            0
          };
          p->body.len = snprintf((char*)p->body.ptr,1024,"{\"type\":\"WATCH_CHANGED\",\"files\":[\"%s\"]}",event->name);
          
          REPLY(Text);
          //free(p);
          //mg_ws_send(c, charBuf, len, WEBSOCKET_OP_TEXT);
        /*
          if ( event->mask & IN_CREATE ) {
            if ( event->mask & IN_ISDIR ) {
              printf( "The directory %s was created.\n", event->name );       
            }
            else {
              printf( "The file %s was created.\n", event->name );
            }
          }
          else if ( event->mask & IN_DELETE ) {
            if ( event->mask & IN_ISDIR ) {
              printf( "The directory %s was deleted.\n", event->name );       
            }
            else {
              printf( "The file %s was deleted.\n", event->name );
            }
          }
          else if ( event->mask & IN_MODIFY ) {
            if ( event->mask & IN_ISDIR ) {
              printf( "The directory %s was modified.\n", event->name );
            }
            else {
              printf( "The file %s was modified.\n", event->name );
            }
          }
        */

        }
        i += EVENT_SIZE + event->len;
      }

      ( void ) inotify_rm_watch( fd, wd );
      ( void ) close( fd );
      break;
    }

    }while(0);
  // mg_wakeup(p->mgr, p->conn_id, "hi!", 3);  // Respond to parent
  // free((void *) p->message.ptr);            // Free all resources that were
  // free(p);                                  // passed to us
  return NULL;
}


// Mongoose event handler function, gets called by the mg_mgr_poll()
static void handler_HTTP(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  // printf("fn ev data %d\n",(size_t)ev_data);
  // printf("fn fn data %d\n",(size_t)fn_data);
  if (ev == MG_EV_HTTP_MSG) {
    // The MG_EV_HTTP_MSG event means HTTP request. `hm` holds parsed request,
    // see https://mongoose.ws/documentation/#struct-mg_http_message
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    //printf("%.*s\n",hm->head.len,hm->head.ptr);


      // Multithreading code path

    //Get raw url from header "GET url HTTP/1.0"
    mg_str raw_url { strchr(hm->head.ptr,' ')+1, 0};
    raw_url.len = strchr(raw_url.ptr,' ') - raw_url.ptr;
    
    

      thread_data *data = new thread_data();  // Worker owns it
      data->conn_id = c->id;
      data->mgr = c->mgr;
      data->method = hm->method.ptr[0];
      data->response = thread_data::ResponseType::NotDone;
      
      data->hm = std::move(*hm);/*{
        .body = mg_strdup(hm->body),
        .head = mg_strdup(hm->head),
        .query = mg_strdup(hm->query)
      };*/

      data->body = mg_strdup(hm->message);               // Pass message
      data->url = mg_str_c_decode(raw_url);
      start_thread(thread_HTTP, data);  // Start thread and pass data
    
    
    printf( "URL[%.*s]\n",(int)data->url.len,data->url.ptr);

  }else if (ev == MG_EV_WAKEUP) {
    printf("Wakeup HTTP\n");

    //thread_data *data = (thread_data *) ev_data;
    thread_data*data = global_thread_data;

    if(data->response == thread_data::ResponseType::Text){
      mg_http_reply(c, 200, "",data->body.ptr);
      //mg_ws_send(c, data->body.ptr, data->body.len, WEBSOCKET_OP_TEXT);
    
    }else if (data->response == thread_data::ResponseType::Upload){
      mg_http_upload(c, &(data->hm), &mg_fs_posix, data->body.ptr, 99999); //path, max_size
      
    }else if (data->response == thread_data::ResponseType::Download){

      struct mg_http_serve_opts opts = {
        //.mime_types = "png=image/png",
      //  .extra_headers = "AA: bb\r\nCC: dd\r\n"
      };


      mg_http_serve_file(c, &(data->hm), data->body.ptr, &opts);
      printf("serving %s\n",data->body.ptr);

    }else if (data->response == thread_data::ResponseType::Writefile){
        mg_file_write(&mg_fs_posix, data->url.ptr, data->body.ptr, data->body.len);
    }else if (data->response == thread_data::ResponseType::WritefileAppend){
      std::ofstream outfile;

      outfile.open(data->url.ptr, std::ios_base::app); // append instead of overwrite
      outfile.write(data->body.ptr, data->body.len);
      outfile.close(); 
      //mg_file_write(&mg_fs_posix, data->url.ptr, data->body.ptr, data->body.len);
    }

    delete data;

    // struct mg_str *data = (struct mg_str *) ev_data;
    // mg_http_reply(c, 200, "", "Result: %.*s\n", data->len, data->ptr);
  }
}

// Mongoose event handler function, gets called by the mg_mgr_poll()
static void handler_WS(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{

  
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    mg_ws_upgrade(c, hm, NULL);
  }
  else if (ev == MG_EV_WAKEUP) 
  {
    //printf("Wakeup WS\n");
    //thread_data *data = (thread_data *) ev_data;
    thread_data *data = global_thread_data;
    if(data->response == thread_data::ResponseType::Text){
      mg_ws_send(c, data->body.ptr, data->body.len, WEBSOCKET_OP_TEXT);
      delete data->body.ptr;
    }

    delete data;
    //mg_http_reply(c, 200, "", "Result: %.*s\n", data->len, data->ptr);
  }
  else if (ev == MG_EV_WS_MSG)
  {

    
    // Got websocket frame. Received data is wm->data. Echo it back!
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
   
      thread_data *data = new thread_data();//(thread_data*) calloc(1, sizeof(*data));  // Worker owns it
      data->conn_id = c->id;
      data->mgr = c->mgr;
      data->method = 'W';
      data->response = thread_data::ResponseType::NotDone;

      data->body = {};//mg_str_c(wm->data);//mg_strdup(hm->message);               // Pass message
      data->url = mg_str_c(wm->data);
      
      start_thread(thread_WS, data);  // Start thread and pass data

  }
}

int main()
{
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);        // Init manager
  mg_log_set(MG_LL_DEBUG);  // Set debug log level. Default is MG_LL_INFO
  mg_http_listen(&mgr, "http://0.0.0.0:" port, handler_HTTP, NULL);  // Setup HTTP listener
  mg_http_listen(&mgr, "ws://0.0.0.0:" port2, handler_WS, NULL);  // Setup WS listener
  mg_wakeup_init(&mgr);  // Initialise wakeup socket pair
  for (;;) mg_mgr_poll(&mgr, 400);                       // Infinite event loop
  // no mgr cleanup, leaving
  return 0;
}
