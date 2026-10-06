# libs — 预编译静态库

```
libs/lib/<Debug|Release>/x64/
  *.lib     MSVC
  *.a       MinGW
```

工程只链接本目录预编译库，头文件在 `SDK/inc` 与 `bridge/`。
