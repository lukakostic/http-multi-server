#include "depends/mongoose.h"
#include "depends/pstream.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;


// TODO   abstract process creation and control to a process class.

template class redi::basic_pstreambuf<char>;
template class redi::pstream_common<char>;
template class redi::basic_pstream<char>;
template class redi::basic_ipstream<char>;
template class redi::basic_opstream<char>;
template class redi::basic_rpstream<char>;


#define port "8742"
#define ARRAY_SIZE(FOO2) (sizeof(FOO2)/sizeof(FOO2[0]))
#define MG_STR_C(MG_STR,STR_NAME) char *STR_NAME = new char[MG_STR.len+1]; strncpy(STR_NAME,MG_STR.ptr,MG_STR.len); STR_NAME[MG_STR.len] = 0;
                                    //
#define PIPE_R 0
#define PIPE_W 1

// NULL terminated mg_str (null counted in length), from mg_str which gets url-decoded
template<int len=256>
static mg_str mg_str_c_decode(mg_str str)
{
  mg_str cstr = {};
  cstr.ptr = new char[len];
  cstr.len = mg_url_decode(str.ptr,(int)str.len,(char*)cstr.ptr,len,0);
  ((char*)cstr.ptr)[cstr.len] = 0;
  return cstr;
};

// NULL terminated mg_str (null counted in length), from mg_str
static mg_str mg_str_c(mg_str str)
{
  if(str.len != 0){
    if(str.ptr[str.len-1] == 0) return str;

    mg_str cstr = {};
    cstr.ptr = new char[str.len+1];
    cstr.len = mg_url_decode(str.ptr,(int)str.len,(char*)cstr.ptr,str.len,0);
    ((char*)cstr.ptr)[cstr.len] = 0;
    return cstr;
  }
  return {};
};

