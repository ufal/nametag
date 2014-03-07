%module nametag_java

%include "../common/nametag.i"

%pragma(java) jniclasscode=%{
  static {
    System.loadLibrary("nametag_java");
  }
%}
