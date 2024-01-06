HTTP multi server - allows you to execute shell commands, write or read files, list files...  

by default runs on localhost:8742  

See commands section.  

# Build

Execute ```./build.sh```

Then you can run ```./server```

# Commands

## GET

```/ls/<path>```

List Files in path

eg: ```/ls//data/test``` 

would list all files & folders in "/data/test"

```/ls-d/<path>```  list only folders  
```/ls-f/<path>``` list only files  

```/readfile/<path>```

read file data (any file type), returned as appropriate mime type in response.

```/shell/<command>```

execute shell command

eg: ```/shell/ls /data``` 

would execute ```ls /data```

## POST

```/writefile/<path>```

write request body to specified file

```/bash```

Executes request body as bash (.sh) script.  
Globbing and everything else works, unlike ```/shell```

```/shell```

Executes request body as (one line) shell command.