// Mongoose event handler function, gets called by the mg_mgr_poll()
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG) {
    // The MG_EV_HTTP_MSG event means HTTP request. `hm` holds parsed request,
    // see https://mongoose.ws/documentation/#struct-mg_http_message
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    struct mg_str match_captures[40];

    if(hm->method.ptr[0]=='P') //POST
    {

      if (mg_http_match_uri(hm, "/upload"))
      {
        mg_http_upload(c, hm, &mg_fs_posix, "/tmp/myfile.bin", 99999); //path, max_size
      }
      else if (mg_http_match_uri(hm, "/shell"))
      {
        auto commands = hm->body;
        printf("\nshell:%.*s\n",commands.len,commands.ptr);
        /*
        int pipefd[2];
        pipe(pipefd);
        int pid = vfork();
        if(pid==0){ //child
  dup2(pipefd[PIPE_W], STDOUT_FILENO);
  close(pipefd[PIPE_R]);
  close(pipefd[PIPE_W]);
        //  close(pipefd[PIPE_R]); //close read end in child
          //write(pipefd[PIPE_W], argv[1], strlen(argv[1]));
        // close(pipefd[PIPE_W]);          // Reader will see EOF 
        }else{
          close(pipefd[PIPE_W]); // close write end in parent
          char buf[1024];
          int len = 0;
          char c_buf;
          while (read(pipefd[PIPE_R], &c_buf, 1) > 0)
              buf[len++] = c_buf;
          
          close(pipefd[PIPE_R]);
          //wait(NULL);                // Wait for child
        }
        */

            using namespace redi;
/*
  char c;
  ipstream who("id -un");
  if (!(who >> c))
      return 1;

  redi::opstream cat("cat");
  if (!(cat << c))
      return 2;

  while (who >> c)
      cat << c;

  cat << '\n' << peof;

  pstream fail("ghghghg", pstreambuf::pstderr);
  std::string s;
  if (!std::getline(fail, s))
      return 3;
  std::cerr << s << '\n';
  
  rpstream who2(commands.ptr);
  
  if (!(who2.out() >> c))
      return 4;
*/
// run a process and create a streambuf that reads its stdout and stderr
redi::ipstream proc(commands.ptr, redi::pstreams::pstdout | redi::pstreams::pstderr);
std::ostringstream ssOut;
std::ostringstream ssErr;
std::string line;
// read child's stdout
ssOut << proc.out().rdbuf();
/*
while (std::getline(proc.out(), line))
  ssOut<<line;
  */
// if reading stdout stopped at EOF then reset the state:
if (proc.eof() && proc.fail())
  proc.clear();
ssErr << proc.out().rdbuf();
// read child's stderr
/*
while (std::getline(proc.err(), line))
  ssErr<<line;
*/
        //vfork, execve
//          mg_http_reply(c, 200, "","");
  mg_http_reply(c, 200, "",ssOut.str().c_str());
  

      }
      else if (mg_http_match_uri(hm, "/bash"))
      {
        auto commands = hm->body;

        char tmpPath[256] = {};
        int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        sprintf(tmpPath,"sh /tmp/%ld-bash.sh",timestamp);
        mg_file_write(&mg_fs_posix,tmpPath+3,commands.ptr,commands.len);

        // char chmodBuf[256] = {};
        // sprintf(chmodBuf,"chmod -x %s",tmpPath);
        // system(chmodBuf);

        printf("\nshell:%.*s\n",commands.len,commands.ptr);
        printf("%s\n",tmpPath);
        /*
        int pipefd[2];
        pipe(pipefd);
        int pid = vfork();
        if(pid==0){ //child
  dup2(pipefd[PIPE_W], STDOUT_FILENO);
  close(pipefd[PIPE_R]);
  close(pipefd[PIPE_W]);
        //  close(pipefd[PIPE_R]); //close read end in child
          //write(pipefd[PIPE_W], argv[1], strlen(argv[1]));
        // close(pipefd[PIPE_W]);          // Reader will see EOF 
        }else{
          close(pipefd[PIPE_W]); // close write end in parent
          char buf[1024];
          int len = 0;
          char c_buf;
          while (read(pipefd[PIPE_R], &c_buf, 1) > 0)
              buf[len++] = c_buf;
          
          close(pipefd[PIPE_R]);
          //wait(NULL);                // Wait for child
        }
        */

            using namespace redi;
/*
  char c;
  ipstream who("id -un");
  if (!(who >> c))
      return 1;

  redi::opstream cat("cat");
  if (!(cat << c))
      return 2;

  while (who >> c)
      cat << c;

  cat << '\n' << peof;

  pstream fail("ghghghg", pstreambuf::pstderr);
  std::string s;
  if (!std::getline(fail, s))
      return 3;
  std::cerr << s << '\n';
  
  rpstream who2(commands.ptr);
  
  if (!(who2.out() >> c))
      return 4;
*/
// run a process and create a streambuf that reads its stdout and stderr

redi::ipstream proc(tmpPath, redi::pstreams::pstdout | redi::pstreams::pstderr);
std::ostringstream ssOut;
std::ostringstream ssErr;
std::string line;
// read child's stdout
ssOut << proc.out().rdbuf();
/*
while (std::getline(proc.out(), line))
  ssOut<<line;
  */
// if reading stdout stopped at EOF then reset the state:
if (proc.eof() && proc.fail())
  proc.clear();
ssErr << proc.out().rdbuf();
// read child's stderr
/*
while (std::getline(proc.err(), line))
  ssErr<<line;
*/
        //vfork, execve
//          mg_http_reply(c, 200, "","");

  mg_http_reply(c, 200, "",ssOut.str().c_str());
  
std::cout<<ssOut.str();
  }
      else if (mg_match(hm->uri, mg_str("/writefile/#"), match_captures))
      {
            auto path_c = mg_str_c_decode(match_captures[0]);

            //MG_STR_C(match_captures[0],path_c);
            //struct mg_fd *fd = mg_fs_open(&mg_fs_posix, path_c, MG_FS_WRITE);
            
            mg_file_write(&mg_fs_posix, path_c.ptr, hm->body.ptr, hm->body.len);
              
            printf("WRITEFILE: %.*s\n",(int)path_c.len,path_c);
            mg_http_reply(c, 200, "","");
            
      }    

    }
    else if(hm->method.ptr[0]=='G') //GET
    {
          
      if (mg_match(hm->uri, mg_str("/shell/#"), match_captures))
      {
        auto commands = mg_str_c_decode(match_captures[0]);
        printf("\nshell:%.*s\n",commands.len,commands.ptr);
        /*
        int pipefd[2];
        pipe(pipefd);
        int pid = vfork();
        if(pid==0){ //child
  dup2(pipefd[PIPE_W], STDOUT_FILENO);
  close(pipefd[PIPE_R]);
  close(pipefd[PIPE_W]);
        //  close(pipefd[PIPE_R]); //close read end in child
        //write(pipefd[PIPE_W], argv[1], strlen(argv[1]));
        // close(pipefd[PIPE_W]);          // Reader will see EOF 
        }else{
          close(pipefd[PIPE_W]); // close write end in parent
          char buf[1024];
          int len = 0;
          char c_buf;
          while (read(pipefd[PIPE_R], &c_buf, 1) > 0)
              buf[len++] = c_buf;
          
          close(pipefd[PIPE_R]);
          //wait(NULL);                // Wait for child
        }
        */

          using namespace redi;
/*
  char c;
  ipstream who("id -un");
  if (!(who >> c))
      return 1;

  redi::opstream cat("cat");
  if (!(cat << c))
      return 2;

  while (who >> c)
      cat << c;

  cat << '\n' << peof;

  pstream fail("ghghghg", pstreambuf::pstderr);
  std::string s;
  if (!std::getline(fail, s))
      return 3;
  std::cerr << s << '\n';
  
  rpstream who2(commands.ptr);
  
  if (!(who2.out() >> c))
    return 4;
*/
// run a process and create a streambuf that reads its stdout and stderr
redi::ipstream proc(commands.ptr, redi::pstreams::pstdout | redi::pstreams::pstderr);
std::ostringstream ssOut;
std::ostringstream ssErr;
std::string line;
// read child's stdout
ssOut << proc.out().rdbuf();
/*
while (std::getline(proc.out(), line))
  ssOut<<line;
  */
// if reading stdout stopped at EOF then reset the state:
if (proc.eof() && proc.fail())
  proc.clear();
ssErr << proc.out().rdbuf();
// read child's stderr
/*
while (std::getline(proc.err(), line))
  ssErr<<line;
*/
        //vfork, execve
//          mg_http_reply(c, 200, "","");
  mg_http_reply(c, 200, "",ssOut.str().c_str());
  

      }
      else if (mg_match(hm->uri, mg_str("/ls/#"), match_captures))
      {
        auto path_c = mg_str_c_decode(match_captures[0]);
        // auto args = mg_str_c_decode(match_captures[1]);
        std::ostringstream ssOut;
        for (const auto & entry : fs::directory_iterator(path_c.ptr)) {
          ssOut << entry.path() << std::endl;
        }

          mg_http_reply(c, 200, "",ssOut.str().c_str());
      }
      else if (mg_match(hm->uri, mg_str("/ls-#/#"), match_captures))
      {
        auto path_c = mg_str_c_decode(match_captures[1]);
        auto args = mg_str_c_decode(match_captures[0]);
        std::ostringstream ssOut;
        bool dirsOnly = (args.ptr[0]=='d');
        bool filesOnly = !dirsOnly;//(args.ptr[0]=='f');
        for (const auto & entry : fs::directory_iterator(path_c.ptr)) {
          bool isDir = entry.is_directory();
          if((isDir&&dirsOnly) || (isDir==false && filesOnly))
            ssOut << entry.path() << std::endl;
        }
        

          mg_http_reply(c, 200, "",ssOut.str().c_str());
      }
      else if (mg_match(hm->uri, mg_str("/readfile/#"), match_captures))
      {
        auto path_c = mg_str_c_decode(match_captures[0]);

        //MG_STR_C(match_captures[0],path_c);
        //struct mg_fd *fd = mg_fs_open(&mg_fs_posix, path_c, MG_FS_READ);
        
        /*
        printf("JSON BEGIN\n");
        for(int i = 0; i<(ARRAY_SIZE(match_captures)); i++){
          if(match_captures[i].len<1) break;
          printf("%ld\n",match_captures[i].len);
          printf("%ld\n",(size_t)match_captures[i].ptr);
          printf("%.*s\n",(int)match_captures[i].len,match_captures[i].ptr);
        }
        printf("JSON END\n");
        */
        struct mg_http_serve_opts opts = {
          //.mime_types = "png=image/png",
        //  .extra_headers = "AA: bb\r\nCC: dd\r\n"
        };
        mg_http_serve_file(c, hm, path_c.ptr, &opts);
          
          printf("%.*s\n",(int)path_c.len,path_c);
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
      else if (mg_match(hm->uri, mg_str("/ls"), match_captures))
      {
        auto path_c = mg_str_c_decode(hm->uri);

        //MG_STR_C(match_captures[0],path_c);
        //struct mg_fd *fd = mg_fs_open(&mg_fs_posix, path_c, MG_FS_WRITE);
        

          
        printf("WRITEFILE: %.*s\n",(int)path_c.len,path_c);
        mg_http_reply(c, 200, "","");
        
      }
      else if (mg_match(hm->uri, mg_str("/writefile:#/#"), match_captures))
      {
        auto path_c = mg_str_c_decode(match_captures[1]);


        //MG_STR_C(match_captures[0],path_c);
        //struct mg_fd *fd = mg_fs_open(&mg_fs_posix, path_c, MG_FS_WRITE);
        

        mg_file_write(&mg_fs_posix, path_c.ptr, hm->body.ptr, hm->body.len);

          
        printf("WRITEFILE OPTS: %.*s\n",(int)path_c.len,path_c);
        mg_http_reply(c, 200, "","");
      }
      else if (mg_http_match_uri(hm, "/json/"))
      {
          // Use mg_http_reply() API function to generate JSON response. It adds a
          // Content-Length header automatically. In the response, we show
          // the requested URI and HTTP body:
          mg_http_reply(c, 200, "", "{%m:%m,%m:%m,%m:%d}\n",  // See mg_snprintf doc
                          MG_ESC("uri"), mg_print_esc, hm->uri.len, hm->uri.ptr,
                          MG_ESC("body"), mg_print_esc, hm->body.len, hm->body.ptr,
                          MG_ESC("time"), std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()
                          );
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
  }
  
}

int main()
{
  // Rest of your initialisation code ...

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);        // Init manager
  mg_log_set(MG_LL_DEBUG);  // Set debug log level. Default is MG_LL_INFO
  mg_http_listen(&mgr, "http://0.0.0.0:" port, fn, NULL);  // Setup listener
  for (;;) mg_mgr_poll(&mgr, 500);                       // Infinite event loop

  return 0;
}
