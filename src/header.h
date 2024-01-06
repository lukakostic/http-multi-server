#include "depends/mongoose.h"
#include "depends/pstream.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/inotify.h>
namespace fs = std::filesystem;


#define ARRAY_SIZE(FOO2) (sizeof(FOO2)/sizeof(FOO2[0]))

static void start_thread(void *(*f)(void *), void *p) {
#ifdef _WIN32
  _beginthread((void(__cdecl *)(void *)) f, 0, p);
#else
#define closesocket(x) close(x)
#include <pthread.h>
  pthread_t thread_id = (pthread_t) 0;
  pthread_attr_t attr;
  (void) pthread_attr_init(&attr);
  (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread_id, &attr, f, p);
  pthread_attr_destroy(&attr);
#endif
}

struct thread_data {
  struct mg_mgr *mgr;
  mg_http_message hm;
  unsigned long conn_id;
  char method; // G = GET , P = POST, W = WebSocket
  mg_str body;
  mg_str url;

  enum ResponseType {
    NotDone,
    Text, //plaintext or json response
    Upload, //user is uploading file
    Download, //user is downloading file
    Writefile,
    WritefileAppend,
  } response;

  ~thread_data(){
    if(body.ptr!=nullptr) delete body.ptr;
    if(url.ptr!=nullptr) delete url.ptr;
  }
};
int minI(int a, int b){return ((a<b)?a:b);}



struct proccess{
  bool running = true;
  int exitCode,errorCode;
  redi::pstream proc;//(commands.ptr, redi::pstreams::pstdout | redi::pstreams::pstderr);
  std::ostringstream ssOut,ssErr;

  proccess(char* cmd, std::_Ios_Openmode io)
  :running(true),proc(cmd, io){
    running = true;

    // read child's stdout
    ssOut << proc.out().rdbuf();
    /*
    while (std::getline(proc.out(), line))
      ssOut<<line;
      */
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
      proc.clear();
    // read child's stderr
    ssErr << proc.out().rdbuf();

    proc.close();   
    exitCode = proc.out().rdbuf()->status();
    errorCode = proc.out().rdbuf()->error();
    running = false;
  }
};

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
  //printf("mg_str_c [%.*s]\n",str.len,str.ptr);
  if(str.len==0) return {0,0};
  if(str.ptr[str.len-1] == 0) return str;
  mg_str cstr = {
    new char[str.len+1],
    str.len
  };
  strncpy((char*)cstr.ptr, str.ptr, str.len);
  ((char*)cstr.ptr)[cstr.len] = '\0';
  return cstr;
};
