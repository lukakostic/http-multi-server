HTTP multi server - allows you to execute shell commands, write or read files, list files...

by default runs on localhost:8742

See commands section.

# Build

Execute ```./build.sh```

# Commands

## GET

```/ls/<path>```
List Files in path

```/ls//data/test``` would list all files & folders in "/data/test"


```/readfile/<path>```

read file data (any file type), returned as appropriate mime type in response.


```/shell/<command>```

execute shell command

```/shell/ls /data``` would execute ```ls /data```

## POST

```/writefile/<path>```

write request body to specified file